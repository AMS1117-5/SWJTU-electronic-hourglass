# 电子沙漏

STM32F030C8T6 + MPU6050 + 双 MAX7219 点阵电子沙漏。

V1.0 软件功能已接齐：沙漏、电子海洋、四项主菜单、时间设置、亮度设置及完成旋律。
显示/重力方向已完成实机标定，正式菜单与两个设置页待用户最终整体体验反馈。

- [正式版操作、构建与验收说明](project/README.md)
- [项目规格和已确认硬件接口](AGENT.md)
- [自动化测试](project/tests/README.md)
- 正式固件：`project/build/Release/project.hex`（也生成 `.elf` / `.bin`）
- 诊断固件：`project/build/Diagnostic/project.hex`

正式版上电进入沙漏，长按 K1 打开菜单。默认每颗沙粒 10 秒、24 颗约 4 分钟；
整轮时间可调为 30～240 秒、步进 30 秒，对应每颗 1.25～10 秒；亮度可调为 0～2。
设置保存在 RAM，重新上电恢复默认。
