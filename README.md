# SekiroTweaks

A drop-in `dinput8.dll` mod for **Sekiro: Shadows Die Twice** that applies an FPS unlock, borderless window and camera fixes every time the game starts. No external launcher to run.

## Features

- **FPS unlock** (30–300, default 120) with the game-speed fix, so the game doesn't run fast
- **Borderless window**: fills the monitor or keeps the game's resolution. Toggle in-game with **F11**
- **Camera auto-rotate off while using the mouse.** The camera stops swinging when you move. It switches live: touch a controller and vanilla behaviour returns, touch the mouse and it's off again
- Optional: **FOV** change, **no camera reset on lock-on**, **custom / ultrawide resolution**

Everything is set in `SekiroTweaks.ini`.

## Install

1. Download the zip from [Releases](../../releases).
2. Copy `dinput8.dll` and `SekiroTweaks.ini` into the folder with `sekiro.exe`
   (Steam: right-click Sekiro → Manage → Browse local files).
3. **In the game's settings, set Screen Mode = Windowed.** The mod turns it into borderless fullscreen. In Fullscreen the game forces 60 Hz.
4. **Turn VSync off for `sekiro.exe` in your GPU driver.** NVIDIA: Vertical sync = Off and Preferred refresh rate = Highest available. AMD: Enhanced Sync.
5. Launch the game normally.

To uninstall, delete `dinput8.dll`.

## Configuration

| Key | Default | Meaning |
|---|---|---|
| `FpsLimit` | 120 | Frame cap, 30–300. `0` = vanilla |
| `Borderless` | 1 | Borderless at startup |
| `BorderlessFillMonitor` | 1 | `1` = fill monitor, `0` = game resolution, centered |
| `BorderlessToggleKey` | 122 (F11) | Virtual-key code. `0` = no hotkey |
| `DisableCameraAutoRotate` | 1 | `0` vanilla, `1` mouse only, `2` always |
| `FovChangePercent` | 0 | −95 to 95 |
| `DisableCameraResetOnLockOn` | 0 | `1` = no camera snap when nothing is in range |
| `CustomResolutionWidth` / `Height` | 0 | Replaces the 1920x1080 entry in the game's list |
| `ChainLoadDll` | (blank) | Load another dinput8 mod, e.g. a renamed Mod Engine |
| `Log` | 1 | Writes `SekiroTweaks.log` (startup and input switches only) |

## Troubleshooting

Check `SekiroTweaks.log` next to the exe. It lists every patch that applied or failed.

## Building

Needs clang and lld-link (any OS). No Windows SDK or CRT required:

```sh
sh src/build.sh   # output: build/dinput8.dll
```

## Credits

Byte patterns and patch values are from [uberhalit/SekiroFpsUnlockAndMore](https://github.com/uberhalit/SekiroFpsUnlockAndMore) (MIT). This project reimplements them as an in-process mod and adds mouse/controller switching for the camera fix.
