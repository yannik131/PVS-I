#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

bool ball_goes_right() {
    return rand() % 2 == 0;
}

int safely_read_integer() {
    int integer;
    char* input = NULL;
    size_t input_length = 0;
    ssize_t getline_return = -1;
    ssize_t sscanf_return;
    char character_after_number;

    while(getline_return == -1 || sscanf_return != 2 || character_after_number != '\n') {
        if(input != NULL) {
            free(input);
            input = NULL;
            clearerr(stdin);
            printf("Input error. Try again.\n");
        }
        printf(">> ");
        getline_return = getline(&input, &input_length, stdin);
        sscanf_return = sscanf(input, "%d%c", &integer, &character_after_number);
    }

    return integer;
}

int main() {
    int number_of_balls = 0;
    int number_of_compartments = 0;

    printf("A Galton Board simulation.\n"
           "First, type in the number of balls.\n");

    while(number_of_balls <= 0) {
        printf("The number of balls needs to be positive.\n");
        number_of_balls = safely_read_integer();
    }

    printf("Now, type in the number of compartments.\n");
    while(number_of_compartments <= 1) {
        printf("The number of compartments has to be greater than 1.\n");
        number_of_compartments = safely_read_integer();
    }

    printf("Running simulation with: \n"
           "Number of balls: %d\n"
           "Number of compartments: %d\n", 
           number_of_balls, number_of_compartments);

    /*Simulation concept:

    Let's say there a 2 balls and 3 compartments.*/


    return 0;
}
