#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include "mpi.h"
#define RIGHT    0
#define LEFT     1

/*
 * Struktura vilice
 *  - request : imam zahtjev za vilicu
 *  - neighbor : id susjeda filozofa s kojim dijelim vilicu
 *  - haveIt : je li vilica kod mene
 *  - clean : je li vilica cista
 */
typedef struct {
    char request;
    int neighbor;
    bool haveIt;
    bool clean;
} fork_;

void printTabs(int rank);
void think();
void eat();
void parseMessage(char forkId);
char othersFork(char forkId);

int numProcs, myRank;
fork_ myForks[2];

int main(int argc, char** argv){
    MPI_Init(&argc, &argv);
    // Inicijalizacija filozofa
    // indeks procesa
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    // ukupan broj procesa
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    srand(time(nullptr) + myRank);
    MPI_Status mpiStatus;

    // inicijalizacija desnog i lijevog susjeda
    myForks[RIGHT].neighbor = (myRank + numProcs - 1) % numProcs;

    myForks[LEFT].neighbor = (myRank + 1) % numProcs;

    /*
     * vilicu na pocetku ima filozof s manjim indeksom
     * vilica je na pocetku prljava
    */
    for(auto &fork : myForks){
        fork.haveIt = fork.neighbor > myRank;
        fork.clean = false;
        fork.request = 0;
    }


    usleep(500000);
    MPI_Barrier(MPI_COMM_WORLD);

    while(true){
        think();        // Filozof misli i istovremeno odgavara na zahtjeve drugih filozofa
        // Filozof treba dvije vilice
        while(!myForks[RIGHT].haveIt || !myForks[LEFT].haveIt)
        {
            int neededFork;
//            printf("Nemam vilicu[0] : %d", myForks[0].haveIt == false);
            for(neededFork = 0; myForks[neededFork].haveIt; neededFork++);

            printTabs(myRank);
            printf("Trazim vilicu (%d)\n", myForks[neededFork].neighbor);

            char forkToRequest = othersFork(neededFork);

            MPI_Send(&forkToRequest, 1, MPI_CHAR, myForks[neededFork].neighbor, 0, MPI_COMM_WORLD);     // Posalji zahtjev za vilicom

            do {        // Cekaj da dobijes trazenu vilicu
                char forkId;
                MPI_Recv(&forkId, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);
                if(mpiStatus.MPI_TAG == 0){
                    parseMessage(forkId);
                } else if (mpiStatus.MPI_TAG == 1){
                    myForks[forkId].haveIt = true;
                    myForks[forkId].clean = true;
                }

            } while(!myForks[neededFork].haveIt);
        }
        eat();      // Jedi

        myForks[0].clean = false;   // Vilice postaju prljave nakon jela

        myForks[1].clean = false;
        for(int i = 0; i < 2; i++){
            if(myForks[i].request){
                char forkToSend = othersFork(i);                // Posalji vilicu ako postoji zahtjev za njom
                MPI_Send(&forkToSend, 1, MPI_CHAR, myForks[i].neighbor, 1, MPI_COMM_WORLD);
                myForks[i].haveIt = false;
                myForks[i].request = 0;
            }
        }
    }

    MPI_Finalize();
    return 0;
}

void printTabs(int rank){
    while(rank--){
        printf("\t\t");
    }
}

void think() {
    int thinkingTime, flag;
    printTabs(myRank);
    printf("mislim\n");
    for(int i = 0; i < 15; i++){
        thinkingTime = rand() % 400000 + 10000;
        usleep(thinkingTime);
        // Neblokirajuca provjera postoji li poruka - zahtjev za vilicom
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if(flag){
            char forkId;
            MPI_Recv(&forkId, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            parseMessage(forkId);
        }
    }
}

void eat() {
    int eatingTime;
    eatingTime = rand() % 5000000 + 100000;
    printTabs(myRank);
    printf("jedem\n");
    usleep(eatingTime);     // spavaj
}

void parseMessage(char forkId) {
    if(myForks[forkId].clean){
        myForks[forkId].request = 1;        // Oznaci zahtjev za vilicom
    } else {                                // Posalji vilicu
        char forkToSend = othersFork(forkId);
        MPI_Send(&forkToSend, 1, MPI_CHAR, myForks[forkId].neighbor, 1, MPI_COMM_WORLD);
        myForks[forkId].request = 0;
        myForks[forkId].haveIt = false;
    }
}

char othersFork(char forkId) {
    if(forkId == LEFT){
        return RIGHT;
    } else {
        return LEFT;
    }
//    return (forkId + 1) % 2;
}
