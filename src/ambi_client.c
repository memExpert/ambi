#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/ambilight_socket"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Использование: %s <команда>\n", argv[0]);
        return 1;
    }

    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Ошибка создания сокета");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Ошибка подключения");
        return 1;
    }

    write(client_fd, argv[1], strlen(argv[1]));
    close(client_fd);
    return 0;
}