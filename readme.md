# TitanPlayer

TitanPlayer 是一个基于 C++、Qt 6.8.3 和 QML 开发的视频播放器。播放器 UI 使用 QML 实现，音视频解码使用 FFmpeg 8.1 x64 SDK，视频画面通过 `QQuickFramebufferObject` 结合 OpenGL 渲染。

## 功能特性

- 无边框主窗口，支持拖动、最大化、最小化、关闭和边缘缩放。
- 支持一次加载一个或多个视频文件。
- 支持播放、暂停、上一条、下一条、进度跳转。
- 支持右侧播放列表展开/收起，双击列表项切换播放。
- 支持音量按钮上方弹出竖向音量调节窗口。
- 支持倍速按钮上方弹出倍速菜单。
- 支持全屏播放。
- 支持截图，并可在设置窗口中配置截图目录和图片格式。
- 使用 FFmpeg 8.1 进行音视频解码，Qt Multimedia 负责音频输出。

## 开发环境

当前项目按以下环境配置：

- Windows x64
- Visual Studio 2026 / MSVC x64 工具链
- Qt 6.8.3 MSVC2022 64-bit
- CMake 3.16+
- FFmpeg 8.1 x64 shared SDK

FFmpeg SDK 已放在项目目录：

```text
third_part/ffmpeg-8.1-full_build-shared
```

项目构建时会链接 `lib` 目录下的 FFmpeg 库，并在构建完成后自动复制 `bin` 目录中的 DLL 到程序输出目录。

## 构建方式

在项目根目录运行：

```bat
build-debug.bat
```

脚本会执行以下步骤：

- 初始化 MSVC x64 编译环境。
- 设置 Qt 6.8.3 的 `bin` 路径。
- 使用 CMake 生成 `build-debug` 目录。
- 编译 Debug 版本。

也可以在已初始化 MSVC 环境后手动执行：

```bat
cmake -B build-debug -S . -DCMAKE_PREFIX_PATH=D:\DevKits\Qt\6.8.3\msvc2022_64
cmake --build build-debug --config Debug --parallel
```

编译产物默认位于：

```text
build-debug/Debug/appTitanPlayer.exe
```

Qt Creator 使用的构建目录通常位于：

```text
build/Desktop_Qt_6_8_3_MSVC2022_64bit-Debug
```

## 目录结构

```text
TitanPlayer/
├── core/                 # 播放控制、FFmpeg 解码、音频输出、音视频队列
├── qml/                  # QML UI 组件
├── render/               # QML OpenGL 视频渲染组件
├── resources/            # SVG 图标资源
├── third_part/           # 第三方库，目前包含 FFmpeg 8.1 x64 SDK
├── VideoPlayer/          # QWidget 版本参考实现
├── Main.qml              # QML 主窗口入口
├── main.cpp              # 应用入口和 QML/C++ 类型注册
├── CMakeLists.txt        # CMake 构建配置
├── resources.qrc         # Qt 资源文件
└── build-debug.bat       # Debug 构建脚本
```

## 核心模块说明

- `core/FFmpegPlayer`：运行在解码线程中，负责打开媒体文件、解码视频帧和音频帧、处理 seek、倍速和播放状态。
- `core/AudioOutput`：基于 `QAudioSink` 输出 PCM 音频。
- `core/PlayerController`：暴露给 QML 的播放控制器，统一管理播放列表、当前文件、播放状态、进度、音量、倍速、截图和设置。
- `render/VideoOutputItem`：QML 可用的视频渲染 Item，使用 OpenGL 纹理显示最新视频帧。
- `qml/ControlBar.qml`：底部播放控制区。
- `qml/TitleBar.qml`：顶部标题栏。
- `qml/PlaylistPanel.qml`：右侧播放列表。
- `qml/SettingsDialog.qml`：设置窗口。

## 文本与翻译约定

- QML 中显示给用户的文本统一使用 `qsTr()`。
- C++ 中显示给用户的文本统一使用 `tr()`。
- 后续可通过 Qt Linguist 工具生成 `.ts` / `.qm` 文件完成多语言支持。

## 常用快捷键

- `Ctrl+O`：打开视频文件。
- `Space`：播放 / 暂停。
- `Left` / `Right`：后退 / 前进 5 秒。
- `Up` / `Down`：调高 / 调低音量。
- `F`：进入 / 退出全屏。
- `Esc`：退出全屏。
- `L`：展开 / 收起播放列表。

## 备注

- 当前视频解码输出为 RGB 帧，再上传为 OpenGL 纹理显示。
- 当前倍速播放使用重采样方式处理音频，适合播放器快速预览场景。
- 若 Qt Creator 运行时看到 “QML debugging is enabled”，这是 Debug 环境下的正常提示。