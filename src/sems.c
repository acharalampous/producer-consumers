#include <stdio.h>
#include "sems.h"

int semsCreate(key_t key, int numberOfSems, int initValue){
    int semid; // id of semaphores set, will be returned

    if(numberOfSems < 0 || initValue < 0 || key < 0){
        //printf("[semsCreate] Invalid values given\n");
        return -1;
    }

    semid = semget(key, numberOfSems, 0666 | IPC_CREAT);
    if(semid < 0){
        //printf("[semsCreate] Error while creating semaphores set\n");
        return -1;
    }

    //  Initialize all semaphores with given value //
    int i;
    for(i = 0; i < numberOfSems; i++){
        int result;
        result = semsSetValue(semid, i, initValue);
        if(result == -1)
            return result;
    }

    return semid;
}

int semsSetValue(int semid, int index, int newValue){
    semun arg;
    int result;

    if(semid < 0 || index < 0){
        //printf("[semsSetValue] Invalid values given\n");
        return -1;
    }

    arg.val = newValue;

    result = semctl(semid,index,SETVAL,arg);
    if(result == -1){
        //printf("[semsSetValue] Error while setting value to semaphore\n");
        return -1;
    }
    else
        return 1;
}

int semsGetValue(int semid, int index){
    int value;
    semun arg; // just for semctl,no use

    if(semid < 0 || index < 0){
        //printf("[semsGetValue] Invalid values given\n");
        return -1;
    }

    value = semctl(semid, index, GETVAL, arg);
    if(value == -1){
        //printf("[semsGetValue] Error while getting value of semaphore\n");
        return -1;
    }

    return value;
}

int semsDelete(int semid){
    int result;
    if(semid < 0){
        //printf("[semsDelete] Invalid values given\n");
        return -1;
    }

    // Delete set of semaphores //
    result = semctl(semid, 0, IPC_RMID);

    return result;
}

int semsGetAll(int semid, int numberOfSems, ushort* values){
    int result;
    semun arg; // arg.array will hold the values of semaphores

    if(semid < 0 && numberOfSems < 0){
        //printf("[semsGetAll] Invalid values given\n");
        return -1;
    }

    arg.array = malloc(sizeof(ushort) * numberOfSems);

    result = semctl(semid, numberOfSems, GETALL, arg);
    if(result == -1){
        //printf("[semsGetAll] Error while getting values of all semaphores\n");
        free(arg.array);
        return -1;
    }

    // Copy from arg.array to given array to return //
    int i;
    for(i = 0; i < numberOfSems; i++)
        values[i] = arg.array[i];

    free(arg.array);

    return 1;
}

int semsSetAll(int semid, int numberOfSems, int newValue){
    int result;
    semun arg; // arg.array will be used to set values to semaphores

    if(semid < 0 && numberOfSems < 0 && newValue < 0){
        //printf("[semsSetAll] Invalid values given\n");
        return -1;
    }

    arg.array = malloc(sizeof(ushort) * numberOfSems);

    // Copy to arg.array to write to semaphores //
    int i;
    for(i = 0; i < numberOfSems; i++)
        arg.array[i] = newValue;

    result = semctl(semid, numberOfSems, SETALL, arg);
    if(result == -1){
        //printf("[semsSetAll] Error while setting values to all semaphores\n");
        free(arg.array);
        return -1;
    }

    free(arg.array);

    return 1;
}

int semsUp(int semid, int index){
    int result;
    struct sembuf sb; // will be used for semop

    if(semid < 0 || index < 0){
        //printf("[semsUp] Invalid values given\n");
        return -1;
    }

    sb.sem_num = index;
    sb.sem_op = 1;
    sb.sem_flg = 0;

    result = semop(semid,&sb,1);
    if(result == -1){
        //printf("[semsUp] Error while up operation\n");
        return -1;
    }
    else
        return 1;
}

int semsDown(int semid, int index){
    int result;
    struct sembuf sb; // will be used for semop

    if(semid < 0 || index < 0){
        //printf("[semsDown] Invalid values given\n");
        return -1;
    }

    sb.sem_num = index;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    result = semop(semid,&sb,1);
    if(result == -1){
        //printf("[semsDown] Error while down operation\n");
        return -1;
    }
    else
        return 1;
}
