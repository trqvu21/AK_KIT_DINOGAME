# Multiplayer Dino Game - Build on AK Embedded Base Kit

<hr>

## I. Giới thiệu

Multiplayer Dino Game là một trò chơi chạy trên **AK Embedded Base Kit**. Dự án được xây dựng để thực hành lập trình nhúng theo mô hình **event-driven**, sử dụng Task, Signal, Timer, Message, State Machine, OLED, Button, Buzzer và giao tiếp không dây **NRF24L01+**.

Game lấy cảm hứng từ Dino runner: người chơi điều khiển khủng long né chướng ngại vật, ăn hộp quà để tấn công đối thủ, và cố gắng đạt điểm cao nhất.

### 1.1 Phần cứng

<p align="center"><img src="resources/images/AK_Embedded_Base_Kit_STM32L151.webp" alt="AK Embedded Base Kit - STM32L151" width="480"/></p>
<p align="center"><strong><em>Hình 1:</em></strong> AK Embedded Base Kit - STM32L151</p>

[AK Embedded Base Kit](https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu) là một evaluation kit dành cho các bạn học phần mềm nhúng nâng cao.

KIT tích hợp LCD **OLED 1.3"**, **3 nút nhấn**, **Buzzer**, **NRF24L01+**, RS485 và Flash ngoài. Trong dự án này, OLED dùng để hiển thị game, nút nhấn dùng để điều khiển, Buzzer dùng cho phản hồi âm thanh, và NRF24L01+ dùng cho chế độ multiplayer.

### 1.2 Mô tả trò chơi

<p align="center"><img src="resources/images/menudino.webp" alt="AK Embedded Base Kit - STM32L151" width="480"/></p>
<p align="center"><strong><em>Hình 2:</em></strong> Màn hình menu game</p>

#### 1.2.1 Đối tượng trong game

| Đối tượng | Tên | Mô tả |
|---|---|---|
| Khủng long | Dino | Nhân vật chính, có thể nhảy hoặc cúi. |
| Xương rồng | Cactus | Chướng ngại vật sát đất, cần nhảy qua. |
| Chim | Bird | Chướng ngại vật bay ở tầm cao/thấp, cần nhảy hoặc cúi để né. |
| Hộp quà | Gift | Vật phẩm đặc biệt, ăn được sẽ cộng điểm và gửi đòn tấn công qua RF. |
| Mây | Cloud | Cảnh nền để màn chơi có chiều sâu. |

#### 1.2.2 Cách chơi

- Khi vào game, thiết bị hiển thị tên ngẫu nhiên dạng `MY NAME: [P73]`.
- Nhấn `DOWN` để gửi trạng thái Ready và chờ đối thủ.
- Thiết bị nhận lời mời hiển thị `[Pxx] INVITES!`.
- Nhấn `UP` để chấp nhận, nhấn `DOWN` để từ chối.
- Khi đang chờ phản hồi, nhấn `DOWN` thêm một lần để vào chế độ Solo.
- Khi chơi, nhấn `UP` để Dino nhảy, giữ `DOWN` để Dino cúi.
- Game kết thúc khi Dino va vào Cactus hoặc Bird.

#### 1.2.3 Cơ chế điểm và độ khó

- Vượt qua Cactus hoặc Bird: `+1` điểm.
- Ăn Gift: `+5` điểm và gửi lệnh `CMD_ATTACK` sang đối thủ.
- Mỗi `15` điểm, game tăng một level tốc độ và hiện thông báo `SPD UP`.
- Ở điểm cao, tốc độ tăng mạnh hơn, khoảng cách vật cản ngắn hơn, Bird xuất hiện nhiều hơn.
- Khi bị tấn công, thiết bị nhận lệnh sẽ hiện `SPEED UP!`, phát âm báo và tăng tốc tạm thời trong một khoảng thời gian ngắn.

## II. Thiết kế event-driven

**Các khái niệm trong event-driven:**

- **Event Driven:** Hệ thống gửi message để kích hoạt hành vi. Task đóng vai trò người nhận, Signal biểu diễn nội dung công việc.
- **Task:** Đơn vị xử lý một nhóm công việc cụ thể. Khi scheduler lấy được message của task, handler tương ứng sẽ được gọi.
- **Message:** Gói sự kiện được đưa vào hàng đợi. Message có thể chỉ chứa Signal hoặc chứa cả Signal và Data.
- **Signal:** Mã định danh hành động cần xử lý, ví dụ `SCREEN_ENTRY`, `AR_GAME_TIME_TICK`, `AR_GAME_RF_SEND_ATTACK`.
- **Handler:** Hàm xử lý message/signal của một task hoặc module.

### 2.1 Mục tiêu kiến trúc

Phiên bản hiện tại đã tách logic khỏi `scr_archery_game.cpp`. Screen chỉ giữ vai trò điều phối màn hình và vòng lặp timer, còn logic game nằm trong các module riêng:

