#include "scr_archery_game.h"
#include "screens.h"
#include "nRF24.h" 
#include "button.h" 
#include <string.h> 
#include <stdlib.h> 

// ============================================================
// CẤU HÌNH VÀ KHAI BÁO
// ============================================================
extern button_t btn_up;
extern button_t btn_down; 
extern void rf_init_hardware_kit(void);
extern uint32_t ar_game_score; 
extern uint8_t current_rf_channel; 

extern const unsigned char bitmap_cloud[];
extern const unsigned char bitmap_gift[];
extern const unsigned char bitmap_bird[]; 
extern const unsigned char bitmap_dino_duck[]; 

#define GROUND_Y_REAL   50      
#define DINO_X          10      
#define DINO_W          16      
#define DINO_H          16      

#define GRAVITY_SCALED  15       
#define JUMP_SCALED     -90     
#define GROUND_Y_SCALED (GROUND_Y_REAL * 10)

#define CACTUS_SPEED_SCALED  35   
#define SPEED_STEP           5    
#define SCORE_STEP           15   
#define MAX_BASE_SPEED       60   
#define ATTACK_BONUS_SPEED   20   
#define BG_SPEED_SCALED      10   

#define MIN_CACTUS_GAP  100  
#define MAX_CACTUS_GAP  250  

#define TYPE_CACTUS    0
#define TYPE_GIFT      1
#define TYPE_BIRD      2 

uint8_t RF_ADDR[] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };

// Các Lệnh (Command)
#define CMD_START    1
#define CMD_I_DIED   2
#define CMD_ATTACK   3  
#define CMD_READY    4 
#define CMD_ACCEPT   5 
#define CMD_REJECT   6 

enum { MP_WAITING = 0, MP_PLAYING, MP_LOSE, MP_WIN };
static uint8_t mp_state = MP_WAITING;
static uint8_t frame_skip = 0;
uint8_t ar_game_state = 0; 
ar_game_setting_t settingsetup; 

// ============================================================
// HỆ THỐNG MATCHMAKING 
// ============================================================
static char my_name[4] = "P00";       
static char opponent_name[4] = "   "; 
static uint8_t lobby_state = 0;       

typedef struct { int16_t y; int16_t v_y; bool is_jumping; bool is_ducking; } dino_t;
static dino_t dino;

typedef struct { int32_t x; int16_t y; uint8_t w; uint8_t h; uint8_t type; bool active; } game_obj_t;
static game_obj_t objects[4]; 

typedef struct { int32_t x; int16_t y; uint8_t w; uint8_t h; } bg_obj_t;
static bg_obj_t bgs[1]; 

static uint16_t attack_timer = 0; 

// ============================================================
// HÀM HỖ TRỢ RF (Gói tin 5 Bytes an toàn tuyệt đối)
// ============================================================
void rf_mode_rx() {
    nRF24_RXMode(
        nRF24_RX_PIPE0, nRF24_ENAA_P0, current_rf_channel, 
        nRF24_DataRate_1Mbps, nRF24_CRC_2byte, 
        RF_ADDR, 5, 5, nRF24_TXPower_0dBm 
    );
}

void rf_send_cmd(uint8_t cmd) {
    // FIX CRASH: Dùng static để mảng không bị xóa khi hàm kết thúc, giúp SPI/DMA đọc an toàn!
    static uint8_t tx_buf[5]; 
    tx_buf[0] = cmd;
    tx_buf[1] = my_name[0];
    tx_buf[2] = my_name[1];
    tx_buf[3] = my_name[2];
    tx_buf[4] = '\0';

    nRF24_TXMode(
        5, 15, current_rf_channel, nRF24_DataRate_1Mbps, 
        nRF24_TXPower_0dBm, nRF24_CRC_2byte, nRF24_PWR_Up, 
        RF_ADDR, 5
    );
    nRF24_TXPacket(tx_buf, 5); 
    rf_mode_rx();
}

