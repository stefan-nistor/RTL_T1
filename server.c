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
    char response[BUFFER_LENGTH];
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

    while (true){
        readBuffer(c, inBuffer);
        removeEnter(inBuffer);

        char p[256];
        strcpy(p, inBuffer);
        strcpy(command, acquireCommand(p));

        // using socketpair
        if(strstr(p, "login : ") == p){
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
                    readBuffer(sock[1], response);
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
        // using pipes
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
        // using fifo
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

                loginState == LOGGED_IN ? writeBuffer(pi[1], MESSAGE_LOGOUT) : writeBuffer(pi[1], MESSAGE_LOGIN_NOK);

                close(pi[1]);
                close(fd);

                exit(0);
            }
            else if ( childPid > 0 ) {
                int fd = open ( FIFO, O_WRONLY, 0666 );
                close(pi[1]);

                writeBuffer(fd, command);
                readBuffer(pi[0], response);

                if(streq(response, MESSAGE_LOGOUT)) {
                    writeBuffer(c, MESSAGE_LOGOUT);
                    loginState = LOGGED_OUT;

                    close (pi[0]);
                    close(fd);
                    waitpid( childPid, NULL, 0 );

                    continue;
                }

                else if(streq(response, MESSAGE_LOGIN_NOK))
                    writeBuffer(c, MESSAGE_LOGIN_NOK);

                close (pi[0]);
                close(fd);
                waitpid( childPid, NULL, 0 );
            } else {
                fprintf(stderr, "Error at fork\n");
                exit(1);
            }
        }

        else if (streq(command, "get-logged-users")){
            int retVal = socketpair(AF_UNIX, SOCK_STREAM, 0, sock);
            if(loginState == LOGGED_IN) {
                pid_t childPid = fork();
                struct User user;

                if (childPid == 0) {
                    close(sock[1]);

                    char childBuf[BUFFER_LENGTH];
                    readBuffer(sock[0], childBuf);

                    getUserInfo(&user);

                    if(&user != NULL) {
                        writeBuffer(sock[0], MESSAGE_USER_OK);

                        writeBuffer(sock[0], user.pUsername);

                        writeBuffer(sock[0], user.pHostname);

                        writeBuffer(sock[0], user.pTime);
                    }
                    close(sock[0]);
                    exit(0);
                } else if (childPid > 0) {
                    close(sock[0]);

                    writeBuffer(sock[1], command);

                    readBuffer(sock[1], response);

                    if(streq(response, MESSAGE_USER_OK)){
                        //MESSAGE_USER_OK
                        writeBuffer(c, response);
                        //Name
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                        //Hostname
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                        //Time
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                    }

                    close(sock[1]);
                    waitpid(childPid, NULL, 0);
                }
                else{
                    fprintf(stderr, "Fork error\n");
                    exit(1);
                }
            }
            else{
                writeBuffer(c, MESSAGE_LOGIN_NOK);
                continue;
            }
        }

        else if(strstr(p, "get-proc-info : ") == p){
            int retVal = socketpair(AF_UNIX, SOCK_STREAM, 0, sock);
            if(loginState == LOGGED_IN){

                pid_t childPid = fork();
                struct PidInfo pidInfo;

                strcpy(param, acquireParam(p));

                if(childPid == 0){
                    close(sock[1]);

                    char childBuf[BUFFER_LENGTH];
                    readBuffer(sock[0], childBuf);

                    if (acquirePidStatus(strtol(childBuf, NULL, 10), &pidInfo) == 0){
                        writeBuffer(sock[0], MESSAGE_PID_OK);
                        writeBuffer(sock[0], pidInfo.pName);
                        writeBuffer(sock[0], pidInfo.pState);
                        writeBuffer(sock[0], pidInfo.pPpid);
                        writeBuffer(sock[0], pidInfo.pUid);
                        writeBuffer(sock[0], pidInfo.pVmsize);
                    }
                    else
                        writeBuffer(sock[0],MESSAGE_PID_BAD);

                    close(sock[0]);
                    exit(0);
                }
                else if (childPid > 0){
                    close(sock[0]);


                    writeBuffer(sock[1], param);

                    readBuffer(sock[1], response);

                    if(streq(response, MESSAGE_PID_OK)) {
                        // MESSAGE_PID_OK
                        writeBuffer(c, response);
                        // Name
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                        // State
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                        // PPid
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                        // Uid
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                        // VmSize
                        readBuffer(sock[1], response);
                        writeBuffer(c, response);
                    }

                    if(streq(response, MESSAGE_PID_BAD)){
                        writeBuffer(c, MESSAGE_PID_BAD);
                        continue;
                    }

                    close(sock[1]);
                    waitpid(childPid, NULL, 0);
                }
            }
            else{
                writeBuffer(c, MESSAGE_LOGIN_NOK);
                continue;
            }
        }
        else if(loginState == LOGGED_IN){
            writeBuffer(c, MESSAGE_UNKNOWN_COMMAND);
            continue;
        }
        else{
            writeBuffer(c, MESSAGE_LOGIN_NOK);
        }
     }

    close(c);
    close(s);

    return 0;
}