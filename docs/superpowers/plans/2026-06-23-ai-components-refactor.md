# AI 组件重构与新组件接入实施计划 (v3)

> **v3 修订说明（2026-08-11，基于对 v2 的实测复核）**
>
> v2 的核心前提**成立**：上游 master 确已把 AI 组件迁到 `src/ai_components/`（`src/ai_components/CMakeLists.txt`、`Kconfig`、`ai_picture/`、`ai_video/`、`assets/` 均存在），`apps/tuya.ai/ai_components/` 已删除（HTTP 404）。
>
> 但 v2 有 3 处会直接卡住执行的错误，v3 已修正：
>
> 1. **任务 3 方向反了。** 上游 `lang_config.h`（64 行）是本仓库版本（43 行）的**严格超集**，本仓库无任何独有宏，上游多出 19 个宏（`ALBUM` / `TAKE_PHOTO` / `THINKING` / `UPLOADING` / `PRINT_*` / `VIEW_IMAGE` …），恰好是 ai_picture 相册 UI 所需。v2 要加的 `ignore_filenames` 会让这 19 个宏永远不可用。且 v2"上游文件覆盖本仓库文件"的机制描述不成立——`copy_include_dir_from_file` 只写 vendor 包目录，从不碰 `libraries/`，真实机制是 include 搜索顺序。**v3 改为：把本仓库 `lang_config.h` 对齐成上游超集，不引入 ignore 机制。**
> 2. **本地验证链路断了。** `generate_index_json.py:82,161` 无条件生成 `https://github.com/tuya/arduino-TuyaOpen/releases/download/<version>/…`，用 `0.0.0-dev` 生成的 index 指向不存在的 release，`arduino-cli core install` 必然 404。CI 能跑是因为先上传 assets 再生成 index。**v3 增加任务 0：给脚本加 `--base-url`，用本地 HTTP server 托管产物。**
> 3. **任务 6 前提错误。** 上游 `ai_picture/Kconfig` 是 `default n` 且无人 `select`；且 `package_platform.py:57-59` 的 `set_platform_ini` 会**整体覆盖**上游 `app_default.config`，真正生效的只有本仓库 `tools/ci/config/t5/app_default.config`（其中有 `CONFIG_ENABLE_COMP_AI_VIDEO=y`，但**没有** AI_PICTURE）。**v3 把开关前置到任务 4，不再作为失败兜底。**
>
> 另有 7 处事实性校正，见下方各任务的「v3 校正」小节。

**目标：** 清理 packager 中已失效的 AI 组件专项补丁；把本仓库 `lang_config.h` 与上游对齐；固定 LVGL 版本与 AI 组件开关；在保持现有 Arduino 类公开 API 100% 不变的前提下，为上游新增的 `ai_picture` / `ai_video` 加 Arduino 包装。

**技术栈：** C/C++ (Arduino + TuyaOpen)、Python 3 (打包脚本)、`arduino-cli`、CMake、`tos.py`、`clang-format-14`。

## 全局约束

- **Arduino 公开 API 不得修改。** 下列类的所有 `public` 成员函数签名必须保持兼容：`TuyaAIClass` / `TuyaAudioClass` / `TuyaMCPClass` / `TuyaSkillClass` / `TuyaUIClass`（`libraries/AIcomponents/src/`）、`Audio`（`libraries/Audio/src/Audio.h`）、`Camera`（`libraries/Camera/src/dvpCamera.h`）、`Display`（`libraries/Display/src/Display.h`）。
- 新组件一律**新建独立类**（`TuyaPicture` / `TuyaVideo`），不动既有类。
- **代码风格：** clang-format-14 + `.clang-format`；任何修改过的 C/C++ 文件（`.c/.cpp/.h/.hpp/.cc/.cxx/.ino`）须通过 `python script/check_format.py --debug --files <file>`；禁止中文字符；文件头须含 `@file` / `@brief` / `@copyright`。
  > ⚠️ `check_format.py` 只检查 C/C++ 后缀（`script/check_format.py:93`），对 `.py` 文件是空操作，不要在 Python 改动后调用它。
- **目标板：T5**（`tools/ci/package-config.json` 中唯一 `enabled: true`）。回归边界 = T5 板上所有 AI/Audio/Camera/Display 示例都能编。
- **版本号联动：** 收尾时同步 `cores/tuya_open/tuya_arduino_version.h`（`PATCH 1 → 2`）、`platform.txt`（`version=1.2.1 → 1.2.2`）、`package.json` / `package_cn.json`。
- **上游是移动靶：** `package-config.json` 的 `sourceBranch` 是 `master`，不是固定 commit。v3 的所有上游结论核对于 2026-08-11 的 master。

---

## 已完成的上游调研（原任务 1，v3 已实测填表，无需重跑）

