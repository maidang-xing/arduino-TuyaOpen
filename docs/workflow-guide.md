# Arduino-TuyaOpen CI/CD 工作流指南

本文档介绍仓库中的三个 GitHub Actions 工作流的工作流程和使用方法。

---

## 目录

- [1. Compile Check（自动编译检查）](#1-compile-check自动编译检查)
- [2. Manual Compile Test（手动编译测试）](#2-manual-compile-test手动编译测试)
- [3. Code Format Check（代码格式检查）](#3-code-format-check代码格式检查)
- [支持的开发板](#支持的开发板)

---

## 1. Compile Check（自动编译检查）

**文件**: `.github/workflows/compile-check.yml`

**触发条件**: 向 `main` 分支推送代码或发起 Pull Request 时自动触发。

### 工作流程

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   get-boards        │────▶│   compile           │────▶│   compile-result    │
│   (解析开发板矩阵)   │     │   (每板一个 Job)     │     │   (结果汇总)         │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

**Job 1 — get-boards（Windows）**
1. 检出代码仓库
2. 解析 `boards.txt`，提取所有 `<board>.build.chip` 条目，生成开发板→芯片映射
3. 扫描 `libraries/` 目录下所有 `.ino` 示例文件
4. 按编译规则构建 **按开发板维度** 的编译矩阵：
   - **T5 系列开发板**（chip=T5）：将**全部**示例路径以分号拼接
   - **非 T5 开发板**：仅包含 `00_IoT_SimpleExample`
5. 输出 JSON 格式的编译矩阵（每个开发板一个 Job）

**Job 2 — compile（Windows，每板一个 Job）**
1. 检出代码仓库
2. 下载并配置 `arduino-cli`（**仅下载安装一次**）
3. 安装 `tuya_open:tuya_open` 核心（**仅安装一次**）
4. 验证核心安装成功
5. 循环编译该开发板对应的所有 sketch，输出进度 `[1/N] Compiling: ...`
6. 统计失败数量，全部通过后输出成功信息

**Job 3 — compile-result（Windows）**
1. 汇总所有编译结果，输出到 GitHub Step Summary
2. 显示编译成功/失败状态

---

## 2. Manual Compile Test（手动编译测试）

**文件**: `.github/workflows/manual-compile-test.yml`

**触发条件**: 手动触发（workflow_dispatch），支持自定义参数。

### 输入参数

| 参数 | 必填 | 默认值 | 说明 |
|------|------|--------|------|
| `board` | 否 | 空（全部开发板） | 指定编译的开发板 ID，如 `t3`、`esp32` |
| `sketch` | 否 | `00_IoT_SimpleExample` | 指定编译的 Sketch 路径 |

### 工作流程

与 Compile Check 流程一致（get-boards → compile → compile-result），区别：

- **指定了 board**：仅编译该开发板
- **指定了 sketch**：所有开发板都编译指定的 Sketch（覆盖 T5 全量规则）
- **均未指定**：与自动编译检查行为一致

### 使用方式

1. 进入 GitHub 仓库 → **Actions** 选项卡
2. 左侧选择 **Manual Compile Test**
3. 点击 **Run workflow**
4. 填写参数后运行

---

## 3. Code Format Check（代码格式检查）

**文件**: `.github/workflows/code-format-check.yml`

**触发条件**: 向 `main` 分支发起 PR 且涉及 C/C++ 文件变更时触发。

**监控文件类型**: `*.c` `*.cpp` `*.h` `*.hpp` `*.cc` `*.cxx`

### 工作流程

```
┌────────────────────────────────────────────────┐
│   format-check (Ubuntu)                        │
│                                                │
│   1. 检出代码 + 获取基准分支                      │
│   2. 安装 clang-format-14                       │
│   3. 运行 python script/check_format.py         │
│   4. 输出检查结果到 Step Summary                  │
└────────────────────────────────────────────────┘
```

**检查内容**:
- **clang-format 风格检查** — 基于 `.clang-format` 配置文件
- **中文字符检测** — 检查是否包含中文字符
- **文件头校验** — 检查 `@file`、`@brief`、`@copyright` 等标准头部

### 本地修复方式

```bash
# 自动格式化文件
clang-format -style=file -i <file>

# 本地运行检查
python script/check_format.py --debug --files <file>
```

---

## 支持的开发板

以下开发板从 `boards.txt` 中动态解析，无需手动维护列表：

| 开发板 ID | 芯片类型 | 编译范围 |
|-----------|---------|---------|
| t2 | t2 | 仅 SimpleExample |
| t3 | t3 | 仅 SimpleExample |
| t5 | T5 | 全部示例 |
| TUYA_T5AI_BOARD | T5 | 全部示例 |
| TUYA_T5AI_CORE | T5 | 全部示例 |
| XH_WB5E | T5 | 全部示例 |
| ln882h | ln882h | 仅 SimpleExample |
| esp32 | esp32 | 仅 SimpleExample |

> 新增开发板时只需在 `boards.txt` 中添加配置，工作流会自动识别并加入编译矩阵。

---

## 运行环境

| 工作流 | 运行系统 | 原因 |
|--------|---------|------|
| Compile Check | Windows | tuya_open 核心编译链仅支持 Windows |
| Manual Compile Test | Windows | 同上 |
| Code Format Check | Ubuntu | clang-format 在 Linux 上安装更方便，且不依赖编译链 |

## 关键设计

### 一次安装，多次编译

编译矩阵按 **开发板维度** 展开（非 board × sketch），每个开发板启动一个 Job：
- arduino-cli 和 tuya_open 核心只下载安装一次
- 该 board 对应的所有 sketch 在同一 Job 内循环编译
- 大幅减少重复下载和安装时间

### Board Manager URL

```
https://github.com/tuya/arduino-tuyaopen/releases/download/global/package_tuya_open_index.json
```

### FQBN 格式

```
tuya_open:tuya_open:<board_id>
```

示例：`tuya_open:tuya_open:t3`、`tuya_open:tuya_open:esp32`
