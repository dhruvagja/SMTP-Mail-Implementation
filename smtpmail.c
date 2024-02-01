#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

#define BASE_DIR "./Networks-Lab/"

int main(int argc, char *argv[]){

    if(argc != 2){
        printf("run as: ./smtpmail <port_no>\n");
        exit(0);
    }

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

    listen(sockfd, 5);
    printf("SMTP Listening on port %d\n", port_no);

    int pid;
    while(1){
        clilen = sizeof(cli_addr);
        smtp_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        printf("hi\n");
        if(smtp_sockfd < 0){
            printf("Accept error\n");
            exit(0);
        }

        if(fork() == 0){
            close(sockfd);

            // when established the connection, send 220 message to client
            printf("Connection established with %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "220 <%s> Service ready\r\n", inet_ntoa(cli_addr.sin_addr));
            send(smtp_sockfd, buf, strlen(buf), 0);
            memset(buf, 0, sizeof(buf));
            ssize_t len = recv(smtp_sockfd, buf, strlen(buf), 0);
            buf[len] = '\0';

            // receiving HELO message from client
            if(buf[0] == 'H' && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'O'){
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "250 OK Hello %s\r\n", inet_ntoa(cli_addr.sin_addr));
                send(smtp_sockfd, buf, strlen(buf), 0);
            }
            // not sure we need to implement error code 500
            // else{
            //     memset(buf, 0, sizeof(buf));
            //     sprintf(buf, "500 Error: bad syntax\n");
            //     send(smtp_sockfd, buf, strlen(buf), 0);
            // }
            
            // recieving MAIL FROM: 
            len = recv(smtp_sockfd, buf, strlen(buf), 0);
            buf[len] = '\0';
            char sending_user[100];  
            for(int i = 0; i < strlen(buf); i++){
                if(buf[i] == '<'){
                    i++;
                    int j = 0;
                    while(buf[i] != '>'){
                        sending_user[j++] = buf[i++];
                    }
                    sending_user[j] = '\0';
                    break;
                }
            }
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "250<%s>... Sender ok\r\n", sending_user);
            send(smtp_sockfd, buf, strlen(buf), 0);

            // receiving RCPT TO:
            memset(buf, 0, sizeof(buf));
            len = recv(smtp_sockfd, buf, sizeof(buf), 0);
            buf[len] = '\0';
            char target_user[100];

            // storing target_user
            for(int i = 0; i < strlen(buf); i++){
                if(buf[i] >= 'a' && buf[i] <= 'z'){
                    int j = 0;
                    while(buf[i] != '\r'){
                        target_user[j++] = buf[i++];
                    }
                    target_user[j] = '\0';
                    break;
                }
            }

            // storing target_username
            char target_username[50];
            int i;
            for(i = 0; i < strlen(target_user); i++){
                if(target_user[i] == '@'){
                    break;
                }
                target_username[i] = target_user[i];
            }
            target_username[i] = '\0';

            // search for target_username in user.txt file
            // char *filename_ = malloc(strlen(BASE_DIR) + strlen("user.txt") + 1);
            // snprintf(filename_, sizeof(filename_), "%suser.txt", BASE_DIR);
            FILE *fptr = fopen("user.txt", "r");
            if (fptr == NULL) {
                perror("Error opening user file");
                exit(EXIT_FAILURE);
            }
            char line[256];
            int found = 0;
            while (fgets(line, sizeof(line), fptr)) {
                char *token = strtok(line, " ");
                if(strcmp(token, target_username) == 0){
                    found = 1;
                    break;
                }
            }
            fclose(fptr);

            // if recipient is found
            if(found == 1){
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "250 root... Recipient ok\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
            }
            // if not found, close the connection and force client to retake the mail from the sender
            else{
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "550 %s No such user\r\n", target_user);
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                continue;
            }
            
            // receiving "DATA" message from client and start receiving the mail
            memset(buf, 0, sizeof(buf));
            len = recv(smtp_sockfd, buf, strlen(buf), 0);
            buf[len] = '\0';
            if(strcmp(buf, "DATA\r\n") == 0){
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "354 Enter mail, end with \".\" on a line by itself\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
            }

            // opening the mymailbox.txt file corresponding to the target user
            char *filename = malloc(strlen(BASE_DIR) + strlen(target_username) + strlen("/mymailbox.txt") + 1);
            snprintf(filename, sizeof(filename), "%s%s/mymailbox.txt", BASE_DIR, target_username);
            printf("Filename: %s\n", filename);
            FILE *file = fopen(filename, "a");
            if (file == NULL) {
                perror("Error opening mailbox file");
                exit(EXIT_FAILURE);
            }

            // receiving the mail and storing in the mymailvox.txt file corresponding to the target user
            while (1) {
                memset(buf, 0, sizeof(buf));
                ssize_t n = recv(smtp_sockfd, buf, strlen(buf), 0);
                // check
                // buf[n] = '\0';

                int flag = 0;
                char recieved[100];

                // is To: is found, then add the recieved time to the mail
                if(buf[0] == 'T' && buf[1] == 'O' && buf[2] == ':'){
                    flag = 1;
                    time_t time_now;
                    struct tm * timeinfo;

                    time(&time_now);
                    timeinfo = localtime(&time_now);

                    char timestring[50];
                    strftime(timestring, sizeof(timestring), "%d-%m-%Y %H:%M:%S", timeinfo);

                    memset(recieved, 0, sizeof(recieved));
                    sprintf(recieved, "Recieved: %s", timestring);
                }

                if (n <= 0) {
                    break;  // Connection closed or error
                }

                if (strcmp(buf, ".\r\n") == 0) {
                    fprintf(file, "%s", buf);
                    break;
                }

                fprintf(file, "%s", buf);
                if(flag) fprintf(file, "%s", recieved);
            }

            fclose(file);
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "250 OK Message accepted for delivery\r\n");
            send(smtp_sockfd, buf, strlen(buf), 0);

            len = recv(smtp_sockfd, buf, strlen(buf), 0);
            buf[len] = '\0';
            if(strcmp(buf, "QUIT\r\n") == 0){
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "221 %s closing connection\r\n", inet_ntoa(cli_addr.sin_addr));
                send(smtp_sockfd, buf, strlen(buf), 0);
            }
            close(smtp_sockfd);
            exit(0);
        }

        close (smtp_sockfd);
    }
}