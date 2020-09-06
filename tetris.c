#include <ncurses.h>

int main()
{
    initscr();
    printw("Hello world");
    getch();
    endwin();

    return 0;
}
