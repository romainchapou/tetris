#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#define WINDOW_WIDTH 10
#define WINDOW_HEIGHT 22

typedef enum TetriminoType {
    TETRIMINO_TYPE_I = 0,
    TETRIMINO_TYPE_O = 1,
    TETRIMINO_TYPE_T = 2,
    TETRIMINO_TYPE_L = 3,
    TETRIMINO_TYPE_J = 4,
    TETRIMINO_TYPE_Z = 5,
    TETRIMINO_TYPE_S = 6,
} TetriminoType;

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
    {{0, 0}, {1, 0}, {1, 1}, {1, 2}}, // 5  : Z
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
bool blocks[WINDOW_WIDTH][WINDOW_HEIGHT];

typedef struct Tetrimino {
    /* Position of the tetrimino */
    int x;
    int y;

    /* Width and height are used for collision detection */
    int width;
    int height;

    /* Rotation angle, from 0 to 3 */
    int angle;

    /* Type can be either I, O, T, L, J, Z or S */
    TetriminoType type;
} Tetrimino;

/* The tetrimino currently controlled by the player */
Tetrimino ctetr;

int get_shape_nb(Tetrimino t)
{
    return t.type + 7 * t.angle;
}

char get_tetromino_widht(Tetrimino t)
{
    int m = 0;
    int shape_nb = get_shape_nb(t);

    for (int i = 0; i < 4; ++i) {
        if (m > shapes[shape_nb][i][0])
            m = shapes[shape_nb][i][0];
    }

    return m + 1;
}

char get_tetromino_height(Tetrimino t)
{
    int m = 0;
    int shape_nb = get_shape_nb(t);

    for (int i = 0; i < 4; ++i) {
        if (m > shapes[shape_nb][i][1])
            m = shapes[shape_nb][i][1];
    }

    return m + 1;
}

/* Each "pixel" is two characters wide */
void print_pixel(int x, int y)
{
    mvwaddch(game_box, 1+y, 1+2*x, ACS_BLOCK);
    mvwaddch(game_box, 1+y, 2+2*x, ACS_BLOCK);
}

void get_new_tetrimino()
{
    ctetr.type = rand() % 7;

    ctetr.x = 5;
    ctetr.y = 0;
    ctetr.angle = 0;

    ctetr.width = get_tetromino_widht(ctetr);
    ctetr.height = get_tetromino_height(ctetr);
}

void add_blocks_to_matrix()
{
    int x;
    int y;

    int shape_nb = get_shape_nb(ctetr);

    for (int i = 0; i < 4; ++i) {
        x = shapes[shape_nb][i][0];
        y = shapes[shape_nb][i][1];

        blocks[ctetr.x + x][ctetr.y + y] = true;
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
        blocks[x][0] = false;
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

    wattron(game_box, COLOR_PAIR(ctetr.type + 1));

    for (int i = 0; i < 4; ++i) {
        x = shapes[shape_nb][i][0];
        y = shapes[shape_nb][i][1];

        print_pixel(ctetr.x + x, ctetr.y + y);
    }

    wattroff(game_box, COLOR_PAIR(ctetr.type + 1));
}

void display_game()
{
    // @Optim : don't clear the whole screen every frame
    clear();
    box(game_box, ACS_VLINE, ACS_HLINE);

    display_current_tetrimino();

    /* Display already fallen tetriminos */
    for (int i = 0; i < WINDOW_WIDTH; ++i) {
        for (int j = 0; j < WINDOW_HEIGHT; ++j) {
            if (blocks[i][j])
                print_pixel(i, j);
        }
    }

    wrefresh(game_box);
}

int main()
{
    /* Initialize graphics */
    initscr();       // Initialize the window
    noecho();        // Don't echo the keypresses
    curs_set(false); // Don't display the cursor
    start_color();

    /* Initialize the colors */
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, -1, -1);

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
