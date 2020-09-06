#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>

#define WINDOW_WIDTH 10
#define WINDOW_HEIGHT 22

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
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 0  : I  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 1  : O  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 2  : T  TODO
    {{0, 0}, {0, 1}, {0, 2}, {1, 2}}, // 3  : L
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 4  : J  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 5  : Z  TODO
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, // 6  : S  TODO

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
int delay = 60000;
int total_time = 0;

char last_input = 'l';
WINDOW *game_box;
bool blocks[WINDOW_WIDTH][WINDOW_HEIGHT];

typedef struct tetrimino {
    /* Position of the tetrimino */
    int x;
    int y;

    /* Width and heihgt are used for collision detection */
    int width;
    int height;

    /* Rotation angle, from 0 to 3 */
    int angle;

    /* Type can be either I, O, T, L, J, Z or S */
    char type;
}tetrimino;

/* The tetrimino currently controlled by the player */
tetrimino ctetr;

int tetrimino_id(tetrimino t)
{
    switch(t.type) {
        case 'I': return 0;
        case 'O': return 1;
        case 'T': return 2;
        case 'L': return 3;
        case 'J': return 4;
        case 'Z': return 5;
        case 'S': return 6;
    }

    assert(0 && "Tetrimino id not found");
}


char get_tetromino_widht()
{
    int m = 0;
    char shape_nb = tetrimino_id(ctetr) + 7*ctetr.angle;

    for (int i = 0; i < 4; ++i) {
        if (m > shapes[shape_nb][i][0])
            m = shapes[shape_nb][i][0];
    }

    return m + 1;
}

char get_tetromino_height()
{
    int m = 0;
    char shape_nb = tetrimino_id(ctetr) + 7*ctetr.angle;

    for (int i = 0; i < 4; ++i) {
        if (m > shapes[shape_nb][i][1])
            m = shapes[shape_nb][i][1];
    }

    return m + 1;
}

void print_pixel(int x, int y)
{
    mvwprintw(game_box, 1+y, 1+2*x, "##");
}

void get_new_tetrimino()
{
    // @Hardcoded
    ctetr = (tetrimino) { 5, 0, 2, 3, 0, 'L'};
}

void update_game()
{
    wtimeout(game_box, 1);
    last_input = wgetch(game_box);
    // wrefresh(game_box);

    switch(last_input) {
        case 'h':
            if (ctetr.x > 0)
                --ctetr.x;
            break;

        case 'l':
            if (ctetr.x + ctetr.width < WINDOW_WIDTH)
                ++ctetr.x;
            break;

        case 'j':
            if (ctetr.y + ctetr.height < WINDOW_HEIGHT + 1)
                ++ctetr.y;
            break;

        case 'q':
            end_game = true;
    }

    if(total_time % 30 == 0)
    {
        if(ctetr.y + ctetr.height > WINDOW_HEIGHT)
            get_new_tetrimino();
        else
            ++ctetr.y;
    }
}

void display_current_tetrimino()
{
    int x;
    int y;

    int shape_nb = tetrimino_id(ctetr) + 7*ctetr.angle;

    for (int i = 0; i < 4; ++i) {
        x = shapes[shape_nb][i][0];
        y = shapes[shape_nb][i][1];

        print_pixel(ctetr.x + x, ctetr.y + y);
    }
}

void display_game()
{
    // @Optim : don't clear the whole screen every frame
    clear();
    wborder(game_box, '|', '|', '-', '-', '+', '+', '+', '+');

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

    get_new_tetrimino();

    // blocks[0][0] = true;
    // blocks[1][0] = true;
    // blocks[1][1] = true;
    // blocks[1][2] = true;

    game_box = subwin(stdscr, 1 + WINDOW_HEIGHT + 2, 1 + 2*WINDOW_WIDTH + 1, 0, 0);
    wborder(game_box, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(game_box);

    while(!end_game)
    {
        clear();

        update_game();
        display_game();

        ++total_time;
        usleep(delay);
    }

    /* Close ncurses */
    endwin();
    
    return 0;
}
