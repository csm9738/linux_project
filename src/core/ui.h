#ifndef UI_H
#define UI_H

typedef enum {
    MAIN_SCREEN,
    CUSTOMIZE_SCREEN
} ScreenState;

int start_ui(const char* git_log_filepath);

#endif // UI_H