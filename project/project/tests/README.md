# 主机端驱动测试

## 推荐：一次运行全部 11 项测试

在固件目录使用 WSL 原生 CMake/GCC（不是 Windows ARM 工具链）：

```bash
cmake -S tests -B build/host-tests -G "Unix Makefiles" -DENABLE_SANITIZERS=ON
cmake --build build/host-tests -j 4
ctest --test-dir build/host-tests --output-on-failure
```

除下面的驱动和物理测试外，还包括：

- 按键：实物引脚对应、消抖、短长按互斥、溢出。
- MAX7219/显示：级联字节序、CS、清屏后唤醒、128 点映射、实时亮度不破坏 framebuffer。
- 设置/UI：整轮 30～240 秒的 30 秒步进和边界、0～2 亮度、两位分钟/秒字形、四项循环菜单。
- 沙漏：运行时变更时间不重置沙量/位置，全部八档时长与非整数秒单粒间隔均可完成。
- 应用集成：真实按键驱动→菜单→两个设置页→海洋/沙漏恢复，以及播放旋律时调整亮度、
  错误期间菜单可用、恢复和重启默认值。

## 单独运行测试

在固件目录 `project/` 下运行（原生 Linux GCC，不是 ARM 交叉编译器）：

```bash
gcc -std=c11 -Wall -Wextra -Werror \
  -Itests/mocks -IDrivers -IConfig \
  tests/mpu6050_test.c Drivers/mpu6050.c \
  -o /tmp/mpu6050_test
/tmp/mpu6050_test
```

测试使用真实的 MPU6050 驱动和模拟 I2C，覆盖地址移位、复位/唤醒等待、寄存器配置、
六轴有符号字节序、DATA_RDY 与样本有效期、身份不符、通信/配置失败、延迟重试恢复和 tick 溢出。
实物 I2C 电气通信、显示效果与传感器方向仍需按 `MPU6050_CALIBRATION.md` 实机验证。

重力转换测试使用真实 Motion 与模拟的传感器样本源：

```bash
gcc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
  -IPhysics -IDrivers -IConfig \
  tests/motion_test.c Physics/motion.c \
  -o /tmp/motion_test
/tmp/motion_test
```

覆盖用户实测三姿态、反向姿态、三维归一化的平面投影、重复样本不重复滤波、
数据失效与恢复、零模长、全量程整数运算及 tick 溢出。
同时验证摇动强度仅随新样本更新、静止后衰减，以及极端读数的饱和行为。

粒子引擎与海洋集成测试：

```bash
gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -IPhysics -IConfig \
  tests/particles_test.c Physics/particles.c -o /tmp/particles_test
/tmp/particles_test

gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -IApp -IPhysics -IDrivers -IConfig \
  tests/ocean_test.c App/ocean.c Physics/particles.c -o /tmp/ocean_test
/tmp/ocean_test
```

覆盖四个方向的堆积和翻转、粒子数量守恒、无重叠、边界与墙体碰撞、禁止斜穿封闭墙角、
弱重力下的运动频率、容量限制、固定种子的可重复性，以及一万步旋转/摇动压力测试。
海洋测试使用真实粒子引擎，模拟 Motion 和 framebuffer，验证渲染、20 ms 调度、
暂停恢复、传感器失效时冻结、长延迟不补跑、平放静止和摇动扰动。

沙漏与蜂鸣器测试（模拟 tick，不需要真实等待多个 4 分钟）：

```bash
gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -IApp -IPhysics -IDrivers -IConfig \
  tests/hourglass_test.c App/hourglass.c Physics/particles.c -o /tmp/hourglass_test
/tmp/hourglass_test

gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/mocks -IDrivers \
  tests/buzzer_test.c Drivers/buzzer.c -o /tmp/buzzer_test
/tmp/buzzer_test
```

覆盖沙量/粒子守恒、10 秒单粒间隔、瓶颈限流、完整 240 秒积分、半速倾斜、水平死区、摇动时禁止偷渡、
失效暂停、诊断恢复、部分反转回原仓、正反方向空仓边沿的一次性事件、上电空仓静音、
持续空仓不重复提示、不同帧间隔及 tick 溢出。
蜂鸣器验证 PWM 通道/频率、5 秒旋律、延迟时跳过过期音符、APB 定时器倍频和出错静音。
