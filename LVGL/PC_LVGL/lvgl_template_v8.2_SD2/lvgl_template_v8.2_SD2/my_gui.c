#include "lvgl/lvgl.h"
#include "my_gui.h"
#include <math.h>
#include <stdlib.h>

#define EYE_SIZE        350
#define PUPIL_SIZE      200
#define HIGHLIGHT_SIZE  50
#define EYE_GAP         20
#define MAX_PUPIL_MOVE  35

#define HIGHLIGHT_X     25
#define HIGHLIGHT_Y     8

#define BLINK_DURATION  120
#define BLINK_MIN_INTERVAL 2500
#define BLINK_MAX_INTERVAL 6000

#define SACCADE_MIN_INTERVAL 1500
#define SACCADE_MAX_INTERVAL 4000
#define SACCADE_DURATION     80

static lv_coord_t left_eye_cx, right_eye_cx, eye_cy;

lv_coord_t target_x = 160;
lv_coord_t target_y = 120;

static float current_pupil_x = 0;
static float current_pupil_y = 0;

typedef struct {
    lv_obj_t *container;
    lv_obj_t *pupil;
    lv_obj_t *highlight;
    lv_coord_t eye_cx;
    lv_coord_t eye_cy;
    lv_coord_t original_height;
} eye_t;

static eye_t eye_left, eye_right;

static void update_eye(eye_t *eye)
{
    float dx = target_x - eye->eye_cx;
    float dy = target_y - eye->eye_cy;
    float dist = sqrt(dx*dx + dy*dy);
    float angle = atan2(dy, dx);
    
    float base_move = (dist > MAX_PUPIL_MOVE) ? MAX_PUPIL_MOVE : dist * 0.35f;
    
    float total_x = cos(angle) * base_move + current_pupil_x;
    float total_y = sin(angle) * base_move + current_pupil_y;
    
    float total_dist = sqrt(total_x*total_x + total_y*total_y);
    if (total_dist > MAX_PUPIL_MOVE) {
        total_x = total_x / total_dist * MAX_PUPIL_MOVE;
        total_y = total_y / total_dist * MAX_PUPIL_MOVE;
    }
    
    lv_coord_t pupil_cx = EYE_SIZE/2 + (lv_coord_t)total_x;
    lv_coord_t pupil_cy = EYE_SIZE/2 + (lv_coord_t)total_y;
    
    lv_obj_set_pos(eye->pupil, pupil_cx - PUPIL_SIZE/2, pupil_cy - PUPIL_SIZE/2);
}

static void saccade_anim_cb(void *var, int32_t v)
{
    float *target = (float *)var;
    *target = (float)v / 10.0f;
    update_eye(&eye_left);
    update_eye(&eye_right);
}

