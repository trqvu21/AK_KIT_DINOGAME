#include "scr_startup.h"
#include "screens.h"

// Gọi các hình ảnh bitmap đã có sẵn của game
extern const unsigned char bitmap_dino[];
extern const unsigned char bitmap_dino_duck[];

extern ar_game_setting_t settingdata; // Khai báo lại biến cấu hình hệ thống

// =================================================================
// BIẾN ANIMATION KHỞI ĐỘNG
// =================================================================
static int anim_frame = 0;   // Đếm khung hình
static int dino_x = 0;       // Tọa độ X của Dino

/*****************************************************************************/
/* View - Startup Screen (Logo DINO Cute) */
/*****************************************************************************/
static void view_scr_startup();

// Giữ nguyên tên struct chuẩn dyn_view_startup của hệ thống
view_dynamic_t dyn_view_startup = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_startup
};

view_screen_t scr_startup = {
	&dyn_view_startup,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

void view_scr_startup() {
	view_render.clear(); // Xóa nền thành màu đen

	// 1. Vẽ Tiêu đề "DINO GAME" ở giữa màn hình
	view_render.setTextSize(2);
	view_render.setTextColor(WHITE);
	view_render.setCursor(12, 10);
	view_render.print("DINO GAME");

	// 2. Chữ Loading nhấp nháy ở dưới
	if (anim_frame % 4 < 2) { // Nhấp nháy chu kỳ
		view_render.setTextSize(1);
		view_render.setCursor(35, 45);
		view_render.print("Loading...");
	}

	// 3. Khủng long lật đật chạy ngang màn hình
	int dino_y = 28;
	
	// Tạo hiệu ứng lật đật bằng cách đổi frame
	if (anim_frame % 2 == 0) {
		view_render.drawBitmap(dino_x, dino_y, bitmap_dino, 16, 16, WHITE);
	} else {
		// Nhấc lên một chút tạo cảm giác đang nhảy bước
		view_render.drawBitmap(dino_x, dino_y - 2, bitmap_dino, 16, 16, WHITE); 
	}

	view_render.update(); // Đẩy khung hình ra OLED
}

/*****************************************************************************/
/* Handle - Startup Screen */
/*****************************************************************************/
void scr_startup_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_DISPLAY_INITIAL: {
		// ĐÂY LÀ TÍN HIỆU BOOT MẶC ĐỊNH CỦA MẠCH AK
		APP_DBG_SIG("AC_DISPLAY_INITIAL\n");
		
		// BẮT BUỘC PHẢI CÓ: Khởi tạo phần cứng OLED
		view_render.initialize();
		view_render_display_on();
		
		// BẮT BUỘC PHẢI CÓ: Đọc cấu hình EEPROM cho loa Buzzer
		eeprom_read(EEPROM_SETTING_START_ADDR, (uint8_t*)&settingdata, sizeof(settingdata));
		BUZZER_Sleep(settingdata.silent);

		// Reset lại animation
		anim_frame = 0;
		dino_x = 0; 

		// Bật Timer chạy Animation (Dùng tín hiệu chuẩn AC_DISPLAY_SHOW_LOGO)
		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_LOGO, 100, TIMER_ONE_SHOT);
		break;
	}

	case AC_DISPLAY_SHOW_LOGO: {
		anim_frame++;
		dino_x += 8; // Tốc độ chạy của Khủng long (8 pixel mỗi frame)

		// Cập nhật lại màn hình
		view_scr_startup();

		// Kiểm tra điều kiện kết thúc
		if (dino_x > 128) {
			// Phát 1 tiếng bíp nhẹ để báo hiệu vào game
			BUZZER_PlayTones(tones_cc); 
			// Chuyển sang màn hình Menu
			SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
		} else {
			// Tiếp tục lặp lại Timer Animation
			timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_LOGO, 100, TIMER_ONE_SHOT);
		}
		break;
	}

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		// Tính năng tiện ích: Bấm MODE để bỏ qua hoạt cảnh (Skip Intro)
		APP_DBG_SIG("SKIP STARTUP\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_LOGO);
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
		break;
	}

	default:
		break;
	}
}