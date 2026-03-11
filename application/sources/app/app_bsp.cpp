#include "button.h"
#include "sys_dbg.h"
#include "app.h"
#include "app_bsp.h"
#include "app_dbg.h"
#include "app_if.h"
#include "task_list.h"
#include "scr_archery_game.h"

// [QUAN TRỌNG] Include Driver để gọi hàm Init
#include "nRF24.h" 

button_t btn_mode;
button_t btn_up;
button_t btn_down;

// --- CALLBACK NÚT BẤM (GIỮ NGUYÊN) ---
void btn_mode_callback(void* b) {
	button_t* me_b = (button_t*)b;
	if (me_b->state == BUTTON_SW_STATE_RELEASED) 
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_MODE_RELEASED);
}

void btn_up_callback(void* b) {
	button_t* me_b = (button_t*)b;
	if (me_b->state == BUTTON_SW_STATE_PRESSED) 
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_UP_PRESSED);
	else if (me_b->state == BUTTON_SW_STATE_RELEASED)
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_UP_RELEASED);
}

void btn_down_callback(void* b) {
	button_t* me_b = (button_t*)b;
	if (me_b->state == BUTTON_SW_STATE_PRESSED) 
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_DOWN_PRESSED);
	else if (me_b->state == BUTTON_SW_STATE_RELEASED)
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_DOWN_RELEASED);
}

// --- HÀM KHỞI TẠO PHẦN CỨNG RF (MỚI THÊM VÀO) ---
// Hàm này sẽ được gọi từ file game khi bắt đầu
void rf_init_hardware_kit() {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    // 1. Bật Clock cho SPI1 và GPIOA (chân SPI nằm ở Port A)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    // 2. Cấu hình chân SCK(PA5), MISO(PA6), MOSI(PA7)
    // Đây là các chân cứng của SPI1 trên STM32L151
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; // Chế độ Alternated Function
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Gán chức năng AF SPI1 cho các chân này
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    // 3. Cấu hình module SPI1
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;       // Mình là chủ, nRF là tớ
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;   // Gửi lần lượt 8 bit
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;          // Idle thấp
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;        // Bắt cạnh lên
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;           // CS điều khiển bằng phần mềm
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // Tốc độ an toàn
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    // 4. Bật SPI1 hoạt động
    SPI_Cmd(SPI1, ENABLE);

    // 5. Gọi hàm khởi tạo của Driver nRF24
    // Hàm này sẽ cấu hình nốt các chân CE, CSN, IRQ
    nRF24_Init();
}