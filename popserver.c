#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

void list(char buf[], int smtp_sockfd, char user[100])
{
    // send +OK
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "+OK\r\n");
    send(smtp_sockfd, buf, strlen(buf), 0);

    char filename[100];
    memset(filename, 0, sizeof(filename));
    strcat(filename, user);
    strcat(filename, "/mymailbox.txt");

    // opening the corresponding user's mailbox
    FILE *fptr = fopen(filename, "r");
    if (fptr == NULL)
    {
        perror("Error opening mail file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    memset(buf, 0, sizeof(buf));
    int sno = 0;
    // sprintf(buf, "%d ", sno);
    char received[100];
    char emailid[100];
    char subject[100];
    char tosend[256];
    while (fgets(line, sizeof(line), fptr))
    {
        char *colon_pos = strstr(line, ":");
        char *start = colon_pos + 1;
        if (colon_pos != NULL)
        {
            char *start = colon_pos + 1;
            while (*start == ' ')
            {
                start++;
            }

            // check if line contains "From:"
            if (strstr(line, "From:") != NULL)
            {
                // sscanf(line, "From: %[^\n]", emailid);
                sscanf(start, "%[^\n]", emailid);
                emailid[strlen(emailid) - 1] = '\0';
                
            }
            // check if line contains "Recieved:"
            else if (strstr(line, "Recieved:") != NULL)
            {
                // sscanf(line, "Recieved: %[^\n]", received);
                sscanf(start, "%[^\n]", received);
                received[strlen(received) - 1] = '\0';
                
                sno++;

                sprintf(buf, "%d %s %s %s\r\n", sno, emailid, received, subject);
                send(smtp_sockfd, buf, strlen(buf), 0);
                

                memset(buf, 0, sizeof(buf));
                memset(received, 0, sizeof(received));
                memset(emailid, 0, sizeof(emailid));
                memset(subject, 0, sizeof(subject));
            }
            // check if line contains "Subject:"
            else if (strstr(line, "Subject:") != NULL)
            {
                // sscanf(line, "Subject: %[^\n]", subject);
                sscanf(start, "%[^\n]", subject);
                subject[strlen(subject) - 1] = '\0';
                
            }
        }
    }
    fclose(fptr);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, ".\r\n");
    send(smtp_sockfd, buf, strlen(buf), 0);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("run as: ./popserver <port_no>\n");
        exit(0);
    }

    int port_no = atoi(argv[1]);
    int clilen, smtp_sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    char buf[100];
    memset(buf, 0, sizeof(buf));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Socket creation failed.\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_no);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Unable to bind local address");
        exit(0);
    }

    listen(sockfd, 10);
    printf("POP3 Listening on port %d\n", port_no);

    while (1)
    {
        clilen = sizeof(cli_addr);
        smtp_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (smtp_sockfd < 0)
        {
            printf("Accept error\n");
            exit(0);
        }

        if (fork() == 0)
        {
            close(sockfd);

            // when connection established with client, send +OK message
            sprintf(buf, "+OK POP3 server ready for %d\r\n", inet_ntoa(cli_addr.sin_addr));
            send(smtp_sockfd, buf, strlen(buf), 0);

            // receving username from client (USER <username>)
            memset(buf, 0, sizeof(buf));
            recv(smtp_sockfd, buf, 100, 0);
            char user[100], pass[100];
            int j = 0;

            // AUTHORIZATION STATE
            if (strncmp(buf, "USER", 4) == 0)
            {
                for (int i = 4; i < strlen(buf); i++)
                {
                    if (buf[i] == ' ')
                        continue;
                    else
                    {
                        while (buf[i] != '\r')
                        {
                            user[j++] = buf[i++];
                        }
                        break;
                    }
                }
                user[j] = '\0';
                FILE *fptr = fopen("user.txt", "r");
                if (fptr == NULL)
                {
                    perror("Error opening user file");
                    exit(EXIT_FAILURE);
                }
                char line[256];
                int found = 0;
                while (fgets(line, sizeof(line), fptr))
                {
                    char *token = strtok(line, " ");
                    if (strcmp(token, user) == 0)
                    {
                        found = 1;
                        break;
                    }
                }
                fclose(fptr);

                if (found)
                {
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "+OK %s is a real hoopy frood\r\n", user);
                    send(smtp_sockfd, buf, strlen(buf), 0);
                }
                else
                {
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "-ERR Sorry no mailbox for %s here\r\n", user);
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    close(smtp_sockfd);
                    exit(0);
                }

                memset(buf, 0, sizeof(buf));
                recv(smtp_sockfd, buf, 100, 0);
                if (strncmp(buf, "PASS", 4) == 0)
                {
                    j = 0;
                    for (int i = 4; i < strlen(buf); i++)
                    {
                        if (buf[i] == ' ')
                            continue;
                        else
                        {
                            while (buf[i] != '\r')
                            {
                                pass[j++] = buf[i++];
                            }
                            break;
                        }
                    }
                    pass[j] = '\0';
                    

                    fptr = fopen("user.txt", "r");
                    if (fptr == NULL)
                    {
                        perror("Error opening user file");
                        exit(EXIT_FAILURE);
                    }
                    found = 0;
                    while (fgets(line, sizeof(line), fptr))
                    {
                        char *token = strtok(line, " ");
                        if (strcmp(token, user) == 0)
                        {
                            token = strtok(NULL, " ");
                            
                            token[strlen(token) - 1] = '\0';
                            if (strcmp(token, pass) == 0)
                            {
                                found = 1;
                                break;
                            }
                        }
                    }

                    fclose(fptr);
                    if (found)
                    {
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "+OK Mailbox open\r\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                    }
                    else
                    {
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "-ERR Invalid password\r\n");
                        printf("Error on PASS\n");
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        close(smtp_sockfd);
                        exit(0);
                    }
                }
                else
                {
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "-ERR Invalid command\r\n");
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    close(smtp_sockfd);
                    exit(0);
                }
            }
            else
            {
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "-ERR Invalid command\r\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                exit(0);
            }

            // receving STAT command from client
            memset(buf, 0, sizeof(buf));
            recv(smtp_sockfd, buf, 100, 0);
            printf("stat = %s\n", buf);
            int count = 0;
            if (strncmp(buf, "STAT", 4) == 0)
            {
                char filename[100];
                memset(filename, 0, sizeof(filename));
                strcat(filename, user);
                strcat(filename, "/mymailbox.txt");

                // opening the corresponding user's mailbox
                FILE *fptr = fopen(filename, "r");
                if (fptr == NULL)
                {
                    perror("Error opening mail file");
                    exit(EXIT_FAILURE);
                }

                // get the size of the mailbox in octets
                fseek(fptr, 0, SEEK_END);
                int size = ftell(fptr);
                fseek(fptr, 0, SEEK_SET);

                char line[256];

                while (fgets(line, sizeof(line), fptr))
                {
                    // if(strcmp(line, ".\r\n") == 0) count++;
                    if (strncmp(line, ".", 1) == 0)
                        count++;
                }

                fclose(fptr);
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "+OK %d %d\r\n", count, size);
                send(smtp_sockfd, buf, strlen(buf), 0);
            }
            else
            {
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "-ERR Invalid command\r\n");
                printf("Error on STAT\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                exit(0);
            }

            // receving LIST command from client
            memset(buf, 0, sizeof(buf));
            recv(smtp_sockfd, buf, 100, 0);
            printf("list = %s\n", buf);
            if (strncmp(buf, "LIST", 4) == 0)
            {
                list(buf, smtp_sockfd, user);
            }
            else
            {
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "-ERR Invalid command\r\n");
                printf("Error on LIST\n");
                send(smtp_sockfd, buf, strlen(buf), 0);
                close(smtp_sockfd);
                exit(0);
            }

            // looping until client sends QUIT command
            // server will be giving response to RETR, DELE, RSET
            int deleted[101] = {0}; // 0 if no mail corresponding to sno, 1 if mail, -1 if deleted
            for (int i = 1; i <= count; i++)
            {
                deleted[i] = 1;
            }
            while (1)
            {
                memset(buf, 0, sizeof(buf));
                recv(smtp_sockfd, buf, 100, 0);
                if (strncmp(buf, "QUIT", 4) == 0)
                {
                    // delete all marked emails and if success +OK else -ERR and then close the connection saying goodbye
                    char filename[100];
                    memset(filename, 0, sizeof(filename));
                    strcat(filename, user);
                    strcat(filename, "/mymailbox.txt");
                    FILE *old = fopen(filename, "r");
                    if (old == NULL)
                    {
                        perror("Error opening mail file");
                        exit(EXIT_FAILURE);
                    }
                    printf("filename = %s\n", filename);
                    // opening the corresponding user's mailbox
                    
                    char newf[100];
                    memset(newf, 0, sizeof(newf));
                    strcat(newf, user);
                    strcat(newf, "/newfile.txt");
                    
                    FILE *new = fopen(newf, "w");
                    if (new == NULL)
                    {
                        perror("Error opening new file");
                        exit(EXIT_FAILURE);
                    }
                    
                    for (int i = 1; i <= count; i++)
                    {   
                        printf("line = %d\n", i);
                        if (deleted[i] == -1)
                        {
                            continue;
                        }
                        char line[256];
                        int temp = 0;
                        
                        fseek(old, 0, SEEK_SET);
                        while (fgets(line, sizeof(line), old))
                        {
                            printf("out line = %s\n", line);
                            if (temp == i - 1)
                            {
                                // while(strcmp(line, ".\r\n") != 0){
                                //     // send(smtp_sockfd, line, strlen(line), 0);
                                //     fputs(line, new);
                                //     printf("line = %s\n", line);
                                //     memset(line, 0, sizeof(line));
                                //     fgets(line, sizeof(line), old);
                                // }
                                while (strncmp(line, ".", 1) != 0)
                                {
                                    // send(smtp_sockfd, line, strlen(line), 0);
                                    fputs(line, new);
                                    printf("line = %s\n", line);
                                    memset(line, 0, sizeof(line));
                                    fgets(line, sizeof(line), old);
                                }
                                fputs(line, new);
                                break;
                            }
                            // if(strcmp(line, ".\r\n") == 0){
                            //     temp++;
                            // }
                            if (strncmp(line, ".", 1) == 0)
                            {
                                temp++;
                            }
                        }


                        

                        
                    }
                    fclose(old);
                    fclose(new);
                    // incomplete
                    remove(filename);
                    rename(newf, filename);
                }
                else if (strncmp(buf, "LIST", 4) == 0)
                {
                    list(buf, smtp_sockfd, user);
                }
                else if (strncmp(buf, "RETR", 4) == 0)
                {
                    char mailno_s[4];
                    for (int i = 5; i < strlen(buf); i++)
                    {
                        mailno_s[i - 5] = buf[i];
                    }
                    mailno_s[strlen(mailno_s) - 1] = '\0';
                    int mailno = atoi(mailno_s);
                    printf("mailno = %d\n", mailno);
                    
                    if (deleted[mailno] == -1)
                    {
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "-ERR Mail %d was deleted\r\n", mailno);
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        continue;
                    }

                    // send +OK
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "+OK\r\n");
                    send(smtp_sockfd, buf, strlen(buf), 0);
                    char filename[100];
                    memset(filename, 0, sizeof(filename));
                    strcat(filename, user);
                    strcat(filename, "/mymailbox.txt");

                    // opening the corresponding user's mailbox
                    FILE *fptr = fopen(filename, "r");
                    if (fptr == NULL)
                    {
                        perror("Error opening mail file");
                        exit(EXIT_FAILURE);
                    }

                    // selecting the mail with corresponding mailno (sno)
                    char line[256];
                    int temp = 0;
                    while (fgets(line, sizeof(line), fptr))
                    {
                        
                        if (temp == mailno - 1)
                        {
                            while (strcmp(line, ".\r\n") != 0)
                            {
                                send(smtp_sockfd, line, strlen(line), 0);
                                
                                memset(line, 0, sizeof(line));
                                fgets(line, sizeof(line), fptr);
                            }
                            break;
                        }
                        if (strcmp(line, ".\r\n") == 0)
                        {
                            temp++;
                        }
                    }
                    fclose(fptr);
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, ".\r\n");
                    send(smtp_sockfd, buf, strlen(buf), 0);
                }
                else if (strncmp(buf, "DELE", 4) == 0)
                {
                    char mailno_s[4];
                    for (int i = 5; i < strlen(buf); i++)
                    {
                        mailno_s[i - 5] = buf[i];
                    }
                    mailno_s[strlen(mailno_s) - 1] = '\0';
                    int mailno = atoi(mailno_s);

                    if (deleted[mailno] == -1)
                    {
                        memset(buf, 0, sizeof(buf));
                        sprintf(buf, "-ERR %d already deleted\r\n", mailno);
                        send(smtp_sockfd, buf, strlen(buf), 0);
                        continue;
                    }

                    deleted[mailno] = -1;
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "+Ok mail %d deleted\r\n", mailno);
                    send(smtp_sockfd, buf, strlen(buf), 0);
                }
            }

            close(smtp_sockfd);
            exit(0);
        }

        close(smtp_sockfd);
    }

    return 0;
}