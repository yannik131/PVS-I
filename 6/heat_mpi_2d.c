#include <stdlib.h>
#include <stdio.h>

void swap(double** a, double** b)
{
	double* temp = *b;
	*b = *a;
	*a = temp;
}


int main(int argc, char** args)
{
    double delta_t = 0.02;
    int grid_size_x = 1024;
    int grid_size_y = 1024;   
    
    int num_time_steps = 3000;
    double conductivity = 0.1;
  
    double* T_k  = (double*) malloc(sizeof(double) * grid_size_x * grid_size_y);
    double* T_kn = (double*) malloc(sizeof(double) * grid_size_x * grid_size_y);

    for (int y = 0; y < grid_size_y; y++)
        for (int x = 0; x < grid_size_x; x++)
            T_k[grid_size_x * y + x] = x + y ;

    int percentile_steps = 10;

    for (int k = 0; k < num_time_steps; k++)
    {
        for (int y = 0; y < grid_size_y; y++)
            for (int x = 0; x < grid_size_x; x++)
            {
                int i = grid_size_x * y + x;
                
                int i_left   =  x !=      0          ? i - 1 : i + 1;
                int i_right  =  x != grid_size_x - 1 ? i + 1 : i - 1;
                int i_down   =  y !=      0          ? i - grid_size_x : i + grid_size_x;
                int i_up     =  y != grid_size_y - 1 ? i + grid_size_x : i - grid_size_x;
                
                double dTdt_i = conductivity * (- 4 * T_k[i] + 
                    T_k[i_left] + T_k[i_right]+T_k[i_down] + T_k[i_up]);
                T_kn[i] = T_k[i] + delta_t * dTdt_i;
            }
        swap(&T_k, &T_kn);
        if(k % (int)(num_time_steps / percentile_steps) == 0) {
            printf("%d%%\n", (int)((float)k / num_time_steps * 100.0));
        }
    }
    
    double T_average = 0;
    for (int y = 0; y < grid_size_y; y++)
        for (int x = 0; x < grid_size_x; x++)
            T_average += T_k[grid_size_x * y + x];
	   
    T_average = T_average / (grid_size_x * grid_size_y);

    printf("T_average: %f \n", T_average);

    free(T_k);
    free(T_kn);

    return 0;
}