| Module | Vai trò |
|---|---|
| `ar_game_dino` | Quản lý Dino, nhảy, cúi, trọng lực và hitbox. |
| `ar_game_objects` | Quản lý Cactus, Bird, Gift, spawn, recycle và va chạm. |
| `ar_game_background` | Quản lý mây và nền. |
| `ar_game_world` | Quản lý điểm, tốc độ, level, trạng thái win/lose và hiệu ứng speed up. |
| `ar_game_rf` | Quản lý NRF24L01+, lobby, ready/accept/reject/solo, attack và died command. |
| `scr_archery_game` | Điều phối Screen Entry, Timer Tick, Button Event và Render. |

Kiến trúc vẫn giữ số lượng task game hiện có để tránh tăng rủi ro tràn bộ nhớ, nhưng đổi vai trò thành các module Dino rõ ràng hơn.

### 2.2 Sơ đồ trình tự

<p align="center"><img src="resources/images/sq.webp" alt="AK Embedded Base Kit - STM32L151" width="480"/></p>
<p align="center"><strong><em>Hình 3:</em></strong> The sequence diagram</p>

### 2.3 Message và Signal chính

| Nhóm | Signal / Event | Mô tả |
|---|---|---|
| Screen | `SCREEN_ENTRY` | Khởi tạo game, đọc setting, reset module và setup RF. |
| Screen | `AR_GAME_TIME_TICK` | Tick 10ms, poll RF và chia nhịp gameplay. |
| Button | `AC_DISPLAY_BUTTON_UP_PRESSED` | Nhảy khi đang chơi. |
| Button | `AC_DISPLAY_BUTTON_UP_RELEASED` | Accept lời mời khi đang ở lobby. |
| Button | `AC_DISPLAY_BUTTON_DOWN_RELEASED` | Ready, Solo hoặc Reject trong lobby. |
| RF | `CMD_READY`, `CMD_ACCEPT`, `CMD_REJECT` | Matchmaking giữa hai thiết bị. |
| RF | `CMD_ATTACK`, `CMD_I_DIED` | Tấn công và báo kết thúc ván. |

### 2.4 Task

Trong code, game vẫn tái sử dụng các Task ID cũ của project để không tăng số task trong hệ thống. Tên task đã được đổi theo vai trò Dino mới.

| Task ID | Handler | Module | Vai trò |
|---|---|---|---|
| `AR_GAME_BACKGROUND_ID` | `ar_game_background_handle` | `ar_game_background` | Setup/reset/update background. |
| `AR_GAME_WORLD_ID` | `ar_game_world_handle` | `ar_game_world` | Score, speed, level, win/lose, attack timer. |
| `AR_GAME_DINO_ID` | `ar_game_dino_handle` | `ar_game_dino` | Dino physics, jump, hitbox. |
| `AR_GAME_OBJECTS_ID` | `ar_game_objects_handle` | `ar_game_objects` | Cactus/Bird/Gift movement, spawn, collision. |
| `AR_GAME_RF_ID` | `ar_game_rf_handle` | `ar_game_rf` | NRF24 command, lobby, attack/died. |
| `AR_GAME_SCREEN_ID` | `scr_archery_game_handle` | `scr_archery_game` | Screen event, timer tick, button dispatch, render frame. |

**Ghi chú hiệu năng:** Trong gameplay, các hàm update chính được gọi trực tiếp từ `scr_archery_game` để tránh overhead message queue và tránh tụt FPS khi spam nút. Các handler task vẫn được giữ cho setup/reset/RF command và để kiến trúc event-driven rõ ràng.

### 2.5 Signal theo module

| Module | Signal | Mô tả |
|---|---|---|
| Screen | `SCREEN_ENTRY` | Đọc setting, reset module, setup RF, bật timer tick. |
| Screen | `AR_GAME_TIME_TICK` | Poll RF, chia nhịp gameplay, update và render frame. |
| Dino | `AR_GAME_DINO_SETUP` | Reset Dino về vị trí mặt đất. |
| Dino | `AR_GAME_DINO_UPDATE` | Cập nhật cúi, nhảy, trọng lực. |
| Dino | `AR_GAME_DINO_JUMP` | Bắt đầu nhảy nếu Dino đang ở mặt đất. |
| Dino | `AR_GAME_DINO_RESET` | Reset Dino sau start/restart. |
| Objects | `AR_GAME_OBJECTS_SETUP` | Tạo mảng object ban đầu. |
| Objects | `AR_GAME_OBJECTS_UPDATE` | Di chuyển, recycle, kiểm tra collision. |
| Objects | `AR_GAME_OBJECTS_RESET` | Reset toàn bộ object. |
| World | `AR_GAME_WORLD_UPDATE` | Tính tốc độ, level, timer thông báo. |
| World | `AR_GAME_WORLD_ATTACK_BEGIN` | Bắt đầu hiệu ứng bị attack. |
| World | `AR_GAME_WORLD_LOSE` | Chuyển trạng thái thua và qua Game Over. |
| World | `AR_GAME_WORLD_WIN` | Chuyển trạng thái thắng và qua Game Over. |
| Background | `AR_GAME_BACKGROUND_UPDATE` | Di chuyển cloud nền. |
| RF | `AR_GAME_RF_SETUP` | Khởi tạo tên người chơi và NRF24. |
| RF | `AR_GAME_RF_POLL` | Đọc packet RF nếu có. |
| RF | `AR_GAME_RF_READY` | Ready, Solo hoặc Reject tùy `lobby_state`. |
| RF | `AR_GAME_RF_ACCEPT` | Chấp nhận lời mời và start match. |
| RF | `AR_GAME_RF_SEND_ATTACK` | Gửi `CMD_ATTACK`. |
| RF | `AR_GAME_RF_SEND_DIED` | Gửi `CMD_I_DIED`. |

