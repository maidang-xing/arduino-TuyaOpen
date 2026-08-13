# 本地相册（AI Picture）

本示例演示如何使用 Arduino-TuyaOpen 框架中的 **TuyaPicture** 模块，初始化 TuyaOpen SDK 提供的本地相册（`ai_picture`）组件，并读取当前相册名称。

> 本示例仅支持 TUYA_T5AI 平台。`TuyaPicture` 封装了上游的 `ai_picture` 组件，该组件将 JPEG 图片保存到**设备本地相册**中——这**不是**云端上传功能。上游 `ai_picture` 默认关闭（`CONFIG_ENABLE_COMP_AI_PICTURE` 默认值为 `n`）；本仓库的 T5 vendor 包配置（`tools/ci/config/t5/app_default.config`）已开启该选项，因此本示例只能配合基于该配置构建的 vendor 包使用。

> 本示例**不会**拍摄照片。它仅初始化 `TuyaPicture` 并打印相册名称；将 JPEG 数据（例如通过 Camera 库拍摄得到）保存到相册的调用被有意保留为注释，而没有实际执行。

### 项目文件结构

```
07_AI_Picture/
└── 07_AI_Picture.ino    # 主程序入口，初始化本地相册并打印相册名称
```

## 代码烧录流程

0. 确保已完成[快速开始](Quick_start.md)中开发环境的搭建。

1. 连接 T5AI 开发板到电脑，打开 Arduino IDE，选择 `TUYA_T5AI` 开发板，并选择正确的烧录端口。

> 注意：T5AI 系列开发板提供双串口通信，连接电脑后会检测到两个串口号，其中 UART0 用于固件烧录，请在 Arduino IDE 中选择正确的烧录口。

2. 在 Arduino IDE 中点击 `文件` -> `示例` -> `AI components` -> `07_AI_Picture`，打开示例代码。

3. 点击 Arduino IDE 左上角按钮烧录代码，终端出现以下信息说明烧录成功。

```bash
[INFO]: Write flash success
[INFO]: CRC check success
[INFO]: Reboot done
[INFO]: Flash write success.
```

> 本示例无需配置设备授权码，也不会连接网络或涂鸦云——它只是本地调用 `ai_picture` 组件。

也可以通过命令行编译：

```bash
arduino-cli compile --fqbn tuya_open:tuya_open:t5 07_AI_Picture.ino
```

## 运行示例

烧录完成后，打开 Arduino IDE 自带的串口监视器（波特率 115200）。`setup()` 只运行一次，负责初始化 `TuyaPicture` 并打印相册名称；随后 `loop()` 仅执行延时。

根据 `07_AI_Picture.ino` 中字面的 `Serial.print`/`Serial.println` 调用，运行成功时会打印：

```
Album name: <TuyaPicture.albumName() 返回的名称>
```

若 `TuyaPicture.begin()` 失败，则会打印：

```
TuyaPicture.begin failed, rt=<错误码>
```

> 相册名称字符串由上游 `ai_picture` 组件在运行时生成，本文档不假设具体的值。

## 示例代码说明

### 初始化

```cpp
OPERATE_RET rt = TuyaPicture.begin();
```

`TuyaPicture.begin()` 初始化 `ai_picture` 模块。该调用是幂等的——在 `begin()` 成功后再次调用同样会返回 `OPRT_OK`。

### 读取相册名称

```cpp
Serial.println(TuyaPicture.albumName());
```

`TuyaPicture.albumName()` 返回当前生效相册的名称。

### 保存照片（本示例未执行）

由于本示例本身不产生 JPEG 数据，保存照片的调用被保留为注释：

```cpp
// A real caller feeds JPEG bytes (e.g. captured from the Camera
// library) into saveToAlbum(), for example:
//
//   char savedName[64] = {0};
//   OPERATE_RET rt = TuyaPicture.saveToAlbum(jpegData, jpegLen, nullptr,
//                                            savedName, sizeof(savedName));
```

实际使用时，调用方需要从其他来源（例如 Camera 库）获取 JPEG 数据，再传给 `saveToAlbum()`。封装层与上游交互时始终使用大小为 `AI_PICTURE_NAME_MAX_LEN + 1` 字节（`AI_PICTURE_NAME_MAX_LEN` 为 64）的内部缓冲区，随后将得到的文件名拷贝到调用方提供的 `outName` 缓冲区中，并按 `outNameSize` 截断。

### TuyaPicture 接口

| 接口 | 签名 | 说明 |
| --- | --- | --- |
| `begin` | `OPERATE_RET begin()` | 初始化相册模块（幂等） |
| `end` | `void end()` | 清除本地初始化标志（上游 `ai_picture` 没有反初始化接口） |
| `isInitialized` | `bool isInitialized()` | 检查模块是否已初始化 |
| `albumName` | `const char *albumName()` | 获取当前相册名称 |
| `saveToAlbum` | `OPERATE_RET saveToAlbum(const uint8_t *data, uint32_t len, const char *hint = nullptr, char *outName = nullptr, uint32_t outNameSize = 0)` | 将 JPEG 图片保存到本地相册 |

`saveToAlbum()` 参数说明：

| 参数 | 说明 |
| --- | --- |
| `data` | JPEG 数据缓冲区 |
| `len` | JPEG 数据长度（字节） |
| `hint` | 期望的文件名；传 `NULL`（默认值）则自动生成基于时间戳的文件名 |
| `outName` | 可选，用于接收实际使用的文件名的缓冲区 |
| `outNameSize` | `outName` 缓冲区的大小 |

成功时返回 `OPRT_OK`，参数非法时（模块未初始化、`data` 为空指针或 `len` 为 0）返回 `OPRT_INVALID_PARM`。

## 相关文档

- [TuyaOpen 官网](https://tuyaopen.ai)
- [Github 代码库](https://github.com/tuya/TuyaOpen)