| 组件 | 上游 Kconfig 默认 | 决策 | 依据 |
|---|---|---|---|
| `ai_picture` | `ENABLE_COMP_AI_PICTURE` **default n**，无人 select，`select ENABLE_IMAGE_ALBUM` | **纳入 → `TuyaPicture`** | API 稳定且简单；须在任务 4 显式开开关 |
| `ai_video` | `ENABLE_COMP_AI_VIDEO` **default n**，但本仓库 config 已有 `=y` | **纳入 → `TuyaVideo`** | 开关已就绪，T5+camera 板有意义 |
| `ai_mode_hold` / `ai_mode_oneshot` | 在 `ai_mode/Kconfig` 下 | **暂缓** | 更适合作为 `TuyaAI` 的新方法；本次不动 `TuyaAI.h` |
| `ai_agent` | — | **跳过** | 与现有 `tuya_ai_service/svc_ai_agent` 语义重叠 |
| `svc_ai_basic` / `svc_ai_codec` / `svc_ai_monitor` | include 路径本就已在包内 | **跳过** | 偏底层服务，Arduino 用户极少直接用 |

**上游实测 API（2026-08-11 master，已逐字核对）：**

```c
/* src/ai_components/ai_picture/include/ai_picture.h  —— 注意它 #include "image_album.h" */
#define AI_PICTURE_NAME_MAX_LEN 64      /* 或 ALBUM_FILENAME_MAX_LEN */
OPERATE_RET ai_picture_init(void);
OPERATE_RET ai_picture_save_to_album(uint8_t *picture, uint32_t len, const char *in_name,
                                     char name[AI_PICTURE_NAME_MAX_LEN + 1]);
char       *ai_picture_get_album_name(void);
/* 无 deinit 符号 */

/* src/ai_components/ai_video/include/ai_video_input.h */
typedef void (*AI_VIDEO_FLUSH_CB)(TDL_CAMERA_FRAME_T *frame);
OPERATE_RET ai_video_init(void);
OPERATE_RET ai_video_start(void);
OPERATE_RET ai_video_stop(void);
OPERATE_RET ai_video_get_jpeg_frame(uint8_t **image_data, uint32_t *image_data_len);
OPERATE_RET ai_video_jpeg_image_free(uint8_t **image_data);
OPERATE_RET ai_video_set_yuv_frame_flush_cb(AI_VIDEO_FLUSH_CB cb);   /* 返回 OPERATE_RET，v2 误写为 void */
```

---

## 文件结构

### 需要修改的文件

| 路径 | 改动性质 |
|---|---|
| `tools/ci/generate_index_json.py` | 新增 `--base-url`，使本地产物可被 `arduino-cli` 安装（任务 0） |
| `tools/ci/packager/package_platform_t5.py` | **删除** `copy_ai_components_libs` / `copy_ai_components_headers` / `update_include_tuya_open_with_ai_components` 三个方法（L107-199）及 `package()` 中的调用（L481-485） |
| `libraries/AIcomponents/src/lang_config.h` | 对齐上游 64 行超集（补 19 个宏），并补齐 `@file/@brief/@copyright` 文件头 |
| `tools/ci/config/t5/app_default.config` | 追加 `CONFIG_ENABLE_COMP_AI_PICTURE=y` 与 LVGL 版本锁定 |

### 需要新建的文件

| 路径 | 职责 |
|---|---|
| `libraries/AIcomponents/src/TuyaPicture.{h,cpp}` | 包装 `src/ai_components/ai_picture/`（**设备端相册存储**，不是云上传） |
| `libraries/AIcomponents/src/TuyaVideo.{h,cpp}` | 包装 `src/ai_components/ai_video/` |
| `libraries/AIcomponents/examples/07_AI_Picture/07_AI_Picture.ino` | 示例 sketch |
| `libraries/AIcomponents/examples/08_AI_Video/08_AI_Video.ino` | 示例 sketch |

### 不需要改的文件

- `libraries/AIcomponents/src/Tuya{AI,Audio,MCP,Skill,UI}.cpp`、`libraries/{Audio,Camera,Display}/src/*` —— API 签名未变（由任务 5 编译回归验证）
- `CMakeLists.txt` —— **v3 校正：** 根 CMakeLists 的 glob 列表**根本不包含** `libraries/AIcomponents/`（只有 BLE/FS/HTTPClient/LittleFS/Log/MQTTClient/SPI/Ticker/TuyaIoT，见 `CMakeLists.txt:34-54`）。AIcomponents 只走 Arduino 路径，所以确实不用改——但不是 v2 说的"被 glob 自动捕获"。
- `tools/ci/packager/package_platform.py` —— **v3 删除了 v2 的 `ignore_filenames` 改造**（见任务 3）

---

## 任务 0：打通本地验证链路（**新增，阻塞后续所有编译验证**）

**目标：** 让本地打出的 vendor 包能真正被 `arduino-cli` 安装。

**Files:**
- Modify: `tools/ci/generate_index_json.py`
- Create: `docs/superpowers/plans/notes/local-verify.md`

- [ ] **Step 0.1：给 `generate_index_json.py` 加 `--base-url`**