// ============================================================
// LOGIC GAME CHÍNH
// ============================================================
void dino_reset() {
    dino.y = GROUND_Y_SCALED - (DINO_H * 10);
    dino.v_y = 0;
    dino.is_jumping = false;
    dino.is_ducking = false;
    
    objects[0] = { (128 + 10) * 10, GROUND_Y_REAL - 16, 16, 16, TYPE_CACTUS, true };
    objects[1] = { objects[0].x + 1500, GROUND_Y_REAL - 16, 16, 16, TYPE_CACTUS, true };
    objects[2] = { objects[1].x + 1200, GROUND_Y_REAL - 28, 8, 8, TYPE_GIFT, true }; 
    objects[3] = { objects[2].x + 1000, GROUND_Y_REAL - 20, 16, 8, TYPE_BIRD, true }; 
    
    bgs[0] = { 120 * 10, 10, 16, 16 }; 
    
    ar_game_score = 0;
    frame_skip = 0; 
    attack_timer = 0;
}

void dino_update() {
    dino.is_ducking = (btn_down.state == BUTTON_SW_STATE_PRESSED); 

    if (dino.is_jumping) {
        dino.y += dino.v_y; 
        dino.v_y += GRAVITY_SCALED;
        if (dino.y >= (GROUND_Y_SCALED - (DINO_H * 10))) {
            dino.y = GROUND_Y_SCALED - (DINO_H * 10);
            dino.is_jumping = false; 
            dino.v_y = 0;
        }
    }
    
    bgs[0].x -= BG_SPEED_SCALED;
    if (bgs[0].x / 10 < -16) bgs[0].x = (128 + (rand() % 50)) * 10;
    
    int32_t start_speed_bonus = (settingsetup.num_arrow - 1) * 10; 
    int32_t base_speed = CACTUS_SPEED_SCALED + start_speed_bonus + (ar_game_score / SCORE_STEP) * SPEED_STEP;
    if (base_speed > MAX_BASE_SPEED + start_speed_bonus) base_speed = MAX_BASE_SPEED + start_speed_bonus; 
    
    int32_t current_speed = base_speed;
    if (attack_timer > 0) {
        attack_timer--;
        current_speed = base_speed + ATTACK_BONUS_SPEED; 
    }

    for (int i = 0; i < 4; i++) {
        if (objects[i].active) {
            objects[i].x -= current_speed;
            int cx = objects[i].x / 10;
            
            if (cx < -objects[i].w) {
                if (objects[i].type == TYPE_CACTUS || objects[i].type == TYPE_BIRD) ar_game_score++;
                
                int32_t max_x = 0;
                for (int j = 0; j < 4; j++) {
                    if (i != j && objects[j].active && objects[j].x > max_x) max_x = objects[j].x;
                }
                
                int other_x = max_x / 10;
                int gap_reduce = (settingsetup.meteoroid_speed - 1) * 15; 
                int current_min_gap = MIN_CACTUS_GAP - gap_reduce;
                int current_max_gap = MAX_CACTUS_GAP - (gap_reduce * 2);
                if (current_min_gap < 40) current_min_gap = 40; 
                if (current_max_gap < current_min_gap + 20) current_max_gap = current_min_gap + 20;

                int new_x = other_x + current_min_gap + (rand() % (current_max_gap - current_min_gap));
                if (new_x < 128) new_x = 128 + current_min_gap + (rand() % 50);
                
                objects[i].x = new_x * 10;
                
                uint8_t rand_type = rand() % 100;
                if (rand_type < 20) { objects[i].type = TYPE_GIFT; objects[i].w = 8; objects[i].h = 8; objects[i].y = GROUND_Y_REAL - 26 - (rand() % 10); } 
                else if (rand_type < 45) { objects[i].type = TYPE_BIRD; objects[i].w = 16; objects[i].h = 8; objects[i].y = (rand() % 100 < 50) ? GROUND_Y_REAL - 14 : GROUND_Y_REAL - 20; } 
                else { objects[i].type = TYPE_CACTUS; objects[i].w = 16; objects[i].h = 16; objects[i].y = GROUND_Y_REAL - 16; }
            }
            
            int16_t dy = dino.y / 10;
            int16_t dino_hit_y = dy;
            int16_t dino_hit_h = DINO_H;
            if (dino.is_ducking) { dino_hit_y = dy + 6; dino_hit_h = 10; }
            
            bool hit_x = (DINO_X + DINO_W - 4 > cx) && (DINO_X + 2 < cx + objects[i].w);
            bool hit_y = (dino_hit_y + dino_hit_h > objects[i].y + 2) && (dino_hit_y + 2 < objects[i].y + objects[i].h);
            
            if (hit_x && hit_y) {
                if (objects[i].type == TYPE_CACTUS || objects[i].type == TYPE_BIRD) {
                    mp_state = MP_LOSE;
                    rf_send_cmd(CMD_I_DIED); 
                    BUZZER_PlayTones(tones_3beep);
                    SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
                } 
                else if (objects[i].type == TYPE_GIFT) {
                    objects[i].active = false; 
                    ar_game_score += 5;        
                    rf_send_cmd(CMD_ATTACK);   
                    BUZZER_PlayTones(tones_cc); 
                }
            }
        } else {
            objects[i].x -= current_speed;
            if (objects[i].x / 10 < -20) objects[i].active = true;
        }
    }
}

