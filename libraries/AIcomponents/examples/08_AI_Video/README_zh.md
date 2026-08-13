# AI 视频（摄像头 JPEG 帧采集）

本示例演示如何使用 Arduino-TuyaOpen 框架中的 **TuyaVideo** 模块，初始化并启动 AI 视频管线（`ai_video`），并持续从摄像头获取 JPEG 帧。

> 本示例仅支持带摄像头模块的 TUYA_T5AI 平台（`board_register_hardware()` 会在视频管线启动前注册包括摄像头在内的板载硬件）。此外还需要 vendor 编译配置中启用 `CONFIG_ENABLE_COMP_AI_VIDEO=y`，该配置已存在于本仓库的 T5 vendor 包配置（`tools/ci/config/t5/app_default.config`）中。

### 项目文件结构

```
08_AI_Video/
└── 08_AI_Video.ino    # 主程序入口，初始化并启动 AI 视频管线，随后轮询获取 JPEG 帧
```

## 代码烧录流程

0. 确保已完成[快速开始](Quick_start.md)中开发环境的搭建。

1. 连接带摄像头的 T5AI 开发板到电脑，打开 Arduino IDE，选择 `TUYA_T5AI` 开发板，并选择正确的烧录端口。

> 注意：T5AI 系列开发板提供双串口通信，连接电脑后会检测到两个串口号，其中 UART0 用于固件烧录，请在 Arduino IDE 中选择正确的烧录口。

2. 在 Arduino IDE 中点击 `文件` -> `示例` -> `AI components` -> `08_AI_Video`，打开示例代码。

3. 点击 Arduino IDE 左上角按钮烧录代码，终端出现以下信息说明烧录成功。

```bash
[INFO]: Write flash success
[INFO]: CRC check success
[INFO]: Reboot done
[INFO]: Flash write success.
```

> 本示例无需配置设备授权码，也不会连接网络或涂鸦云——它只是本地调用 `ai_video` 组件。

也可以通过命令行编译：

```bash
arduino-cli compile --fqbn tuya_open:tuya_open:t5 08_AI_Video.ino
```

## 运行示例

烧录完成后，打开 Arduino IDE 自带的串口监视器（波特率 115200）。`setup()` 中调用了 `Log.begin()`，示例通过 `PR_NOTICE`/`PR_ERR` 宏输出日志。

根据 `08_AI_Video.ino` 中字面的 `PR_NOTICE`/`PR_ERR` 调用，`setup()` 首先打印启动横幅：

```
============ Tuya AI Video ==============
Compile time:        <编译日期，来自 __DATE__>
```

若 `TuyaVideo.begin()` 或 `TuyaVideo.start()` 失败，会打印对应的错误信息，并进入 `delay(1000)` 死循环：

```
Failed to initialize AI video
```

```
Failed to start AI video
```

否则，`loop()` 每 100 毫秒轮询一次，每当成功获取到一帧时打印：

```
JPEG frame length: <字节数>
```

## 示例代码说明

### 硬件注册

```cpp
board_register_hardware();
```

在初始化视频管线之前，通过 `board_com_api.h` 注册板载硬件（包括摄像头）。

### 初始化与启动

```cpp
if (OPRT_OK != TuyaVideo.begin()) { ... }
if (OPRT_OK != TuyaVideo.start()) { ... }
```

`TuyaVideo.begin()` 初始化 `ai_video` 组件，该调用是幂等的——若已初始化则立即返回 `OPRT_OK`。随后 `TuyaVideo.start()` 启动视频管线；本示例将这两个调用中的任意一个失败都视为致命错误，并停在 `delay(1000)` 死循环中。

### 帧轮询循环

```cpp
uint8_t *jpegData = nullptr;
uint32_t jpegLen  = 0;

if (OPRT_OK == TuyaVideo.getJpegFrame(&jpegData, &jpegLen)) {
    PR_NOTICE("JPEG frame length: %u", jpegLen);
}

TuyaVideo.freeJpegFrame(&jpegData);

delay(100);
```

`TuyaVideo.getJpegFrame()` 返回最新的 JPEG 帧（如果有的话）。在本示例中，无论 `getJpegFrame()` 是否返回 `OPRT_OK`，每次循环都会调用 `TuyaVideo.freeJpegFrame(&jpegData)`。每一帧只要通过 `getJpegFrame()` 成功获取，最终都必须调用 `freeJpegFrame()` 释放，以避免帧缓冲区泄漏。

### YUV 帧回调（本示例未使用）

`TuyaVideo` 还提供了一个原始帧回调接口，本示例并未注册：

```cpp
OPERATE_RET setYuvFrameCallback(AI_VIDEO_FLUSH_CB cb);
```

为保持接口文档的完整性，下方一并列出该接口，它属于 `TuyaVideo` 的 API 组成部分。

### TuyaVideo 接口

| 接口 | 签名 | 说明 |
| --- | --- | --- |
| `begin` | `OPERATE_RET begin()` | 初始化 AI 视频组件（幂等） |
| `end` | `void end()` | 若管线正在运行则停止，并清除本地初始化标志（上游 `ai_video_input.h` 没有反初始化接口） |
| `isInitialized` | `bool isInitialized()` | 检查组件是否已初始化 |
| `start` | `OPERATE_RET start()` | 启动视频管线 |
| `stop` | `OPERATE_RET stop()` | 停止视频管线 |
| `getJpegFrame` | `OPERATE_RET getJpegFrame(uint8_t **data, uint32_t *len)` | 获取最新的 JPEG 帧；使用完毕后需调用 `freeJpegFrame()` |
| `freeJpegFrame` | `OPERATE_RET freeJpegFrame(uint8_t **data)` | 释放此前由 `getJpegFrame()` 返回的 JPEG 帧 |
| `setYuvFrameCallback` | `OPERATE_RET setYuvFrameCallback(AI_VIDEO_FLUSH_CB cb)` | 注册每一帧原始 YUV 数据到来时的回调 |

除 `end()` 和 `isInitialized()` 外，若组件尚未初始化，其余方法均返回 `OPRT_INVALID_PARM`（对于 `getJpegFrame`/`freeJpegFrame`/`setYuvFrameCallback`，指针或回调参数为空时同样返回该错误码）。

## 相关文档

- [TuyaOpen 官网](https://tuyaopen.ai)
- [Github 代码库](https://github.com/tuya/TuyaOpen)
