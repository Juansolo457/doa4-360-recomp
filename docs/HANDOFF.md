# Handoff: Dead or Alive 4 360 (2026-10-29)

Read [PLAN.md](PLAN.md) for the goals, decisions, milestones and the lessons from the sister
projects, and [FINDINGS.md](FINDINGS.md) for the disc and executable facts. This file is the
state, the build/run detail and where things live. Update it at the end of every session; the
next model starts here, not from chat history.

## Where it stands
**M1 nearly there (2026-10-29): runs 2 and 3 reach the main menus, Sparring and character select at
the game's 60 fps, then crash loading a fight with Ryu Hayabusa; run 4 played a full fight with Kasumi.** Run 3's probe showed
the cause is an out-of-range ID (-256); a second probe for where it comes from is in the
manifest, not yet built. M0 is done (below).

### Run 5 (2026-10-29, Ryu as player 1, 180 s; `port/logs/smoke-20261029-082927.log`)
Same crash (`crash-20261029-083044.txt`). **The call-site probe named the source:** lr
0x8240DB64, opcode 20 of the motion-script interpreter in `sub_8240CE58` (dispatch at
0x8240D390, a 160-entry jump table at 0x8240D3BC that the generated switch matches entry for
entry). Opcode 20 reads the halfword after it from the player's script stream and passes it,
sign-extended, as the ID: 0xFF00, so -256.
- **The original does the same lookup** (signed bound check, then `lwz` through the entry), so on
  the console this would read 0x42C80000 too. That address is in the 64K-page heap, which only
  `X_MEM_LARGE_PAGES` allocations fill; nothing in our logs lives there.
- **The byte pattern `00 14 FF 00` is common:** 362 hits in `mot.afs`, in every character's
  motion files, Kasumi's included (run 4 played fine). So a raw search can't tell real data from
  an interpreter that fell out of step with the stream.
- **Third probe (not built):** `Doa4ProbeScriptOp` at the dispatch keeps each player's last 32
  (pointer, opcode) pairs; `Doa4ProbeIdSource` dumps them and 96 bytes of the stream when a
  negative ID appears. Then find those bytes in `ryu.mot` (entry 13) or `m_ryu.mot` (14) and
  check whether the opcode boundaries line up.

### Run 4 (2026-10-29, call-site probe built; `port/logs/smoke-20261029-082038.log`)
**A full fight plays.** The user chose Watch mode with Kasumi and a random stage: the cherry
blossom bridge and the river stage, both fighters, health bars and hits, all drawn correctly at
60 fps, then back to character select. No probe line, no crash. The user then picked Ryu
Hayabusa; the 120 s limit closed the game while that fight loaded (no crash report).
- **Ryu Hayabusa is the common factor:** player 0 was Ryu in run 2 (vs Christie) and run 3 (vs
  Hitomi, Las Vegas stage, crash as the fight opened). So the 0xFF00 halfword is most likely in
  Ryu's own data (a motion or sound event table), read through one of the 7 call sites.
- **Next run:** pick Ryu as player 1, smoke `-Seconds 180` so the load finishes.

### Run 3 (2026-10-29, ID-table probe built; `port/logs/smoke-20261029-080319.log`)
The user drove the pad into Sparring again. 60 fps throughout, then the same crash at +68 s on
Now Loading. **The probe answered:** the ID in player 0's field 4 is **0xFFFFFF00 (-256)**, index
-256 into the 8-entry table at 0x82CE86D8. The table is intact; the index is out of range.
- **Where the ID comes from:** `sub_82373680` is the setter (`stw r4,4(r11)`). Of its 3 callers,
  only `sub_82372CB0` passes a caller's ID; the other two pass 0. `sub_82372CB0` has 7 call sites
  (`sub_8241FFE0`, `sub_823802A0`, `sub_8240CE58` x3, `sub_823F9418`, `sub_823F9E48`), and every one
  sign-extends a halfword (`extsh`), as the original code does. So the value is the halfword 0xFF00
  from game data, or data we loaded or swapped wrong. Not yet known which.
- **Second probe (not built):** `Doa4ProbeIdSource` at `sub_82372CB0`'s entry logs every negative
  ID with the return address (which call site) and the caller's 196(r1), the halfword array of
  `sub_8240CE58`'s two `lhzx` sites. `lr` comes from the thread's guest context, since midasm
  hooks only take registers.

