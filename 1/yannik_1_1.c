#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

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

void init_array_with(int* array, size_t size, int value) {
    for(size_t i = 0; i < size; ++i) {
        array[i] = value;
    }
}

void print_2d_array(int** array, int size) {
    for(int i = 0; i < size; ++i) {
        int* arr = array[i];
        for(int j = 0; j < i + 2; ++j) {
            printf("%d ", arr[j]);
        }
        printf("\n");
    }
    printf("\n");
}


void count_and_clear_last_row(int** game_field, int* histogram, int number_of_compartments) {
    int* last_row = game_field[number_of_compartments - 1];
    for(int i = 0; i < number_of_compartments; ++i) {
        if(last_row[i] != -1) {
            ++histogram[i];
            last_row[i] = -1;
            return;
        }
    }
}

/*
Starts with the last row and works itself up.
The last row is already assumed to be empty.
*/
void let_balls_fall_1_row(int** game_field, int number_of_compartments) {
    for(int i = number_of_compartments - 1; i > 0; --i) {
        int* row_above = game_field[i - 1];
        int size_of_row_above = i + 1;

        // Get the index and ball number of the ball in the row above
        int ball_number;
        int index_of_ball_above = -1;
        for(int j = 0; j < size_of_row_above; ++j) {
            if(row_above[j] != -1) {
                ball_number = row_above[j];
                index_of_ball_above = j;
                break;
            }
        }
        if(index_of_ball_above == -1) {
            //No ball in the above row, continue
            continue;
        }

        //Determine the new index of the ball in the current row
        int new_index = index_of_ball_above;
        if(ball_goes_right()) {
            new_index = index_of_ball_above + 1;
        }

        //Assign the ball to the new index and remove it from the old
        int* current_row = game_field[i];
        current_row[new_index] = ball_number;
        row_above[index_of_ball_above] = -1;
    }
}


void insert_ball_at_top(int** game_field, int number_of_balls) {
    if(number_of_balls <= 0) {
        return;
    }
    int index = ball_goes_right()? 1 : 0;
    game_field[0][index] = number_of_balls;
}

/*
The histogram well be displayed in a quadratic grid.
So it needs to be normalized to the number of compartments.
*/
void display_histogram(int* histogram, int number_of_compartments) {
    int max = -1;
    for(int i = 0; i < number_of_compartments; ++i) {
        if(histogram[i] > max) {
            max = histogram[i];
        }
    }
    for(int i = number_of_compartments; i > -1; --i) {
        for(int j = 0; j < number_of_compartments; ++j) {
            if(histogram[j] > i * max / number_of_compartments) {
                printf("X ");
            }
            else {
                if(i == 0) {
                    printf("- ");
                }
                else {
                    printf("  ");
                }
            }
        }
        printf("\n");
    }
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
    printf("This should be lower or equal the number of columns of your terminal to display the histogram correctly.\n");
    while(number_of_compartments <= 1) {
        printf("The number of compartments has to be greater than 1.\n");
        number_of_compartments = safely_read_integer();
    }

    printf("\nRunning simulation with: \n"
           "Number of balls: %d\n"
           "Number of compartments: %d\n\n", 
           number_of_balls, number_of_compartments);

    printf("Print board once after insertion of the top ball and one iteration?\n1: yes, other: no\n");
    bool print_board = safely_read_integer() == 1;
    if(print_board) {
        printf("Printing the board. -1 denotes an empty field. Other numbers represent the balls.\n\n");
    }

    /*Simulation concept:

    Let's say there are 2 balls and 3 compartments. 
    Every ball will have a unique number starting with 0.
    Every row of the board corresponds to an n+2 array, where n
    is the row number (starting with zero). It will contain a ball
    number at the ball's current position and -1 everywhere else.
    Every iteration, a ball is put in the first row, if any are left.
    Every following ball will go down 1 row.
    After two iterations, the arrays may look like this:
    0 2
    0 0 1

    */

    int** game_field = (int**)malloc(number_of_compartments * sizeof(int*));
    for(size_t i = 0; i < number_of_compartments; ++i) {
        size_t size = i + 2;
        game_field[i] = (int*)malloc(size * sizeof(int));
        init_array_with(game_field[i], size, -1);
    }

    int* histogram = (int*)malloc(number_of_compartments * sizeof(int));
    init_array_with(histogram, number_of_compartments, 0);

    int number_of_iterations = number_of_balls + number_of_compartments;
    for(int i = 0; i < number_of_iterations; ++i) {
        insert_ball_at_top(game_field, number_of_balls);
        let_balls_fall_1_row(game_field, number_of_compartments);
        if(print_board) {
            print_2d_array(game_field, number_of_compartments);
        }
        count_and_clear_last_row(game_field, histogram, number_of_compartments);
        --number_of_balls;
    }

    display_histogram(histogram, number_of_compartments);

    printf("\nUnderlying histogram:\n");
    for(int i = 0; i < number_of_compartments; ++i) {
        printf("%d ", histogram[i]);
    }
    printf("\n");

    return 0;
}
