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

// Function to list the mails in the given fomat in a temprory file
void listing(){
    FILE * list = fopen("list.txt", "w");
    FILE * temp = fopen("temp.txt", "r");

    char line[256];
    int sno = 0;
    char received[100];
    char emailid[100];
    char subject[100];
    char tosend[256];

    while(fgets(line, sizeof(line), temp)){
        char *colon_pos = strstr(line, ":");

        char *start = colon_pos + 1;
        if(colon_pos != NULL){
            char *start = colon_pos + 1;
            while(*start == ' '){
                start++;
            }


            if(strstr(line, "From:")!= NULL){
                sscanf(start, "%[^\n]", emailid);
                emailid[strlen(emailid) - 1] = '\0';

            }

            else if(strstr(line, "Recieved:")!= NULL){
                sscanf(start, "%[^\n]", received);
                received[strlen(received) - 1] = '\0';


                sno++;
                fprintf(list, "%d %s %s %s\n", sno, emailid, received, subject);
                
            }

            else if(strstr(line, "Subject:")!= NULL){
                sscanf(start, "%[^\n]", subject);
                subject[strlen(subject) - 1] = '\0';
            }
        }
    }

    fclose(temp);
    remove("temp.txt");
    fclose(list);
    return;
}


// Function to check if the given string is in the format of "xxxxxxxx@xxxxxx"
int check_xy(char str[], char *username){
    
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


// Function to replace '\n' with '\r\n' in the given buffer
void replace(char *buffer){
    size_t len = strlen(buffer);
    if(buffer[len-1] == '\n'){
        buffer[len-1] = '\r';
        buffer[len] = '\n';
        buffer[len+1] = '\0';

    }else{
        buffer[len] = '\r';
        buffer[len+1] = '\n';
        buffer[len+2] = '\0';
    }

    return;

}

int main(int argc, char *argv[]){

    if(argc != 4){
        printf("run as: ./mailclient <server_ip> <smtp_port> <pop3_port>\n");
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

        // If use wants to manage mail
        if(choice == 1){
            int sockfd;
            struct sockaddr_in server_addr;
            //Creating socket
            if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                perror("Error in creating socket\n");
                exit(0);
            }

            server_addr.sin_family = AF_INET;
            inet_aton(server_ip, &server_addr.sin_addr);
            server_addr.sin_port = htons(pop3_port);
            //Connecting to server
            if((connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0){
                perror("Unable to connect to server\n");
                exit(0);
            }

            char buffer[MAXLINE];
            memset(buffer, 0, MAXLINE);

            ssize_t len = recv(sockfd, buffer, MAXLINE, 0);
            //printf("S : %s", buffer);
            
            if(strncmp(buffer, "+OK", 3) != 0){
                printf("Error in connecting to POP3 server\n");
                close(sockfd);
                continue;
            }
            //Sending username and password
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "USER ");
            strcat(buffer, user);
            strcat(buffer, "\r\n");
            send(sockfd, buffer, strlen(buffer), 0);

            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);
            printf("S : %s", buffer);

            if(strncmp(buffer, "-ERR", 4) == 0){
                printf("Invalid user\n");
                close(sockfd);
                return 0;
            }

            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "PASS ");
            strcat(buffer, pass);
            strcat(buffer, "\r\n");
            send(sockfd, buffer, strlen(buffer), 0);

            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);
            //Checking password
            if(strncmp(buffer, "-ERR", 4) == 0){
                printf("Invalid password\n");
                close(sockfd);
                return 0;
            }

            //Using command STAT to get the number of mails
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "STAT\r\n");
            send(sockfd, buffer, strlen(buffer), 0);
            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);

            //printf("S : %s", buffer);
            int n;
            char temp[MAXLINE];

            int i = 3;
            int z = 0;
            for(i=0; i<strlen(buffer); i++){
                if(buffer[i] == ' '){
                    i++;
                    while(buffer[i] != ' '){
                        temp[z++] = buffer[i++];
                    }
                    break;
                }
            }
            // printf("temp = %s\n", temp);
            // strcat(temp, "\0");
            n = atoi(temp);
            int deleted[n];
            for(int i = 0; i<n; i++){
                deleted[i] = 0;
            }
            printf("\nYou have %d mails\n\n", n);

            //Using command RETF to recieve all mails to create list in the given format
            memset(buffer, 0, MAXLINE);
            FILE *file = fopen("temp.txt", "w");
            for(int i = 0; i< n; i++){
                if(deleted[i] == 1){
                    continue;
                }
                char tempchar[10];
                memset(buffer, 0, MAXLINE);
                strcpy(buffer, "RETR ");
                sprintf(tempchar,"%d", i+1);
                strcat(buffer, tempchar);
                strcat(buffer, "\r\n");

                send(sockfd, buffer, strlen(buffer), 0);
                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);
                if(strncmp(buffer, "-ERR", 4) == 0){
                    printf("Error in retrieving mail\n");
                    remove("list.txt");
                    close(sockfd);
                    continue;
                }
                
                while(1){
                    memset(buffer, 0, MAXLINE);
                    len = recv(sockfd, buffer, MAXLINE, 0);

                    for(int j = 0; j<len ; j++){
                       
                        if(buffer[j] == '.' && (j+1 < len) && buffer[j+1] == '\r'&& (j+2 < len) && buffer[j+2] == '\n'){
                            fputs(buffer, file);
                            goto next3;
                        }
                        else if(buffer[j] == '\r' && (j+1)<len && buffer[j+1] == '\n' && j+2 < len &&buffer[j+2] == '.'){
                            fputs(buffer, file);
                            goto next3;
                        }
                        else if(buffer[j] == '\n' && (j+1)<len && buffer[j+1] == '.'){
                            fputs(buffer, file);
                            goto next3;
                        }
                    }
                    
                    fputs(buffer, file);
                }
                next3:
            }
            fclose(file);
            //Creating list of mails
            listing();

            //
            int input;
            int delete[n+1];
            for(int i = 1; i<=n; i++){
                delete[i] = 0;
            }

            while(1){
                
                FILE *list = fopen("list.txt", "r");
                char line[256];

                for(int i = 1; i<=n ; i++){
                    fgets(line,sizeof(line), list);
                    if(delete[i] == 1){
                        continue;
                    }
                    //memset(line, 256, 0);
                    
                    printf("%s", line);

                }
                fclose(list);

                printf("\nEnter the mail no. to see: ");
                scanf("%d", &input);
                // if input is -1 then quit
                if(input == -1){
                    memset(buffer, 0, MAXLINE);
                    strcpy(buffer, "QUIT\r\n");

                    send(sockfd, buffer, strlen(buffer), 0);
                    break;
                }
                // Check for valid input
                while(input < 1 || input > n){
                    printf("Invalid input\n");
                    printf("\nEnter the mail no. to see: ");
                    scanf("%d", &input);
                    if(input > 0 && input <=n){
                        break;
                    }
                }

                
                // Using command RETR to recieve the mail body
                memset(buffer, 0, MAXLINE);
                strcpy(buffer, "RETR ");
                memset(temp, 0, MAXLINE);
                sprintf(temp, "%d", input);
                strcat(buffer, temp);

                strcat(buffer, "\r\n");
                send(sockfd, buffer, strlen(buffer), 0);

                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);
                printf("S : %s", buffer);
                if(strncmp(buffer, "-ERR", 4) == 0){
                    printf("Error in retrieving mail\n");
                    continue;
                    
                }
                // Recieveing mail body
                while(1){
                    
                    memset(buffer, 0, MAXLINE);
                    len = recv(sockfd, buffer, MAXLINE, 0);

                    for(int j = 0; j<len ; j++){
                        if(buffer[j] == '.' && (j+1 < len) && buffer[j+1] == '\r'&& (j+2 < len) && buffer[j+2] == '\n'){
                            memset(temp, 0, MAXLINE);
                            strncpy(temp, buffer, j);
                            printf("%s\n", buffer);
                            goto next1;
                        }
                        else if(buffer[j] == '\r' && (j+1)<len && buffer[j+1] == '\n' && j+2 < len &&buffer[j+2] == '.'){
                            memset(temp, 0, MAXLINE);
                            strncpy(temp, buffer, j);
                            printf("%s\n", buffer);
                            goto next1;
                        }
                        else if(buffer[j] == '\n' && (j+1)<len && buffer[j+1] == '.'){
                            memset(temp, 0, MAXLINE);
                            strncpy(temp, buffer, j);
                            printf("%s\n", buffer);
                            goto next1;
                        }
                    }
                    
                    printf("%s", buffer);
                }
                next1:

                
                printf("Enter character 'd' to delete above mail\n");
                while((getchar()) != '\n');
                char inputchar;
                inputchar = getchar();

                if(inputchar == 'd'){
                    memset(buffer, 0, MAXLINE);
                    //Using command DELE to delete the mail
                    strcpy(buffer, "DELE ");
                    memset(temp, 0, MAXLINE);
                    sprintf(temp, "%d", input);
                    strcat(buffer, temp);
                    strcat(buffer, "\r\n");
                    send(sockfd, buffer, strlen(buffer), 0);

                    memset(buffer, 0, MAXLINE);
                    len = recv(sockfd, buffer, MAXLINE, 0);
                    //printf("%s", buffer);
                    if(strncmp(buffer, "-ERR", 4) == 0){
                        printf("Error in deleting mail\n");
                        remove("list.txt");
                        close(sockfd);
                        continue;
                    }
                    if(strncmp(buffer, "+OK", 3) == 0){
                        printf("Mail deleted\n");
                        //printf("input = %d\n", input);
                        delete[input] = 1;
                    }

                }


            }

            if(input == -1){

                close(sockfd);
                remove("list.txt");
                continue;
            }
             

        }
        //If user wants to send mail
        else if(choice == 2){
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
            //checking the format of the mail
            char domain[MAXLINE];
            for(int j = 0; j< MAXLINE; j++){
                if(from[j] == '@'){
                    int k;
                    for(k = j+1; k< strlen(from); k++){
                        if(from[k] == '\n'){
                            domain[k-j-1] = '\0';
                            break;
                        }
                        domain[k-j-1] = from[k];
                    }
                    if(domain[k - j -1] == '\0'){
                        break;
                    }
                }

            }

            if(strcmp(domain, server_ip) != 0){
                printf("Domain of the sender should be the ip address fo the server\n");
                continue;
            }
            //Checking the format of from, to and subject
            if((strncmp(from, "From:", 5) != 0) || (strncmp(to, "To:", 3) != 0) || (strncmp(subject, "Subject:", 8) != 0)){
                printf("Incorrect format\n");
                continue;
            }

            int pos = 0;
            char from_username[MAXLINE], to_username[MAXLINE];
            for(int j = 6; j<strlen(from); j++){
                if(from[j] != ' '){
                    pos = j;
                    break;
                }
            }
            
            strncpy(from_username, from+pos, strlen(from)-pos-1);
            if(!check_xy(from_username,user)){
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
            memset(to_username, 0, MAXLINE);
            strncpy(to_username, to+pos, strlen(to)-pos-1);
            if(!check_xy(to_username,to_username)){
                printf("Incorrect format\n");
                continue;
            }
            
            char buffer[MAXLINE];
            memset(buffer, 0, MAXLINE);
            ssize_t len = recv(sockfd, buffer, MAXLINE, 0);
            // Send command HELO to server to identity
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "HELO ");
            strcat(buffer, server_ip);
            strcat(buffer, "\r\n");
            send(sockfd, buffer, strlen(buffer), 0);
            
            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);
            
            memset(buffer, 0, MAXLINE);
            // Send command MAIL to server to verify senders mail
            strcpy(buffer, "MAIL ");
            strcat(buffer, "FROM: <");
            char cleaned_from[MAXLINE];
            memset(cleaned_from, 0, MAXLINE);
            strcat(cleaned_from, user);
            strcat(cleaned_from, "@");
            strcat(cleaned_from, server_ip);
            
            strcat(buffer, cleaned_from);
            strcat(buffer, ">");
            strcat(buffer, "\r\n");

            send(sockfd, buffer, strlen(buffer), 0);
            
            memset(buffer, 0, MAXLINE);
            len = recv(sockfd, buffer, MAXLINE, 0);
            //Sending command RCPT to server to verify recievers mail
            memset(buffer, 0, MAXLINE);
            strcpy(buffer, "RCPT TO: ");
            char cleaned_to[MAXLINE];
            memset(cleaned_to, 0, MAXLINE);
            strcat(cleaned_to,"<");
            strcat(cleaned_to, to_username);
            strcat(cleaned_to, "@");
            strcat(cleaned_to, server_ip);
            strcat(cleaned_to, ">");
            strcat(buffer, cleaned_to);
            strcat(buffer, "\r\n");
            send(sockfd, buffer, strlen(buffer), 0);
            
            memset(buffer, 0, MAXLINE);

            len = recv(sockfd, buffer, MAXLINE, 0);
            
            char temp[4];
            strncpy(temp, buffer,3);
            int code = atoi(temp);

            if(code == 550){
                printf("Error in sending mail: %s\n", buffer);
                close(sockfd);
                continue;
            }else if(code == 250){
                
                memset(buffer, 0, MAXLINE);

                //Sending command DATA to server to send the mail
                strcpy(buffer, "DATA\r\n");
                send(sockfd, buffer, strlen(buffer), 0);
                

                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);
                
                replace(from);
                send(sockfd, from, strlen(from), 0);
                
                replace(to);
                send(sockfd, to, strlen(to), 0);
                
                replace(subject);
                send(sockfd, subject, strlen(subject), 0);
                
                for(int j = 0; j<=i; j++){
                    memset(buffer, 0, MAXLINE);
                    strcpy(buffer, body[j]);
                    for(int j = 0; j<MAXLINE; j++){
                        if(buffer[j] == '\n'){
                            buffer[j] = '\r';
                            buffer[j+1] = '\n';
                            break;
                        }
                    }
                    send(sockfd, buffer, strlen(buffer), 0);
                }


                memset(buffer, 0, MAXLINE);
                len = recv(sockfd, buffer, MAXLINE, 0);
                memset(buffer, 0, MAXLINE);
                //Sending command QUIT to server to quit
                strcpy(buffer, "QUIT\r\n");
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