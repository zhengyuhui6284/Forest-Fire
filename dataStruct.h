//
//  dataStruct.h
//  forestFire
//
//  Created by Yuhui Zheng on 6/25/14.
//  Copyright (c) 2014 Yuhui Zheng. All rights reserved.
//

#ifndef SEMUN_H
#define SEMUN_H

union sem_arg {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
#if defined(__linux__)
    struct seminfo * _buf;
#endif
};

typedef union sem_arg semun;
#endif



#ifndef forestFire_dataStruct_h
#define forestFire_dataStruct_h

/* status */
enum status {
    ALIVE,
    BURNING,
    DEAD
};

typedef enum status status;

/* simulation size */
#define ROW 32
#define COL 62

/* accuracy */
#define ACCU 10000

/* shared memory */
typedef struct {
    int semid;                        /* semaphore for shared mem   */
    
    status trees[ROW][COL];
    unsigned int count;               /* updated each loop          */
    
    float pa,pb;                      /* set by main                */
    unsigned int proc, freq, iter;    /* set by main                */
    
} shared;


/* file control */
char filename[32];
char append[20];
const char *pref = "./r/itr";
const char *type = ".txt";

#endif