现状 `make_download_url(owner, repo, tag, filename)` 返回硬编码的 GitHub release URL（L32-33），被 L82/L88/L161 使用。改造要求：

- 新增可选参数 `--base-url`（默认 `None`）。
- 给定时，所有产物 URL 变为 `{base_url}/{filename}`（去掉重复斜杠）；未给定时**行为完全不变**（CI 路径不受影响）。
- 实现方式建议：把 `base_url` 作为 `make_download_url` 的可选形参透传，不要改函数名或调用点数量。

- [ ] **Step 0.2：确认/安装工具链**

```bash
python3 -c "import pytest" 2>/dev/null || pip install --user pytest
command -v arduino-cli || {
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$HOME/.local/bin sh
  export PATH="$HOME/.local/bin:$PATH"
}
arduino-cli version
```

- [ ] **Step 0.3：定点清理本地 clone（不要整个删）**

```bash
cd tools/ci/output/vendor-T5 && git clean -fd src/ai_components && cd -
```

> **v3 实测更正：原先写的 `rm -rf tools/ci/output/vendor-T5` 是错的，代价极大。** 该 clone 共 4.0G，其中 `platform/` 占 2.7G，而 clone 自己的 `.gitignore:21` 是 `/platform/*/` —— 整个 `platform/` 是 **git 忽略的下载物**，含 694M 的 `platform/tools/gcc-arm-none-eabi-10.3-2021.10` 工具链和 150M 的安装包。`git reset --hard` / `checkout` **根本不会碰它**，所以就地更新可以完整保留工具链；整个删掉则要重新下载 2.7G。
>
> 真正需要清理的只有 28 个由旧 hack 生成的**未跟踪** `src/ai_components/*.h`（`assets/include/` 里恰好缺 `lang_config.h`，正是 hack `exclude_headers` 的指纹）。上游现在已跟踪 `src/ai_components/`，这些未跟踪文件会让 `git checkout` 冲突失败，进而触发 `package_handler.py:43` 的 `rmtree` 全量重 clone —— 定点 `git clean` 就能避免。
>
> clone 里另有 2194 个已跟踪文件的本地修改，由 packager 自己的 `git reset --hard origin/master` 处理，不用管。

- [ ] **Step 0.4：写本地验证 runbook**

把下面这套「打包 → 起 HTTP server → 生成 index → 装 core」的完整命令写进 `docs/superpowers/plans/notes/local-verify.md`，供任务 5/8 复用：

```bash
# 1) 打包（长任务：clone + submodule + tos build）
python tools/ci/package_release.py --version 0.0.0-dev --target t5
python tools/ci/package_release.py --version 0.0.0-dev --target arduino --checkout-path "$(pwd)"

# 2) 本地托管产物
cd output/0.0.0-dev && python3 -m http.server 8765 & cd -

# 3) 生成指向本地的 index
python tools/ci/generate_index_json.py \
  --base-index package.json --manifest output/0.0.0-dev/manifest.json \
  --version 0.0.0-dev --config tools/ci/package-config.json \
  --github-owner tuya --github-repo arduino-TuyaOpen \
  --base-url "http://127.0.0.1:8765" \
  --output /tmp/test_index.json

python tools/ci/validate_index.py --index /tmp/test_index.json \
  --version 0.0.0-dev --manifest output/0.0.0-dev/manifest.json

# 4) 安装
arduino-cli config init --overwrite
arduino-cli config add board_manager.additional_urls "file:///tmp/test_index.json"
arduino-cli core update-index
arduino-cli core install tuya_open:tuya_open
```

---

## 任务 1：修复 T5 配置腐烂 + 增加打包期符号校验（**v3 新增，阻塞 Stage B**）

**背景（实测）：** `package_platform.py:57-59` 的 `set_platform_ini` 会用本仓库的 `tools/ci/config/t5/app_default.config` **整体覆盖**上游的同名文件；而 kconfig 对**未知符号是静默忽略**的。两者叠加 = 上游每改一次符号名，vendor 包就静默少一块功能，且打包全程零告警。

用 2026-05-09 的旧 clone 定位每个符号的归属文件、再比对当前 master 的同一文件，确认 **3 个符号已失效**：

| 仓库现有配置 | 归属文件 | 当前 master | 后果 |
|---|---|---|---|
| `CONFIG_TUYA_T5AI_BOARD_EX_MODULE_35565LCD=y` | `boards/T5AI/TUYA_T5AI_BOARD/Kconfig` | 已改名 `TUYA_T5AI_BOARD_LCD_35565` | LCD choice 回落默认 `LCD_NONE`，连带丢掉 `select ENABLE_DISPLAY` / `ENABLE_TP` / `ENABLE_LVGL_DUAL_DISP_BUFF` → **vendor 包没有显示支持** |
| `CONFIG_ENABLE_EX_MODULE_CAMERA=y` | 同上 | 已改名 `TUYA_T5AI_BOARD_CAMERA`（`select ENABLE_CAMERA`） | 板级摄像头不注册 → 影响 Camera 库与 `TuyaVideo` |
| `CONFIG_LVGL_ENABLE_TP=y` | `src/liblvgl/Kconfig` | 已改名 `ENABLE_LVGL_TP` | 触摸关闭 |
| `CONFIG_BOARD_CHOICE_T5AI=y` | `boards/Kconfig` | ✅ 仍存在 | 无问题（但内层板型选择靠 choice 默认，建议一并钉死） |

