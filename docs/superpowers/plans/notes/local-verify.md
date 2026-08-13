# 本地验证 Runbook：打包 → 起 HTTP server → 生成 index → 装 core → 编译

来源：`docs/superpowers/plans/2026-06-23-ai-components-refactor.md` 任务 0 / Step 0.4。
供任务 5、任务 8 的编译回归验证复用——目的是让本地打出的 vendor 包能被
`arduino-cli` 真实安装，而不必等一个真正的 GitHub Release 存在。

## 前置条件

- `tools/ci/generate_index_json.py` 已支持 `--base-url`（任务 0 Step 0.1，见下方
  「关于 --base-url」一节）。未打这个补丁时，本 runbook 第 3 步生成的 index 会
  指向 `https://github.com/{owner}/{repo}/releases/download/...`，而这个 release
  在本地验证阶段并不存在，`arduino-cli core install` 会 404。
- 已完成 Step 0.3：删掉旧的、被上游遗留 hack 污染的本地 clone：

  ```bash
  rm -rf tools/ci/output/vendor-T5
  ```

  如果不删，`tools/ci/output/vendor-T5` 可能还停在很旧的上游 commit，且里面混有
  由旧 hack 生成的、Git 未跟踪的 `src/ai_components/*.h` 文件。上游现在已经把
  `src/ai_components/` 纳入跟踪，`git checkout` 会因为这些未跟踪文件冲突而失败，
  进而触发 `package_handler.py` 里 `rmtree` 式的全量重新 clone——与其让脚本
  慢慢发现这个问题，不如手工先删干净，省一轮网络 clone。

- 已确认工具链齐备（Step 0.2）：`arduino-cli`、`pytest` 等。

## 步骤

### 1）打包

```bash
# 长任务：会 clone TuyaOpen + 其 submodule，并跑一次完整的 tos.py 构建。
# 网络依赖重、耗时长（含拉取整个 vendor SDK 仓库与工具链），不要在网络受限
# 或想快速迭代的场景下反复跑；跑之前确认 tools/ci/output/vendor-T5 已按上面
# Step 0.3 清理过，否则可能因未跟踪文件冲突触发一次隐式的全量重新 clone。
python tools/ci/package_release.py --version 0.0.0-dev --target t5
python tools/ci/package_release.py --version 0.0.0-dev --target arduino --checkout-path "$(pwd)"
```

### 2）本地托管产物

```bash
cd output/0.0.0-dev && python3 -m http.server 8765 & cd -
```

### 3）生成指向本地的 index

用新增的 `--base-url` 参数，把所有产物 URL 从
`https://github.com/{owner}/{repo}/releases/download/{tag}/{filename}`
换成 `{base-url}/{filename}`（脚本内部会去掉 `base-url` 末尾多余的斜杠，保证
拼接处只有一个 `/`）：

```bash
python tools/ci/generate_index_json.py \
  --base-index package.json --manifest output/0.0.0-dev/manifest.json \
  --version 0.0.0-dev --config tools/ci/package-config.json \
  --github-owner tuya --github-repo arduino-TuyaOpen \
  --base-url "http://127.0.0.1:8765" \
  --output /tmp/test_index.json

python tools/ci/validate_index.py --index /tmp/test_index.json \
  --version 0.0.0-dev --manifest output/0.0.0-dev/manifest.json
```

> 关于 `--base-url`：这是可选参数，默认 `None`。不传时行为与改造前完全一致
> （字节级相同，走真正的 CI 发布路径，生成的还是 GitHub release URL）；只有
> 显式传入时，才会把每个产物的下载 URL 替换成 `{base-url}/{filename}`。所以
> CI 流水线（`.github/workflows/release-ci-cd.yml`）不需要跟着改。

### 4）安装

```bash
arduino-cli config init --overwrite
arduino-cli config add board_manager.additional_urls "file:///tmp/test_index.json"
arduino-cli core update-index
arduino-cli core install tuya_open:tuya_open
```

### 5）编译验证（任务 5 / 任务 8 复用本 runbook 时接着做）

安装成功后，用仓库里已有的示例 sketch 走一遍
`arduino-cli compile --fqbn tuya_open:tuya_open:<board> <sketch>.ino`，
具体板子/示例清单见 `CLAUDE.md` 中「Testing a sketch locally」一节。
