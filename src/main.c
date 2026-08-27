#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>

#include <stdio.h>
#include <string.h>

#include <lvgl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(main);

static uint32_t count;
static int32_t offset_y = 0;
static int32_t offset_x = 0;

static struct gpio_dt_spec right_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static struct gpio_dt_spec left_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0});
static struct gpio_dt_spec up_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_dt_spec down_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0});
static struct gpio_callback button_callback_right;
static struct gpio_callback button_callback_left;
static struct gpio_callback button_callback_up;
static struct gpio_callback button_callback_down;

/* Move cirkle left */
static void button_left_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset_x -= 10;
}

/* Move cirkle right */
static void button_right_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset_x += 10;
}

/* Move cirkle up */
static void button_up_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset_y -= 10;
}

/* Move cirkle down */
static void button_down_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset_y += 10;
}

int main(void)
{
	char count_str[11] = {0};
	const struct device *display_dev;
	static lv_style_t my_style;
	lv_obj_t *count_label;
	int ret;

	/* Check if the display device is ready	 */
	display_dev =  DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device is not ready");
		return 0;
	}

	/* Setup button 1 to clear the time count */
	if (gpio_is_ready_dt(&left_button)) {
		int err;

		err = gpio_pin_configure_dt(&left_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return 0;
		}

		gpio_init_callback(&button_callback_left, button_left_callback,
				   		   BIT(left_button.pin));

		err = gpio_add_callback(left_button.port, &button_callback_left);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&left_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return 0;
		}
	}

	if (gpio_is_ready_dt(&right_button)) {
		int err;

		err = gpio_pin_configure_dt(&right_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return 0;
		}

		gpio_init_callback(&button_callback_right, button_right_callback,
				   		   BIT(right_button.pin));

		err = gpio_add_callback(right_button.port, &button_callback_right);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&right_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return 0;
		}
	}

	if (gpio_is_ready_dt(&up_button)) {
		int err;

		err = gpio_pin_configure_dt(&up_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return 0;
		}

		gpio_init_callback(&button_callback_up, button_up_callback,
				   		   BIT(up_button.pin));

		err = gpio_add_callback(up_button.port, &button_callback_up);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&up_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return 0;
		}
	}

	if (gpio_is_ready_dt(&down_button)) {
		int err;

		err = gpio_pin_configure_dt(&down_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return 0;
		}

		gpio_init_callback(&button_callback_down, button_down_callback,
				   		   BIT(down_button.pin));

		err = gpio_add_callback(down_button.port, &button_callback_down);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&down_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return 0;
		}
	}
	
	lv_style_init(&my_style);
	lv_style_set_text_font(&my_style, &lv_font_montserrat_28);

	/* Create a label to display time count and align it at the bottom center */
	count_label = lv_label_create(lv_screen_active());
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	/* Draw a 25px diameter circle in the middle of the display */
	lv_obj_t *circle = lv_obj_create(lv_screen_active());
	lv_obj_set_size(circle, 25, 25);
	lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
	lv_obj_align(circle, LV_ALIGN_CENTER, 0, 0);

	lv_timer_handler();

	/* Turn off display blanking to ensure the display is active
	 * Check the return value and if it is less than 0 and not equal to -ENOSYS,
	 * log an error message and return 0
	 */
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return 0;
	}

	/* Loop and display increasing time count */
	while (1) {
		
		lv_obj_align(circle, LV_ALIGN_CENTER, offset_x, offset_y);
		
		/* To update the display, call the LVGL timer handler */
		lv_timer_handler();
		/* Increment the time count */
		++count;
		/* Delay for 9.501 ms - each loop will represent ~10 ms */
		k_sleep(K_USEC(9501));
	}
}
