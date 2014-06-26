//
//  nextStage.c
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

static shared* ptr;

/* local func: check if neighbors are burning   */
status neighbors(int row, int col);

/* local func: output results to './r/file.txt' */
void display(status next_tree[ROW][COL]);


/* MAIN -- each nextStage is a process          */
int main(int argc, char **argv) {

    int segment_id = atoi(argv[0]);
    
    /* attach shared memory */
    ptr = (shared *)shmat(segment_id, 0, 0);
    
    /* random seed */
    int seed = (int)time(NULL);
    srand(seed);
    
    /* semaphore operation define */
    struct sembuf sem_wait[1];
    sem_wait[0].sem_num = 0;
    sem_wait[0].sem_op = -1;
    sem_wait[0].sem_flg = 0;
    
    struct sembuf sem_rel[1];
    sem_rel[0].sem_num = 0;
    sem_rel[0].sem_op = 1;
    sem_rel[0].sem_flg = 0;
    
    /* for v. before C99 */
    int i,j;
    
    
    status next_trees[ROW][COL];
    //printf("[Child]: segment_id = %d, check pb: %f, pa: %f, iter: %d\n", segment_id, ptr->pb, ptr->pa, ptr->iter);
    
    while (1) {
        
        /* semaphore -- read/write on shared_memory segment */
        
        if ( semop(ptr->semid, sem_wait, 1) == -1) {
            printf("[Child]: pid %d - semaphore error\n", getpid());
            exit(EXIT_FAILURE);
        }
        
        if ( (ptr->count) >= (ptr->iter) ) {
            break;
        }
        
        //printf("[Child]: count: %d, pid: %d\n\n", ptr->count, getpid());
        
        for (i=0; i<ROW; i++) {
            for (j=0; j<COL; j++) {
                switch (ptr->trees[i][j]) {
                        
                    case ALIVE:
                        if (neighbors(i, j) == BURNING ) {
                            next_trees[i][j] = BURNING;
                        } else {
                            seed = rand();
                            srand(seed);
                            if ( seed%ACCU < ptr->pb*ACCU) {    /* accuracy 0.0001 */
                                next_trees[i][j] = BURNING;
                            } else {
                                next_trees[i][j] = ALIVE;
                            }
                        }
                        break;
                        
                    case BURNING:
                        next_trees[i][j] = DEAD;
                        break;
                        
                    case DEAD:
                        seed = rand();
                        srand(seed);
                        if ( seed%ACCU < ptr->pa*ACCU) {        /* accuracy 0.0001 */
                            next_trees[i][j] = ALIVE;
                        } else {
                            next_trees[i][j] = DEAD;
                        }
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        /* update contents in ptr->trees */
        for (i = 0; i<ROW; i++) {
            for(j=0; j<COL; j++) {
                ptr->trees[i][j] = next_trees[i][j];
            }
        }
        
        ptr->count++;
        
        
        /* semaphore -- finished read/write on shared_memory segment */
        if ( semop(ptr->semid, sem_rel, 1) == -1) {
            printf("[Child]: pid %d - semaphore error\n", getpid());
            exit(EXIT_FAILURE);
        }
        
        
        /* write next_trees[][] to file */
        if ( ptr->count % ptr->freq == 0) {
            display(next_trees);
        }
        

    }
    
   
    //printf("[Child]: preparing to detach\n");
    
    /* detach shared memory */
    shmdt(ptr);
    
    /* write next_tree[][] to file */
    
    _exit(EXIT_SUCCESS);
}



/* local func: check if neighbors are burning   */
status neighbors(int row, int col) {
    
    /* check 4 neighbors: left, right, top, bottom */
    /* DEAD neighbors won't affect. consider as ALIVE */

    if (col != 0 && ptr->trees[row][col-1] == BURNING) {
        return BURNING;                                     /* left is burning */
    }
    
    if (col != COL-1 && ptr->trees[row][col+1] == BURNING) {
        return BURNING;                                     /* right is burning */
    }
    
    if (row != 0 && ptr->trees[row-1][col] == BURNING) {
        return BURNING;                                     /* top is burning */
    }
    
    if (row != ROW-1 && ptr->trees[row+1][col] == BURNING) {
        return BURNING;                                     /* bottom is burning */
    }
    
    return ALIVE;
}


/* local func: output results to './r/file.txt' */
void display(status next_tree[ROW][COL]) {
    
    /* file io */
    FILE *fd;

    sprintf(append, "%d", ptr->count);
    //TODO: check range
    strcpy(filename, "");
    strcat(filename, pref);
    strcat(filename, append);
    strcat(filename, type);
    //printf("[Child]: %s", filename);
    
    int i,j; /* for v. before C99 */
    
    fd = fopen(filename, "w+");
    
    for (i=0; i<ROW; i++) {
        for (j=0; j<COL; j++) {
            switch (next_tree[i][j]) {
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
    
    fclose(fd);

}
