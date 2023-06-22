#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#define PORT_NUM 4000
#define LISTEN_BACKLOG 100

typedef struct ChatUser
{
    int fd;
    char chat_name[100];
} ChatUser;

ChatUser register_list[100];
int register_size = 0;

// dang ki nguoi su dung
void add_service(int sockfd, char *chat_name)
{
    memset(register_list[register_size].chat_name, 0, sizeof(register_list[register_size].chat_name));
    register_list[register_size].fd = sockfd;
    strcpy(register_list[register_size].chat_name, chat_name);
    register_size++;
}

// xoa nguoi su dung
void remove_service(int sockfd)
{
    for (int i = 0; i < register_size; i++)
    {
        if (register_list[i].fd == sockfd)
        {
            for (int j = i; j < register_size - 1; j++)
            {
                register_list[j] = register_list[j + 1];
            }
            break;
        }
    }
    register_size--;
}

void show_service()
{
    system("clear");
    printf("%-5s%-20s%-20s\n", "STT", "File descriptor", "Chat name");
    for (int i = 0; i < register_size; i++)
    {
        printf("%-5d%-20d%-20s\n", i, register_list[i].fd, register_list[i].chat_name);
    }
}

void broadcast(int user_fd, char *message)
{
    char buffer[1000], name[100];
    uint16_t sent_byte;

    for (int i = 0; i < register_size; i++)
    {
        if (register_list[i].fd == user_fd)
        {
            strcpy(name, register_list[i].chat_name);
            break;
        }
    }

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%s: %s", name, message);
    sent_byte = strlen(buffer);

    for (int i = 0; i < register_size; i++)
    {
        if (register_list[i].fd != user_fd)
        {
            write(register_list[i].fd, &sent_byte, sizeof(sent_byte));
            write(register_list[i].fd, buffer, sent_byte);
        }
    }
}

void *socket_handler(void *fun_arg)
{
    int *client_fd = (int *)fun_arg, login_result;
    uint16_t sent_byte, receive_byte;
    char message[1000], username[100];

    printf("accept client: %d\n", *client_fd);

    memset(username, 0, sizeof(username));
    read(*client_fd, &receive_byte, sizeof(receive_byte));
    read(*client_fd, username, receive_byte);

    printf("name: %s\n", username);
    add_service(*client_fd, username);
    show_service();

    while (1)
    {
        int read_status;
        memset(message, 0, sizeof(message));
        if (read(*client_fd, &receive_byte, sizeof(receive_byte)) == 0)
        {
            break;
        }
        if (read(*client_fd, message, receive_byte) == 0)
        {
            break;
        }

        printf("%s: %s\n", username, message);
        broadcast(*client_fd, message);
    }

out:
    remove_service(*client_fd);
    show_service();
    close(*client_fd);
    free(client_fd);
	pthread_exit(NULL);
}

int main()
{
    int server_fd, len, otp;
    struct sockaddr_in server_addr, client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        fprintf(stderr, "create socket error\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NUM);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    otp = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &otp, sizeof(otp));

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        fprintf(stderr, "binding error\n");
        exit(0);
    }

    if (listen(server_fd, LISTEN_BACKLOG) == -1)
    {
        fprintf(stderr, "listen error\n");
        exit(0);
    }

    printf("listening at port %d\n", PORT_NUM);

    while (1)
    {
        int *pclient_fd = malloc(sizeof(int));
        pthread_t thread;
        *pclient_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (*pclient_fd == -1)
        {
            fprintf(stderr, "accept error\n");
            exit(0);
        }
        pthread_create(&thread, NULL, socket_handler, pclient_fd);
    }
    return 0;
}
