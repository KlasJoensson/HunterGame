#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "joystick.h"
#include "display.h"

LOG_MODULE_REGISTER(game);

static uint32_t time_count;
static uint32_t hit_count = 0;
static int32_t player_coordinates[2] = {0, 0};
static int is_hunter_active = 0;
int has_food = 0;
// Food coordinates let's say the food is at (1000, 1000) which is outside the display when not displayed.
static int food_coordinates[2] = {1000, 1000};
static int hunter_coordinates[2] = {1000, 1000};
static int fire = 0;
static int food_time = 250;
static int hunter_time = 750;
static int use_callback = 1;

static void start_game();
void show_start_screen();

int set_callback() {
    use_callback = !use_callback;
    return use_callback;
}

void fake_button_pressed() {
	fire = 1;
}

/* Move player left */
static void move_player_left() {
	player_coordinates[0] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(player_coordinates[1], 2)));
	if (player_coordinates[0] < -max_offset) {
		player_coordinates[0] = max_offset;
	}
}

/* Move player right */
static void move_player_right() {
	player_coordinates[0] += 10;
	int max_offset = (int)(sqrt(14400 - pow(player_coordinates[1], 2)));
	if (player_coordinates[0] > max_offset) {
		player_coordinates[0] = -max_offset;
	}
}

/* Move player up */
static void move_player_up() {
	player_coordinates[1] -= 10;
	int max_offset = (int)(sqrt(14400 - pow(player_coordinates[0], 2)));
	if (player_coordinates[1] < -max_offset) {
		player_coordinates[1] = max_offset;
	}
}

/* Move player down */
static void move_player_down() {
	player_coordinates[1] += 10;
	int max_offset = (int)(sqrt(14400 - pow(player_coordinates[0], 2)));
	if (player_coordinates[1] > max_offset) {
		player_coordinates[1] = -max_offset;
	}
}

static void show_food() {
    has_food = 1;
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
		dx = player_coordinates[0] - hunter_coordinates[0];
		dy = player_coordinates[1] - hunter_coordinates[1];
	} else if (has_food) {
		dx = player_coordinates[0] - food_coordinates[0];
		dy = player_coordinates[1] - food_coordinates[1];
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

static void reset_food() {
	remove_food();
	food_coordinates[0] = 1000;
	food_coordinates[1] = 1000;
	has_food = 0;
	LOG_INF("Food reset");
}

static void create_hunter() {
	hunter_coordinates[0] = food_coordinates[0];
	hunter_coordinates[1] = food_coordinates[1];
	reset_food();
	add_hunter(hunter_coordinates[0], hunter_coordinates[1]);
	is_hunter_active = 1;
    LOG_INF("Hunter created at [%d, %d]", hunter_coordinates[0], hunter_coordinates[1]);
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
		update_player_position(player_coordinates[0], player_coordinates[1]);
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

static void reset_hunter() {
	remove_hunter();
	hunter_coordinates[0] = 1000;
	hunter_coordinates[1] = 1000;
	time_count = 0;
	is_hunter_active = 0;
	LOG_INF("Hunter reset");
}

static void move_hunter() {
    // move the hunter one pxel closer to the player
    if (time_count % 10 == 0) { // Move the hunter every 10 loops (~100 ms)
        if (hunter_coordinates[0] < player_coordinates[0]) {
            hunter_coordinates[0]++;
        } else if (hunter_coordinates[0] > player_coordinates[0]) {
            hunter_coordinates[0]--;
        }
        if (hunter_coordinates[1] < player_coordinates[1]) {
            hunter_coordinates[1]++;
        } else if (hunter_coordinates[1] > player_coordinates[1]) {
            hunter_coordinates[1]--;
        }
        update_hunter_position(hunter_coordinates[0], hunter_coordinates[1]);
    }
}


static void play_game() {
	LOG_INF("Let the game begin...");
	while (1) {
		/* Increment the time count */
		++time_count;
		move_player();
		if (time_count == food_time) {
			show_food();
			LOG_INF("New food created at [%d, %d]", food_coordinates[0], food_coordinates[1]);
		} else if (time_count == hunter_time) {				
				create_hunter();	
		} else if (time_count == 1250) {
				reset_hunter();
		} else {
			if (check_collision()) {			
				if (is_hunter_active) {
					LOG_INF("Collision detected when on [%d, %d] with hunter at [%d, %d]", player_coordinates[0], player_coordinates[1], hunter_coordinates[0], hunter_coordinates[1]);
					reset_hunter();
					end_game();
				}
				if (has_food) {
					LOG_INF("Collision detected when on [%d, %d] with food at [%d, %d]", player_coordinates[0], player_coordinates[1], food_coordinates[0], food_coordinates[1]);
					reset_food();
					update_count_label(++hit_count);
					time_count = 0;
					if (hit_count%5 == 0) {
						food_time = (food_time>0)? (food_time-50): 1;
						hunter_time = (hunter_time>200)? (hunter_time-50): 200; 
					}
				}	
			} else if (is_hunter_active) {
				move_hunter();	
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
	player_coordinates[0] = 0;
	player_coordinates[1] = 0;
    food_time = 250;
	hunter_time = 750;
}

void start_game() {
	reset_game_var();
	srand(time(NULL));
	remove_info_label();
	create_player();
	update_count_label(hit_count);
	play_game();
}

void show_start_screen() {
	show_info_label("Press fire to start");
	LOG_INF("Start screen up, waiting for button press");
	while (1) {
		if (is_button_pressed() || fire) {
			fire = 0;
			start_game();
		} 
	}
}