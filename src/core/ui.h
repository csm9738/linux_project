#ifndef UI_H
#define UI_H

typedef enum {
    MAIN_SCREEN,
    CUSTOMIZE_SCREEN,
    PALETTE_EDIT_SCREEN
} ScreenState;

int start_ui(const char* git_log_filepath, const char* project_root);

#endif // UI_H