#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAXLINE 100

int check_xy(char *str, char *username){
    
    int cnt = 0, space = 0;

    for(int i = 0; i<strlen(str); i++){
        if(str[i] == '@'){
            cnt++;
        }
        if(str[i] == ' '){
            space++;
        }
    }

    if(cnt != 1 || space != 0){
        return 0;
    }

    char *x = strtok(str, "@");
    char *y = strtok(NULL, "@");

    if(strlen(x) == 0 || strlen(y) == 0){
        return 0;
    }

    if(strcmp(x,username) != 0){
        return 0;
    }
    return 1;

}

int main(int argc, char *argv[]){

    if(argc != 4){
        return 0;
    }
    char *server_ip = argv[1];
    int smtp_port = atoi(argv[2]);
    int pop3_port = atoi(argv[3]);


    char user[MAXLINE], pass[MAXLINE];

    printf("Enter username: ");
    scanf("%s", user);
    printf("Enter password: ");
    scanf("%s", pass);
    

    while(1){
        int choice;

        printf("Choose from the following:\n");
        printf("1. Manage Mail\n");
        printf("2. Send Mail\n");
        printf("3. Quit\n");
        printf("Choice = ");
        scanf("%d", &choice);

        if(choice == 1){
            continue;
        }else if(choice == 2){
            int sockfd;
            struct sockaddr_in server_addr;

            if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                perror("Error in creating socket\n");
                exit(0);
            }

            server_addr.sin_family = AF_INET;
            inet_aton(server_ip, &server_addr.sin_addr);
            server_addr.sin_port = htons(smtp_port);


            if((connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0){
                perror("Unable to connect to server\n");
                exit(0);
            }


            char from[MAXLINE], to[MAXLINE], subject[MAXLINE], body[50][MAXLINE];

            printf("****Type the mail****\n");
            fflush(stdout);
            fgets(body[0], MAXLINE,stdin);
            fgets(from, MAXLINE,stdin);
            fgets(to, MAXLINE,stdin);
            fgets(subject, MAXLINE,stdin);
            int i = -1;
            
            do{ 
                i++;
                fgets(body[i], MAXLINE,stdin);
            }while(strcmp(body[i], ".\n") != 0);
            
            if((strncmp(from, "From:", 5) != 0) || (strncmp(to, "To:", 3) != 0) || (strncmp(subject, "Subject:", 8) != 0)){
                printf("Incorrect format\n");
                continue;
            }

            int pos = 0;
            char str[MAXLINE];
            for(int j = 6; j<strlen(from); j++){
                if(from[j] != ' '){
                    pos = j;
                    break;
                }
            }
            strncpy(str, from+pos, strlen(from)-pos-1);
            if(!check_xy(str,username)){
                printf("Incorrect format\n");
                continue;
            }
            pos = 0;
            for(int j = 3; j<strlen(to); j++){
                if(to[j] != ' '){
                    pos = j;
                    break;
                }
            }
            memset(str, 0, MAXLINE);
            strncpy(str, to+pos, strlen(to)-pos-1);
            if(!check_xy(str,username)){
                printf("Incorrect format\n");
                continue;
            }
            
            char buffer[MAXLINE];
            memset(buffer, 0, MAXLINE);
            ssize_t len = recv(sockfd, buffer, MAXLINE, 0);
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "HELO ");
            strcat(buffer, server_ip);
            strcat(buffer, "\n");
            send(sockfd, buffer, strlen(buffer), 0);


            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "MAIL ");
            strcat(buffer, from);
            strcat(buffer, "\n");

            send(sockfd, buffer, strlen(buffer), 0);
            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "RCPT ");
            strcat(buffer, to);
            strcat(buffer, "\n");
            send(sockfd, buffer, strlen(buffer), 0);
            memset(buffer, 0, MAXLINE);

            len = recv(sockfd, buffer, MAXLINE, 0);

            int code = atoi(buffer, 3);

            if(code == 550){
                printf("Error in sending mail: %s\n", buffer);
                close(sockfd);
                continue;
            }else if(code == 250){
                
                memset(buffer, 0, MAXLINE);
                strcpy(buffer, "DATA\0");
                send(sockfd, buffer, strlen(buffer), 0);

                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);

                memset(buffer, 0, MAXLINE);
                send(sockfd, from, strlen(from), 0);
                send(sockfd, to, strlen(to), 0);
                send(sockfd, subject, strlen(subject), 0);
                for(int j = 0; j<=i; j++){
                    send(sockfd, body[j], strlen(body[j]), 0);
                }


                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);
                memset(buffer, 0, MAXLINE);
                strcpy(buffer, "QUIT\0");
                send(sockfd, buffer, strlen(buffer), 0);
                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);


                close(sockfd);
                printf("Mail sent successfully\n");

            }





        }else if(choice == 3){
            break;
        }


    }
        return 0;
}