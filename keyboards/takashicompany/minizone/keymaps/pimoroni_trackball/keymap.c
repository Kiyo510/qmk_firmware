// Copyright 2022 takashicompany (@takashicompany)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#include "pointing_device.h"

enum custom_keycodes {
    KC_MY_BTN1 = SAFE_RANGE,
    KC_MY_BTN2,
    KC_MY_BTN3,
    KC_MY_SCR,
};


enum click_state {
    NONE = 0,
    WAITING,    // マウスレイヤーが有効になるのを待つ
    CLICKABLE,  // マウスレイヤー有効になりクリック入力が取れる
    CLICKING,   // クリックをしている
    SCROLLING   // スクロール中
};

enum click_state state;     // 現在のクリック入力受付の状態
uint16_t click_timer;       // タイマー。状態に応じて時間で判定する

uint16_t to_clickable_time = 10;   // この秒数(千分の一秒)、WAITING状態ならクリックレイヤーが有効になる
uint16_t to_reset_time = 1000; // この秒数(千分の一秒)、CLICKABLE状態ならクリックレイヤーが無効になる

uint16_t click_layer = 9;   // マウス入力が可能になった際に有効になるレイヤー

int16_t scroll_v_counter;   // 垂直スクロールの入力をカウントする
int16_t scroll_h_counter;   // 水平スクロールの入力をカウントする

int16_t scroll_v_threshold = 15;    // この閾値を超える度に垂直スクロールが実行される
int16_t scroll_h_threshold = 15;    // この閾値を超える度に水平スクロールが実行される