### Run 2 (2026-10-29, FPSCR hooks built; `port/logs/smoke-20261029-063849.log`)
Captures in `private/captures/doa4-20261029-063849/`, contact sheet `private/captures/run2-sheet.png`.
- **The FP crash is fixed:** no C000008F; the audio worker runs.
- **On screen, all drawn correctly:** the storm-sea intro (+10 s), clouds (+15 s), the sunrise
  over the peaks (+20 s), the **Mode Select** menu over the shrine stage with the mode icons
  and help text (+30-35 s), **Sparring character select** with the roster, Hayabusa and
  Christie in 3D (+40-50 s), then **Now Loading** (+55 s). No autopilot ran: the menu choices
  came from the connected Xbox Series X pad.
- **The game's own frame rate** (`Doa4FrameMark`): 60.0-60.3 fps, median 16.6-16.7 ms in every
  10 s window; three single long frames (1.2 s, 1.2 s, 0.54 s) at loads.
- **Crash at +58 s** on the main thread, on Now Loading: access violation reading guest
  0x42C80000 in `sub_82372840` (`crash-20261029-063947.txt`; stack: `sub_82371DC0`,
  `sub_8234F030`, `sub_82354DB0`, `sub_824F4878`, `sub_824F41E0`, `xstart`). 0x42C80000 is
  100.0f used as a pointer. The function takes a per-player object (580 bytes each at
  0x8302BC98), reads an ID from its field 4 and looks it up in `{count, array}` tables at
  0x82CE86F8 (IDs below 0x2000 and from 0x3000) or 0x82CE88F0 (0x2000-0x2FFF, index ID-0x2000),
  then loads two pointers from the entry. In the image both tables are 8 and 10 valid pointers,
  with no 100.0 just before either array; 100.0 words sit about 120 slots before, in
  neighbouring tables. The bound check is a signed compare, so a negative or wrong ID would read
  there; the other reading is that something overwrote the table at run time. Unproven.
- **Probe (built in run 3):** `src/id_table_probe.cpp`, a midasm hook just after the entry load
  (0x823729BC), logs every non-pointer entry with the ID, index, count, array and player slot.

### Run 1 (2026-10-29, the user's OK; `port/logs/smoke-20261029-063237.log`)
Captures in `private/captures/doa4-20261029-063237/`.
- **Vulkan works:** RTX 4090, the Xenos plugin's Vulkan backend, presents from +0.15 s at
  1280x720; `readback_resolve` 'none'. The Xbox Series X pad connects as player 1.
- **The fiber hooks work:** ConvertThreadToFiber on the main thread, then CreateFiber (start
  0x824F3310, 32 KB stack). No "unregistered function".
- **On screen:** the game's copyright / FEDERAL WARNING text, crisp and correct (+10 s).
- **Crash at +12.2 s:** C000008F (inexact) on the audio worker in `sub_8297D748`, under the
  worker's callback `sub_82968368`, MXCSR 0x8060: every FP exception unmasked
  (`crash-20261029-063249.txt`). The same as Quake 4's and GRAW's first boots: the runtime never
  seeds the audio worker's context.
- **Fix (manifest, not yet built):** `Doa4GuardFpscr` hooked at the callback's entry and before
  each of the image's six `mtfsf` (`src/fpscr_guard.cpp`, as Quake 4).
