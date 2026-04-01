#include "scr_archery_game.h"
#include "screens.h"
#include "nRF24.h" 
#include "button.h" 

// ============================================================
// CẤU HÌNH VÀ KHAI BÁO
// ============================================================
// Đã xóa #define MASTER! Cả 2 máy đều bình đẳng (Peer-to-Peer)

extern button_t btn_up;
extern button_t btn_down; 
extern void rf_init_hardware_kit(void);
extern uint32_t ar_game_score; 
extern uint8_t current_rf_channel; // Kênh sóng lấy từ Sảnh chờ (Lobby)

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
#define CACTUS_SPEED_SCALED  35   
#define SPEED_STEP           5    
#define SCORE_STEP           15   
#define MAX_BASE_SPEED       60   
#define ATTACK_BONUS_SPEED   20   
#define BG_SPEED_SCALED      10   

#define MIN_CACTUS_GAP  100  
#define MAX_CACTUS_GAP  250  

// LOẠI ĐỐI TƯỢNG
#define TYPE_CACTUS    0
#define TYPE_GIFT      1
#define TYPE_BIRD      2 

// CẤU HÌNH RF GIAO THỨC
uint8_t RF_ADDR[] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };

#define CMD_START    1
#define CMD_I_DIED   2
#define CMD_ATTACK   3  
#define CMD_READY    4 // <-- Lệnh mới: Báo danh sẵn sàng

// BIẾN TOÀN CỤC VÀ TRẠNG THÁI
enum { MP_WAITING = 0, MP_PLAYING, MP_LOSE, MP_WIN };
static uint8_t mp_state = MP_WAITING;
static uint8_t frame_skip = 0;
uint8_t ar_game_state = 0; 
ar_game_setting_t settingsetup; 

// Biến kiểm soát trạng thái Ngang hàng (P2P)
static bool my_ready = false;
static bool enemy_ready = false;

// QUẢN LÝ THỰC THỂ
typedef struct { int16_t y; int16_t v_y; bool is_jumping; bool is_ducking; } dino_t;
static dino_t dino;

typedef struct { int32_t x; int16_t y; uint8_t w; uint8_t h; uint8_t type; bool active; } game_obj_t;
static game_obj_t objects[4]; 

typedef struct { int32_t x; int16_t y; uint8_t w; uint8_t h; } bg_obj_t;
static bg_obj_t bgs[1]; 

static uint16_t attack_timer = 0; 


// ============================================================
// HÀM HỖ TRỢ RF (Dùng kênh sóng động từ Lobby)
// ============================================================
void rf_mode_rx() {
    nRF24_RXMode(
        nRF24_RX_PIPE0, nRF24_ENAA_P0, current_rf_channel, 
        nRF24_DataRate_1Mbps, nRF24_CRC_2byte, 
        RF_ADDR, 5, 1, nRF24_TXPower_0dBm
    );
}

void rf_send_cmd(uint8_t cmd) {
    uint8_t tx_buf[1] = {cmd};
    nRF24_TXMode(
        5, 15, current_rf_channel, nRF24_DataRate_1Mbps, 
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
    int32_t start_speed_bonus = (settingsetup.num_arrow - 1) * 10; 
    int32_t base_speed = CACTUS_SPEED_SCALED + start_speed_bonus + (ar_game_score / SCORE_STEP) * SPEED_STEP;
    
    if (base_speed > MAX_BASE_SPEED + start_speed_bonus) {
        base_speed = MAX_BASE_SPEED + start_speed_bonus; 
    }
    
    int32_t current_speed = base_speed;
    if (attack_timer > 0) {
        attack_timer--;
        current_speed = base_speed + ATTACK_BONUS_SPEED; 
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
                
                // Mật độ chướng ngại vật phụ thuộc Setting Độ khó
                int gap_reduce = (settingsetup.meteoroid_speed - 1) * 15; 
                int current_min_gap = MIN_CACTUS_GAP - gap_reduce;
                int current_max_gap = MAX_CACTUS_GAP - (gap_reduce * 2);

                if (current_min_gap < 40) current_min_gap = 40; 
                if (current_max_gap < current_min_gap + 20) current_max_gap = current_min_gap + 20;

                int new_x = other_x + current_min_gap + (rand() % (current_max_gap - current_min_gap));
                if (new_x < 128) {
                    new_x = 128 + current_min_gap + (rand() % 50);
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
                    objects[i].w = 16; 
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
                    
                    // Chuyển sang màn hình GAME OVER xịn xò
                    SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
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
    else { 
        // Giao diện Sảnh chờ Đồng bộ (Ngang hàng)
        view_render.setTextSize(1); 
        if (my_ready && !enemy_ready) {
            view_render.setCursor(10, 15);
            view_render.print("WAITING OPPONENT...");
            // Thêm dòng hướng dẫn bấm lần 2 để ép chạy Solo
            view_render.setCursor(10, 35);
            view_render.print("PRESS DOWN TO SOLO"); 
        } else if (!my_ready && enemy_ready) {
            view_render.setCursor(10, 15);
            view_render.print("OPPONENT IS READY!");
            view_render.setCursor(10, 35);
            view_render.print("BTN DOWN TO START");
        } else if (!my_ready && !enemy_ready) {
            view_render.setCursor(10, 25);
            view_render.print("BTN DOWN TO READY");
        } else {
            view_render.setCursor(30, 25);
            view_render.print("STARTING...");
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
            // Đọc cài đặt người chơi đã lưu trước khi reset game
            eeprom_read(EEPROM_SETTING_START_ADDR, (uint8_t*)&settingsetup, sizeof(settingsetup));
            
            dino_reset();
            mp_state = MP_WAITING;
            
            // Khởi tạo trạng thái phòng chờ
            my_ready = false;
            enemy_ready = false;
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
                if (rx_data == CMD_READY) {
                    enemy_ready = true;
                    BUZZER_PlayTones(tones_cc); // Bíp nhẹ phát báo đối thủ đã sẵn sàng
                }
                if (rx_data == CMD_START) {
                    my_ready = true; enemy_ready = true;
                }
                if (rx_data == CMD_I_DIED) {
                    mp_state = MP_WIN;
                    BUZZER_PlayTones(tones_cc);
                    SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
                }
                if (rx_data == CMD_ATTACK) {
                    attack_timer = 300; 
                    BUZZER_PlayTones(tones_startup); 
                }
            }
            
            // LOGIC CHỐT HẠ: Cả 2 cùng Ready thì Xông pha!
            if (mp_state == MP_WAITING && my_ready && enemy_ready) {
                dino_reset();
                mp_state = MP_PLAYING;
                BUZZER_PlayTones(tones_startup);
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

        // ========================================================
        // SỬ DỤNG EVENT_RELEASED CHO THAO TÁC DOUBLE TAP BÁ BẠO!
        // ========================================================
        case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
            if (mp_state == MP_WAITING) {
                if (!my_ready) {
                    // Bấm lần 1: Báo Ready
                    my_ready = true;
                    rf_send_cmd(CMD_READY); 
                    BUZZER_PlayTones(tones_cc);
                } 
                else if (!enemy_ready) {
                    // Bấm lần 2: Đợi lâu quá, ép chạy Solo luôn!
                    enemy_ready = true;
                    rf_send_cmd(CMD_START); // Kéo luôn đối thủ vào game nếu họ vừa mới chui vào
                    
                    // Reset ngay và luôn không cần đợi Tick
                    dino_reset();
                    mp_state = MP_PLAYING;
                    BUZZER_PlayTones(tones_startup);
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

        default: {
            break;
        }
    }
}