#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#define WINDOW_WIDTH 10
#define WINDOW_HEIGHT 22

typedef enum BlockType {
    BLOCK_TYPE_NONE = 0,
    BLOCK_TYPE_I    = 1,
    BLOCK_TYPE_O    = 2,
    BLOCK_TYPE_T    = 3,
    BLOCK_TYPE_L    = 4,
    BLOCK_TYPE_J    = 5,
    BLOCK_TYPE_Z    = 6,
    BLOCK_TYPE_S    = 7,
} BlockType;

/*
 * The convention used for the coordinates is that (0, 0) is the closest point
 * to the shape such that every block in the shape has positive coordinates.
 *
 * For example, the first block of the L shaped tetrimino coincide with (0, 0),
 * but for the S shaped tetrimino, the (0, 0) point does not coincide with any
 * of its blocks.
 *
 *   L tetrimino                      S tetrimino
 *                                                     
 *  +---------> x                    +---------> x
 *  |                                |
 *  |  11 <--- (0, 0)       (0, 0) ---> X 3344
 *  |  22                            |  1122 
 *  |  3344                          |  
 *  |                                |
 *  v y                              v y
 *
 */
char shapes[28][4][2] = {

    /* angle 0 */
    {{0, 0}, {0, 1}, {0, 2}, {0, 3}}, // 0  : I
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, // 1  : O
    {{0, 1}, {1, 1}, {2, 1}, {1, 0}}, // 2  : T
    {{0, 0}, {0, 1}, {0, 2}, {1, 2}}, // 3  : L
    {{1, 0}, {1, 1}, {1, 2}, {0, 2}}, // 4  : J
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, // 5  : Z
    {{0, 1}, {1, 1}, {1, 0}, {2, 0}}, // 6  : S

    /* angle 1 */
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 7  : I  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 8  : O  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 9  : T  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 10 : L  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 11 : J  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 12 : Z  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 13 : S  TODO

    /* angle 2 */
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 14 : I  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 15 : O  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 16 : T  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 17 : L  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 18 : J  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 19 : Z  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 20 : S  TODO

    /* angle 3 */
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 21 : I  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 22 : O  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 23 : T  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 24 : L  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 25 : J  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 26 : Z  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 27 : S  TODO
};

bool end_game = false;
int delay = 15000;
int total_time = 0; // @Cleanup : make sure this doesn't overflow

WINDOW *game_box;
BlockType blocks[WINDOW_WIDTH][WINDOW_HEIGHT];

typedef struct Tetrimino {
    /* Position of the tetrimino */
    int x;
    int y;

    /* Rotation angle, from 0 to 3 */
    int angle;

    /* Type can be either I, O, T, L, J, Z or S */
    BlockType type;
} Tetrimino;

/* The tetrimino currently controlled by the player */
Tetrimino ctetr;

// @Optim : precompute this
int get_shape_nb(Tetrimino t)
{
    return (t.type - 1) + 7 * t.angle;
}

/* Each "pixel" is two characters wide */
void print_pixel(int x, int y)
{
    mvwaddch(game_box, 1+y, 1+2*x, ACS_BLOCK);
    mvwaddch(game_box, 1+y, 2+2*x, ACS_BLOCK);
}

void get_new_tetrimino()
{
    ctetr.type = 1 + rand() % 7;

    ctetr.x = 5;
    ctetr.y = 0;
    ctetr.angle = 0;
}

void add_blocks_to_matrix()
{
    int x;
    int y;

    int shape_nb = get_shape_nb(ctetr);

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[shape_nb][i][0];
        y = ctetr.y + shapes[shape_nb][i][1];

        blocks[x][y] = ctetr.type;
    }
}

bool line_is_complete(int i)
{
    for (int j = 0; j < WINDOW_WIDTH; ++j) {
        if (!blocks[j][i])
            return false;
    }

    return true;
}

void remove_line(int i)
{
    for (int j = i; j > 0; --j)
        for (int x = 0; x < WINDOW_WIDTH; ++x)
            blocks[x][j] = blocks[x][j-1];

    for (int x = 0; x < WINDOW_WIDTH; ++x)
        blocks[x][0] = BLOCK_TYPE_NONE;
}

void check_for_complete_line()
{
    for (int i = 0; i < WINDOW_HEIGHT; ++i)
        if (line_is_complete(i))
            remove_line(i);
}

// @Hack : this simple check will do for now, but we will eventually need
// something better
void check_for_game_over()
{
    for (int x = 0; x <  WINDOW_WIDTH; ++x)
        if(blocks[x][0])
            end_game = true;
}