### 2.6 Bitmap và tài nguyên

| Bitmap | File | Kích thước | Chức năng |
|---|---|---:|---|
| `bitmap_dino` | `screens_bitmap.cpp` | 16x16 | Dino đứng/chạy/nhảy. |
| `bitmap_dino_duck` | `screens_bitmap.cpp` | 16x16 | Dino cúi. |
| `bitmap_cactus` | `screens_bitmap.cpp` | 16x16 | Cactus. |
| `bitmap_bird` | `screens_bitmap.cpp` | 16x8 | Bird. |
| `bitmap_gift` | `screens_bitmap.cpp` | 8x8 | Gift attack. |
| `bitmap_cloud` | `screens_bitmap.cpp` | 16x16 | Cloud background. |

## III. Sequence chi tiết cho từng đối tượng

### 3.1 Dino

**Sequence diagram:**

```mermaid
sequenceDiagram
    autonumber
    participant Player
    participant Screen as scr_archery_game
    participant Dino as ar_game_dino
    participant Down as btn_down
    participant Objects as ar_game_objects
    participant OLED as view_render

    rect rgba(0, 180, 80, 0.12)
    Note over Screen, Dino: SETUP / RESET
    Screen->>Dino: ar_game_dino_reset()
    activate Dino
    Dino->>Dino: dino.y = AR_DINO_GROUND_Y_SCALED - (AR_DINO_H * 10)
    Dino->>Dino: dino.v_y = 0
    Dino->>Dino: dino.is_jumping = false
    Dino->>Dino: dino.is_ducking = false
    deactivate Dino
    end

    rect rgba(255, 200, 0, 0.14)
    Note over Player, Dino: ACTION - Nhảy
    Player->>Screen: AC_DISPLAY_BUTTON_UP_PRESSED
    activate Screen
    alt ar_game_mp_state == AR_DINO_MP_PLAYING
        Screen->>Dino: ar_game_dino_jump()
        activate Dino
        alt !dino.is_jumping
            Dino->>Dino: dino.v_y = AR_DINO_JUMP_SCALED
            Dino->>Dino: dino.is_jumping = true
        else Dino đang nhảy
            Dino->>Dino: bỏ qua input
        end
        deactivate Dino
    end
    deactivate Screen
    end

    rect rgba(0, 140, 255, 0.12)
    Note over Screen, Dino: UPDATE - Cúi, nhảy, trọng lực
    Screen->>Dino: ar_game_dino_update()
    activate Dino
    Dino->>Down: đọc btn_down.state
    Down-->>Dino: BUTTON_SW_STATE_PRESSED / RELEASED
    Dino->>Dino: dino.is_ducking = (btn_down.state == BUTTON_SW_STATE_PRESSED)
    alt dino.is_jumping
        Dino->>Dino: dino.y += dino.v_y
        Dino->>Dino: dino.v_y += AR_DINO_GRAVITY_SCALED
        alt Dino chạm đất
            Dino->>Dino: dino.y = ground
            Dino->>Dino: dino.is_jumping = false
            Dino->>Dino: dino.v_y = 0
        end
    end
    deactivate Dino
    end

    rect rgba(255, 80, 80, 0.12)
    Note over Objects, Dino: HITBOX - Kiểm tra va chạm
    Objects->>Dino: ar_game_dino_hit_test(obj)
    activate Dino
    alt dino.is_ducking
        Dino->>Dino: giảm chiều cao hitbox
    else bình thường
        Dino->>Dino: dùng hitbox đầy đủ
    end
    Dino-->>Objects: true / false
    deactivate Dino
    end

    rect rgba(120, 120, 255, 0.10)
    Note over Screen, OLED: RENDER - Vẽ Dino
    Screen->>Dino: ar_game_dino_render()
    activate Dino
    alt dino.is_ducking
        Dino->>OLED: drawBitmap(bitmap_dino_duck)
    else đứng / nhảy
        Dino->>OLED: drawBitmap(bitmap_dino)
    end
    deactivate Dino
    end
```

