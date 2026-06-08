# Multiplayer Dino Game - Build on AK Embedded Base Kit

<hr>

## I. Introduction

Multiplayer Dino Game is a game running on the **AK Embedded Base Kit**. The project is built to practice embedded programming following the **event-driven** model, utilizing Tasks, Signals, Timers, Messages, State Machines, OLED, Buttons, Buzzer, and **NRF24L01+** wireless communication.

The game is inspired by the Dino runner: the player controls a dinosaur to dodge obstacles, collect gift boxes to attack opponents, and strive to achieve the highest score.

### 1.1 Hardware

<p align="center"><img src="resources/images/AK_Embedded_Base_Kit_STM32L151.webp" alt="AK Embedded Base Kit - STM32L151" width="480"/></p>
<p align="center"><strong><em>Figure 1:</em></strong> AK Embedded Base Kit - STM32L151</p>

[AK Embedded Base Kit](https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu) is an evaluation kit for those studying advanced embedded software.

The KIT integrates a **1.3" OLED** LCD, **3 push buttons**, **Buzzer**, **NRF24L01+**, RS485, and external Flash. In this project, the OLED is used to display the game, buttons are used for control, the Buzzer provides audio feedback, and the NRF24L01+ is used for the multiplayer mode.

### 1.2 Game Description

<p align="center"><img src="resources/images/menudino.webp" alt="AK Embedded Base Kit - STM32L151" width="480"/></p>
<p align="center"><strong><em>Figure 2:</em></strong> Game menu screen</p>

#### 1.2.1 In-game Objects

| Object | Name | Description |
|---|---|---|
| Dinosaur | Dino | The main character, can jump or duck. |
| Cactus | Cactus | Ground-level obstacle, must be jumped over. |
| Bird | Bird | Flying obstacle at high/low levels, requires jumping or ducking to dodge. |
| Gift Box | Gift | Special item, collecting it grants points and sends an attack via RF. |
| Cloud | Cloud | Background element to give the level depth. |

#### 1.2.2 Gameplay

- Upon entering the game, the device displays the `DINO ROOM` lobby and a random name format like `[P73]`.
- Two kits selecting the same room will see each other's IDs in the lobby.
- Press `DOWN` to change the current kit's status to `READY`.
- If there is only one kit in the room after getting Ready, the screen shows `BTN DOWN PLAY SOLO`; press `DOWN` again to play single-player.
- When both kits are `READY`, both screens display `STARTING` and then the game begins.
- During gameplay, press `UP` to make Dino jump, hold `DOWN` to make Dino duck.
- The game ends when Dino collides with a Cactus or Bird; the first kit to die shows `YOU LOSE`, the remaining kit shows `YOU WIN`.

#### 1.2.3 Scoring and Difficulty Mechanics

- Passing a Cactus or Bird: `+1` point.
- Collecting a Gift: `+5` points and sends the `CMD_ATTACK` command to the opponent.
- Every `15` points, the game increases one speed level and displays an `SPD UP` notification.
- At higher scores, the speed increases significantly, the distance between obstacles shortens, and Birds appear more frequently.
- When attacked, the receiving device will display `SPEED UP!`, play a warning sound, and temporarily increase speed for a short duration.

## II. Event-driven Design

**Event-driven concepts:**

- **Event Driven:** The system sends messages to trigger behaviors. Tasks act as receivers, and Signals represent the content of the work.
- **Task:** A processing unit for a specific group of tasks. When the scheduler retrieves a task's message, the corresponding handler is called.
- **Message:** An event packet placed into the queue. A message can contain only a Signal or both a Signal and Data.
- **Signal:** The identifier of the action to be processed, e.g., `SCREEN_ENTRY`, `AR_GAME_TIME_TICK`, `AR_GAME_RF_SEND_ATTACK`.
- **Handler:** The function that processes messages/signals of a task or module.

### 2.1 Architectural Objectives

