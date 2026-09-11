#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "display.h"
#include "joystick.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(main);

static int use_callback = 1;

static struct gpio_dt_spec mode_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_dt_spec fire_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static struct gpio_callback button_callback_mode;
static struct gpio_callback button_callback_fire;

static void button_mode_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	use_callback = set_callback();
	LOG_INF("Use callback: %d", use_callback);
}

static void fire_button_callback(const struct device *port,
								 struct gpio_callback *cb,
								 uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	LOG_INF("Fake fire button pressed");
	fake_button_pressed();
}

static int configure_mode_button() {
	int err;
	if (!gpio_is_ready_dt(&mode_button)) {
		LOG_ERR("Mode button %s is not ready", mode_button.port->name);
		return -1;
	}

	err = gpio_pin_configure_dt(&mode_button, GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure mode button gpio: %d", err);
		return -1;
	}

	gpio_init_callback(&button_callback_mode, button_mode_callback,
		BIT(mode_button.pin));

	err = gpio_add_callback(mode_button.port, &button_callback_mode);
	if (err) {
		LOG_ERR("Failed to add mode button callback: %d", err);
		return -1;
	}

	err = gpio_pin_interrupt_configure_dt(&mode_button,
					      				  GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Failed to enable mode button callback: %d", err);
		return -1;
	}

	return 0;
}

//keep in main
static int configure_fire_button() {
	int err;
	if (!gpio_is_ready_dt(&fire_button)) {
		LOG_ERR("Fire button %s is not ready", fire_button.port->name);
		return -1;
	}

	err = gpio_pin_configure_dt(&fire_button, GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure fire button gpio: %d", err);
		return -1;
	}

	gpio_init_callback(&button_callback_fire, fire_button_callback,
		BIT(fire_button.pin));

	err = gpio_add_callback(fire_button.port, &button_callback_fire);
	if (err) {
		LOG_ERR("Failed to add fire button callback: %d", err);
		return -1;
	}

	err = gpio_pin_interrupt_configure_dt(&fire_button,
					      				  GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Failed to enable fire button callback: %d", err);
		return -1;
	}

	return 0;
}

static int configure_device() {
	int ret;

	/*configure mode btn*/
	ret = configure_mode_button();
	if (ret < 0) {
		LOG_ERR("Failed to configure mode button");
		return -1;
	}
	/*configure fire btn*/
	ret = configure_fire_button();
	if (ret < 0) {
		LOG_ERR("Failed to configure fire button");
		return -1;
	}

	LOG_INF("Device successfully configured");

	return 0;
}

int main(void) {
	int ret = configure_display();
	if (ret < 0) {
		LOG_ERR("Failed to configure display");
		return -1;
	}

	ret = configure_device();
	if (ret < 0) {
		LOG_ERR("Could not configure the device");
		return -1;
	}

	ret = configure_joystick();
	if (ret < 0) {
		LOG_ERR("Could not configure the joystick");
		return -1;
	}

	init_display();
	
	show_start_screen();
	
	return 0;
}
