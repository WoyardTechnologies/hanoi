// PDSC task 2 - Wojciech Walczak 250280
#include "primlib.h"
#include <stdlib.h>
#include <unistd.h>

#define SCREEN_WIDTH gfx_screenWidth()
#define SCREEN_HEIGHT gfx_screenHeight()

#define N_OF_PEGS 3
#define N_OF_DISCS 10

#define FAILURE 0
#define SUCCESS 1
#define NONE 0

const float PEG_WIDTH_SCALE = 0.1; // multiplier of a peg's assigned chunk of screen width
const float PEG_HEIGHT_SCALE = 0.75; // multiplier of screen height above floor level
const float DISC_WIDTH_SCALE = 0.75; // multiplier of a peg's assigned chunk of screen width
const float DISC_STACK_HEIGHT_SCALE = 0.9; // multiplier of a peg's height, <1 leaves a bit of peg sticking out
const float FLOOR_THICKNESS_SCALE = 0.1; // multiplier of screen height
const float Y_ABOVE_PEGS_OFFSET_SCALE = 0.5; // multiplier of y distance left above pegs
const float DISC_SPACING_SCALE = 0.1; // disc spacing to separate same-color discs

// speed the game can be tweaked with these 2 values:
const int DELAY_BETWEEN_FRAMES = 16; // (in ms)
const int MOVEMENT_STEPS = 16;

// to be initialised on startup
int FLOOR_Y;
int PEG_CHUNK_WIDTH;
int PEG_WIDTH;
int PEG_HEIGHT;
int DEFAULT_DISC_WIDTH;
int DISC_HEIGHT;
int DISC_SPACING;
int Y_ABOVE_PEGS;

int held_disc = 0;
int pegs[N_OF_PEGS][N_OF_DISCS + 1] = {0};

void hanoi_init() {
    FLOOR_Y = SCREEN_HEIGHT * (1 - FLOOR_THICKNESS_SCALE);
    PEG_CHUNK_WIDTH = SCREEN_WIDTH / N_OF_PEGS;
    PEG_WIDTH = PEG_CHUNK_WIDTH * PEG_WIDTH_SCALE;
    PEG_HEIGHT = FLOOR_Y * PEG_HEIGHT_SCALE;
    DEFAULT_DISC_WIDTH = PEG_CHUNK_WIDTH * DISC_WIDTH_SCALE;
    DISC_HEIGHT = PEG_HEIGHT / (float) N_OF_DISCS * DISC_STACK_HEIGHT_SCALE;
    DISC_SPACING = DISC_HEIGHT * DISC_SPACING_SCALE;
    Y_ABOVE_PEGS = (SCREEN_HEIGHT - PEG_HEIGHT) * Y_ABOVE_PEGS_OFFSET_SCALE;
}

int get_x_of_peg(int peg) {
    return PEG_CHUNK_WIDTH * peg + PEG_CHUNK_WIDTH / 2;
}

int get_y_of_row(int row) {
    return FLOOR_Y - (row - 1) * DISC_HEIGHT;
}

int get_peg_from_keypress() {
    int key = gfx_getkey() - '0';
    if (key == 0) {
        return 9;
    }
    return key - 1;
}

void draw_peg(int center_x, int bottom_y) {
    gfx_filledRect(center_x - PEG_WIDTH / 2, bottom_y - PEG_HEIGHT,
                   center_x + PEG_WIDTH / 2, bottom_y,
                   YELLOW);
}

void draw_disc(int center_x, int bottom_y, int disc_number) {
    int radius = (DEFAULT_DISC_WIDTH * (disc_number / (float) N_OF_DISCS) + PEG_WIDTH) / 2;
    enum color disc_color;
    switch (disc_number % 3) {
        case 0:
            disc_color = BLUE;
            break;
        case 1:
            disc_color = CYAN;
            break;
        default:
            disc_color = WHITE;
            break;
    }
    gfx_filledRect(center_x - radius, bottom_y - DISC_HEIGHT + DISC_SPACING,
                   center_x + radius, bottom_y,
                   disc_color);

}

void draw_pegs_and_discs() {
    int center_x;
    for (int peg_id = 0; peg_id < N_OF_PEGS; peg_id++) {
        center_x = get_x_of_peg(peg_id);
        draw_peg(center_x, FLOOR_Y);
        for (int row = 1; row <= N_OF_DISCS; row++) {
            if (pegs[peg_id][row] == 0) {
                break; // avoids trying to draw discs on empty pegs
            }
            draw_disc(center_x, get_y_of_row(row), pegs[peg_id][row]);
        }
    }
}

