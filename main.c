//
//  main.c
//  forestFire
//
//  Created by Yuhui Zheng on 6/25/14.
//  Copyright (c) 2014 Yuhui Zheng. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>

#include "dataStruct.h"

int main(int argc, char **argv)
{

    /* user inputs */
    float pb, pa;
    unsigned int proc, freq, iter;
    
    /* shared memory */
    int segment_id;
    shared *ptr;
    
    /* processes */
    pid_t child_id;
    char *arg_list[2] = {"\0", NULL};               /* null terminates. osx could work without it. linux can't */
    arg_list[0] = (char *)malloc(sizeof(char)*32);
    
    /* semaphore */
    semun arg;
    arg.val = 1;
    
    /* for v. before C99 */
    int i,j;
    
    /* all the processes shall terminate, not blocking */
    struct sembuf sem_care[1];
    sem_care[0].sem_num = 0;
    sem_care[0].sem_op = 6;
    sem_care[0].sem_flg = 0;
    
    /* parsing user inputs */
    if (argc != 6) {
        printf("[error]: Not enough arguments. <processes> <iterations> <pb> <pa> <output freq>\n");
        exit(EXIT_FAILURE);
    }
    
    proc = atoi(argv[1]);
    pb = atof(argv[3]);
    pa = atof(argv[4]);
    iter = atoi(argv[2]);
    freq = atoi(argv[5]);
    
    if ( proc<1 || proc>16) {
        printf("[error]: please keep <processes> between 1 and 16.\n");
        exit(EXIT_FAILURE);
    }
    
    if ( pb<0 || pb>1.0 || pa<0 || pa>1.0) {
        printf("[error]: please keep <pb> and <pa> between 0 and 1.\n");
        exit(EXIT_FAILURE);
    }

    if ( iter<1 ) {
        printf("[warning]: <iter> is smaller than 1. Rounded up to 1\n");
        iter = 1;
    }
    
    if (freq < 1) {
        printf("[warning]: <freq> is smaller than 1. Rounded up to 1\n");
        freq = 1;
    }
    
    if ( freq > iter ) {
        printf("[warning]: <output freq> is larger than <iterations>.\n Thus only output once after execution\n");
        freq = iter;
    }
    
    //printf("[main] arguments: %d, %d, %f, %f, %d\n", proc, iter, pb, pa, freq);
    
    /* clear previous simulation results */
    system("exec rm -r ./r/*");
    
    /* shared memory: allocate, attach, init */
    segment_id = shmget (IPC_PRIVATE, sizeof(shared), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    
    ptr = (shared *)shmat(segment_id, 0, 0);    /* attach shared mem to this process */

    if (ptr == (shared *) -1) {
        printf("[error]: shared memory\n");
        exit(EXIT_FAILURE);
    }
    
    /* Semaphore */
    ptr->semid = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0777);
    
    if (ptr->semid == -1) {
        printf("[error]: semaphore creation\n");
        exit(EXIT_FAILURE);
    }

    if (semctl(ptr->semid, 0, SETVAL, arg) == -1) {
        printf("[error]: semaphore initialization\n");
        exit(EXIT_FAILURE);
    }
    
    /* initialize shared memory */
    ptr->pb = pb;
    ptr->pa = pa;
    ptr->proc = proc;
    ptr->iter = iter;
    ptr->freq = freq;
    ptr->count = 0;
    
    for (i=0; i<ROW; i++) {
        for (j=0; j<COL; j++) {
            ptr->trees[i][j] = ALIVE;
        }
    }

    printf("[main]: start to create child processes\n ...\n");
    /* create processes (new program nextStage)- if error, exit */
    sprintf(arg_list[0], "%d", segment_id);     /* prepare argument list for child */

    for (i=0; i<proc; i++) {
        
        child_id = fork();
        
        switch (child_id) {
            case -1:    /* exit on error */
                printf("[error]: failed to fork()\n");
                exit(EXIT_FAILURE);
                
            case 0:     /* parent proc */
                if (execve("./nextStage", arg_list, NULL) == -1) {
                    printf("[error]: failed to execve()\n");
                    exit(EXIT_FAILURE);
                }
                
            default:    /* child proc */
                break;
        }
    
    }
    
    /* wait until all child processes finish                        */
    /* if any child proc terminates, ptr->count reached limitation  */
    /* At this time, removing semaphore is safe                     */
    while (proc != 0) {
        child_id = wait(NULL);
        
        if (child_id == -1) {
            //printf("[main] %d finished. now %d left\n", child_id, proc);
            break;
        }
        
        proc--;
        

        if ( semop(ptr->semid, sem_care, 1) == -1) {
            printf("[Child]: pid %d - semaphore error\n", getpid());
            exit(EXIT_FAILURE);
        }

        //printf("[main] %d finished. now %d left\n", child_id, proc);
    }
    
    /* check if need to print the last iteration */
    if (ptr->iter % ptr->freq != 0) {
        FILE *fd;
        
        strcpy(filename, "./r/iter_final.txt");

        //printf("[Child]: %s", filename);
        
        fd = fopen(filename, "w+");
        
        for (i=0; i<ROW; i++) {
            for (j=0; j<COL; j++) {
                switch (ptr->trees[i][j]) {
                    case ALIVE:
                        fputc('+', fd);
                        break;
                    case DEAD:
                        fputc('-', fd);
                        break;
                    case BURNING:
                        fputc('*', fd);
                        break;
                    default:
                        break;
                }
            }
            fputc('\n', fd);
        }
    }
    
    /* dealloc semaphore */
    if (semctl(ptr->semid, 1, IPC_RMID) == -1) {
        printf("[error]: semaphore deletion\n");
        exit(EXIT_FAILURE);
    }
    
    /* shared memory: detach, dealloc */
    shmdt(ptr);
    shmctl (segment_id, IPC_RMID, 0);
    
    printf("[main]: All processes have finished\n simulation results are under ./r");
    printf("\n\n");
    

    exit(EXIT_SUCCESS);
}




