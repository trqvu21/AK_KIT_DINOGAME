#include "scr_multi_lobby.h"

// Khởi tạo kênh sóng mặc định (sẽ bị ghi đè khi chọn phòng)
uint8_t current_rf_channel = 40; 
// Mặc định vào sảnh là phòng 01
static uint8_t room_number = 1;

static void view_scr_multi_lobby();

view_dynamic_t dyn_view_item_multi_lobby = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_multi_lobby
};

view_screen_t scr_multi_lobby = {
	&dyn_view_item_multi_lobby,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

void view_scr_multi_lobby() {
	view_render.clear();
	
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	
	// Tiêu đề Sảnh chờ
	view_render.setCursor(25, 5);
	view_render.print("MULTIPLAYER");
	view_render.drawFastHLine(20, 15, 88, WHITE);

	// Hiển thị Số phòng (Chèn thêm số 0 nếu < 10 cho đẹp)
	view_render.setCursor(30, 30);
	view_render.print("ROOM: [ ");
	if (room_number < 10) view_render.print("0");
	view_render.print(room_number);
	view_render.print(" ]");

	// Hướng dẫn phím bấm
	view_render.setCursor(15, 50);
	view_render.print("MODE: JOIN ROOM");

	view_render.update();
}

void scr_multi_lobby_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY LOBBY\n");
		view_render.initialize();
		view_render_display_on();
	}
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		// Tăng số phòng
		room_number++;
		if (room_number > 99) room_number = 1;
		BUZZER_PlayTones(tones_cc);
		view_scr_multi_lobby(); // Vẽ lại màn hình ngay
	}
		break;

	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		// Giảm số phòng
		room_number--;
		if (room_number < 1) room_number = 99;
		BUZZER_PlayTones(tones_cc);
		view_scr_multi_lobby(); // Vẽ lại màn hình ngay
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("JOIN_ROOM\n");
		// Chốt phòng: Tính toán kênh sóng thực tế (VD: Kênh cơ sở 10 + Số phòng)
		current_rf_channel = 10 + room_number;
		
		// Chuyển sang màn hình Game chính
		SCREEN_TRAN(scr_archery_game_handle, &scr_archery_game);
		BUZZER_PlayTones(tones_startup);
	}
		break;

    case AC_DISPLAY_BUTTON_MODE_LONG_PRESSED: {
        // Nhấn giữ MODE để hủy, quay lại Menu chính
        SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
        BUZZER_PlayTones(tones_cc);
    }
        break;

	default:
		break;
	}
}