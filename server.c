#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

typedef enum {
    LOGGED_IN,
    LOGGED_OUT,
    LOGIN_FAILED
} LoginState;

int main() {

    int sock[2];
    int retVal = socketpair(AF_UNIX, SOCK_STREAM, 0, sock);

    char inBuffer[BUFFER_LENGTH], outBuffer[BUFFER_LENGTH], consoleBuffer[BUFFER_LENGTH];
    char command[BUFFER_LENGTH], param[BUFFER_LENGTH];
    LoginState loginState = LOGGED_OUT;
    int s = socket(
            AF_INET,
            SOCK_STREAM,
            0
    );

    int toggle = 1;
    setsockopt (
            s,
            SOL_SOCKET,
            SO_REUSEADDR,
            & toggle,
            sizeof ( toggle )
    );

    struct sockaddr_in serverAddressInformation;

    serverAddressInformation.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddressInformation.sin_port = htons(34000);
    serverAddressInformation.sin_family = AF_INET;

    bind (
            s,
            ( struct sockaddr * ) & serverAddressInformation,
            sizeof ( serverAddressInformation )
    );

    listen ( s, 10 );

    struct sockaddr_in clientAddressInformation;
    socklen_t len = sizeof ( struct sockaddr_in );

    int c = accept (
            s,
            ( struct sockaddr * ) & clientAddressInformation,
            & len
    );

    char msg[256];
    int msglen;
    char response[512];

//    read ( c, & msglen, sizeof(int) );
//    read ( c, msg, msglen );
//
//    sprintf(response, "Hello %s you dumb idiot", msg);
//
//    msglen = strlen( response );
//    write( c, & msglen, sizeof (int));
//    write( c, & response, msglen );


    while (true){
        readBuffer(c, inBuffer);

        removeEnter(inBuffer);
        //__auto_type p = removeLeadingSpaces(inBuffer);
        char * p[256];
        strcpy(p, inBuffer);
        strcpy(command, acquireCommand(p));

        printf("~%s~\n", command);
        fflush(stdout);

        if(strstr(p, "login") == p){
             if(loginState == LOGGED_OUT || loginState == LOGIN_FAILED){

                 //int sock[2];

                 strcpy(param, acquireParam(p));

                 pid_t childPid = fork();

                 if (childPid == 0) {
                     close(sock[1]);

                     char childBuf [BUFFER_LENGTH];

                     readBuffer(sock[0], childBuf);

                     usernameOk(param) ? writeBuffer(sock[0], MESSAGE_LOGIN_OK) : writeBuffer(sock[0], MESSAGE_LOGIN_NOK);


                     close(sock[0]);
                     exit(0);
                 }else if (childPid > 0){

                     close(sock[0]);
                     writeBuffer(sock[1], param);
                     readBuffer(sock[1], response);
                     if(streq(MESSAGE_LOGIN_OK, response)){
                         loginState = LOGGED_IN;
                         writeBuffer(c, MESSAGE_LOGIN_OK);
                     } else if (streq(MESSAGE_LOGIN_NOK, response)){
                         loginState = LOGIN_FAILED;
                         writeBuffer(c, MESSAGE_LOGIN_NOK);
                         break;
                     }

                     close(sock[1]);

                     waitpid(childPid, NULL, 0);
                 }
                 else{
                     fprintf(stderr, "Could not fork process");
                     return 1;
                 }
             }
         }

     }

    close(c);
    close(s);

    return 0;
}