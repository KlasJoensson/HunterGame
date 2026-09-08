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
#include "joystick.h"

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
static int use_callback = 0;
static int fire = 0;

char count_str[11] = {0};
char info_str[21] = {0};	
static lv_style_t end_txt_style;
static lv_style_t start_txt_style;
lv_obj_t *count_label;
lv_obj_t *info_label;
lv_obj_t *circle;

static struct gpio_dt_spec mode_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_dt_spec fire_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static struct gpio_callback button_callback_mode;
static struct gpio_callback button_callback_fire;

static void start_game();

/* Move circle left */
static void move_circle_left() {
	offset[0] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[1], 2)));
	if (offset[0] < -max_offset) {
		offset[0] = max_offset;
	}
}

/* Move circle right */
static void move_circle_right() {
	offset[0] += 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[1], 2)));
	if (offset[0] > max_offset) {
		offset[0] = -max_offset;
	}
}

/* Move circle up */
static void move_circle_up() {
	offset[1] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[0], 2)));
	if (offset[1] < -max_offset) {
		offset[1] = max_offset;
	}
}

/* Move circle down */
static void move_circle_down() {
	offset[1] += 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[0], 2)));
	if (offset[1] > max_offset) {
		offset[1] = -max_offset;
	}
}

static void button_mode_callback(const struct device *port,
								struct gpio_callback *cb,
								uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	use_callback = !use_callback;
	LOG_INF("Use callback: %d", use_callback);
}

static void fire_button_callback(const struct device *port,
								 struct gpio_callback *cb,
								 uint32_t pins) {
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	LOG_INF("Fake fire button pressed");
	fire = 1;
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

	LOG_INF("Device successfully configured.");

	return 0;
}

static void move_circle() {
	int pin_status = use_callback ? get_status() : get_pin_status();
	if (pin_status != 0) {
		LOG_INF("Pin status: %d", pin_status);
	}
	
	if (pin_status & 4) { 
		move_circle_left();
	}
	if (pin_status & 8) { 
		move_circle_right();
	}
	if (pin_status & 1) { 
		move_circle_up();
	}
	if (pin_status & 2) { 
		move_circle_down();
	}
	lv_obj_align(circle, LV_ALIGN_CENTER, offset[0], offset[1]);	
}

static void show_start_screen() {
	hit_count = 0;
	info_label = lv_label_create(lv_screen_active());
	lv_obj_add_style(info_label, &start_txt_style, LV_STATE_DEFAULT);
	lv_label_set_text(info_label, "Press fire to start");
	lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);
	lv_timer_handler();
	LOG_INF("Start screen up, waiting for button press");
	int pin_status;
	while (1) {
		pin_status = use_callback ? get_status() : get_pin_status();
		if (pin_status != 0) {
			LOG_INF("Pin status: %d", pin_status);
		}
		if (pin_status == 16) {
			lv_obj_del(info_label);
			start_game();
		} else if (fire) {
			fire = 0;
			lv_obj_del(info_label);
			start_game();
		}
	}
}

static void end_game() {
	lv_obj_del(circle);

	info_label = lv_label_create(lv_screen_active());
	lv_obj_add_style(info_label, &end_txt_style, LV_STATE_DEFAULT);
	lv_label_set_text(info_label, "GAME OVER");
	lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);
	lv_timer_handler();
	LOG_INF("Game over!");
	int pin_status;
	while (1) {
		pin_status = use_callback ? get_status() : get_pin_status();
		if (pin_status != 0) {
			LOG_INF("Pin status: %d", pin_status);
		}
		if (pin_status == 16) {
			lv_obj_del(info_label);
			lv_obj_del(count_label);
			show_start_screen();
		} else if (fire) {
			fire = 0;
			lv_obj_del(info_label);
			lv_obj_del(count_label);
			show_start_screen();
		}
	}
}

static void play_game() {
	LOG_INF("Let the game begin...");
	while (1) {
		move_circle();
		/* Increment the time count */
		++time_count;
		if (time_count == 250) {
			show_food();
			LOG_INF("New circle created at [%d, %d].", food_coordinates[0], food_coordinates[1]);
		} else if (time_count == 750) {
				create_hunter();
				LOG_INF("Hunter created at [%d, %d].", hunter_coordinates[0], hunter_coordinates[1]);
		} else if (time_count == 1250) {
				remove_hunter();
				time_count = 0;
				LOG_INF("Hunter removed.");
		} else {
			if (check_collision()) {			
				if (is_hunter_active) {
					LOG_INF("Collision detected when on [%d, %d] with hunter at [%d, %d].", offset[0], offset[1], hunter_coordinates[0], hunter_coordinates[1]);
					remove_hunter();
					time_count = 0;
					end_game();
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
		sprintf(count_str, "%d", hit_count);
		lv_label_set_text(count_label, count_str);
		lv_timer_handler();
		k_sleep(K_USEC(9501));
	}
}

static void start_game() {
	srand(time(NULL));
    /* Create a label to display time count and align it at the bottom center */
	count_label = lv_label_create(lv_screen_active());
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	/* Draw a 25px diameter circle in the middle of the display */
	circle = lv_obj_create(lv_screen_active());
	lv_obj_set_size(circle, 25, 25);
	lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
	lv_obj_align(circle, LV_ALIGN_CENTER, 0, 0);
	sprintf(count_str, "%d", hit_count);
	lv_label_set_text(count_label, count_str);
	/* To update the display, call the LVGL timer handler */
	lv_timer_handler();
	play_game();
}



int main(void) {
	int ret = configure_device();
	if (ret < 0) {
		LOG_ERR("Could not configure the device");
		return -1;
	}

	ret = configure_joystick();
	if (ret < 0) {
		LOG_ERR("Could not configure the joystick");
		return -1;
	}

	lv_style_init(&end_txt_style);
	lv_style_set_text_font(&end_txt_style, &lv_font_montserrat_28);
	lv_style_init(&start_txt_style);
	lv_style_set_text_font(&start_txt_style, &lv_font_montserrat_22);
	
	show_start_screen();

	return 0;
}