> ⚠️ **必须在任务 2-8 的任何一次打包之前完成。** 否则 Stage B 会用一个没有显示支持的配置跑完整轮构建，随后的编译回归全线失败，且报错形态酷似"删 hack 删坏了"，排查方向会被彻底带偏。

**Files:**
- Modify: `tools/ci/config/t5/app_default.config`
- Modify: `tools/ci/packager/package_platform.py`（新增校验方法）
- Modify: `tools/ci/package-config.json`（临时 pin commit）

- [ ] **Step 1.1：替换失效符号**

把 3 行改成当前 master 的名字，并把板型显式钉死：

```
CONFIG_BOARD_CHOICE_TUYA_T5AI_BOARD=y
CONFIG_TUYA_T5AI_BOARD_LCD_35565=y
CONFIG_TUYA_T5AI_BOARD_CAMERA=y
CONFIG_ENABLE_LVGL_TP=y
```

（`CONFIG_BOARD_CHOICE_T5AI=y` 保留不动。）

- [ ] **Step 1.2：加打包期符号校验（治本）**

在 `package_platform.py` 新增 `verify_kconfig_applied()`，并在 `build_platform()` 成功返回前调用：

- 读 `{build_app_path}/app_default.config`（即真正生效的那一份）与 `{build_app_path}/.build/include/tuya_kconfig.h`（构建解析出的结果）。
- 对每条 `CONFIG_X=<value>`：断言 `tuya_kconfig.h` 里存在 `#define X`，且值一致（字符串/整数都要比）。
- 对每条 `# CONFIG_X is not set`：断言 `X` **未**被定义。
- 任一条不满足 → `logging.error` 打印全部失配项并返回 `False`，让打包**直接失败**而不是静默出包。

> 这一步是本次唯一能一劳永逸消掉"上游改名 → 静默少功能"这一整类问题的改动，价值高于任务 1.1 本身。

- [ ] **Step 1.3：临时把上游 pin 到固定 commit**

`tools/ci/package-config.json` 的 `platforms.t5.sourceBranch` 当前是 `master`（移动靶）。验证期间改成一个具体 commit，让 Stage B 的所有结论可复现；验证结束再决定是否改回。

- [ ] **Step 1.4：验证**

Step 1.1/1.3 是纯配置改动，静态可查；Step 1.2 的真正验收发生在 Stage B 第一次打包时——**如果校验一次都没报错，反而要怀疑它没被调用**，可故意插入一个假符号（如 `CONFIG_DEFINITELY_NOT_A_SYMBOL=y`）确认它能 fail，再删掉。

---

## 任务 2：删除 packager 中已失效的 AI 组件专项 hack

**v3 校正（动机）：** v2 说这些代码"会误把 src/ 下的头文件再复制一遍造成 include 路径重复"——**不对**。在新上游下它们是**无害死代码**：`copy_ai_components_headers` 在 L145-147 就因 `apps/tuya.ai/ai_components` 不存在而 `return []`，**早于** L157-159 的 `rmtree`；`copy_ai_components_libs` 同样在 L114-116 warning 后 `return True`。所以删除是**纯清理，零功能变化**——这反而让本任务风险极低。

**Files:**
- Modify: `tools/ci/packager/package_platform_t5.py`

- [ ] **Step 2.1：删除三个方法**（当前行号）
  - `copy_ai_components_libs(self, output_lib_path)` — L107-141
  - `copy_ai_components_headers(self, output_tmp_path)` — L143-178
  - `update_include_tuya_open_with_ai_components(self, include_paths)` — L180-199

- [ ] **Step 2.2：删除 `package()` 中的调用**（L481-485 整段）

```python
        logging.info("Processing ai_components...")
        self.copy_ai_components_libs(output_lib_path)
        ai_include_paths = self.copy_ai_components_headers(output_tmp_path)
        if ai_include_paths:
            self.update_include_tuya_open_with_ai_components(ai_include_paths)
```

紧随其后的 `if not self.copy_tuya_open(output_tmp_path): return False` 保留不变。

- [ ] **Step 2.3：静态自检**

```bash
python -m py_compile tools/ci/packager/package_platform_t5.py
grep -rn "ai_components" tools/ci/packager/*.py    # 预期：0 行
grep -n "^import\|^from" tools/ci/packager/package_platform_t5.py
```

若 `subprocess` / `shutil` 在删除后不再被该文件使用，一并清理 import；仍被使用则保留。

