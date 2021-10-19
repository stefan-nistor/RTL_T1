#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
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
    while (true) {
        printf("Welcome! Please login.\n");
        read(0, consoleBuffer, BUFFER_LENGTH);

        strcpy(outBuffer, consoleBuffer);
        writeBuffer(s, outBuffer);

        readBuffer(s, inBuffer);

        if(streq(inBuffer, MESSAGE_LOGIN_OK)) {
          printf("Login successful!\n");
          fflush(stdout);
          break;
        }
    }
    return 0;
}