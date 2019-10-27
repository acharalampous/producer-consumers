#include <stdio.h>
#include <unistd.h>
#include <time.h> // for srand
#include <sys/time.h> // for timedifference
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h> // for memcpy

#include "sems.h"

// struct for shared memory
typedef struct ShMem{
    int value;
    struct timeval timeSignature;
}ShMem;

// Compute time difference in millisecods //
float timeElapsed(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
}

int main(int argc, char* argv[]){
    int sharedMemID; // shared memory between processes
    ShMem* sharedData; // content of shared memory
    key_t key; // key for shared memory identifier
    int n,m;
    int status; // will be used for father to wait for childs to exit

    srand(time(NULL)); // seed for random numbers

    if(argc != 3){
        printf("**ERROR** Invalid input. You must enter number of processes and cells! Abort.\n");
        return -1;
    }

    n = atoi(argv[1]);
    m = atoi(argv[2]);
    if(n < 1 || m < 1){
        printf("**ERROR** Invalid input. N and M must be greater than zero! Abort.\n");
        return -1;
    }
    else{
        printf("**Program executed succesfully.**\n");
        printf(" Given Parameters:\n");
        printf("  Number Of Processes to Create: %d\n",n);
        printf("  Number Of Values in array to Create: %d\n",m);
    }

    // Create shared memory //
    sharedMemID = shmget((key_t)5, sizeof(ShMem), IPC_CREAT | 0644);
    if (sharedMemID < 0) {
        printf("Error while creating shared Memory! Abort.\n");
        exit(1);
    }

    // Attach Shared memory //
    sharedData = shmat(sharedMemID, (void *)0, 0);
    if(sharedData == (ShMem*) -1){
        printf("Error while attaching shared Memory! Abort.\n");
        exit(1);
    }


    //  create a single semaphore which will "notify" the writer //
    //  when he can write new data in shared memory              //
    int semWriter = semsCreate((key_t)1, 1, 1);

    //  set of n semaphores, for each process, to make them avoid//
    //  to read the same data more than once                     //
    int semReaders = semsCreate((key_t)2, n, 0);

    //  semaphore that will keep number of children that read   //
    //  the specific data at the specific moment                //
    //  When this becomes equal to number of sems, inform writer//
    int Read = semsCreate((key_t)3, 1, 0);

    //  mutex for printing in file //
    int semMutex = semsCreate((key_t)4, 1, 0);


    //  Create n processes. Each child will exit at a specific i,   //
    //  so each one will have a unique i. That i will be the        //
    //  process' semaphore's index in semReaders                    //
    int i;
    pid_t pid; // will be used to identify childs and parent
    for(i = 0; i < n; i++){
        pid = fork();
        if(pid == 0) // this is a child, so avoid fork
            break;
    }


    if(pid != 0){ // Parent's part
        // Create the array to be copied //
        int* FArray = malloc(sizeof(int) * m);
        if(FArray == NULL){
            printf("Error while allocating master Matrix");
            return -1;
        }

        // Fill the table with random numbers and write to file //
        FILE* fp = fopen("results.txt", "w");
        fprintf(fp,"Master Array: ");
        for(i = 0; i < m; i++){
            FArray[i] = rand() % 30;
            fprintf(fp,"%d ",FArray[i]);
        }
        fprintf(fp, "\n\n");
        fflush(fp);
        fclose(fp);

        // Copy array cell by cell to shared mem
        struct timeval writeTime; // for timeSignal
        for(i = 0; i < m; i++){
            semsDown(semWriter, 0); //Block Writing, until all processes copy data from mem

            sharedData->value = FArray[i]; // write value

            gettimeofday(&writeTime, 0); // set timeSignal
            memcpy(&sharedData->timeSignature, &writeTime,sizeof(writeTime));

            semsSetValue(Read, 0, 0);
            semsSetAll(semReaders, n, 1); // up all processes to read next data
        }
        semsDown(semWriter, 0); // wait until all children read last value
        semsUp(semMutex, 0); // let children write to file

        // Free Master Array //
        free(FArray);
    } // end parent's part
    else{ // child's part

        // Create the array that child will use to store values //
        int* CArray = malloc(sizeof(int) * m);
        if(CArray == NULL){
            printf("Error while allocating child's Matrix\n");
            return -1;
        }

        // Copy step by step from shared memory
        int j;
        struct timeval writeTime; // for timeSignal
        struct timeval readTime;
        float totalTime = 0.0; // average time for reading all values from memory
        for(j = 0; j < m; j++){
            semsDown(semReaders, i); //Block i Process from reading the same data twice

            CArray[j] = sharedData->value;

            gettimeofday(&readTime, 0);
            memcpy(&writeTime, &sharedData->timeSignature, sizeof(writeTime));
            totalTime += timeElapsed(writeTime, readTime);

            semsUp(Read, 0);
            if(semsGetValue(Read, 0) == n)
                semsUp(semWriter, 0);
        }
        float averageTime = totalTime / m; // compute average time

        semsDown(semMutex, 0); // Mutual exclusion, for printing to file

        FILE *fp = fopen("results.txt","a"); // each kid will append it's info

        // Print process' info //
        fprintf(fp,"******************\n");
        fprintf(fp,"Process #%d [%d]\n",i + 1,getpid());
        fprintf(fp,"******************\n");
        fprintf(fp,"Finished in: %f milliseconds\n",averageTime);

        // Print array of process //
        int k;
        fprintf(fp,"My array:");
        for(k = 0; k < m; k++){
            fprintf(fp," %d",CArray[k]);
        }
        fprintf(fp,"\n\n");

        fflush(fp);
        fclose(fp);

        semsUp(semMutex, 0);

        free(CArray); // free child's array
        exit(0);
    } // end child part


    // wait for children to exit //
    wait(&status);

    // Dettach shared data //
    if(shmdt(sharedData) == -1){
        printf("Error while dettaching shared Data\n");
    }

    // Delete Shared memory //
    if(shmctl(sharedMemID, IPC_RMID, 0) == -1){
        printf("Error while deleting shared Memory\n");
        printf("id: %d\n",i);
    }

    // Delete All Semaphores //
    semsDelete(semWriter);
    semsDelete(semReaders);
    semsDelete(Read);
    semsDelete(semMutex);

    printf("**End of procedure** Check results.txt for results.\n");

    return 1;
}
