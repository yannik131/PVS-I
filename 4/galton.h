// The galton board implementation and the MPI code were separated to avoid
// confusion
#ifndef GALTON_H_INCLUDED
#define GALTON_H_INCLUDED

typedef struct SimulationState {
    int number_of_balls;
    int number_of_compartments;
    int number_of_rows; // This is always number_of_compartments - 1
} SimulationState;

void display_histogram(int *histogram, SimulationState state);
int *run_simulation(SimulationState state);
SimulationState read_simulation_state();

#endif