- [ ] **Step 2.4：打包后验证（依赖任务 0，可在任务 5 一并做）**

```bash
tar tjf output/0.0.0-dev/vendor-T5-*.tar.bz2 | grep -E 'src/ai_components' | head -30
```

预期能看到 `src/ai_components/*/include/*.h`。**v3 增强：** 头文件在≠符号在，必须同时验证符号：

```bash
mkdir -p /tmp/vpk && tar xjf output/0.0.0-dev/vendor-T5-*.tar.bz2 -C /tmp/vpk
for a in $(find /tmp/vpk -name '*.a'); do
  nm -A "$a" 2>/dev/null | grep -E ' T (ai_picture_init|ai_video_init|ai_chat_main)' && echo "  ^^ in $a"
done
```

若符号一个都找不到 → 旧 hack 的 `ar rcs` 合并确实还在承担链接职责，任务 2 需要回滚并改为"保留 libs 合并、只删 headers 部分"。

- [ ] **Step 2.5：commit**

```bash
git add tools/ci/packager/package_platform_t5.py
git commit -m "ci(t5): drop dead ai_components compatibility hack

Upstream moved AI components from apps/tuya.ai/ai_components into
src/ai_components/<sub>/include/, so these three methods now early-return
without doing anything. They are dead code kept alive only for the old
layout. Remove them; src/ai_components is picked up by get_include_flags
like any other SDK component."
```

---

## 任务 3：把本仓库 `lang_config.h` 对齐上游超集（**v3 方向已反转**）

**背景：** 实测 macro 集合 diff——上游 64 行版本 ⊃ 本仓库 43 行版本，本仓库**零独有宏**，上游独有 19 个：

```
ADD_IMAGE ALBUM ALL_PHOTOS CAMERA CANCEL CONFIRM_TEXT DELETE_TEXT NO_IMAGE
PRINT_FAILED PRINT_IMAGE PRINTING PRINT_SUCCESS PROVISIONING
RECOGNIZE_IMAGE_PROMPT SELECT_TEXT TAKE_PHOTO THINKING UPLOADING VIEW_IMAGE
```

v2 想用 `ignore_filenames` 把上游版本挡在包外，会让这 19 个宏永久缺失（且它们正是 ai_picture 相册 UI 要用的）。**v3 不改 `package_platform.py`，改为把本仓库版本补成超集。**

**Files:**
- Modify: `libraries/AIcomponents/src/lang_config.h`

- [ ] **Step 3.1：拉上游版本**

```bash
curl -s https://raw.githubusercontent.com/tuya/TuyaOpen/master/src/ai_components/assets/include/lang_config.h -o /tmp/up_lang.h
```

- [ ] **Step 3.2：合并**

要求：
- 保留本仓库现有的 43 行内所有宏**及其取值**（避免改变既有示例的显示文案）。
- 追加上游独有的 19 个宏。**⚠️ 实测校正：上游 master 的 `lang_config.h` 是 zh-CN 版本**（`#define LANG_CODE "zh-CN"`、`STANDBY "待命"`、`THINKING "思考中..."`），上游仓库里**没有** en-US 变体。因此只能沿用**宏名**，字符串值必须自行译成英文——直接照抄会违反本仓库"禁止中文字符"的 CI 规则。验收脚本比对的也只是宏名集合。
- 保留现有的 include guard `__LANGUAGE_CONFIG_H__`、`en_us` 定义、`extern "C"` 结构。
- **补齐文件头** `@file` / `@brief` / `@copyright`（现有文件只有 `// Auto-generated language config`，一旦被改动就会被 `check_format.py` 拦下）。
- 禁止中文字符。

> **由此暴露的一个新问题（必须在任务 5 验证）：** 上游是 zh-CN、本仓库是 en-US，两份 `lang_config.h` 现在**语义冲突**而不只是数量差异。哪一份赢由 include 搜索顺序决定（vendor 包的 `-iwithprefixbefore src/...` vs Arduino 库目录的 `-I`）。如果 vendor 包那份赢，**Arduino 示例的界面文案会变成中文**。任务 5 编译通过后，必须在预处理输出里确认实际取到的是哪一份：
>
> ```bash
> arduino-cli compile --fqbn tuya_open:tuya_open:t5 --preprocess \
>   libraries/AIcomponents/examples/YourChatBot/YourChatBot.ino 2>/dev/null | grep -m1 LANG_CODE
> # 或直接看编译后的 .elf 里有没有中文字符串
> ```
>
> 若上游那份赢，处理方式是把本仓库的 `lang_config.h` 改名（例如 `arduino_lang_config.h`）并在 Arduino 侧显式包含，而不是继续赌搜索顺序。

- [ ] **Step 3.3：验证宏集合已成超集**

