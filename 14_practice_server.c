#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/poll.h>
/* author - Vladimir Pugovkin */
#define MAX_CONNECTIONS 100

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sockaddr = {};

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddr.sin_port = htons(8000);

    int ret_con = bind(server_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret_con == -1)
    {
        perror("Bind: ");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("Listen: ");
        close(server_fd);
        return -1;
    }

    struct pollfd pfd[MAX_CONNECTIONS];

    for (int i = 0; i < MAX_CONNECTIONS; ++i)
    {
        pfd[i].fd = -1;
        pfd[i].events = POLLIN;
        pfd[i].revents = 0;
    }

    pfd[0].fd = server_fd;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;

    int nfds = 1;

    while (1)
    {

        int n = poll(pfd, nfds, -1);
        if (n <= 0)
            continue;

        if (pfd[0].revents & POLLIN)
        {
            socklen_t tt = sizeof(sockaddr);
            int client_socket = accept(server_fd, (struct sockaddr *)&sockaddr, &tt);
            if (client_socket == -1)
            {
                perror("Accept: ");
            }
            else
            {
                if (nfds < MAX_CONNECTIONS)
                {
                    pfd[nfds].fd = client_socket;
                    pfd[nfds].events = POLLIN;
                    pfd[nfds].revents = 0;

                    ++nfds;
                }
                else
                {
                    close(client_socket);
                }
                pfd[0].revents = 0;
            }
        }

        for (int i = 1; i < nfds;)
        {
            if (pfd[i].revents & POLLIN)
            {
                char buff[1024];
                int received = recv(pfd[i].fd, buff, sizeof(buff), 0);
                if (received <= 0)
                {
                    close(pfd[i].fd);
                    pfd[i] = pfd[nfds - 1];
                    pfd[i].revents = 0;
                    --nfds;
                    continue;
                }
                send(pfd[i].fd, buff, received, 0);
            }
            pfd[i].revents = 0;
            ++i;
        }
    }
    close(server_fd);
    return 0;
}
