#include "scr_game_over.h"

extern uint32_t ar_game_score; 

// =================================================================
// BIẾN ANIMATION & TRẠNG THÁI
// =================================================================
static ar_game_score_t gamescore_over; 
static bool is_new_record = false;

static int8_t anim_y = 0;   // Dịch chuyển lên xuống cho chữ GAME OVER
static int8_t anim_dir = 1; 
static bool blink_state = true; // Trạng thái nhấp nháy chữ NEW BEST

/*****************************************************************************/
/* View - Game Over */
/*****************************************************************************/
static void view_scr_game_over();

view_dynamic_t dyn_view_item_game_over = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_over
};

view_screen_t scr_game_over = {
	&dyn_view_item_game_over,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

void view_scr_game_over() {
	view_render.clear(); // Xóa nền
	
	// =========================================================
	// 1. CHỮ "GAME OVER" LƠ LỬNG
	// =========================================================
	view_render.setTextSize(2);
	view_render.setTextColor(WHITE);
	view_render.setCursor(12, 5 + anim_y); // Cộng anim_y để trôi bồng bềnh
	view_render.print("GAME OVER");

	// =========================================================
	// 2. ĐIỂM SỐ & KỶ LỤC
	// =========================================================
	view_render.setTextSize(1);
	view_render.setCursor(20, 28);
	view_render.print("SCORE:");
	view_render.setCursor(65, 28);
	view_render.print(ar_game_score);

	if (is_new_record) {
		// Nếu phá kỷ lục -> Nhấp nháy dòng chữ NEW BEST!
		if (blink_state) {
			view_render.setCursor(20, 38);
			view_render.print("NEW BEST!");
			view_render.setCursor(80, 38);
			view_render.print(gamescore_over.score_1st);
		}
	} else {
		// Nếu không phá kỷ lục -> Hiện bình thường
		view_render.setCursor(20, 38);
		view_render.print("BEST :");
		view_render.setCursor(65, 38);
		view_render.print(gamescore_over.score_1st);
	}

	// =========================================================
	// 3. VẼ VECTOR 3 ICON MINI (Thay cho bitmap to đùng)
	// =========================================================
	
	// ICON 1: Chơi lại (Nút DOWN) - Hình vòng lặp (Restart)
	int x1 = 25, y1 = 54;
	view_render.drawFastHLine(x1+2, y1+1, 4, WHITE); // Vòng cung trên
	view_render.drawFastHLine(x1+2, y1+7, 4, WHITE); // Vòng cung dưới
	view_render.drawFastVLine(x1+1, y1+3, 3, WHITE); // Vòng cung trái
	view_render.drawFastVLine(x1+7, y1+4, 2, WHITE); // Vòng cung phải
	view_render.drawPixel(x1+2, y1+2, WHITE);        // Bo góc trái trên
	view_render.drawPixel(x1+2, y1+6, WHITE);        // Bo góc trái dưới
	view_render.drawPixel(x1+6, y1+6, WHITE);        // Bo góc phải dưới
	view_render.drawLine(x1+5, y1-1, x1+7, y1+1, WHITE); // Cánh mũi tên trên
	view_render.drawLine(x1+5, y1+3, x1+7, y1+1, WHITE); // Cánh mũi tên dưới

	// ICON 2: Bảng Xếp Hạng (Nút UP) - Bục nhận giải 3 bậc
	int x2 = 60, y2 = 54;
	// Bậc 2 (Trái)
	view_render.drawFastHLine(x2, y2+4, 3, WHITE);
	view_render.drawFastVLine(x2, y2+4, 5, WHITE);
	// Bậc 1 (Giữa)
	view_render.drawFastHLine(x2+3, y2, 4, WHITE);
	view_render.drawFastVLine(x2+3, y2, 9, WHITE);
	view_render.drawFastVLine(x2+7, y2, 9, WHITE);
	// Bậc 3 (Phải)
	view_render.drawFastHLine(x2+8, y2+6, 3, WHITE);
	view_render.drawFastVLine(x2+11, y2+6, 3, WHITE);
	view_render.drawFastHLine(x2, y2+9, 12, WHITE); // Mặt đất bục

	// ICON 3: Trang chủ (Nút MODE) - Hình Ngôi nhà mini
	int x3 = 96, y3 = 54;
	view_render.drawLine(x3, y3+4, x3+4, y3, WHITE);     // Mái trái
	view_render.drawLine(x3+4, y3, x3+8, y3+4, WHITE);   // Mái phải
	view_render.drawFastVLine(x3+1, y3+4, 5, WHITE);     // Tường trái
	view_render.drawFastVLine(x3+7, y3+4, 5, WHITE);     // Tường phải
	view_render.drawFastHLine(x3+1, y3+8, 7, WHITE);     // Sàn nhà
	view_render.drawFastVLine(x3+3, y3+6, 3, WHITE);     // Cửa
	view_render.drawFastVLine(x3+5, y3+6, 3, WHITE);     // Cửa

	view_render.update();
}

/*****************************************************************************/
/* Handle - Game Over */
/*****************************************************************************/
void scr_game_over_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		
		eeprom_read(EEPROM_SCORE_START_ADDR, (uint8_t*)&gamescore_over, sizeof(gamescore_over));
		is_new_record = false;
		
		// Thuật toán kiểm tra kỷ lục
		if (ar_game_score > gamescore_over.score_1st) {
			gamescore_over.score_3rd = gamescore_over.score_2nd;
			gamescore_over.score_2nd = gamescore_over.score_1st;
			gamescore_over.score_1st = ar_game_score;
			is_new_record = true;
		} 
		else if (ar_game_score > gamescore_over.score_2nd && ar_game_score < gamescore_over.score_1st) {
			gamescore_over.score_3rd = gamescore_over.score_2nd;
			gamescore_over.score_2nd = ar_game_score;
			is_new_record = true; // Lọt top 2
		}
		else if (ar_game_score > gamescore_over.score_3rd && ar_game_score < gamescore_over.score_2nd) {
			gamescore_over.score_3rd = ar_game_score;
			is_new_record = true; // Lọt top 3
		}

		if (is_new_record) {
			eeprom_write(EEPROM_SCORE_START_ADDR, (uint8_t*)&gamescore_over, sizeof(gamescore_over));
		}

		// Khởi tạo animation
		anim_y = 0;
		anim_dir = 1;
		blink_state = true;

		// Bật Timer chu kỳ 100ms để chạy Animation
		timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 100, TIMER_ONE_SHOT);

		view_render.initialize();
		view_render_display_on();
	}
		break;

	case AR_GAME_TIME_TICK: {
		// 1. Tính toán tọa độ lơ lửng cho chữ GAME OVER
		anim_y += anim_dir;
		if (anim_y >= 2 || anim_y <= -2) {
			anim_dir = -anim_dir; 
		}
		
		// 2. Chớp tắt chữ nếu có Kỷ lục mới
		blink_state = !blink_state;

		// Vẽ lại màn hình
		view_scr_game_over();

		// Lặp lại Timer
		timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 150, TIMER_ONE_SHOT);
		break;
	}

	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		APP_DBG_SIG("RESTART_GAME\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK); // TẮT TIMER
		SCREEN_TRAN(scr_archery_game_handle, &scr_archery_game);
		BUZZER_PlayTones(tones_startup);
	}
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		APP_DBG_SIG("GO_CHARTS\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK); // TẮT TIMER
		SCREEN_TRAN(scr_charts_game_handle, &scr_charts_game);
		BUZZER_PlayTones(tones_cc);
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("GO_HOME\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK); // TẮT TIMER
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
		BUZZER_PlayTones(tones_cc);
	}
		break;

	default:
		break;
	}
}