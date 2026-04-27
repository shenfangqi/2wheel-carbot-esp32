# ROS Ackermann 单位示例

这个文档把当前项目里一个固定例子，从 `ROS / 网页输入` 到 `电机 PWM` 的单位流转整理出来，并标明每个公式里参数的含义。

## 示例输入

- ROS 输入
  - `linear.x = 0.20 m/s`
  - `angular.z = 2.0 rad/s`
- 网页输入
  - `forward + left`
  - 映射为
    - `v = 0.20 m/s`
    - `w = 2.0 rad/s`

## 固定参数

- `r = 0.050 m`
  - 含义：轮子半径
- `L = 0.185 m`
  - 含义：轴距
- `kp = 0.8`
  - 含义：PID 比例系数
- `ki = 0.05`
  - 含义：PID 积分系数
- `kd = 0.0`
  - 含义：PID 微分系数
- `dt = 0.01 s`
  - 含义：PID 更新周期
- `pulses_per_rev = 1040`
  - 含义：编码器每转一圈的脉冲数
- `servo_limit = [-35 deg, 35 deg]`
  - 含义：舵机目标转角限幅范围
- `PWM_MOTOR_DUTY_TICK_MAX = 400 ticks`
  - 含义：电机 PWM 一个完整周期的 tick 数
- `PWM_MOTOR_DEAD_ZONE = 200 ticks`
  - 含义：电机死区补偿

## 第一步：输入命令

- `v = 0.20 m/s`
  - 含义：小车目标线速度
- `w = 2.0 rad/s`
  - 含义：小车目标角速度

## 第二步：Ackermann 线速度换算成轮速 RPM

公式：

```text
wheel_rpm = (v * 60) / (2 * pi * r)
```

参数含义：

- `wheel_rpm`
  - 含义：目标轮速，单位是每分钟转多少圈
- `v`
  - 含义：小车线速度，单位是米每秒
- `pi`
  - 含义：圆周率
- `r`
  - 含义：轮子半径，单位是米

代入示例：

```text
wheel_rpm = (0.20 * 60) / (2 * pi * 0.050) = 38.20 rpm
```

## 第三步：Ackermann 角速度换算成舵机目标角度

公式：

```text
steering_angle_deg = atan((L * w) / v) * 180 / pi
```

参数含义：

- `steering_angle_deg`
  - 含义：目标转向角，单位是度
- `L`
  - 含义：轴距，单位是米
- `w`
  - 含义：小车角速度，单位是弧度每秒
- `v`
  - 含义：小车线速度，单位是米每秒
- `pi`
  - 含义：圆周率

代入示例：

```text
steering_angle_deg = atan((0.185 * 2.0) / 0.20) * 180 / pi = 61.56 deg
```

限幅公式：

```text
steering_angle_deg = clamp(61.56, -35, 35) = 35 deg
```

参数含义：

- `clamp(x, min, max)`
  - 含义：把 `x` 限制在 `[min, max]` 范围内

## 第四步：Ackermann 输出到底盘目标转速

公式：

```text
M1_target_rpm = -wheel_rpm
M3_target_rpm =  wheel_rpm
```

参数含义：

- `M1_target_rpm`
  - 含义：M1 电机目标转速
- `M3_target_rpm`
  - 含义：M3 电机目标转速
- `wheel_rpm`
  - 含义：由 Ackermann 线速度换算得到的轮速

代入示例：

```text
M1_target_rpm = -38.20 rpm
M3_target_rpm =  38.20 rpm
```

## 第五步：编码器脉冲换算成实际 RPM

公式：

```text
actual_rpm = delta_count * 60 / (pulses_per_rev * dt)
```

参数含义：

- `actual_rpm`
  - 含义：实际测得的轮速，单位是 RPM
- `delta_count`
  - 含义：一个 PID 周期内编码器脉冲变化量
- `pulses_per_rev`
  - 含义：编码器每转一圈对应的脉冲数
- `dt`
  - 含义：PID 周期，单位是秒

这里沿用前面举例时的假设：

```text
delta_count = 0
actual_rpm = 0 * 60 / (1040 * 0.01) = 0 rpm
```

