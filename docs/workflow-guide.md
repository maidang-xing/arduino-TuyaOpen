# GitHub 工作流说明文档

本仓库配置了两个 GitHub Actions 工作流，用于自动编译检查 Arduino 示例项目。

---

## 工作流概览

| 工作流 | 文件 | 触发方式 | 用途 |
|--------|------|----------|------|
| Compile Check | `.github/workflows/compile-check.yml` | push / PR 到 main 分支 | 自动编译检查 |
| Manual Compile Test | `.github/workflows/manual-compile-test.yml` | 手动触发 | 按需编译测试 |

---

## 1. Compile Check（自动编译检查）

### 触发条件

- 向 `main` 分支推送代码 (`push`)
- 向 `main` 分支创建拉取请求 (`pull_request`)

### 工作流程

1. **解析开发板** — 在 Ubuntu 环境中从 `boards.txt` 自动提取所有开发板 ID
2. **编译示例** — 在 Windows 环境中，为每个开发板并行编译指定示例
3. **输出结果** — 汇总所有开发板的编译结果

### 编译内容

- **开发板**：从 `boards.txt` 动态读取，当前包含：
  - `t2` (T2)
  - `t3` (T3)
  - `t5` (T5)
  - `TUYA_T5AI_BOARD`
  - `TUYA_T5AI_CORE`
  - `XH_WB5E` (XH WB5E)
  - `ln882h` (LN882H)
  - `esp32` (ESP32)
- **示例**：`libraries/AIcomponents/examples/00_IoT_SimpleExample/00_IoT_SimpleExample.ino`
- **FQBN 格式**：`tuya_open:tuya_open:<board_id>`

### 编译环境

- 运行平台：`windows-latest`
- 工具：`arduino-cli`（自动从官方下载最新版本）
- 开发板包来源：[package_tuya_open_index.json](https://github.com/maidang-xing/arduino-TuyaOpen/releases/download/global/package_tuya_open_index.json)

---

## 2. Manual Compile Test（手动编译测试）

### 触发方式

在 GitHub 仓库页面手动触发。

### 操作步骤

1. 打开仓库的 GitHub 页面
2. 点击顶部导航栏的 **Actions** 标签
3. 在左侧工作流列表中选择 **Manual Compile Test**
4. 点击右侧的 **Run workflow** 按钮
5. 填写参数（可选）：
   - **Board to compile**：指定要编译的开发板 ID（留空则编译所有开发板）
   - **Sketch path**：指定要编译的示例路径（默认为 `libraries/AIcomponents/examples/00_IoT_SimpleExample/00_IoT_SimpleExample.ino`）
6. 点击绿色的 **Run workflow** 按钮启动

### 参数说明

| 参数 | 是否必填 | 默认值 | 说明 |
|------|----------|--------|------|
| `board` | 否 | 空（编译全部） | 指定单个开发板 ID，如 `t2`、`esp32` 等 |
| `sketch` | 否 | `libraries/AIcomponents/examples/00_IoT_SimpleExample/00_IoT_SimpleExample.ino` | 要编译的 `.ino` 文件路径 |

### 使用示例

- **编译所有开发板**：两个输入框均使用默认值，直接点击 Run workflow
- **仅编译 T2 开发板**：在 Board 输入框填写 `t2`
- **编译自定义示例**：修改 Sketch path 为目标 `.ino` 文件路径

---

## 查看编译结果

1. 进入 **Actions** 页面
2. 点击对应的工作流运行记录
3. 查看各开发板的编译 Job：
   - ✅ 绿色勾表示编译成功
   - ❌ 红色叉表示编译失败，点击进入查看详细日志
4. 在运行记录底部的 **Summary** 中可查看编译结果汇总

---

## 添加新开发板

如需支持新的开发板，只需在 `boards.txt` 中添加对应配置即可。工作流会自动从 `boards.txt` 解析所有开发板 ID，无需修改工作流文件。

---

## 工作流架构图

```
┌─────────────────────────────────────────────────────┐
│               Compile Check / Manual Test           │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌───────────────┐                                  │
│  │  get-boards   │  (Ubuntu)                        │
│  │  解析 boards  │  从 boards.txt 提取开发板列表     │
│  └───────┬───────┘                                  │
│          │ board-list                                │
│          ▼                                          │
│  ┌───────────────┐  并行执行 (Windows)               │
│  │   compile     │  ┌─────┐ ┌─────┐ ┌─────┐        │
│  │   (matrix)    │  │ t2  │ │ t3  │ │ t5  │ ...    │
│  │               │  └─────┘ └─────┘ └─────┘        │
│  └───────┬───────┘                                  │
│          │                                          │
│          ▼                                          │
│  ┌───────────────┐                                  │
│  │compile-result │  (Ubuntu)                        │
│  │  结果汇总     │  输出 Summary                     │
│  └───────────────┘                                  │
│                                                     │
└─────────────────────────────────────────────────────┘
```
