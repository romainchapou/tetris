#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 10
#define WINDOW_HEIGHT 20

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

/* Possible shapes of all tetriminos given a rotation angle */
char shapes[28][4][2] = {
    /* angle 0 -- facing down */
    {{-2,  0}, {-1,  0}, { 0,  0}, { 1,  0}},  //  0 : I
    {{-1,  0}, { 0,  0}, {-1,  1}, { 0,  1}},  //  1 : O
    {{-1,  0}, { 0,  0}, { 1,  0}, { 0,  1}},  //  2 : T
    {{-1,  0}, { 0,  0}, { 1,  0}, {-1,  1}},  //  3 : L
    {{-1,  0}, { 0,  0}, { 1,  0}, { 1,  1}},  //  4 : J
    {{-1,  0}, { 0,  0}, { 0,  1}, { 1,  1}},  //  5 : Z
    {{ 0,  0}, { 1,  0}, {-1,  1}, { 0,  1}},  //  6 : S

    /* angle 1 -- facing left */
    {{ 0, -2}, { 0, -1}, { 0,  0}, { 0,  1}},  //  7 : I
    {{-1,  0}, { 0,  0}, {-1,  1}, { 0,  1}},  //  8 : O
    {{ 0, -1}, {-1,  0}, { 0,  0}, { 0,  1}},  //  9 : T
    {{-1, -1}, { 0, -1}, { 0,  0}, { 0,  1}},  // 10 : L
    {{ 0, -1}, { 0,  0}, {-1,  1}, { 0,  1}},  // 11 : J
    {{ 1, -1}, { 0,  0}, { 1,  0}, { 0,  1}},  // 12 : Z
    {{ 0, -1}, { 0,  0}, { 1,  0}, { 1,  1}},  // 13 : S

    /* angle 2 -- facing up */
    {{-2,  0}, {-1,  0}, { 0,  0}, { 1,  0}},  // 14 : I
    {{-1,  0}, { 0,  0}, {-1,  1}, { 0,  1}},  // 15 : O
    {{-1,  0}, { 0,  0}, { 1,  0}, { 0, -1}},  // 16 : T
    {{ 1, -1}, {-1,  0}, { 0,  0}, { 1,  0}},  // 17 : L
    {{-1, -1}, {-1,  0}, { 0,  0}, { 1,  0}},  // 18 : J
    {{-1,  0}, { 0,  0}, { 0,  1}, { 1,  1}},  // 19 : Z
    {{ 0,  0}, { 1,  0}, {-1,  1}, { 0,  1}},  // 20 : S

    /* angle 3 -- facing right */
    {{ 0, -2}, { 0, -1}, { 0,  0}, { 0,  1}},  // 21 : I
    {{-1,  0}, { 0,  0}, {-1,  1}, { 0,  1}},  // 22 : O
    {{ 0, -1}, { 0,  0}, { 1,  0}, { 0,  1}},  // 23 : T
    {{ 0, -1}, { 0,  0}, { 0,  1}, { 1,  1}},  // 24 : L
    {{ 0, -1}, { 1, -1}, { 0,  0}, { 0,  1}},  // 25 : J
    {{ 1, -1}, { 0,  0}, { 1,  0}, { 0,  1}},  // 26 : Z
    {{ 0, -1}, { 0,  0}, { 1,  0}, { 1,  1}},  // 27 : S
};

/* Values used to center the tetrimino in the preview box */
char center_lengths[7] = {
    5, // I
    5, // O
    4, // T
    4, // L
    4, // J
    4, // Z
    4, // S
};

/* Run at around 60 fps */
const int refresh_delay = 16640;

/* Path to the file were the highscore is stored,
 * $HOME/.config/ncurses_tetris/highscore */
char* high_score_file;

long old_highscore = 0;
long highscore = 0;
long score = 0;

int level = 0;
int start_level = 0;
int lines_before_next_level;
int cleared_lines = 0;

/* Tetrimino's position will be updated once every fall_rate frame */
int fall_rate = 48;

bool end_game = false;
bool game_is_paused = false;

// @Hack : This starts at one so the game doesn't update right away
int nb_frames = 1;

