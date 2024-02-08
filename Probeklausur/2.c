//Run this with -oversubscribe if 16 cores are not available on your system
#include <mpi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void flushAndSleep() {
    fflush(stdout);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    
    nanosleep(&ts, NULL);
}

int getWorldSize() {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    
    return worldSize;
}

/**
 * @brief Method: 
 * - Processes left (right) of the root column receive data from their left (right) neighbor
 * - Once they received the data or if they don't have a left (right) neighbor, they reduce and send the result to their right (left) neighbor
 * - Processes in the root column above (below) the root receive data from their top (bottom) neighbor
 * - Once they received the data or if they don't have a top (bottom) neighbor, they reduce and send the result to their bottom (top) neighbor
 * - The root receives data from all sides
 * - Once the root received all data, it reduces and writes the result to recvbuf
*/
void reduce_grid(void* sendbuf, void* recvbuf, int count, MPI_Comm comm) {
    void* tmpBufRecv = malloc(count * sizeof(double));
    void* tmpBufReduce = malloc(count * sizeof(double));
    memcpy(tmpBufReduce, sendbuf, count * sizeof(double));
    
    int coords[2], dims[2], periods[2];
    MPI_Cart_get(comm, 2, dims, periods, coords);
    
    int top, right, bottom, left;
    MPI_Cart_shift(comm, 0, 1, &left, &right);
    MPI_Cart_shift(comm, 1, 1, &top, &bottom);
    
    int rootCoords[2] = {dims[0] / 2, dims[1] / 2};
    int inRootColumn = coords[0] == rootCoords[0];
    int isRoot = inRootColumn && coords[1] == rootCoords[1];
    
    #define send(target) MPI_Send(tmpBufReduce, count, MPI_DOUBLE, target, 0, comm);
    #define recv(src) \
        MPI_Recv(tmpBufRecv, count, MPI_DOUBLE, src, 0, comm, MPI_STATUS_IGNORE); \
        for(int i = 0; i < count; ++i) { \
            ((double*)tmpBufReduce)[i] += ((double*)tmpBufRecv)[i]; \
        }
    
    
    if(left == MPI_PROC_NULL) {
        send(right);
    }
    else if(right == MPI_PROC_NULL) {
        send(left);
    }
    else if(!inRootColumn) {
        if(coords[0] > rootCoords[0]) {
            recv(right);
            send(left);
        }
        else {
            recv(left);
            send(right);
        }
    }
    else if(!isRoot) {
        recv(left);
        recv(right);
        if(top == MPI_PROC_NULL) {
            send(bottom);
        }
        else if(bottom == MPI_PROC_NULL) {
            send(top);
        }
        else if(coords[1] < rootCoords[1]) {
            recv(top);
            send(bottom);
        }
        else {
            recv(bottom);
            send(top);
        }
    }
    else {
        recv(top);
        recv(right);
        recv(bottom);
        recv(left);
        memcpy(recvbuf, tmpBufReduce, count * sizeof(double));
    }
    
    free(tmpBufRecv);
    free(tmpBufReduce);
}

int main() {
    MPI_Init(NULL, NULL);
    
    int dims[2] = {4, 4};
    int periods[2] = {0, 0};
    MPI_Comm gridComm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0,  &gridComm);
    
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double send = (double)rank;
    double receive;
    
    reduce_grid(&send, &receive, 1, gridComm);
    
    MPI_Finalize();
}