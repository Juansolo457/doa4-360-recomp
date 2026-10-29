# Findings: the disc and the executable

Facts about the supported disc and its `default.xex`, with how each was found. Every address is
a virtual address in the decrypted image (base `0x82000000`) of the executable below. Nothing
about where the copy came from is recorded here.

## The disc
- **Dead or Alive 4 (USA, Europe) (En,Ja,Fr,De,Es,It,Zh,Ko) (Rev 1)**, Redump-verified. ISO
  7,834,892,288 bytes.
- Game partition at `0xFD90000`: **59 files, 6.73 GiB** (`tools/xdvdfs.py <iso> list`).
  `tools/extract.ps1` unpacks it into `port/assets` (6.8 GB on disk).
- Layout:
  - `default.xex`, the only executable (15.7 MB);
  - CRI **AFS archives**: `chrone.afs` 366 MB, `chrtwo.afs` 548 MB, `chrthree.afs` 763 MB
    (characters), `stg.afs` 815 MB (stages), `avt.afs` 226 MB, `spr.afs` 184 MB (sprites),
    `snd.afs` 125 MB, `mot.afs` 75 MB (motions), `cmn.afs` 9 MB, `anm.afs` 3 MB;
  - `SOUND/`: **XACT** `doa4_sound.xgs`, `bgm_stream.xwb` (195 MB) and 23 per-character
    `voice_<name>_stream.xwb` (0.7-2.1 MB each) plus `voice_sys_stream.xwb` (9.6 MB). The
    non-streamed banks (`voice_*.xwb/.xsb`, `stage_*.xwb`, `con_*.xwb`, `hos_*.xwb`,
    `system_se.xwb`) named in the image live inside `snd.afs`;
  - `movie/*.sfd`: **Sofdec** (CRI MPEG) videos, 3.9 GB: one ending per character
    (`ayane.sfd` ... `zack.sfd`, 132-333 MB; `helena.sfd` 755 MB), `doa4_usa_cm.sfd` (64 MB),
    `ninja_vi_hd.sfd` (75 MB, the Ninja Gaiden trailer);
  - `dic/ximejpmain.dic`, `ximejpsub.dic`, `ximejpnameaddr.dic`: Japanese IME (XIME).
- No `$SystemUpdate` folder on this disc.

## The executable
| | |
|---|---|
| File | `default.xex`, 16,465,920 bytes |
| SHA-256 | `0dba23465e091a03c119a9fb06765ac9cd51e7c14cc22e2130f0c11bf28e7530` (`tools/build.ps1` checks it) |
| Title, version | `544307D1`, **0.0.0.9** (the Rev 1 disc carries the updated executable), media `606FFE2A`, disc 0/0 |
| Built | **2006-02-14 15:43 UTC** (PE checksum `0103B2C5`); original base `0x00400000` |
| XDK | **2.0.2135**: XAPILIB .4, D3DX9, XGRAPHC .3, XBOXKRNL, XNET .1, XONLINE .1, XAUD .1, XACT .1, LIBCPMT, XHV, XMP, X3DAUD .1, D3D9LTCG. The oldest XDK of our recomps (GRAW 2417, F.E.A.R. and DOAX2 3529, LOTR Conquest 7645) |
| Image | base `0x82000000`, size `0x1600000` (23,068,672 bytes decrypted), entry `0x827929B0`, default stack 256 KB; AES-encrypted (retail key), **basic** compression (`tools/xex_basic_decrypt.py` can decrypt it; `xexdump` was used) |

Sections (`xexdump`, 2026-10-29):
| Section | Address | Size | |
|---|---|---|---|
| .rdata | `0x82000400` | `0x2C6428` | |
| .pdata | `0x822C6A00` | `0x1D708` | function table, ~15,000 entries |
| .text | `0x822F0000` | `0x86EDFC` | 8.4 MB of code |
| PSFD00 | `0x82B5EE00` | `0x5B44` | code: CRI Sofdec's own section |
| .data | `0x82B70000` | `0xA35018` | 10.2 MB |
| .XEXID | `0x835A5200` | `0x4` | |
| .idata | `0x835B0000` | `0x426` | |
| .XBLD | `0x835C0000` | `0xD0` | |
| .reloc | `0x835C0200` | `0xDD8E4` | |

