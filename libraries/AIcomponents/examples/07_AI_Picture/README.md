# On-Device Photo Album (AI Picture)

This example demonstrates how to use the **TuyaPicture** module in the Arduino-TuyaOpen framework to initialize the on-device photo album (`ai_picture`) provided by the TuyaOpen SDK and read the current album name.

> This example only supports the TUYA_T5AI platform. `TuyaPicture` wraps the upstream `ai_picture` component, which saves JPEG pictures into an **on-device photo album** — it is **not** a cloud upload. Upstream `ai_picture` is disabled by default (`CONFIG_ENABLE_COMP_AI_PICTURE` defaults to `n`); this repository's T5 vendor package config (`tools/ci/config/t5/app_default.config`) turns it on, so this example only works against a vendor package built from that configuration.

> This example does **not** capture a photo. It only initializes `TuyaPicture` and prints the album name — saving a JPEG (e.g. one captured with the Camera library) is intentionally left as a comment in the sketch instead of being invoked.

### Project File Structure

```
07_AI_Picture/
└── 07_AI_Picture.ino    # Main program entry; initializes the on-device photo album and prints its name
```

## Flashing Procedure

0. Make sure you have completed the development environment setup described in [Quick Start](Quick_start.md).

1. Connect the T5AI development board to your computer, open Arduino IDE, select the `TUYA_T5AI` board, and choose the correct upload port.

> Note: T5AI series development boards provide dual serial port communication. When connected to a computer, two serial port numbers will be detected. UART0 is used for firmware flashing — please select the correct upload port in Arduino IDE.

2. In Arduino IDE, click `File` -> `Examples` -> `AI components` -> `07_AI_Picture` to open the example code.

3. Click the upload button in the top-left corner of Arduino IDE to flash the code. The following messages in the terminal indicate a successful flash.

```bash
[INFO]: Write flash success
[INFO]: CRC check success
[INFO]: Reboot done
[INFO]: Flash write success.
```

> This example has no device authorization code to configure and does not connect to the network or the Tuya cloud — it only exercises the local `ai_picture` component.

Alternatively, compile from the command line:

```bash
arduino-cli compile --fqbn tuya_open:tuya_open:t5 07_AI_Picture.ino
```

## Running the Example

Open the Arduino IDE built-in Serial Monitor (baud rate 115200) after flashing. `setup()` runs once, initializes `TuyaPicture`, and prints the album name; `loop()` then simply delays.

Based on the literal `Serial.print`/`Serial.println` calls in `07_AI_Picture.ino`, a successful run prints:

```
Album name: <name returned by TuyaPicture.albumName()>
```

If `TuyaPicture.begin()` fails, it instead prints:

```
TuyaPicture.begin failed, rt=<error code>
```

> The album name string is produced by the upstream `ai_picture` component at runtime; this document does not assume a specific value.

## Example Code Explanation

### Initialization

```cpp
OPERATE_RET rt = TuyaPicture.begin();
```

`TuyaPicture.begin()` initializes the `ai_picture` module. It is idempotent — calling it again after a successful `begin()` also returns `OPRT_OK`.

### Reading the Album Name

```cpp
Serial.println(TuyaPicture.albumName());
```

`TuyaPicture.albumName()` returns the name of the currently active on-device album.

### Saving a Picture (not exercised by this example)

The sketch leaves the save path as a comment, since it does not itself produce JPEG bytes:

```cpp
// A real caller feeds JPEG bytes (e.g. captured from the Camera
// library) into saveToAlbum(), for example:
//
//   char savedName[64] = {0};
//   OPERATE_RET rt = TuyaPicture.saveToAlbum(jpegData, jpegLen, nullptr,
//                                            savedName, sizeof(savedName));
```

A real caller would obtain JPEG bytes from another source (e.g. the Camera library) and pass them to `saveToAlbum()`. The wrapper always uses an internal buffer sized `AI_PICTURE_NAME_MAX_LEN + 1` bytes (`AI_PICTURE_NAME_MAX_LEN` is 64) when talking to upstream, then copies the resulting filename into the caller-supplied `outName` buffer, truncated to `outNameSize` bytes.

### TuyaPicture API

| Method | Signature | Description |
| --- | --- | --- |
| `begin` | `OPERATE_RET begin()` | Initialize the picture album module (idempotent) |
| `end` | `void end()` | Clear the local initialized flag (upstream `ai_picture` has no deinit symbol) |
| `isInitialized` | `bool isInitialized()` | Check whether the module is initialized |
| `albumName` | `const char *albumName()` | Get the name of the current album |
| `saveToAlbum` | `OPERATE_RET saveToAlbum(const uint8_t *data, uint32_t len, const char *hint = nullptr, char *outName = nullptr, uint32_t outNameSize = 0)` | Save a JPEG picture to the on-device album |

`saveToAlbum()` parameters:

| Parameter | Description |
| --- | --- |
| `data` | JPEG data buffer |
| `len` | JPEG data length in bytes |
| `hint` | Desired filename; `NULL` (the default) auto-generates a timestamp-based name |
| `outName` | Optional buffer to receive the actual filename used |
| `outNameSize` | Size of the `outName` buffer |

Returns `OPRT_OK` on success, `OPRT_INVALID_PARM` on bad arguments (module not initialized, `data` is null, or `len` is 0).

## Related Documentation

- [TuyaOpen Official Website](https://tuyaopen.ai)
- [GitHub Repository](https://github.com/tuya/TuyaOpen)
