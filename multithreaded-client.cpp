#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
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

#define max(a, b) ((a) > (b) ? (a) : (b))
std::mutex screen;

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

void read_server(int fd, unsigned short rows, unsigned short columns)
{
    WINDOW *window = newwin(rows / 2, columns, 0, 0);
    const char *header = "MESSAGES:";
    int lineno = 0;

    {
        std::unique_lock<std::mutex> lock(screen);

        mvwaddstr(window, 1, 1, header);
        box(window, 0, 0);
        wrefresh(window);
    }

    char buffer[MESSAGE_LENGTH];
    bool done = false;

    while (not done)
    {
        // 1. Read a message from the server (blocks until a message can be read)
        if (read(fd, buffer, MESSAGE_LENGTH) == 0)
        {
            {
                std::unique_lock<std::mutex> lock(screen);

                wprintw(window, "ERROR: server terminated");
                exit(EXIT_FAILURE);
            }
        }

        // 2. Null-terminate the buffer before printing
        buffer[MESSAGE_LENGTH - 1] = '\0';

        // 3. Print the message to the current line
        {
            std::unique_lock<std::mutex> lock(screen);

            mvwprintw(window, next_line(window, header, &lineno, rows / 2 - 1), 1, "Server: >%s<\n", buffer);
            wrefresh(window);
        }
    }

    // 4. Delete the window, including its border
    {
        std::unique_lock<std::mutex> lock(screen);

        wborder(window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh(window);
        delwin(window);
    }
}

void write_server(int fd, unsigned short rows, unsigned short columns)
{
    WINDOW *window = newwin(rows / 2 - 1, columns, rows / 2 + 1, 0);
    const char *header = "INPUT:";
    int lineno = 0;

    {
        std::unique_lock<std::mutex> lock(screen);

        mvwaddstr(window, 1, 1, header);
        box(window, 0, 0);
        wrefresh(window);
    }

    char buffer[MESSAGE_LENGTH]{'\0'};
    bool done = false;

    while (not done)
    {
        // 1. Null-terminate the buffer before reading input
        buffer[MESSAGE_LENGTH - 1] = '\0';

        // 2. Read user input
        {
            std::unique_lock<std::mutex> lock(screen);
            mvwgetnstr(window, next_line(window, header, &lineno, rows / 2 - 2), 1, buffer, MESSAGE_LENGTH);
        }

        // 3. Write to the server
        write(fd, buffer, strlen(buffer) + 1);

        // 4. Update the window
        {
            std::unique_lock<std::mutex> lock(screen);

            wclear(window);
            mvwaddstr(window, 1, 1, header);
            box(window, 0, 0);
            wrefresh(window);
        }
    }

    {
        std::unique_lock<std::mutex> lock(screen);

        wborder(window, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh(window);
        delwin(window);
    }
}

int main()
{
    std::vector<std::thread> threads;
    sockaddr_in server_adddress;
    winsize dimensions;
    int fd;

    // 1. Initalize NCurses
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &dimensions);
    initscr();

    // 2. Socket
    errchk ( fd = socket(AF_INET, SOCK_STREAM, 0), "socket");

    // 3. Connect
    bzero(&server_adddress, sizeof(server_adddress));
    server_adddress.sin_family = AF_INET;
    server_adddress.sin_port = htons(PORT);
    errchk( inet_pton(AF_INET, LOCALHOST, &server_adddress.sin_addr), "inet_pton" );

    errchk (connect(fd, (sockaddr *)&server_adddress, sizeof(server_adddress)), "connect");

    // 4. Echo
    threads.push_back(std::thread(read_server, fd, dimensions.ws_row, dimensions.ws_col));
    threads.push_back(std::thread(write_server, fd, dimensions.ws_row, dimensions.ws_col));

    // 5. Join threads
    for (std::thread &thread : threads)
    {
        thread.join();
    }

    // 6. Deinitialize NCurses and close socket
    endwin();
    close(fd);

    return EXIT_SUCCESS;
}