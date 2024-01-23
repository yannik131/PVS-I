#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Process {
    int rank; //Rank of this process
    int worldSize; //Total number of processes
    int worldGridSize; //Edge length of the process grid, sqrt(worldSize)
    int coordinates[2]; //Grid coordinates of this process
    MPI_Comm gridComm; //Shared communication among all processes
    double* grid; //The simulation values for this process, unneeded entries are 0
    int N; //The edge length of the simulation grid
    int totalGridSize; //N * N
    int gridEdgeLength; //N / worldGridSize, number of entries on the border of each process in the simulation grid
} Process;

int calculateSquare(int N) {
    int root = sqrt(N);
    assert(root * root == N);

    return root;
}

void printProcess(Process info) {
    printf("World size: %d\ngridEdgeLength: %d\nRank: %d\n(x, y): (%d, %d)\n\n", info.worldSize, info.gridEdgeLength, info.rank, info.coordinates[0],
           info.coordinates[1]);
}

double calculateEntry(int N, double x, double y) { return y * N + x; }

int isInInterval(int x, int lowerBound, int upperBound) { return x >= lowerBound && x <= upperBound; }

/**
 * @brief Creates a N x N array, but only fills a block of blockSize at (x, y)
 * with initial values.
 * @note To reduce RAM usage, it would be better to create a (blockSize+1) x
 * (blockSize+1) array and pad the rows and columns around the center with zeros
 * and later use that padding to synchronize the values. However, to make
 * indexing simpler, the N x N array is used instead.
 */
void createLocalGrid(int N, Process* process) {
    int x = process->coordinates[0];
    int y = process->coordinates[1];

    process->grid = (double *)calloc(N * N, sizeof(double));
    process->N = N;
    process->totalGridSize = N * N;

    assert(N * N % process->worldSize == 0);      // Make sure the grid can be distributed
    int gridEdgeLength = N / process->worldGridSize;
    process->gridEdgeLength =  gridEdgeLength;
    
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int index = N * i + j;
            if (isInInterval(i, y * gridEdgeLength, (y + 1) * gridEdgeLength - 1) &&
                isInInterval(j, x * gridEdgeLength, (x + 1) * gridEdgeLength - 1)) {
                process->grid[index] = calculateEntry(N, j, i);
            }
        }
    }
}

