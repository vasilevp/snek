#include <list>
#include <random>
#include <unistd.h>
#include <fcntl.h>

#include "position.hpp"
#include "entity.hpp"
#include "game.hpp"
#include "network.h"

using namespace std;

int main(int argc, char *argv[])
{
    int server_fd = start_server();

    for (;;)
    {
        int client_fd = accept_connection(server_fd);

        int pid = fork();
        if (pid != 0)
        {
            char str[1024];
            inet_ntop(AF_INET, &(address.sin_addr), str, INET_ADDRSTRLEN);
            printf("Accepted connection from %s, creating child process %d", str, pid);
            close(client_fd);
            continue;
        }

        // redirect output to network
        dup2(client_fd, STDOUT_FILENO);
        dup2(client_fd, STDIN_FILENO);

        // set the correct telnet parameters
        write(client_fd, "\377\375\042\377\373\001", 6);

        // read telnet greeting, ignore it
        char buf[1024];
        read(client_fd, buf, 1024);

        Game g;
        g.Run();

        close(client_fd);

        if (pid == 0)
        {
            exit(EXIT_SUCCESS);
        }
    }
}