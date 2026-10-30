# Handoff: Dead or Alive 4 360 (2026-10-29)

Read [PLAN.md](PLAN.md) for the goals, decisions, milestones and the lessons from the sister
projects, and [FINDINGS.md](FINDINGS.md) for the disc and executable facts. This file is the
state, the build/run detail and where things live. Update it at the end of every session; the
next model starts here, not from chat history.

## Where it stands
**M1 reached, M2 close (2026-10-29): menus, character select and full fights play at 60 fps,
every character tried so far, Ryu Hayabusa included since run 9** (his scripts were left
unrelocated because his file loaded at guest 0x10000; `src/low_memory.cpp` keeps that range
free). 4K internal (`-Scale 3`) also holds 60 fps (run 10). M0 is done (below). PR #1 (all of
it, 1440p Windows default) was reviewed by CodeRabbit and merged on 2026-10-29. **Now: the native
Linux build for the Steam Deck** (branch `linux-native`, below): the SDK and the game build in
WSL2 and boot natively to the title screen (Linux run 1); the Deck build is on the Deck, waiting
for the user's first run there.

### Run 10 (2026-10-29, 4K internal: `--resolution_scale=3 --anisotropic_override=5`; `port/logs/smoke-20261029-104915.log`)
First run of the build without the trace and guard, and the first at 4K: the renderer reports
"draw-scale swap sizing: packet=1280x720, src_scaled=3840x2160" (and its standard "Vulkan draw
resolution scaling is experimental" warning). The user played Sparring, Ryu against Leifang on
the helipad. **60.0 fps in every 10 s window**, no crash, no new errors (only the known `cache:`
and `StfsControlDevice` stubs). The captures (the 2560x1440 window, so 4K downsampled) are clean:
sharp HUD text and move list, no seams or half-pixel lines on models or stage, menus and select
screen correct. Two near-white frames fall on critical hits, most likely the game's hit flash;
not compared against scale 1. Real 4K output needs fullscreen on a 4K display
(`tools/launch.ps1 -Scale 3` without `-Windowed`).

### Run 9 (2026-10-29, low-memory reservation; `port/logs/smoke-20261029-104238.log`)
**Ryu Hayabusa is fixed.** "DOA4: reserved guest 0x00010000-0x01000000" at start; his scripts
start at 0x0101xxxx with real opcodes (`0002 000A 005A ...`, where run 8 read record data);
hovering him shows the face close-up, a cut to the full body, then the upper body with the sword,
as in the real game; in Watch he scores (71, 141, 240, 406, where runs 7-8 stayed at 0); the user
then played a match against Kasumi (story intro, rounds, KO, replays) and reported it "played
great". The guard never fired. 60.0 fps in every 10 s window, the game's 1280x720 shown in a
2560x1440 window. The trace and the guard are removed (FINDINGS.md corrects the -256 reading).

### Run 8 (2026-10-29, animation trace; `port/logs/smoke-20261029-095748.log`)
The user hovered Ryu, Kasumi, Hayate and others, then played Watch with Ryu against Kasumi, and
recorded it (NVIDIA capture `Desktop 2026.10.29 - 09.58.46.07.mp4`, not in the repo).
- **Ryu's select model warps:** it flattens to a vertical sliver and vanishes (recording 5.5-6 s
  and 26 s); every other character holds a normal close-up. **In the fight Ryu never lands a hit**
  (his score stays 0, Kasumi's reaches 180), as in run 7. The guard dropped -256 once.
- **The trace found the cause (FINDINGS.md, "Ryu Hayabusa's unrelocated scripts"):** Ryu's motion
  scripts are looked up in a table that was never relocated, so they run his file's header and
  record data. His file sits at guest 0x10000.
- **Experiment (not built):** `src/low_memory.cpp` reserves guest 0x10000-0x1000000 before the
  game starts, so nothing loads that low (as on the console). The trace and guard stay in to
  check it: expect slot 0's script starts above 16 MB, no "dropped animation ID", Ryu's select
  close-up intact and his AI fighting back.

### Run 7 (2026-10-29, animation-ID guard built; `port/logs/smoke-20261029-091851.log`)
**No crash with Ryu Hayabusa.** The guard dropped -256 four times (its log cap) and two Watch-mode
fights with Ryu played to the end, Hayate on the ring stage and on the helipad, at 60 fps, then
back to the title screen (the game was closed there, `TerminateTitle`, no crash report).
- **Open: the user reports Ryu's animations look wrong** ("the animation timing thing"), on the
  select screen and in the fight. The user's desktop recording of the second fight (NVIDIA
  capture, 83 s, 60 fps) shows smooth pacing: a new game frame in every captured frame, one hitch.
  In stills the fight looks plausible (throws, knockdowns, effects). The one clear oddity: on the
  select screen Ryu's model drifts up out of his panel after the face close-up (panel empty for
  about 1.5 s), then comes back cut off at the shoulders and slides down.
