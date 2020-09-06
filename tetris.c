#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>


#define BLOCK_WIDTH 10
#define BLOCK_HEIGHT 22

bool end_game = false;
int delay = 60000;
int total_time = 0;

char last_input = 'l';
WINDOW *game_box;
bool blocks[BLOCK_WIDTH][BLOCK_HEIGHT];

typedef struct tetrimino {
    /* Position of the tetrimino */
    int x;
    int y;

    // TODO add width and height

    /* Rotation angle, from 0 to 3 */
    int angle;

    /* Type can be either I, O, T, L, J, Z or S */
    char type;
}tetrimino;

tetrimino current_tetrimino;

void print_pixel(int x, int y)
{
    mvwprintw(game_box, 1+y, 1+2*x, "##");
}

void update_game()
{
    wtimeout(game_box, 1);
    last_input = wgetch(game_box);
    // wrefresh(game_box);

    switch(last_input) {
        case 'h':
            --current_tetrimino.x;
            break;

        case 'l':
            ++current_tetrimino.x;
            break;

        case 'j':
            ++current_tetrimino.y;
            break;

        case 'q':
            end_game = true;
    }

    if(total_time % 30 == 0)
        ++current_tetrimino.y;

    if(current_tetrimino.y > BLOCK_HEIGHT)
        current_tetrimino.y = 0;

    if(current_tetrimino.x > BLOCK_WIDTH)
        current_tetrimino.x = 0;
    else if(current_tetrimino.x < 0)
        current_tetrimino.x = BLOCK_WIDTH;
}

void display_tetrimino(tetrimino t)
{
    switch(t.type) {
        case 'I':
            assert(0 && "Not done yet");
            break;

        case 'O':
            assert(0 && "Not done yet");
            break;

        case 'T':
            assert(0 && "Not done yet");
            break;

        case 'L':
            print_pixel(t.x, t.y);
            print_pixel(t.x, t.y + 1);
            print_pixel(t.x, t.y + 2);
            print_pixel(t.x + 1, t.y + 2);
            break;

        case 'J':
            assert(0 && "Not done yet");
            break;

        case 'Z':
            assert(0 && "Not done yet");
            break;

        case 'S':
            assert(0 && "Not done yet");
            break;
    }
}

void display_game()
{
    clear();
    wborder(game_box, '|', '|', '-', '-', '+', '+', '+', '+');

    display_tetrimino(current_tetrimino);

    /* Display already fallen tetriminos */
    for (int i = 0; i < BLOCK_WIDTH; ++i) {
        for (int j = 0; j < BLOCK_HEIGHT; ++j) {
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

    current_tetrimino = (tetrimino) { 5, 0, 0, 'L'};

    // blocks[0][0] = true;
    // blocks[1][0] = true;
    // blocks[1][1] = true;
    // blocks[1][2] = true;

    game_box = subwin(stdscr, 1 + BLOCK_HEIGHT + 2, 1 + 2*BLOCK_WIDTH + 1, 0, 0);
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
