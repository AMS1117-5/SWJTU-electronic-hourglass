# SWJTU Electronic Hourglass

西南交通大学电子科协招新礼物 —— 电子沙漏。

这是一个基于 MCU、MPU6050 和 LED 点阵实现的电子沙漏项目。设备可以感知自身姿态，并根据重力方向模拟沙粒流动，同时提供倒计时、菜单和蜂鸣器提示功能。

## Features

* MPU6050 姿态感知
* 双 LED 点阵显示
* 网格粒子沙粒模拟
* 沙粒根据重力方向移动
* 可设置倒计时时间
* 倒计时结束后蜂鸣器提示
* 按键菜单操作

## Repository Structure

```text
datasheet/      芯片与器件数据手册
pcb/            原理图、PCB 与相关硬件文件
project/        MCU 固件源码与 CMake 工程
README.md       项目说明
LICENSE         开源许可证
```

## Firmware

固件源码位于：

```text
project/
```

项目使用 CMake 进行构建。

主要软件模块包括：

* MPU6050 驱动
* 姿态与重力方向计算
* LED 点阵驱动
* 沙粒模拟
* 按键处理
* 菜单状态机
* 蜂鸣器控制

## Hardware

主要器件包括：

* MCU
* MPU6050 六轴 IMU
* LED 点阵及驱动芯片
* 按键
* 蜂鸣器
* 电源与充电电路

具体原理图和 PCB 文件请查看：

```text
pcb/
```

## Build

进入固件工程目录后使用 CMake 构建。

```bash
cmake --preset Debug
cmake --build --preset Debug
```

具体工具链和烧录方式以后会继续补充。

## Project Purpose

本项目作为电子科协招新礼物制作，同时开放工程文件和源码，供新生学习嵌入式开发、PCB 设计、传感器应用和简单物理模拟。

欢迎阅读、学习、修改和二次开发。

## License

本项目的源码和工程文件按照仓库中的 `LICENSE` 文件开放。

如进行二次发布，请保留原项目来源及作者信息。

## Credits

Project: SWJTU Electronic Science and Technology Association Welcome Gift

Author: Frederich

Year: 2026
