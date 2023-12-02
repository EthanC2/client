#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <ncurses.h>

#include "include/constants.hpp"
#include "include/error.hpp"

#define max(a,b) (a) > (b) ? (a) : (b)

WINDOW *wmessages;
int rows, columns;
int server_fd;

inline int next_line(WINDOW *window, const char *header, int *line, int length)
{
    if (*line >= length - 1)
    {
        wclear(window);
        mvwaddstr(window, 1, 1, header);
        box(window, 0, 0);
        wrefresh(window);
    }

    *line = max((*line + 1) % length, 2);
    return *line;
}

void read_server(int signo)
{
    //fputs("SIGIO fired", stdout);
    //static const char *header = "MESSAGES:";
    static char buffer[MESSAGE_LENGTH];
    //static int line = 0, prev_x, prev_y;
    //getyx(stdscr, prev_x, prev_y);

    read(server_fd, buffer, MESSAGE_LENGTH);
    printf("[SERVER]: >%s<", buffer);
    //mvwprintw(wmessages, next_line(wmessages, header, &line, rows/2 - 1), 1, "Client (from server): >%s<\n", buffer);
    //move(prev_x, prev_y);
}

void write_server()
{
    WINDOW *winput = newwin(rows/2 - 1, columns, rows/2 + 1, 0);
    char buffer[MESSAGE_LENGTH] {'\0'};
    const char *header = "INPUT:";
    bool done = false;
    int lineno = 0;

    mvwaddstr(winput, 1, 1, header);
    box(winput, 0, 0);
	wrefresh(winput);

    while (not done)
    {
        mvwgetnstr(winput, next_line(winput, header, &lineno, rows/2 - 2), 1, buffer, MESSAGE_LENGTH);

        write(server_fd, buffer, strlen(buffer) + 1);

        mvwaddstr(winput, 1, 1, header);
        box(winput, 0, 0);
        wrefresh(winput);
    }

    wborder(winput, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	wrefresh(winput);
	delwin(winput);
}

int main()
{
    sockaddr_in server_adddress;
    winsize dimensions;
    int flags;

    // 1. Initalize NCurses
    //initscr();
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &dimensions);
    rows = dimensions.ws_row;
    columns = dimensions.ws_col;
    //wmessages = newwin(rows/2, columns, 0, 0);

    // 2. Socket
    errchk( server_fd = socket(AF_INET, SOCK_STREAM, 0), "socket");

    // 3. Connect
    bzero(&server_adddress, sizeof(server_adddress));
    server_adddress.sin_family = AF_INET;
    server_adddress.sin_port = htons(PORT);
    errchk( inet_pton(AF_INET, LOCALHOST, &server_adddress.sin_addr), "inet_pton");
    errchk( connect(server_fd, (sockaddr*) &server_adddress, sizeof(server_adddress)), "connect");

    // N. Register the socket for signal-based I/O
    errchk( flags = fcntl(server_fd, F_GETFL), "fcntl - FGET_FL");
    errchk( fcntl(server_fd, F_SETOWN, getpid()), "fcntl - F_SETOWN" );
    errchk( fcntl(server_fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC), "fcntl - FSETFL" );
    signal(SIGIO, read_server);

    // 4. Main
    //write_server();
    do
    {
        sleep(1);
        fputs(".\n", stdout);
    } while (true);
    

    // 5. Deinitialize NCurses and close socket
    //wborder(wmessages, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	//wrefresh(wmessages);
	//delwin(wmessages);
    //endwin();

    errchk( close(server_fd), "close");

    return EXIT_SUCCESS;
}