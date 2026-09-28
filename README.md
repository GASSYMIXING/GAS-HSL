# GAS-HSL
A lightweight real-time HSL color filter for OBS Studio.
# GAS HSL

GAS HSL 是一款面向 **OBS Studio** 的实时 HSL 调色滤镜插件，主要用于直播、摄像机画面校色、多机位匹配以及简单的人像与产品颜色调整。

它直接作为 OBS 的视频滤镜运行，可以为摄像头、采集卡、媒体源等画面实时进行 HSL 调整，无需额外打开独立调色软件。

插件提供全局 HSL 控制，并支持对不同颜色区域进行独立调整，包括：

- Red
- Orange
- Yellow
- Green
- Cyan
- Blue
- Purple
- Magenta

每个颜色区域均可独立调整 Hue、Saturation 和 Lightness。

在实现上，GAS HSL 使用 GPU Shader 完成实时图像处理，并采用平滑的色相范围过渡，而不是简单的硬色域切割，从而尽量减少不同颜色之间的断层和突变。同时针对低饱和度区域进行了保护，降低对白色、灰色和黑色区域产生意外染色的情况。

## 主要功能

- OBS 原生效果滤镜
- Master Hue / Saturation / Lightness
- 8 色独立 HSL 调整
- 平滑 Hue Mask
- Hue 循环色相处理
- 低饱和度区域保护
- GPU 实时处理
- 多滤镜实例独立参数
- 参数自动随 OBS 保存
- Bypass
- Reset
- 中文 / 英文本地化

## 适用场景

GAS HSL 更偏向直播和实时视频场景，例如：

- 摄像机肤色微调
- 直播间灯光偏色修正
- 产品颜色校正
- 多台摄像机色彩统一
- 特定颜色增强或减弱
- OBS 内快速完成基础调色

项目目标不是取代专业调色软件，而是提供一个 **轻量、直观、低延迟，并且可以长期挂在 OBS 视频源上的实时 HSL 工具**。

## 开源说明

GAS HSL 目前作为开源项目发布。

欢迎提交：

- Bug Report
- Feature Request
- Pull Request
- OBS 版本兼容性反馈
- Shader 与调色算法优化建议

如果你也在做 OBS、直播工具、实时视频处理或者图像 Shader，欢迎一起完善这个项目。
