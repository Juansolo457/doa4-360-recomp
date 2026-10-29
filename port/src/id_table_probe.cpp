// DOA4 360 - diagnostic for run 2's crash (docs/HANDOFF.md, "Run 2").
//
// sub_82372840 (per-player state, 580 bytes each at 0x8302BC98) looks an entry up by the ID in
// its field 4: IDs 0x2000-0x2FFF use the table at 0x82CE88F0 (index ID - 0x2000), others the one
// at 0x82CE86F8 (index ID). Each table is {count, pointer to an array of entry pointers}; the
// bound check is a signed compare, so a negative index passes it. Run 2 died on the entry's first
// load with the entry = 0x42C80000 (100.0f), on the Now Loading screen into the first fight.
//
// Doa4ProbeIdTable runs just after `lwzx r8,r3,r4` (0x823729BC) and logs every entry that is not
// zero and not a guest pointer into the image, with the ID, index, count and player slot, so the
// next run says whether the index was out of range or the table itself was overwritten. It logs
// nothing while entries are sane.

#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/stack.h>
#include <rex/types.h>

namespace {
std::atomic<uint32_t> g_reports{0};
}  // namespace

void Doa4ProbeIdTable(PPCRegister& r4, PPCRegister& r5, PPCRegister& r6, PPCRegister& r8,
                      PPCRegister& r11, PPCRegister& r28) {
  const uint32_t entry = r8.u32;
  if (entry == 0 || (entry >= 0x82000000u && entry < 0x83600000u)) return;
  if (g_reports.fetch_add(1) >= 32) return;
  REXLOG_ERROR(
      "DOA4 probe: id table entry 0x{:08X} is not a pointer: id 0x{:08X} ({}), index {}, "
      "count {}, array 0x{:08X}, player {}",
      entry, r11.u32, r11.s32, r5.s32, r6.s32, r4.u32, r28.u32);
}

// Run 3 (2026-10-29) showed the ID is -256 (0xFFFFFF00) with the table intact. The only setter
// that takes a caller's ID, sub_82373680, is reached through sub_82372CB0, and its 7 call sites
// all pass a sign-extended halfword, so 0xFF00 came from game data. Doa4ProbeIdSource runs at
// sub_82372CB0's entry and logs every negative ID with the return address (the call site) and the
// caller's stack word 196(r1), which holds the halfword array in sub_8240CE58's two lhzx sites.
namespace {
std::atomic<uint32_t> g_source_reports{0};

uint32_t LoadGuestU32(uint32_t address) {
  const uint8_t* p = rex::ppc::detail::current_base() + address;
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
}  // namespace

void Doa4ProbeIdSource(PPCRegister& r1, PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                       PPCRegister& r6) {
  if (r4.s32 >= 0) return;
  if (g_source_reports.fetch_add(1) >= 32) return;
  const PPCContext& ctx = rex::ppc::detail::current_ctx();
  REXLOG_ERROR(
      "DOA4 probe: negative id 0x{:08X} ({}) set from lr 0x{:08X}: r3 {} r5 {} r6 {}, "
      "caller 196(r1) 0x{:08X}",
      r4.u32, r4.s32, uint32_t(ctx.lr), r3.s32, r5.s32, r6.s32, LoadGuestU32(r1.u32 + 196));
}
