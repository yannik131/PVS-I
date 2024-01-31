#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Aufgabe 6.2
 * union: Vereinigung            -> {a, b, c, d, e, f, g, h, j, l}
 * intersection: Schnittmenge    -> {c, d, f, g, h}
 * difference(g2, g1): g2-g1     -> {a, l}
 * difference(g1, g2): g1-g2     -> {b, e, j}
 */

/**
 * Aufgabe 6.1
 * Notes:
 * - My solution copies the whole grid to every process and lets the processes work only on the relevant
 * subsection of the grid. It would have been more memory efficient if every process only copied the relevant
 * section instead of the whole grid
 * Zusatzfrage:
 * - In this exercise, every process has to synchronize its top, right, bottom and left border which contain m
 * / n values each. In total, 4 * m / n * n = 4 * m values have to be transmitted if each process handles a m
 * * m square. If we don't assign squares to each process, each process still handles m * m / n elements.
 * These elements form blocks which have a small height of (m * m / n) / m = m / n, if we assume m >= n. Since
 * height * width = m * m / n, we get m / n * width = m * m / n <-> width = m. n*(2*width + 2*height) = n * 2
 * * m + 2 * m = 2 * m * (n + 1) elements have to be transmitted, which is more compared to the 4 * m elements
 * for 2D-squares with square numbers > 1. For m < n, this is probably true as well, since squares minimize
 * the maximum edge length among rectangles. In case the number of processes is not a square number and
 * squares are therefore impossible to form, rectangles being as close as possible to squares would probably
 * be the best method.
 */

typedef struct Process {
    int rank;           // Rank of this process
    int worldSize;      // Total number of processes
    int worldGridSize;  // Edge length of the process grid, sqrt(worldSize)
    int coordinates[2]; // Grid coordinates (x, y) of this process
    MPI_Comm gridComm;  // Shared communication among all processes
    double *grid;       // The simulation values for this process, unneeded entries are not used
    int N;              // The edge length of the simulation grid
    int totalGridSize;  // N * N
    int borderLength; // N / worldGridSize, number of entries on the border of each process in the simulation
                      // grid
} Process;

/**
 * @brief Takes the dimension of the N x N 2d heat grid and creates a process grid with MPI_Cart_create to
 * evenly distribute the processes among the grid. Each process handles a grid of process.borderLength x
 * process.borderLength
 */
Process createProcessGrid(int N) {
    Process process;
    MPI_Comm_rank(MPI_COMM_WORLD, &process.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process.worldSize);

    process.worldGridSize = sqrt(process.worldSize);
    assert(process.worldGridSize * process.worldGridSize ==
           process.worldSize); // The number of processes must be a square number

    int dimensions[2] = {process.worldGridSize, process.worldGridSize};
    int periods[2] = {1, 1};

    process.N = N;
    process.totalGridSize = N * N;
    assert(process.totalGridSize % process.worldSize == 0); // Make sure the grid can be distributed
    int borderLength = N / process.worldGridSize;
    process.borderLength = borderLength;

    MPI_Cart_create(MPI_COMM_WORLD, process.worldGridSize, dimensions, periods, 0, &process.gridComm);
    MPI_Cart_coords(process.gridComm, process.rank, process.worldGridSize, process.coordinates);

    return process;
}

/**
 * @brief To send the left and right border elements of the grid, the indexes of those elements are gathered
 * first. With the indexes, the corresponding elements can be put in one single array and be sent/received.
 * @note Doing this more than once is stupid.
 */
int *getLeftRightBorderIndexes(int N, int *coordinates, int borderLength, int right) {
    int *indexes = (int *)malloc(borderLength * sizeof(int));
    int index = (N * coordinates[1] + coordinates[0]) * borderLength;

    if (right) {
        index += borderLength - 1;
    }

    for (int i = 0; i < borderLength; ++i) {
        indexes[i] = index;
        index += N;
    }
    return indexes;
}

/**
 * @brief To send the top and bottom border, the elements can be sent directly if the correct offset is
 * obtained.
 */