所以：

```text
M1_actual_rpm = 0 rpm
M3_actual_rpm = 0 rpm
```

## 第六步：PID 误差

公式：

```text
error = target_rpm - actual_rpm
```

参数含义：

- `error`
  - 含义：PID 看到的速度误差
- `target_rpm`
  - 含义：目标转速
- `actual_rpm`
  - 含义：实际转速

代入示例：

```text
M1_error = -38.20 - 0 = -38.20 rpm
M3_error =  38.20 - 0 =  38.20 rpm
```

## 第七步：PID 积分项

公式：

```text
integral = error * dt
```

参数含义：

- `integral`
  - 含义：PID 的积分项
- `error`
  - 含义：当前误差
- `dt`
  - 含义：PID 周期

代入示例：

```text
M1_integral = -38.20 * 0.01 = -0.382
M3_integral =  38.20 * 0.01 =  0.382
```

## 第八步：PID 原始输出

公式：

```text
output = kp * error + ki * integral + kd * derivative
```

参数含义：

- `output`
  - 含义：PID 原始输出
- `kp`
  - 含义：比例系数
- `ki`
  - 含义：积分系数
- `kd`
  - 含义：微分系数
- `error`
  - 含义：当前误差
- `integral`
  - 含义：积分项
- `derivative`
  - 含义：误差变化率

这里沿用前面举例时的假设：

```text
derivative = 0
M1_output = 0.8 * (-38.20) + 0.05 * (-0.382) + 0.0 * 0 = -30.58
M3_output = 0.8 *   38.20  + 0.05 *   0.382  + 0.0 * 0 =  30.58
```

## 第九步：PID 输出转成整数 PWM 命令

公式：

```text
pwm_output = (int)output
```

参数含义：

- `pwm_output`
  - 含义：转成整数后的 PWM 命令
- `output`
  - 含义：PID 原始输出

代入示例：

```text
M1_pwm_output = -30
M3_pwm_output =  30
```

## 第十步：PWM 限幅

公式：

```text
pwm_output = clamp(pwm_output, -200, 200)
```

参数含义：

- `pwm_output`
  - 含义：限幅后的 PWM 命令

代入示例：

```text
M1_pwm_output = -30
M3_pwm_output =  30
```

## 第十一步：电机死区补偿

公式：

```text
speed > 0 -> duty = speed + 200
speed < 0 -> duty = speed - 200
speed = 0 -> duty = 0
```

参数含义：

- `speed`
  - 含义：PID 给出的带符号 PWM 命令
- `duty`
  - 含义：送给电机驱动的 duty tick
- `200`
  - 含义：死区补偿值

代入示例：

```text
M1_duty_cmd = -30 - 200 = -230 ticks
M3_duty_cmd =  30 + 200 =  230 ticks
```

## 第十二步：最终发给电机驱动

映射结果：

```text
M1 = reverse, 230 ticks
M3 = forward, 230 ticks
```

参数含义：

- `reverse`
  - 含义：电机反转方向
- `forward`
  - 含义：电机正转方向
- `230 ticks`
  - 含义：最终送给电机驱动的 PWM 占空命令幅值

## 第十三步：PWM 占空比

公式：

```text
duty_ratio = duty_ticks / period_ticks
```

参数含义：

- `duty_ratio`
  - 含义：PWM 占空比
- `duty_ticks`
  - 含义：实际使用的 duty tick
- `period_ticks`
  - 含义：一个 PWM 周期对应的总 tick 数

代入示例：

```text
period_ticks = 400 ticks
duty_ratio = 230 / 400 = 57.5%
```

## 简短总结

- 小车输入命令
  - `0.20 m/s`
  - `2.0 rad/s`
- Ackermann 输出
  - `38.20 rpm`
  - `35 deg`
- 电机目标
  - `M1 = -38.20 rpm`
  - `M3 = 38.20 rpm`
- 第一拍 PID 示例
  - `actual = 0 rpm`
  - `PID output = +/-30`
  - `driver duty = +/-230 ticks`
  - `PWM duty = 57.5%`
