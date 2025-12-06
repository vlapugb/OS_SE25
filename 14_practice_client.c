#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

/* author: Vladimir Pugovkin */
int main()
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sockaddr = {};

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddr.sin_port = htons(8000);

    int ret_con = connect(client_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret_con == -1)
    {
        perror("Connect: ");
        return -1;
    }

    char get_http[] = "GET /\n\n";
    if (send(client_fd, get_http, sizeof(get_http), 0) < 0)
    {
        perror("Send: ");
        close(client_fd);
        return -1;
    }
    while (1)
    {
        char buff[1024];
        int received_b = recv(client_fd, buff, sizeof(buff), 0);
        if (received_b <= 0)
        {
            close(client_fd);
            return -1;
        }
        else
        {
            write(1, buff, received_b);
        }
    }

    return 0;
}
