#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int reduce_sequential(const int *sendbuf, int *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    assert(datatype == MPI_INT);
    assert(op == MPI_SUM);
    
    int rank;
    MPI_Comm_rank(comm, &rank);
    
    int worldSize;
    MPI_Comm_size(comm, &worldSize);
    
    if(rank != root) {
        MPI_Send(sendbuf, count, datatype, root, 0, MPI_COMM_WORLD);
    }
    else {
        for(int i = 0; i < count; ++i) {
            recvbuf[i] = sendbuf[i];
        }
        for(int i = 0; i < worldSize; ++i) {
            if(i == root) {
                continue;
            }
            int* received = (int*)malloc(count * sizeof(int));
            MPI_Recv(received, count, datatype, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for(int i = 0; i < count; ++i) {
                recvbuf[i] += received[i];
            }
        }
    }
    
    return MPI_SUCCESS;
}

int main() {
    MPI_Init(NULL, NULL);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    
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