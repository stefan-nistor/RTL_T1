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

    char msg[256], response[512];
    int msglen;

    while (true){
        readBuffer(c, inBuffer);
        removeEnter(inBuffer);
        char * p[256];
        strcpy(p, inBuffer);
        strcpy(command, acquireCommand(p));

        //printf("~%s~\n", command);
        //fflush(stdout);

        if(strstr(p, "login") == p){
            if(loginState == LOGGED_OUT || loginState == LOGIN_FAILED){
                pid_t childPid = fork();

                strcpy(param, acquireParam(p));
                printf("~#%s~\n", param);
                //fflush(stdout);

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
                     }
                     close(sock[1]);

                     waitpid(childPid, NULL, 0);
                 } else{
                     fprintf(stderr, "Could not fork process\n");
                     return 1;
                 }
             } else if(loginState == LOGGED_IN){
                 writeBuffer(c, MESSAGE_LOGIN_ALREADY);
                 continue;
             }
         }
        else if (streq(command, "quit")){
            int fd1[2], fd2[2];

            if(pipe(fd1) == -1){
                fprintf(stderr, "Error at pipe\n");
                exit(1);
            }
            if(pipe(fd2) == -1){
                fprintf(stderr, "Error at pipe\n");
                exit(1);
            }

            pid_t childPid = fork();

            if(childPid == 0){
                close(fd1[1]);
                char childBuf[BUFFER_LENGTH];
                readBuffer(fd1[0], childBuf);

                close(fd1[0]);
                close(fd2[0]);

                writeBuffer(fd2[1], MESSAGE_QUIT);

                close(fd2[1]);
            } else if (childPid > 0){
                close(fd1[0]);

                writeBuffer(fd1[1], command);
                readBuffer(fd2[0], response);

                if(streq(response, MESSAGE_QUIT)){
                    writeBuffer(c, MESSAGE_QUIT);
                    exit(0);
                }
                close(fd2[0]);
            } else {
                fprintf(stderr, "Error at fork\n");
                exit(1);
            }


        }
     }

    close(c);
    close(s);

    return 0;
}