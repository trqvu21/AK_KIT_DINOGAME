# Multiplayer Dino Game - Build on AK Embedded Base Kit

<center><img src="[https://github.com/ak-embedded-software/archery-game/blob/main/resources/images/epcb_archery_game.webp](https://github.com/ak-embedded-software/archery-game/blob/main/resources/images/epcb_archery_game.webp)" alt="Dino game banner" width="100%"/></center>

<hr>

## I. Giới thiệu

**Multiplayer Dino Game** là một tựa game sinh tồn màn hình ngang vô tận (Endless Runner) chạy trên nền tảng hệ điều hành thời gian thực (RTOS) của AK Embedded Base Kit. 

Khác với phiên bản game Khủng long Offline truyền thống trên trình duyệt, dự án này mang đến một bước đột phá: **Tương tác hai người chơi (Multiplayer) theo thời gian thực**. Thông qua module vô tuyến NRF24L01+, hai board mạch độc lập có thể đồng bộ trạng thái game và cho phép người chơi "tấn công" lẫn nhau bằng cách gửi các hiệu ứng trừng phạt qua sóng RF.

Quá trình phát triển tựa game này là một bài thực hành thực tế sâu sắc về Lập trình Hướng sự kiện (Event-Driven), Quản lý bộ nhớ (Object Pooling), Tối ưu hóa đồ họa 2D (Hitbox & Animation) và xử lý độ trễ trong truyền thông không dây.

### 1.1 Phần cứng

<p align="center"><img src="[https://github.com/ak-embedded-software/archery-game/blob/main/resources/images/AK_Embedded_Base_Kit_STM32L151.webp](https://github.com/ak-embedded-software/archery-game/blob/main/resources/images/AK_Embedded_Base_Kit_STM32L151.webp)" alt="AK Embedded Base Kit - STM32L151" width="480"/></p>
<p align="center"><strong><em>Hình 1:</em></strong> AK Embedded Base Kit - STM32L151</p>

[AK Embedded Base Kit](https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu) là một evaluation kit dành cho các bạn học phần mềm nhúng nâng cao. Trong dự án này, hệ thống khai thác các phần cứng cốt lõi sau:
* **LCD OLED 1.3":** Hiển thị không gian trò chơi với tốc độ quét (frame rate) cực cao nhờ giao tiếp I2C/SPI được tối ưu hóa.
* **Nút nhấn (Buttons):** Sử dụng `btn_up` và `btn_down` để thực hiện các thao tác cơ động.
* **Còi Buzzer:** Phát âm thanh cảnh báo, ăn điểm và hiệu ứng Game Over (Non-blocking).
* **Module NRF24L01+:** Kênh truyền nhận dữ liệu tần số 2.4GHz để đồng bộ trạng thái giữa 2 máy chơi game.

### 1.2 Mô tả trò chơi và Đối tượng
Phần mô tả sau giải thích cách chơi, luật chơi và cơ chế xử lý nội bộ của hệ thống Multiplayer Dino.

#### 1.2.1 Các đối tượng (Object) trong game:
Để tối ưu hóa RAM của vi điều khiển, game áp dụng mô hình **Entity Management (Quản lý Thực thể)**.

| Đối tượng | Tên biến / Struct | Loại | Mô tả |
| :--- | :--- | :--- | :--- |
| **Khủng long** | `dino_t dino` | Người chơi | Nhân vật chính. Có thể chạy, nhảy lên cao (tránh Xương rồng/Chim bay thấp) hoặc cúi gập người (tránh Chim bay cao). |
| **Xương rồng** | `game_obj_t objects[]` | Chướng ngại vật | Nằm sát mặt đất. Chiều cao 16px. Người chơi bắt buộc phải nhảy qua. Chạm vào sẽ Game Over. |
| **Chim Dực long** | `game_obj_t objects[]` | Chướng ngại vật | Bay lơ lửng ở 2 tầm cao ngẫu nhiên. **Bay thấp:** Bắt buộc nhảy qua. **Bay cao:** Bắt buộc phải cúi người né. |
| **Hộp quà** | `game_obj_t objects[]` | Vật phẩm (Item)| Lơ lửng trên không. Khi mạo hiểm nhảy ăn được, kích hoạt kỹ năng "Tấn công" gửi qua sóng RF. |
| **Đám mây** | `bg_obj_t bgs[]` | Cảnh nền | Trôi liên tục ở phía sau với tốc độ chậm (Speed = 10) tạo hiệu ứng chiều sâu (Parallax Scrolling). |

#### 1.2.2 Luật chơi và Cách chơi:
1. **Khởi động & Đồng bộ:** Hai người chơi bật máy cùng lúc. Board mạch được cấu hình là `MASTER` sẽ hiển thị "PRESS DOWN TO START". Người chơi nhấn nút **[Down]**, máy Master sẽ phát sóng RF lệnh `CMD_START` để cả 2 board bắt đầu game vào cùng 1 mili-giây.
2. **Thao tác điều khiển:**
   * **Nhảy lên (Jump):** Nhấn nút **[Up]**. Dùng để né Xương rồng và Chim bay thấp. Khủng long chịu tác động của trọng lực ảo (Gravity) nên sẽ rơi xuống theo quỹ đạo Parabol.
   * **Cúi người (Duck):** Nhấn giữ nút **[Down]**. Khủng long sẽ gập người xuống sát đất, thu nhỏ một nửa hộp va chạm (Hitbox) để né Chim bay cao. Nhả nút ra Khủng long sẽ đứng thẳng lại.