| Module | Role |
|---|---|
| `ar_game_dino` | Manages Dino, jumping, ducking, gravity, and hitboxes. |
| `ar_game_objects` | Manages Cactus, Bird, Gift, spawning, recycling, and collisions. |
| `ar_game_background` | Manages clouds and the background. |
| `ar_game_world` | Manages score, speed, level, win/lose states, and speed up effects. |
| `ar_game_rf` | Manages NRF24L01+, room lobby, hello/ready/starting, attack, and died commands. |
| `scr_dino_game` | Coordinates Screen Entry, Timer Tick, Button Events, and Rendering. |

### 2.2 Sequence Diagram
The **Sequence Diagram** is used to describe the sequence of Messages and the interaction flow between objects in a system.

<p align="center"><img src="resources/images/fnsq.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 3:</em></strong> The sequence diagram</p>

### 2.3 Main Messages and Signals

| Group | Signal / Event | Description |
|---|---|---|
| Screen | `SCREEN_ENTRY` | Initializes the game, reads settings, resets modules, and sets up RF. |
| Screen | `AR_GAME_TIME_TICK` | 10ms tick, polls RF, and divides gameplay pacing. |
| Button | `AC_DISPLAY_BUTTON_UP_PRESSED` | Jump during gameplay. |
| Button | `AC_DISPLAY_BUTTON_UP_RELEASED` | Not used in the new lobby, updates mask only. |
| Button | `AC_DISPLAY_BUTTON_DOWN_RELEASED` | Ready in the waiting room. |
| RF | `CMD_HELLO`, `CMD_READY`, `CMD_STARTING` | Displays ID in the room, syncs Ready, and Starting countdown. |
| RF | `CMD_ATTACK`, `CMD_I_DIED` | Attacks and reports the end of the match. |

### 2.4 Tasks

In the code, the game still reuses the old Task IDs from the previous project to avoid increasing the total number of tasks in the system. The task names have been changed according to the new Dino roles.

| Task ID | Handler | Module | Role |
|---|---|---|---|
| `AR_GAME_BACKGROUND_ID` | `ar_game_background_handle` | `ar_game_background` | Setup/reset/update background. |
| `AR_GAME_WORLD_ID` | `ar_game_world_handle` | `ar_game_world` | Score, speed, level, win/lose, attack timer. |
| `AR_GAME_DINO_ID` | `ar_game_dino_handle` | `ar_game_dino` | Dino physics, jump, hitbox. |
| `AR_GAME_OBJECTS_ID` | `ar_game_objects_handle` | `ar_game_objects` | Cactus/Bird/Gift movement, spawn, collision. |
| `AR_GAME_RF_ID` | `ar_game_rf_handle` | `ar_game_rf` | NRF24 commands, lobby, attack/died. |
| `AR_GAME_SCREEN_ID` | `scr_dino_game_handle` | `scr_dino_game` | Screen events, timer tick, button dispatch, render frame. |

**Performance Note:** During gameplay, the main update functions are called directly from `scr_dino_game` to avoid message queue overhead and prevent FPS drops when buttons are spammed. The task handlers are retained for setup/reset/RF commands and to keep the event-driven architecture clear.

### 2.5 Signals by Module

