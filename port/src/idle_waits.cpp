// DOA4 360 - sleep instead of spinning in two of the game's waits (the Steam Deck's profile of a
// fight, docs/HANDOFF.md "Deck run 1" and "Deck run 2").
//
// On the Deck the CPU and GPU share ~15 W, and the frame rate waits on the GPU command thread.
// Two of the game's threads spun a whole core each while they waited, taking power from it:
//  - The job worker, sub_823FC5A0, loops for ever over a ring of 16 jobs at 0x830ADB08 (the index
//    of the slot it takes next, then 12-byte slots: a state byte, the function, its argument),
//    under the critical section at 0x830ADBCC. When the slot holds no posted job (state 1) it
//    takes and drops the lock and looks again. sub_823FC650 posts the jobs (loads, whose owners
//    look for state 3, done, once a frame). Doa4JobWorkerIdle, at the loop's back edge, sleeps
//    until Doa4JobPosted (after sub_823FC650 posts one) wakes it, or 2 ms.
//  - The game's Direct3D waits for the GPU: for a fence (sub_8275A0B8) or for room in the command
//    ring (sub_827598B0), polling the values the command processor writes back, through
//    sub_82761AB8 (which also gives up after 2 s without progress). Doa4GpuWaitBegin marks where
//    each wait starts; Doa4GpuWaitPoll, each time round the loop, spins for the wait's first
//    doa4_gpu_wait_spin_us and then sleeps doa4_gpu_wait_sleep_us a time (graw360-recomp's
//    GrawRingWaitYield sleeps 200 us flat). The command processor is the slower thread, so a wait
//    usually ends with more of its work queued behind the fence; the short spin keeps short waits
//    as quick as before.
// Every 10 s, after "DOA4 frames: ...", "DOA4 waits: ..." logs how many waits there were and how
// long they took (frame_timing.cpp).

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/stack.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <sys/prctl.h>
#endif

REXCVAR_DEFINE_BOOL(doa4_job_worker_sleep, true, "DOA4",
                    "The game's job worker sleeps until a job is posted instead of spinning on "
                    "its empty ring (false = spin as on the 360)")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

REXCVAR_DEFINE_INT32(doa4_gpu_wait_spin_us, 20, "DOA4",
                     "When the game waits for the GPU (a fence, or room in the command ring), "
                     "spin this long (microseconds) before sleeping")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

REXCVAR_DEFINE_INT32(doa4_gpu_wait_sleep_us, 100, "DOA4",
                     "Then sleep this long (microseconds) each time round the wait instead of "
                     "spinning; 0 = spin as on the 360")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace {

using Clock = std::chrono::steady_clock;

constexpr uint32_t kJobSlots = 16, kJobSlotSize = 12;
constexpr uint8_t kJobPosted = 1;

std::atomic<uint64_t> g_job_generation{0};  // one more for each posted job
std::atomic<uint32_t> g_job_sleepers{0};
std::mutex g_job_mutex;
std::condition_variable g_job_cond;

struct GpuWait {  // the calling thread's current wait
  Clock::time_point begin, last;
  bool polled = false;
};
thread_local GpuWait t_gpu_wait;

// Since the last "DOA4 waits" line.
std::atomic<uint32_t> g_gpu_waits{0}, g_gpu_sleeps{0}, g_jobs_posted{0}, g_job_sleeps{0};
std::atomic<int64_t> g_gpu_wait_us{0};

const uint8_t* GuestBase() {
  static const uint8_t* const base = rex::ppc::detail::current_base();
  return base;
}

uint32_t LoadGuestU32(uint32_t address) {
  const uint8_t* p = GuestBase() + address;
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}

#if defined(_WIN32)
void SleepUs(int32_t us) {
  // A high-resolution waitable timer sleeps for well under a millisecond (Sleep can't).
  thread_local HANDLE timer = CreateWaitableTimerExW(
      nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
  if (!timer) {
    Sleep(0);
    return;
  }
  LARGE_INTEGER due;
  due.QuadPart = -int64_t(us) * 10;  // relative, in 100 ns units
  if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE)) {
    WaitForSingleObject(timer, INFINITE);
  }
}
#else
void SleepUs(int32_t us) {
#if defined(__linux__)
  // A sleep may end up to the thread's timer slack late, 50 us by default: half as long again as
  // a 100 us sleep. 10 us still lets the kernel group wake-ups.
  thread_local const bool slack_set = prctl(PR_SET_TIMERSLACK, 10000UL, 0, 0, 0) == 0;
  (void)slack_set;
#endif
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}
#endif

}  // namespace