static void start_saccade(void)
{
    float new_offset_x = (rand() % 40 - 20);
    float new_offset_y = (rand() % 30 - 15);
    
    lv_anim_t a;
    
    lv_anim_init(&a);
    lv_anim_set_var(&a, &current_pupil_x);
    lv_anim_set_exec_cb(&a, saccade_anim_cb);
    lv_anim_set_values(&a, (int32_t)(current_pupil_x * 10), (int32_t)(new_offset_x * 10));
    lv_anim_set_time(&a, SACCADE_DURATION);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    
    lv_anim_init(&a);
    lv_anim_set_var(&a, &current_pupil_y);
    lv_anim_set_exec_cb(&a, saccade_anim_cb);
    lv_anim_set_values(&a, (int32_t)(current_pupil_y * 10), (int32_t)(new_offset_y * 10));
    lv_anim_set_time(&a, SACCADE_DURATION);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void saccade_timer_cb(lv_timer_t *t)
{
    start_saccade();
    
    uint32_t next_interval = SACCADE_MIN_INTERVAL + 
        (rand() % (SACCADE_MAX_INTERVAL - SACCADE_MIN_INTERVAL));
    lv_timer_set_period(t, next_interval);
}

static void blink_anim_cb(void *var, int32_t v)
{
    eye_t *eye = (eye_t *)var;
    lv_coord_t new_height = (lv_coord_t)((float)eye->original_height * (float)v / 100.0f);
    lv_obj_set_height(eye->container, new_height);
    
    lv_coord_t new_y = eye->eye_cy - new_height / 2;
    lv_obj_set_y(eye->container, new_y);
    
    float scale = (float)v / 100.0f;
    lv_coord_t pupil_height = (lv_coord_t)((float)PUPIL_SIZE * scale);
    lv_obj_set_height(eye->pupil, pupil_height);
}

static void start_blink(void)
{
    lv_anim_t a;
    
    lv_anim_init(&a);
    lv_anim_set_var(&a, &eye_left);
    lv_anim_set_exec_cb(&a, blink_anim_cb);
    lv_anim_set_values(&a, 100, 5);
    lv_anim_set_time(&a, BLINK_DURATION);
    lv_anim_set_playback_time(&a, BLINK_DURATION);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    
    lv_anim_init(&a);
    lv_anim_set_var(&a, &eye_right);
    lv_anim_set_exec_cb(&a, blink_anim_cb);
    lv_anim_set_values(&a, 100, 5);
    lv_anim_set_time(&a, BLINK_DURATION);
    lv_anim_set_playback_time(&a, BLINK_DURATION);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void blink_timer_cb(lv_timer_t *t)
{
    start_blink();
    
    uint32_t next_interval = BLINK_MIN_INTERVAL + 
        (rand() % (BLINK_MAX_INTERVAL - BLINK_MIN_INTERVAL));
    lv_timer_set_period(t, next_interval);
}

static void eye_timer(lv_timer_t *t)
{
    update_eye(&eye_left);
    update_eye(&eye_right);
}

void my_GUI(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(15, 15, 25), 0);
    
    left_eye_cx = 400 - EYE_SIZE/2 - EYE_GAP/2;
    right_eye_cx = 400 + EYE_SIZE/2 + EYE_GAP/2;
    eye_cy = 240;
    
    eye_left.eye_cx = left_eye_cx;
    eye_left.eye_cy = eye_cy;
    eye_left.original_height = EYE_SIZE;
    eye_right.eye_cx = right_eye_cx;
    eye_right.eye_cy = eye_cy;
    eye_right.original_height = EYE_SIZE;
    
    lv_obj_t *eye_l = lv_obj_create(lv_scr_act());
    lv_obj_set_size(eye_l, EYE_SIZE, EYE_SIZE);
    lv_obj_set_pos(eye_l, left_eye_cx - EYE_SIZE/2, eye_cy - EYE_SIZE/2);
    lv_obj_set_style_bg_color(eye_l, lv_color_white(), 0);
    lv_obj_set_style_radius(eye_l, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(eye_l, 0, 0);
    lv_obj_set_style_clip_corner(eye_l, true, 0);
    lv_obj_clear_flag(eye_l, LV_OBJ_FLAG_SCROLLABLE);
    eye_left.container = eye_l;
    
    eye_left.pupil = lv_obj_create(eye_l);
    lv_obj_set_size(eye_left.pupil, PUPIL_SIZE, PUPIL_SIZE);
    lv_obj_set_style_bg_color(eye_left.pupil, lv_color_black(), 0);
    lv_obj_set_style_radius(eye_left.pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(eye_left.pupil, 0, 0);
    lv_obj_clear_flag(eye_left.pupil, LV_OBJ_FLAG_SCROLLABLE);
    
    eye_left.highlight = lv_obj_create(eye_left.pupil);
    lv_obj_set_size(eye_left.highlight, HIGHLIGHT_SIZE, HIGHLIGHT_SIZE);
    lv_obj_set_style_bg_color(eye_left.highlight, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(eye_left.highlight, LV_OPA_70, 0);
    lv_obj_set_style_radius(eye_left.highlight, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(eye_left.highlight, 0, 0);
    lv_obj_set_pos(eye_left.highlight, HIGHLIGHT_X, HIGHLIGHT_Y);
    
    lv_obj_t *eye_r = lv_obj_create(lv_scr_act());
    lv_obj_set_size(eye_r, EYE_SIZE, EYE_SIZE);
    lv_obj_set_pos(eye_r, right_eye_cx - EYE_SIZE/2, eye_cy - EYE_SIZE/2);
    lv_obj_set_style_bg_color(eye_r, lv_color_white(), 0);
    lv_obj_set_style_radius(eye_r, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(eye_r, 0, 0);
    lv_obj_set_style_clip_corner(eye_r, true, 0);
    lv_obj_clear_flag(eye_r, LV_OBJ_FLAG_SCROLLABLE);
    eye_right.container = eye_r;
    
    eye_right.pupil = lv_obj_create(eye_r);
    lv_obj_set_size(eye_right.pupil, PUPIL_SIZE, PUPIL_SIZE);
    lv_obj_set_style_bg_color(eye_right.pupil, lv_color_black(), 0);
    lv_obj_set_style_radius(eye_right.pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(eye_right.pupil, 0, 0);
    lv_obj_clear_flag(eye_right.pupil, LV_OBJ_FLAG_SCROLLABLE);

    eye_right.highlight = lv_obj_create(eye_right.pupil);
    lv_obj_set_size(eye_right.highlight, HIGHLIGHT_SIZE, HIGHLIGHT_SIZE);
    lv_obj_set_style_bg_color(eye_right.highlight, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(eye_right.highlight, LV_OPA_70, 0);
    lv_obj_set_style_radius(eye_right.highlight, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(eye_right.highlight, 0, 0);
    lv_obj_set_pos(eye_right.highlight, HIGHLIGHT_X, HIGHLIGHT_Y);
    
    update_eye(&eye_left);
    update_eye(&eye_right);
    
    lv_timer_create(eye_timer, 16, NULL);
    
    lv_timer_create(blink_timer_cb, BLINK_MIN_INTERVAL, NULL);
    
    lv_timer_create(saccade_timer_cb, SACCADE_MIN_INTERVAL, NULL);
}