```bash
curl -s https://raw.githubusercontent.com/tuya/TuyaOpen/master/src/ai_components/assets/include/lang_config.h \
  | grep -oE '^#define [A-Z_0-9]+' | awk '{print $2}' | sort > /tmp/up.txt
grep -oE '^#define [A-Z_0-9]+' libraries/AIcomponents/src/lang_config.h | awk '{print $2}' | sort > /tmp/loc.txt
comm -23 /tmp/up.txt /tmp/loc.txt      # 预期：0 行（本仓库不再缺任何上游宏）
```

- [ ] **Step 3.4：格式检查 + commit**

```bash
clang-format -style=file -i libraries/AIcomponents/src/lang_config.h
python script/check_format.py --debug --files libraries/AIcomponents/src/lang_config.h
git add libraries/AIcomponents/src/lang_config.h
git commit -m "feat(AIcomponents): sync lang_config.h with upstream superset

Upstream's src/ai_components/assets/include/lang_config.h defines 19
macros this repo's copy lacked (ALBUM, TAKE_PHOTO, THINKING, PRINT_*,
VIEW_IMAGE, ...), all needed by the ai_picture album UI. Whichever copy
wins the include search order, the Arduino one must not be a subset."
```

---

## 任务 4：固定 LVGL 版本 + 打开 ai_picture 开关

**v3 校正（风险重估）：** v2 担心的"v8/v9 头同时拉入"**不会发生**——`src/liblvgl/v8/CMakeLists.txt` 首行即 `if (CONFIG_LVGL_VERSION_8 STREQUAL "y")`，v9 同理，且 Kconfig 是 `choice`，必然二选一。实测本地 `compile_commands.json` 中 src 类 include 只有 `liblvgl/v9/{,conf,lvgl,port}`，**当前生效版本 = v9**。所以 pin v9 = 锁定现状，安全；真正的风险是**盲选**——若写成 v8 会静默切版本并编挂 Display 库。

**Files:**
- Modify: `tools/ci/config/t5/app_default.config`

- [ ] **Step 4.1：追加配置行**

在 `tools/ci/config/t5/app_default.config` 末尾追加（注意现有文件最后一行 `CONFIG_LVGL_ENABLE_TP=y` **没有结尾换行**，追加前先补换行）：

```
CONFIG_ENABLE_LIBLVGL=y
CONFIG_LVGL_VERSION_9=y
# CONFIG_LVGL_VERSION_8 is not set
CONFIG_ENABLE_COMP_AI_PICTURE=y
```

> `CONFIG_ENABLE_COMP_AI_PICTURE` 是任务 6 的**硬前提**：上游 `ai_picture/Kconfig` 为 `default n` 且无人 select，本仓库这份 config 会整体覆盖上游的 `app_default.config`（`package_platform.py:57-59`）。它还会 `select ENABLE_IMAGE_ALBUM`，从而把 `src/image_album/` 的 include 一并带进包（`ai_picture.h` 直接 `#include "image_album.h"`，缺了必编不过）。
>
> `CONFIG_ENABLE_AI_COMPONENTS` **不需要**加——上游 `your_chat_bot/Kconfig` 的 `config APP_CONFIG ... default y / select ENABLE_AI_COMPONENTS` 已经保证它开着。

- [ ] **Step 4.2：打包后核对（依赖任务 0）**

```bash
tar tjf output/0.0.0-dev/vendor-T5-*.tar.bz2 | grep -oE 'liblvgl/(v8|v9)' | sort -u   # 预期只有一行 v9
tar tjf output/0.0.0-dev/vendor-T5-*.tar.bz2 | grep -E 'ai_picture|image_album' | head  # 预期非空
```

- [ ] **Step 4.3：编译 LVGLdemo 确认没打破现状**

```bash
arduino-cli compile --clean --fqbn tuya_open:tuya_open:t5 libraries/Display/examples/LVGLdemo/LVGLdemo.ino
```

- [ ] **Step 4.4：commit**

```bash
git add tools/ci/config/t5/app_default.config
git commit -m "ci(t5): pin LVGL to v9 and enable ai_picture component

LVGL v9 is what the build already resolves to (verified against
compile_commands.json); pinning only guards against upstream changing the
Kconfig choice default. ENABLE_COMP_AI_PICTURE defaults to n upstream and
nothing selects it, so it must be enabled here for TuyaPicture to build
(it also pulls in ENABLE_IMAGE_ALBUM, required by ai_picture.h)."
```

---

## 任务 5：编译回归 —— 验证既有 Arduino AI 库无需改动

**目标：** 验证"上游 AI 函数 API 签名未变"这一结论。任何 sketch 编译失败都必须在 `.cpp` 内消化，**不允许改 `.h`**。

- [ ] **Step 5.1：按任务 0 的 runbook 打包 + 安装 core**
- [ ] **Step 5.2：编译全集**

```bash
fail=()
for sk in $(find libraries/AIcomponents libraries/Audio libraries/Camera libraries/Display -path '*/examples/*' -name "*.ino" | sort); do
  if arduino-cli compile --clean --fqbn tuya_open:tuya_open:t5 "$sk" > "/tmp/c-$(basename "$sk" .ino).log" 2>&1; then
    echo "OK   $sk"; else echo "FAIL $sk"; fail+=("$sk"); fi
done
echo "Failures: ${#fail[@]}"; printf '%s\n' "${fail[@]}"
```

