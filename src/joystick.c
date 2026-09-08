#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

/**
 * Receiver module for handling GPIO inputs.
 * This module reads the status of specific GPIO pins (P1.10, P1.06, P1.07, P1.08, and P1.04).
 * It provides functions to configure the receiver and read pin statuses.
 */

LOG_MODULE_REGISTER(receiver, LOG_LEVEL_INF);

/* P0.05 device (used by read_P1_05) */
#define P0_05_PORT_NODE DT_NODELABEL(gpio1)
const struct device *gpio_05 = DEVICE_DT_GET(P0_05_PORT_NODE);
#define P1_05_PIN 5

/* P1.07 device (used by read_P1_06) */
#define P1_06_PORT_NODE DT_NODELABEL(gpio1)
const struct device *gpio_06 = DEVICE_DT_GET(P1_06_PORT_NODE);
#define P1_06_PIN 6

/* P1.06 device (used by read_P1_07) */
#define P1_07_PORT_NODE DT_NODELABEL(gpio1)
const struct device *gpio_07 = DEVICE_DT_GET(P1_07_PORT_NODE);
#define P1_07_PIN 7

/* P1.05 device (used by read_P1_08) */
#define P1_08_PORT_NODE DT_NODELABEL(gpio1)
const struct device *gpio_08 = DEVICE_DT_GET(P1_08_PORT_NODE);
#define P1_08_PIN 8

/* P1.04 device (used by read_P1_04) */
#define P1_04_PORT_NODE DT_NODELABEL(gpio1)
const struct device *gpio_04 = DEVICE_DT_GET(P1_04_PORT_NODE);
#define P1_04_PIN 4


static struct gpio_dt_spec stick_up = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jsup), gpios, {0});
static struct gpio_dt_spec stick_down = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jsdown), gpios, {0});
static struct gpio_dt_spec stick_left = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jsleft), gpios, {0});
static struct gpio_dt_spec stick_right = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jsright), gpios, {0});
static struct gpio_dt_spec stick_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(jsbtn), gpios, {0});

static int pin_status_0 = 0;
static int pin_status_1 = 0;
static int pin_status_2 = 0;
static int pin_status_3 = 0;
static int pin_status_4 = 0;

static int status[5] = {0};
static struct gpio_callback callback_up;
static struct gpio_callback callback_down;
static struct gpio_callback callback_left;
static struct gpio_callback callback_right;
static struct gpio_callback callback_button;

int read_P1_05(void) {

    if (!device_is_ready(gpio_05)) {
        LOG_ERR("gpio_05 device not ready");
        return -1;
    }

    return gpio_pin_get(gpio_05, P1_05_PIN);
}

int read_P1_06(void) {
    if (!device_is_ready(gpio_06)) {
        LOG_ERR("gpio_06 device not ready");
        return -1;
    }
	
    return gpio_pin_get(gpio_06, P1_06_PIN);
}

/* Returns 1 if pin P1.06 is high, 0 if low, or a negative error code */
int read_P1_07(void) {
    if (!device_is_ready(gpio_07)) {
        LOG_ERR("gpio_07 device not ready");
        return -1;
    }

    return gpio_pin_get(gpio_07, P1_07_PIN);
}

int read_P1_08(void) {
    if (!device_is_ready(gpio_08)) {
        LOG_ERR("gpio_08 device not ready");
        return -1;
    }

    return gpio_pin_get(gpio_08, P1_08_PIN);
}

int read_P1_04(void) {
    if (!device_is_ready(gpio_04)) {
        LOG_ERR("gpio_04 device not ready");
        return -1;
    }

    return gpio_pin_get(gpio_04, P1_04_PIN);
}

static void stick_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);

	switch (pins) {
		case 16: // Pin P1.04: Button pressed
			status[4] = 1;
			break;
		case 64: // Pin P1.06: Stick down
			status[2] = 1;
			break;
		case 32: // Pin P1.05: Stick up
			status[3] = 1;
			break;
		case 128: // Pin P1.07: Stick left
			status[1] = 1;
			break;
		case 256: // Pin P1.08: Stick right
			status[0] = 1;
			break;
		default:
			break;
	}
	
}