### Imports: all but data exports implemented
`tools/xeximports.py` against ReXGlue `bd833a2`'s export tables (2026-10-29): every imported
kernel and XAM function has an implementation except **15**, all data exports the loader
provides or the C runtime's string formatters: `DbgPrint`, `sprintf`, `_vsnprintf`, `vsprintf`,
`vswprintf`, `ExThreadObjectType`, `KeDebugMonitorData`, `KeTimeStampBundle`,
`XexExecutableModuleHandle`, `ExLoadedCommandLine`, `VdGlobalDevice`, `VdGlobalXamDevice`,
`VdGpuClockInMHz`, `VdHSIOCalibrationLock`, `KeCertMonitorData`. The identical set DOAX2 had;
nothing on the boot path. Notable imports: the full XMA context set (hardware audio decode),
`XAudioRegisterRenderDriverClient`, `VdSwap`, `VdInitializeEDRAM`/`VdRetrainEDRAM`,
`StfsCreateDevice` (the `cache:` probe), `XeKeysConsolePrivateKeySign` (Live signing),
`XamContentCreate`, `XNotifyGetNext`, `XGetLanguage`, `XGetVideoMode`, `XGetGameRegion`.

### Engine and middleware (from the decrypted image's strings, 2026-10-29)
- **Team Ninja's in-house engine**, the same family as Dead or Alive Xtreme 2 (title `544307D2`,
  XDK 3529, eight months later): the same AFS/XACT/Sofdec/XIME/libjpeg stack and the same devkit
  path style. The devkit paths `D:\DOA4\`, `D:\Sound\*.xwb`, `D:\bin\ninja.bin`,
  `D:\dat\char\<id>\<id>.cat/.tpr` name a development layout; the disc packs that data into the
  AFS archives, so expect failed `D:\` opens in the log (harmless in DOAX2).
- **CRI middleware** dated Oct/Nov 2005: Sofdec (SFD 1.976, MPV, MPS, SFH, SFX, SUD), ADX
  threads (`adxm_destroy_thrd (vsync)`: a thread that waits on vblank), CRS critical sections,
  DCT, ERR, DLG. "(c)CRI", "Copyright (c) 1999-2004 CRI-MW".
- **libjpeg** (the IJG messages: "Corrupt JPEG data", "Bogus JPEG colorspace", Adobe APP14) and
  the photo container name `photodata.ptc`: the engine's photo code, shared with DOAX2, whose
  libjpeg error exit **longjmps** (its album crash until `setjmp`/`longjmp` were named in the
  manifest).
- **The Xbox 360 shader assembler and the D3DX HLSL compiler** are in the image (assembler
  diagnostics, "---- Vertex Shader / Pixel Shader", `CompileWithYUVConversion`, XGraphics
  microcode source paths from `d:\xenon\oct05\`): some shaders are assembled or compiled at run
  time, as in DOAX2 and GRAW (whose first crash was in this compiler's parser, through a
  misread jump table).
- MSVC RTTI (`Base Class Descriptor`, `Type Descriptor`): C++ with virtual classes throughout.
- **Xbox Live everywhere**: "Currently in a session" in eight languages, the `xbos`/`xuacs`
  content and subscription endpoints (`XeContentEnumerate.ashx`, `XeCreateLiveAccount.ashx`...),
  XHV voice, achievements ("Let's play DEAD OR ALIVE 4 and get this Achievement!").
- No Bink, Wwise, Havok, Scaleform or Lua.

### Emulator state
Xenia Canary [game-compatibility #207](https://github.com/xenia-canary/game-compatibility/issues/207):
`state-playable`. No published static recompilation of this title was found (2026-10-29).

## Decrypting for analysis
```
..\quake4-definitive\build\xexdump\xexdump.exe port\assets\default.xex private\xex\default
```
writes `private/xex/default.bin` (the image at `0x82000000`), `.sections.txt` and `.symbols.txt`
(the import thunks). `tools/xex_basic_decrypt.py` works too (basic compression) given the retail
key in `DOA4_XEX_KEY` or `tools/xex_key.txt` (ignored); ReXGlue's code generator reads the XEX
directly and needs neither.
