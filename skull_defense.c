/* Skull Defense
 * Copyright (C) 2013 Clayton G. Hobbs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <uzebox.h>

#include "data/tiles.inc"
#include "data/fonts6x8.inc"


#define RAND(x) (rand() / (RAND_MAX / (x) + 1))
#define MAPXY(x, y) ((map[((x) + MAP_X*(y))/4] >> (2*(3-((x)%4)))) % 4)
#define MAPSET(x, y, tile) \
do { \
    map[((x) + MAP_X*(y))/4] &= ~(0x03 << (2*(3-((x)%4)))); \
    map[((x) + MAP_X*(y))/4] |= tile << (2*(3-((x)%4))); \
} while (0)

#define GUI 0x01
#define MAP 0x04
#define PLAYER (MAP + 4)
#define ENEMY (PLAYER + 3)

#define GVERT 0x0
#define GLTEE 0x1
#define GHORI 0x2

#define GRASS 0x0
#define FLOOR 0x1
#define DIRT  0x2
#define WALL  0x3

#define GUI_X 8
#define MAP_X 32
#define MAP_Y 28

#define CTR_N 7
#define CTR_S 20
#define CTR_W 8
#define CTR_E 23
#define CTR_VAR 4

#define BUILDING_MAX 3
#define EDGE_WALL 0x80
#define EDGE_HOLE 0x40
#define EDGE_HOLEPOS 0x3F


volatile uint8_t map[MAP_X / 4 * MAP_Y];
volatile uint8_t rooms[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t edges[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* [room] edge
 * [0]  0 [1]  1 [2]
 *  2      3      4
 * [3]  5 [4]  6 [5]
 *  7      8      9
 * [6] 10 [7] 11 [9]
 * in general
 *     n-3
 * n-1 [r] n
 *     n+2
 * n=r+2(r/3)
 */

struct button_info {
    uint16_t btn_held;
    uint16_t btn_prev;
    uint16_t btn_down;
    uint16_t btn_up;
};

volatile struct button_info _btn;

struct vector {
    int8_t x;
    int8_t y;
};


void _btn_events(void)
{
    _btn.btn_prev = _btn.btn_held;
    _btn.btn_held = ReadJoypad(0);
    _btn.btn_down |= _btn.btn_held & (_btn.btn_held ^ _btn.btn_prev);
    _btn.btn_up |= _btn.btn_prev & (_btn.btn_held ^ _btn.btn_prev);
}

void read_buttons(struct button_info *b)
{
    b->btn_held = _btn.btn_held;
    b->btn_prev = _btn.btn_prev;
    b->btn_down = _btn.btn_down;
    b->btn_up = _btn.btn_up;
    _btn.btn_down = 0;
    _btn.btn_up = 0;
}


void draw_status_bar(void)
{
    Fill(GUI_X - 1, 0, 1, 3, GUI + GVERT);
    SetTile(GUI_X - 1, 3, GUI + GLTEE);
    Fill(GUI_X - 1, 4, 1, 24, GUI + GVERT);
    Fill(0, 3, GUI_X - 1, 1, GUI + GHORI);
}

void draw_map(void)
{
    for (uint8_t y = 0; y < MAP_Y; y++)
        for (uint8_t x = 0; x < MAP_X; x++)
            SetTile(GUI_X + x, y, MAP + MAPXY(x, y));
}

void press_start(void)
{
    uint16_t seed = 0;
    struct button_info b;

    ClearVram();

    Print(8, 2, PSTR("I N T E L L I G E N C E"));
    Fill(7, 3, 25, 1, GUI + GHORI);
    Print(13, 5, PSTR("SKULL DEFENSE"));

    do {
        if (seed % 60 == 0) {
            Print(14, 20, PSTR("PRESS START"));
        } else if (seed % 60 == 30) {
            Print(14, 20, PSTR("           "));
        }
        seed++;
        WaitVsync(1);
        read_buttons(&b);
    } while ((b.btn_up & BTN_START) == 0);

    srand(seed);
}

void generate_level(void)
{
    uint8_t building_count = RAND(BUILDING_MAX) + 1;
    uint8_t h_bounds[4] = {0, CTR_N + RAND(CTR_VAR), CTR_S - RAND(CTR_VAR),
        27};
    uint8_t v_bounds[4] = {0, CTR_W + RAND(CTR_VAR), CTR_E - RAND(CTR_VAR),
        31};
    uint8_t i, r, x, y;

    // Clear the map efficiently
    for (i = 0; i < MAP_X * MAP_Y / 4; i++)
        map[i] = 0x00;

    // Place rooms
    for (; building_count > 0; building_count--) {
        r = RAND(9);
        for (i = h_bounds[r/3]; i <= h_bounds[r/3+1]; i++) {
            MAPSET(v_bounds[r%3], i, WALL);
            MAPSET(v_bounds[r%3+1], i, WALL);
        }
        for (i = v_bounds[r%3]; i <= v_bounds[r%3+1]; i++) {
            MAPSET(i, h_bounds[r/3], WALL);
            MAPSET(i, h_bounds[r/3+1], WALL);
        }
        for (x = v_bounds[r%3] + 1; x <= v_bounds[r%3+1] - 1; x++) {
            for (y = h_bounds[r/3] + 1; y <= h_bounds[r/3+1] - 1; y++) {
                MAPSET(x, y, FLOOR);
            }
        }
    }
}

int main()
{
    uint8_t level = 1;
    uint16_t cash = 0;

    SetTileTable(tiles);
    SetFontTilesIndex(TILES_SIZE);
    SetUserPostVsyncCallback(&_btn_events);

    press_start();
    ClearVram();

    struct vector p = {RAND(MAP_X), RAND(MAP_Y)};
    struct vector dp = {0, 0};
    struct button_info b;
    draw_status_bar();
    Print(1, 1, PSTR("LVL"));
    PrintChar(1, 2, '$');
    generate_level();
    draw_map();
    for (;;) {
        PrintByte(6, 1, level, false);
        PrintInt(6, 2, cash, false);

        read_buttons(&b);
        if (b.btn_down & BTN_LEFT)
            dp.x -= 1;
        if (b.btn_down & BTN_RIGHT)
            dp.x += 1;
        if (b.btn_down & BTN_UP)
            dp.y -= 1;
        if (b.btn_down & BTN_DOWN)
            dp.y += 1;
        if (b.btn_down & BTN_SELECT) {
            generate_level();
            draw_map();
        }

        if (dp.x && p.x + dp.x >= 0 && p.x + dp.x < MAP_X &&
                MAPXY(p.x + dp.x, p.y) != WALL && dp.y == 0) {
            SetTile(p.x + GUI_X, p.y, MAP + MAPXY(p.x, p.y));
            p.x += dp.x;
        }
        if (dp.y && p.y + dp.y >= 0 && p.y + dp.y < MAP_Y &&
                MAPXY(p.x, p.y + dp.y) != WALL) {
            SetTile(p.x + GUI_X, p.y, MAP + MAPXY(p.x, p.y));
            p.y += dp.y;
        }
        dp.x = 0;
        dp.y = 0;

        SetTile(p.x + GUI_X, p.y, PLAYER + MAPXY(p.x, p.y));

        WaitVsync(6);
    }
} 