**Tóm tắt nguyên lý:** Dino nhận hành động nhảy từ button event, nhận trạng thái cúi từ `btn_down.state`, tự cập nhật trọng lực theo nhịp gameplay và cung cấp hàm hitbox cho module Objects.

### 3.2 Objects: Cactus, Bird, Gift

**Sequence diagram:**

```mermaid
sequenceDiagram
    autonumber
    participant Screen as scr_archery_game
    participant Objects as ar_game_objects
    participant World as ar_game_world
    participant Dino as ar_game_dino
    participant RF as ar_game_rf
    participant OLED as view_render

    rect rgba(0, 180, 80, 0.12)
    Note over Screen, Objects: SETUP / RESET
    Screen->>Objects: ar_game_objects_reset()
    activate Objects
    Objects->>Objects: object_set(ar_game_objects[0], Cactus)
    Objects->>Objects: object_set(ar_game_objects[1], Cactus)
    Objects->>Objects: object_set(ar_game_objects[2], Gift)
    Objects->>Objects: object_set(ar_game_objects[3], Bird)
    deactivate Objects
    end

    rect rgba(0, 140, 255, 0.12)
    Note over Screen, Objects: UPDATE - Di chuyển và sinh lại object
    Screen->>Objects: ar_game_objects_update()
    activate Objects
    loop i = 0..AR_DINO_OBJECT_COUNT-1
        Objects->>World: đọc ar_game_current_speed
        Objects->>Objects: obj->x -= ar_game_current_speed
        alt obj->active == false
            Objects->>Objects: chờ object đi ra khỏi màn hình
        else object ra khỏi màn hình
            alt object_is_obstacle(obj)
                Objects->>World: ar_game_score++
            end
            Objects->>Objects: recycle_object(i)
        end
        Objects->>Objects: handle_object_collision(obj)
    end
    deactivate Objects
    end

    rect rgba(255, 80, 80, 0.12)
    Note over Objects, RF: COLLISION - Xử lý va chạm
    Objects->>Dino: ar_game_dino_hit_test(obj)
    alt không va chạm
        Dino-->>Objects: false
    else Hit Cactus hoặc Bird
        Dino-->>Objects: true
        Objects->>RF: task_post_pure_msg(AR_GAME_RF_ID, AR_GAME_RF_SEND_DIED)
        Objects->>World: task_post_pure_msg(AR_GAME_WORLD_ID, AR_GAME_WORLD_LOSE)
    else Hit Gift
        Dino-->>Objects: true
        Objects->>Objects: obj->active = false
        Objects->>World: ar_game_score += 5
        Objects->>RF: task_post_pure_msg(AR_GAME_RF_ID, AR_GAME_RF_SEND_ATTACK)
    end
    end

    rect rgba(120, 120, 255, 0.10)
    Note over Screen, OLED: RENDER - Vẽ object
    Screen->>Objects: ar_game_objects_render()
    activate Objects
    loop từng object đang active
        alt AR_DINO_OBJ_CACTUS
            Objects->>OLED: drawBitmap(bitmap_cactus)
        else AR_DINO_OBJ_BIRD
            Objects->>OLED: drawBitmap(bitmap_bird)
        else AR_DINO_OBJ_GIFT
            Objects->>OLED: draw sparkle pixels
            Objects->>OLED: drawBitmap(bitmap_gift)
        end
    end
    deactivate Objects
    end
```

**Tóm tắt nguyên lý:** Objects chịu trách nhiệm tạo nhịp chơi chính: di chuyển vật cản, recycle vật cản, tăng điểm, kiểm tra va chạm và gửi event sang World/RF.

### 3.3 World

**Sequence diagram:**

