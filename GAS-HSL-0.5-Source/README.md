# GAS-HSL — Version 0.5

Native OBS Studio video effect filter for Windows x64. Built and tested with OBS 31.1.2 and Direct3D 11.

## Install

Close OBS. Run `GAS-HSL-0.5-Setup.exe` as administrator. It installs into `C:\ProgramData\obs-studio\plugins\gas-hsl\`. The resulting files include:

```text
C:\ProgramData\obs-studio\plugins\gas-hsl\bin\64bit\gas-hsl.dll
C:\ProgramData\obs-studio\plugins\gas-hsl\gas-hsl.dll
C:\ProgramData\obs-studio\plugins\gas-hsl\data\effects\hsl.effect
C:\ProgramData\obs-studio\plugins\gas-hsl\data\locale\en-US.ini
C:\ProgramData\obs-studio\plugins\gas-hsl\data\locale\zh-CN.ini
```

Restart OBS, then right-click a video source → Filters → add an Effect Filter → GAS-HSL. Each filter has its own settings. **Enable Processing** passes the original picture through when unchecked. **Reset All** zeros all HSL controls and restores the default custom color. The bottom of the settings panel shows **Create by Gassy** and a website link.

To sample from the visible OBS preview, click **Pick from OBS preview**, then click the desired pixel in the preview. The sampled color is saved in **CUSTOM COLOR → Pick target color**; use the three custom HSL sliders below it to adjust that hue. Click the button again before sampling to cancel. You can also choose a color directly in OBS's native color control. Sampling reads the displayed screen pixel, so the preview must be visible and unobstructed. It does not inspect the source's original texture.

The installer puts the DLL in both the OBS 31/32 and OBS 33+ plugin locations. The plugin was built against OBS 31.1.2; later OBS versions still need testing. [OBS plugin locations](https://obsproject.com/kb/legacy-plugin-locations)

## Source layout

```text
CMakeLists.txt
src/plugin-main.c
data/effects/hsl.effect
data/locale/en-US.ini
data/locale/zh-CN.ini
tests/check_masks.py
installer/GAS-HSL.iss
installer/logo.ico
```

## Processing

The `gas_hsl_filter` source is `OBS_SOURCE_TYPE_FILTER`. `video_render` calls OBS's filter begin, sets the HSL uniforms, then calls filter end. Unchecking **Enable Processing** calls `obs_source_skip_video_filter` before any HSL draw. The effect is created once per filter instance and destroyed with it; settings live in OBS Source Settings.

In the pixel shader: unpremultiply RGB → convert to HSL → apply master hue, saturation, lightness → calculate soft hue weights → apply the weighted regional adjustments → apply the custom hue region → convert to RGB → premultiply and restore the original alpha. Hue uses a circular distance, so 359° and 1° are neighbors. The eight fixed centers and soft outer radii are together in `hsl.effect`. The selected color sets the custom region's center, with a 45° soft radius; gray picks suppress that region. The custom adjustments do not alter the eight fixed regions when their sliders are zero. Overlapping fixed-region weights are normalized; low saturation fades regional influence with `smoothstep(0.02, 0.12, saturation)`. Lightness moves toward white or black by at most 65% of the available distance, avoiding immediate clipping. The shader clamps its SDR output.

## Build

Use CMake, Visual Studio 2022 x64, and a matching OBS/libobs development package that provides `OBS::libobs` and `libobsConfig.cmake`:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -Dlibobs_DIR="C:\path\to\libobs\cmake"
cmake --build build --config Release
cmake --install build --config Release --prefix release
```

The resulting DLL is `build/Release/gas-hsl.dll` with the Visual Studio generator. `release/gas-hsl/` is the folder to install. Run `python tests/check_masks.py` for the small, dependency-free mask check.

Build the installer with Inno Setup 7: `ISCC.exe installer/GAS-HSL.iss`. The output is `GAS-HSL-0.5-Setup.exe` one directory above this project folder. The installer is not code signed.

## Verification and limits

- Windows x64 Release DLL built against the locally installed OBS 31.1.2 runtime and matching official headers.
- Effect loaded and compiled through OBS's Direct3D 11 backend.
- GPU pixel checks passed for red desaturation, blue isolation, orange and custom-color responses on a skin-like color, gray preservation, hue wrap, boundary continuity, and alpha preservation.
- Three filter instances loaded, kept separate settings, and were destroyed cleanly in a libobs test harness.
- The preview picker button armed and canceled its Windows mouse hook in a libobs test harness. An actual click on the OBS preview has not yet been tested in the OBS GUI.
- OBS GUI tests with Media Source, Image Source, Video Capture Device, and Display Capture remain to be done. Long-running stability and 1080p60/4K performance have not been measured. SDR/Rec.709 is the supported color workflow; HDR is not covered.
