#include <zephyr/logging/log.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <string.h>

char count_str[11] = {0};
char info_str[21] = {0};	
static lv_style_t end_txt_style;
static lv_style_t start_txt_style;
lv_obj_t *count_label;
lv_obj_t *info_label;
lv_obj_t *player;
lv_obj_t *food = NULL;
lv_obj_t *hunter = NULL;

LOG_MODULE_REGISTER(display);

int configure_display() {
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

    return 0;
}

void init_display() {
    lv_style_init(&end_txt_style);
	lv_style_set_text_font(&end_txt_style, &lv_font_montserrat_28);
	lv_style_init(&start_txt_style);
	lv_style_set_text_font(&start_txt_style, &lv_font_montserrat_22);
}

void show_info_label(char *text) {
    if (info_label == NULL) {
        info_label = lv_label_create(lv_screen_active());
	    lv_obj_add_style(info_label, &start_txt_style, LV_STATE_DEFAULT);
    }
	lv_label_set_text(info_label, text);
	lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);
	lv_timer_handler();
}

void remove_info_label() {
	if (info_label != NULL) {
		lv_obj_del(info_label);
		info_label = NULL;
	}
}

void update_count_label(int hit_count) {
    if (count_label == NULL) {
        count_label = lv_label_create(lv_screen_active());
        lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
	sprintf(count_str, "%d", hit_count);
	lv_label_set_text(count_label, count_str);
	lv_timer_handler();
}

void remove_count_label() {
    if (count_label != NULL) {
        lv_obj_del(count_label);
        count_label = NULL;
    }
}

void create_player() {
    if (player == NULL) {
        player = lv_obj_create(lv_screen_active());
	    lv_obj_set_size(player, 25, 25);
	    lv_obj_set_style_radius(player, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    }
	lv_obj_align(player, LV_ALIGN_CENTER, 0, 0);
    lv_timer_handler();
}

void update_player_position(int x, int y) {
    lv_obj_align(player, LV_ALIGN_CENTER, x, y);
    lv_timer_handler();
}

void add_food(int x, int y) {
    if (food == NULL) {
        food = lv_obj_create(lv_screen_active());
        lv_obj_set_size(food, 15, 15);
        lv_obj_set_style_radius(food, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    }
    lv_obj_align(food, LV_ALIGN_CENTER, x, y);
    lv_timer_handler();
}

void remove_food() {
    if (food != NULL) {
        lv_obj_del(food);
        food = NULL;
    }
}

void add_hunter(int x, int y) {
    if (hunter == NULL) {
        hunter = lv_obj_create(lv_screen_active());
        lv_obj_set_size(hunter, 25, 25);
        lv_obj_set_style_radius(hunter, 0, LV_PART_MAIN);
    }
    lv_obj_align(hunter, LV_ALIGN_CENTER, x, y);
    lv_timer_handler();
}

void remove_hunter() {
    if (hunter != NULL) {
        lv_obj_del(hunter);
        hunter = NULL;
    }
}

void update_hunter_position(int x, int y) {
    lv_obj_align(hunter, LV_ALIGN_CENTER, x, y);
    lv_timer_handler();
}

void remove_player() {
    if (player != NULL) {
        lv_obj_del(player);
        player = NULL;
    }
}
