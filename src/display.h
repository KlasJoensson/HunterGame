#ifndef DISPLAY_H
#define DISPLAY_H

int configure_display();
void init_display();
void show_info_label(char *text);
void remove_info_label();
void update_count_label(int hit_count);
void remove_count_label();


void create_player();
void update_player_position(int x, int y);
void remove_player();

void add_food(int x, int y);
void remove_food();

void add_hunter(int x, int y);
void remove_hunter();
void update_hunter_position(int x, int y);

#endif // DISPLAY_H