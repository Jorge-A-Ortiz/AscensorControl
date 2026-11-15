#include <lvgl.h>
#include "ui.h"
#include <stdio.h>
#include "platform_audio.h"

static lv_obj_t *floor_labels[2], *arrows[2], *door_image, *status_label, *door_warning;
static lv_obj_t *floor_buttons[3];
static lv_obj_t *emergency_label;
static lv_obj_t *door_left_leaf, *door_right_leaf;
static int32_t last_door_frame = -1;
static int current_floor = 1, target_floor = 0;
static int phase = 0; /* 0: en espera, 1: viajando */
static int pending[2] = {0, 0};
static unsigned pressed_mask = 0;
static bool emergency_active = false;
static bool waiting_started = false;
static uint32_t wait_start, move_start, arrival_time;
static bool arrival_dwell = false;
static int door_opening = 0;
static void door_opened(lv_anim_t *animation) {
    (void)animation;
    door_opening = 0;
    if (!pending[0] && !emergency_active) lv_label_set_text(status_label, "Puertas abiertas");
}

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y); lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
static lv_obj_t *label(lv_obj_t *parent, const char *text, uint32_t color, const lv_font_t *font) {
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, text); lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(o, font, 0); return o;
}
static void displays(int floor, int direction) {
    char buf[4]; snprintf(buf, sizeof(buf), "%d", floor);
    for (int i = 0; i < 2; i++) {
        if (!floor_labels[i] || !arrows[i]) continue;
        lv_label_set_text(floor_labels[i], buf);
        lv_label_set_text(arrows[i], direction > 0 ? LV_SYMBOL_UP : direction < 0 ? LV_SYMBOL_DOWN : "-");
    }
}
/* Dibuja los mismos 60 estados de la hoja, sin decodificar una imagen
 * de 2400 x 2160 en cada refresco. Solo se actualiza el area de la puerta. */
