// Compiled and executed with mpicc -o 4_2_h -O3 4_2_h.c -Wall && mpirun -np 8
// 4_2_h
#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * g)
 * 1. Jeder Prozess kann mit seinen Daten vor dem Synchronisieren alle bis auf 2
 * Einträge berechnen. Während des Wartens kann also bereits gerechnet werden.
 * Schema:
 *  - Schicke Randwerte an Nachbarn
 *  - Warte nicht-blockierend auf diese Randwerte (MPI_Irecv)
 *  - Berechne alle Einträge, die ohne die nötigen Randwerte berechnet werden
 * können
 *  - Nach der Rechnung: Warte auf die Randwerte, falls sie immer noch nicht
 * angekommen sind
 *  - Sobald die Randwerte angekommen sind, können die 2 verbliebenen Werte
 * berechnet werden
 *  - Beginne von vorn
 * 2. Bezogen auf a) - f): MPI_Recv ist blockierend und es sind 2 Aufrufe nötig
 * -> Nur mit einem gleichzeitig
 * 3. Für diese Aufgabe muss jeder Prozess zwei double-Werte schicken und
 * empfangen. Mit 8 byte pro double werden np*8 byte verschickt - eine
 * vernachlässigbare Datenmenge bei Übetragungsraten von GBit/s (die
 * Übetragungszeit liegt im Nanosekundenbereich und wird ab hier vernachlässigt)
 * 4. Ein Zeitschritt dauert 0.3ms + 0.1ms (+ Übertragungszeit) = 0.4ms.
 * 0.3ms, also 3/4 = 75% der Zeit wird gerechnet.
 *
 * f)
 * Jetzt kann während der 0.1s Wartezeit gerechnet werden, also dauert ein
 * Zeitschritt nur noch 0.3ms und 100% der Zeit wird gerechnet.
 */

/**
 * @brief Gets the rank (i. e. "Process number") of the current process
 */
int get_rank() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    return rank;
}

/**
 * @brief Returns the total number of processes
 */
int get_number_of_processes() {
    int np;
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    return np;
}

/**
 * @brief Sends and receives the temperatures on the left and right end of a
 * block to the neighboring left and right processes to synchronize the
 * temperatures at the edges
 * @param T_k The current temperatures with block_size entries
 * @param block_size Size of T_k
 * @param r Rank of this process
 * @param np Number of processes
 * @param request A MPI_Request that will be updated once the border values have
 * been received
 */
void synchronize_borders(double *T_k, int grid_size, int block_size, int r,
                         int np, MPI_Request *request) {
    int left_neighbor_rank = (r - 1) % np;
    if (left_neighbor_rank < 0) {
        left_neighbor_rank += np;
    }
    int right_neighbor_rank = (r + 1) % np;

    int index_left_send = r * block_size;
    int index_right_send = (r + 1) * block_size - 1;

    int index_left_receive = index_left_send - 1;
    if (index_left_receive < 0) {
        index_left_receive = grid_size - 1;
    }
    int index_right_receive = (index_right_send + 1) % grid_size;

    MPI_Send(T_k + index_left_send, 1, MPI_DOUBLE, left_neighbor_rank, 0,
             MPI_COMM_WORLD);
    // Element on the left from neighbor = Element on the right
    MPI_Irecv(T_k + index_right_receive, 1, MPI_DOUBLE, right_neighbor_rank, 0,
              MPI_COMM_WORLD, request);

    MPI_Send(T_k + index_right_send, 1, MPI_DOUBLE, right_neighbor_rank, 0,
             MPI_COMM_WORLD);
    // Element on the right from neighbor = Element on the left
    MPI_Irecv(T_k + index_left_receive, 1, MPI_DOUBLE, left_neighbor_rank, 0,
              MPI_COMM_WORLD, request);
}

/**
 * @brief Lets all processes send the average temperature to the master process.
 * The master process then averages the sum and prints it
 * @param T_average The calculated average temperature for this process
 * @param r The rank of this process
 * @param np Number of processes
 */
void gather_and_print_averages(double T_average, int r, int np) {
    if (r != 0) {
        // T_average for r == 0 doesn't need to be sent, it's already passed as
        // an argument
        MPI_Send(&T_average, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    if (r == 0) {
        double T_received;
        printf("Master avg: %f\n", T_average);
        for (int i = 1; i < np; ++i) {
            MPI_Recv(&T_received, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            printf("Received avg: %f\n", T_received);
            T_average += T_received;
        }
        T_average = T_average / np;
        printf("T_average: %f", T_average);
    }
}

/**
 * @brief swaps the content of two double arrays
 * @param a Pointer to the first address of the first array
 * @param b Pointer to the first address of the second array
 * @note The given implementation was incorrect, it assigned local variables in
 * the function body
 */
void swap(double **a, double **b) {
    double *temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Calculates the new temperature for a cell at index i
 * @param T_k The current temperatures
 * @param T_kn The calculated temperatures
 * @param grid_size Total number of cells
 * @param i The index of the value to be calculated
 */
void calculate_value(double *T_k, double *T_kn, int grid_size, int i) {
    const double delta_t = 0.02;
    const double conductivity = 0.1;

    int i_left = (i != 0) ? i - 1 : 1;
    int i_right = (i != grid_size - 1) ? i + 1 : i - 1;
    double dTdt_i = conductivity * (-2 * T_k[i] + T_k[i_left] + T_k[i_right]);
    T_kn[i] = T_k[i] + delta_t * dTdt_i;
}

int main() {
    MPI_Init(NULL, NULL); // MPI_INIT(&argc, &argv) if we want to use the
                          // arguments (which we don't)
    int num_procs = get_number_of_processes();
    int r = get_rank();

    int grid_size = 512 * 1024 * 1024;
    int num_time_steps = 3000;

    assert(grid_size % num_procs == 0); // Abort if the grid can not evenly be
                                        // distributed among the processes
    int block_size = grid_size / num_procs;

    double *T_k = (double *)malloc(sizeof(double) * grid_size);
    double *T_kn = (double *)malloc(sizeof(double) * grid_size);

    int block_begin = r * block_size;
    // Removed - 1 at the end, alternatively iterate to <= block_end
    int block_end = block_size * (r + 1);

    for (int i = block_begin; i < block_end; ++i) {
        T_k[i] = i;
    }

    for (int k = 0; k < num_time_steps; ++k) {
        MPI_Request request;
        synchronize_borders(T_k, grid_size, block_size, r, num_procs, &request);
        for (int i = block_begin + 1; i < block_end - 1; ++i) {
            calculate_value(T_k, T_kn, grid_size, i);
        }

        MPI_Wait(&request, MPI_STATUS_IGNORE);

        calculate_value(T_k, T_kn, grid_size, block_begin);
        calculate_value(T_k, T_kn, grid_size, block_end - 1);

        swap(&T_k, &T_kn);
    }

    double T_average = 0;
    for (int i = block_begin; i < block_end; ++i) {
        T_average += T_k[i];
    }
    T_average = T_average / block_size;

    gather_and_print_averages(T_average, r, num_procs);

    MPI_Finalize();
}