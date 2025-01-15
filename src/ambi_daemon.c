#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#include "../include/config.h"

void handle_client(int client_fd) {
    char buffer[256];
    read(client_fd, buffer, sizeof(buffer));
    printf("Command received: %s\n", buffer);
    close(client_fd);
}

int main() {
    struct tm* tm_ptr;
    time_t current_time;


    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error: socket is not created.");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error: can't link socket.\n");
        return 1;
    }

    listen(server_fd, 5);
    printf("ambi_daemon started successfully.\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Error accept");
            continue;
        }
        handle_client(client_fd);

        current_time = time(NULL);
        tm_ptr = localtime(&current_time);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}