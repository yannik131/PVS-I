#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

void swap(float** a, float** b) {
    float* temp = *a;
    *a = *b;
    *b = temp;
}

float compute_contribution // Cheap to compute
    (float s_1, float s_2)
{
    return (s_1 + 1) / (s_2 + 2); //Dummy implementation for testing
}

/**
 * Method: 
 * - Every process has the initial S_k
 * - Each process calculates entries start to start + n of S_kn
 * - The different parts of S_kn are then distributed
 * - Each process puts the parts together to get S_kn in full
 * - Repeat step 2
*/

int main(int argc, char **args)
{
    MPI_Init(&argc, &args);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    
    int sim_size;
    if(rank == 0) {
        printf("Enter simulation size:\n");
        scanf("%i", &sim_size);
    }
    MPI_Bcast(&sim_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    //Calculate displacements and receive counts for each process
    //If sim_size / worldSize = nInaccurate R remaining, the first
    //remaining processes get 1 extra value to work with
    int nInaccurate = sim_size / worldSize;
    int remaining = sim_size - nInaccurate * nInaccurate;
    
    int* displs = (int*)malloc(worldSize * sizeof(int));
    int* revcounts = (int*)malloc(worldSize * sizeof(int));
    
    int displSum = 0;
    
    for(int i = 0; i < worldSize; ++i) {
        displs[i] = displSum;
        int count = nInaccurate;
        if(remaining > 0) {
            ++count;
            --remaining;
        }
        displSum += count;
        revcounts[i] = count;
    }
    
    int num_time_steps = 3;
    float *S_k = malloc(sizeof(float) * sim_size);
    float *S_kn = malloc(sizeof(float) * sim_size);
    for (int i = 0; i < sim_size; i++)
        S_k[i] = i;
        
    for (int k = 0; k < num_time_steps; k++)
    {
        for (int i = displs[rank]; i < displs[rank] + revcounts[rank]; i++)
        {
            double dSdt_i = 0;
            for (int j = 0; j < sim_size; j++)
                dSdt_i += compute_contribution(S_k[i], S_k[j]);
            S_kn[i] = S_k[i] + dSdt_i;
        }
        MPI_Allgatherv(S_kn + displs[rank], revcounts[rank], MPI_FLOAT, S_kn, revcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        swap(&S_k, &S_kn);
    }
    float S_max = __FLT_MIN__;
    for (int i = 0; i < sim_size; i++)
        S_max = fmaxf(S_max, S_k[i]);
    printf("S_max: %f\n", S_max);
    free(S_k);
    free(S_kn);
    return 0;
}