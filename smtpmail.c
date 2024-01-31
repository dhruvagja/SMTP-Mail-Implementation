#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BASE_DIR "./Networks-Lab/"

int main(int argc, char *argv[]){
    int port_no = atoi(argv[1]);
    int clilen, smtp_sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    char buf[100];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("Socket creation failed.\n");
        exit(0);
    }
    
    serv_addr.sin_family = AF_INET;   
    serv_addr.sin_port = htons(port_no);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("Unable to bind local address");
        exit(0);
    }

    listen(sockfd, 10);
    printf("SMTP Listening on port %d\n", port_no);

    int pid;
    while(1){
        clilen = sizeof(cli_addr);
        smtp_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(smtp_sockfd < 0){
            printf("Accept error\n");
            exit(0);
        }

        if((pid = fork()) == 0){
            close(sockfd);

            printf("Connection established with %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

            // handling the client request
            while (1) {
                ssize_t n = recv(smtp_sockfd, buf, sizeof(buf) - 1, 0);
                if (n <= 0) {
                    break;  // Connection closed or error
                }

                buf[n] = '\0';
                int i;
                char username[50];
                if(buf[0] == 'T' && buf[1] == 'o'){
                    for(i = 0; i < strlen(buf); i++){
                        if(buf[i] == ':'){
                            break;
                        }
                    }
                    i++;
                    
                    int j = 0;
                    while(buf[i] != '@'){
                        username[j] = buf[i];
                        i++;
                        j++;
                    }
                    username[j] = '\0';
                    printf("Username: %s\n", username);
                }

                if (strcmp(buf, ".\r\n") == 0) {
                    break;
                }

                char *filename = malloc(strlen(BASE_DIR) + strlen(username) + strlen("/mymailbox.txt") + 1);
                
                snprintf(filename, sizeof(filename), "%s%s/mymailbox.txt", BASE_DIR, username);
                printf("Filename: %s\n", filename);
                FILE *file = fopen(filename, "a");
                if (file == NULL) {
                    perror("Error opening mailbox file");
                    exit(EXIT_FAILURE);
                }

                fprintf(file, "%s", buf);

                fclose(file);
            }

            close(smtp_sockfd);
            exit(0);
        }

        close (smtp_sockfd);
    }
}