int get_status(void) {
	int result = 0;
	if (status[0]) {
		result += 8;
		LOG_INF("P1.08 status read: Right");
	}
	if (status[1]) {
		result += 4;
		LOG_INF("P1.07 status read: Left");
	}
	if (status[2]) {
		result += 2;
		LOG_INF("P1.06 status read: Down");
	}
	if (status[3]) {
		result += 1;
		LOG_INF("P1.05 status read: Up");
	}
	if (status[4]) {
		result += 16;
		LOG_INF("P1.04 status read: Button");
	}
	/* Clear the status array after reading */
	for (int i = 0; i < 5; i++) {
		status[i] = 0;
	}
	return result;
}

/* Returns a bitmask representing the status of pins P1.08, P1.06, P1.07, P1.08 and P1.04 */
int get_pin_status(void) {
	int result = 0;
	int pin_status = read_P1_05();
	if (pin_status < 0) {
		LOG_ERR("Failed to read P1.05 (error %d)", pin_status);
    } else {
		if (pin_status != pin_status_0) {
			result += 1;
			LOG_INF("P1.05 ON: Up");
		} 
		pin_status_0 = pin_status;
	}
	pin_status = read_P1_06();
	if (pin_status < 0) {
		LOG_ERR("Failed to read P1.06 (error %d)", pin_status);
	} else {		
		if (pin_status != pin_status_1) {
			result += 2;
			LOG_INF("P1.06 ON: Down");
		} 	
		pin_status_1 = pin_status;
	}
	pin_status = read_P1_07();
	if (pin_status < 0) {
		LOG_ERR("Failed to read P1.07 (error %d)", pin_status);
	} else {
		if (pin_status != pin_status_2) {
			result += 4;
			LOG_INF("P1.07 ON: Left");
		} 
		pin_status_2 = pin_status;
	}
	pin_status = read_P1_08();
	if (pin_status < 0) {
		LOG_ERR("Failed to read P1.08 (error %d)", pin_status);
	} else {
		if (pin_status != pin_status_3) {
			result += 8;
			LOG_INF("P1.08 ON: Right");
		}
		pin_status_3 = pin_status;
	}
	pin_status = read_P1_04();
	if (pin_status < 0) {
		LOG_ERR("Failed to read P1.04 (error %d)", pin_status);
	} else {
		pin_status_4 = pin_status;
		if (pin_status != pin_status_4) {
			result += 16;
			LOG_INF("P1.04 ON: Button");
		}
		pin_status_4 = pin_status;
	}

	return result;
}

int configure_joystick(void) {
	int err;
	if (gpio_is_ready_dt(&stick_up)) {
		
		err = gpio_pin_configure_dt(&stick_up, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&callback_up, stick_callback, 
			BIT(stick_up.pin));
				   		   

		err = gpio_add_callback(stick_up.port, &callback_up);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&stick_up,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable stick up callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Stick up %s is not ready", stick_up.port->name);
		return -1;
	}

	if (gpio_is_ready_dt(&stick_down)) {
		err = gpio_pin_configure_dt(&stick_down, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure stick down gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&callback_down, stick_callback, 
			BIT(stick_down.pin));

		err = gpio_add_callback(stick_down.port, &callback_down);
		if (err) {
			LOG_ERR("Failed to add stick down callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&stick_down,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable stick down callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Stick down %s is not ready", stick_down.port->name);
		return -1;
	}

	if (gpio_is_ready_dt(&stick_left)) {
		err = gpio_pin_configure_dt(&stick_left, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure stick left gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&callback_left, stick_callback, 
			BIT(stick_left.pin));

		err = gpio_add_callback(stick_left.port, &callback_left);
		if (err) {
			LOG_ERR("Failed to add stick left callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&stick_left,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable stick left callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Stick left %s is not ready", stick_left.port->name);
		return -1;
	}

	if (gpio_is_ready_dt(&stick_right)) {
		err = gpio_pin_configure_dt(&stick_right, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure stick right gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&callback_right, stick_callback, 
			BIT(stick_right.pin));

		err = gpio_add_callback(stick_right.port, &callback_right);
		if (err) {
			LOG_ERR("Failed to add stick right callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&stick_right,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable stick right callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Stick right %s is not ready", stick_right.port->name);
		return -1;
	}

	if (gpio_is_ready_dt(&stick_button)) {
		err = gpio_pin_configure_dt(&stick_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&callback_button, stick_callback, 
			BIT(stick_button.pin));

		err = gpio_add_callback(stick_button.port, &callback_button);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&stick_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			//return -1;
		}
	} else {
		LOG_ERR("Button %s is not ready", stick_button.port->name);
		return -1;
	}

	LOG_INF("Joysick joyfully configured");

	return 0;
}