```mermaid
sequenceDiagram
    autonumber
    participant Screen as scr_archery_game
    participant World as ar_game_world
    participant RF as ar_game_rf
    participant Objects as ar_game_objects
    participant OLED as view_render
    participant GameOver as scr_game_over

    rect rgba(0, 180, 80, 0.12)
    Note over Screen, World: RESET - Đưa World về trạng thái đầu trận
    Screen->>World: ar_game_world_reset()
    activate World
    World->>World: ar_game_score = 0
    World->>World: ar_game_frame_skip = 0
    World->>World: ar_game_attack_timer = 0
    World->>World: ar_game_current_speed = AR_DINO_BASE_SPEED_SCALED
    World->>World: speed_level = 0
    World->>World: speed_up_notice_timer = 0
    deactivate World
    end

    rect rgba(0, 140, 255, 0.12)
    Note over Screen, World: UPDATE - Tính tốc độ và độ khó
    Screen->>World: ar_game_world_update()
    activate World
    World->>World: world_update_speed_notice()
    World->>World: ar_game_current_speed = world_base_speed()
    alt ar_game_attack_timer > 0
        World->>World: ar_game_attack_timer--
        World->>World: ar_game_current_speed += AR_DINO_ATTACK_BONUS_SPEED
    end
    alt speed_up_notice_timer > 0
        World->>World: speed_up_notice_timer--
    end
    deactivate World
    end

    rect rgba(255, 80, 80, 0.12)
    Note over RF, GameOver: ATTACK / WIN / LOSE
    RF->>World: AR_GAME_WORLD_ATTACK_BEGIN
    activate World
    World->>World: ar_game_attack_timer = AR_DINO_ATTACK_TICKS
    deactivate World

    Objects->>World: AR_GAME_WORLD_LOSE
    activate World
    World->>World: ar_game_mp_state = AR_DINO_MP_LOSE
    World->>GameOver: SCREEN_TRAN(scr_game_over_handle, &scr_game_over)
    deactivate World

    RF->>World: AR_GAME_WORLD_WIN
    activate World
    World->>World: ar_game_mp_state = AR_DINO_MP_WIN
    World->>GameOver: SCREEN_TRAN(scr_game_over_handle, &scr_game_over)
    deactivate World
    end

    rect rgba(120, 120, 255, 0.10)
    Note over Screen, OLED: RENDER - HUD và cảnh báo
    Screen->>World: ar_game_world_render_hud()
    activate World
    World->>OLED: vẽ ground line
    World->>OLED: vẽ score box
    deactivate World

    Screen->>World: ar_game_world_render_attack_warning()
    activate World
    alt speed_up_notice_timer > 0
        World->>OLED: print "SPD UP"
    end
    alt ar_game_attack_timer > 0
        World->>OLED: print "SPEED UP!"
    end
    deactivate World
    end
```

**Tóm tắt nguyên lý:** World không trực tiếp điều khiển object, nhưng cung cấp tốc độ hiện tại và trạng thái game. Đây là module quyết định độ khó theo điểm, setting và attack.

### 3.4 RF / Multiplayer

**Sequence diagram:**

```mermaid
sequenceDiagram
    autonumber
    participant Screen as scr_archery_game
    participant RF as ar_game_rf
    participant NRF as NRF24L01+
    participant World as ar_game_world
    participant Dino as ar_game_dino
    participant Objects as ar_game_objects
    participant BG as ar_game_background

    rect rgba(0, 180, 80, 0.12)
    Note over Screen, NRF: RF SETUP
    Screen->>RF: ar_game_rf_setup()
    activate RF
    RF->>RF: init_player_name()
    RF->>RF: lobby_state = 0
    RF->>RF: opponent_name[0] = '\0'
    RF->>NRF: rf_init_hardware_kit()
    RF->>NRF: rf_mode_rx()
    deactivate RF
    end

    rect rgba(255, 200, 0, 0.14)
    Note over Screen, NRF: SEND - Gửi lệnh lobby
    Screen->>RF: ar_game_rf_ready()
    activate RF
    alt lobby_state == 0
        RF->>RF: lobby_state = 1
        RF->>NRF: rf_send_cmd(CMD_READY)
    else lobby_state == 1
        RF->>RF: lobby_state = 3
        RF->>RF: opponent_name[0] = '\0'
        RF->>NRF: rf_send_cmd(CMD_START)
        RF->>RF: start_match()
    else lobby_state == 2
        RF->>RF: ar_game_rf_reject()
        RF->>NRF: rf_send_cmd(CMD_REJECT)
    end
    deactivate RF

    Screen->>RF: ar_game_rf_accept()
    activate RF
    alt ar_game_mp_state == AR_DINO_MP_WAITING && lobby_state == 2
        RF->>NRF: rf_send_cmd(CMD_ACCEPT)
        RF->>RF: start_match()
    end
    deactivate RF
    end

    rect rgba(0, 140, 255, 0.12)
    Note over Screen, World: RECEIVE - Nhận packet từ đối thủ
    Screen->>RF: ar_game_rf_poll()
    activate RF
    RF->>NRF: nRF24_RXPacket(rx_data, 5)
    alt có packet
        RF->>RF: cmd = rx_data[0]
        RF->>RF: rx_name = rx_data[1..3]
        alt ar_game_mp_state == AR_DINO_MP_WAITING
            RF->>RF: handle_waiting_packet(cmd, rx_name)
        else ar_game_mp_state == AR_DINO_MP_PLAYING
            RF->>RF: handle_playing_packet(cmd, rx_name)
        end
    end
    deactivate RF
    end

    rect rgba(255, 80, 80, 0.12)
    Note over RF, World: PLAYING COMMAND - Attack hoặc Win
    RF->>RF: packet_is_from_opponent(rx_name)
    alt cmd == CMD_ATTACK
        RF->>World: task_post_pure_msg(AR_GAME_WORLD_ID, AR_GAME_WORLD_ATTACK_BEGIN)
    else cmd == CMD_I_DIED
        RF->>World: task_post_pure_msg(AR_GAME_WORLD_ID, AR_GAME_WORLD_WIN)
    end
    end

    rect rgba(120, 120, 255, 0.10)
    Note over RF, BG: START MATCH - Reset module và bắt đầu trận
    RF->>RF: start_match()
    activate RF
    RF->>World: ar_game_world_reset()
    RF->>Dino: ar_game_dino_reset()
    RF->>Objects: ar_game_objects_reset()
    RF->>BG: ar_game_background_reset()
    RF->>RF: ar_game_mp_state = AR_DINO_MP_PLAYING
    deactivate RF
    end
```

