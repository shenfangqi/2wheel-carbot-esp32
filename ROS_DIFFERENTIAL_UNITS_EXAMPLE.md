# ROS 履带差速运动学与单位示例

当前 Carbot 是履带式差速/滑移转向底盘。ROS `geometry_msgs/msg/Twist` 的运动学输入为 `linear.x = v`（`m/s`）和 `angular.z = w`（`rad/s`，正值左转）。舵机不是当前底盘的运动学转向源；URDF、Nav2、Isaac 都不应从舵机角度计算曲率、里程计或 `cmd_vel`。

## 固件标定参数

| 参数 | 值 | 用途 |
|---|---:|---|
| 有效差速轮距 `b_eff` | `0.254 m` | 差速换算、转向计数、里程计和理想差速仿真 |
| 实际履带中心几何间距 `b_geom` | `0.225 m` | URDF 外观、碰撞体和关节位置 |
| 有效驱动轮半径 `r_eff` | `0.02175 m` | 线速度与编码器/RPM 换算 |

`0.254 m` 是地面转向标定得到的有效轮距，包含履带侧滑、刮擦和传动误差。`0.225 m` 只描述实体几何，不能替换已标定控制或里程计公式中的 `0.254 m`。

## Twist 到左右履带速度

```text
v_l = v - w * b_eff / 2
v_r = v + w * b_eff / 2
rpm = v_track * 60 / (2 * pi * r_eff)
```

例如 `v = 0.20 m/s`、`w = 1.0 rad/s`：

```text
v_l = 0.073 m/s  ->  32.05 rpm
v_r = 0.327 m/s  -> 143.56 rpm
```

当前接线中 M1 是右履带，M3 是左履带；车辆前进时 M1 目标为正、M3 目标为负：

```text
motor_pid_controller_set_target_rpm(right_rpm, -left_rpm)
```

该符号映射属于电机安装/接线约定，不应改变 ROS 坐标系或差速公式。

## 原地转向与里程计

当 `v = 0` 时：

```text
v_l = -w * b_eff / 2
v_r =  w * b_eff / 2
track_distance = abs(theta) * b_eff / 2
```

左右履带位移增量为 `delta_l`、`delta_r` 时：

```text
delta_s     = (delta_r + delta_l) / 2
delta_theta = (delta_r - delta_l) / b_eff
```

这些公式统一使用 `b_eff = 0.254 m`。

## URDF、Isaac 与 Nav2 移植约定

- URDF 可保留履带、主动轮和从动轮的可视关节；它们用于显示或关节状态，不作为底盘转向来源。
- `base_link`/`base_footprint` 的平面运动、TF 和里程计由差速/滑移运动学产生，不从舵机关节推导。
- Nav2 输出标准 `cmd_vel`，底盘插件选 differential drive 或 skid steer，不选 Ackermann、bicycle 或 steering controller。
- Isaac 若先采用理想差速模型，将 wheel separation/track width 设为 `0.254 m`，以匹配真车已标定转向行为。
- URDF 中履带中心、碰撞体和视觉网格可按 `0.225 m` 实测几何布置；不要把该值复制到控制或里程计公式。
- 若以后加入显式履带接触/侧滑模型，应重新验证等效轮距；得到新标定前，`0.254 m` 仍是控制与里程计真值。
- 履带可视动画可以由各自累计行程驱动，但不得反馈为第二套底盘里程计。

## 快速检查

1. `angular.z > 0` 时左履带变慢、右履带变快，车辆左转。
2. `linear.x = 0` 且 `angular.z != 0` 时车辆原地转向。
3. 舵机关节固定、隐藏或运动时，`base_link` 运动学结果都不变。
4. 控制器和里程计使用 `0.254 m`，URDF 几何布局可使用 `0.225 m`。
5. TF/里程计只有一个权威来源，不由履带可视关节重复积分。
