# Handoff: Dead or Alive 4 360 (2026-10-29)

Read [PLAN.md](PLAN.md) for the goals, decisions, milestones and the lessons from the sister
projects, and [FINDINGS.md](FINDINGS.md) for the disc and executable facts. This file is the
state, the build/run detail and where things live. Update it at the end of every session; the
next model starts here, not from chat history.

## Where it stands
**M0 done (2026-10-29): `doa4.exe` builds clean; it has never been run.** The first run needs
the user's OK (it opens a window on their screen).

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
its end): rerun there with LOTR's own range.

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
1. **First smoke run** (`tools/smoke.ps1 -Seconds 60 -CaptureEvery 5`, headless; the user's OK
   first): expect the warning screen, the Sofdec intro, the title. Watch for "Call to invalid or
   unregistered function" (a missed entry: the next manifest round), fiber activity
   (ConvertThreadToFiber / CreateFiber), FPSCR faults on the audio worker (`fpscr_guard.cpp` is
   linked but not hooked), and the frame log ("DOA4 frames: ..." every 10 s from `Doa4FrameMark`).
2. Then `tools/autopilot/boot.txt` with the real timings, into the main menu (M1).
3. An upstream issue for the halfword-index switch bug; LOTR's `find_vcall_thunks.py` rerun.
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
