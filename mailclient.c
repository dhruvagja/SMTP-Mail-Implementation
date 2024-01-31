#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


int main(){
    int choice;

    printf("Choose from the following:\n");
    printf("1. Manage Mail\n");
    printf("2. Send Mail\n");
    printf("3. Quit\n");

    scanf("%d", &choice);


    switch(choice){
        case 1:
            printf("Manage Mail\n");
            break;
        case 2:
            printf("Send Mail\n");
            break;
        case 3:
            printf("Quit\n");
            break;
        default:
            printf("Invalid Choice\n");
            break;
    }
}