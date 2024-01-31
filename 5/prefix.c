#include <mpi.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int* createArray(int N) {
    int* array = (int*)malloc(N * sizeof(int));
    
    for(int i = 0; i < N; ++i) {
        array[i] = i;
    }
    
    return array;
}

int main() {
    MPI_Init(NULL, NULL);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    
    int N = 10;
    assert(N % worldSize == 0);
    
    int sectionSize = N / worldSize;
    int* array = (int*)malloc(sectionSize * sizeof(int));
    int* wholeArray;
    
    if(rank == 0) {
        wholeArray = createArray(N);
        int sum = wholeArray[0];
        printf("Expected sum: ");
        for(int i = 1; i < N; ++i) {
            printf("%i ", sum);
            sum += wholeArray[i];
        }
        printf("\n");
    }
    
    int result = MPI_Scatter(wholeArray, sectionSize, MPI_INT, array, sectionSize, MPI_INT, 0, MPI_COMM_WORLD);
    if(result != MPI_SUCCESS) {
        printf("Rank %i: Scatter failed\n", rank);
    }
    
    if(rank == 0) {
        printf("Scattering: ");
        for(int i = 0; i < N; ++i) {
            printf("%i ", wholeArray[i]);
        }
        printf("\n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    for(int i = 0; i < worldSize; ++i) {
        if(rank == i) {
            printf("Rank: %i, received: ", rank);
            for(int i = 0; i < sectionSize; ++i) {
                printf("%i ", array[i]);
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    if(rank == 0) {
        array = wholeArray;
    }
    for(int i = 1; i < sectionSize; ++i) {
        array[i] = array[i - 1] + array[i];
    }
    
    int leftSum = 0;
    int rightSum = array[sectionSize - 1];
    int leftRank = rank - 1;
    int rightRank = rank + 1;
    
    if(rank == 0) {
        MPI_Send(&rightSum, 1, MPI_INT, rightRank, 0, MPI_COMM_WORLD);
    }
    else if(rank == worldSize - 1) {
        MPI_Recv(&leftSum, 1, MPI_INT, leftRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Sendrecv(&rightSum, 1, MPI_INT, rightRank, 0, &leftSum, 1, MPI_INT, leftRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    if(rank != 0) {
        for(int i = 0; i  < sectionSize; ++i) {
            array[i] += leftSum;
        }
        MPI_Send(array, sectionSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    if(rank == 0) {
        for(int i = 1; i < worldSize; ++i) {
            MPI_Recv(wholeArray + i * sectionSize, sectionSize, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        printf("Total prefix sum:\n");
        for(int i = 0; i < N; ++i) {
            printf("%i ", wholeArray[i]);
        }
    }
    
    MPI_Finalize();
}