#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h> 

int waitForKeypress(int timeout_sec)
{
    static struct termios oldt, newt;
    fd_set fdset;
    struct timeval timeout;
    int status;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO );
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    status = select (STDIN_FILENO+1, &fdset, NULL, NULL, &timeout);
    if (status == 1) getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return status;
}

