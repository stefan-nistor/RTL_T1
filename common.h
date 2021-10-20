//
// Created by stefan on 19.10.2021.
//

#ifndef RTL_T1_COMMON_H
#define RTL_T1_COMMON_H
#define BUFFER_LENGTH 256
#define MESSAGE_LOGIN_OK "loginOK"
#define MESSAGE_LOGIN_NOK "loginNOK"
#define MESSAGE_LOGIN_ALREADY "loginAlready"
#define MESSAGE_QUIT "quit"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

static bool streq(const char * p1, const char * p2 ){
    return strcmp(p1, p2) == 0;
}

static void writeBuffer(int fd, const char* pBuf){
    __auto_type bufLen = strlen(pBuf);
    write(fd, & bufLen, 4);
    write(fd, pBuf, bufLen);
}

static void readBuffer(int fd, char * pBuf){
    memset(pBuf, 0, BUFFER_LENGTH);
    int bufLen;
    int retVal = read(fd, & bufLen, 4);
    read(fd, pBuf, bufLen);
}

static void removeEnter(const char * pBuf){
    char * pEndl = strchr(pBuf, '\n');
    if(pEndl != NULL)
        * pEndl = '\0';
}

static bool usernameOk (const char * user){
    FILE * usernameFile = fopen("../users.txt", "r");
    char userInFile[BUFFER_LENGTH];

    if(usernameFile == NULL) return false;

    while (! feof(usernameFile)){
        fgets( userInFile, BUFFER_LENGTH, usernameFile);
        removeEnter(userInFile);

        if(streq(user, userInFile)){
            fclose(usernameFile);
            return true;
        }
    }
    fclose(usernameFile);
    return false;
}

static char * acquireParam(const char * pBuf){
    char * p = strstr(pBuf, " : ");
    if(p==NULL) return NULL;

    p+=3;
    while( p!=NULL && (*p == ' ' || *p == '|')) p++;
    return p;
}

static char* removeLeadingSpaces(char * pBuf){
    if(pBuf == NULL)    return NULL;
    while (*pBuf == ' ') {
        pBuf++;
    }
    return pBuf;
}

static char* acquireCommand (char *pBuf){
    char *p;
    //memset(p, 0, BUFFER_LENGTH);
    strcpy(p, pBuf);
    return strtok(p, " ");
}

#endif //RTL_T1_COMMON_H