// ============================================================
// HIỂN THỊ (VIEW RENDERER)
// ============================================================
void view_scr_dino_game() {
    view_render.clear();
    
    if (mp_state == MP_PLAYING) { 
        view_render.drawFastHLine(0, GROUND_Y_REAL, 128, WHITE);
        view_render.setCursor(90, 2); 
        view_render.print(ar_game_score);
        
        view_render.drawBitmap(bgs[0].x / 10, bgs[0].y, bitmap_cloud, bgs[0].w, bgs[0].h, WHITE);
        
        for (int i = 0; i < 4; i++) {
            if (objects[i].active) {
                const unsigned char* fg_bmp;
                if (objects[i].type == TYPE_CACTUS) fg_bmp = bitmap_cactus;
                else if (objects[i].type == TYPE_BIRD) fg_bmp = bitmap_bird; 
                else fg_bmp = bitmap_gift;
                view_render.drawBitmap(objects[i].x / 10, objects[i].y, fg_bmp, objects[i].w, objects[i].h, WHITE);
            }
        }
        
        if (dino.is_ducking) view_render.drawBitmap(DINO_X, dino.y / 10, bitmap_dino_duck, DINO_W, DINO_H, WHITE);
        else view_render.drawBitmap(DINO_X, dino.y / 10, bitmap_dino, DINO_W, DINO_H, WHITE);
        
        if (attack_timer > 0 && (attack_timer % 10 < 5)) {
            view_render.setTextSize(1); 
            view_render.setCursor(30, 10); 
            view_render.print("SPEED UP!");
        }
    } 
    else { 
        view_render.setTextSize(1); 
        
        if (lobby_state == 0) { 
            view_render.setCursor(15, 15);
            view_render.print("MY NAME: ["); view_render.print(my_name); view_render.print("]");
            view_render.setCursor(15, 35);
            view_render.print("BTN DOWN TO READY");
        } 
        else if (lobby_state == 1) { 
            view_render.setCursor(15, 15);
            view_render.print("WAITING REPLY...");
            view_render.setCursor(15, 35);
            view_render.print("BTN DOWN TO SOLO");
        } 
        else if (lobby_state == 2) { 
            view_render.setCursor(10, 10);
            view_render.print("["); view_render.print(opponent_name); view_render.print("] INVITES!");
            view_render.setCursor(10, 30);
            view_render.print("UP  : ACCEPT");
            view_render.setCursor(10, 45);
            view_render.print("DOWN: REJECT");
        }
    }
}

static void view_wrapper() { view_scr_dino_game(); }

view_dynamic_t dyn_view_item_archery_game = { { ITEM_TYPE_DYNAMIC }, view_wrapper };
view_screen_t scr_archery_game = { &dyn_view_item_archery_game, ITEM_NULL, ITEM_NULL, .focus_item = 0 };

