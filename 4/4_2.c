#include <mpi.h>
#include <stdio.h>
#include <assert.h>
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
 * @brief Sends and receives the temperatures on the left and right end of a block to the neighboring left and right processes to synchronize the temperatures at the edges
 * @param T_k The current temperatures with block_size entries
 * @param block_size Size of T_k
 * @param r Rank of this process
 * @param np Number of processes
*/
void synchronize_borders(double* T_k, int grid_size, int block_size, int r, int np) {
    int left_neighbor_rank = (r - 1) % np;
    if(left_neighbor_rank < 0) {
        left_neighbor_rank += np;
    }
    int right_neighbor_rank = (r + 1) % np;
    
    int index_left_send = r * block_size;
    int index_right_send = (r + 1) * block_size - 1;
    
    int index_left_receive = index_left_send - 1;
    if(index_left_receive < 0) {
        index_left_receive = grid_size - 1;
    }
    int index_right_receive = index_right_send + 1;
    if(index_right_receive == grid_size) {
        index_right_receive = 0;
    }
    
    for(int i = index_left_send; i < index_right_send; ++i) {
        //printf("%i (before): T_k[%i] = %f\n", r, i, T_k[i]);
    }
    
    //printf("%i -> %i: %f (left)\n", r, left_neighbor_rank, *(T_k + index_left_send));
    MPI_Send(T_k + index_left_send, 1, MPI_DOUBLE, left_neighbor_rank, 0, MPI_COMM_WORLD);
    MPI_Recv(T_k + index_right_receive, 1, MPI_DOUBLE, left_neighbor_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("%i: Received %f (left)\n", r, T_k[index_left_receive]);
    
    //printf("%i -> %i: %f (right)\n", r, right_neighbor_rank, *(T_k + index_right_send));
    MPI_Send(T_k + index_right_send, 1, MPI_DOUBLE, right_neighbor_rank, 0, MPI_COMM_WORLD);
    MPI_Recv(T_k + index_left_receive, 1, MPI_DOUBLE, right_neighbor_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("%i: Received %f (right)\n", r, T_k[index_right_receive]);
    
    for(int i = index_left_send; i < index_right_send; ++i) {
        //printf("%i (after): T_k[%i] = %f\n", r, i, T_k[i]);
    }
}

/**
 * @brief Lets all processes send the average temperature to the master process. The master process then averages the sum and prints it
 * @param T_average The calculated average temperature for this process
 * @param r The rank of this process
 * @param np Number of processes
*/
void gather_and_print_averages(double T_average, int r, int np) {
    if(r != 0) {
        //T_average for r == 0 doesn't need to be sent, it's already passed as an argument
        MPI_Send(&T_average, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
    
    if(r == 0) {
        double T_received;
        printf("Master avg: %f\n", T_average);
        for(int i = 1; i < np; ++i) {
            MPI_Recv(&T_received, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
 * @note The given implementation incorrect, it assigned local variables in the function body
*/
void swap(double** a, double** b) {
    double* temp = *a;
    *a = *b;
    *b = temp;
}

void print_array(double* arr, int size) {
    for(int i = 0; i < size; ++i) {
        printf("%f ", arr[i]);
    }
    printf("\n");
}

int main() {
    MPI_Init(NULL, NULL); //MPI_INIT(&argc, &argv) if we want to use the arguments (which we don't)
    int num_procs = get_number_of_processes();
    int r = get_rank();
    
    double delta_t = 0.02;
    int grid_size = 4;
    int num_time_steps = 3;
    double conductivity = 0.1;
    
    assert(grid_size % num_procs == 0); //Abort if the grid can not evenly be distributed among the processes
    int block_size = grid_size / num_procs;
    
    double* T_k = (double*) malloc(sizeof(double) * grid_size);
    double* T_kn = (double*) malloc(sizeof(double) * grid_size);
    
    int block_begin = r * block_size;
    int block_end = block_size * (r + 1); //Removed - 1 at the end, alternatively iterate to <= block_end
    //printf("%i: block_begin = %i, block_end = %i\n", r, block_begin, block_end);
    
    //The original code assigned temperatures from 0K (impossible) to about 500 million K (temperature in the sun's core: ~15.6 million K)
    int max_T = 1000; 
    int min_T = 273; 
    double a = (max_T - min_T) / (grid_size - 1);
    double b = min_T;
    
    //Assign temperatures from 273K (room temperature) to 1000K (red glowing iron)
    for(int i = block_begin; i < block_end; ++i) {
        T_k[i] = i*a + b;
        //printf("%i (before): T_k[%i] = %f\n", r, i, T_k[i]);
    }
    
    for(int k = 0; k < num_time_steps; ++k) {
        printf("Prozess %i (vorher):\n", r);
        print_array(T_k, grid_size);
        synchronize_borders(T_k, grid_size, block_size, r, num_procs);
        printf("Prozess %i (nachher):\n", r);
        print_array(T_k, grid_size);
        return 0;
        for(int i = block_begin; i < block_end; ++i) {
            int i_left = (i != 0)? i-1 : 1;
            int i_right = (i != grid_size-1)? i+1 : i-1;
            double dTdt_i = conductivity * (-2*T_k[i] + T_k[i_left] + T_k[i_right]);
            T_kn[i] = T_k[i] + delta_t * dTdt_i;
        }
        for(int i = block_begin; i < block_end; ++i) {
            //printf("%i. %i (during): T_k[%i] = %f\n", k, r, i, T_kn[i]);
        }
        
        swap(&T_k, &T_kn);
    }
    
    for(int i = block_begin; i < block_end; ++i) {
        //printf("%i (after): T_k[%i] = %f\n", r, i, T_k[i]);
    }
    
    double T_average = 0;
    for(int i = block_begin; i < block_end; ++i) {
        T_average += T_k[i];
    }
    T_average = T_average / block_size;
    
    gather_and_print_averages(T_average, r, num_procs);
    
    MPI_Finalize();
}