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

#include "joystick.h"
#include "display.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(main);

static uint32_t time_count;
static uint32_t hit_count = 0;
static int32_t offset[2] = {0, 0};
static int is_hunter_active = 0;
int has_food = 0;
// Food coordinates let's say the food is at (1000, 1000) which is outside the display when not displayed.
static int food_coordinates[2] = {1000, 1000};
static int hunter_coordinates[2] = {1000, 1000};
static int use_callback = 1;
static int fire = 0;

static struct gpio_dt_spec mode_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_dt_spec fire_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static struct gpio_callback button_callback_mode;
static struct gpio_callback button_callback_fire;

static void start_game();

/* Move player left */
static void move_player_left() {
	offset[0] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[1], 2)));
	if (offset[0] < -max_offset) {
		offset[0] = max_offset;
	}
}

/* Move player right */
static void move_player_right() {
	offset[0] += 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[1], 2)));
	if (offset[0] > max_offset) {
		offset[0] = -max_offset;
	}
}

/* Move player up */
static void move_player_up() {
	offset[1] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(offset[0], 2)));
	if (offset[1] < -max_offset) {
		offset[1] = max_offset;
	}
}

/* Move player down */
static void move_player_down() {
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
	int  y = rand() % 241 - 120; // Random y offset between -120 and 120
	int max_offset = (int)(sqrt(14400 - pow(y, 2)));
	int x = rand() % (2 * max_offset + 1) - max_offset; // Random x offset based on y
	food_coordinates[0] = x;
	food_coordinates[1] = y;
	add_food(x, y);
	LOG_INF("New food at [%d, %d]", x, y);
}

static int check_collision() {
	int dx;
	int dy;
	if (is_hunter_active) {
		dx = offset[0] - hunter_coordinates[0];
		dy = offset[1] - hunter_coordinates[1];
	} else if (has_food) {
		dx = offset[0] - food_coordinates[0];
		dy = offset[1] - food_coordinates[1];
	} else {
		return 0; // No collision if neither hunter nor food is active
	}
	int distance_squared = dx * dx + dy * dy;
	if (distance_squared <= 100) { // 10^2 = 100
		return 1; // Collision detected
	} else {
		return 0; // No collision
	}
	
}

static void create_hunter() {
	hunter_coordinates[0] = food_coordinates[0];
	hunter_coordinates[1] = food_coordinates[1];
	remove_food();
	add_hunter(hunter_coordinates[0], hunter_coordinates[1]);
	is_hunter_active = 1;
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

static void move_player() {
	int pin_status = use_callback ? get_status() : get_pin_status();
	if (pin_status != 0) {
		LOG_INF("Pin status: %d", pin_status);
		if (pin_status & 4) { 
			move_player_left();
		}
		if (pin_status & 8) { 
			move_player_right();
		}
		if (pin_status & 1) { 
			move_player_up();
		}
		if (pin_status & 2) { 
			move_player_down();
		}
		if (pin_status & 16) { 
			return;
		}
		update_player_position(offset[0], offset[1]);
	}
}

static void show_start_screen() {
	show_info_label("Press fire to start");
	LOG_INF("Start screen up, waiting for button press");
	while (1) {
		if (is_button_pressed() || fire) {
			fire = 0;
			start_game();
		} 
	}
}

static void end_game() {
	remove_player();
	show_info_label("GAME OVER");
	LOG_INF("Game over!");
	while (1) {
		if (is_button_pressed() || fire) {
			fire = 0;
			remove_info_label();
			remove_count_label();
			show_start_screen();
		}
	}
}

static void play_game() {
	int food_time = 250;
	int hunter_time = 750;
	LOG_INF("Let the game begin...");
	while (1) {
		/* Increment the time count */
		++time_count;
		move_player();
		if (time_count == food_time) {
			show_food();
			has_food = 1;
			LOG_INF("New food created at [%d, %d]", food_coordinates[0], food_coordinates[1]);
		} else if (time_count == hunter_time) {
				remove_food();
				has_food = 0;
				create_hunter();
				is_hunter_active = 1;
				LOG_INF("Hunter created at [%d, %d]", hunter_coordinates[0], hunter_coordinates[1]);
		} else if (time_count == 1250) {
				remove_hunter();
				time_count = 0;
				is_hunter_active = 0;
				LOG_INF("Hunter removed");
		} else {
			if (check_collision()) {			
				if (is_hunter_active) {
					LOG_INF("Collision detected when on [%d, %d] with hunter at [%d, %d]", offset[0], offset[1], hunter_coordinates[0], hunter_coordinates[1]);
					remove_hunter();
					hunter_coordinates[0] = 1000;
					hunter_coordinates[1] = 1000;
					time_count = 0;
					end_game();
				}
				if (has_food) {
					LOG_INF("Collision detected when on [%d, %d] with food at [%d, %d]", offset[0], offset[1], food_coordinates[0], food_coordinates[1]);
					remove_food();
					food_coordinates[0] = 1000;
					food_coordinates[1] = 1000;
					update_count_label(++hit_count);
					time_count = 0;
					if (hit_count%5 == 0) {
						food_time = (food_time>0)? (food_time-50): 1;
						hunter_time = (hunter_time>200)? (hunter_time-50): 200; 
					}
				}	
			} else if (is_hunter_active) {
				// move the hunter one pxel closer to the player
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
				update_hunter_position(hunter_coordinates[0], hunter_coordinates[1]);
			} 
		}
		
		k_sleep(K_USEC(9501));
	}
}

static void reset_game_var() {
	hit_count = 0;
	time_count = 0;
	has_food = 0;
	is_hunter_active = 0;
	offset[0] = 0;
	offset[1] = 0;
}

static void start_game() {
	reset_game_var();
	srand(time(NULL));
	remove_info_label();
	create_player();
	update_count_label(hit_count);
	play_game();
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
