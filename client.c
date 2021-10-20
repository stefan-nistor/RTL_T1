#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "common.h"

int main () {

    char inBuffer[BUFFER_LENGTH], outBuffer[BUFFER_LENGTH], consoleBuffer[BUFFER_LENGTH];

    int s = socket(
            AF_INET,
            SOCK_STREAM,
            0
    );

    struct sockaddr_in serverAddressInformation;

    serverAddressInformation.sin_port = htons(34000);
    serverAddressInformation.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddressInformation.sin_family = AF_INET;

    connect ( s, ( struct sockaddr * ) & serverAddressInformation,
              sizeof ( serverAddressInformation ) );

    char msg[256];
    int msglen;

/*
    strcpy ( msg, "Nistor" );
    msglen = strlen(msg);
    write ( s, & msglen, sizeof (int));
    write ( s, msg, msglen );

    read ( s, & msglen, sizeof(int) );
    read ( s, msg, msglen );
    printf("I received : %s\n", msg);
*/

    printf("Welcome! Please login.\n");
    while (true) {
        read(0, consoleBuffer, BUFFER_LENGTH);
        removeEnter(consoleBuffer);
        strcpy(outBuffer, consoleBuffer);

        printf("~consoleBuffer>%s~\n", consoleBuffer);
        writeBuffer(s, outBuffer);


        readBuffer(s, inBuffer);

        if(streq(inBuffer, MESSAGE_LOGIN_OK)) {
          printf("Login successful! ");
          printf("Available commands:\nget-logged-users\t|\tget-proc-info : <pid>\t|\tlogout\t|\tquit \n");
          fflush(stdout);
        }
        else if (streq(inBuffer, MESSAGE_LOGIN_NOK)){
            printf("Login unsuccessful. Access denied\n");
            fflush(stdout);
            continue;
        }
        else if (streq(inBuffer, MESSAGE_LOGIN_ALREADY)){
            printf("Already logged in.\n");
            fflush(stdout);
            continue;
        }
        else if (streq(inBuffer, MESSAGE_QUIT)){
            printf("Quit\n");
            exit(0);
        }
        else if (streq(inBuffer, MESSAGE_NOT_LOGGED_IN)){
            printf("Please login first.\n");
            continue;
        }
        else if (streq(inBuffer, MESSAGE_LOGOUT)){
            printf("Logged out. You can login again\n");
            fflush(stdout);
            continue;
        }
        else if (streq(inBuffer, MESSAGE_LOGOUT_NOK)){
            printf("Cannot log out\n");
            fflush(stdout);
            continue;
        }
        else if(streq(inBuffer, MESSAGE_UNKNOWN_COMMAND)){
            printf("Unknown command\n");
            fflush(stdout);
            continue;
        }

    }
    return 0;
}