**Tóm tắt nguyên lý:** RF quản lý cả lobby và command trong trận. Mỗi gói gửi 5 byte gồm command và tên người gửi để tránh nhận nhầm packet không thuộc phiên hiện tại.

### 3.5 Background

**Sequence diagram:**

```mermaid
sequenceDiagram
    autonumber
    participant Screen as scr_archery_game
    participant BG as ar_game_background
    participant OLED as view_render

    rect rgba(0, 180, 80, 0.12)
    Screen->>BG: ar_game_background_reset()
    BG->>BG: cloud.x = 120 * 10
    BG->>BG: cloud.y = 10
    BG->>BG: cloud.w/h = 16
    end

    rect rgba(0, 140, 255, 0.12)
    Screen->>BG: ar_game_background_update()
    BG->>BG: cloud.x -= AR_DINO_BG_SPEED_SCALED
    alt cloud leaves screen
        BG->>BG: cloud.x = random position after right edge
    end
    end

    rect rgba(120, 120, 255, 0.10)
    Screen->>BG: ar_game_background_render()
    BG->>OLED: drawBitmap(bitmap_cloud)
    end
```

**Tóm tắt nguyên lý:** Background chỉ xử lý cloud nền để game có chiều sâu, không ảnh hưởng collision.

### 3.6 Screen

**Sequence diagram:**

```mermaid
sequenceDiagram
    autonumber
    participant Player
    participant AK
    participant Screen as scr_archery_game
    participant RF as ar_game_rf
    participant World as ar_game_world
    participant Dino as ar_game_dino
    participant Objects as ar_game_objects
    participant BG as ar_game_background
    participant OLED as view_render

    rect rgba(0, 180, 80, 0.12)
    Note over AK, BG: SCREEN_ENTRY
    AK->>Screen: SCREEN_ENTRY
    activate Screen
    Screen->>Screen: eeprom_read(EEPROM_SETTING_START_ADDR, settingsetup)
    Screen->>Screen: ar_game_state = GAME_PLAY
    Screen->>Screen: ar_game_mp_state = AR_DINO_MP_WAITING
    Screen->>Screen: gameplay_tick_divider = 0
    Screen->>World: ar_game_world_reset()
    Screen->>Dino: ar_game_dino_reset()
    Screen->>Objects: ar_game_objects_reset()
    Screen->>BG: ar_game_background_reset()
    Screen->>RF: ar_game_rf_setup()
    Screen->>AK: timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT)
    deactivate Screen
    end

    rect rgba(0, 140, 255, 0.12)
    Note over AK, OLED: AR_GAME_TIME_TICK
    AK->>Screen: AR_GAME_TIME_TICK
    activate Screen
    Screen->>RF: ar_game_rf_poll()
    alt ar_game_mp_state == AR_DINO_MP_PLAYING
        Screen->>Screen: gameplay_tick_divider++
        alt gameplay_tick_divider >= 2
            Screen->>Screen: gameplay_tick_divider = 0
            Screen->>World: ar_game_world_update()
            Screen->>Dino: ar_game_dino_update()
            Screen->>BG: ar_game_background_update()
            Screen->>Objects: ar_game_objects_update()
            Screen->>Screen: view_scr_dino_game()
            Screen->>OLED: view_render.update()
        end
    else ar_game_mp_state != AR_DINO_MP_PLAYING
        Screen->>Screen: gameplay_tick_divider = 0
    end
    Screen->>AK: timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT)
    Screen->>Screen: SCREEN_NONE_UPDATE_MASK()
    deactivate Screen
    end

    rect rgba(255, 200, 0, 0.14)
    Note over Player, RF: BUTTON EVENT
    Player->>Screen: AC_DISPLAY_BUTTON_UP_PRESSED
    activate Screen
    alt ar_game_mp_state == AR_DINO_MP_PLAYING
        Screen->>Dino: ar_game_dino_jump()
    end
    Screen->>Screen: SCREEN_NONE_UPDATE_MASK()
    deactivate Screen

    Player->>Screen: AC_DISPLAY_BUTTON_UP_RELEASED
    activate Screen
    alt ar_game_mp_state == AR_DINO_MP_WAITING
        Screen->>RF: ar_game_rf_accept()
    else đang chơi
        Screen->>Screen: SCREEN_NONE_UPDATE_MASK()
    end
    deactivate Screen

    Player->>Screen: AC_DISPLAY_BUTTON_DOWN_RELEASED
    activate Screen
    alt ar_game_mp_state == AR_DINO_MP_WAITING
        Screen->>RF: ar_game_rf_ready()
    else đang chơi
        Screen->>Screen: SCREEN_NONE_UPDATE_MASK()
    end
    deactivate Screen
    end

    rect rgba(180, 180, 180, 0.12)
    Note over Player, Screen: EXIT
    Player->>Screen: AC_DISPLAY_BUTTON_MODE_RELEASED
    activate Screen
    Screen->>Screen: ar_game_state = GAME_OFF
    Screen->>AK: timer_remove_attr(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK)
    Screen->>Screen: SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game)
    deactivate Screen
    end
```

