#include <lvgl.h>
#include "ui.h"

int main(void) {
    lv_init();
    lv_sdl_window_create(1024, 600);
    lv_sdl_mouse_create();
    ui_create();
    while (1) {
        uint32_t wait_ms = lv_timer_handler();
        lv_delay_ms(wait_ms > 20 ? 20 : (wait_ms ? wait_ms : 1));
    }
    return 0;
}
