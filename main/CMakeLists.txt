idf_component_register(
    SRCS
        "app_main.c"

        "app_config/app_config.c"
        "app_config/config_store.c"
        "app_config/usb_config_cli.c"

        "network/wifi_manager.c"
        "network/uros_transport.c"

        "drivers/servo_driver.c"
        "drivers/motor_driver.c"
        "drivers/encoder_driver.c"
        "drivers/status_led.c"
        "drivers/buzzer.c"
        "drivers/battery_monitor.c"

        "control/servo_controller.c"
        "control/differential_controller.c"
        "control/command_mux.c"
        "control/motor_pid_controller.c"
        "control/odometry_estimator.c"
        "control/safety_manager.c"

        "ros_interface/ros_publishers.c"
        "ros_interface/ros_subscribers.c"
        "ros_interface/ros_executor.c"
        "ros_interface/ros_time.c"

        "utils/telemetry_buffer.c"

    INCLUDE_DIRS
        "."
        "app_config"
        "network"
        "drivers"
        "control"
        "ros_interface"
        "utils"

		REQUIRES
            driver
            esp_adc
            icm42670p
            esp_timer
		    esp_wifi
		    esp_event
		    esp_netif
		    nvs_flash
		    esp_driver_uart
            esp_driver_mcpwm
            micro_ros_espidf_component
)