WINDOW* level_box;
WINDOW* score_box;
WINDOW* highscore_box;
WINDOW* lines_box;
WINDOW* game_box;
WINDOW* pause_box;
WINDOW* next_piece_box;

BlockType blocks[WINDOW_WIDTH][WINDOW_HEIGHT];

typedef struct Tetrimino {
    /* Position of the tetrimino */
    int x;
    int y;

    /* Rotation angle, from 0 to 3 */
    int angle;

    /* Type can be either I, O, T, L, J, Z or S */
    BlockType type;

    /* Combination of type and angle, id of the shape in the shapes array */
    int shape_number;
} Tetrimino;

/* The tetrimino currently controlled by the player */
Tetrimino ctetr;

/* The tetrimino available next */
Tetrimino ntetr;

// @Optim : precompute this
int get_shape_nb(BlockType type, int angle)
{
    return (type - 1) + 7 * angle;
}

/* Each "pixel" is two characters wide */
void print_pixel(int x, int y, WINDOW* window)
{
    if (y >= 0) {
        mvwaddch(window, 1+y, 1+2*x, ACS_BLOCK);
        mvwaddch(window, 1+y, 2+2*x, ACS_BLOCK);
    }
}

void print_shiny_pixel(int x, int y, WINDOW* window)
{
    if (y >= 0) {
        mvwaddch(window, 1+y, 1+2*x, ACS_CKBOARD);
        mvwaddch(window, 1+y, 2+2*x, ACS_CKBOARD);
    }
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

/* @Cleanup : maybe have the highscore file be in the same directory as the
 * source code so as to not clutter ~/.config */
/* Initialise the value of high_score_file.
 * This is needed as $HOME must be expanded */
void init_highscore_info()
{
    char* high_score_dir = NULL;
    char* home_dir = getenv("HOME");

    int len_home_path = strlen(home_dir);
    high_score_dir = calloc(len_home_path + 100, sizeof(char));
    high_score_file = calloc(len_home_path + 100, sizeof(char));

    strcat(high_score_dir, home_dir);
    strcat(high_score_dir, "/.config/ncurses_tetris");

    /* Create ~/.config/ncurses_tetris/ if needed */
    mkdir(high_score_dir, 0777);

    strcat(high_score_file, high_score_dir);
    strcat(high_score_file, "/highscore");

    free(high_score_dir);
}

void deinit_highscore_info()
{
    free(high_score_file);
}

void read_highscore()
{
    FILE* file = fopen(high_score_file, "r");

    if (file != NULL) {
        if (fscanf(file, "%ld\n", &highscore) != 1)
            assert(0 && "Reading highscore from file failed");

        old_highscore = highscore;
        fclose(file);
    }
}

void update_highscore()
{
    if (highscore > old_highscore) {
        FILE* file = fopen(high_score_file, "w");

        if (file != NULL) {
            fprintf(file, "%ld\n", highscore);
            fclose(file);
        } else {
            assert(0 && "Highscore file could not be opened");
        }
    }
}

Tetrimino make_new_tetrimino(BlockType type)
{
    Tetrimino t;

    t.type = type;
    t.angle = 0;

    t.x = 5;
    t.y = 0;

    t.shape_number = get_shape_nb(t.type, t.angle);

    return t;
}

/* Use the same random generator as NES Tetris */
void get_new_tetrimino()
{
    BlockType old_type = ntetr.type;
    BlockType new_type = rand() % 8;

    if (new_type == old_type || new_type == 0)
        new_type = 1 + rand() % 7;

    ctetr = ntetr;
    ntetr = make_new_tetrimino(new_type);
}

bool shape_can_fit(int tx, int ty, int shape_number)
{
    int x; int y;

    for (int i = 0; i < 4; ++i) {
        x = tx + shapes[shape_number][i][0];
        y = ty + shapes[shape_number][i][1];

        if (y < 0)
            continue;
        else if (x < 0 || x >= WINDOW_WIDTH || y >= WINDOW_HEIGHT || blocks[x][y])
            return false;
    }

    return true;
}

void rotate_tetrimino(int angle)
{
    // C modulos really are the greatest
    if (angle == -1)
        angle = 3;

    assert(angle >= 0);

    int new_angle = (ctetr.angle + angle) % 4;
    int new_shape_nb = get_shape_nb(ctetr.type, new_angle);

    if (shape_can_fit(ctetr.x, ctetr.y, new_shape_nb)) {
        ctetr.angle = new_angle;
        ctetr.shape_number = new_shape_nb;
    }
}

void add_blocks_to_board()
{
    int x;
    int y;

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[ctetr.shape_number][i][0];
        y = ctetr.y + shapes[ctetr.shape_number][i][1];

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

int score_factor(int nb_completed_lines)
{
    switch (nb_completed_lines)
    {
        case 0: return 0;
        case 1: return 40;
        case 2: return 100;
        case 3: return 300;
        case 4: return 1200;
    }

    assert(0 && "Impossible number of lines cleared");
}

void highlight_line(int line_nb)
{
    wattron(game_box, COLOR_WHITE);

    for (int i = 0; i < WINDOW_WIDTH; ++i)
        print_shiny_pixel(i, line_nb, game_box);

    wattroff(game_box, COLOR_WHITE);
}

void check_for_complete_lines()
{
    int nb_completed_lines = 0;
    int lines_to_be_removed[4] = {-1, -1, -1, -1};

    /* Find the lines to be removed */
    // @Optim : only do this for lines close to the fallen tetrimino
    for (int i = 0; i < WINDOW_HEIGHT; ++i) {
        if (line_is_complete(i)) {
            lines_to_be_removed[nb_completed_lines] = i;
            ++nb_completed_lines;
        }
    }

    /* Highlight all lines to be removed */
    for (int i = 0; i < nb_completed_lines; ++i)
            highlight_line(lines_to_be_removed[i]);
    wrefresh(game_box);

    /* Freeze for 20 frames */
    if (nb_completed_lines > 0)
        usleep(20*refresh_delay);

    /* Actually remove the lines */
    for (int i = 0; i < nb_completed_lines; ++i)
        remove_line(lines_to_be_removed[i]);

    score += (level + 1) * score_factor(nb_completed_lines);
    cleared_lines += nb_completed_lines;
}

// @Hack : this simple check will do for now, but we will eventually need
// something better
void check_for_game_over()
{
    for (int x = 0; x <  WINDOW_WIDTH; ++x)
        if (blocks[x][0])
            end_game = true;
}

bool can_move_down()
{
    int x;
    int y;

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[ctetr.shape_number][i][0];
        y = ctetr.y + shapes[ctetr.shape_number][i][1];

        if (y < 0)
            continue;
        if (y >= WINDOW_HEIGHT - 1 || blocks[x][y + 1])
            return false;
    }

    return true;
}

bool can_move_right()
{
    int x;
    int y;

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[ctetr.shape_number][i][0];
        y = ctetr.y + shapes[ctetr.shape_number][i][1];

        if (x >= WINDOW_WIDTH - 1 || (y >= 0 && blocks[x + 1][y]))
            return false;
    }

    return true;
}