| Module | Signal | Description |
|---|---|---|
| Screen | `SCREEN_ENTRY` | Reads settings, resets modules, setups RF, enables timer tick. |
| Screen | `AR_GAME_TIME_TICK` | Polls RF, divides gameplay pacing, updates and renders frames. |
| Dino | `AR_GAME_DINO_SETUP` | Resets Dino to the ground position. |
| Dino | `AR_GAME_DINO_UPDATE` | Updates ducking, jumping, gravity. |
| Dino | `AR_GAME_DINO_JUMP` | Initiates jump if Dino is on the ground. |
| Dino | `AR_GAME_DINO_RESET` | Resets Dino after start/restart. |
| Objects | `AR_GAME_OBJECTS_SETUP` | Creates the initial object array. |
| Objects | `AR_GAME_OBJECTS_UPDATE` | Moves, recycles, checks for collisions. |
| Objects | `AR_GAME_OBJECTS_RESET` | Resets all objects. |
| World | `AR_GAME_WORLD_UPDATE` | Calculates speed, level, notification timers. |
| World | `AR_GAME_WORLD_ATTACK_BEGIN` | Starts the attacked effect. |
| World | `AR_GAME_WORLD_LOSE` | Transitions to lose state and moves to Game Over. |
| World | `AR_GAME_WORLD_WIN` | Transitions to win state and moves to Game Over. |
| Background | `AR_GAME_BACKGROUND_UPDATE` | Moves background clouds. |
| RF | `AR_GAME_RF_SETUP` | Initializes player name and NRF24. |
| RF | `AR_GAME_RF_POLL` | Reads RF packets if available. |
| RF | `AR_GAME_RF_READY` | Sets `local_ready`, sends `CMD_READY`, starts Starting if 2 Readys are met. |
| RF | `AR_GAME_RF_ACCEPT` | Kept for compatibility, now calls the same logic as `ar_game_rf_ready()`. |
| RF | `AR_GAME_RF_SEND_ATTACK` | Sends `CMD_ATTACK`. |
| RF | `AR_GAME_RF_SEND_DIED` | Sends `CMD_I_DIED`. |

### 2.6 Bitmaps and Resources

| Bitmap | File | Size | Function |
|---|---|---:|---|
| `bitmap_dino` | `screens_bitmap.cpp` | 16x16 | Dino standing/running/jumping. |
| `bitmap_dino_duck` | `screens_bitmap.cpp` | 16x16 | Dino ducking. |
| `bitmap_cactus` | `screens_bitmap.cpp` | 16x16 | Cactus. |
| `bitmap_bird` | `screens_bitmap.cpp` | 16x8 | Bird. |
| `bitmap_gift` | `screens_bitmap.cpp` | 8x8 | Gift attack. |
| `bitmap_cloud` | `screens_bitmap.cpp` | 16x16 | Cloud background. |

## III. Detailed Sequence for Each Object

### 3.1 Dino

<p align="center"><img src="resources/images/dino.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 4:</em></strong> Dino sequence diagram</p>

**Principle Summary:** Dino receives the jump action from button events, ducking state from `btn_down.state`, self-updates gravity according to the gameplay pacing, and provides a hitbox function for the Objects module.

### 3.2 Objects: Cactus, Bird, Gift

<p align="center"><img src="resources/images/obj.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 5:</em></strong> Objects sequence diagram</p>

**Principle Summary:** Objects are responsible for creating the main gameplay rhythm: moving obstacles, recycling obstacles, increasing scores, checking collisions, and sending events to World/RF.

### 3.3 World

<p align="center"><img src="resources/images/world.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 6:</em></strong> World sequence diagram</p>

**Principle Summary:** World does not directly control objects but provides the current speed and game state. It is the module that determines difficulty based on scores, settings, and attacks.

### 3.4 RF / Multiplayer

<p align="center"><img src="resources/images/rf.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 7:</em></strong> RF sequence diagram</p>

**Principle Summary:** RF manages both the lobby and in-match commands. Each sent packet consists of 5 bytes including the command and the sender's name to avoid receiving incorrect packets not belonging to the current session.

### 3.5 Background

<p align="center"><img src="resources/images/BG.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 8:</em></strong> Background sequence diagram</p>

**Principle Summary:** Background only processes background clouds to give the game depth and does not affect collisions.

### 3.6 Screen

<p align="center"><img src="resources/images/SCR.webp" alt="AK Embedded Base Kit - STM32L151" width="720"/></p>
<p align="center"><strong><em>Figure 9:</em></strong> Screen sequence diagram</p>

