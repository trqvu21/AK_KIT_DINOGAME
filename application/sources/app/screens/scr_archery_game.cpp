#include "scr_archery_game.h"
#include "screens.h"
#include "nRF24.h" 
#include "button.h" 

// ============================================================
// CẤU HÌNH VÀ KHAI BÁO
// ============================================================
#define MASTER true

extern button_t btn_up;
extern button_t btn_down; 
extern void rf_init_hardware_kit(void);
extern uint32_t ar_game_score; 

// Khai báo các hình ảnh đồ họa liên kết ngoài
extern const unsigned char bitmap_cloud[];
extern const unsigned char bitmap_gift[];
extern const unsigned char bitmap_bird[]; 
extern const unsigned char bitmap_dino_duck[]; 

// CẤU HÌNH GAME VẬT LÝ
#define GROUND_Y_REAL   50      
#define DINO_X          10      
#define DINO_W          16      
#define DINO_H          16      

#define GRAVITY_SCALED  15       
#define JUMP_SCALED     -90     
#define GROUND_Y_SCALED (GROUND_Y_REAL * 10)

// TỐC ĐỘ VÀ TĂNG ĐỘ KHÓ
#define CACTUS_SPEED_SCALED  35   // Tốc độ khởi điểm
#define SPEED_STEP           5    // Mỗi lần tăng tốc sẽ cộng thêm 5
#define SCORE_STEP           15   // Cứ mỗi 15 điểm thì tăng tốc 1 lần
#define MAX_BASE_SPEED       60   // Tốc độ cơ bản tối đa để tránh lỗi xuyên thấu
#define ATTACK_BONUS_SPEED   20   // Tốc độ phạt (Cộng thêm vào tốc độ hiện tại)
#define BG_SPEED_SCALED      10   // Tốc độ mây trôi

#define MIN_CACTUS_GAP  100  
#define MAX_CACTUS_GAP  250  

// LOẠI ĐỐI TƯỢNG
#define TYPE_CACTUS    0
#define TYPE_GIFT      1
#define TYPE_BIRD      2 

// CẤU HÌNH RF GIAO THỨC
uint8_t RF_ADDR[] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };
#define RF_CHANNEL   40   

#define CMD_START    1
#define CMD_I_DIED   2
#define CMD_ATTACK   3  

// BIẾN TOÀN CỤC VÀ TRẠNG THÁI
enum { MP_WAITING = 0, MP_PLAYING, MP_LOSE, MP_WIN };
static uint8_t mp_state = MP_WAITING;
static uint8_t frame_skip = 0;
uint8_t ar_game_state = 0; 
ar_game_setting_t settingsetup; 

// QUẢN LÝ THỰC THỂ
typedef struct { int16_t y; int16_t v_y; bool is_jumping; bool is_ducking; } dino_t;
static dino_t dino;

typedef struct { int32_t x; int16_t y; uint8_t w; uint8_t h; uint8_t type; bool active; } game_obj_t;
static game_obj_t objects[4]; 

typedef struct { int32_t x; int16_t y; uint8_t w; uint8_t h; } bg_obj_t;
static bg_obj_t bgs[1]; 

static uint16_t attack_timer = 0; 


// ============================================================
// HÀM HỖ TRỢ RF
// ============================================================
void rf_mode_rx() {
    nRF24_RXMode(
        nRF24_RX_PIPE0, nRF24_ENAA_P0, RF_CHANNEL, 
        nRF24_DataRate_1Mbps, nRF24_CRC_2byte, 
        RF_ADDR, 5, 1, nRF24_TXPower_0dBm
    );
}

void rf_send_cmd(uint8_t cmd) {
    uint8_t tx_buf[1] = {cmd};
    nRF24_TXMode(
        5, 15, RF_CHANNEL, nRF24_DataRate_1Mbps, 
        nRF24_TXPower_0dBm, nRF24_CRC_2byte, nRF24_PWR_Up, 
        RF_ADDR, 5
    );
    nRF24_TXPacket(tx_buf, 1);
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
    
    objects[0] = { (128 + 10) * 10, GROUND_Y_REAL - 16, 8, 16, TYPE_CACTUS, true };
    objects[1] = { objects[0].x + 1500, GROUND_Y_REAL - 16, 8, 16, TYPE_CACTUS, true };
    objects[2] = { objects[1].x + 1200, GROUND_Y_REAL - 28, 8, 8, TYPE_GIFT, true }; 
    objects[3] = { objects[2].x + 1000, GROUND_Y_REAL - 20, 16, 8, TYPE_BIRD, true }; 
    
    bgs[0] = { 120 * 10, 10, 16, 8 }; 
    
    ar_game_score = 0;
    frame_skip = 0; 
    attack_timer = 0;
}