bool can_move_left()
{
    int x;
    int y;

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[ctetr.shape_number][i][0];
        y = ctetr.y + shapes[ctetr.shape_number][i][1];

        if (x <= 0 || (y >= 0 && blocks[x - 1][y]))
            return false;
    }

    return true;
}

int new_fall_rate()
{
    switch (level) {
        case 0: return 48;
        case 1: return 43;
        case 2: return 38;
        case 3: return 33;
        case 4: return 28;
        case 5: return 23;
        case 6: return 18;
        case 7: return 13;
        case 8: return 8;
        case 9: return 6;

        case 10:
        case 11:
        case 12:
            return 5;

        case 13:
        case 14:
        case 15:
            return 4;

        case 16:
        case 17:
        case 18:
            return 3;
    }

    if (level >= 19 && level <= 28)
        return 2;
    else if (level >= 29)
        return 1;
    else
        assert(0 && "Impossible level value");
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
    int last_input = wgetch(game_box);

    if (nb_frames % fall_rate == 0) {
        if (!can_move_down()) {
            add_blocks_to_board();
            check_for_complete_lines();
            check_for_game_over();

            // @Incomplete : add proper delay before this
            get_new_tetrimino();
        } else {
            ++ctetr.y;
        }
    }

    /* Update the level if needed */
    if (cleared_lines >= lines_before_next_level) {
        ++level;
        lines_before_next_level += 10;
        fall_rate = new_fall_rate();
        nb_frames = 0;
    }

    switch (last_input) {
        case 'h':
        case KEY_LEFT:
            if (can_move_left())
                --ctetr.x;
            break;

        case 'l':
        case KEY_RIGHT:
            if (can_move_right())
                ++ctetr.x;
            break;

        case 'j':
        case KEY_DOWN:
            if (can_move_down()) {
                ++ctetr.y;

                /* Reset the timer when the tetrimino is about to be dropped as
                 * to give the player a last chance to place it correctly
                 * without any timer luck involved */
                if (!can_move_down())
                    nb_frames = 0;
            }
            break;

        case 'k':
        case 'c':
        case KEY_UP:
            rotate_tetrimino(1);
            break;

        case 'e':
        case 'x':
            rotate_tetrimino(-1);
            break;

        case 'p':
            game_is_paused = true;
            break;

        case 'q':
            end_game = true;
    }

    if (score > highscore)
        highscore = score;
}

