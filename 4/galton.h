// Damit der MPI-Code nicht mit der Implementierung des Galton-Bretts vermischt
// wird, wurde beides auf verschiedene Dateien aufgeteilt
#ifndef GALTON_H_INCLUDED
#define GALTON_H_INCLUDED

typedef struct SimulationState {
    int number_of_balls;
    int number_of_compartments;
    int number_of_rows;
} SimulationState;

void display_histogram(int *histogram, SimulationState state);
int *run_simulation(SimulationState state);
SimulationState read_simulation_state();

#endif