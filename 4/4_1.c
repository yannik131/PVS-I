#include "galton.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

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
 * @brief Runs the simulation on a single process. The balls are evenly
 * distributed among the processes. The results are sent to the master process
 * (rank 0) with MPI_Send
 * @param state The state of the simulation (total number of balls, compartments
 * and rows of the galton board)
 */
void run_simulation_process(SimulationState state) {
    int rank = get_rank();
    int np = get_number_of_processes() -
             1; // Discount the master process, it's not simulating

    int remainder = state.number_of_balls % np;
    state.number_of_balls = state.number_of_balls / np;

    if (rank <= remainder) {
        // If we can't evenly distribute the balls among the processes, the
        // first n = remainder processes will take 1 extra ball.
        ++state.number_of_balls;
    }

    int *histogram = run_simulation(state);
    int size = state.number_of_compartments;
    MPI_Send(histogram, size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    free(histogram);
}

/**
 * @brief Gathers the histograms sent by the other processes (ranks unequal 0)
 * using MPI_Recv, adds the results up and displays them
 * @param state State of the simulation
 */
void gather_simulation_results(SimulationState state) {
    int np = get_number_of_processes();
    int size = state.number_of_compartments;
    int *histogram_sum = (int *)calloc(size, sizeof(int));
    int *histogram = (int *)malloc(size * sizeof(int));
    for (int i = 1; i < np; ++i) {
        MPI_Recv(histogram, size, MPI_INT, i, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        printf("Result of process %i: ", i);
        for (int j = 0; j < size; ++j) {
            printf("%i ", histogram[j]);
            histogram_sum[j] += histogram[j];
        }
        printf("\n");
    }

    display_histogram(histogram_sum, state);
    free(histogram);
}

/**
 * @brief To broadcast the SimulationState struct across the processes, its
 * layout has to be defined here so that MPI can understand the sent data
 * @param MPI_state The MPI_Datatype that needs its layout to be defined
 * @note In this particular example, the SimulationState struct just contains 3
 * byte sized fields (3 ints), so MPI_BYTE could also be used as the datatype
 * for the broadcast without problems (with sizeof(SimulationState) for count).
 * This would not work for non-byte-sized fields like char*, so I wanted to
 * practice the general approach.
 */
void MPI_simulation_state_define_layout(MPI_Datatype *MPI_state) {
    const int N = 3; // Number of struct members

    MPI_Datatype types[N] = {MPI_INT, MPI_INT, MPI_INT}; // types of the members
    int blocklengths[N] = {
        1, 1, 1}; // 3 ints with length 1 (array would have length n)
    MPI_Aint offsets[N];

    // MPI doesn't know about the layout of the struct. Offsets tell MPI where
    // to find what
    offsets[0] = offsetof(SimulationState, number_of_balls);
    offsets[1] = offsetof(SimulationState, number_of_compartments);
    offsets[2] = offsetof(SimulationState, number_of_rows);

    MPI_Type_create_struct(N, blocklengths, offsets, types, MPI_state);
    MPI_Type_commit(MPI_state);
}

/**
 * @brief Lets the master process read in the simulation state from the user and
 * uses MPI_Bcast to broadcast it to the other processes
 * @return The simulation state that was put in by the user
 */
SimulationState read_and_broadcast_state() {
    SimulationState state;
    MPI_Datatype MPI_state;

    int rank = get_rank();
    if (rank == 0) {
        state = read_simulation_state();
    }

    MPI_simulation_state_define_layout(&MPI_state);
    MPI_Bcast(&state, 1, MPI_state, 0, MPI_COMM_WORLD);
    MPI_Type_free(&MPI_state);

    return state;
}

/**
 * @brief Runs the galton board simulation on the number of processes specified
 * by the -np option of mpirun. The processes of rank != 0 execute the
 * simulation and the master process (rank = 0) gathers the results and displays
 * them.
 */
int main() {
    MPI_Init(NULL, NULL);

    SimulationState state = read_and_broadcast_state();

    if (get_rank() != 0) {
        run_simulation_process(state);
    } else {
        gather_simulation_results(state);
    }

    MPI_Finalize();
    return 0;
}