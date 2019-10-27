#pragma once

#include <stdlib.h>
#include <sys/sem.h>
#include <sys/ipc.h>

//  will be used for the arg parameter of semaphores //
//  (same from lectures)                             //
typedef union semun{
    int val;
    struct semid_ds *buf;
    ushort* array;
}semun;

//  Takes the number of semaphores and an initial value  //
//  and initiliazes a set with sempahores with that value//
int semsCreate(key_t , int, int);

//  Delete given set
int semsDelete(int);

//  Takes a semaphore (set and an index) and sets it's value //
//  equal to the given value                                 //
int semsSetValue(int, int, int);

//  Returns the value of the requested semaphore of set//
int semsGetValue(int, int);

//  Writes in given array, the values of all semaphores //
int semsGetAll(int, int, ushort*);

// Sets all values of semaphores //
int semsSetAll(int, int, int);

// Up specific semaphore //
int semsUp(int, int);

// Down specific semaphore //
int semsDown(int, int);