void draw_background() {
    gfx_filledRect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BLACK);
    gfx_filledRect(0, FLOOR_Y, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, GREEN);
}

void draw_end_screen() {
    gfx_filledRect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BLACK);
    gfx_textout(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "CONGRATULATIONS, YOU WON!", YELLOW);
    gfx_updateScreen();
    while (!gfx_isKeyDown(SDLK_ESCAPE)) {
        SDL_Delay(DELAY_BETWEEN_FRAMES);
    }
}

void animate_disc_move(int x1, int y1, int x2, int y2) {
    if (x1 == x2 && y1 == y2) {
        return;
    }
    int step_x = (x2 - x1) / MOVEMENT_STEPS;
    int step_y = (y2 - y1) / MOVEMENT_STEPS;
    int x = x1 + (x2 - x1) % MOVEMENT_STEPS;
    int y = y1 + (y2 - y1) % MOVEMENT_STEPS;
    for (int i = 0; i < MOVEMENT_STEPS; i++) {
        draw_background();
        draw_pegs_and_discs();
        draw_disc(x, y, held_disc);
        gfx_updateScreen();
        x += step_x;
        y += step_y;
        SDL_Delay(DELAY_BETWEEN_FRAMES);
    }
}

int lift_disc_from_peg(int peg) {
    if (peg >= N_OF_PEGS || peg < 0) {
        return FAILURE;
    }
    int row = pegs[peg][0];
    if (held_disc == 0 && row != 0) {
        held_disc = pegs[peg][row];
        pegs[peg][row] = 0;
        pegs[peg][0] -= 1;
        animate_disc_move(get_x_of_peg(peg), get_y_of_row(pegs[peg][0] + 1),
                          get_x_of_peg(peg), Y_ABOVE_PEGS);
        return SUCCESS;
    }
    return FAILURE;
}

int move_disc_to_peg(int origin_peg, int dest_peg) {
    if (dest_peg >= N_OF_PEGS || dest_peg < 0) {
        return FAILURE; // move impossible
    }
    int dest_row = pegs[dest_peg][0];
    if (dest_row >= N_OF_DISCS || (held_disc >= pegs[dest_peg][dest_row] && pegs[dest_peg][dest_row] != 0)) {
        return FAILURE; // move impossible
    }
    animate_disc_move(get_x_of_peg(origin_peg), Y_ABOVE_PEGS,
                      get_x_of_peg(dest_peg), Y_ABOVE_PEGS);
    animate_disc_move(get_x_of_peg(dest_peg), Y_ABOVE_PEGS,
                      get_x_of_peg(dest_peg), get_y_of_row(pegs[dest_peg][0] + 1));
    pegs[dest_peg][dest_row + 1] = held_disc;
    pegs[dest_peg][0] += 1;
    return SUCCESS;
}

int check_for_win(int peg_id) {
    // checks if any peg other than the first one is fully stacked (win condition)
    if (peg_id > 0 && peg_id < N_OF_PEGS &&
        pegs[peg_id][0] == N_OF_DISCS) {
            return SUCCESS;
    }
    return FAILURE;
}

int main() {
    if (gfx_init()) {
        exit(3);
    }
    hanoi_init();
    int selected_peg1, selected_peg2;
    pegs[0][0] = N_OF_DISCS; // storing index of top element for faster 'stack' access
    for (int i = 1; i <= N_OF_DISCS; i++) {
        pegs[0][i] = N_OF_DISCS + 1 - i; // assigning disc numbers to first peg in array (pre-stacked tower)
    }
    while (!gfx_isKeyDown(SDLK_ESCAPE)) {
        draw_background();
        draw_pegs_and_discs();
        gfx_updateScreen();

        selected_peg1 = get_peg_from_keypress();
        if (lift_disc_from_peg(selected_peg1) == SUCCESS) {
            selected_peg2 = get_peg_from_keypress();
            if (move_disc_to_peg(selected_peg1, selected_peg2) == FAILURE) {
                move_disc_to_peg(selected_peg1, selected_peg1);
            } else {
                if (check_for_win(selected_peg2) == SUCCESS) {
                    draw_end_screen();
                    return 1;
                }
            }
            held_disc = 0;
        }
    }
    return 0;
}