int getTopBottomBorderOffset(int N, int *coordinates, int borderLength, int top) {
    //(coordinates[0], coordinates[1]) * borderLength = (x, y)
    // offset = N*y + x
    int offset = (N * coordinates[1] + coordinates[0]) * borderLength;
    if (top) {
        return offset;
    }

    return offset + (borderLength - 1) * N;
}

/**
 * @brief Sends count elements from source to targetRank and receives count elements from sourceRank and
writes them to destination
 * @note Creating all requests in synchronize_borders and waiting on them causes not all borders to sync and
the program
// to crash. Maybe because of simultaneous access to the array?
*/
void sendReceive(int count, MPI_Comm comm, double *source, int targetRank, double *destination,
                 int sourceRank) {
    MPI_Request request;
    MPI_Isendrecv(source, count, MPI_DOUBLE, targetRank, 0, destination, count, MPI_DOUBLE, sourceRank, 0,
                  comm, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
}

/**
 * @brief Wrapper for sendReceive for sending/receiving top/bottom border
 */
void sendReceiveWithOffset(Process process, int sourceOffset, int targetRank, int destinationOffset,
                           int sourceRank) {
    double *source = process.grid + sourceOffset;
    double *destination = process.grid + destinationOffset;

    sendReceive(process.borderLength, process.gridComm, source, targetRank, destination, sourceRank);
}

/**
 * @brief Wrapper for sendReceive for sending/receiving left/right border. This case requires the appropriate
 * indexes to be determined, so that the values can be sent/received in one go
 */
void sendReceiveWithIndexes(Process process, int *sourceIndexes, int targetRank, int *destinationIndexes,
                            int sourceRank) {
    double *source = (double *)malloc(process.borderLength * sizeof(double));
    for (int i = 0; i < process.borderLength; ++i) {
        source[i] = process.grid[sourceIndexes[i]];
    }
    double *destination = (double *)malloc(process.borderLength * sizeof(double));

    sendReceive(process.borderLength, process.gridComm, source, targetRank, destination, sourceRank);

    for (int i = 0; i < process.borderLength; ++i) {
        process.grid[destinationIndexes[i]] = destination[i];
    }

    free(source);
    free(destination);
}

/**
 * @brief Sends every border (top, right, bottom, left) to the appropriate neighbor process and receives the
 * sent border. The borders are synced with all processes in this order, i. e. all processes will first send
 * and then receive the top border with MPI_ISendrecv, then the right one and so on.
 */
void synchronize_borders(Process process) {
    int topNeighbor[2] = {process.coordinates[0], process.coordinates[1] - 1};
    int rightNeighbor[2] = {process.coordinates[0] + 1, process.coordinates[1]};
    int bottomNeighbor[2] = {process.coordinates[0], process.coordinates[1] + 1};
    int leftNeighbor[2] = {process.coordinates[0] - 1, process.coordinates[1]};

    int topNeighborRank;
    int bottomNeighborRank;
    int leftNeighborRank;
    int rightNeighborRank;

    MPI_Cart_shift(process.gridComm, 0, 1, &leftNeighborRank, &rightNeighborRank);
    MPI_Cart_shift(process.gridComm, 1, 1, &topNeighborRank, &bottomNeighborRank);

    MPI_Cart_coords(process.gridComm, topNeighborRank, process.worldGridSize, topNeighbor);
    MPI_Cart_coords(process.gridComm, rightNeighborRank, process.worldGridSize, rightNeighbor);
    MPI_Cart_coords(process.gridComm, bottomNeighborRank, process.worldGridSize, bottomNeighbor);
    MPI_Cart_coords(process.gridComm, leftNeighborRank, process.worldGridSize, leftNeighbor);
    
    // Send top, receive bottom
    int sourceOffset = getTopBottomBorderOffset(process.N, process.coordinates, process.borderLength, 1);
    int destinationOffset = getTopBottomBorderOffset(process.N, bottomNeighbor, process.borderLength, 1);

    sendReceiveWithOffset(process, sourceOffset, topNeighborRank, destinationOffset, bottomNeighborRank);

    // Send bottom, receive top
    sourceOffset = getTopBottomBorderOffset(process.N, process.coordinates, process.borderLength, 0);
    destinationOffset = getTopBottomBorderOffset(process.N, topNeighbor, process.borderLength, 0);

    sendReceiveWithOffset(process, sourceOffset, bottomNeighborRank, destinationOffset, topNeighborRank);

    // Send right, receive left
    int *sourceIndexes = getLeftRightBorderIndexes(process.N, process.coordinates, process.borderLength, 1);
    int *destinationIndexes = getLeftRightBorderIndexes(process.N, leftNeighbor, process.borderLength, 1);

    sendReceiveWithIndexes(process, sourceIndexes, rightNeighborRank, destinationIndexes, leftNeighborRank);

    free(sourceIndexes);
    free(destinationIndexes);

    // Send left, receive right
    sourceIndexes = getLeftRightBorderIndexes(process.N, process.coordinates, process.borderLength, 0);
    destinationIndexes = getLeftRightBorderIndexes(process.N, rightNeighbor, process.borderLength, 0);

    sendReceiveWithIndexes(process, sourceIndexes, leftNeighborRank, destinationIndexes, rightNeighborRank);

    free(sourceIndexes);
    free(destinationIndexes);
}

void swap(double **a, double **b) {
    double *temp = *b;
    *b = *a;
    *a = temp;
}

int main(int argc, char **args) {
    MPI_Init(NULL, NULL);

    double delta_t = 0.02;
    int grid_size_x = 1024;
    int grid_size_y = 1024;
    assert(grid_size_x == grid_size_y);

    int num_time_steps = 3000;
    double conductivity = 0.1;

    double *T_k = (double *)malloc(sizeof(double) * grid_size_x * grid_size_y);
    double *T_kn = (double *)malloc(sizeof(double) * grid_size_x * grid_size_y);

    Process process = createProcessGrid(grid_size_x);
    process.grid = T_k;

    int xOffset = process.coordinates[0] * process.borderLength;
    int yOffset = process.coordinates[1] * process.borderLength;

    for (int y = yOffset; y < yOffset + process.borderLength; y++) {
        for (int x = xOffset; x < xOffset + process.borderLength; x++) {
            T_k[grid_size_x * y + x] = x + y;
        }
    }

    for (int k = 0; k < num_time_steps; k++) {
        synchronize_borders(process);
        for (int y = yOffset; y < yOffset + process.borderLength; y++) {
            for (int x = xOffset; x < xOffset + process.borderLength; x++) {
                int i = grid_size_x * y + x;

                int i_left = x != 0 ? i - 1 : i + 1;
                int i_right = x != grid_size_x - 1 ? i + 1 : i - 1;
                int i_down = y != 0 ? i - grid_size_x : i + grid_size_x;
                int i_up = y != grid_size_y - 1 ? i + grid_size_x : i - grid_size_x;

                double dTdt_i =
                    conductivity * (-4 * T_k[i] + T_k[i_left] + T_k[i_right] + T_k[i_down] + T_k[i_up]);
                T_kn[i] = T_k[i] + delta_t * dTdt_i;
            }
        }
        swap(&T_k, &T_kn);
    }

    double T_sum = 0;
    for (int y = yOffset; y < yOffset + process.borderLength; y++) {
        for (int x = xOffset; x < xOffset + process.borderLength; x++) {
            T_sum += T_k[grid_size_x * y + x];
        }
    }

    double T_global_sum;
    MPI_Reduce(&T_sum, &T_global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (process.rank == 0) {
        double T_average = T_global_sum / (grid_size_x * grid_size_x);
        // The average is decreasing slightly with every time step, probably due to a off-by-one error
        // somewhere
        printf("Average: %f\n", T_average);
    }

    free(T_k);
    free(T_kn);

    MPI_Finalize();

    return 0;
}