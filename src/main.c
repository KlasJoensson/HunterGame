#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>


#include <lvgl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(main);

static uint32_t time_count;
static uint32_t hit_count = 0;
static int32_t offset[2] = {0, 0};
static int has_food = 0;
static int is_hunter_active = 0;
lv_obj_t *food = NULL;
lv_obj_t *hunter = NULL;
// Food coordinates let's say the food is at (1000, 1000) which is outside the display when not displayed.
static int food_coordinates[2] = {1000, 1000};
static int hunter_coordinates[2] = {1000, 1000};

static struct gpio_dt_spec right_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static struct gpio_dt_spec left_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0});
static struct gpio_dt_spec up_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_dt_spec down_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0});
static struct gpio_callback button_callback_right;
static struct gpio_callback button_callback_left;
static struct gpio_callback button_callback_up;
static struct gpio_callback button_callback_down;

/* Move circle left */
static void button_left_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset[0] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[1], 2)));
	if (offset[0] < -max_offset) {
		offset[0] = max_offset;
	}
}

/* Move circle right */
static void button_right_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset[0] += 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[1], 2)));
	if (offset[0] > max_offset) {
		offset[0] = -max_offset;
	}
}

/* Move circle up */
static void button_up_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset[1] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[0], 2)));
	if (offset[1] < -max_offset) {
		offset[1] = max_offset;
	}
}

/* Move circle down */
static void button_down_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	offset[1] += 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[0], 2)));
	if (offset[1] > max_offset) {
		offset[1] = -max_offset;
	}
}

static void show_food() {
	if (food == NULL) {
		has_food = 1;
		food = lv_obj_create(lv_screen_active());
		lv_obj_set_size(food, 15, 15);
		lv_obj_set_style_radius(food, LV_RADIUS_CIRCLE, LV_PART_MAIN);
	}
	int  y = rand() % 241 - 120; // Random y offset between -120 and 120
	int max_offset = (int)(sqrt(14400 - pow(y, 2)));
	int x = rand() % (2 * max_offset + 1) - max_offset; // Random x offset based on y 
	lv_obj_align(food, LV_ALIGN_CENTER, x, y);
	food_coordinates[0] = x;
	food_coordinates[1] = y;
	LOG_INF("New circle at [%d, %d].", x, y);
}

static int check_collision() {
	int dx;
	int dy;
	if (food_coordinates[0] == 1000 && !(hunter_coordinates[0] > 500)) {
		dx = offset[0] - hunter_coordinates[0];
		dy = offset[1] - hunter_coordinates[1];
	} else {
		dx = offset[0] - food_coordinates[0];
		dy = offset[1] - food_coordinates[1];
	}
	int distance_squared = dx * dx + dy * dy;
	if (distance_squared <= 100) { // 10^2 = 100
		return 1; // Collision detected
	} else {
		return 0; // No collision
	}
	
}

// Move new circle out of view when it is not displayed
static void hide_food() {
	if (has_food) {
		food_coordinates[0] = 1000; // Move food coordinates out of view
		food_coordinates[1] = 1000;
		lv_obj_align(food, LV_ALIGN_CENTER, food_coordinates[0], food_coordinates[1]);
		LOG_INF("New circle hidden.");
	}
}

static void create_hunter() {
	// create a square at the same place as the circle to represent the hunter
	if (hunter == NULL) {
		hunter = lv_obj_create(lv_screen_active());
		lv_obj_set_size(hunter, 20, 20);
		lv_obj_set_style_radius(hunter, 0, LV_PART_MAIN);
	} 
	hunter_coordinates[0] = food_coordinates[0];
	hunter_coordinates[1] = food_coordinates[1];
	hide_food();
	is_hunter_active = 1;
	lv_obj_align(hunter, LV_ALIGN_CENTER, hunter_coordinates[0], hunter_coordinates[1]);
}

static void remove_hunter() {
	if (hunter != NULL) {
		lv_obj_del(hunter);
		hunter = NULL;
	}
	is_hunter_active = 0;
	hunter_coordinates[0] = 1000; // Move hunter coordinates out of view
	hunter_coordinates[1] = 1000;
}

static int configure_left_button() {
	/* Setup button 1 to clear the time count */
	if (gpio_is_ready_dt(&left_button)) {
		int err;

		err = gpio_pin_configure_dt(&left_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&button_callback_left, button_left_callback, 
			BIT(left_button.pin));
				   		   

		err = gpio_add_callback(left_button.port, &button_callback_left);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&left_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Button device %s is not ready", left_button.port->name);
		return -1;
	}

	return 0;
}

static int configure_right_button() {
	/* Setup button 1 to clear the time count */
	if (gpio_is_ready_dt(&right_button)) {
		int err;

		err = gpio_pin_configure_dt(&right_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&button_callback_right, button_right_callback, 
			BIT(right_button.pin));
				   		   

		err = gpio_add_callback(right_button.port, &button_callback_right);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&right_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Button device %s is not ready", right_button.port->name);
		return -1;
	}

	return 0;
}