void printGrid(Process process) {
    int x = process.coordinates[0];
    int y = process.coordinates[1];
    printf("Rank: %d\n", process.rank);
    
    for (int i = 0; i < process.N; ++i) {
        for (int j = 0; j < process.N; ++j) {
            int index = process.N * i + j;
            if (isInInterval(i, y * process.gridEdgeLength, (y + 1) * process.gridEdgeLength - 1) &&
                isInInterval(j, x * process.gridEdgeLength, (x + 1) * process.gridEdgeLength - 1)) {
            }
            printf("%.0f ", process.grid[index]);
            if (process.grid[index] < 10) {
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

Process createProcessGrid() {
    Process process;
    MPI_Comm_rank(MPI_COMM_WORLD, &process.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process.worldSize);

    process.worldGridSize = calculateSquare(process.worldSize);
    int dimensions[2] = {process.worldGridSize, process.worldGridSize};
    int periods[2] = {1, 1};

    MPI_Cart_create(MPI_COMM_WORLD, process.worldGridSize, dimensions, periods, 0, &process.gridComm);
    MPI_Cart_coords(process.gridComm, process.rank, process.worldGridSize, process.coordinates);

    return process;
}

void sanatizeNeighborCoordinates(int* coordinates[4], int n) {
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 2; ++j) {
            if(coordinates[i][j] < 0) {
                coordinates[i][j] = n - 1;
            }
            else if(coordinates[i][j] == n) {
                coordinates[i][j] = 0;
            }
        }
    }
}

int* getLeftRightBorderIndexes(int N, int* coordinates, int gridEdgeLength, int right) {
    int* indexes = (int*)malloc(gridEdgeLength * sizeof(int));
    int index = (N * coordinates[1] + coordinates[0]) * gridEdgeLength;
    
    if(right) {
        index += gridEdgeLength - 1;
    }
    
    for(int i = 0; i < gridEdgeLength; ++i) {
        indexes[i] = index;
        index += N;
    }
    
    return indexes;
}

int getTopBottomBorderOffset(int N, int* coordinates, int gridEdgeLength, int top) {
    //(coordinates[0], coordinates[1]) * gridEdgeLength = (x, y)
    //offset = N*y + x
    int offset = (N * coordinates[1] + coordinates[0]) * gridEdgeLength;
    if(top) {
        return offset;
    }
    
    return offset + (gridEdgeLength - 1) * N;
}

//TODO: Return requests, wait outside for them all at once
//Note: Creating 8 requests in synchronize_borders and waiting on them causes not all borders to sync and the program to crash. Maybe because of simultaneous access to the array?
void sendReceive(int count, MPI_Comm comm, double* source, int targetRank, double* destination, int sourceRank) {
    MPI_Request request;
    MPI_Isendrecv(source, count, MPI_DOUBLE, targetRank, 0, destination, count, MPI_DOUBLE, sourceRank, 0, comm, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    
    /*
    int bufferSize = count + MPI_BSEND_OVERHEAD;
    void* sendBuffer = malloc(bufferSize);
    MPI_Buffer_attach(sendBuffer, bufferSize);
    
    MPI_Request sendRequest;
    //printf("Sending: %d -> %d\n", process.rank, targetRank);
    MPI_Ibsend(source, count, MPI_DOUBLE, targetRank, 0, comm, &sendRequest);
    
    MPI_Request receiveRequest;
    //printf("Receiving: %d <- %d\n", process.rank, sourceRank);
    MPI_Irecv(destination, count, MPI_DOUBLE, sourceRank, 0, comm, &receiveRequest);
    
    MPI_Wait(&sendRequest, MPI_STATUS_IGNORE);
    MPI_Wait(&receiveRequest, MPI_STATUS_IGNORE);
    
    MPI_Buffer_detach(&sendBuffer, &bufferSize);
    free(sendBuffer);*/
}

/**
 * @brief Sends data at sourceOffset to targetRank and receives data at destinationOffset from sourceRank
*/
void sendReceiveWithOffset(Process process, int sourceOffset, int targetRank, int destinationOffset, int sourceRank) {
    double* source = process.grid + sourceOffset;
    double* destination = process.grid + destinationOffset;
    
    sendReceive(process.gridEdgeLength, process.gridComm, source, targetRank, destination, sourceRank);
}

void sendReceiveWithIndexes(Process process, int* sourceIndexes, int targetRank, int* destinationIndexes, int sourceRank) {
    double* source = (double*)malloc(process.gridEdgeLength * sizeof(double));
    for(int i = 0; i < process.gridEdgeLength; ++i) {
        source[i] = process.grid[sourceIndexes[i]];
    }
    double* destination = (double*)malloc(process.gridEdgeLength * sizeof(double));
    
    sendReceive(process.gridEdgeLength, process.gridComm, source, targetRank, destination, sourceRank);
    
    for(int i = 0; i < process.gridEdgeLength; ++i) {
        process.grid[destinationIndexes[i]] = destination[i];
    }
    
    free(source);
    free(destination);
}

void synchronize_borders(Process process) {
    int upperNeighbor[2] = {process.coordinates[0], process.coordinates[1]-1};
    int rightNeighbor[2] = {process.coordinates[0]+1, process.coordinates[1]};
    int lowerNeighbor[2] = {process.coordinates[0], process.coordinates[1]+1};
    int leftNeighbor[2] = {process.coordinates[0]-1, process.coordinates[1]};
    
    int* neighbors[4] = {upperNeighbor, rightNeighbor, lowerNeighbor, leftNeighbor};
    sanatizeNeighborCoordinates(neighbors, process.worldGridSize);
    
    int topNeighborRank;
    int bottomNeighborRank;
    int leftNeighborRank;
    int rightNeighborRank;
    
    MPI_Cart_rank(process.gridComm, upperNeighbor, &topNeighborRank);
    MPI_Cart_rank(process.gridComm, lowerNeighbor, &bottomNeighborRank);
    MPI_Cart_rank(process.gridComm, leftNeighbor, &leftNeighborRank);
    MPI_Cart_rank(process.gridComm, rightNeighbor, &rightNeighborRank);
    
    //Send top, receive bottom
    int sourceOffset = getTopBottomBorderOffset(process.N, process.coordinates, process.gridEdgeLength, 1);
    int destinationOffset = getTopBottomBorderOffset(process.N, lowerNeighbor, process.gridEdgeLength, 1);
    
    sendReceiveWithOffset(process, sourceOffset, topNeighborRank, destinationOffset, bottomNeighborRank);
    
    //Send bottom, receive top
    sourceOffset = getTopBottomBorderOffset(process.N, process.coordinates, process.gridEdgeLength, 0);
    destinationOffset = getTopBottomBorderOffset(process.N, upperNeighbor, process.gridEdgeLength, 0);
    
    sendReceiveWithOffset(process, sourceOffset, bottomNeighborRank, destinationOffset, topNeighborRank);
    
    //Send right, receive left
    int* sourceIndexes = getLeftRightBorderIndexes(process.N, process.coordinates, process.gridEdgeLength, 1);
    int* destinationIndexes = getLeftRightBorderIndexes(process.N, leftNeighbor, process.gridEdgeLength, 1);
    
    sendReceiveWithIndexes(process, sourceIndexes, rightNeighborRank, destinationIndexes, leftNeighborRank);
    
    free(sourceIndexes);
    free(destinationIndexes);
    
    //Send left, receive right
    sourceIndexes = getLeftRightBorderIndexes(process.N, process.coordinates, process.gridEdgeLength, 0);
    destinationIndexes = getLeftRightBorderIndexes(process.N, rightNeighbor, process.gridEdgeLength, 0);
    
    sendReceiveWithIndexes(process, sourceIndexes, leftNeighborRank, destinationIndexes, rightNeighborRank);
    
    free(sourceIndexes);
    free(destinationIndexes);
}

int main() {
    MPI_Init(NULL, NULL);

    Process process = createProcessGrid();

    for (int i = 0; i < process.worldSize; ++i) {
        if (process.rank == i) {
            //printProcess(process);
            createLocalGrid(10, &process);
        }

        MPI_Barrier(process.gridComm);
    }
    
    synchronize_borders(process);
    
    for (int i = 0; i < process.worldSize; ++i) {
        if (process.rank == i) {
            printGrid(process);
        }

        MPI_Barrier(process.gridComm);
    }
    
    MPI_Finalize();
}