- **The guard is probably not the cause:** the table it guards looks like facial expressions
  (the same function picks a random entry about every 60 frames, like idle blinking). Not proven.
- **The user picked the select screen as the clearest case.** Compared with the real game (a
  Series X backwards-compatibility video and a story-mode walkthrough on YouTube): there,
  hovering Ryu shows the face close-up for about 0.8 s, then cuts to a full-body shot, a pose with
  a vanish, then the upper body with the sword, all in frame. In the port the face close-up holds
  about 3 s and the camera never cuts: the model keeps animating inside the close-up framing, so
  the head turns and rises out of the panel, the panel is empty about 1.5 s, and the body comes
  back cut off at the shoulders. **Hayate's sequence in the port is right** (face, full body,
  jump, pose, upper body), so the fault follows Ryu's data.
- **Not the guard:** it logs its first 4 drops, at 09:19:36, 09:19:49, 09:21:02 and 09:21:13, all
  in fights; the select-screen hover was at about 09:20:23.
- **Trace (not built):** `src/anim_trace.cpp` logs, per game frame, each player's motion-script
  starts (with their first bytes), every animation-ID request (`sub_82373680`, `sub_82372D78`),
  the consumer's gate bytes, and every request applied or dropped as stale. Run: hover Ryu on the
  left panel about 8 s without pressing A, then Hayate about 8 s, then Kasumi, then pick Ryu.

### Run 6 (2026-10-29, script-dispatch probe, Ryu as player 1; `port/logs/smoke-20261029-085747.log`)
> **Superseded by runs 8 and 9:** the reading below, "a bug in the original game", was wrong. The
> bytes were section 8's records run as a script, because Ryu's script table was never
> relocated. FINDINGS.md, "Ryu Hayabusa's unrelocated scripts", has the evidence; the guard
> described here was removed.