3. **Mục tiêu:** Sinh tồn lâu nhất có thể, đồng thời cố gắng thu thập **Hộp quà** để tiêu diệt đối thủ.

#### 1.2.3 Cơ chế hoạt động (Game Mechanics):
* **Hệ thống Tính điểm (Scoring):**
  * Vượt qua thành công 1 Xương rồng hoặc 1 Chim Dực long: **+1 Điểm**.
  * Thu thập được Hộp quà: **+5 Điểm thưởng**.
* **Hệ thống Độ khó (Dynamic Speed Scaling):**
  * Tốc độ di chuyển nền khởi điểm là `35`.
  * Cứ mỗi **15 điểm** tích lũy, tốc độ game được cộng thêm `5` đơn vị.
  * Mức trần (Speed Cap) được khóa ở mức `60` để ngăn hiện tượng "Xuyên thấu Box" (Tunneling Effect) do vật thể trượt đi quá số pixel chiều rộng của nó trong 1 chu kỳ vẽ.
* **Cơ chế Multiplayer - Ném tạ (Attack System):**
  * Khi Máy A điều khiển Khủng long ăn được **Hộp quà**, nó lập tức gọi hàm `rf_send_cmd(CMD_ATTACK)` sang Máy B.
  * Khi Máy B nhận được lệnh này, còi báo động khẩn cấp (`tones_startup`) sẽ vang lên. Màn hình Máy B chớp nháy dòng chữ **"SPEED UP!"**. Toàn bộ chướng ngại vật trên Máy B sẽ lao đến với **Siêu Tốc Độ** (Tốc độ cơ bản hiện tại + `20`) trong thời gian 3 giây (300 Time Ticks).
* **Kết thúc (Game Over):**
  * Khi Khủng long chạm vào chướng ngại vật, hệ thống gửi lệnh `CMD_I_DIED` thông báo "Báo Tử" sang máy đối thủ.
  * Máy chạm chướng ngại vật sẽ chuyển sang `MP_LOSE` (YOU LOSE), máy đối thủ nhận được tin báo tử sẽ tự động chuyển sang `MP_WIN` (YOU WIN).

<hr>

## II. Thiết kế Hệ thống & Kiến trúc (Architecture Design)
### 2.1 Môi trường Event-Driven (AK OS)
Trò chơi được xây dựng dựa trên nguyên lý Event-driven (Hệ thống hướng sự kiện). Các sự kiện (phím bấm, đếm thời gian, nhận sóng RF) được gọi là các **Signal** và ném vào hàng đợi (Message Queue) để **Task** tuần tự lấy ra xử lý.

### 2.2 Tối ưu hóa: Bài toán Tràn bộ nhớ (MF 31 - Queue Overflow)
Trong giai đoạn đầu phát triển, game gặp lỗi chí mạng `MF 31` (Memory Fault - cạn kiệt Pool Message).
* **Nguyên nhân:** Việc dùng `TIMER_PERIODIC` (Đồng hồ chu kỳ 10ms) liên tục ném sự kiện đếm giờ vào Queue. Tuy nhiên, màn hình OLED cần tới ~35ms để render 1 khung hình. CPU bị nghẽn ở hàm render, dẫn đến Timer liên tục nhồi lệnh vào Queue gây tràn bộ nhớ. Thao tác "Spam" phím bấm của người chơi càng làm Queue sụp đổ nhanh hơn.
* **Giải pháp - Kỹ thuật `TIMER_ONE_SHOT`:**
  Trò chơi chuyển sang chiến lược đồng bộ Frame-by-Frame. Bộ đếm `TIMER_ONE_SHOT` chỉ được đặt 1 lần. CPU sẽ thoải mái tính toán logic, nhận sóng RF, đọc phím và vẽ toàn bộ ra màn hình OLED. **Chỉ khi nào hoàn tất 100%**, CPU mới gọi lệnh tái thiết lập (Set Timer) cho chu kỳ tiếp theo.
  Hàng đợi hệ điều hành luôn được giữ trống trải, triệt tiêu hoàn toàn lỗi Crash `MF 31` dù chơi ở tốc độ cao nhất.

### 2.3 Sơ đồ Máy Trạng Thái (State Machine)
Biến `mp_state` quản lý luồng điều hướng của game:
```text
[MP_WAITING] ----- (Nhận nút DOWN / RF báo START) -----> [MP_PLAYING]
      ^                                                        |
      |                                                        |
      +----- (Nhận nút DOWN / Chết / Đối thủ chết) <-----------+
                 [MP_LOSE]    hoặc    [MP_WIN]
```

<hr>

