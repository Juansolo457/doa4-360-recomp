// DOA4 360 - diagnostic for the select-screen fault with Ryu Hayabusa (docs/HANDOFF.md, "Run 7").
//
// On the select screen Ryu's camera holds the face close-up while his model animates out of the
// panel; on real hardware it cuts to a full-body shot after about 0.8 s, and Hayate's sequence
// is right in the port. This traces the per-player animation requests and the motion-script
// starts, stamped with the game frame, to compare Ryu with another character on the same panel:
// - Doa4TraceScriptOp, the interpreter dispatch in sub_8240CE58 (0x8240D390): logs a script
//   start (the stream pointer jumping) with its first 16 bytes, to find it in anm.afs.
// - Doa4TraceSetId, the ID setter sub_82373680: logs ID, r5, r6 and whether a request is pending.
// - Doa4TraceSetSpecial, sub_82372D78 (ID 0x6000 with parameters).
// - Doa4TraceGate, sub_82372840 after its first gate load (0x82372868): logs the gate bytes
//   0x82FCC8DF / 0x82FCC8DE when they change.
// - Doa4TraceApply, sub_82372840 at the stamp compare (0x823728D4): logs each applied or stale
//   request.
// Remove once the select-screen fault is understood.

#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/stack.h>
#include <rex/types.h>

extern std::atomic<uint32_t> g_doa4_frame_number;

namespace {
constexpr uint32_t kRecords = 0x8302BC98;
constexpr uint32_t kRecordSize = 580;
std::atomic<uint32_t> g_lines{0};

bool Budget() { return g_lines.fetch_add(1, std::memory_order_relaxed) < 40000; }
uint32_t Frame() { return g_doa4_frame_number.load(std::memory_order_relaxed); }

const uint8_t* Guest(uint32_t address) {
  return rex::ppc::detail::current_base() + address;
}
uint32_t LoadU32(uint32_t address) {
  const uint8_t* p = Guest(address);
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
int32_t RecordIndex(uint32_t record) {
  if (record < kRecords) return -1;
  const uint32_t off = record - kRecords;
  return off % kRecordSize == 0 ? int32_t(off / kRecordSize) : -1;
}

struct Stream {
  uint32_t last_ptr = 0;
  uint32_t last_start = 0;
};
Stream g_streams[8];
uint8_t g_gate[8][2];
bool g_gate_seen[8];
}  // namespace

void Doa4TraceScriptOp(PPCRegister& r4, PPCRegister& r29) {
  if (r29.u32 >= 8) return;
  Stream& s = g_streams[r29.u32];
  const uint32_t ptr = r4.u32;
  const bool contiguous = s.last_ptr && ptr > s.last_ptr && ptr <= s.last_ptr + 16;
  s.last_ptr = ptr;
  if (contiguous || ptr == s.last_start) return;
  s.last_start = ptr;
  if (!Budget()) return;
  REXLOG_INFO("DOA4 trace: f{} slot {} script 0x{:08X}: {:08X} {:08X} {:08X} {:08X}", Frame(),
              r29.u32, ptr, LoadU32(ptr), LoadU32(ptr + 4), LoadU32(ptr + 8), LoadU32(ptr + 12));
}

void Doa4TraceSetId(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5, PPCRegister& r6) {
  if (!Budget()) return;
  const uint32_t record = r3.u32;
  REXLOG_INFO(
      "DOA4 trace: f{} player {} set id {} (0x{:04X}) r5 {} r6 {}; pending {} stamp 0x{:08X} "
      "now 0x{:08X}",
      Frame(), RecordIndex(record), r4.s32, r4.u32 & 0xFFFF, r5.s32, r6.s32,
      LoadU32(record + 12), LoadU32(record + 0), LoadU32(0x830369F0));
}

void Doa4TraceSetSpecial(PPCRegister& r3, PPCRegister& r4, PPCRegister& r6) {
  if (!Budget()) return;
  REXLOG_INFO("DOA4 trace: f{} slot {} set special 0x6000 r4 0x{:08X} r6 {}", Frame(), r3.s32,
              r4.u32, r6.s32);
}

void Doa4TraceGate(PPCRegister& r10, PPCRegister& r28) {
  const uint32_t p = r28.u32;
  if (p >= 8) return;
  const uint8_t a = uint8_t(r10.u32), b = *Guest(0x82FCC8DE);
  if (g_gate_seen[p] && g_gate[p][0] == a && g_gate[p][1] == b) return;
  g_gate_seen[p] = true;
  g_gate[p][0] = a;
  g_gate[p][1] = b;
  if (!Budget()) return;
  REXLOG_INFO("DOA4 trace: f{} player {} gate C8DF {} C8DE 0x{:02X} (applies when 0 and &0x60)",
              Frame(), p, a, b);
}

void Doa4TraceApply(PPCRegister& r7, PPCRegister& r11, PPCRegister& r28, PPCRegister& r31) {
  if (!Budget()) return;
  const int32_t id = int32_t(LoadU32(r31.u32 + 4));
  REXLOG_INFO("DOA4 trace: f{} player {} {} id {} (0x{:04X}) stamp 0x{:08X} now 0x{:08X}", Frame(),
              r28.u32, r7.u32 == r11.u32 ? "apply" : "stale", id, uint32_t(id) & 0xFFFF, r7.u32,
              r11.u32);
}
