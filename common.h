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
#define MESSAGE_LOGOUT "logout"
#define MESSAGE_UNKNOWN_COMMAND "badCommand"
#define MESSAGE_PID_BAD "badPid"
#define MESSAGE_PID_OK "goodPid"
#define MESSAGE_USER_OK "userOk"
#define FIFO "fifoComm"


#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pwd.h>
#include <utmp.h>
#include <time.h>


typedef enum {
    LOGGED_IN,
    LOGGED_OUT,
    LOGIN_FAILED
} LoginState;

struct User{
    char pUsername[BUFFER_LENGTH];
    char pHostname[BUFFER_LENGTH];
    char pTime[BUFFER_LENGTH];
};

struct PidInfo{
    char pName[BUFFER_LENGTH];
    char pState[BUFFER_LENGTH];
    char pPpid[BUFFER_LENGTH];
    char pUid[BUFFER_LENGTH];
    char pVmsize[BUFFER_LENGTH];
};

static void getUserInfo(struct User  *user ){
    struct utmp *entry;
    time_t timestamp;
    utmpname("/var/log/wtmp");
    setutent();
    while((entry = getutent()) != NULL){
        if (entry->ut_type == USER_PROCESS ) {
            strcpy(user->pUsername, entry->ut_user);
            timestamp = entry->ut_tv.tv_sec;
            strcpy(user->pTime, asctime(localtime(&timestamp)));
        }
    }

    gethostname(user->pHostname, sizeof(user->pHostname));
    endutent();
}

static bool streq(const char * p1, const char * p2 ){
    return strcmp(p1, p2) == 0;
}

static void writeBuffer(int fd,  void* pBuf){
    __auto_type bufLen = strlen(pBuf);
    write(fd, & bufLen, 4);
    write(fd, pBuf, bufLen);
}

static void readBuffer(int fd, void * pBuf){
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
    char p[256];
    //memset(p, 0, BUFFER_LENGTH);
    strcpy(p, pBuf);
    return strtok(p, " ");
}

static int acquirePidStatus (pid_t pid, struct PidInfo *p){
    char pidStatusPath[BUFFER_LENGTH], entry[BUFFER_LENGTH];
    sprintf(pidStatusPath, "/proc/%d/status", pid);

    FILE * pidStatusFile = fopen(pidStatusPath, "r");

    if(pidStatusFile == NULL) return -1;

    while(! feof(pidStatusFile)){
        fgets(entry, BUFFER_LENGTH, pidStatusFile);
        removeEnter(entry);

        if(strstr(entry, "Name") == entry)      strcpy(p->pName, entry), printf("~~%s\n", p->pName);
        if(strstr(entry, "State") == entry)     strcpy(p->pState, entry), printf("~~%s\n", p->pState);
        if(strstr(entry, "PPid") == entry)      strcpy(p->pPpid, entry), printf("~~%s\n", p->pPpid);
        if(strstr(entry, "Uid") == entry)       strcpy(p->pUid, entry), printf("~~%s\n", p->pUid);
        if(strstr(entry, "VmSize") == entry)    strcpy(p->pVmsize, entry), printf("~~%s\n", p->pVmsize);
    }

    fclose(pidStatusFile);
    return 0;
}

#endif //RTL_T1_COMMON_H
