# Handoff: Dead or Alive 4 360 (2026-10-29)

Read [PLAN.md](PLAN.md) for the goals, decisions, milestones and the lessons from the sister
projects, and [FINDINGS.md](FINDINGS.md) for the disc and executable facts. This file is the
state, the build/run detail and where things live. Update it at the end of every session; the
next model starts here, not from chat history.

## Where it stands
**M0 in progress (2026-10-29): bootstrapped, nothing translated or built yet.**
- The repo is `../lotr-conquest-360-recomp`'s shape renamed (`lotrc` -> `doa4`, `LotrConquestApp`
  -> `Doa4App`, `LOTRC_` cvars -> `DOA4_`, the XEX hash and title); LOTR's autopilot scripts
  and game-specific docs were not copied. `tools/audit_repo.py` and `.gitignore` know DOA4's
  formats (`.afs`, `.sfd`, `.xgs`, `.dic`, `.tpr`, `.mot`, `.ptc`; the MPEG program-stream
  signature). `tools/test_audit_repo.py`: 4 passed.
- The disc is extracted: `port/assets` (6.8 GB, 59 files), `default.xex` SHA-256 as in
  FINDINGS.md. The decrypted image and import list were made in a scratch folder; redo them
  into `private/xex/` with the `xexdump` line in FINDINGS.md when analysis starts.
- `.coderabbit.yaml` written (from `../hunter-xbe-recomp`'s, with this repo's paths).
- `port/doa4_manifest.toml` is the bare scaffold: **no function hints, no `setjmp`/`longjmp`,
  no fiber hooks, no `Doa4FrameMark` hook yet** (its comment lists what to find).
  `src/frame_timing.cpp` compiles but is never called until the VdSwap `midasm_hook` exists.
- GitHub: https://github.com/Juansolo457/doa4-360-recomp, **public**, created 2026-10-29 with
  the bootstrap commit on `master`; `master` locked as the sister repos (PR only, `audit`
  required and strict, conversations resolved, admins included, no force push or delete); merge
  commits kept, branches deleted on merge. The first `audit` run passed. CodeRabbit covers the
  repo (the account's GitHub App installation lists it); PR #1 is its first review here.
- **Not done:** codegen and the first build (need the user's OK). See "Next steps".

## Next steps
1. **Codegen round 1** (`tools/build.ps1`, with the user's OK): expect `b` targets "not in any
   function" (leaf thunks) and possibly byte-indexed jump tables. After it:
   `check_switch_tables.py` (the shader compiler is in this image), `find_missed_functions.py`
   wholesale, `find_vcall_thunks.py`; drop any candidate that yields an "Unresolved" warning.
2. **Before the first run:** find the MS CRT `setjmp`/`longjmp` (the layout is in the manifest
   comment) and the five XAPI fiber routines (DOAX2 identified them by their error codes 1280 and
   1281 and the 0xA50 context at KTHREAD+0x164); bind both in the manifest. Find the game's
   `VdSwap` call site for `Doa4FrameMark`.
3. **First smoke run** (`tools/smoke.ps1 -Seconds 60 -CaptureEvery 5`, headless, the user's OK):
   expect the warning screen, the Sofdec intro, the title. Then `tools/autopilot/boot.txt` with
   the real timings filled in.
4. Every later change on a branch and through a PR (CONTRIBUTING.md); CodeRabbit reviews it.

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
