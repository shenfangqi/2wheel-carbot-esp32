#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d)"

cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_wheel_ticks.c" -o "$test_dir/test_wheel_ticks"
"$test_dir/test_wheel_ticks"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_safety_manager.c" "$repo_dir/main/control/safety_manager.c" \
  -o "$test_dir/test_safety_manager"
"$test_dir/test_safety_manager"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/components/icm42670p" \
  "$repo_dir/tests/test_imu_units.c" -o "$test_dir/test_imu_units"
"$test_dir/test_imu_units"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_ros_time_math.c" -o "$test_dir/test_ros_time_math"
"$test_dir/test_ros_time_math"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_ros_time_state.c" -o "$test_dir/test_ros_time_state"
"$test_dir/test_ros_time_state"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_ros_health.c" -o "$test_dir/test_ros_health"
"$test_dir/test_ros_health"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_ros_watchdog.c" -o "$test_dir/test_ros_watchdog"
"$test_dir/test_ros_watchdog"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_publisher_schedule.c" -o "$test_dir/test_publisher_schedule"
"$test_dir/test_publisher_schedule"
cc -std=c11 -Wall -Wextra -Werror -I"$repo_dir/main" \
  "$repo_dir/tests/test_differential_command.c" -lm \
  -o "$test_dir/test_differential_command"
"$test_dir/test_differential_command"

grep -q '^uint64 sequence$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/WheelTicks.msg"
grep -q '^uint32 boot_id$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/WheelTicks.msg"
grep -q '^uint64 device_stamp_us$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/WheelTicks.msg"
grep -q '^int64 left_ticks$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/WheelTicks.msg"
grep -q '^int64 right_ticks$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/WheelTicks.msg"
grep -q '^bool time_synchronized$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
grep -q '^uint32 invalid_cmd_count$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
grep -q '^uint8 last_disconnect_reason$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
grep -q '^uint32 consecutive_ping_failures$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
grep -q '^uint64 session_uptime_ms$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
grep -q '^uint64 last_time_sync_age_ms$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
grep -q '^uint32 time_sync_fail_count$' "$repo_dir/extra_ros_packages/carbot_msgs/msg/CarbotStatus.msg"
echo "host unit tests passed"
