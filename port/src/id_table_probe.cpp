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