**Tóm tắt nguyên lý:** Screen là nơi nối các module lại với nhau. Screen không giữ logic vật lý, spawn hay RF packet; nó chỉ gọi đúng module theo đúng thời điểm.

## IV. Cấu trúc dữ liệu

Các struct chính được đặt trong `ar_game_common.h`.

```cpp
typedef struct {
    int16_t y;
    int16_t v_y;
    bool is_jumping;
    bool is_ducking;
} ar_game_dino_t;

typedef struct {
    int32_t x;
    int16_t y;
    uint8_t w;
    uint8_t h;
    uint8_t type;
    bool active;
} ar_game_object_t;

typedef struct {
    int32_t x;
    int16_t y;
    uint8_t w;
    uint8_t h;
} ar_game_bg_t;
```

| Biến | Module | Chức năng |
|---|---|---|
| `dino` | `ar_game_dino` | Trạng thái Dino hiện tại. |
| `ar_game_objects[4]` | `ar_game_objects` | Danh sách Cactus, Bird, Gift. |
| `ar_game_cloud` | `ar_game_background` | Mây nền. |
| `ar_game_score` | `ar_game_world` | Điểm hiện tại. |
| `ar_game_current_speed` | `ar_game_world` | Tốc độ chạy hiện tại. |

## V. Luồng code chính

### 5.1 Screen Entry

`scr_archery_game.cpp` chịu trách nhiệm khởi tạo màn chơi.

```cpp
case SCREEN_ENTRY: {
    eeprom_read(EEPROM_SETTING_START_ADDR, (uint8_t*)&settingsetup, sizeof(settingsetup));
    ar_game_state = GAME_PLAY;
    ar_game_mp_state = AR_DINO_MP_WAITING;
    gameplay_tick_divider = 0;

    ar_game_world_reset();
    ar_game_dino_reset();
    ar_game_objects_reset();
    ar_game_background_reset();
    ar_game_rf_setup();

    timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT);
}
break;
```

### 5.2 Gameplay Tick

Timer vẫn chạy 10ms để RF phản hồi nhanh, nhưng gameplay được chia nhịp để tốc độ và trọng lực không quá nhanh.

```cpp
case AR_GAME_TIME_TICK: {
    ar_game_rf_poll();

    if (ar_game_mp_state == AR_DINO_MP_PLAYING) {
        gameplay_tick_divider++;
        if (gameplay_tick_divider >= 2) {
            gameplay_tick_divider = 0;

            ar_game_world_update();
            ar_game_dino_update();
            ar_game_background_update();
            ar_game_objects_update();

            view_scr_dino_game();
            view_render.update();
        }
    }

    timer_set(AC_TASK_DISPLAY_ID, AR_GAME_TIME_TICK, 10, TIMER_ONE_SHOT);
    SCREEN_NONE_UPDATE_MASK();
}
break;
```

### 5.3 Dino Physics và Hitbox

```cpp
void ar_game_dino_update() {
    dino.is_ducking = (btn_down.state == BUTTON_SW_STATE_PRESSED);

    if (dino.is_jumping) {
        dino.y += dino.v_y;
        dino.v_y += AR_DINO_GRAVITY_SCALED;

        if (dino.y >= (AR_DINO_GROUND_Y_SCALED - (AR_DINO_H * 10))) {
            dino.y = AR_DINO_GROUND_Y_SCALED - (AR_DINO_H * 10);
            dino.is_jumping = false;
            dino.v_y = 0;
        }
    }
}

bool ar_game_dino_hit_test(const ar_game_object_t* obj) {
    int16_t dy = dino.y / 10;
    int16_t dino_hit_y = dy;
    int16_t dino_hit_h = AR_DINO_H;
    int16_t obj_x = obj->x / 10;

    if (dino.is_ducking) {
        dino_hit_y = dy + 6;
        dino_hit_h = 10;
    }

    bool hit_x = (AR_DINO_X + AR_DINO_W - 4 > obj_x) &&
                 (AR_DINO_X + 2 < obj_x + obj->w);
    bool hit_y = (dino_hit_y + dino_hit_h > obj->y + 2) &&
                 (dino_hit_y + 2 < obj->y + obj->h);

    return hit_x && hit_y;
}
```

### 5.4 Tăng độ khó