int16_t after_click_lock_movement = 0;      // クリック入力後の移動量を測定する変数

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    LAYOUT(
        LT(6, KC_Q), KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, 
        KC_A, KC_S, LT(5, KC_D), KC_F, KC_G, KC_H, KC_J, LT(5, KC_K), KC_L, KC_ENT, 
        LSFT_T(KC_Z), LGUI_T(KC_X), KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, LCTL_T(KC_DOT), KC_BSPC, 
        KC_LCTL, KC_LGUI, LALT_T(KC_LANG2), LSFT_T(KC_TAB), KC_SPC, LT(1, KC_LANG1), KC_PGUP, KC_NO
    ),

    LAYOUT(
        KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, 
        LCTL_T(KC_EQL), KC_LBRC, KC_SLSH, KC_MINS, KC_RO, KC_SCLN, KC_QUOT, KC_RBRC, KC_NUHS, KC_JYEN, 
        KC_LSFT, KC_LGUI, KC_LALT, KC_LANG2, KC_LSFT, KC_SPC, KC_LANG1, KC_TRNS, KC_TRNS, KC_DEL, 
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    LAYOUT(LT(6, KC_Q), KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, 
        KC_A, KC_S, LT(5, KC_D), KC_F, KC_G, KC_H, KC_J, LT(5, KC_K), KC_L, KC_ENT, 
        LSFT_T(KC_Z), LGUI_T(KC_X), KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, LCTL_T(KC_DOT), KC_BSPC, 
        KC_TRNS, KC_TRNS, LALT_T(KC_LANG2), LSFT_T(KC_TAB), KC_SPC, LT(3, KC_LANG1), KC_TRNS, KC_TRNS
    ),

    LAYOUT(
        KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, 
        KC_CIRC, KC_AT, KC_SLSH, KC_MINS, KC_UNDS, KC_SCLN, KC_COLN, KC_LBRC, KC_RBRC, KC_JYEN, 
        MO(4), KC_LGUI, KC_LALT, KC_LANG2, KC_LSFT, KC_SPC, KC_LANG1, KC_TRNS, KC_TRNS, KC_DEL, 
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    LAYOUT(
        KC_EXLM, KC_DQUO, KC_HASH, KC_DLR, KC_PERC, KC_AMPR, KC_QUOT, KC_LPRN, KC_RPRN, KC_0, 
        KC_TILD, KC_GRV, KC_QUES, KC_EQL, KC_UNDS, KC_PLUS, KC_ASTR, KC_LCBR, KC_RCBR, KC_PIPE, 
        KC_LSFT, KC_LGUI, KC_LALT, KC_LANG2, KC_LSFT, KC_SPC, KC_LANG1, KC_TRNS, KC_TRNS, KC_DEL, 
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    LAYOUT(
        KC_ESC, KC_TAB, KC_NO, KC_NO, KC_NO, KC_MS_BTN1, KC_MS_BTN2, KC_UP, KC_NO, KC_NO, 
        KC_LCTL, KC_TRNS, KC_QUES, KC_EXLM, KC_NO, KC_WH_U, KC_LEFT, KC_DOWN, KC_RGHT, KC_NO, 
        KC_LSFT, KC_LGUI, KC_LALT, KC_LANG2, KC_TRNS, KC_WH_D, KC_LANG1, KC_NO, KC_NO, KC_DEL, 
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    LAYOUT(
        KC_NO, KC_TAB, KC_NO, KC_NO, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, 
        KC_NO, KC_NO, KC_NO, KC_NO, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, 
        KC_LSFT, KC_NO, KC_NO, KC_NO, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO, MO(7), MO(8), 
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    LAYOUT(
        RGB_TOG, RGB_MOD, RGB_HUI, RGB_SAI, RGB_VAI, KC_NO, KC_NO, KC_NO, DF(0), DF(2), 
        RGB_M_P, RGB_M_B, RGB_M_R, RGB_M_SW, RGB_M_SN, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, 
        RGB_M_K, RGB_M_X, RGB_M_G, KC_NO, KC_NO, RESET, KC_NO, KC_NO, KC_NO, KC_NO, 
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

     LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_MY_BTN1, KC_MY_SCR, KC_MY_BTN2, KC_MY_BTN3, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_MY_SCR, KC_MY_BTN1, KC_TRNS, KC_MY_SCR, KC_MY_BTN2, KC_MY_BTN3, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    )
};

// クリック用のレイヤーを有効にする
void enable_click_layer(void) {
    layer_on(click_layer);
    click_timer = timer_read();
    state = CLICKABLE;
}

// クリック用のレイヤーを無効にする
void disable_click_layer(void) {
    state = NONE;
    layer_off(click_layer);
    scroll_v_counter = 0;
    scroll_h_counter = 0;
}

// 自前の絶対数を返す関数
int16_t my_abs(int16_t num) {
    if (num < 0) {
        num = -num;
    }

    return num;
}

// 現在クリックが可能な状態か
bool is_clickable_mode(void) {
    return state == CLICKABLE || state == CLICKING || state == SCROLLING;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    
    switch (keycode) {
        case KC_MY_BTN1:
        case KC_MY_BTN2:
        case KC_MY_BTN3:
        {
            report_mouse_t currentReport = pointing_device_get_report();

            // どこのビットを対象にするか
            uint8_t btn = 1 << (keycode - KC_MY_BTN1);
            
            if (record->event.pressed) {
                // ビットORは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットのどちらかが「1」の場合に「1」にします。
                currentReport.buttons |= btn;
                state = CLICKING;
                after_click_lock_movement = 30;
            } else {
                // ビットANDは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットが共に「1」の場合だけ「1」にします。
                currentReport.buttons &= ~btn;
                enable_click_layer();
            }

            pointing_device_set_report(currentReport);
            pointing_device_send();
            return false;
        }

        case KC_MY_SCR:
            if (record->event.pressed) {
                state = SCROLLING;
            } else {
                // disable_click_layer(); // スクロールキーを離したらクリックレイヤーを無効にする
                enable_click_layer();   // スクロールキーを離した時に再度クリックレイヤーを有効にする
            }
         return false;

         default:
            if  (record->event.pressed) {
                disable_click_layer();
            }
        
    }
   
    return true;
}

int history_length = 10;

int16_t history_x[10] = {};
int16_t history_y[10] = {};
int16_t history_t[10] = {};


report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {

    int16_t current_x = state == SCROLLING ? mouse_report.x : history_x[0];
    int16_t current_y = state == SCROLLING ? mouse_report.y : history_y[0];
    int16_t current_h = 0;
    int16_t current_v = 0;

    int start = 1;
    int read_count = 10;

    if (current_x != 0 || current_y != 0)
    {
        for (int i = start; i < start + read_count && i < history_length; i++)
        {
            current_x += history_x[i];
            current_y += history_y[i];
            start = i;
        }
    }

    for (int i = start; i < history_length; i++)
    {
        history_x[i - start] = history_x[i];
        history_y[i - start] = history_y[i];
    }

    history_x[history_length - 1] = mouse_report.x;
    history_y[history_length - 1] = mouse_report.y;

    if (current_x != 0 || current_y != 0) {
        
        switch (state) {
            case CLICKABLE:
                click_timer = timer_read();
                break;

            case CLICKING:
                after_click_lock_movement -= my_abs(current_x) + my_abs(current_y);

                if (after_click_lock_movement > 0) {
                    current_x = 0;
                    current_y = 0;
                }

                break;

            case SCROLLING:
            {
                int8_t rep_v = 0;
                int8_t rep_h = 0;

                // 垂直スクロールの方の感度を高める
                if (my_abs(current_y) * 2 > my_abs(current_x)) {

                    scroll_v_counter += current_y;
                    while (my_abs(scroll_v_counter) > scroll_v_threshold) {
                        if (scroll_v_counter < 0) {
                            scroll_v_counter += scroll_v_threshold;
                            rep_v += scroll_v_threshold;
                        } else {
                            scroll_v_counter -= scroll_v_threshold;
                            rep_v -= scroll_v_threshold;
                        }
                        
                    }
                } else {

                    scroll_h_counter += current_x;

                    while (my_abs(scroll_h_counter) > scroll_h_threshold) {
                        if (scroll_h_counter < 0) {
                            scroll_h_counter += scroll_h_threshold;
                            rep_h += scroll_h_threshold;
                        } else {
                            scroll_h_counter -= scroll_h_threshold;
                            rep_h -= scroll_h_threshold;
                        }
                    }
                }

                current_h = rep_h / scroll_h_threshold;
                current_v = -rep_v / scroll_v_threshold;
                current_x = 0;
                current_y = 0;
            }
                break;

            case WAITING:
                if (timer_elapsed(click_timer) > to_clickable_time) {
                    enable_click_layer();
                }
                break;

            default:
                click_timer = timer_read();
                state = WAITING;
        }
    }
    else
    {
        switch (state) {
            case CLICKING:
            case SCROLLING:

                break;

            case CLICKABLE:
                if (timer_elapsed(click_timer) > to_reset_time) {
                    disable_click_layer();
                }
                break;

             case WAITING:
                if (timer_elapsed(click_timer) > 50) {
                    state = NONE;
                }
                break;

            default:
                state = NONE;
        }
    }

    mouse_report.x = current_x * 2;
    mouse_report.y = current_y * 2;
    mouse_report.h = current_h * 2;
    mouse_report.v = current_v * 2;

    return mouse_report;

}