Same crash. **The dump settled it:** the stream the interpreter ran is Ryu's `ryu.bin` from
`anm.afs` (not `mot.afs`), loaded at guest 0x10000, byte for byte, and the walk is in step with
the format (tagged records the interpreter skips 2 bytes at a time by design, via an opcode-length
table). Opcode 20 really carries 0xFF00 in Ryu's data: **a bug in the original game**, an
out-of-bounds read the console survives. Full write-up in FINDINGS.md, "The negative animation
ID in Ryu Hayabusa's script".
- **The fix (not built):** the three probes are gone; `src/anim_id_guard.cpp` hooks 0x823729B0,
  just after the signed bound check, and jumps to 0x82372B14 for a negative index, the original's
  own path for an out-of-range ID or a null entry. It logs the first 4 drops ("DOA4: dropped
  animation ID ...").
- **Not confirmed:** why the console survives (likely: the page is committed in the game's 64K-page
  heap and the read returns heap data) and whether these streams are Ryu's face (masked).

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

## Native Linux (2026-10-29, branch linux-native)
The second stream (the user, 2026-10-29): a native build for the Steam Deck, no Proton or Wine,
720p by default there. **Builds and boots to the title in WSL2 (Linux run 1, below); the Deck
build is on the Deck, not yet run there** (every build needs the user's OK).
- **Scripts** (from graw360-recomp via doax2-360-recomp, which builds and runs natively in WSL2):
  `tools/setup-linux.sh`, `tools/build-linux.sh`, `tools/build-deck.sh` (Ubuntu 22.04 container
  `tools/linux/deck.Containerfile`, podman, glibc 2.35 at most), and `tools/steamdeck/`:
  `make_linux_package.sh` (`--deck deck@steamdeck`; `--with-game` copies `port/assets` until a
  copy has finished, marked by `.game-complete` beside `game/`), `doa4-linux.sh` (the launcher,
  which stops if it can't create `user/`), `doa4.toml` (fullscreen, 720p, `resolution_scale = 1`,
  16x AF, the low-memory reservation on, log flush every second), `README-SteamDeck-Linux.txt`.
  The Deck folder is `~/recomps/DOA4-360-SteamDeck` (AGENTS.md).
- **SDK:** the Windows tree 12884d9 (GRAW's set plus vexptefp-underflow, the same as GRAW's), then
  nine Linux patches in their own commit, tree **2f06e47**: GRAW's four (`launch-title-path`,
  `linux-crash-renderdoc`, `linux-shm-unlink`, `linux-crash-watchdog`; GRAW's Linux tree f481d5e)
  and DOAX2's three for this engine: `bind-low-port` (system link's VDP socket binds port 1000,
  which Linux refuses to a normal user), `linux-socket-ioctl` (Winsock FIONBIO/FIONREAD and errno
  to WSA codes: DOAX2 drew no first frame without both) and `linux-fiber-destroy`. DOA4 imports
  the same socket calls (`NetDll_bind`, `ioctlsocket`, `recvfrom`, `sendto`, `WSAGetLastError`)
  and uses XAPI fibers, so these are expected to be needed; not yet proven for DOA4. Then this
  project's two from the Deck profile (Deck run 1, below): `linux-wait-multiple` and
  `register-info-log`.
  - Tree history: **c66e664** was built first (WSL2 and the Deck, 2026-10-29, both tree
    checks passing), with DOAX2's two socket patches as DOAX2 has them. PR #2's review narrowed
    both (ff9a4d1, never built by itself): the low ports are mapped only for peers on the local
    network (broadcast, loopback, the private ranges, link-local), so a server elsewhere keeps
    port 80 or 53; and `socket_ioctl` passes any command it doesn't translate to `ioctl`
    unchanged again, as upstream did, instead of failing it. DOAX2 still has the earlier
    versions. `setup-linux.sh` now swaps an older Linux-patch commit for the new one by itself
    (no `--force`), so the rebuild keeps the build folder and recompiles only what changed.
  - **5e1f93a** (built in WSL2 and the Deck container on 2026-10-29: Deck build 2, below) adds
    the profile's first two fixes on top of ff9a4d1, both Linux-only so the Windows tree stays
    12884d9:
    - `linux-wait-multiple`: `WaitMultiple` sleeps on a shared condition variable until a
      handle may have been signaled, the timeout or 1 ms, instead of `sleep_for` a truncated
      millisecond count (0 inside every 1 ms alertable slice). Every place a handle's
      `cond_.notify_all()` signaled it (event, semaphore release, mutant release, timer, thread
      exit) now calls `NotifyAll()`, which also moves a generation counter on and wakes the
      sleepers. The alertable slice is rounded up instead of down, so its last millisecond
      doesn't spin either.
    - `register-info-log`: `CommandProcessor::WriteRegister` looks up `GetRegisterInfo` only
      when the GPU log would write the debug message it's for.
  - **2f06e47** (built in WSL2 and the Deck container on 2026-10-29; not on the Deck, which has
    5e1f93a) is the review's second round on `bind-low-port`. Mapping by address range missed
    peers on any other range, such as a VPN's (Tailscale's 100.64/10): a bind to port 1000 used
    11000, but a send to 1000 went to 1000. Now a destination or source is mapped only on a low
    port this title has bound itself, whatever its address. System link's peers run the same
    title and bind the same ports; a server's port 80 or 53, never bound here, keeps its number.
    `setup-linux.sh` now moves only a checkout at a tree an earlier version of it made
    (`old_linux_trees`), so an amended Linux-patch commit is left for `--force`.
