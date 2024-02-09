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


        if(fork() == 0){
            close(sockfd);
            printf("Connection established with %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            
            // send the welcome message
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "220 <%s> Service ready\r\n", inet_ntoa(cli_addr.sin_addr));
            send(smtp_sockfd, buf, strlen(buf), 0);
            memset(buf, 0, sizeof(buf));

            int hello = 0, mail = 0, rcpt = 0, data = 0, quit = 0;

            char target_username[100];
            while(1){
                memset(buf, 0, sizeof(buf));
                ssize_t len = recv(smtp_sockfd, buf, 100, 0);
                
                // if HELO message sent by client
                if(strncmp(buf, "HELO", 4) == 0){
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "250 OK Hello %s\r\n", inet_ntoa(cli_addr.sin_addr));
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    hello = 1;
                }
                // if MAIL message sent by client
                else if(strncmp(buf, "MAIL", 4) == 0){
                    // if HELO message is not sent by client
                    if(hello == 0){
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "503 Bad sequence of commands\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        continue;
                    }
                    char sending_user[100];  
                    // storing sending_user
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
                    mail = 1;
                }
                // if RCPT message sent by client
                else if(strncmp(buf, "RCPT", 4) == 0){
                    // if MAIL command is not sent by the client
                    if(mail == 0){
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "503 Bad sequence of commands\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        continue;
                    }
                    // storing target_username
                    memset(target_username, 0, sizeof(target_username));
                    int i;
                    for(i = 0; i < strlen(buf); i++){
                        if(buf[i] >= 'a' && buf[i] <= 'z'){
                            int j = 0;
                            while(buf[i] != '@' && buf[i] != ' '){
                                target_username[j++] = buf[i++];
                            }
                            target_username[j] = '\0';
                            break;
                        }
                    }
                    // opening the file containing the list of users
                    FILE *fptr = fopen("user.txt", "r");
                    if (fptr == NULL) {
                        perror("Error opening user file");
                        exit(EXIT_FAILURE);
                    }
                    char line[256];
                    int found = 0;

                    // checking if the recipient is present in the list of users
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
                        rcpt = 1;
                    }
                    // if not found, close the connection and force client to retake the mail from the sender
                    else{
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "550 No such user\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        continue;
                    }
                }
                // if DATA message sent by client
                else if(strncmp(buf,"DATA", 4) == 0){
                    // if RCPT command is not sent by the client
                    if(rcpt == 0){
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "503 Bad sequence of commands\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        continue;
                    }
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "354 Enter mail, end with \".\" on a line by itself\r\n");
                    send(smtp_sockfd, buf, strlen(buf), 0);

                    char filename[200];
                    memset(filename, 0, sizeof(filename));
                    // strcat(filename, BASE_DIR);
                    strcat(filename, target_username);
                    strcat(filename, "/mymailbox.txt");
                    
                    FILE *file = fopen(filename, "a");
                    if (file == NULL) {
                        perror("Error opening mailbox file");
                        exit(EXIT_FAILURE);
                    }
                    int count = 0;

                    // timestamp
                    time_t time_now;
                    struct tm * timeinfo;
                    time(&time_now);
                    timeinfo = localtime(&time_now);
                    char timestring[50];
                    char recieved[100];
                    strftime(timestring, sizeof(timestring), "%d-%m-%Y %H:%M:%S", timeinfo);
                    memset(recieved, 0, sizeof(recieved));
                    sprintf(recieved, "Recieved: %s", timestring);

                    // receiving the mail and storing in the mymailbox.txt file corresponding to the target user
                    while (1) {
                        memset(buf, 0, sizeof(buf));
                        ssize_t n = recv(smtp_sockfd, buf, 100, 0);
                        buf[n] = '\0';

                        int flag = 1;

                        if (n <= 0) {
                            break;  // Connection closed or error
                        }
                        for(int i = 0; i < n; i++){
                            if(buf[i] == '\n'){
                                count++;
                                if(count == 3){
                                    flag = 0;
                                    fprintf(file, "%.*s", i+1, buf);
                                    fprintf(file, "%s\n", recieved);
                                    fprintf(file, "%s", buf+i+1);
                                    
                                }
                            }
                            if(buf[i] == '\r' &&(i+2 < n)&& buf[i+1] == '\n' && buf[i+2] == '.'){
                                if(flag)fprintf(file, "%s", buf);
                                goto exit;
                            }
                            else if(buf[i] == '\n' && (i+1 < n) &&buf[i+1] == '.'){
                                if(flag)fprintf(file, "%s", buf);
                                goto exit;
                            }
                            else if(buf[i] == '.' && (i+1 < n) && buf[i+1] == '\r'){
                                if(flag)fprintf(file, "%s", buf);
                                goto exit;
                            }
                        }

                        if(flag){
                            fprintf(file, "%s", buf);
                        }
                        flag = 1;
                    }
                    exit:
                    fclose(file);

                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "250 OK Message accepted for delivery\r\n");
                    send(smtp_sockfd, buf, strlen(buf), 0);
                }
                else if(strncmp(buf, "QUIT", 4) == 0){
                    // if DATA command is not sent by the client
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "221 %s closing connection\r\n", inet_ntoa(cli_addr.sin_addr));
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    
                    close(smtp_sockfd);
                    exit(0);
                }
            }
        }


        close(smtp_sockfd);
        
    }
}