bool can_move_down()
{
    int x;
    int y;

    int shape_nb = get_shape_nb(ctetr);

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[shape_nb][i][0];
        y = ctetr.y + shapes[shape_nb][i][1];

        if (y >= WINDOW_HEIGHT - 1 || blocks[x][y + 1])
            return false;
    }

    return true;
}

bool can_move_right()
{
    int x;
    int y;

    int shape_nb = get_shape_nb(ctetr);

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[shape_nb][i][0];
        y = ctetr.y + shapes[shape_nb][i][1];

        if (x >= WINDOW_WIDTH - 1 || (y >= 0 && blocks[x + 1][y]))
            return false;
    }

    return true;
}

bool can_move_left()
{
    int x;
    int y;

    int shape_nb = get_shape_nb(ctetr);

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[shape_nb][i][0];
        y = ctetr.y + shapes[shape_nb][i][1];

        if (x <= 0 || (y >= 0 && blocks[x - 1][y]))
            return false;
    }

    return true;
}

void update_game()
{
    /*
     * Get an input from the keyboard without waiting.
     * This works pretty well and even seems to add inputs to an internal queue
     * when they can't be treated right now.
     *
     * @Cleanup : See if it's possible to set the size of the input queue
     * (something like 3 should work nicely) as it causes weird behaviour when
     * mashing the keys.
     */
    wtimeout(game_box, 0);
    char last_input = wgetch(game_box);

    if (total_time % 60 == 0) {
        if (!can_move_down()) {
            add_blocks_to_matrix();
            check_for_complete_line();
            check_for_game_over();
            get_new_tetrimino();
        }
        else
            ++ctetr.y;
    }

    switch(last_input) {
        case 'h':
            if (can_move_left())
                --ctetr.x;
            break;

        case 'l':
            if (can_move_right())
                ++ctetr.x;
            break;

        case 'j':
            if (can_move_down())
                ++ctetr.y;
            break;

        case 'q':
            end_game = true;
    }
}

void display_current_tetrimino()
{
    int x;
    int y;

    int shape_nb = get_shape_nb(ctetr);

    wattron(game_box, COLOR_PAIR(ctetr.type));

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[shape_nb][i][0];
        y = ctetr.y + shapes[shape_nb][i][1];

        print_pixel(x, y);
    }

    wattroff(game_box, COLOR_PAIR(ctetr.type));
}

void display_game()
{
    // @Optim : don't clear the whole screen every frame
    clear();
    box(game_box, ACS_VLINE, ACS_HLINE);

    display_current_tetrimino();

    /*
     * @Optim : is it better to loop over the matrix for each color, so we loop
     * 7 times but we also change colors only 7 times, or to loop once and
     * change the color for each block ?
     */

    /* Display already fallen tetriminos */
    for (BlockType color = 1; color <= 7; ++color) {
        wattron(game_box, COLOR_PAIR(color));

        for (int i = 0; i < WINDOW_WIDTH; ++i)
            for (int j = 0; j < WINDOW_HEIGHT; ++j)
                if (blocks[i][j] == color)
                    print_pixel(i, j);

        wattroff(game_box, COLOR_PAIR(color));
    }

    wrefresh(game_box);
}

int main()
{
    /* Initialize graphics */
    initscr();       // Initialize the window
    noecho();        // Don't echo the keypresses
    curs_set(false); // Don't display the cursor
    start_color();   // Use colors

    /* Initialize the colors */
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);     /* I tetrimino */
    init_pair(2, COLOR_YELLOW, -1);   /* O tetrimino */
    init_pair(3, COLOR_MAGENTA, -1);  /* T tetrimino */
    init_pair(4, -1, -1);             /* L tetrimino */  /* foreground color */
    init_pair(5, COLOR_BLUE, -1);     /* J tetrimino */
    init_pair(6, COLOR_RED, -1);      /* Z tetrimino */
    init_pair(7, COLOR_GREEN, -1);    /* S tetrimino */

    /* Initalize the seed used to randomly spawn tetriminos */
    srand(time(NULL));

    get_new_tetrimino();

    game_box = subwin(stdscr, WINDOW_HEIGHT + 2, 2*WINDOW_WIDTH + 2, 0, 0);
    box(game_box, ACS_VLINE, ACS_HLINE);
    wrefresh(game_box);

    while (!end_game) {
        clear();

        update_game();
        display_game();

        ++total_time;
        usleep(delay);
    }

    /* Close ncurses */
    endwin();

    printf("Game over!\nYour score is : Not implemented yet !\n");
    
    return 0;
}
