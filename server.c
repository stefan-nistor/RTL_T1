#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "common.h"



int main() {

    int sock[2];

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
        printf("#buf :%s\n", inBuffer);
        fflush(stdout);
        char p[256];
        strcpy(p, inBuffer);
        strcpy(command, acquireCommand(p));

        printf("#command :%s\n", command);
        fflush(stdout);

        if(strstr(p, "login : ") == p) // using socketpair
        {
            int retVal = socketpair(AF_UNIX, SOCK_STREAM, 0, sock);

            if(loginState == LOGGED_OUT || loginState == LOGIN_FAILED){
                pid_t childPid = fork();

                strcpy(param, acquireParam(p));

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
                     fflush(stdout);

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
        else if (streq(command, "quit"))    // using pipes
        {
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
            }
            else if (childPid > 0){
                close(fd1[0]);

                writeBuffer(fd1[1], command);
                readBuffer(fd2[0], response);

                if(streq(response, MESSAGE_QUIT)){
                    writeBuffer(c, MESSAGE_QUIT);
                    exit(0);
                }
                close(fd2[0]);
            }
            else {
                fprintf(stderr, "Error at fork\n");
                exit(1);
            }
        }

        else if ( streq ( command, "logout" ) ) {
            if ( -1 == mkfifo ( FIFO, 0666 ) ) {
                unlink ( FIFO );
                if ( -1 == mkfifo ( FIFO, 0666 ) ) {
                    fprintf(stderr, "Error at mkfifo\n");
                    exit(1);
                }
            }

            int pi[2];
            pipe(pi);
            pid_t childPid = fork ();

            if ( childPid == 0 ) {
                int fd = open ( FIFO, O_RDONLY, 0666 );

                close(pi[0]);
                char childBuf[BUFFER_LENGTH];
                readBuffer(fd, childBuf);


                printf("~%s~\n", childBuf);
                fflush(stdout);

                loginState == LOGGED_IN ? writeBuffer(pi[1], MESSAGE_LOGOUT) : writeBuffer(pi[1], MESSAGE_LOGOUT_NOK);

                close(pi[1]);
                close(fd);

                exit(0);
            }
            else if ( childPid > 0 ) {
                int fd = open ( FIFO, O_WRONLY, 0666 );
                close(pi[1]);

                writeBuffer(fd, command);
                readBuffer(pi[0], response);

                printf("~A%s~\n", response);
                fflush(stdout);

                if(streq(response, MESSAGE_LOGOUT)) {
                    writeBuffer(c, MESSAGE_LOGOUT);
                    loginState = LOGGED_OUT;

                    close (pi[0]);
                    close(fd);
                    waitpid( childPid, NULL, 0 );

                    continue;
                }

                else if(streq(response, MESSAGE_LOGOUT_NOK))
                    writeBuffer(c, MESSAGE_LOGOUT_NOK);


                close (pi[0]);
                close(fd);
                waitpid( childPid, NULL, 0 );
            } else {
                fprintf(stderr, "Error at fork\n");
                exit(1);
            }
        }

//        else if(streq(command, "logout"))   // using fifo
//        {
//            if (-1 == mkfifo(FIFO, 0666)){
//                fprintf(stderr, "Error at mkfifo\n");
//                exit(1);
//            }
//
//            pid_t childPid = fork();
//            if(childPid == 0){
//                if(-1 == open(FIFO, O_RDONLY)){
//                    fprintf(stderr, "Failed to open read end of external pipe\n");
//                    exit(1);
//                }
//
//            }
//
//        }

        else if (streq(command, "get-logged-users")){
            pid_t childPid = fork();

            if (childPid == 0){
                close(sock[1]);
                char childBuf[BUFFER_LENGTH];
                readBuffer(sock[0], childBuf);

                if(loginState != LOGGED_IN){
                    writeBuffer(sock[0], MESSAGE_NOT_LOGGED_IN);
                }
                else{

                }
            }
            else if (childPid > 0){
                close(sock[0]);

                writeBuffer(sock[1], command);
                readBuffer(sock[1], response);
            }

        }
        else{
            writeBuffer(c, MESSAGE_UNKNOWN_COMMAND);
            continue;
        }
     }

    close(c);
    close(s);

    return 0;
}