- **Host code:** already guarded for non-Windows (`crash_report.cpp`, `cpu_check.cpp`,
  `timer_resolution.cpp`, `frame_timing.cpp`'s RenderDoc hook), from the LOTR shape. It built on
  Linux unchanged, with no compiler warnings. One gap from run 1: `ui_style.cpp` looked for the
  dialog font only in `C:\Windows\Fonts`, so on Linux the runtime's dialogs kept the SDK's 10 px
  font. It now tries Noto Sans (SteamOS's default sans; the Deck has it and DejaVu Sans) and
  DejaVu Sans at the Arch and Debian paths. In the Deck build (52e6f37); not yet seen running.
  Ubuntu's Noto path (`truetype/noto`) was added after it, in review.
- **WSL2** (Ubuntu 26.04, clang 21, CMake 4.2, podman): checkout `~/doa4-360-recomp`, cloned
  from the Windows checkout (its `origin`), with `port/assets` a link to the Windows checkout's;
  SDK in its `third_party/rexglue-sdk-bd833a2`; logs in `~/doa4-logs`. GRAW's and DOAX2's Linux
  SDKs are other trees (GRAW's host 379fa50, its Deck f481d5e; DOAX2's fc48868), so DOA4 has its
  own. The `doa4-deck-build` image is the same Containerfile as the existing `doax2-deck-build`,
  so podman's layer cache should make it quick.

### Linux run 1 (2026-10-29, WSL2, RelWithDebInfo; `~/doa4-logs/run1.log`)
The user's OK. llvmpipe (software Vulkan: WSL2 shows no GPU to Vulkan here), 720p
(`--resolution_scale=1`), windowed, `tools/autopilot/boot.txt`, 240 s then SIGINT:
```
cd port/out/build/linux-amd64-relwithdebinfo
./doa4 --no-fullscreen --resolution 720p --resolution_scale=1 --game_data_root <repo>/port/assets \
  --user_data_root <repo>/port/user-linux --cache_root <repo>/port/cache-linux \
  --log_file ~/doa4-logs/run1.log --log_flush_interval 1 --log_level debug \
  --doa4_autopilot=<repo>/tools/autopilot/boot.txt
```
- **Boots natively to the title screen**: the warning screens and the logo at 60 fps, then the 3D
  title (the cherry-blossom stage, PRESS START) at 1.3-2.3 fps, llvmpipe's limit (DOAX2 had
  1-2 fps on it). The runtime's "Select a Device" save prompt came up after the autopilot's
  last A press (60 s), so it stayed open; no profile or save was written.
- No crash and no new errors: the 17 `ResolvePath(cache:\)` errors and the STFS stubs match the
  Windows logs (the game probes the 360's cache partition for its AFS files). Audio runs through
  SDL; the game's fibers are created and deleted; the low-memory reservation is logged. SIGINT
  shut it down cleanly ("Title terminated; hard-exiting process").
- PipeWire printed a missing `client.conf` on stdout (WSLg has no PipeWire config); harmless.
- The dialog font gap above.

### The Deck build (2026-10-29, commit 52e6f37; `~/doa4-logs/build-deck.log`)
The user's OK. `tools/build-deck.sh`, then `--config Release`: the new `doa4-deck-build` image,
its own SDK (`third_party/rexglue-sdk-bd833a2-deck`, tree c66e664) and both builds. The newest
glibc any binary needs is 2.35 (`librexruntime`; `doa4` needs 2.34) and the newest libstdc++ is
GLIBCXX_3.4.32. `make_linux_package.sh` made the 296 MB folder in `private/steamdeck/`.
- Its `--deck` copy hung: WSL's `~/.ssh` has no key for the Deck (the passwordless key is
  Windows'), so ssh waited at a password prompt no one could see. The folder went over with
  WSL's `tar` piped into Windows' ssh instead (executable bits kept, `doa4`'s hash matching), then
  `port/assets` into `game/` (59 files, 7.22 GB, 62 s; `default.xex`'s hash checked). The script
  now runs ssh with `BatchMode=yes` and checks the connection first, so a missing key stops it
  at once with a message; SETUP.md says WSL needs its own key. This copy predates the
  `.game-complete` marker, so a later `--with-game` would have copied `game/` again; the marker
  was written there by hand with Deck build 2 (the copy was checked complete).
- On the Deck (SteamOS 3.9.5, glibc 2.43, RADV VANGOGH): `ldd` finds every library for both
  programs, and its libstdc++ has GLIBCXX_3.4.32.

### Deck build 2 (2026-10-29, commit 7698c54, tree 5e1f93a)
The user's OK. The profile's two fixes and the review's earlier changes (tree ff9a4d1, the font
path). `setup-linux.sh` moved both SDK checkouts from c66e664 by itself and rebuilt only what
changed: in WSL2 the SDK took 37 s and the game 8 s, and the Deck container's two builds 44 s.
The four changed SDK files compiled with no new warnings. The newest glibc is still 2.35 and
the newest libstdc++ GLIBCXX_3.4.32. The package went to the Deck through Windows' ssh
(`doa4`'s hash matching; `logs/`, `user/` and `game/` untouched, the new launcher included), and
`doa4-prof.sh` was started again (unit `doa4-prof-215839`, into
`~/recomps/doa4-prof/1029-215839/`) to profile the next fights.

### Deck run 1 (2026-10-29, the user; Release, 720p, through Steam; logs `doa4_001`-`004.log`)
Four sessions, menus and several fights. The user's report: gameplay intact, no graphical or
animation problems, but **fights feel laggy, as if at 30 Hz**, the moves slower than the buttons.
- The game's own frame lines (`DOA4 frames`, every 10 s) explain it: menus and select screens
  hold 60 fps, but **fights run at 42-50 fps** (median frame 20-23 ms, only 1-7% of frames at a
  60 Hz vblank, none at 30 Hz). DOA4 steps once per frame, so that is the fight in slow motion
  (about 75-80% speed), not a 30 Hz lock. On Windows the same fights hold 60.0 (median 16.6 ms).
- The first session's 1-11 fps stretches were shader compiles (no cache yet); the later
  sessions, with `user/cache`, had none.
- Not the cause: the render target path is the host one (`render_target_path_vulkan` unset; FSI
  is only used when asked for), and the swapchain is 720p (1080p in session 3, on an external
  display).
- Next: `~/recomps/doa4-prof.sh` on the Deck (started over SSH, waits for `doa4`): every 10 s the
  GPU busy %, clocks and the busiest threads' CPU %, plus 8 s perf samples every 40 s, into
  `~/recomps/doa4-prof/<time>/`. A fight under it says whether the frame waits on the GPU, the
  GPU command thread or the game's own thread. Cheap test either way: `anisotropic_override` -1
  (the game's own filtering) instead of 16x.
- **Profile (session 5, three fights, `~/recomps/doa4-prof/1029-175258`):** the Deck is CPU and
  power bound, not GPU bound.
  - GPU busy 28-56% throughout, but four threads each use a whole core, so the CPU runs at
    2.5-3.1 GHz (3.5 GHz max).
  - In the 13 windows under 54 fps the GPU command thread is about 90% busy, against about 74%
    in the 23 windows at 57-60. It is the limit. The other three busy threads don't change
    between slow and fast windows, because they're waiting:
    - `Audio Worker` (~96%), an SDK bug. The audio worker's alertable 500 ms `WaitAny` runs as
      1 ms slices of `PosixConditionBase::WaitMultiple`. When no handle is signaled, that
      sleeps `duration_cast<milliseconds>(end_time - now)`, which is always 0 inside a 1 ms
      slice, so `sleep_for(0)` returns at once. Every alertable multi-object wait on Linux
      spins at 100%, user time only (measured 97% user, 0% system). The same code is in
      upstream `main`.
    - `XThread4DFFD6C0` (~99%), the game's job worker `sub_823FC5A0`. It loops forever doing
      `RtlEnterCriticalSection`, a check of a 16-slot job ring, then `RtlLeaveCriticalSection`,
      with no wait when the ring is empty. Its samples are almost all that lock traffic, on
      the lock the main thread posts jobs under.
    - `Main XThread` (~90%) mostly spins in `sub_827598B0` / `sub_8275A0B8`. They loop on
      `sub_82761AB8`, which compares two command-processor pointers against a 2 s timeout,
      with a 100-step delay loop in between: the main thread waits for the GPU command thread
      to catch up.
  - The GPU command thread itself: about 65% `librexgpu-xenos`, 17% RADV, 13% libc copies.
    About a third of it is register writes (`WriteRegister`, `GetRegisterInfo`, the Vulkan
    `WriteRegister`, `ExecutePacketType0`). `CommandProcessor::WriteRegister` calls
    `RegisterFile::GetRegisterInfo` (a switch over the whole register table) on every write,
    only to log an unknown register at debug level: about 10% of the thread.
- **Planned fixes, smallest first** (each needs the user's OK to build):
  1. Linux SDK: make `WaitMultiple` really wait. Either sleep the true remaining time, or
     better, wake on a signal through a shared generation counter and condition variable,
     with the 1 ms slice as a fallback. **Done** (`linux-wait-multiple`), on the Deck since
     Deck build 2; its effect is still to be measured.
  2. SDK: look up the register info only when debug logging is on. **Done**
     (`register-info-log`), on the Deck since Deck build 2.
  3. Codegen: midasm hooks that pause or yield in the job worker's empty-ring path and in the
     main thread's wait loop.
  Each one gets a Deck profile run before the next.
- **The 120 fps the Steam overlay shows** is the host presenter (mailbox present mode on a
  120 Hz external display, HDMI audio in this session), repeating the game's last frame. It
  isn't the game's rate.
- **PRESS START sits on an opaque white bar** on the title screen (the user's report). Windows
  draws it too (the 1440p captures), so it isn't the Deck. On the 360 the chrome letters blink
  with nothing behind them (a 360 recording of the title screen). Probably a quad whose alpha
  is lost; needs a RenderDoc capture of the title on Windows.

## Next steps
1. **Linux stream, with the user's OK for each build:** done: the SDK, the build and Linux run 1
   in WSL2 (boots to the title), and the Deck build, copied to `~/recomps/DOA4-360-SteamDeck`
   with the disc's files. Next: the user's Deck run at 720p, through Steam (the folder's README)
   and the controller (PLAN.md M5: 60 fps in a 2-character fight or a measured list of what
   stops it); it also shows the dialog font. Its logs stay in the Deck folder's `logs/`: pull
   them over SSH from Windows. Deck build 2 (above) brought the review's SDK patch changes, the
   font path and the launcher's `user/` check.
   Deck run 1 is done (above): fights at 42-50 fps, so the next Linux work is that profile, and
   the white PRESS START bar on every platform. The profile's first two fixes are on the Deck
   (Deck build 2), with `doa4-prof.sh` waiting for the game: next, the user's fights under it,
   then the frame lines and profile against run 1's, then fix 3 if the command thread still
   limits the frame rate.
2. **Windows (merged):** M2 checks (more characters, Story, Survival, the critical-hit flash at
   scale 1), the 1440p default measured.
3. Find the relocation check that skips Ryu's table (FINDINGS.md), to confirm the reason.
4. An autopilot script into Sparring (`tools/autopilot/`), with the timings from run 2's
   captures, so a fight can be reached without a person at the pad.
5. An upstream issue for the halfword-index switch bug. (LOTR's `find_vcall_thunks.py` rerun is done: lotr-conquest-360-recomp PR #2, still 0 forwarders.)
6. Every change on a branch and through a PR (CONTRIBUTING.md); CodeRabbit reviews it.

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
| `tools/setup-linux.sh`, `build-linux.sh`, `build-deck.sh`, `tools/linux/` | the native Linux and Deck builds (above, "Native Linux"); the seven `linux_patches` in `tools/patches/` |
| `tools/steamdeck/` | the Deck folder: launcher, `doa4.toml` (720p), README, `make_linux_package.sh` |
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
