#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ncurses.h>

#include "include/constants.hpp"
#include "include/error.hpp"

#define max(a,b) (a) > (b) ? (a) : (b)

inline int next_line(WINDOW *window, const char *header, int *line, int rows)
{
    if (*line >= rows - 1)
    {
        wclear(window);
        mvwaddstr(window, 1, 1, header);
        box(window, 0, 0);
        wrefresh(window);
    }

    *line = max((*line + 1) % rows, 2);
    return *line;
}

void echo(int fd, unsigned short rows, unsigned short columns)
{
    // Create windows
    WINDOW *message_window = newwin(rows/2, columns, 0, 0);
    WINDOW *input_window = newwin(rows/2 - 1, columns, rows/2 + 1, 0);
    const char *message_window_header = "MESSAGES:";
    const char *input_window_header = "INPUT:";
    int message_window_line = 0;
    int input_window_line = 0;

    mvwaddstr(message_window, 1, 1, message_window_header);
    mvwaddstr(input_window, 1, 1, input_window_header);

    box(message_window, 0, 0);
	wrefresh(message_window);

    box(input_window, 0, 0);
	wrefresh(input_window);

    // Echo client
    char client_buffer[MESSAGE_LENGTH] {'\0'};
    char server_buffer[MESSAGE_LENGTH];
    bool done = false;

    while (not done)
    {
        mvwgetnstr(input_window, next_line(input_window, input_window_header, &input_window_line, rows/2 - 2), 1, client_buffer, MESSAGE_LENGTH);
        //mvwprintw(message_window, next_line(message_window, &message_window_line, rows/2 - 1), 1, "Client: (from stdin): >%s<\n", client_buffer);

        write(fd, client_buffer, strlen(client_buffer)+1);

        if (strcmp(client_buffer, "exit") == 0)
        {
            done = true;
        }
        else
        {
            if (read(fd, server_buffer, MESSAGE_LENGTH) == 0)
            {
                wprintw(message_window, "ERROR: server terminated");
                exit(EXIT_FAILURE);
            }
        }

        mvwprintw(message_window, next_line(message_window, message_window_header, &message_window_line, rows/2 - 1), 1, "Client (from server): >%s<\n", server_buffer);

        // Update windows
        mvwaddstr(message_window, 1, 1, message_window_header);
        box(input_window, 0, 0);
        wrefresh(message_window);

        mvwaddstr(input_window, 1, 1, input_window_header);
        box(input_window, 0, 0);
        wrefresh(input_window);
    }

    // Delete windows
    wborder(message_window, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	wrefresh(message_window);
	delwin(message_window);

    wborder(input_window, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	wrefresh(input_window);
	delwin(input_window);
}

int main()
{
    sockaddr_in server_adddress;
    winsize dimensions;
    int fd;

    // 1. Initalize NCurses
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &dimensions);
    initscr();

    // 2. Socket
    errchk( fd = socket(AF_INET, SOCK_STREAM, 0), "socket");

    // 3. Connect
    bzero(&server_adddress, sizeof(server_adddress));
    server_adddress.sin_family = AF_INET;
    server_adddress.sin_port = htons(PORT);
    errchk( inet_pton(AF_INET, LOCALHOST, &server_adddress.sin_addr), "inet_pton");
    errchk( connect(fd, (sockaddr*) &server_adddress, sizeof(server_adddress)), "connect");

    // 4. Echo 
    echo(fd, dimensions.ws_row, dimensions.ws_col);

    // 5. Deinitialize NCurses and close socket
    endwin();
    close(fd);

    return EXIT_SUCCESS;
}