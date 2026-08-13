# AI Video (Camera JPEG Frame Capture)

This example demonstrates how to use the **TuyaVideo** module in the Arduino-TuyaOpen framework to initialize and start the AI video pipeline (`ai_video`) and continuously fetch JPEG frames from a camera.

> This example only supports the TUYA_T5AI platform with a camera module attached (`board_register_hardware()` registers the onboard hardware, including the camera, before the video pipeline starts). It also requires `CONFIG_ENABLE_COMP_AI_VIDEO=y` in the vendor build, which is already present in this repository's T5 vendor package config (`tools/ci/config/t5/app_default.config`).

### Project File Structure

```
08_AI_Video/
└── 08_AI_Video.ino    # Main program entry; initializes and starts the AI video pipeline, then polls for JPEG frames
```

## Flashing Procedure

0. Make sure you have completed the development environment setup described in [Quick Start](Quick_start.md).

1. Connect the camera-equipped T5AI development board to your computer, open Arduino IDE, select the `TUYA_T5AI` board, and choose the correct upload port.

> Note: T5AI series development boards provide dual serial port communication. When connected to a computer, two serial port numbers will be detected. UART0 is used for firmware flashing — please select the correct upload port in Arduino IDE.

2. In Arduino IDE, click `File` -> `Examples` -> `AI components` -> `08_AI_Video` to open the example code.

3. Click the upload button in the top-left corner of Arduino IDE to flash the code. The following messages in the terminal indicate a successful flash.

```bash
[INFO]: Write flash success
[INFO]: CRC check success
[INFO]: Reboot done
[INFO]: Flash write success.
```

> This example has no device authorization code to configure and does not connect to the network or the Tuya cloud — it only exercises the local `ai_video` component.

Alternatively, compile from the command line:

```bash
arduino-cli compile --fqbn tuya_open:tuya_open:t5 08_AI_Video.ino
```

## Running the Example

Open the Arduino IDE built-in Serial Monitor (baud rate 115200) after flashing. `setup()` calls `Log.begin()` and the sketch logs through the `PR_NOTICE`/`PR_ERR` macros.

Based on the literal `PR_NOTICE`/`PR_ERR` calls in `08_AI_Video.ino`, `setup()` first prints a startup banner:

```
============ Tuya AI Video ==============
Compile time:        <build date, from __DATE__>
```

If `TuyaVideo.begin()` or `TuyaVideo.start()` fails, the corresponding message is printed and the sketch halts in an infinite `delay(1000)` loop:

```
Failed to initialize AI video
```

```
Failed to start AI video
```

Otherwise, `loop()` polls for a frame every 100 ms and, for every frame it successfully obtains, prints:

```
JPEG frame length: <bytes>
```

## Example Code Explanation

### Hardware Registration

```cpp
board_register_hardware();
```

Registers the onboard hardware (including the camera) via `board_com_api.h` before the video pipeline is initialized.

### Initialization and Start

```cpp
if (OPRT_OK != TuyaVideo.begin()) { ... }
if (OPRT_OK != TuyaVideo.start()) { ... }
```

`TuyaVideo.begin()` initializes the `ai_video` component and is idempotent — it returns `OPRT_OK` immediately if already initialized. `TuyaVideo.start()` then starts the video pipeline; the sketch treats either call failing as fatal and stops in a `delay(1000)` loop.

### Frame Polling Loop

```cpp
uint8_t *jpegData = nullptr;
uint32_t jpegLen  = 0;

if (OPRT_OK == TuyaVideo.getJpegFrame(&jpegData, &jpegLen)) {
    PR_NOTICE("JPEG frame length: %u", jpegLen);
}

TuyaVideo.freeJpegFrame(&jpegData);

delay(100);
```

`TuyaVideo.getJpegFrame()` returns the latest JPEG frame, if one is available. In this example, `TuyaVideo.freeJpegFrame(&jpegData)` is called on every loop iteration, regardless of whether `getJpegFrame()` returned `OPRT_OK`. Every frame that was successfully obtained from `getJpegFrame()` must eventually be released via `freeJpegFrame()` to avoid leaking frame buffers.

### YUV Frame Callback (not used in this example)

`TuyaVideo` also exposes a raw-frame callback that this example does not register:

```cpp
OPERATE_RET setYuvFrameCallback(AI_VIDEO_FLUSH_CB cb);
```

It is documented below for completeness since it is part of the `TuyaVideo` API surface.

### TuyaVideo API

| Method | Signature | Description |
| --- | --- | --- |
| `begin` | `OPERATE_RET begin()` | Initialize the AI video component (idempotent) |
| `end` | `void end()` | Stop the pipeline if running and clear the initialized flag (upstream `ai_video_input.h` has no deinit symbol) |
| `isInitialized` | `bool isInitialized()` | Check whether the component is initialized |
| `start` | `OPERATE_RET start()` | Start the video pipeline |
| `stop` | `OPERATE_RET stop()` | Stop the video pipeline |
| `getJpegFrame` | `OPERATE_RET getJpegFrame(uint8_t **data, uint32_t *len)` | Get the latest JPEG frame; call `freeJpegFrame()` once done |
| `freeJpegFrame` | `OPERATE_RET freeJpegFrame(uint8_t **data)` | Free a JPEG frame previously returned by `getJpegFrame()` |
| `setYuvFrameCallback` | `OPERATE_RET setYuvFrameCallback(AI_VIDEO_FLUSH_CB cb)` | Register a callback invoked for each raw YUV frame |

All methods except `end()` and `isInitialized()` return `OPRT_INVALID_PARM` if the component has not been initialized (or, for `getJpegFrame`/`freeJpegFrame`/`setYuvFrameCallback`, if the pointer/callback arguments are null).

## Related Documentation

- [TuyaOpen Official Website](https://tuyaopen.ai)
- [GitHub Repository](https://github.com/tuya/TuyaOpen)
