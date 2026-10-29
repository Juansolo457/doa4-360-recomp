// DOA4 360 - guard for an out-of-bounds read in the original game (docs/FINDINGS.md, "The
// negative animation ID in Ryu Hayabusa's script").
//
// sub_82372840 applies a per-player request {stamp, ID, ...} (580-byte records at 0x8302BC98)
// set by opcode 20 of the motion-script interpreter (sub_8240CE58). For IDs outside the
// 0x2000/0x4000/0x6000 ranges it indexes the 8-entry table {count, array} at 0x82CE86F8 with a
// signed bound check only, then stores the entry's two words as pointers to two animation
// streams of that player (records +20 and +276). Ryu Hayabusa's script (anm.afs, ryu.bin) passes
// 0xFF00, so -256: the game reads the word 1 KB before the array (0x82CE82D8, the float 100.0 =
// 0x42C80000) and loads through it. Here that page is not committed and the load faults (runs 2,
// 3, 5 and 6 crashed loading a fight with Ryu).
//
// Not confirmed: why the console survives. 0x42C80000 is in the 64K-page range the game's XAPI
// heap allocates from (its NtAllocateVirtualMemory calls pass MEM_LARGE_PAGES | MEM_HEAP), so the
// likely answer is that the page is committed there and the read returns heap data. The streams
// may be Ryu's facial animation, hidden by his mask; also unconfirmed.
//
// Doa4GuardAnimId runs just after the bound check (0x823729B0) and, for a negative index, jumps to
// 0x82372B14, where the original code goes for an out-of-range ID or a null entry: the request is
// dropped and the player's previous streams stay.

#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/types.h>

namespace {
std::atomic<uint32_t> g_reports{0};
}  // namespace

bool Doa4GuardAnimId(PPCRegister& r5, PPCRegister& r28) {
  if (r5.s32 >= 0) return false;
  if (g_reports.fetch_add(1) < 4) {
    REXLOG_INFO("DOA4: dropped animation ID {} for player {} (the original reads past its table)",
                r5.s32, r28.u32);
  }
  return true;
}
