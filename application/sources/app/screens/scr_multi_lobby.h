#ifndef __SCR_MULTI_LOBBY_H__
#define __SCR_MULTI_LOBBY_H__

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"
#include "sys_ctrl.h"
#include "sys_dbg.h"
#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_display.h"
#include "view_render.h"
#include "buzzer.h"
#include "screens.h"
#include "screens_bitmap.h"

// Biến lưu kênh sóng toàn cục dùng chung cho cả 2 máy trong phòng
extern uint8_t current_rf_channel;

extern view_dynamic_t dyn_view_item_multi_lobby;
extern view_screen_t scr_multi_lobby;
extern void scr_multi_lobby_handle(ak_msg_t* msg);

#endif //__SCR_MULTI_LOBBY_H__