- [ ] **Step 5.3：失败时的排查顺序**
  1. `#include` 找不到 → 该头在上游 `src/` 的哪个子目录？它的 include 路径是否出现在 `compile_commands.json` 的**第一条**命令里（`get_include_flags` 只读 `compile_json[0]["command"]`，见 `package_platform_t5.py:81`——**不是** v2 说的"扫描所有 `-I`"）→ 多半是 menuconfig 没开 → 回任务 4 加开关重打包。
  2. 符号未定义 → 在对应 `.cpp` 内加 `static inline` 兼容层，**保持 `.h` 公开签名不变**。
  3. 每修一个，重跑 Step 5.2 整轮。

- [ ] **Step 5.4：commit（仅当真改了 .cpp；预期零改动 → 无 commit）**

---

## 任务 6：新增 `TuyaPicture` Arduino 包装

**前提：** 任务 4 已写入 `CONFIG_ENABLE_COMP_AI_PICTURE=y`。

**Files:**
- Create: `libraries/AIcomponents/src/TuyaPicture.{h,cpp}`、`libraries/AIcomponents/examples/07_AI_Picture/07_AI_Picture.ino`

**Interfaces:**
- Consumes：`ai_picture.h` 的 `ai_picture_init` / `ai_picture_save_to_album` / `ai_picture_get_album_name`（签名见上方「上游实测 API」）
- Produces：`TuyaPictureClass` + 全局实例 `TuyaPicture`
  - `OPERATE_RET begin()` → `ai_picture_init()`，幂等
  - `void end()` → 仅翻转标志（上游**无 deinit 符号**）
  - `bool isInitialized()`
  - `const char *albumName()` → `ai_picture_get_album_name()`
  - `OPERATE_RET saveToAlbum(const uint8_t *data, uint32_t len, const char *hint = nullptr, char *outName = nullptr, uint32_t outNameSize = 0)`

- [ ] **Step 6.1-6.3：写 `.h` / `.cpp` / 示例**

实现要点：
- `.h` 用 `extern "C" { #include "tuya_cloud_types.h" #include "ai_picture.h" }` 包住 C 头。
- `saveToAlbum` 内部用栈上 `char nameBuf[AI_PICTURE_NAME_MAX_LEN + 1] = {0}`，因为上游形参是**定长数组** `char name[AI_PICTURE_NAME_MAX_LEN + 1]`，不能直接把用户的短 buffer 传进去（会溢出）；成功后再 `strncpy` 到 `outName` 并强制补 `\0`。
- 上游第一个形参是 `uint8_t *`（非 const），需要 `const_cast<uint8_t *>(data)`。
- 参数校验失败返回 `OPRT_INVALID_PARM`。
- 三个文件都要 `@file/@brief/@copyright` 头、无中文、clang-format 过。

- [ ] **Step 6.4：格式 + 编译**

```bash
clang-format -style=file -i libraries/AIcomponents/src/TuyaPicture.h libraries/AIcomponents/src/TuyaPicture.cpp
python script/check_format.py --debug --files libraries/AIcomponents/src/TuyaPicture.h libraries/AIcomponents/src/TuyaPicture.cpp libraries/AIcomponents/examples/07_AI_Picture/07_AI_Picture.ino
arduino-cli compile --clean --fqbn tuya_open:tuya_open:t5 libraries/AIcomponents/examples/07_AI_Picture/07_AI_Picture.ino
```

- [ ] **Step 6.5：commit**（信息中说明是"设备端相册存储"，不是云上传）

---

## 任务 7：新增 `TuyaVideo` Arduino 包装

**前提：** 本仓库 config 已有 `CONFIG_ENABLE_COMP_AI_VIDEO=y`（无需新增开关）。

**Files:**
- Create: `libraries/AIcomponents/src/TuyaVideo.{h,cpp}`、`libraries/AIcomponents/examples/08_AI_Video/08_AI_Video.ino`

**Interfaces：**
- `OPERATE_RET begin()` / `void end()` / `bool isInitialized()`
- `OPERATE_RET start()` / `OPERATE_RET stop()`
- `OPERATE_RET getJpegFrame(uint8_t **data, uint32_t *len)`
- `OPERATE_RET freeJpegFrame(uint8_t **data)`
- `OPERATE_RET setYuvFrameCallback(AI_VIDEO_FLUSH_CB cb)` —— **v3 校正：上游返回 `OPERATE_RET`，v2 写成 `void` 是错的**

> ⚠️ `AI_VIDEO_FLUSH_CB` 的形参是 `TDL_CAMERA_FRAME_T *`。把它放进公开 `.h` 会把 `tdl_camera` 的头依赖传导给用户 sketch——`.h` 里必须一并 `#include "tdl_camera_manage.h"`（或上游放置该类型的头），否则示例编不过。写 `.h` 前先确认该类型的定义头名。

