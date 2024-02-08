#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

int reduce_sequential(const int *sendbuf, int *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    assert(datatype == MPI_INT);
    assert(op == MPI_SUM);
    
    int rank;
    MPI_Comm_rank(comm, &rank);
    
    int worldSize;
    MPI_Comm_size(comm, &worldSize);
    
    int* gatherBuffer = (int*)malloc(worldSize * count * sizeof(int));
    MPI_Gather(sendbuf, count, datatype, gatherBuffer, count, datatype, root, comm);
    
    if(rank == root) {
        memset(recvbuf, 0, count * sizeof(int));
        for(int i = 0; i < worldSize; ++i) {
            for(int j = 0; j < count; ++j) {
                recvbuf[j] += gatherBuffer[i*count + j];
            }
        }
    }
    
    return MPI_SUCCESS;
}

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

int getLeftChildRank() { return getRank() * 2 + 1; }
int getRightChildRank() { return getRank() * 2 + 2; }
int getParentRank() { return (int)((getRank() - 1) / 2); }

void receiveFromChildren(int** leftReceive, int** rightReceive, int count) {
    int leftChildRank = getLeftChildRank();
    int rightChildRank = getRightChildRank();
    int worldSize = getWorldSize();
    
    if(leftChildRank < worldSize) {
        *leftReceive = (int*)malloc(count * sizeof(int));
        MPI_Recv(*leftReceive, count, MPI_INT, leftChildRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Rank %i received from left (%i): ", getRank(), leftChildRank);
        for(int i = 0; i < count; ++i) printf("%i ", *leftReceive[i]);
        printf("\n");
    }
    if(rightChildRank < worldSize) {
        *rightReceive = (int*)malloc(count * sizeof(int));
        MPI_Recv(*rightReceive, count, MPI_INT, rightChildRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Rank %i received from right (%i): ", getRank(), rightChildRank);
        for(int i = 0; i < count; ++i) printf("%i ", *rightReceive[i]);
        printf("\n");
    }
}

int reduce_tree(const int *sendbuf, int *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    int* leftReceive = NULL;
    int* rightReceive = NULL;
    
    receiveFromChildren(&leftReceive, &rightReceive, count);
    
    for(int i = 0; i < count; ++i) {
        recvbuf[i] = sendbuf[i];
        if(leftReceive) recvbuf[i] += leftReceive[i];
        if(rightReceive) recvbuf[i] += rightReceive[i];
    }
    
    free(leftReceive);
    free(rightReceive);

    if(getRank() == 0) {
        return MPI_SUCCESS;
    }
    
    MPI_Send(recvbuf, count, MPI_INT, getParentRank(), 0, MPI_COMM_WORLD);
    
    return MPI_SUCCESS;
}


int main() {
    MPI_Init(NULL, NULL);
    
    int rank = getRank();
    int worldSize = getWorldSize();
    
    int sum = -1;
    reduce_sequential(&rank, &sum, 1, MPI_INT, MPI_SUM, 1, MPI_COMM_WORLD);
    
    for(int i = 0; i < worldSize; ++i) {
        if(rank == i) {
            printf("Rank: %i. Sum: %i\n", rank, sum);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
    
    return 0;
}