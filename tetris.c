// tetris.c
// Minimal playable Tetris using ncurses.
// Compile: gcc tetris.c -o tetris -lncurses

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define BOARD_W 10
#define BOARD_H 20

typedef struct { int x, y; } Vec2;

#define SHAPES 7
#define SIZE 4

// Tetromino definitions (4x4 grids, row-major, 1 = block)
const int tetromino[SHAPES][SIZE][SIZE] = {
    // I
    {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    // O
    {
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // T
    {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // S
    {
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // Z
    {
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // J
    {
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // L
    {
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};

int board[BOARD_H][BOARD_W];
int curShape, curRot;
Vec2 curPos;
int score = 0;
int gameOver = 0;
int level = 1;
int linesCleared = 0;

// returns 1 if cell at shape r,c is set after rotation rot (0..3)
int shape_at(int shape, int rot, int r, int c) {
    int val = 0;
    switch (rot & 3) {
        case 0: val = tetromino[shape][r][c]; break;
        case 1: val = tetromino[shape][SIZE-1-c][r]; break;
        case 2: val = tetromino[shape][SIZE-1-r][SIZE-1-c]; break;
        case 3: val = tetromino[shape][c][SIZE-1-r]; break;
    }
    return val;
}

int collide(int nx, int ny, int nrot) {
    for (int r=0;r<SIZE;r++) for (int c=0;c<SIZE;c++) {
        if (shape_at(curShape, nrot, r, c)) {
            int bx = nx + c;
            int by = ny + r;
            if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return 1;
            if (by >= 0 && board[by][bx]) return 1;
        }
    }
    return 0;
}

void place_piece() {
    for (int r=0;r<SIZE;r++) for (int c=0;c<SIZE;c++) {
        if (shape_at(curShape, curRot, r, c)) {
            int bx = curPos.x + c;
            int by = curPos.y + r;
            if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W)
                board[by][bx] = curShape + 1; // store id > 0
        }
    }
}


void clear_lines() {
    int cleared = 0;
    for (int r = BOARD_H-1; r>=0; r--) {
        int full = 1;
        for (int c=0;c<BOARD_W;c++) if (!board[r][c]) { full = 0; break; }
        if (full) {
            cleared++;
            // move rows down
            for (int rr = r; rr > 0; rr--) for (int cc=0;cc<BOARD_W;cc++) board[rr][cc] = board[rr-1][cc];
            for (int cc=0;cc<BOARD_W;cc++) board[0][cc] = 0;
            r++; // recheck same row idx after pull-down
        }
    }
    if (cleared) {
        linesCleared += cleared;
        // simple scoring: standard-ish
        int pts = 0;
        if (cleared == 1) pts = 40;
        if (cleared == 2) pts = 100;
        if (cleared == 3) pts = 300;
        if (cleared == 4) pts = 1200;
        score += pts * level;
        // simple level up every 10 lines
        level = 1 + (linesCleared / 10);
    }
}

void spawn_piece() {
    curShape = rand() % SHAPES;
    curRot = 0;
    curPos.x = (BOARD_W/2) - 2;
    curPos.y = -1; // start above visible area
    if (collide(curPos.x, curPos.y, curRot)) gameOver = 1;
}

void init_game() {
    memset(board, 0, sizeof(board));
    srand((unsigned)time(NULL));
    score = 0;
    level = 1;
    linesCleared = 0;
    gameOver = 0;
    spawn_piece();
}

void draw_board(WINDOW *win) {
    werase(win);
    box(win, 0, 0);
    // draw board cells
    for (int r=0;r<BOARD_H;r++) for (int c=0;c<BOARD_W;c++) {
        int v = board[r][c];
        if (v) {
            mvwaddch(win, 1 + r, 1 + c*2, ACS_CKBOARD);
            mvwaddch(win, 1 + r, 1 + c*2+1, ACS_CKBOARD);
        } else {
            mvwaddch(win, 1 + r, 1 + c*2, ' ');
            mvwaddch(win, 1 + r, 1 + c*2+1, ' ');
        }
    }
    // draw current piece
    for (int r=0;r<SIZE;r++) for (int c=0;c<SIZE;c++) {
        if (shape_at(curShape, curRot, r, c)) {
            int bx = curPos.x + c;
            int by = curPos.y + r;
            if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
                mvwaddch(win, 1 + by, 1 + bx*2, ACS_CKBOARD);
                mvwaddch(win, 1 + by, 1 + bx*2+1, ACS_CKBOARD);
            }
        }
    }
    // status
    mvwprintw(win, 1, BOARD_W*2 + 3, "Score: %d", score);
    mvwprintw(win, 2, BOARD_W*2 + 3, "Level: %d", level);
    mvwprintw(win, 4, BOARD_W*2 + 3, "Lines: %d", linesCleared);
    mvwprintw(win, 6, BOARD_W*2 + 3, "Controls:");
    mvwprintw(win, 7, BOARD_W*2 + 3, "Arrows: move/rot");
    mvwprintw(win, 8, BOARD_W*2 + 3, "Space: hard drop");
    mvwprintw(win, 9, BOARD_W*2 + 3, "q: quit");
    if (gameOver) {
        mvwprintw(win, BOARD_H/2, BOARD_W - 4, "GAME OVER");
        mvwprintw(win, BOARD_H/2+1, BOARD_W - 8, "Press q to exit");
    }
    wrefresh(win);
}

long time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000L) + (tv.tv_usec / 1000L);
}

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    timeout(0);

    int win_h = BOARD_H + 2;
    int win_w = BOARD_W*2 + 2 + 20;
    WINDOW *win = newwin(win_h, win_w, 1, 1);

    init_game();

    long last_tick = time_ms();
    long drop_interval = 1000; // ms, will scale with level

    while (1) {
        int ch = getch();
        if (ch == 'q') break;
        if (gameOver) {
            // let user quit
            draw_board(win);
            usleep(30000);
            continue;
        }
        // input handling
        if (ch == KEY_LEFT) {
            if (!collide(curPos.x - 1, curPos.y, curRot)) curPos.x--;
        } else if (ch == KEY_RIGHT) {
            if (!collide(curPos.x + 1, curPos.y, curRot)) curPos.x++;
        } else if (ch == KEY_UP) {
            if (!collide(curPos.x, curPos.y, curRot + 1)) curRot++;
        } else if (ch == KEY_DOWN) {
            // soft drop
            if (!collide(curPos.x, curPos.y + 1, curRot))
                curPos.y++;
            else {
                // lock if can't move down
                place_piece();
                clear_lines();
                spawn_piece();
            }
            last_tick = time_ms(); // reset gravity timer a bit
        } else if (ch == ' ') {
            // hard drop
            while (!collide(curPos.x, curPos.y + 1, curRot)) curPos.y++;
            place_piece();
            clear_lines();
            spawn_piece();
            last_tick = time_ms();
        }

        // gravity timer scaling by level
        drop_interval = 1000 - (level - 1) * 80;
        if (drop_interval < 100) drop_interval = 100;

        long now = time_ms();
        if (now - last_tick >= drop_interval) {
            if (!collide(curPos.x, curPos.y + 1, curRot)) {
                curPos.y++;
            } else {
                place_piece();
                clear_lines();
                spawn_piece();
            }
            last_tick = now;
        }

        draw_board(win);
        usleep(20000); // small sleep to reduce CPU
    }

    delwin(win);
    endwin();
    return 0;
}