// ============================================================
// XỬ LÝ SỰ KIỆN (EVENT HANDLER)
// ============================================================
void scr_archery_game_handle(ak_msg_t* msg) {
    switch (msg->sig) {
        
        case SCREEN_ENTRY: {
            eeprom_read(EEPROM_SETTING_START_ADDR, (uint8_t*)&settingsetup, sizeof(settingsetup));
            
            static bool name_initialized = false;
            if (!name_initialized) {
                // FIX CRASH 2: Lấy Hạt giống Random từ SysTick (An toàn trên mọi dòng Chip Cortex-M)
                uint32_t safe_random_seed = *(volatile uint32_t*)0xE000E018; 
                srand(safe_random_seed); 

                my_name[0] = 'P';
                my_name[1] = '0' + (rand() % 10); 
                my_name[2] = '0' + (rand() % 10);
                my_name[3] = '\0';
                name_initialized = true;
            }

            dino_reset();
            mp_state = MP_WAITING;
            lobby_state = 0; 
            opponent_name[0] = '\0';
            ar_game_state = 1; 
            
            rf_init_hardware_kit(); 
            rf_mode_rx();
            
            timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT);
            break;
        }

        case AR_GAME_TIME_TICK: {
            if (mp_state == MP_PLAYING && !dino.is_jumping) {
                if (btn_up.state == BUTTON_SW_STATE_PRESSED) {
                    dino.v_y = JUMP_SCALED; 
                    dino.is_jumping = true; 
                }
            }

            // Dùng static để nhận data an toàn
            static uint8_t rx_data[5];
            if (nRF24_RXPacket(rx_data, 5) == nRF24_RX_PCKT_PIPE0) {
                uint8_t cmd = rx_data[0];
                char rx_name[4];
                rx_name[0] = rx_data[1]; rx_name[1] = rx_data[2]; rx_name[2] = rx_data[3]; rx_name[3] = '\0';

                if (mp_state == MP_WAITING) {
                    if (cmd == CMD_READY) {
                        if (lobby_state == 0) { 
                            strcpy(opponent_name, rx_name);
                            lobby_state = 2; 
                            BUZZER_PlayTones(tones_cc);
                        } else if (lobby_state == 1) { 
                            strcpy(opponent_name, rx_name);
                            rf_send_cmd(CMD_ACCEPT); 
                            dino_reset();
                            mp_state = MP_PLAYING;
                            BUZZER_PlayTones(tones_startup);
                        }
                    }
                    else if (cmd == CMD_ACCEPT && lobby_state == 1) {
                        strcpy(opponent_name, rx_name); 
                        dino_reset();
                        mp_state = MP_PLAYING;
                        BUZZER_PlayTones(tones_startup);
                    }
                    else if (cmd == CMD_REJECT && lobby_state == 1) {
                        lobby_state = 0;
                        BUZZER_PlayTones(tones_3beep);
                    }
                }
                else if (mp_state == MP_PLAYING) {
                    // CƠ CHẾ CÁCH LY (SESSION ISOLATION): Chỉ nghe người đã khóa mục tiêu!
                    if (strcmp(rx_name, opponent_name) == 0 || opponent_name[0] == '\0') {
                        if (cmd == CMD_I_DIED) {
                            mp_state = MP_WIN;
                            BUZZER_PlayTones(tones_cc);
                            SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
                        }
                        if (cmd == CMD_ATTACK) {
                            attack_timer = 300; 
                            BUZZER_PlayTones(tones_startup); 
                        }
                    }
                }
            }
            
            if (mp_state == MP_PLAYING) dino_update();

            frame_skip++;
            if (frame_skip >= 3) { view_render.update(); frame_skip = 0; }
            
            timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT);
            break;
        }

        case AC_DISPLAY_BUTTON_UP_RELEASED: {
            if (mp_state == MP_WAITING && lobby_state == 2) {
                rf_send_cmd(CMD_ACCEPT);
                dino_reset();
                mp_state = MP_PLAYING;
                BUZZER_PlayTones(tones_startup);
            }
            break;
        }

        case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
            if (mp_state == MP_WAITING) {
                if (lobby_state == 0) {
                    lobby_state = 1;
                    rf_send_cmd(CMD_READY); 
                    BUZZER_PlayTones(tones_cc);
                } 
                else if (lobby_state == 1) {
                    lobby_state = 3; 
                    opponent_name[0] = '\0'; 
                    rf_send_cmd(CMD_START);  
                    
                    dino_reset();
                    mp_state = MP_PLAYING;
                    BUZZER_PlayTones(tones_startup);
                }
                else if (lobby_state == 2) {
                    rf_send_cmd(CMD_REJECT);
                    lobby_state = 0; 
                    BUZZER_PlayTones(tones_cc);
                }
            }
            break;
        }

        case AC_DISPLAY_BUTTON_MODE_RELEASED: {
            ar_game_state = 0; 
            timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK);
            SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
            break;
        }

        default: break;
    }
}