**Principle Summary:** Screen is where all modules connect together. Screen does not hold physical logic, spawning, or RF packets; it only calls the correct modules at the correct times.

## IV. Data Structures

The main structs are located in `ar_game_common.h`.

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

| Variable | Module | Function |
|---|---|---|
| `dino` | `ar_game_dino` | Current Dino state. |
| `ar_game_objects[4]` | `ar_game_objects` | List of Cactus, Bird, Gift. |
| `ar_game_cloud` | `ar_game_background` | Background cloud. |
| `ar_game_score` | `ar_game_world` | Current score. |
| `ar_game_current_speed` | `ar_game_world` | Current running speed. |

## V. Main Code Flow

### 5.1 Screen Entry

`scr_dino_game.cpp` is responsible for initializing the game screen.

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

### 5.3 Dino Physics and Hitbox

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

### 5.4 Difficulty

`ar_game_world_update()` calculates the current speed based on settings and score

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

Current mechanics:

| Score | Effect |
|---|---|
| Every 15 points | Increases speed level and shows SPD UP. |
| Higher scores| Shorter distance between obstacles. |
| Higher scores| Birds appear more frequently. |
| Been attacked | Temporarily increases speed and shows `SPEED UP!`. |

### 5.5 RF Multiplayer

Each RF packet includes the command and the sender's name.

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

Main commands:

| Command | Function |
|---|---|
| `CMD_HELLO` | Broadcasts ID so kits in the same room can see each other. |
| `CMD_READY` | Indicates the current kit is Ready. |
| `CMD_STARTING` | Syncs the Starting screen before gameplay begins. |
| `CMD_START` | Old command still received for compatibility. |
| `CMD_ATTACK` | Gift attack, forces the opponent to speed up. |
| `CMD_I_DIED` | Reports that the player has lost. |

## VI. Display and Audio

### 6.1 Bitmap

Bitmaps are stored in `screens_bitmap.cpp` as`PROGMEM`arrays.

| Bitmap | Size | Function |
|---|---:|---|
| `bitmap_dino` | 16x16 | Dino standing/running/jumping. |
| `bitmap_dino_duck` | 16x16 | Dino ducking. |
| `bitmap_cactus` | 16x16 | Cactus. |
| `bitmap_bird` | 16x8 | Bird. |
| `bitmap_gift` | 8x8 | Gift. |
| `bitmap_cloud` | 16x16 | Cloud background. |

### 6.2 Render gameplay

Screen calls the rendering modules in a fixed order.

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

### 6.3 Audio

| Tone | Usage |
|---|---|
| `tones_cc` | Ready, picking up a Gift, confirming an action. |
| `tones_startup` | Game start or being attacked. |
| `tones_3beep` | Game Over upon losing. |

## VII. Build and Flash Firmware

Build application:

```bash
cd application
make all
```

File firmware sau build:

```text
application/build_ak-base-kit-stm32l151-application/ak-base-kit-stm32l151-application.bin
```

Flash via AK bootloader:

```bash
make flash dev=/dev/ttyUSB0
```

Flash via ST-Link:

```bash
make flash
```

``` Note
Thank you for visiting this project.
If you have any questions, suggestions, or feedback regarding this project or its development process, please contact me directly.
```

**My contact:** <br/>
<a href="https://github.com/trqvu21">
  <img src="https://img.shields.io/badge/GitHub-QuangVu-181717?style=flat&logo=github&logoColor=white"/>
</a>
<a href="https://www.linkedin.com/in/trqvu21/">
  <img src="https://img.shields.io/badge/LinkedIn-Truong%20Quang%20Vu-0A66C2?style=flat&logo=linkedin&logoColor=white"/>
</a>
<a href="mailto:tqv4560@gmail.com">
  <img src="https://img.shields.io/badge/Gmail-tqv4560%40gmail.com-EA4335?style=flat&logo=gmail&logoColor=white"/>
</a>
