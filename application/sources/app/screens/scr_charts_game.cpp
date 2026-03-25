#include "scr_charts_game.h"

/*****************************************************************************/
/* Variable Declaration - Charts game */
/*****************************************************************************/
static ar_game_score_t gamescore_charts;

/*****************************************************************************/
/* View - Charts game */
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
	view_render.clear();
	
	// Tiêu đề (Dùng lại bitmap HIGH SCORE cũ)
	view_render.drawBitmap(34, 2, bitmap_icon_charts, 60, 20, WHITE);

	// Khung viền tổng thể cho bảng xếp hạng
	view_render.drawRoundRect(8, 24, 112, 38, 3, WHITE);

	// ==========================================
	// TOP 1: Nổi bật nhất (Nền trắng, chữ đen)
	// ==========================================
	view_render.fillRoundRect(8, 24, 112, 14, 3, WHITE);
	view_render.setTextColor(BLACK);
	view_render.setCursor(16, 27);
	view_render.print("1ST :");
	view_render.setCursor(55, 27);
	view_render.print(gamescore_charts.score_1st);
	
	// Thêm icon Khủng long đứng cạnh kỷ lục Top 1 (Vẽ bằng màu ĐEN để đục lỗ nền trắng)
	view_render.drawBitmap(95, 23, bitmap_dino, 16, 16, BLACK);

	// ==========================================
	// TOP 2: Nền đen, chữ trắng
	// ==========================================
	view_render.setTextColor(WHITE);
	view_render.setCursor(16, 40);
	view_render.print("2ND :");
	view_render.setCursor(55, 40);
	view_render.print(gamescore_charts.score_2nd);

	// ==========================================
	// TOP 3: Nền đen, chữ trắng
	// ==========================================
	view_render.setCursor(16, 52);
	view_render.print("3RD :");
	view_render.setCursor(55, 52);
	view_render.print(gamescore_charts.score_3rd);

	view_render.update();
}

/*****************************************************************************/
/* Handle - Charts game */
/*****************************************************************************/
void scr_charts_game_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		// Đọc điểm từ bộ nhớ EEPROM khi vừa vào màn hình
		eeprom_read(EEPROM_SCORE_START_ADDR, (uint8_t*)&gamescore_charts, sizeof(gamescore_charts));
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: 
	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		// Bấm MODE hoặc DOWN để thoát về Menu
		APP_DBG_SIG("EXIT_CHARTS\n");
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);	
		BUZZER_PlayTones(tones_cc);
	}
		break;

	case AC_DISPLAY_BUTTON_UP_PRESSED: {
		// Nhấn giữ nút UP để Reset toàn bộ điểm (Xóa bảng vàng)
		APP_DBG_SIG("RESET_SCORE\n");
		gamescore_charts.score_1st = 0;
		gamescore_charts.score_2nd = 0;
		gamescore_charts.score_3rd = 0;
		
		// Lưu lại vào EEPROM
		eeprom_write(EEPROM_SCORE_START_ADDR, (uint8_t*)&gamescore_charts, sizeof(gamescore_charts));
		view_scr_charts_game();
		BUZZER_PlayTones(tones_cc);
	}
		break;

	default:
		break;
	}
}