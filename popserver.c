#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("run as: ./popserver <port_no>\n");
        exit(0);
    }

    int port_no = atoi(argv[1]);
    int clilen, smtp_sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    char buf[100];
    memset(buf, 0, sizeof(buf));

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
    printf("POP3 Listening on port %d\n", port_no);

    while(1){
        clilen = sizeof(cli_addr);
        smtp_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
       
        if(smtp_sockfd < 0){
            printf("Accept error\n");
            exit(0);
        }

        if(fork() == 0){
            close(sockfd);

            // when connection established with client, send +OK message
            sprintf(buf, "OK+ POP3 server ready for %d\r\n", inet_ntoa(cli_addr.sin_addr));
            send(smtp_sockfd, buf, strlen(buf), 0);

            // receving username from client (USER <username>)
            memset(buf, 0, sizeof(buf));
            recv(smtp_sockfd, buf, 100, 0);
            char user[100], pass[100];
            int j = 0;

            // AUTHORIZATION STATE
            if(strncmp(buf, "USER", 4) == 0){
                for(int i=4; i<strlen(buf); i++){
                    if(buf[i] == ' ') continue;
                    else{
                        while(buf[i] != '\r'){
                            user[j++] = buf[i++];
                        }
                        break;
                    }
                }
                user[j] = '\0';
                FILE *fptr = fopen("user.txt", "r");
                if (fptr == NULL) {
                    perror("Error opening user file");
                    exit(EXIT_FAILURE);
                }
                char line[256];
                int found = 0;
                while (fgets(line, sizeof(line), fptr)) {
                    char *token = strtok(line, " ");
                    if(strcmp(token, user) == 0){
                        found = 1;
                        break;
                    }
                }
                fclose(fptr);

                if(found){
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "OK+ %s is a real hoopy frood\r\n", user);
                    send(smtp_sockfd, buf, strlen(buf), 0);

                }
                else{
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "ERR- Sorry no mailbox for %s here\r\n", user);
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    close(smtp_sockfd);
                    exit(0);
                }

                memset(buf, 0, sizeof(buf));
                recv(smtp_sockfd, buf, 100, 0);
                if(strncmp(buf, "PASS", 4) == 0){
                    j = 0;
                    for(int i=4; i<strlen(buf); i++){
                        if(buf[i] == ' ') continue;
                        else{
                            while(buf[i] != '\r'){
                                pass[j++] = buf[i++];
                            }
                            break;
                        }
                    }
                    pass[j] = '\0';

                    fptr = fopen("user.txt", "r");
                    if (fptr == NULL) {
                        perror("Error opening user file");
                        exit(EXIT_FAILURE);
                    }
                    found = 0;
                    while (fgets(line, sizeof(line), fptr)) {
                        char *token = strtok(line, " ");
                        if(strcmp(token, user) == 0){
                            token = strtok(NULL, " ");
                            if(strcmp(token, pass) == 0){
                                found = 1;
                                break;
                            }
                        }
                    }

                    fclose(fptr);
                    if(found){
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "OK+ Mailbox open\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                    }
                    else{
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "ERR- Invalid password\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        close(smtp_sockfd);
                        exit(0);
                    }
                }
                else{
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "ERR- Invalid command\r\n");
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    close(smtp_sockfd);
                    exit(0);
                }

            }
            else{
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "ERR- Invalid command\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                exit(0);
            }

            // receving STAT command from client
            memset(buf, 0, sizeof(buf));
            recv(smtp_sockfd, buf, 100, 0);
            int count = 0;
            if(strncmp(buf, "STAT", 4) == 0){
                char filename[100];
                memset(filename, 0, sizeof(filename));
                strcat(filename, user);
                strcat(filename, "/mymailbox.txt");

                // opening the corresponding user's mailbox
                FILE *fptr = fopen(filename, "r");
                if (fptr == NULL) {
                    perror("Error opening mail file");
                    exit(EXIT_FAILURE);
                }

                // get the size of the mailbox in octets
                fseek(fptr, 0, SEEK_END); 
                int size = ftell(fptr); 
                fseek(fptr, 0, SEEK_SET); 

                char line[256];
                
                while (fgets(line, sizeof(line), fptr)) {
                    if(strcmp(line, ".\n") == 0) count++;
                }
                
                fclose(fptr);
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "OK+ %d %d\r\n", count, size);
                send(smtp_sockfd, buf, strlen(buf), 0);
            }
            else{
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "ERR- Invalid command\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                exit(0);
            }

            // receving LIST command from client
            memset(buf, 0, sizeof(buf));
            recv(smtp_sockfd, buf, 100, 0);
            if(strncmp(buf, "LIST", 4) == 0){

                // send OK+
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "OK+\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                
                char filename[100];
                memset(filename, 0, sizeof(filename));
                strcat(filename, user);
                strcat(filename, "/mymailbox.txt");

                // opening the corresponding user's mailbox
                FILE *fptr = fopen(filename, "r");
                if (fptr == NULL) {
                    perror("Error opening mail file");
                    exit(EXIT_FAILURE);
                }

                char line[256];
                memset(buf, 0, sizeof(buf));
                int sno = 1;
                sprintf(buf, "%d", sno);
                char received[100];
                while (fgets(line, sizeof(line), fptr)) {
                    
                    if(strncmp(line, "From:", 5)){
                        char emailid[100];
                        j = 0;
                        for(int i=6; i<strlen(line); i++){
                            emailid[j++] = line[i];
                        }
                        emailid[j] = ' ';
                        // emailid[j+1] = '\0';
                        strcat(buf, emailid);
                    }

                    else if(strncmp(line, "Recieved:", 9)){
                        memset(buf, 0, sizeof(buf));
                        j = 0;
                        for(int i=10; i<strlen(line); i++){
                            received[j++] = line[i];
                        }
                        received[j] = ' ';
                        // received[j+1] = '\0';
                        // strcat(buf, received);
                    }

                    else if(strncmp(line, "Subject:", 8)){
                        char subject[100];
                        j = 0;
                        for(int i=9; i<strlen(line); i++){
                            subject[j++] = line[i];
                        }
                        subject[j] = ' ';
                        strcat(buf, received);
                        strcat(buf, subject);
                        strcat(buf, "\r\n");
                        sno++;
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "%d", sno);
                    }

                }
                fclose(fptr);
                memset(buf, 0, sizeof(buf));
                sprintf(buf, ".\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);


            }
            else{
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "ERR- Invalid command\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                exit(0);
            }

        }
    
    }

    return 0;
}