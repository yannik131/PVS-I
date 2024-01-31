#include <mpi.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

int getRank() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    return rank;
}

int getWorldSize() {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    
    return worldSize;
}

void printArray(int* arr, int count) {
    for(int i = 0; i < count; ++i) {
        printf("%i ", arr[i]);
    }
}

void flushAndSleep() {
    fflush(stdout);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    
    nanosleep(&ts, NULL);
}

void printArrayForAll(int* arr, int count) {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    int rank = getRank();
    
    if(rank == 0) {
        printf("Buffer content:\n");
        flushAndSleep();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    for(int i = 0; i < worldSize; ++i) {
        if(rank == i) {
            printf("Rank %i: ", i);
            printArray(arr, count);
            printf("\n");
            flushAndSleep();
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    if(rank == 0) {
        printf("\n");
        flushAndSleep();
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

void sendReceive() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank == 0) {
        printf("Sending with MPI_Send, receiving with MPI_Recv\n");
        MPI_Send(data, 3, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    if(rank == 1) {
        MPI_Recv(buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void sendReceive2() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank == 0) {
        printf("Sending and receiving with Sendrecv\n");
        MPI_Sendrecv(data, 3, MPI_INT, 1, 0, buf, 3, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    if(rank == 1) {
        MPI_Sendrecv(data, 3, MPI_INT, 0, 0, buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void iSendReceive() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank == 0) {
        printf("Sending with MPI_Isend, receiving with MPI_Irecv\n");
        MPI_Request request;
        MPI_Isend(data, 3, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
    if(rank == 1) {
        MPI_Request request;
        MPI_Irecv(buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
        //Instead of MPI_Wait, MPI_Test is also possible:
        int flag;
        int i = 0;
        do {
            MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
             ++i;
        } while(!flag);
        printf("Tested %i times\n", i);
    }
    
    printArrayForAll(buf, 3);
}

void iSendReceive2() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank == 0) {
        printf("Sending and receiving with MPI_Isendrecv\n");
        MPI_Request request;
        MPI_Isendrecv(data, 3, MPI_INT, 1, 0, buf, 3, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
    if(rank == 1) {
        MPI_Request request;
        MPI_Isendrecv(data, 3, MPI_INT, 0, 0, buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void bSendReceive() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank == 0) {
        printf("Sending with MPI_Bsend, receiving with MPI_Recv\n");
        int bSendBufSize = 3 * sizeof(int) + MPI_BSEND_OVERHEAD;
        char* bSendBuf = (char*)malloc(3 * sizeof(int));
        MPI_Buffer_attach(bSendBuf, bSendBufSize);
        
        MPI_Bsend(data, 3, MPI_INT, 1, 0, MPI_COMM_WORLD);
        
        MPI_Buffer_detach(&bSendBuf, &bSendBufSize);
        free(bSendBuf);
    }
    if(rank == 1) {
        MPI_Recv(buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void iBSendReceive() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
        
    if(rank == 0) {
        printf("Sending with MPI_Ibsend, receiving with MPI_Irecv\n");
        int bSendBufSize = 3 * sizeof(int) + MPI_BSEND_OVERHEAD;
        char* bSendBuf = (char*)malloc(3 * sizeof(int));
        MPI_Buffer_attach(bSendBuf, bSendBufSize);
        
        MPI_Request request;
        MPI_Ibsend(data, 3, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        
        MPI_Buffer_detach(&bSendBuf, &bSendBufSize);
        free(bSendBuf);
    }
    if(rank == 1) {
        MPI_Request request;
        MPI_Irecv(buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void probe() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    if(rank == 0) {
        printf("Using MPI_Probe and MPI_Get_count to determine the number of elements to receive\n");
        MPI_Send(data, 3, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    if(rank == 1) {
        MPI_Status status;
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
        
        int count;
        MPI_Get_count(&status, MPI_INT, &count);
        
        printf("1: Receiving %i elements\n", count);
        MPI_Recv(buf, count, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void iProbe() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    if(rank == 0) {
        printf("Using MPI_Iprobe and MPI_Get_count to determine the number of elements to receive\n");
        MPI_Send(data, 3, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    if(rank == 1) {
        MPI_Status status;
        int flag;
        
        do {
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
        } while(!flag);
        
        int count;
        MPI_Get_count(&status, MPI_INT, &count);
        
        printf("1: Receiving %i elements\n", count);
        MPI_Recv(buf, count, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    printArrayForAll(buf, 3);
}

void broadcast() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    int* ptr;
    if(rank == 0) {
        printf("Using MPI_Bcast to broadcast an array\n");
        ptr = data;
    }
    else {
        ptr = buf;
    }
    
    MPI_Bcast(ptr, 3, MPI_INT, 0, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 3);
}

void reduce() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank != 0) {
        data[1] = 10;
    }
    
    if(rank == 0) printf("Using MPI_Reduce with MPI_SUM\n");
    MPI_Reduce(data, buf, 3, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    printArrayForAll(buf, 3);
    
    if(rank == 0) printf("With MPI_MIN\n");
    MPI_Reduce(data, buf, 3, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    printArrayForAll(buf, 3);
    
    if(rank == 0) printf("With MPI_MAX\n");
    MPI_Reduce(data, buf, 3, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    printArrayForAll(buf, 3);
    
    if(rank == 0) printf("With MPI_PROD\n");
    MPI_Reduce(data, buf, 3, MPI_INT, MPI_PROD, 0, MPI_COMM_WORLD);
    printArrayForAll(buf, 3);
    
    typedef struct RankItem {
        int rank;
        int value;
    } RankItem;
    
    RankItem itemToSend = {.rank = getRank(), .value = (getRank() + 1) * 10};
    
    RankItem min = {.rank = -1, .value = -1};
    RankItem max = {.rank = -1, .value = -1};
    
    if(rank == 0) printf("With MPI_MINLOC. Printing rankOfMinValue minValue\n");
    MPI_Reduce(&itemToSend, &min, 1, MPI_2INT, MPI_MINLOC, 0, MPI_COMM_WORLD);
    int minData[2] = {min.rank, min.value};
    printArrayForAll(minData, 2);
    
    if(rank == 0) printf("With MPI_MAXLOC. Printing rankOfMaxValue maxValue \n");
    MPI_Reduce(&itemToSend, &max, 1, MPI_2INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);
    int maxData[2] = {max.rank, max.value};
    printArrayForAll(maxData, 2);
}

void allReduce() {
    int data[3] = {1, 2, 3};
    int buf[3] = {0, 0, 0};
    int rank = getRank();
    
    if(rank != 0) {
        data[1] = 10;
    }
    
    if(rank == 0) printf("Using MPI_Allreduce to sum two arrays\n");
    MPI_Allreduce(data, buf, 3, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 3);
}

void gather() {
    int data1[3] = {1, 2, 3};
    int data2[3] = {4, 5, 6};
    int buf[6] = {0, 0, 0, 0, 0, 0};
    int rank = getRank();
    
    int* ptr = data1;
    if(rank == 1) {
        ptr = data2;
    }
    
    if(rank == 0) printf("Using MPI_Gather to gather two arrays\n");
    MPI_Gather(ptr, 3, MPI_INT, buf, 3, MPI_INT, 0, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 6);
    
    for(int i = 0; i < 6; ++i) buf[i] = 0;
    
    if(rank == 0) printf("Using MPI_Allgather to gather two arrays\n");
    MPI_Allgather(ptr, 3, MPI_INT, buf, 3, MPI_INT, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 6);
}

void gatherv() {
    int data1[3] = {1, 2, 3};
    int data2[4] = {4, 5, 6, 7};
    int buf[7] = {0, 0, 0, 0, 0, 0, 0};
    int displacements[2] = {0, 3};
    int receiveCounts[2] = {3, 4};
    
    int* ptr = data1;
    int count = 3;
    if(getRank() == 1) {
        printf("Using MPI_Gatherv to send two arrays of different length\n");
        ptr = data2;
        count = 4;
    }
    
    MPI_Gatherv(ptr, count, MPI_INT, buf, receiveCounts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 7);
    
    for(int i = 0; i < 7; ++i) buf[i] = 0;
    
    if(getRank() == 1) {
        printf("Using MPI_Allgatherv to send two arrays of different length\n");
    }
    
    MPI_Allgatherv(ptr, count, MPI_INT, buf, receiveCounts, displacements, MPI_INT, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 7);
}

void scatter() {
    int data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int buf[4] = {0, 0, 0, 0};
    
    if(getRank() == 0) {
        printf("Using MPI_Scatter to distribute an array with 8 elements\n");
    }
    
    MPI_Scatter(data, 4, MPI_INT, buf, 4, MPI_INT, 0, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 4);
}

void scatterv() {
    int data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int buf[5] = {0, 0, 0, 0, 0};
    int displacements[2] = {0, 5};
    int sendCounts[2] = {5, 3};
    int receiveCount = 5;
    
    if(getRank() == 1) {
        receiveCount = 3;
        printf("Using MPI_Scatterv to distribute an array with 8 elements, receiving 5 and 3 elements\n");
    }
    
    MPI_Scatterv(data, sendCounts, displacements, MPI_INT, buf, receiveCount, MPI_INT, 0, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 5);
}

void alltoall() {
    int data1[4] = {1, 2, 3, 4};
    int data2[4] = {5, 6, 7, 8};
    int buf[4] = {0, 0, 0, 0};
    
    int* ptr = data1;
    if(getRank() == 1) {
        printf("Using MPI_Alltoall to send an array from each process to every other\n");
        ptr = data2;
    }
    
    //vgl https://rookiehpc.org/mpi/docs/mpi_alltoall/index.html
    MPI_Alltoall(ptr, 2, MPI_INT, buf, 2, MPI_INT, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 4);
}

void alltoallv() {
    int data1[3] = {1, 2, 3};
    int data2[4] = {4, 5, 6, 7};
    int buf[4] = {0, 0, 0, 0};
    
    //Process 0: Send {1} to 0 and {2, 3} to 1
    int sendCounts1[2] = {1, 2};
    int sendDisplacements1[2] = {0, 1};
    
    //Process 1: Send {4, 5, 6} to 0 and {7} to 1
    int sendCounts2[2] = {3, 1};
    int sendDisplacements2[2] = {0, 3};
    
    //Process 0: Receive {1} and {4, 5, 6}
    int receiveCounts1[2] = {1, 3};
    int displacements1[2] = {0, 1};
    
    //Process 1: Receive {2, 3} and {7}
    int receiveCounts2[2] = {2, 1};
    int displacements2[2] = {0, 2};
    
    int* sendPtr = sendCounts1;
    int* recvPtr = receiveCounts1;
    int* dataPtr = data1;
    int* rcvDisPtr = displacements1;
    int* sendDisPtr = sendDisplacements1;
    
    if(getRank() == 1) {
        printf("Using MPI_Alltoallv to do some crazy shit.\n");
        sendPtr = sendCounts2;
        recvPtr = receiveCounts2;
        dataPtr = data2;
        rcvDisPtr = displacements2;
        sendDisPtr = sendDisplacements2;
    }
    
    MPI_Alltoallv(dataPtr, sendPtr, sendDisPtr, MPI_INT, buf, recvPtr, rcvDisPtr, MPI_INT, MPI_COMM_WORLD);
    
    printArrayForAll(buf, 4);
}

void barrier() {
    if(getRank() == 0) {
        printf("Using MPI_Barrier");
        flushAndSleep();
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    if(getRank() == 1) {
        printf(" to synchronize this\n");
    }
}

void split() {
    if(getRank() == 0) {
        printf("Using MPI_Comm_split to create a new comm\n");
    }
    int rank = getRank();
    int color = rank % 2 == 0? 0 : MPI_UNDEFINED;
    
    MPI_Comm newComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &newComm);
    
    int newRank = -1;
    if(newComm != MPI_COMM_NULL) {
        MPI_Comm_rank(newComm, &newRank);
    }
    
    for(int i = 0; i < getWorldSize(); ++i) {
        if(rank == i) {
            printf("Previous rank: %i, new rank: %i\n", getRank(), newRank);
            flushAndSleep();
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

void group() {
    if(getRank() == 0) {
        printf("\nUsing MPI_Comm_group and MPI_Comm_incl and MPI_Comm_create to create a new group\n");
    }
    MPI_Group worldGroup;
    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    
    MPI_Group newGroup;
    int ranks[2] = {2, 3};
    MPI_Group_incl(worldGroup, 2, ranks, &newGroup);
    
    MPI_Comm newComm;
    MPI_Comm_create(MPI_COMM_WORLD, newGroup, &newComm);
    
    int newRank = -1;
    if(newComm != MPI_COMM_NULL) {
        MPI_Comm_rank(newComm, &newRank);
    }
    
    for(int i = 0; i < getWorldSize(); ++i) {
        if(getRank() == i) {
            printf("Previous rank: %i, new rank: %i\n", getRank(), newRank);
            flushAndSleep();
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    if(newComm != MPI_COMM_NULL) {
        MPI_Comm_free(&newComm);
    }
    MPI_Group_free(&worldGroup);
    MPI_Group_free(&newGroup);
}

void cart() {
    if(getRank() == 0) {
        printf("\nUsing MPI_Cart_create\n");
    }
    MPI_Comm gridComm;
    int dimensions[2] = {2, 2};
    int periods[2] = {0, 0};
    
    MPI_Cart_create(MPI_COMM_WORLD, 2, dimensions, periods, 0, &gridComm);
    int coords[2];
    MPI_Cart_coords(gridComm, getRank(), 2, coords);
    int newRank;
    MPI_Cart_rank(gridComm, coords, &newRank);
    
    for(int i = 0; i < getWorldSize(); ++i) {
        if(getRank() == i) {
            int left;
            int right;
            int top;
            int bottom;
            MPI_Cart_shift(gridComm, 0, 1, &left, &right);
            MPI_Cart_shift(gridComm, 1, 1, &top, &bottom);
            printf("Previous rank: %i, new rank: %i, coordinates: (%i, %i), left: %i, right: %i, top: %i, bottom: %i\n", getRank(), newRank, coords[0], coords[1], left, right, top, bottom);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

int main() {
    MPI_Init(NULL, NULL);
    
    if(getWorldSize() == 2) {
        sendReceive();
        sendReceive2();
        iSendReceive();
        iSendReceive2();
        bSendReceive();
        iBSendReceive();
        probe();
        iProbe();
        broadcast();
        reduce();
        allReduce();
        gather();
        gatherv();
        scatter();
        scatterv();
        alltoall();
        alltoallv();
        barrier();
    }
    else if(getWorldSize() == 4) {
        split();
        group();
        cart();
    }
    else {
        printf("Only world sized 2 and 4 are supported.\n");
    }
    
    MPI_Finalize();
}