void dino_update() {
    dino.is_ducking = (btn_down.state == BUTTON_SW_STATE_PRESSED); 

    // --- 1. Cập nhật vật lý Khủng long ---
    if (dino.is_jumping) {
        dino.y += dino.v_y; 
        dino.v_y += GRAVITY_SCALED;
        
        if (dino.y >= (GROUND_Y_SCALED - (DINO_H * 10))) {
            dino.y = GROUND_Y_SCALED - (DINO_H * 10);
            dino.is_jumping = false; 
            dino.v_y = 0;
        }
    }
    
    // --- 2. Cập nhật cảnh nền (Mây) ---
    bgs[0].x -= BG_SPEED_SCALED;
    if (bgs[0].x / 10 < -16) {
        bgs[0].x = (128 + (rand() % 50)) * 10;
    }
    
    // --- 3. Tính toán Tốc độ & Độ khó ---
    int32_t base_speed = CACTUS_SPEED_SCALED + (ar_game_score / SCORE_STEP) * SPEED_STEP;
    if (base_speed > MAX_BASE_SPEED) {
        base_speed = MAX_BASE_SPEED; // Chốt chặn tối đa
    }
    
    int32_t current_speed = base_speed;
    if (attack_timer > 0) {
        attack_timer--;
        current_speed = base_speed + ATTACK_BONUS_SPEED; // Phạt tốc độ
    }

    // --- 4. Cập nhật Chướng ngại vật và Xét Va chạm ---
    for (int i = 0; i < 4; i++) {
        if (objects[i].active) {
            objects[i].x -= current_speed;
            int cx = objects[i].x / 10;
            
            // a. Tái chế vật thể khi trôi ra khỏi màn hình
            if (cx < -objects[i].w) {
                if (objects[i].type == TYPE_CACTUS || objects[i].type == TYPE_BIRD) {
                    ar_game_score++;
                }
                
                int32_t max_x = 0;
                for (int j = 0; j < 4; j++) {
                    if (i != j && objects[j].active && objects[j].x > max_x) {
                        max_x = objects[j].x;
                    }
                }
                
                int other_x = max_x / 10;
                int new_x = other_x + MIN_CACTUS_GAP + (rand() % (MAX_CACTUS_GAP - MIN_CACTUS_GAP));
                if (new_x < 128) {
                    new_x = 128 + MIN_CACTUS_GAP + (rand() % 50);
                }
                
                objects[i].x = new_x * 10;
                
                uint8_t rand_type = rand() % 100;
                if (rand_type < 20) { 
                    objects[i].type = TYPE_GIFT;
                    objects[i].w = 8; 
                    objects[i].h = 8;
                    objects[i].y = GROUND_Y_REAL - 26 - (rand() % 10); 
                } 
                else if (rand_type < 45) { 
                    objects[i].type = TYPE_BIRD;
                    objects[i].w = 16; 
                    objects[i].h = 8;
                    objects[i].y = (rand() % 100 < 50) ? GROUND_Y_REAL - 14 : GROUND_Y_REAL - 20; 
                } 
                else { 
                    objects[i].type = TYPE_CACTUS;
                    objects[i].w = 8; 
                    objects[i].h = 16;
                    objects[i].y = GROUND_Y_REAL - 16; 
                }
            }
            
            // b. Logic Hitbox & Va chạm
            int16_t dy = dino.y / 10;
            int16_t dino_hit_y = dy;
            int16_t dino_hit_h = DINO_H;
            
            if (dino.is_ducking) {
                dino_hit_y = dy + 6; 
                dino_hit_h = 10;
            }
            
            bool hit_x = (DINO_X + DINO_W - 4 > cx) && (DINO_X + 2 < cx + objects[i].w);
            bool hit_y = (dino_hit_y + dino_hit_h > objects[i].y + 2) && (dino_hit_y + 2 < objects[i].y + objects[i].h);
            
            if (hit_x && hit_y) {
                if (objects[i].type == TYPE_CACTUS || objects[i].type == TYPE_BIRD) {
                    mp_state = MP_LOSE;
                    rf_send_cmd(CMD_I_DIED); 
                    BUZZER_PlayTones(tones_3beep);
                } 
                else if (objects[i].type == TYPE_GIFT) {
                    objects[i].active = false; 
                    ar_game_score += 5;        
                    rf_send_cmd(CMD_ATTACK);   
                    BUZZER_PlayTones(tones_cc); 
                }
            }
        } 
        else {
            // Đẩy vật thể đã bị ăn ra khỏi màn hình
            objects[i].x -= current_speed;
            if (objects[i].x / 10 < -20) {
                objects[i].active = true;
            }
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
        
        if (dino.is_ducking) {
            view_render.drawBitmap(DINO_X, dino.y / 10, bitmap_dino_duck, DINO_W, DINO_H, WHITE);
        } else {
            view_render.drawBitmap(DINO_X, dino.y / 10, bitmap_dino, DINO_W, DINO_H, WHITE);
        }
        
        if (attack_timer > 0 && (attack_timer % 10 < 5)) {
            view_render.setTextSize(1); 
            view_render.setCursor(30, 10); 
            view_render.print("SPEED UP!");
        }
    } 
    else if (mp_state == MP_LOSE) { 
        view_render.setTextSize(2); 
        view_render.setCursor(15, 25); 
        view_render.print("YOU LOSE");
    } 
    else if (mp_state == MP_WIN) { 
        view_render.setTextSize(2); 
        view_render.setCursor(20, 25); 
        view_render.print("YOU WIN!");
    }
    else { 
        view_render.setTextSize(1); 
        view_render.setCursor(10, 25);
        if (MASTER) {
            view_render.print("PRESS DOWN TO START");
        } else {
            view_render.print("WAITING FOR MASTER...");
        }
    }
}

static void view_wrapper() { 
    view_scr_dino_game(); 
}

view_dynamic_t dyn_view_item_archery_game = { 
    { ITEM_TYPE_DYNAMIC }, 
    view_wrapper 
};

view_screen_t scr_archery_game = { 
    &dyn_view_item_archery_game, 
    ITEM_NULL, 
    ITEM_NULL, 
    .focus_item = 0 
};


// ============================================================
// XỬ LÝ SỰ KIỆN (EVENT HANDLER)
// ============================================================
void scr_archery_game_handle(ak_msg_t* msg) {
    switch (msg->sig) {
        
        case SCREEN_ENTRY: {
            dino_reset();
            mp_state = MP_WAITING;
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

            uint8_t rx_data;
            if (nRF24_RXPacket(&rx_data, 1) == nRF24_RX_PCKT_PIPE0) {
                if (rx_data == CMD_START) {
                    dino_reset();
                    mp_state = MP_PLAYING;
                }
                if (rx_data == CMD_I_DIED) {
                    mp_state = MP_WIN;
                    BUZZER_PlayTones(tones_cc);
                }
                if (rx_data == CMD_ATTACK) {
                    attack_timer = 300; 
                    BUZZER_PlayTones(tones_startup); 
                }
            }
            
            if (mp_state == MP_PLAYING) {
                dino_update();
            }

            frame_skip++;
            if (frame_skip >= 3) {
                view_render.update();
                frame_skip = 0;
            }
            
            timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT);
            break;
        }

        case AC_DISPLAY_BUTTON_DOWN_PRESSED: {
            if (MASTER && mp_state == MP_WAITING) {
                rf_send_cmd(CMD_START); 
                dino_reset();
                mp_state = MP_PLAYING;  
            }
            if (mp_state == MP_WIN || mp_state == MP_LOSE) {
                mp_state = MP_WAITING;
                dino_reset();
            }
            break;
        }

        case AC_DISPLAY_BUTTON_MODE_RELEASED: {
            ar_game_state = 0; 
            timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK);
            SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
            break;
        }

        default: {
            break;
        }
    }
}