- Noise, none fatal: `D:\DOA4\` opens fail (devkit path); **`cache:\chrone.afs` probes** (52
  `cache:` lines): the game looks for a cached copy of its character archive on the cache
  partition (StfsCreateDevice is a stub, so there is none) and should fall back to the disc;
  watch for load-time cost later. `IoDismountVolumeByFileHandle` stub, devkit-memory warning.

### M0 (2026-10-29): `doa4.exe` builds clean

`port/out/build/win-amd64-relwithdebinfo/doa4.exe` (52 MB) with `rexgpu-xenosrd.dll` and
`rexruntimerd.dll` beside it, byte-identical to the Vulkan-only install's (SHA-256 checked). The
renderer DLL imports no Direct3D 12 (its import table is the C runtime, Tracy and the runtime
DLL; Vulkan is loaded at run time).

Verified on the translation (final codegen, build 5):
- codegen: no errors or warnings;
- `check_switch_tables.py`: 826 switches, 5 flagged, all benign (FINDINGS.md "Switches"); 2
  more were a code generator bug, fixed by overrides;
- `check_vector_aliasing.py`: no vpkd3d128 hazards in 240,454 vector instructions;
- `find_missed_functions.py`: at a fixed point (only the four dropped candidates remain);
- `find_vcall_thunks.py`: 2 matches, both false positives (FINDINGS.md).

Manifest rounds (`port/doa4_manifest.toml`; builds in `private/build-1..5.log`):
1. three `b` targets "not in any function", each a function start right after the previous
   function's last branch (0x8296EAC8, 0x82450CB0, 0x829CCF50);
2. `doa4.exe` linked; one warning, "Unresolved b target 0x82793F80 from 0x827925E8": the fiber
   context save/load routine behind SwitchToFiber's thunk, as in DOAX2's round 2 (its
   0x82785670). Added with it, before any run: `setjmp_address`/`longjmp_address` (0x82A04560 /
   0x82A04220, matched against DOAX2's pair over 24 instructions with immediates masked), the
   five XAPI fiber routines in `[entrypoint.rexcrt]` (FINDINGS.md), and the `Doa4FrameMark`
   midasm hook on the one VdSwap call (0x8275B0BC);
3. clean; then the finder's 96 candidates wholesale and two switch overrides. Four candidates
   split functions ("Unresolved" warnings) and were dropped (0x82330258, 0x827D5708, 0x829400E8,
   0x82A0FC34); build 5 clean.

**A code generator bug, new in this family** (FINDINGS.md "Switches"): where the index is a
sign-extended halfword loaded from a table into r11, the generated switch reads r3, the value the
table offset was built from. Two switches (sub_8251D328, sub_8251DA10), both overridden. Worth an
upstream issue to ReXGlue with the two sites as examples.

`find_vcall_thunks.py` had Quake 4's `.text` range hard-coded (0x82150000, 0x813604), unchanged
through lotr-conquest-360-recomp; it now carries DOA4's (0x822F0000, 0x86EDFC). **LOTR's "0
forwarders" ran on the wrong range** (it skipped the first 640 KB of LOTR's code and read past
its end). Rerun on LOTR's own range (lotr-conquest-360-recomp PR #2, 2026-10-29): still 0.

Repository: https://github.com/Juansolo457/doa4-360-recomp, **public**, created 2026-10-29 with
the bootstrap commit on `master`. Branch protection read back from the GitHub API
(`repos/Juansolo457/doa4-360-recomp/branches/master/protection`, 2026-10-29): `audit` required
and strict, conversation resolution on, admins included, force pushes and deletion off. Merge
commits kept, branches deleted on merge. The first `audit` run on `master` passed. CodeRabbit
lists the repository in its dashboard (Repositories: doa4-360-recomp, Public) and reviewed PR #1.

Bootstrap notes: the repo is `../lotr-conquest-360-recomp`'s shape renamed (`lotrc` -> `doa4`,
`LotrConquestApp` -> `Doa4App`, `LOTRC_` cvars -> `DOA4_`, the XEX hash and title); LOTR's forced
`readback_resolve=fast` was taken out (DOA4 starts on the SDK default and logs it). The audit and
`.gitignore` know DOA4's formats (`.afs`, `.sfd`, `.xgs`, `.dic`, `.tpr`, `.mot`, `.ptc`, the MPEG
program-stream signature). The disc is extracted to `port/assets` (6.8 GB, 59 files); the
decrypted image, import names and `.pdata` list are in `private/xex/`.

## Next steps
1. **Rebuild with the script-dispatch probe and rerun with Ryu as player 1, 180 s** (needs the
   user's OK): the dump shows the opcodes before the bad one. If the stream is out of step, find
   the handler that consumed the wrong number of bytes (a translation bug); if it lines up with
   `ryu.mot`, the console must have 0x42C80000 mapped, so look at the game's allocations. Then
   fix it and take all three probes out.
2. An autopilot script into Sparring (`tools/autopilot/`), with the timings from run 2's
   captures, so a fight can be reached without a person at the pad.
3. An upstream issue for the halfword-index switch bug. (LOTR's `find_vcall_thunks.py` rerun is done: lotr-conquest-360-recomp PR #2, still 0 forwarders.)
4. Every change on a branch and through a PR (CONTRIBUTING.md); CodeRabbit reviews it.

## How it is built and run
```
pwsh -NoProfile -File tools/setup.ps1                     # once: toolchain + Vulkan-only SDK (~1 h)
pwsh -NoProfile -File tools/extract.ps1 -Iso 'D:\x360-recomps\DOA4\<the Rev 1 disc>.iso'
pwsh -NoProfile -File tools/build.ps1 [-Jobs 12]          # codegen when needed, then compile
pwsh -NoProfile -File tools/smoke.ps1 -Seconds 60         # boot check; its window shows on the user's screen
pwsh -NoProfile -File tools/launch.ps1 -Windowed          # a window on the user's screen: ask first
```
- **On this PC** `.tools/` is a junction to `../n3-360-recomp/reference/N3-Recomp/.tools` (LLVM
  20.1.8, CMake 3.31.8, Ninja 1.13.1) and `third_party/rexglue-sdk-bd833a2` is a junction to
  `../quake4-360-recomp/third_party/rexglue-sdk-bd833a2`: ReXGlue `bd833a2` with GRAW's patch
  set, Quake 4's and LOTR's `vexptefp-underflow` (tree `12884d9`, HEAD `cf0b7a0`), built
  Vulkan-only. `tools/setup.ps1` checks the tree and the install stamp `doa4-sdk-tree`, which
  was written by hand (2026-10-29) with the same tree hash since the install was built from it;
  LOTR's `lotrc-sdk-tree` sits beside it. Anyone else runs `setup.ps1` and gets their own copy.
  Quake 4's `setup.ps1` still expects tree `54ab593`: fix in passing when it is next touched.
- `build.ps1` enters a VS 2022 DevShell, checks the XEX hash, runs `doa4_codegen`, reconfigures
  once on the first run, then builds `port/out/build/win-amd64-relwithdebinfo/doa4.exe`.

## Analysis tools (all read-only on the game's files)
| | |
|---|---|
| `tools/xdvdfs.py <iso> list \| extract <dir> [names]` | XDVDFS reader; `tools/extract.ps1` wraps it |
| `tools/xexinfo.py <xex>` | XEX2 headers: title, build time, entry, image size, encryption, XDK libraries |
| `..\quake4-definitive\build\xexdump\xexdump.exe <xex> private\xex\default` | the decrypted image (`default.bin`), sections and the import names |
| `tools/xex_basic_decrypt.py <xex> <out.bin>` | the same without xexdump, given the retail key (`DOA4_XEX_KEY`) |
| `tools/xeximports.py <xex> <image.bin> <sdk src>` | kernel/XAM imports against the SDK's export tables |
| `tools/check_switch_tables.py <image.bin> port/generated/default [--toml out.toml]` | replays every generated `switch` against the image; rerun after every codegen |
| `tools/find_missed_functions.py <image.bin> port/generated/default` | function entries the analyser missed, as manifest lines |
| `tools/check_vector_aliasing.py port/generated/default` | the vpkd3d128 hazard; `build.ps1` runs it |
| `tools/find_vcall_thunks.py <image.bin> port/generated/default <pdata_funcs.txt>` | virtual-call forwarders with no function-table entry or static reference |
| `tools/scan_d3d.py port/generated/default` | functions that build PM4 packets (only if a native renderer is attempted) |
| `tools/rdoc_capture.ps1 -Times 212,215` | a scripted run under RenderDoc (needs the `Doa4FrameMark` hook) |
| `tools/rdoc_*.py` | run with `qrenderdoc --python` (RenderDoc 1.46 in `../graw360-recomp/private/tools/renderdoc/`) |
| `--doa4_audio_dump <file>` + `tools/audio_report.py`, `tools/audio_6ch_report.py` | the audio output as raw float PCM with per-second levels, clipping, submit-rate gaps |

## Where things live
| | |
|---|---|
| `port/assets/` | the extracted disc (ignored) |
| `port/doa4_manifest.toml` | function hints and hooks for the code generator |
| `port/src/` | host code: `doa4_app.h` (Vulkan plugin by name, paths, controllers), `crash_report`, `frame_timing`, `fpscr_guard`, `autopilot`, `ui_pad`, `ui_style`, `audio_dump`, `timer_resolution`, `cpu_check` |
| `tools/patches/` | the SDK patch set `setup.ps1` applies (shared with the sister repos) |
| `tools/autopilot/` | pad scripts for `smoke.ps1 -Autopilot` (`boot.txt`, timings unverified) |
| `private/` | local only: decrypted image (`xex/`), captures, logs, Deck packages |
| `docs/` | PLAN, HANDOFF (this), FINDINGS, SETUP |

## Known traps (from the sister projects; the full list is PLAN.md "Lessons applied")
- Jump tables on a 64 KB boundary and byte-indexed tables are misread by `bd833a2`.
- Fibers: translated code can't switch stacks; bind the XAPI routines to `rexcrt`.
- `longjmp` through translated code without the manifest entries corrupts the host stack.
- Renderer options only as `--name=value`.
- A pad press in a runtime dialog needs `SetNavCursorVisible(true)` (in `ui_pad.cpp` already).
- `readback_resolve` defaults to `none`; decide per feature, log the chosen value.
