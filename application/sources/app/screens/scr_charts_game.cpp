#include "scr_charts_game.h"
#include "screens.h"

// Biến lưu trữ bảng điểm đọc từ EEPROM
static ar_game_score_t gamescore_charts;

// =================================================================
// BIẾN ANIMATION LƠ LỬNG
// =================================================================
static int8_t anim_y = 0;   // Biên độ dịch chuyển Y
static int8_t anim_dir = 1; // Hướng dịch chuyển (1: Đi xuống, -1: Đi lên)

/*****************************************************************************/
/* View - Charts Game (Bục nhận giải Podium) */
/*****************************************************************************/
static void view_scr_charts_game();

view_dynamic_t dyn_view_item_charts_game = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_charts_game
};

view_screen_t scr_charts_game = {
	&dyn_view_item_charts_game,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

void view_scr_charts_game() {
	view_render.clear(); // Xóa sạch nền đen

	// 1. Vẽ Tiêu đề & Hướng dẫn
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(0, 0);
	view_render.print("<[M]"); // Bấm phím Mode để thoát
	
	view_render.setCursor(45, 0);
	view_render.print("CHARTS");

	view_render.setCursor(95, 0);
	view_render.print("[^]RS"); // Hướng dẫn bấm phím UP để Reset điểm

	// =================================================================
	// 2. VẼ BỤC NHẬN GIẢI (PODIUM OUTLINES) - GIỮ NGUYÊN
	// =================================================================
	view_render.drawFastHLine(0, 63, 128, WHITE);     // Mặt đất

	view_render.drawFastHLine(0, 42, 42, WHITE);      // Mặt bục 2
	view_render.drawFastVLine(42, 28, 14, WHITE);     // Vách nối 2-1

	view_render.drawFastHLine(42, 28, 44, WHITE);     // Mặt bục 1
	view_render.drawFastVLine(86, 28, 22, WHITE);     // Vách nối 1-3

	view_render.drawFastHLine(86, 50, 42, WHITE);     // Mặt bục 3

	// =================================================================
	// 3. VẼ SỐ HẠNG (1, 2, 3) VÀO GIỮA BỤC (ĐÃ THIẾT KẾ LẠI)
	// Chuyển sang cỡ chữ nhỏ (size 1) để cân đối hơn.
	// Cập nhật lại vị trí setCursor để căn giữa ô bục.
	// =================================================================
	view_render.setTextSize(1);
	
	// Hạng 2 (Bên trái, bục rộng 42, cao 21)
	view_render.setCursor(18, 48); 
	view_render.print("2"); 
	
	// Hạng 1 (Chính giữa, bục rộng 44, cao 35)
	view_render.setCursor(61, 41); 
	view_render.print("1"); 
	
	// Hạng 3 (Bên phải, bục rộng 42, cao 13)
	view_render.setCursor(104, 52); 
	view_render.print("3"); 

	// =================================================================
	// 4. VẼ ĐIỂM SỐ LƠ LỬNG TRÊN BỤC - GIỮ NGUYÊN
	// =================================================================
	// Cỡ chữ nhỏ (size 1) và lơ lửng nhẹ nhàng.
	
	// Điểm TOP 2 (Trái)
	view_render.setCursor(10, 30 + anim_y);
	view_render.print(gamescore_charts.score_2nd);

	// Điểm TOP 1 (Giữa)
	view_render.setCursor(52, 14 + anim_y);
	view_render.print(gamescore_charts.score_1st);

	// Điểm TOP 3 (Phải)
	view_render.setCursor(96, 38 + anim_y);
	view_render.print(gamescore_charts.score_3rd);

	view_render.update(); // Đẩy toàn bộ hình ảnh ra màn hình OLED
}

/*****************************************************************************/
/* Handle - Charts Game */
/*****************************************************************************/
void scr_charts_game_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		
		// Đọc top 3 điểm số từ bộ nhớ EEPROM
		eeprom_read(EEPROM_SCORE_START_ADDR, (uint8_t*)&gamescore_charts, sizeof(gamescore_charts));

		// Khởi tạo trạng thái Animation ban đầu
		anim_y = 0;
		anim_dir = 1;

		// Bật Timer để chạy Animation
		timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 100, TIMER_ONE_SHOT);

		view_render.initialize();
		view_render_display_on();
	}
		break;

	case AR_GAME_TIME_TICK: {
		// Sóng nhấp nhả (Biên độ -2 đến +2 pixel)
		anim_y += anim_dir;
		if (anim_y >= 2 || anim_y <= -2) {
			anim_dir = -anim_dir; // Đảo chiều khi chạm đỉnh/đáy
		}

		// Tự động vẽ lại màn hình liên tục với biến số đã cập nhật
		view_scr_charts_game();

		// Lặp lại Timer
		timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 100, TIMER_ONE_SHOT);
		break;
	}

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		APP_DBG_SIG("RESET_CHARTS\n");

		// 1. Cập nhật biến tạm trên RAM về 0
		gamescore_charts.score_1st = 0;
		gamescore_charts.score_2nd = 0;
		gamescore_charts.score_3rd = 0;

		// 2. Ghi đè vào bộ nhớ vĩnh viễn (EEPROM)
		eeprom_write(EEPROM_SCORE_START_ADDR, (uint8_t*)&gamescore_charts, sizeof(gamescore_charts));

		// 3. Kêu 3 tiếng bíp báo hiệu đã xóa thành công
		BUZZER_PlayTones(tones_3beep);
		break;
	}

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("GO_HOME\n");
		
		// Phải tắt Timer Animation trước khi chuyển cảnh
		timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK);
		
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
		BUZZER_PlayTones(tones_cc);
	}
		break;

	default:
		break;
	}
}