static void door_frame(void *image, int32_t frame) {
    (void)image;
    if (frame == last_door_frame) return;
    last_door_frame = frame;
    int32_t width = (97 * frame + 29) / 59;
    if (width == 0) {
        lv_obj_add_flag(door_left_leaf, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(door_right_leaf, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(door_left_leaf, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(door_right_leaf, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(door_left_leaf, width);
        lv_obj_set_width(door_right_leaf, width);
        lv_obj_set_x(door_right_leaf, 217 - width);
    }
}
static void door_closed(lv_anim_t *animation) {
    (void)animation;
    lv_label_set_text(door_warning, "");
}
static void doors(int open) {
    /* Revertir desde el cuadro visible, sin saltar a un extremo. */
    int32_t start_frame = last_door_frame < 0 ? 0 : last_door_frame;
    lv_anim_delete(door_image, door_frame);
    if (!open) door_opening = 0;
    int32_t end_frame = open ? 0 : 59;
    uint32_t full_time = open ? 1000 : 5000;
    uint32_t remaining_frames = open ? (uint32_t)start_frame : (uint32_t)(59 - start_frame);
    if (!remaining_frames) {
        door_frame(door_image, end_frame);
        if (open) { door_opening = 0; lv_label_set_text(status_label, "Puertas abiertas"); }
        else lv_label_set_text(door_warning, "");
        return;
    }
    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a, door_image); lv_anim_set_exec_cb(&a, door_frame);
    lv_anim_set_values(&a, start_frame, end_frame);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_completed_cb(&a, open ? door_opened : door_closed);
    lv_anim_set_time(&a, (full_time * remaining_frames + 58) / 59);
    lv_anim_start(&a);
}
static void update_buttons(void) {
    for (int i = 0; i < 3; i++) {
        int floor = i + 1;
        bool requested = target_floor == floor || pending[0] == floor || pending[1] == floor;
        lv_obj_set_style_border_color(floor_buttons[i], lv_color_hex(requested ? 0xffb43b : 0x3e4144), 0);
    }
}
static void sort_pending(void) {
    if (!pending[0] || !pending[1] || phase != 0) return;
    if ((current_floor == 1 && pending[0] > pending[1]) ||
        (current_floor == 3 && pending[0] < pending[1])) {
        int tmp = pending[0]; pending[0] = pending[1]; pending[1] = tmp;
    }
}
static void start_door_closing(void) {
    if (door_opening || last_door_frame < 59) {
        lv_label_set_text(door_warning, "Cuidado con las manos");
        doors(0);
    }
}
static void begin_waiting(void) {
    if (!pending[0] || emergency_active) return;
    sort_pending();
    waiting_started = false; /* Temporizador PLC requiere botones liberados. */
    arrival_dwell = false;
    start_door_closing();
    lv_label_set_text(status_label, "Esperando para iniciar viaje");
}
static void step(lv_timer_t *t) {
    (void)t;
    if (emergency_active) return;
    uint32_t now = lv_tick_get();
    if (phase == 0) {
        if (!pending[0]) { waiting_started = false; return; }
        sort_pending();
        if (arrival_dwell && lv_tick_elaps(arrival_time) >= 2500) {
            arrival_dwell = false;
            start_door_closing();
        }
        if (pressed_mask) { waiting_started = false; return; }
        if (!waiting_started) { wait_start = now; waiting_started = true; }
        uint32_t elapsed = lv_tick_elaps(wait_start);
        if (elapsed < 7500) {
            char msg[48];
            snprintf(msg, sizeof(msg), "Espera: %lu.%lu s", (unsigned long)((7500 - elapsed) / 1000),
                     (unsigned long)(((7500 - elapsed) % 1000) / 100));
            lv_label_set_text(status_label, msg);
            return;
        }
        arrival_dwell = false;
        lv_anim_delete(door_image, door_frame);
        door_opening = 0;
        door_frame(door_image, 59);
        target_floor = pending[0]; pending[0] = pending[1]; pending[1] = 0;
        waiting_started = false;
        phase = 1; move_start = now;
        lv_label_set_text(door_warning, "");
        displays(current_floor, target_floor > current_floor ? 1 : -1);
        lv_label_set_text(status_label, "Elevador en movimiento");
        update_buttons();
        return;
    }
    /* Posicion provisional para el simulador; no representa entradas X003-X005. */
    if (lv_tick_elaps(move_start) < 1100) return;
    move_start = now;
    current_floor += target_floor > current_floor ? 1 : -1;
    displays(current_floor, current_floor == target_floor ? 0 : (target_floor > current_floor ? 1 : -1));
    if (current_floor == target_floor) {
        platform_audio_play_arrival();
        phase = 0; target_floor = 0;
        lv_label_set_text(status_label, "Piso alcanzado - abriendo puertas");
        door_opening = 1; doors(1);
        update_buttons();
        if (pending[0]) {
            sort_pending();
            arrival_time = now;
            arrival_dwell = true;
            waiting_started = !pressed_mask;
            if (waiting_started) wait_start = now;
        }
    }
}
static void select_floor(lv_event_t *event) {
    int floor = (int)(intptr_t)lv_event_get_user_data(event);
    lv_event_code_t code = lv_event_get_code(event);
    unsigned bit = 1U << (floor - 1);
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        pressed_mask &= ~bit;
        return;
    }
    if (code != LV_EVENT_PRESSED) return;
    pressed_mask |= bit;
    if (phase == 0) waiting_started = false;
    if (phase == 0 && arrival_dwell && !emergency_active && floor != current_floor) {
        arrival_dwell = false;
        start_door_closing();
    }
    if (emergency_active || floor == target_floor || floor == pending[0] || floor == pending[1] ||
        (phase == 0 && floor == current_floor)) return;
    if (!pending[0]) pending[0] = floor;
    else if (!pending[1]) pending[1] = floor;
    else { lv_label_set_text(status_label, "Cola de solicitudes llena"); return; }
    update_buttons();
    if (phase == 0 && !waiting_started && last_door_frame < 59 && !lv_anim_get(door_image, door_frame)) begin_waiting();
    else if (phase == 0 && door_opening) begin_waiting();
    else if (phase == 0 && last_door_frame == 0) begin_waiting();
}
static void emergency(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    lv_obj_t *button = lv_event_get_target(event);
    emergency_active = !emergency_active;
    if (emergency_active) {
        lv_obj_add_state(button, LV_STATE_CHECKED);
        waiting_started = false; arrival_dwell = false;
        lv_obj_set_style_text_color(emergency_label, lv_color_hex(0xffffff), 0);
        lv_anim_delete(door_image, door_frame); door_opening = 0;
        lv_label_set_text(door_warning, "");
        displays(current_floor, 0);
        lv_label_set_text(status_label, "EMERGENCIA - simulacion detenida");
    } else {
        lv_obj_remove_state(button, LV_STATE_CHECKED);
        lv_obj_set_style_text_color(emergency_label, lv_color_hex(0x34464c), 0);
        if (phase == 1) {
            move_start = lv_tick_get();
            displays(current_floor, target_floor > current_floor ? 1 : -1);
            lv_label_set_text(status_label, "Elevador en movimiento");
        } else if (pending[0]) begin_waiting();
        else {
            door_opening = 1; doors(1);
            lv_label_set_text(status_label, "Abriendo puertas");
        }
    }
}
static void indicator(lv_obj_t *parent, int x, int y, int index) {
    lv_obj_t *frame = box(parent, x, y, 120, 57, 0x7a878c, 6);
    lv_obj_t *screen = box(frame, 5, 5, 110, 47, 0x101619, 3);
    floor_labels[index] = label(screen, "1", 0xffce65, &lv_font_montserrat_28);
    lv_obj_align(floor_labels[index], LV_ALIGN_CENTER, -18, 0);
    arrows[index] = label(screen, "-", 0xffce65, &lv_font_montserrat_24);
    lv_obj_align(arrows[index], LV_ALIGN_CENTER, 22, 0);
}
static const lv_style_prop_t button_transition_properties[] = {
    LV_STYLE_BG_COLOR, LV_STYLE_TRANSLATE_Y, LV_STYLE_SHADOW_WIDTH,
    LV_STYLE_PROP_INV
};
static lv_style_transition_dsc_t button_transition;

static void style_panel_button(lv_obj_t *button, int emergency_button) {
    lv_obj_remove_style_all(button);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0xdde0e1), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(emergency_button ? 0xb83b3b : 0xa7adaf), LV_STATE_PRESSED);
    if (emergency_button) {
        lv_obj_set_style_bg_color(button, lv_color_hex(0xb83b3b), LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x842a2a), LV_STATE_CHECKED | LV_STATE_PRESSED);
    }
    lv_obj_set_style_border_width(button, 2, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x3e4144), 0);
    lv_obj_set_style_shadow_width(button, 3, 0);
    lv_obj_set_style_shadow_color(button, lv_color_hex(0x5f6264), 0);
    lv_obj_set_style_shadow_ofs_y(button, 2, 0);
    lv_obj_set_style_shadow_width(button, 0, LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(button, 2, LV_STATE_PRESSED);
    lv_obj_set_style_transition(button, &button_transition, 0);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
}

