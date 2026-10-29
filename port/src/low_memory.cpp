// DOA4 360 - keep the bottom of guest memory free, as on the console (docs/FINDINGS.md, "Ryu
// Hayabusa's unrelocated scripts").
//
// The runtime's 4K-page heap allocates bottom-up from 0x10000, so the first file the game loads
// lands there: that was Ryu Hayabusa's animation scripts (anm.afs, ryu.bin, 0x21504 bytes). His
// motion-script table (header section 21, 1024 file offsets) then stayed raw: run 8's trace shows
// slot 0 running scripts at 0x105D8, 0x1090C, 0x10968, the table's raw entries, so the
// interpreter ran his header and record data (an ID of -256 and a crash, the select-screen warp,
// an AI that never attacks). The likely reason is a relocation that skips values that already
// look like pointers (at or above the file's base); with the base at 0x10000 his offsets
// 0x10000-0x21504 do. On the console nothing the game loads sits that low.
//
// Doa4ReserveLowMemory reserves 0x10000-0x1000000 (no access) before the game starts, so the
// game's allocations begin at 16 MB. Run 9 (2026-10-29): Ryu's scripts start at 0x0101xxxx with
// real opcodes, his select sequence matches the real game, and he fights normally.
// --doa4_reserve_low_memory=false turns it off for A/B runs.

#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

// Read once, before the game's first instruction: a change takes effect at the next start.
REXCVAR_DEFINE_BOOL(doa4_reserve_low_memory, true, "DOA4",
                    "Reserve guest 0x10000-0x1000000 so game allocations start at 16 MB")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

void Doa4ReserveLowMemory() {
  if (!REXCVAR_GET(doa4_reserve_low_memory)) {
    REXLOG_INFO("DOA4: low guest memory not reserved (--doa4_reserve_low_memory=false)");
    return;
  }
  constexpr uint32_t kBase = 0x00010000, kEnd = 0x01000000;
  auto* memory = rex::system::kernel_memory();
  auto* heap = memory ? memory->LookupHeap(kBase) : nullptr;
  const bool ok = heap && heap->AllocFixed(kBase, kEnd - kBase, 0x1000,
                                           rex::memory::kMemoryAllocationReserve,
                                           rex::memory::kMemoryProtectNoAccess);
  if (ok) {
    REXLOG_INFO("DOA4: reserved guest 0x{:08X}-0x{:08X}; game allocations start above it", kBase,
                kEnd);
  } else {
    REXLOG_ERROR("DOA4: could not reserve guest 0x{:08X}-0x{:08X} (already in use?)", kBase, kEnd);
  }
}
