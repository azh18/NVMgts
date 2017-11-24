#include <iostream>
#include <sys/shm.h> // symphony header
#include <sys/socket.h> //socket header
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/time.h> // accurate timer
#include <netinet/in.h>
#include "const.hpp"
#include <string>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>


using namespace std;

typedef struct SMDATA
{
    double dataDou[4];
    int type;
    char stringData[10000];
    bool flag[3];
    int dataInt[4];
} SMDATA;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int creat_sem(key_t key);
int set_semvalue(int semid);
int sem_p(int semid);
int sem_v(int semid);
int del_sem(int semid);
int socketMsgHandler(char* msg, int client_fd);
double getTimeus();

void* shm[SMTYPE_NUM] = {NULL};
SMDATA *shared[SMTYPE_NUM] = {NULL};
int shmid[SMTYPE_NUM];
int semid[SMTYPE_NUM];
int nowState = -1;// 0-7 is real state; -1 is not do anything..
FILE *p1;

int main()
{
    cout << "Hello world!" << endl;
    FILE* pTemp;
    pTemp = fopen("RangeQueryResult.txt","w+");
    fprintf(pTemp, "runrunrun\n");
    fclose(pTemp); // to clean up the RangeQueryResult.txt
    //-------------------------------------------------
    // initial shm and semaphore
    for(int i=0; i<SMTYPE_NUM; i++)
    {
        shmid[i] = shmget((key_t)i+1,sizeof(SMDATA),0666|IPC_CREAT);
        if(shmid[i] == -1)
        {
            // fail
            printf("failed to create shared memory :%d",shmid[i]);
        }
        shm[i] = shmat(shmid[i],0,0);
        if(shm[i]==(void*)-1)
        {
            printf("shmat err\n");
        }
        shared[i] = (SMDATA*)shm[i];
        // initial semaphore
        semid[i] = creat_sem((key_t)i);
        set_semvalue(semid[i]);
        shared[i]->flag[0] = false;
        shared[i]->flag[1] = false;
        shared[i]->flag[2] = false;
    }
    shared[4]->dataInt[3] = -1;
    //------------------------------------------------------------
    // initial socket

    int client_fd;
    struct sockaddr_in ser_addr;
    struct sockaddr_in cli_addr;
    char buffer[1024]; // buffer
    int ser_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // non-block mode
    if(ser_sockfd<0)
    {
        // fail
        printf("fail to create socket.\n");
    }

    //bzero(&ser_addr, addrlen);
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(SERVER_PORT);
    int on = 1;
    setsockopt(ser_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    if(bind(ser_sockfd, (struct sockaddr*)&ser_addr,sizeof(struct sockaddr_in))<0)
    {
        printf("bind error.\n");
    }
    if(listen(ser_sockfd,5)<0)
    {
        printf("listen fail.\n");
        close(ser_sockfd);
    }
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int closeMe = 0;
    int numTrajs = 0,numPoints = 0;
    printf("tes>>>>%d\n",EWOULDBLOCK);
    p1 = popen("/home/zbw/NVMgts_Interative/NVMgts/testGTS > run.log","r");
    while(true)
    {
        client_fd = accept4(ser_sockfd, (struct sockaddr*)&cli_addr, &addrlen,SOCK_NONBLOCK);

        if(client_fd<0)
        {
            if(errno == EWOULDBLOCK)
            {
                sleep(1);
                continue;
            }
            else
            {
                printf("Accept error: %d .\n",client_fd);
                close(ser_sockfd);
            }
        }
        else
            break;
    }



    // start service.
    char sendMsg[1024];
    nowState = -1;
    //system("/home/zbw/NVMgts_Interative/NVMgts/testGTS &");
    //system("chmod 777 /home/zbw/NVMgts_Interative/NVMgts/testGTS");
    //p1 = popen("/home/zbw/NVMgts_Interative/NVMgts/testGTS >> log0.txt","r");
    //system("/home/zbw/NVMgts_Interative/NVMgts/testGTS >> log.txt &");
    while(!closeMe)
    {
        // shm 0
        //--------------------------------------------------------
        // load data
        bool haveFinisedLoad, haveStartedLoad;
        if(sem_p(semid[0]))
        {
            printf("sem_p fail.\n");
        }

        haveStartedLoad = shared[0]->flag[1];
        haveFinisedLoad = shared[0]->flag[0];

            if(haveFinisedLoad)
            {
                numTrajs = shared[0]->dataInt[0];
                numPoints = shared[0]->dataInt[1];
                double startLoadTime,endLoadTime;
                startLoadTime = shared[0]->dataDou[0];
                endLoadTime = shared[0]->dataDou[1];
                // send info about num through socket

                char tempStr[1024];
                // int strLen = 0+2+2;
                memset(sendMsg,0,sizeof(sendMsg));
                strcat(sendMsg, "0;");
                sprintf(tempStr, "%d:", numTrajs);
                strcat(sendMsg,tempStr);
                sprintf(tempStr, "%d:",numPoints);
                strcat(sendMsg,tempStr);
                sprintf(tempStr, "%lf:", (endLoadTime - startLoadTime)/1000000);
                strcat(sendMsg,tempStr);
                strcat(sendMsg, ";");
                strcat(sendMsg, "1;|");
                send(client_fd, sendMsg, strlen(sendMsg) +1, 0); // send message

                // after finish loading, set flag to false
                shared[0]->flag[0] = false;
                shared[0]->flag[1] = false;
            }
        if(sem_v(semid[0]))
        {
            printf("sem_v fail.\n");
        }
        // shm 1
        //-----------------------------------------------------
        // batch from gts to client
        if(sem_p(semid[1]))
        {
            printf("sem_p fail.\n");
        }
        bool allFinish,finishOne;
        allFinish = shared[1]->flag[0];
        finishOne = shared[1]->flag[1];
        if(finishOne)
        {
            // only if this is true, batch query need to be check
            int finishNum = shared[1]->dataInt[0];
            double runningTimeOne = shared[1]->dataDou[3] - shared[1]->dataDou[2];
            printf("Task %d finished using %lf s.\n",finishNum,runningTimeOne/1000000);
            // send this message through socket
            //.......
            // maybe not need to return through socket because of the high latency of internet
            shared[1]->flag[1] = false;
            if(allFinish)
            {
                double runningTimeBatch = shared[1]->dataDou[1] - shared[1]->dataDou[0];
                printf("All queries are finished using %lf s.\n",runningTimeOne/1000000);
                // send this message through socket
                // for SENDBatch, format of msg is 1;time:;finishFlag=1;
                char tempStr[1024];
                // int strLen = 0+2+2;
                memset(sendMsg,0,sizeof(sendMsg));
                strcat(sendMsg, "1;");
                if(shared[1]->dataInt[1]==1)
                {
                    sprintf(tempStr, "%lf:-1:", runningTimeBatch/1000000); // if in demo, the time not represent the efficiency
                    shared[1]->dataInt[1] = 0;
                }
                else
                {
                    sprintf(tempStr, "%lf:", runningTimeBatch/1000000);
                }
                strcat(sendMsg,tempStr);
                strcat(sendMsg, ";");
                strcat(sendMsg, "1;|");
                send(client_fd, sendMsg, strlen(sendMsg) +1, 0); // send message
                //........
                shared[1]->flag[0] = false;
            }

        }
        // batch from client to gts is implemented in socket

        if(sem_v(semid[1]))
        {
            printf("sem_v fail.\n");
        }


        // shm 2
        //-----------------------------------------------------
        // single from gts to client
        if(sem_p(semid[2]))
        {
            printf("sem_p fail.\n");
        }
        allFinish = shared[2]->flag[0];
        if(allFinish)
        {
            double runningTime = shared[2]->dataDou[1] - shared[2]->dataDou[0];
            printf("This query is finished using %lf s.\n", runningTime/1000000);
            // send this msg through socket
            //......
            //char result[2*1024*1024];
            char result[1000];
            // size_t fileLength = shared[2]->dataInt[1];
            sprintf(result, "Result Number: %d, some results are displayed:\n", shared[2]->dataInt[1]);
            strcat(result, shared[2]->stringData);
            if(sem_v(semid[2]))
            {
                printf("sem_v fail.\n");
            }
            //FILE *resultFile = fopen(fileName, "r+");
            //int readBytes = fread(result, sizeof(char), fileLength, resultFile);
            //fclose(resultFile);

            memset(sendMsg, 0, sizeof(sendMsg));
            strcat(sendMsg, "2;");
            strcat(sendMsg, result);
            strcat(sendMsg, ";");
            strcat(sendMsg, "1;|");
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            if(sem_p(semid[2]))
            {
                printf("sem_p fail.\n");
            }
            shared[2]->flag[0] = false;
            shared[2]->flag[1] = false;
        }


        if(sem_v(semid[2]))
        {
            printf("sem_v fail.\n");
        }

        // shm 3
        //-----------------------------------------------------
        // change mode of system
        // get mode result from system
        if(sem_p(semid[3]))
        {
            printf("sem_p fail.\n");
        }
        if(shared[3]->flag[1])
        {
            char modeNow;
            modeNow = shared[3]->stringData[0];
            char msg[10] = {0};
            sprintf(msg, "%c;",modeNow);
            memset(sendMsg,0,sizeof(sendMsg));
            strcat(sendMsg, "3;");
            strcat(sendMsg, msg);
            strcat(sendMsg, "1;|");
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            shared[3]->flag[0] = false;
            shared[3]->flag[1] = false;
        }
        if(sem_v(semid[3]))
        {
            printf("sem_v fail.\n");
        }
        // shm 4
        //-----------------------------------------------------
        // sys State
        if(sem_p(semid[4]))
        {
            printf("sem_p fail.\n");
        }
        if(shared[4]->flag[0]) // a new state
        {
            numTrajs = shared[4]->dataInt[0];
            numPoints = shared[4]->dataInt[1];
            char tempStr[100];
            int queryIdNow = shared[4]->dataInt[2];
            nowState = shared[4]->dataInt[3];
            char modeNow = shared[4]->stringData[0];
            memset(sendMsg,0,sizeof(sendMsg));
            strcat(sendMsg,"4;");
            sprintf(tempStr, "%d:%d:%c:%d:%d:;",numTrajs,numPoints,modeNow, queryIdNow,nowState);
            strcat(sendMsg, tempStr);
            strcat(sendMsg, "1;|");
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            printf("state changed:");
            printf("%s\n",sendMsg);
            shared[4]->flag[0] = false;
            shared[4]->flag[1] = false;
        }
        if(sem_v(semid[4]))
        {
            printf("sem_v fail.\n");
        }

        // shm 5
        //-----------------------------------------------------
        // clean
        if(sem_p(semid[5]))
        {
            printf("sem_p fail.\n");
        }
        if(shared[5]->flag[0]) // clean finished
        {
            memset(sendMsg,0,sizeof(sendMsg));
            strcpy(sendMsg, "5;1:;1;|"); // clean finish
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            shared[5]->flag[0] = false;
            shared[5]->flag[1] = false;
        }
        if(sem_v(semid[5]))
        {
            printf("sem_v fail.\n");
        }
        // shm 6
        //-----------------------------------------------------
        // demo
        if(sem_p(semid[6]))
        {
            printf("sem_p fail.\n");
        }
        if(shared[6]->flag[0]) // demo finish
        {
            double startTime = shared[6]->dataDou[0];
            double endTime = shared[6]->dataDou[1];
            memset(sendMsg,0,sizeof(sendMsg));
            sprintf(sendMsg, "6;%lf:3:;1;|",(endTime - startTime)/1000000 - 2); //-2 is because sleep(2) in demo
            printf("%s\n",sendMsg);
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            shared[6]->flag[0] = false;
            shared[6]->flag[1] = false;
        }
        if(sem_v(semid[6]))
        {
            printf("sem_v fail.\n");
        }

        // -----------------------------------------------------------------------------
        // recv socket from GUI
        memset(buffer,0, sizeof(buffer));
        int lenRecv = recv(client_fd, buffer, sizeof(buffer), 0);
        if(lenRecv<0)
        {
            if(errno != EWOULDBLOCK)
            {
                printf("recv error: %d.\n",errno);
            }
        }



        if(lenRecv > 0)
        {
            // split packets use "|"
            char* msg = new char[1024];
            memcpy(msg,buffer, sizeof(buffer));
            printf("[RECV]%s\n",msg);
            char* temp;
            vector<char*> splitMsg;
            temp = strtok(msg,"|");
            while (temp != NULL) {
                splitMsg.push_back(temp);
                temp = strtok(NULL,"|");
            }

            for(int i=0;i<=splitMsg.size()-1;i++)
            {
                printf("[DEBUG]");
                printf("%s\n",splitMsg[i]);
                int msgType = socketMsgHandler(splitMsg[i],client_fd);
                // ....
                if(msgType == 13) // close app
                {
                    memset(sendMsg, 0, sizeof(sendMsg));
                    strcpy(sendMsg, "13;;1;|");
                    send(client_fd, sendMsg, strlen(sendMsg), 0);
                    // kill process of NVMGTS
                    if(sem_p(semid[7]))
                    {
                        printf("sem_p fail.\n");
                    }
                    shared[7]->flag[1] = true;
                    shared[7]->flag[0] = false;
                    if(sem_v(semid[7]))
                    {
                        printf("sem_v fail.\n");
                    }
                    // waiting for close
                    while(1)
                    {
                        if(sem_p(semid[7]))
                        {
                            printf("sem_p fail.\n");
                        }
                        if(shared[7]->flag[0])
                        {
                            closeMe = 1;
                            break;
                        }
                        else
                        {
                            sleep(3);
                        }
                        if(sem_v(semid[7]))
                        {
                            printf("sem_v fail.\n");
                        }
                    }
                }
            }
            delete msg;

        }
    }


    // close socket.....
    close(client_fd);
    close(ser_sockfd);

    // close shm,semaphore....
    for(int i=0; i<SMTYPE_NUM; i++)
    {
        if(shmdt(shm[i])==-1)
        {
            printf("fail shmdt.\n");
        }
        if(shmctl(shmid[i],IPC_RMID,0)==-1)
        {
            printf("fail shm_rmID.\n");
        }
        if(del_sem(semid[i]))
            printf("delete sem fail.\n");
    }

    return 0;

}


/*
handle all socket msg
return msg text in char* msg, and return the type for int
*/
int socketMsgHandler(char* msg, int client_fd)
{

    // for LOAD, format of msg is 0;fileName;finishFlag;
    char* temp = NULL;
    temp = strtok(msg,";");
    int msgType = atoi(temp);
    switch(msgType)
    {
    case 0: // load data
    {
        if(sem_p(semid[0]))
        {
            printf("sem_p fail.\n");
        }
        temp = strtok(NULL, ";"); // datafile No
        shared[0]->dataInt[2] = atoi(temp);
        shared[0]->flag[1] = true; //start Flag
        shared[0]->flag[0] = false;
        nowState = 0;
        if(sem_v(semid[0]))
        {
            printf("sem_v fail.\n");
        }
        break;
    }
    case 1: // batch query
    {
        if(sem_p(semid[1]))
        {
            printf("sem_p fail.\n");
        }
        char* batchFileName = strtok(NULL, ";");
        memset(shared[1]->stringData,0,1000);
        strcpy(shared[1]->stringData, batchFileName);
        shared[1]->flag[2] = true; //start Batch
        shared[1]->flag[0] = false;
        shared[1]->flag[1] = false;
        nowState = 1;
        if(sem_v(semid[1]))
        {
            printf("sem_v fail.\n");
        }
        break;
    }
    case 2: // single query
    {
        if(sem_p(semid[2]))
        {
            printf("sem_p fail.\n");
        }
        char* MBRinfo = strtok(NULL, ";");
        // char* flag = strtok(NULL,";");
        double xmin,xmax,ymin,ymax;
        char* temp;
        temp = strtok(MBRinfo, ",");
        xmin = atof(temp);
        temp = strtok(NULL, ",");
        xmax = atof(temp);
        temp = strtok(NULL, ",");
        ymin = atof(temp);
        temp = strtok(NULL, ",");
        ymax = atof(temp);
        shared[2]->dataDou[0] = xmin;
        shared[2]->dataDou[1] = xmax;
        shared[2]->dataDou[2] = ymin;
        shared[2]->dataDou[3] = ymax;
        shared[2]->flag[0] = false;
        shared[2]->flag[1] = true; // gotoSingle
        nowState = 2;
        if(sem_v(semid[2]))
        {
            printf("sem_v fail.\n");
        }
        break;
    }
    case 3: // change mode
    {
        if(sem_p(semid[3]))
        {
            printf("sem_p fail.\n");
        }
        char* modeStr = strtok(NULL, ";");
        shared[3]->stringData[0] = modeStr[0];
        shared[3]->flag[0] = true;
        shared[3]->flag[1] = false;
        nowState = 3;
        if(sem_v(semid[3]))
        {
            printf("sem_v fail.\n");
        }
        break;
    }
    case 4: //sysState
    {
        if(sem_p(semid[4]))
        {
            printf("sem_p fail.\n");
        }
        // get sysState and send out
        //not implement here, periodically in main
        if(sem_v(semid[4]))
        {
            printf("sem_v fail.\n");
        }
        break;
    }
    case 5: // clean
    {
        if(sem_p(semid[5]))
        {
            printf("sem_p fail.\n");
        }
        if((shared[5]->flag[1]==0)) // not in clean
        {
            shared[5]->flag[1] = true;
            shared[5]->flag[0] = false;
            nowState = 5;
        }
        if(sem_v(semid[5]))
        {
            printf("sem_v fail.\n");
        }
        break;
    }
    case 6: //demo
    {
        char* dataFileNoStr = strtok(NULL, ";");
        int dataFileNo = atoi(dataFileNoStr);  // demo use file No. to run
        if(sem_p(semid[6]))
        {
            printf("sem_p fail.\n");
        }
        if((shared[6]->flag[1]==0)) // not in demo
        {
            char sendMsg[1024]; // send buffer
            shared[6]->flag[1] = true;
            shared[6]->flag[0] = false;
            nowState = 6;
            shared[6]->dataInt[0] = dataFileNo;
            // check fileNo -> batch -> interrupt -> recovery -> batch -> output recovery time
            if(sem_v(semid[6]))
            {
                printf("sem_v fail.\n");
            }
            // start batch
            if(sem_p(semid[1]))
            {
                printf("sem_p fail.\n");
            }
            shared[1]->flag[2] = true; //start Batch
            shared[1]->flag[0] = false;
            shared[1]->flag[1] = false;
            if(sem_v(semid[1]))
            {
                printf("sem_v fail.\n");
            }
            // interrupt
            printf("before\n");
            sleep(1);
            printf("after.\n");
            // kill process
            //how to kill?
            p1 = popen("pkill -9 test", "r");
            //pclose(p1);
            // send break info
            memset(sendMsg, 0, sizeof(sendMsg));
            strcpy(sendMsg, "6;-1:1;1;|");
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            if(sem_p(semid[6]))
            {
                printf("sem_p fail.\n");
            }
            shared[6]->dataDou[0] = getTimeus();
            if(sem_v(semid[6]))
            {
                printf("sem_v fail.\n");
            }
            //sleep(2);
            // recovery process ./test l
            // how to recovery?
            //system("chmod 777 /home/zbw/NVMgts_Interative/NVMgts");
            // system("chmod 777 /home/zbw/NVMgts_Interative/NVMgts/testGTS");
            printf("start run..");
            sleep(2);
            // send break info
            memset(sendMsg, 0, sizeof(sendMsg));
            strcpy(sendMsg, "6;-1:2;1;|");
            send(client_fd, sendMsg, strlen(sendMsg), 0);
            p1 = popen("/home/zbw/NVMgts_Interative/NVMgts/testGTS > run.log","r");
            //p1 = popen("/home/zbw/NVMgts_Interative/NVMgts/testGTS","r");
            // pid_t c = system("/home/zbw/NVMgts_Interative/NVMgts/testGTS");
            // c = system("/home/zbw/NVMgts_Interative/NVMgts/testGTS >> log.txt &");
            //printf("runned. return %d",WEXITSTATUS(c));
            // waiting for finish, go to while
        }
        else
        {
            if(sem_v(semid[6]))
            {
                printf("sem_v fail.\n");
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return msgType;
}

int creat_sem(key_t key)
{
    int semid = 0;

    semid = semget(key+1, 1, IPC_CREAT|0666);
    if(semid == -1)
    {
        printf("%s : semid = -1!\n",__func__);
        return -1;
    }

    return semid;

}

int set_semvalue(int semid)
{
    union semun sem_arg;
    sem_arg.val = 1;

    if(semctl(semid, 0, SETVAL, sem_arg) == -1)
    {
        printf("%s : can't set value for sem!\n",__func__);
        return -1;
    }
    return 0;
}

int sem_p(int semid)
{
    struct sembuf sem_arg;
    sem_arg.sem_num = 0;
    sem_arg.sem_op = -1;
    sem_arg.sem_flg = SEM_UNDO;

    if(semop(semid, & sem_arg, 1) == -1)
    {
        printf("%s : can't do the sem_p!\n",__func__);
        return -1;
    }
    return 0;
}

int sem_v(int semid)
{
    struct sembuf sem_arg;
    sem_arg.sem_num = 0;
    sem_arg.sem_op = 1;
    sem_arg.sem_flg = SEM_UNDO;

    if(semop(semid, & sem_arg, 1) == -1)
    {
        printf("%s : can't do the sem_v!\n",__func__);
        return -1;
    }
    return 0;
}

int del_sem(int semid)
{
    if(semctl(semid, 0, IPC_RMID) == -1)
    {
        printf("%s : can't rm the sem!\n",__func__);
        return -1;
    }
    return 0;
}

double getTimeus()
{
    struct timeval time;
    gettimeofday(&time,NULL);
    double ms =1000000*(time.tv_sec) + time.tv_usec;
    return ms;
}