`ar_game_world_update()` tính tốc độ hiện tại dựa trên setting và điểm số.

```cpp
void ar_game_world_update() {
    int32_t start_speed_bonus = (settingsetup.num_arrow - 1) * AR_DINO_SETTING_SPEED_STEP;
    int32_t base_speed = AR_DINO_BASE_SPEED_SCALED + start_speed_bonus;
    uint8_t next_speed_level = ar_game_score / AR_DINO_SCORE_STEP;

    if (next_speed_level > speed_level) {
        speed_level = next_speed_level;
        speed_up_notice_timer = 90;
    }

    base_speed += (ar_game_score / AR_DINO_SCORE_STEP) * AR_DINO_SPEED_STEP;
    if (base_speed > AR_DINO_MAX_BASE_SPEED) {
        base_speed = AR_DINO_MAX_BASE_SPEED;
    }

    ar_game_current_speed = base_speed;
}
```

Cơ chế hiện tại:

| Mốc điểm | Ảnh hưởng |
|---|---|
| Mỗi 15 điểm | Tăng speed level và hiện `SPD UP`. |
| Điểm càng cao | Khoảng cách vật cản càng ngắn. |
| Điểm càng cao | Bird xuất hiện nhiều hơn. |
| Bị tấn công | Tăng tốc tạm thời và hiện `SPEED UP!`. |

### 5.5 RF Multiplayer

Mỗi gói RF gồm command và tên người gửi.

```cpp
static void rf_send_cmd(uint8_t cmd) {
    static uint8_t tx_buf[5];
    tx_buf[0] = cmd;
    tx_buf[1] = my_name[0];
    tx_buf[2] = my_name[1];
    tx_buf[3] = my_name[2];
    tx_buf[4] = '\0';

    nRF24_TXMode(5, 15, current_rf_channel, nRF24_DataRate_1Mbps,
                 nRF24_TXPower_0dBm, nRF24_CRC_2byte,
                 nRF24_PWR_Up, RF_ADDR, 5);
    nRF24_TXPacket(tx_buf, 5);
    rf_mode_rx();
}
```

Các command chính:

| Command | Ý nghĩa |
|---|---|
| `CMD_READY` | Gửi lời mời/chờ đối thủ. |
| `CMD_ACCEPT` | Chấp nhận lời mời. |
| `CMD_REJECT` | Từ chối lời mời. |
| `CMD_START` | Bắt đầu solo/local start. |
| `CMD_ATTACK` | Gift attack, ép đối thủ speed up. |
| `CMD_I_DIED` | Báo mình đã thua. |

## VI. Hiển thị và âm thanh

### 6.1 Bitmap

Bitmap được lưu trong `screens_bitmap.cpp` dưới dạng mảng `PROGMEM`.

| Bitmap | Kích thước | Chức năng |
|---|---:|---|
| `bitmap_dino` | 16x16 | Dino đứng/chạy/nhảy. |
| `bitmap_dino_duck` | 16x16 | Dino cúi. |
| `bitmap_cactus` | 16x16 | Xương rồng. |
| `bitmap_bird` | 16x8 | Chim. |
| `bitmap_gift` | 8x8 | Hộp quà. |
| `bitmap_cloud` | 16x16 | Mây nền. |

### 6.2 Render gameplay

Screen gọi các module render theo thứ tự cố định.

```cpp
static void render_gameplay() {
    ar_game_world_render_hud();
    ar_game_background_render();
    ar_game_objects_render();
    ar_game_dino_render();
    ar_game_world_render_attack_warning();
}

void view_scr_dino_game() {
    view_render.clear();

    if (ar_game_mp_state == AR_DINO_MP_PLAYING) {
        render_gameplay();
    }
    else {
        ar_game_rf_render_lobby();
    }
}
```

### 6.3 Âm thanh

| Tone | Sử dụng |
|---|---|
| `tones_cc` | Ready, nhặt Gift, xác nhận thao tác. |
| `tones_startup` | Bắt đầu game hoặc bị attack. |
| `tones_3beep` | Game Over hoặc reject. |

## VII. Build và nạp firmware

Build application:

```bash
cd application
make all
```

File firmware sau build:

```text
application/build_ak-base-kit-stm32l151-application/ak-base-kit-stm32l151-application.bin
```

Nạp qua bootloader AK:

```bash
make flash dev=/dev/ttyUSB0
```

Nạp qua ST-Link:

```bash
make flash
```

## VIII. Ghi chú triển khai

- Game giữ public screen name `scr_archery_game` để tương thích với Menu, Game Over và Charts.
- Menu không bị thay đổi trong refactor.
- `SCREEN_NONE_UPDATE_MASK()` được dùng để tránh screen manager tự render lại sau các message nút, giúp spam nút nhảy không làm tụt FPS.
- RF vẫn poll mỗi 10ms, còn gameplay physics chạy theo nhịp chia 20ms để tốc độ và trọng lực ổn định hơn trên kit thật.