static int configure_up_button() {
	/* Setup button 1 to clear the time count */
	if (gpio_is_ready_dt(&up_button)) {
		int err;

		err = gpio_pin_configure_dt(&up_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&button_callback_up, button_up_callback, 
			BIT(up_button.pin));
				   		   

		err = gpio_add_callback(up_button.port, &button_callback_up);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&up_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Button device %s is not ready", up_button.port->name);
		return -1;
	}

	return 0;
}

static int configure_down_button() {
	/* Setup button 1 to clear the time count */
	if (gpio_is_ready_dt(&down_button)) {
		int err;

		err = gpio_pin_configure_dt(&down_button, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -1;
		}

		gpio_init_callback(&button_callback_down, button_down_callback, 
			BIT(down_button.pin));
				   		   

		err = gpio_add_callback(down_button.port, &button_callback_down);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -1;
		}

		err = gpio_pin_interrupt_configure_dt(&down_button,
						      				  GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to enable button callback: %d", err);
			return -1;
		}
	} else {
		LOG_ERR("Button device %s is not ready", down_button.port->name);
		return -1;
	}

	return 0;
}

static int configure_device() {
	int ret;
	const struct device *display_dev;
	/* Check if the display device is ready	 */
	display_dev =  DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device is not ready");
		return -1;
	}
	/* Turn off display blanking to ensure the display is active
	 * Check the return value and if it is less than 0 and not equal to -ENOSYS,
	 * log an error message and return 0
	 */
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return -1;
	}

	/*configure left btn*/
	ret = configure_left_button();
	if (ret < 0) {
		LOG_ERR("Failed to configure left button");
		return -1;
	}
	/*configure right btn*/
	ret = configure_right_button();
	if (ret < 0) {
		LOG_ERR("Failed to configure right button");
		return -1;
	}
	/*configure up btn*/
	ret = configure_up_button();
	if (ret < 0) {
		LOG_ERR("Failed to configure up button");
		return -1;
	}
	/*configure down btn*/
	ret = configure_down_button();
	if (ret < 0) {
		LOG_ERR("Failed to configure down button");
		return -1;
	}
	
	LOG_INF("Device successfully configured.");

	return 0;
}

int main(void) {
	int ret = configure_device();
	if (ret < 0) {
		LOG_ERR("Could not configure the device");
		return -1;
	}

	char count_str[11] = {0};	
	static lv_style_t my_style;
	lv_obj_t *count_label;
	srand(time(NULL));
	
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

	/* Loop and display increasing time count */
	while (1) {
		
		lv_obj_align(circle, LV_ALIGN_CENTER, offset[0], offset[1]);
		sprintf(count_str, "%d", hit_count);
		lv_label_set_text(count_label, count_str);
		/* To update the display, call the LVGL timer handler */
		lv_timer_handler();
		/* Increment the time count */
		++time_count;
		if (time_count == 1000) {
			show_food();
			LOG_INF("New circle created at [%d, %d].", food_coordinates[0], food_coordinates[1]);
		} else if (time_count == 1500) {
				create_hunter();
				LOG_INF("Hunter created at [%d, %d].", hunter_coordinates[0], hunter_coordinates[1]);
		} else if (time_count == 2000) {
				remove_hunter();
				time_count = 0;
				LOG_INF("Hunter removed.");
		} else {
			if (check_collision()) {			
				if (is_hunter_active) {
					LOG_INF("Collision detected when on [%d, %d] with hunter at [%d, %d].", offset[0], offset[1], hunter_coordinates[0], hunter_coordinates[1]);
					remove_hunter();
					hit_count--;
					time_count = 0;
				} else {
					LOG_INF("Collision detected when on [%d, %d] with food at [%d, %d].", offset[0], offset[1], food_coordinates[0], food_coordinates[1]);
					hide_food();
					hit_count++;
					time_count = 0;
				}	
			} else if (is_hunter_active) {
				// move the hunter one pxel closer to the circle
				if (time_count % 10 == 0) { // Move the hunter every 10 loops (~100 ms)
					if (hunter_coordinates[0] < offset[0]) {
						hunter_coordinates[0]++;
					} else if (hunter_coordinates[0] > offset[0]) {
						hunter_coordinates[0]--;
					}
					if (hunter_coordinates[1] < offset[1]) {
						hunter_coordinates[1]++;
					} else if (hunter_coordinates[1] > offset[1]) {
						hunter_coordinates[1]--;
					}
				}
				lv_obj_align(hunter, LV_ALIGN_CENTER, hunter_coordinates[0], hunter_coordinates[1]);
			} 
		}

		/* Delay for 9.501 ms - each loop will represent ~10 ms */
		k_sleep(K_USEC(9501));
	}

	return 0;
}