void update_pause()
{
    wtimeout(game_box, 0);
    int last_input = wgetch(game_box);

    switch (last_input) {
        case 'p':
            game_is_paused = false;
            break;

        case 'q':
            end_game = true;
    }
}

void display_current_tetrimino()
{
    int x;
    int y;

    wattron(game_box, COLOR_PAIR(ctetr.type));

    for (int i = 0; i < 4; ++i) {
        x = ctetr.x + shapes[ctetr.shape_number][i][0];
        y = ctetr.y + shapes[ctetr.shape_number][i][1];

        print_pixel(x, y, game_box);
    }

    wattroff(game_box, COLOR_PAIR(ctetr.type));
}

void display_next_tetrimino()
{
    int x;
    int y;
    int center_length = center_lengths[ntetr.type - 1];

    wattron(next_piece_box, COLOR_PAIR(ntetr.type));

    for (int i = 0; i < 4; ++i) {
        x = shapes[ntetr.shape_number][i][0];
        y = shapes[ntetr.shape_number][i][1];

        mvwaddch(next_piece_box, 1+y, 1 + center_length + 2*x, ACS_BLOCK);
        mvwaddch(next_piece_box, 1+y, 2 + center_length + 2*x, ACS_BLOCK);
    }

    wattroff(next_piece_box, COLOR_PAIR(ntetr.type));
}

void display_game()
{
    // @Optim : don't clear the whole screen every frame
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

        // @Cleanup : should be (x, y) instead of (i, j)
        for (int i = 0; i < WINDOW_WIDTH; ++i)
            for (int j = 0; j < WINDOW_HEIGHT; ++j)
                if (blocks[i][j] == color)
                    print_pixel(i, j, game_box);

        wattroff(game_box, COLOR_PAIR(color));
    }

    wrefresh(game_box);
}

void display_level()
{
    box(level_box, ACS_VLINE, ACS_HLINE);
    mvwprintw(level_box, 0, 1, "Level");
    mvwprintw(level_box, 1, 1, "%ld", level);

    wrefresh(level_box);
}

void display_score()
{
    box(score_box, ACS_VLINE, ACS_HLINE);
    mvwprintw(score_box, 0, 1, "Score");
    mvwprintw(score_box, 1, 1, "%ld", score);

    wrefresh(score_box);
}

void display_next_piece()
{
    box(next_piece_box, ACS_VLINE, ACS_HLINE);
    mvwprintw(next_piece_box, 0, 1, "Next");

    display_next_tetrimino();

    wrefresh(next_piece_box);
}

void display_highscore()
{
    box(highscore_box, ACS_VLINE, ACS_HLINE);
    mvwprintw(highscore_box, 0, 1, "Highscore");
    mvwprintw(highscore_box, 1, 1, "%ld", highscore);

    wrefresh(highscore_box);
}

