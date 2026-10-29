# Plan: Dead or Alive 4 (Xbox 360), statically recompiled

Written 2026-10-29 from the user's brief and a first look at the disc. Once the repo exists, the
state lives in `HANDOFF.md` and the disc/executable facts in `FINDINGS.md` (the sister repos'
shape). This file holds the goals, the decisions, the milestones and everything the earlier
recomps taught that applies here.

## Goal
The Xbox 360 **Dead or Alive 4** (Team Ninja / Tecmo, 2005; the Rev 1 USA/Europe disc) running
natively on **Windows first, Linux (the Steam Deck, native, no Proton) later**, its PowerPC code
translated to x86-64 with ReXGlue and drawn with **Vulkan**. The console game as it was: its
menus, its 60 Hz fighting, its controller feel, at any resolution, with the local modes complete
(Story, Time Attack, Survival, Versus, Team Battle, Sparring, Watch, Theater). Xbox Live play is
out of scope; the menus must cope with "no connection" cleanly.

Order of work: **the PC (Windows) build first, native Linux after, both Vulkan only.**

## Decisions (the user, 2026-10-29)
- **Vulkan is the only render API.** No D3D12 backend, option or fallback. The SDK is built from
  source with `REXGLUE_USE_D3D12=OFF` (DOAX2's recipe) and the host loads the Xenos plugin's
  Vulkan backend by name (`LoadGpuPlugin("xenos", "vulkan")`).
- **CodeRabbit reviews every pull request.** `master` is locked like the sister repos: every
  change through a PR, the `audit` check, an up-to-date branch, every review thread resolved,
  admins included. CodeRabbit gets a `.coderabbit.yaml` from day one (section "Review").
- **PC first, Linux later.** The native Linux/Deck build is a milestone of its own (M5), after
  the Windows build plays well. A Proton test package on the Deck may come earlier as a
  cheap preview, as LOTR and DOAX2 did; it is not the Linux deliverable.
- **Source only.** No release ever carries translated game code, game files or keys. Players
  build from their own disc (SETUP.md). The repository is **public** from day one.
- **No builds, codegen or game windows without the user's OK.** They test other games on this
  PC. Never close or kill a game they started.
- **Use what the sister repos learned.** The template is `../lotr-conquest-360-recomp`
  (the newest shape: AGENTS.md, scripts, host code, SDK patch set, CI); the engine knowledge is
  `../doax2-360-recomp` (same studio, same engine family, same middleware).

## What the disc and the executable are (2026-10-29, read-only look)
Nothing about where the copy came from is recorded, here or anywhere.

| | |
|---|---|
| Disc | `Dead or Alive 4 (USA, Europe) (En,Ja,Fr,De,Es,It,Zh,Ko) (Rev 1)`, Redump-verified; ISO 7,834,892,288 bytes at `D:\x360-recomps\DOA4\` (read-only) |
| Game partition | at `0xFD90000`: **59 files, 6.73 GiB** (`tools/xdvdfs.py <iso> list`) |
| `default.xex` | 16,465,920 bytes; title **`544307D1`**, media `606FFE2A`, version **0.0.0.9** (the Rev 1 title update baked in), built **2006-02-14 15:43 UTC**; encrypted (retail key), basic compression; image base `0x82000000`, size `0x1600000` (decrypted 23,068,672 bytes), entry `0x827929B0`, default stack 256 KB; original base `0x00400000` |
| XDK | **2.0.2135** (XAPILIB .4, D3DX9, XGRAPHC .3, XBOXKRNL, XNET, XONLINE, XAUD, XACT, LIBCPMT, XHV, XMP, X3DAUD, D3D9LTCG). **The oldest XDK of any of our recomps** (GRAW 2417, F.E.A.R. and DOAX2 3529, LOTR 7645) |
| Sections | `.rdata` 0x82000400 (2.8 MB), `.pdata` 0x822C6A00 (0x1D708: ~15,000 function entries), `.text` 0x822F0000 (8.4 MB), `PSFD00` 0x82B5EE00 (code, 23 KB: CRI Sofdec's own section), `.data` 0x82B70000 (10.2 MB), `.XEXID`, `.idata`, `.XBLD`, `.reloc` |
| Imports | checked against ReXGlue `bd833a2`'s export tables (`tools/xeximports.py`): **15 without an implementation, all data exports or the `sprintf` family** (`KeTimeStampBundle`, `VdGlobalDevice`, `ExLoadedCommandLine`, `XexExecutableModuleHandle`, `KeCertMonitorData`, ...), the identical set DOAX2 had. Nothing on the boot path |
| Disc layout | `default.xex`; CRI **AFS archives** `chrone/chrtwo/chrthree.afs` (characters, 1.6 GB), `stg.afs` (stages, 815 MB), `spr.afs`, `avt.afs`, `mot.afs`, `anm.afs`, `cmn.afs`, `snd.afs`; `SOUND/` **XACT** (`doa4_sound.xgs`, `bgm_stream.xwb` 195 MB, 23 per-character `voice_*_stream.xwb`); `movie/*.sfd` **Sofdec** endings per character (3.9 GB, `helena.sfd` alone 755 MB) plus `doa4_usa_cm.sfd` and `ninja_vi_hd.sfd`; `dic/ximej*.dic` (XIME) |
| Middleware (strings) | CRI libraries dated Oct/Nov 2005 (SFD/MPV/MPS/DCT/CRS/ERR/DLG...: Sofdec video, ADX threads with a `vsync` thread), **libjpeg** (`photodata.ptc` lives here too: the engine's photo code, shared with DOAX2), the **Xbox 360 shader assembler and D3DX compiler** in the image (some shaders are assembled at run time, as DOAX2 and GRAW), MSVC RTTI, the devkit path `D:\DOA4\` (every `D:\Sound\...xwb` string points at a devkit build; the disc has `SOUND/`) |
| Online | Xbox Live everywhere: session strings ("Currently in a session" in 8 languages), the `xbos`/`xuacs` content and subscription endpoints, XHV voice, achievements. The SDK stubs `NetDll_XNet*`; the menus must handle sign-in failure |
| Emulator state | Xenia Canary [game-compatibility #207](https://github.com/xenia-canary/game-compatibility/issues/207): `state-playable`. No published static recompilation of this title found |

The DOAX2 executable is the same engine eight months later (title `544307D2`, XDK 3529): the same
AFS/XACT/Sofdec/XIME/libjpeg stack, the same devkit path style, the same run-time shader
compile. Expect its traps, in the same order (section "Lessons").

## Starting points
| Source | What we take |
|---|---|
| `../lotr-conquest-360-recomp` | the repo shape: `AGENTS.md` + `CLAUDE.md` import, `CONTRIBUTING.md`, `.github/workflows/audit.yml`, `tools/` (setup, extract, build, smoke, launch, audit, codegen checks, RenderDoc scripts, audio dump), `port/src` (app, crash reporter, FPSCR guard, frame timing, autopilot, pad bridge, timer resolution), `docs/PLAN|HANDOFF|FINDINGS|SETUP` |
| `../doax2-360-recomp` | the engine: `[entrypoint.rexcrt]` fiber hooks, `setjmp/longjmp` in the manifest, the A8-resolve, stencil-bit and dump-wrap SDK patches, `--gpu_probe_textures`, `gpu_frame_stats`, the hang watchdog and `-StackDump`, `gpu_compare.ps1`, autopilot scripts through Team Ninja menus, the XMP title-music patch, the Deck profile recipe |
| `../graw360-recomp` | the SDK patch set's origin, the Linux/Deck container build (podman, Ubuntu 22.04), the native renderer (`native/`) if measurements ever call for it, `tools/sample_profile.py` and PERF.md |
| `../fear360-recomp` | the most played-in port: `docs/LINUX_NATIVE.md`, Deck optimisation history (Release build, register options, PGO), the smoke/release checklists |
| `../hunter-xbe-recomp/.coderabbit.yaml` | the only CodeRabbit config in the family; the model for ours |
| `../quake4-360-recomp/third_party/rexglue-sdk-bd833a2` | the shared SDK checkout (junction): ReXGlue `bd833a2` + GRAW's patches + Quake 4's + LOTR's `vexptefp-underflow`, tree `cf0b7a0`, built Vulkan-only |

## Architecture
```
default.xex (your disc) --rexglue codegen--> port/generated/default   (C++, one function per guest function)
                                                    |
port/src        host: Doa4App (Vulkan plugin by name, paths, controllers), crash reports, hang
                watchdog, frame timing (VdSwap hook), FPSCR guard, autopilot, pad bridge
                                                    |
ReXGlue SDK bd833a2 + tools/patches, built from source, REXGLUE_USE_D3D12=OFF:
  runtime  (kernel/XAM, VFS over port/assets, threads + fibers (rexcrt), XAudio/XMA, SDL3 window and pads)
  Xenos plugin, Vulkan backend: emulates the console GPU from the game's command buffers
    (host render targets "fbo" by default; "fsi" exact EDRAM available for diagnosis)
```
The game is Team Ninja's in-house engine with CRI middleware (Sofdec video, ADX), XACT audio,
XIME text input and libjpeg. There is no Bink, Wwise, Havok or Scaleform here.

## Milestones
| # | Milestone | Done when |
|---|---|---|
| M0 | **Repo, toolchain, first translation** | Repo on GitHub, locked, CodeRabbit on, `audit` green; the LOTR shape copied and renamed (`doa4`, `Doa4App`, `DOA4_XEX_KEY`, hashes); Vulkan-only SDK linked through the shared junction (tree `cf0b7a0`); disc extracted to `port/assets`; `FINDINGS.md` with the facts above and the XEX SHA-256; codegen clean: 0 wrong switch tables, no vpkd3d128 hazards, `find_missed_functions.py` at a fixed point, `find_vcall_thunks.py` 0; `doa4.exe` links. **No run yet** |
| M1 | **Boots to the main menu** | Warning screen, Sofdec intro, title, main menu drawn on the Vulkan backend; a controller moves through it; sign-in and storage prompts answered (headless and by pad); crash reporter, hang report and frame log from the first run; reference captures of each screen |
| M2 | **A fight plays** | Versus (2 local players or CPU) and one Story chapter from character select to the results screen with the ending movie; saves and the replay/Theater path work; the HUD, stage effects, shadows, water and cloth checked against console footage; first visual-bug pass |
| M3 | **Windows playable** | Every offline mode start to finish; Live menus fail gracefully; **a steady 60 fps at the game's own pacing** (frame CSV: p99 under 20 ms at 720p on the user's PC); resolution scale 1-3 with correct UI; the user's play-test with a real pad and fighting-game input feel checked (no added input latency over the console's) |
| M4 | **Source-only 0.1 release (Windows)** | SETUP.md verified by a clean-machine build (the `graw-setup-test` method); README, LICENSES, NOTICE; tag `v0.1.0`; the release checklist from F.E.A.R. |
| M5 | **Native Linux and the Deck** | GRAW's podman/Ubuntu 22.04 build of the SDK and the game, Vulkan on RADV; the package in `~/recomps/DOA4-360-SteamDeck/` (SSH `deck@steamdeck`); the user's Deck run: 60 fps in a 2-character fight at 720p or a measured list of what stops it |
| M6 | **Performance and extras** | Measured bottlenecks (per-draw CPU on the render thread, pipeline-compile hitches, GPU time per pass); Release build and PGO as F.E.A.R. did; shader pipeline pre-warm; decide on a native renderer only on numbers; 1.0 |

Each milestone is at least one pull request. M0 and M1 may share one, as LOTR's did.

## Lessons applied (what the sister repos paid for; do these before they bite)
**Translation (M0)**
- Run `check_switch_tables.py` after every codegen: ReXGlue `bd833a2` misreads any jump table on a
  64 KB boundary (GRAW's shader-compiler parser crash) and oversizes **byte-indexed** tables
  (LOTR's two). This image compiles shaders at run time through the same D3DX parser GRAW's
  crash was in.
- Name `setjmp_address` / `longjmp_address` in the manifest in M0, not after a crash: libjpeg's
  error exit `longjmp`s (DOAX2's photo-album crash). Find the MS CRT pair by its layout
  (f14-f31 at 0, r1 at 144, r13 onward from 152).
- Expect **XAPI fibers**: DOAX2 runs its game logic on fibers (`SwitchToFiber` from 114 sites)
  and crashed 2 s in until `[entrypoint.rexcrt]` bound ConvertThreadToFiber / CreateFiber /
  SwitchToFiber / DeleteFiber / ConvertFiberToThread to the SDK's hooks. Look for the five
  routines (ConvertThreadToFiber fails with 1280, ConvertFiberToThread with 1281; the 0xA50
  context at KTHREAD+0x164) before the first run and bind them if present.
- Add missed-function candidates wholesale, then drop any that produce an "Unresolved" codegen
  warning (it split a function). Keep landing pads inside `.pdata` functions that warn nothing.
- `tools/check_vector_aliasing.py` every build (the `vpkd3d128` hazard patch is in the tree).
- The SDK's **`vexptefp` underflow** (LOTR's blown-out audio) is already in the shared tree;
  XACT's own gain math may hit the same estimate. Keep `tools/audio_dump` to prove levels.

**First runs (M1)**
- Host **fbo** render targets by default; keep `--render_target_path_vulkan=fsi` for A/B
  (DOAX2's red band was an SDK stencil-transfer bug on NVIDIA, patched in the tree; FSI costs 3x
  GPU in 3D and is a diagnosis tool, not a default).
- Plugin options only as `--name=value`; the space form is dropped silently.
- `readback_resolve`: `none` is the SDK default; DOAX2 needed `some` for photos and LOTR `fast`
  for exposure. Decide on evidence (a feature that reads a resolve on the CPU), log the chosen
  value at start-up, and let the command line override by source, not value.
- The A8 (k_8 swap) resolve must store alpha (DOAX2's black shadow patches), the EDRAM dump must
  wrap (DOAX2's pool black screen): both patches are in the tree. The shadow mask here may be the
  same technique: check the first stage with `--gpu_probe_textures`.
- Harmless noise to expect and not chase: `cache:` has no device (StfsCreateDevice stub), the
  devkit `D:\DOA4\` opens, devkit-memory warnings, `ShaderDump` probes, `default.xexp` probe.
- `ui_pad.cpp` must call `ImGui::SetNavCursorVisible(true)` on a pad press (DOAX2's device
  selector fix) or A alone never confirms a dialog.
- Shared controllers by default (a virtual XInput pad takes slot 0 and the real pad becomes an
  unsigned player 2). **Versus needs two pads:** keep the one-controller-per-slot mode selectable
  and test it in M2; DOAX2 and LOTR never needed it.
- Crash reporter (`sub_<address>` frames), the hang report after N seconds, `smoke.ps1
  -StackDump` and the `--gpu_frame_stats` patch from the first run; a frame counter on the game's
  one `VdSwap` call as soon as it is found (the game's own fps, not the presenter's).
- Autopilot scripts with second-stamped pad presses (`tools/autopilot/*.txt`), `-FreshProfile`,
  a capture every N seconds, contact sheets. The intro timings drift with the host: say so in
  each script.

**Pacing (M3): the point of this game**
- DOA4 is a 60 Hz fighter. DOAX2 paced at 17 ms in menus and capped 30 in play; LOTR at 30. The
  `vblank-pacing` patch and `--video_mode_refresh_rate` are in the tree (F.E.A.R. doubled the
  virtual vblank for 60). First measure what the game waits on (VdSwap, vblank interrupt, the
  ring-buffer read pointer spin DOAX2 found at 27% of the game thread on the Deck), then keep
  the console's 60 exactly. No uncapping: fighting-game logic is frame-stepped.
- Measure input latency once at M3 (a frame-stamped button press to its first visible effect)
  so the port is never slower than the console.

**Repo and process**
- `python -I tools/audit_repo.py` before every commit; CI runs it over every commit
  (`--history`). Game files, decrypted images, generated code, caches, captures, logs, saves and
  keys never leave the PC; `private/` holds the local ones.
- `tools/setup.ps1` checks the SDK tree hash and install stamp; on this PC `.tools/` and
  `third_party/rexglue-sdk-bd833a2` are junctions to the sister repos' copies. Any new SDK patch
  goes into `tools/patches` here **and** is committed in the shared checkout, and every sister
  repo's `setup.ps1` expected hash moves with it (Quake 4's still expects `54ab593`: fix in
  passing).
- Commit as `juansolo457 <5344231+Juansolo457@users.noreply.github.com>`.
- `HANDOFF.md` updated at the end of every session; `FINDINGS.md` keyed by the XEX hash.

## Review: CodeRabbit
- `.coderabbit.yaml` at the root (from `../hunter-xbe-recomp`'s), profile `assertive`,
  auto-review on `master` PRs, drafts off, no poem, walkthrough collapsed; path filters exclude
  `port/generated/**`, `port/assets/**`, `private/**`, `**/out/**`.
- Path instructions: `port/src/**` (guest/host boundary: 32-bit guest pointers, cvar lifecycle,
  Vulkan-only invariant, nothing from the game pasted in beyond hook addresses);
  `port/doa4_manifest.toml` (every address with the round and the evidence that proved it);
  `tools/patches/**` (SDK patches must stay generic and upstreamable, list each in HANDOFF);
  `tools/**` (`python -I`, `audit_repo.py` and its tests guard the repo: loosening is a security
  review); `docs/**` (consistency with HANDOFF/FINDINGS, no promises of binaries).
- Process: fix what holds up, answer what doesn't with the reason, resolve the thread; a
  review-only PR (DOAX2's #2) when work landed on master before the lock, never again after.
- Enable the repo in the CodeRabbit dashboard right after creation (the GitHub App is installed
  on the account; private repos are covered, as `doax2` and `hunter` are).

## Risks
- **XDK 2.0.2135** predates everything ReXGlue has run here. Launch-era kernel and XAM calling
  patterns (early XamShowSigninUI flows, `XNotify` polling, early XAudio) may expose SDK stubs
  nobody has hit. Mitigation: the full import list is implemented; the trace log on the first
  run; Xenia's playable state says the surface is emulable.
- **Live-first menus.** The main menu's online entries and the sign-in nag at boot must fail
  closed. If the game insists on a profile, the SDK's local profile answers it (DOAX2/LOTR did).
- **Sofdec on the main thread + ADX vsync thread.** The CRI timeout string
  (`adxm_destroy_thrd (vsync)`) shows a thread that waits on vblank; a wrong vblank cadence
  stalls movie teardown. DOAX2's movies played first time; the per-character endings here are
  up to 755 MB and stream from `movie/`.
- **Run-time shader assembly** through the image's own D3DX: the GRAW jump-table bug class.
- **Two-player input and 60 Hz pacing** are new requirements for this family; neither sister
  port was held to them.
- **Cloth, hair, water and stage destruction** are the renderer's hard parts here (the DOAX2
  water and shadow fixes were all SDK bugs, found with RenderDoc + the `rdoc_*.py` scripts).

## Settled in review (the user, 2026-10-29, Plannotator)
- **Public repository** from day one: `Juansolo457/doa4-360-recomp`, locked master, source only.
- **Ask before every build and run** (the default rule stands; no standing OK for this repo).
- **A Proton test package on the Deck after M2** (a fight plays), before the native Linux build.
- Scope stays the offline game on this disc. Title updates, DLC costumes and Xbox Live play are
  not planned; revisit only if the user asks.