## III. Phân tích Thuật toán Cốt lõi (Core Algorithms)

### 3.1 Thuật toán Va chạm với Hitbox Động (Dynamic AABB Collision)
Thuật toán AABB (Axis-Aligned Bounding Box) được dùng để xét va chạm hình chữ nhật. Điểm đặc biệt trong game là hộp va chạm của Khủng long (Hitbox) có thể **biến hình theo thời gian thực** dựa trên trạng thái `is_ducking` (cúi người).

```cpp
int16_t dy = dino.y / 10;          // Tọa độ Y hiện tại
int16_t dino_hit_y = dy;           // Y bắt đầu của Hitbox
int16_t dino_hit_h = DINO_H;       // Chiều cao Hitbox (Mặc định 16px)

// Khi người chơi nhấn phím DOWN
if (dino.is_ducking) {
    dino_hit_y = dy + 6;           // Đẩy đỉnh đầu của Hitbox tụt xuống 6px
    dino_hit_h = 10;               // Ép chiều cao Hitbox xuống còn 10px
}

// Tính toán giao cắt trục X và Y
bool hit_x = (DINO_X + DINO_W - 4 > cx) && (DINO_X + 2 < cx + objects[i].w);
bool hit_y = (dino_hit_y + dino_hit_h > objects[i].y + 2) && (dino_hit_y + 2 < objects[i].y + objects[i].h);

if (hit_x && hit_y) {
    // Kích hoạt Game Over hoặc Ăn điểm thưởng
}
```
*Nhờ cơ chế này, nửa thân trên của khung hình được xem như "vô hình", giúp Khủng long có thể luồn lách qua con Chim đang bay ở độ cao (Y = -20) một cách hoàn hảo mà chân vẫn chạm đất.*

### 3.2 Hệ thống Quản lý Đối tượng (Object Pooling & Recycling)
Khởi tạo động (`malloc/free`) trong RTOS cho game tốc độ cao sẽ gây phân mảnh và lag. Dự án giải quyết bằng **Hồ chứa đối tượng (Object Pool)** tĩnh:
```cpp
static game_obj_t objects[4]; // 4 vật thể được dùng lại vĩnh viễn
```
Thuật toán cuốn chiếu (Recycling): Khi vật thể thứ `i` trượt ra khỏi mép trái màn hình (`cx < -objects[i].w`), hệ thống sẽ không xóa nó, mà "bốc" nó lên và thả vào tọa độ X xa nhất ở mép phải màn hình, đồng thời Random lại bản chất của nó (Tỷ lệ: 20% Quà, 25% Chim, 55% Xương rồng).

### 3.3 Giao thức Đa người chơi qua nRF24L01+
Sử dụng hàm API của hệ thống để truyền/nhận lệnh 1 byte (1Mbps, Channel 40).
* `CMD_START (1)`: Bắt đầu game đồng bộ.
* `CMD_I_DIED (2)`: Báo tử.
* `CMD_ATTACK (3)`: Nhận lệnh tấn công. Biến `attack_timer` lập tức được bơm đầy 300 chu kỳ, kích hoạt còi hú và tăng đột biến biến `current_speed`.

<hr>

## IV. Đồ họa và Âm thanh
### 4.1 Đồ họa (Graphics & Bitmaps)
Do tài nguyên RAM hạn chế, tất cả dữ liệu hình ảnh được lưu thẳng vào bộ nhớ ROM (Flash) dưới dạng mảng hằng số Hex `PROGMEM`.

| Nguồn tĩnh (PROGMEM) | Kích thước | Chức năng trong game |
| :--- | :--- | :--- |
| `bitmap_dino` | 16x16 px | Khủng long tư thế chạy bình thường |
| `bitmap_dino_duck` | 16x16 px | Khủng long tư thế nằm sấp (Đã tính toán bù Y) |
| `bitmap_cactus` | 8x16 px | Chướng ngại vật sát đất |
| `bitmap_bird` | 16x8 px | Chướng ngại vật bay trên không |
| `bitmap_gift` | 8x8 px | Vật phẩm Kỹ năng |
| `bitmap_cloud` | 16x8 px | Nền đồ họa tạo chiều sâu |

Hàm `view_render.drawBitmap()` được tối ưu để chỉ vẽ những vật thể có cờ `active = true` nằm trong mảng `objects[]`.

### 4.2 Âm thanh (Audio Feedback)
Âm thanh được phát thông qua bộ còi Buzzer của phần cứng theo phương thức Non-blocking (Không chặn CPU) để không làm mất FPS của game.
* **Còi ăn quà (`tones_cc`):** 1 tiếng bíp ngắn. Báo hiệu thành công nhặt được Hộp quà.
* **Còi báo động (`tones_startup`):** Chuỗi âm thanh réo rắt liên tục. Phản hồi cho người chơi biết họ vừa bị đối thủ sử dụng kỹ năng Tăng tốc (Speed Up).
* **Còi tử trận (`tones_3beep`):** 3 tiếng trầm ngắt quãng, báo hiệu Game Over do đụng vật cản.

---