void display_lines()
{
    box(lines_box, ACS_VLINE, ACS_HLINE);
    mvwprintw(lines_box, 0, 1, "Lines");
    mvwprintw(lines_box, 1, 1, "%ld", cleared_lines);

    wrefresh(lines_box);
}

void draw_game()
{
    clear();

    display_game();
    display_score();
    display_level();
    display_next_piece();
    display_highscore();
    display_lines();
}

void draw_pause()
{
    box(pause_box, ACS_VLINE, ACS_HLINE);
    mvwprintw(pause_box, 1, 1, "Paused");

    wrefresh(pause_box);
}

int main()
{
    /* Initialize ncurses */
    initscr();       // Initialize the window
    noecho();        // Don't echo the key presses
    curs_set(false); // Don't display the cursor
    cbreak();        // Get input character by character
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

    /* Initialize the seed used to randomly spawn tetriminos */
    srand(time(NULL));

    /* Initialize game window */
    game_box = subwin(stdscr, WINDOW_HEIGHT + 2, 2*WINDOW_WIDTH + 2, 0, 0);
    box(game_box, ACS_VLINE, ACS_HLINE);
    wrefresh(game_box);
    keypad(game_box, true); // Enable arrow keys

    /* Initialize level window */
    level_box = subwin(stdscr, 1 + 2, WINDOW_WIDTH + 2, 2, 2*WINDOW_WIDTH + 2);
    box(level_box, ACS_VLINE, ACS_HLINE);
    wrefresh(level_box);

    /* Initialize lines window */
    lines_box = subwin(stdscr, 1 + 2, WINDOW_WIDTH + 2, 5, 2*WINDOW_WIDTH + 2);
    box(lines_box, ACS_VLINE, ACS_HLINE);
    wrefresh(lines_box);

    /* Initialize next piece window */
    next_piece_box = subwin(stdscr, 2 + 2, WINDOW_WIDTH + 2, 9, 2*WINDOW_WIDTH + 2);
    box(next_piece_box, ACS_VLINE, ACS_HLINE);
    wrefresh(next_piece_box);

    /* Initialize score window */
    score_box = subwin(stdscr, 1 + 2, WINDOW_WIDTH + 2, 14, 2*WINDOW_WIDTH + 2);
    box(score_box, ACS_VLINE, ACS_HLINE);
    wrefresh(score_box);

    /* Initialize highscore window */
    highscore_box = subwin(stdscr, 1 + 2, WINDOW_WIDTH + 2, 17, 2*WINDOW_WIDTH + 2);
    box(highscore_box, ACS_VLINE, ACS_HLINE);
    wrefresh(highscore_box);

    /* Initialize pause window */
    pause_box = subwin(stdscr, 3, 8, WINDOW_HEIGHT / 2, 7);
    box(pause_box, ACS_VLINE, ACS_HLINE);
    wrefresh(pause_box);

    init_highscore_info(); // must be done before read_highscore
    read_highscore();

    /* Initialize the tetriminos */
    ntetr = make_new_tetrimino(1 + rand() % 7);
    get_new_tetrimino();

    /* For the starting level, this is the formula to get the lines to be
     * cleared before getting to the next one. After that, a new level is
     * reached after clearing 10 lines. See https://tetris.wiki/Scoring
     */
    lines_before_next_level = min(10 * start_level + 10, max(100, 10 * start_level - 50));

    while (!end_game) {
        if (!game_is_paused) {
            update_game();
            draw_game();

            // @Hack : this makes the game run at an unpredictable frame rate
            usleep(refresh_delay);
        } else {
            update_pause();
            draw_pause();

            // When paused, the game doesn't need to be updated as frequently.
            usleep(10*refresh_delay);
        }

        ++nb_frames;
    }

    /* Close ncurses */
    endwin();

    update_highscore();
    deinit_highscore_info();

    printf("Game over!\nYour score is : %ld\n", score);

    if (highscore > old_highscore)
        printf("This is a new highscore!\n");
    
    return 0;
}