按任务 6 同款五步法执行。

---

## 任务 8：端到端验证 + 发版联动

- [ ] **Step 8.1：清盘重打 + 按任务 0 runbook 重装 core**
- [ ] **Step 8.2：跑 `validate_index.py` + 任务 5 的全集编译**（预期全 OK）
- [ ] **Step 8.3：非 T5 板烟雾测试**

```bash
for board in t2 t3 ln882h esp32; do
  arduino-cli compile --clean --fqbn tuya_open:tuya_open:$board \
    libraries/AIcomponents/examples/00_IoT_SimpleExample/00_IoT_SimpleExample.ino 2>&1 | tail -3
done
```

> 这些板 `enabled: false`，本地没有对应 vendor 包时会直接失败——那是**预期**，记一笔即可，不算回归。

- [ ] **Step 8.4：升 PATCH 版本**：`tuya_arduino_version.h` `PATCH 1→2`、`platform.txt` `version=1.2.1→1.2.2`、`package.json` / `package_cn.json` 同步。
- [ ] **Step 8.5：全局格式与拼写**

```bash
python script/check_format.py --base origin/main
codespell --config .codespellrc
```

- [ ] **Step 8.6：commit 版本号**
- [ ] **Step 8.7（可选）：** `gh workflow run release-ci-cd.yml -f release_tag=1.2.2-rc1` 演练。

---

## 执行分期与当前进度（截至 2026-08-11）

### ✅ Phase A —— 纯代码，已完成并验收

| 项 | 产物 | 验收 |
|---|---|---|
| 任务 0.1 | `generate_index_json.py` 加 `--base-url` | 不传该参数时输出与改前逐字节一致 |
| 任务 2 | 删除 3 个失效方法 + 调用点（-100 行） | `py_compile` 通过；`tools/`+`.github/` 无残留引用 |
| 任务 3 | `lang_config.h` 补齐 19 个宏 | 宏名集合与上游一致；既有取值未变 |
| 任务 4.1 | LVGL pin v9 + `ENABLE_COMP_AI_PICTURE=y` | 无行粘连 |
| 任务 6 | `TuyaPicture.{h,cpp}` + 07 示例 | 缓冲区处理正确（定长 `nameBuf` + 有界 `strncpy`） |
| 任务 7 | `TuyaVideo.{h,cpp}` + 08 示例 | 上游返回值/空指针保护均已核对 |
| 补漏 | 07/08 各补 `README.md` + `README_zh.md` | 与同级示例体例一致 |

全部通过 `check_format.py` 与 `codespell`。**尚未 commit**，且当前在 `main` 分支上。

### ⬜ Phase A' —— 新增的阻塞项（纯代码，可立即做）

- **任务 1**（配置腐烂修复 + 打包期符号校验 + pin commit）—— 见上，**必须先于任何一次打包**。

### ⬜ Phase B —— 需 arduino-cli + 完整 T5 SDK 构建（串行，小时级）

顺序：任务 0.2（装 arduino-cli 1.2.2）→ 0.3（定点 `git clean`）→ **打包一次** → 任务 2.4（含 `nm` 符号检查）+ 4.2 + 1.2 的校验自证 → 0.4 runbook 装 core → 任务 4.3、5、6.4、7 编译 → 任务 8（版本号 + 端到端）。

> **只打一次包。** 每次打包都要 fetch + submodule + `tos.py clean` 全量重建，是小时级操作；把所有代码改动做完再进 Stage B。

**Phase B 的三个必查项**（编译通过 ≠ 正确）：

| 查什么 | 命令 | 不合格意味着 |
|---|---|---|
| AI 符号真进了库 | `nm` 找 `ai_picture_init` / `ai_video_init` / `ai_chat_main` | 任务 2 删过头，需保留 libs 合并、只删 headers 部分 |
| LVGL 只有一个版本 | `tar tjf … \| grep -oE 'liblvgl/(v8\|v9)' \| sort -u` | Kconfig choice 未生效 |
| `lang_config.h` 谁赢 | 预处理输出 `grep LANG_CODE`，看 `en-US` 还是 `zh-CN` | 上游那份赢 → 示例界面变中文，需改名显式包含 |

### 环境现状

`clang-format-14` ✅ ／ `codespell` ⚠️ 仅在 scratchpad venv（系统 pip 被 PEP 668 锁）／ `arduino-cli` ❌ 未装／磁盘 22G 可用（clone 4G + 工具链 694M 已在盘）。

> Phase B 的每一次全量打包都要 clone TuyaOpen + submodule + `tos.py clean` 全量重建，是小时级操作。**把 Phase A 全部做完再进 Phase B，只打一次包**，不要按 v2 那样在任务 2/3/4/8 各打一次（既慢，又因为 `sourceBranch: master` 是移动靶而可能对着不同 commit）。
