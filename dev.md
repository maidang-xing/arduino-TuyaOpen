# 创建github工作流

创建arduino 仓库工作流，为仓库的 main 分支提交 push 和 PR 后触发编译检查功能，编译指定的示例并且输出检查信息。


要求：
1. 编译环境：windows，下载 arduino-cli 工具，在windows上编译。

2. 编译开发板：从仓库的release页面，https://github.com/maidang-xing/arduino-TuyaOpen/releases/download/global/package_tuya_open_index.json  下载开发板包，并在arduino-cli中添加开发板管理器URL。

3. 编译项目从 boards.txt 读取开发板信息。

4. 编译示例：所有开发板目前只编译：libraries/AIcomponents/examples/00_IoT_SimpleExample/00_IoT_SimpleExample.ino 示例，编译完成后输出编译结果。

5. 添加手动测试工作流：在仓库的 actions 页面，添加手动测试工作流，手动触发工作流，查看编译结果。

6. 完成上面任务后生成一份 md 文档进行说明，并且介绍工作流的操作方法。