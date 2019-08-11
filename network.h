#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static struct sockaddr_in address;
static socklen_t addrlen = sizeof(address);

int start_server()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

int accept_connection(int server_fd)
{
    int client_fd = 0;
    if (client_fd = accept(server_fd, (sockaddr *)&address, &addrlen); client_fd < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return client_fd;
}