void ui_create(void) {
    platform_audio_init();
    lv_style_transition_dsc_init(&button_transition, button_transition_properties,
                                 lv_anim_path_ease_out, 100, 0, NULL);
    lv_obj_t *root = lv_screen_active();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x999a9c), 0);
    lv_obj_t *background = lv_image_create(root);
    lv_image_set_src(background, "A:assets/interior-ascensor-2d.png");
    lv_image_set_pivot(background, 0, 0);
    lv_image_set_scale(background, 160);
    lv_obj_set_pos(background, 0, 0);
    status_label = label(root, "Selecciona un piso", 0x35494e, &lv_font_montserrat_16);
    lv_obj_set_pos(status_label, 52, 558);

    lv_obj_t *panel = lv_obj_create(root);
    lv_obj_remove_style_all(panel);
    lv_obj_set_pos(panel, 635, 204);
    lv_obj_set_size(panel, 210, 360);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    /* Match the door's dark outline and flat gray metal frame. */
    box(panel, 41, 5, 150, 355, 0x3e4144, 0);
    box(panel, 43, 7, 146, 353, 0xaaaeb0, 0);
    box(panel, 45, 9, 142, 14, 0xc4c7c9, 0);
    box(panel, 49, 25, 134, 331, 0xb5b8ba, 0);
    /* Indicador dibujado a medida: marco del mismo estilo que la puerta. */
    box(panel, 54, 20, 124, 52, 0x3e4144, 0);
    box(panel, 56, 22, 120, 48, 0xaaaeb0, 0);
    box(panel, 59, 25, 114, 42, 0x111315, 0);
    floor_labels[0] = label(panel, "1", 0xffce65, &lv_font_montserrat_24);
    lv_obj_set_pos(floor_labels[0], 91, 31);
    arrows[0] = label(panel, "-", 0xffce65, &lv_font_montserrat_24);
    lv_obj_set_pos(arrows[0], 126, 31);
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = lv_button_create(panel); floor_buttons[i] = b;
        style_panel_button(b, 0);
        lv_obj_set_pos(b, 80, 95 + i * 63); lv_obj_set_size(b, 65, 55);
        char text[2] = {(char)('1' + i), 0}; lv_obj_t *l = label(b, text, 0x34464c, &lv_font_montserrat_24); lv_obj_center(l);
        lv_obj_add_event_cb(b, select_floor, LV_EVENT_ALL, (void *)(intptr_t)(i + 1));
    }
    lv_obj_t *alarm = lv_button_create(panel);
    style_panel_button(alarm, 1);
    lv_obj_set_pos(alarm, 46, 302); lv_obj_set_size(alarm, 133, 42);
    emergency_label = label(alarm, "EMERGENCIA", 0x34464c, &lv_font_montserrat_14);
    lv_obj_center(emergency_label);
    lv_obj_add_event_cb(alarm, emergency, LV_EVENT_ALL, NULL);


    /* Recuadro de advertencias sobre la puerta. */
    box(root, 414, 159, 193, 42, 0x202224, 1);
    box(root, 416, 161, 189, 38, 0xaaaeb0, 0);
    box(root, 419, 164, 183, 32, 0x0a0b0c, 0);
    door_warning = label(root, "", 0xff3333, &lv_font_montserrat_14);
    lv_obj_set_width(door_warning, 190);
    lv_obj_set_style_text_align(door_warning, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(door_warning, 417, 171);

    lv_obj_t *door_view = lv_obj_create(root);
    lv_obj_remove_style_all(door_view);
    lv_obj_set_pos(door_view, 392, 204);
    lv_obj_set_size(door_view, 240, 360);
    lv_obj_remove_flag(door_view, LV_OBJ_FLAG_SCROLLABLE);
    door_image = door_view;
    box(door_view, 7, 5, 226, 355, 0x3e4144, 0);
    box(door_view, 9, 7, 222, 353, 0xaaaeb0, 0);
    box(door_view, 11, 9, 218, 15, 0xc4c7c9, 0);
    box(door_view, 20, 24, 200, 336, 0x777c80, 0);
    box(door_view, 23, 27, 194, 330, 0x474a4d, 0);
    door_left_leaf = box(door_view, 23, 27, 1, 330, 0xb5b8ba, 0);
    door_right_leaf = box(door_view, 216, 27, 1, 330, 0xb5b8ba, 0);
    box(door_view, 9, 24, 12, 336, 0xc4c7c9, 0);
    box(door_view, 219, 24, 12, 336, 0xaaaeb0, 0);
    box(door_view, 21, 24, 2, 336, 0x64686b, 0);
    box(door_view, 217, 24, 2, 336, 0x64686b, 0);
    door_frame(door_image, 0);
    displays(1, 0);
    lv_timer_create(step, 100, NULL);
}