// After sub_823FC650 has posted a job and dropped the ring's lock.
void Doa4JobPosted() {
  g_jobs_posted.fetch_add(1, std::memory_order_relaxed);
  g_job_generation.fetch_add(1);
  if (g_job_sleepers.load() != 0) {
    std::lock_guard<std::mutex> lock(g_job_mutex);
    g_job_cond.notify_all();
  }
}

// At the job worker's back edge, with the ring's lock dropped; r30 = the ring.
void Doa4JobWorkerIdle(PPCRegister& r30) {
  if (!REXCVAR_GET(doa4_job_worker_sleep)) return;
  // Read before the slot: a job posted after this read changes it, so it isn't slept through.
  const uint64_t generation = g_job_generation.load();
  const uint32_t index = LoadGuestU32(r30.u32);
  if (index >= kJobSlots || GuestBase()[r30.u32 + 4 + index * kJobSlotSize] == kJobPosted) return;
  g_job_sleeps.fetch_add(1, std::memory_order_relaxed);
  g_job_sleepers.fetch_add(1);
  {
    std::unique_lock<std::mutex> lock(g_job_mutex);
    g_job_cond.wait_for(lock, std::chrono::milliseconds(2),
                        [generation] { return g_job_generation.load() != generation; });
  }
  g_job_sleepers.fetch_sub(1);
}

// Where a Direct3D wait for the GPU starts (before its first check).
void Doa4GpuWaitBegin() {
  GpuWait& wait = t_gpu_wait;
  if (wait.polled) {  // the thread's previous wait, now over
    g_gpu_wait_us.fetch_add(
        std::chrono::duration_cast<std::chrono::microseconds>(wait.last - wait.begin).count(),
        std::memory_order_relaxed);
    wait.polled = false;
  }
  wait.begin = Clock::now();
}

// Each time round the wait, before it polls the GPU's progress again.
void Doa4GpuWaitPoll() {
  GpuWait& wait = t_gpu_wait;
  if (!wait.polled) {
    wait.polled = true;
    g_gpu_waits.fetch_add(1, std::memory_order_relaxed);
  }
  wait.last = Clock::now();
  const int32_t sleep_us = REXCVAR_GET(doa4_gpu_wait_sleep_us);
  if (sleep_us <= 0 ||
      wait.last - wait.begin < std::chrono::microseconds(REXCVAR_GET(doa4_gpu_wait_spin_us))) {
    return;
  }
  SleepUs(sleep_us);
  g_gpu_sleeps.fetch_add(1, std::memory_order_relaxed);
  wait.last = Clock::now();
}

// From frame_timing.cpp's 10 s summary: the waits since the last one, over `frames` frames that
// took `window_s` seconds. With no frames (at the first frame) it only starts the count.
void Doa4LogIdleWaits(size_t frames, double window_s) {
  const uint32_t waits = g_gpu_waits.exchange(0), sleeps = g_gpu_sleeps.exchange(0);
  const uint32_t jobs = g_jobs_posted.exchange(0), job_sleeps = g_job_sleeps.exchange(0);
  const double waited_ms = g_gpu_wait_us.exchange(0) / 1000.0;
  if (frames == 0 || window_s <= 0) return;
  REXLOG_INFO(
      "DOA4 waits: for the GPU {} ({:.1f} a frame), {:.0f} ms in all ({:.2f} ms a frame, {:.0f}% "
      "of the time), {} sleeps; {} jobs posted, the job worker slept {} times",
      waits, double(waits) / frames, waited_ms, waited_ms / frames, waited_ms / (window_s * 10.0),
      sleeps, jobs, job_sleeps);
}
