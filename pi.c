#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

// Defining constants
#define MIN_THREADS 1
#define MIN_POINTS 1

#define RADIUS 1.0

#define TRUE 1
#define FALSE 0

// Global variables
int nr_threads;
int nr_points;
int* counts; // keeps an array of counts of points inside the circle for each thread

// Function prototypes
void* generate_points(void*);
void generate_random(float*, float*);
int is_in_circle(float, float);

int main(int argc, char *argv[]) {

	if (argc < 3)
		puts("Insufficient parameters passed!\nMake sure you enter: "
				"pi <N> <K>");
	else if (argc > 3)
		puts("Too many parameters passed!\nMake sure you enter: "
				"pi <N> <K>");
	else if (atoi(argv[1]) < MIN_THREADS || atoi(argv[2]) < MIN_POINTS)
		printf(
				"%s Make sure the first argument <N> "
						"is an integer greater than %d and the second argument is greater than %d!\n",
				argv[1], MIN_THREADS,
				MIN_POINTS);
	else {

		// Parse command line arguments
		nr_threads = atoi(argv[1]);
		nr_points = atoi(argv[2]);

		// Initialize counts
		counts = malloc(nr_threads * sizeof(int));

		// Start threads
		pthread_t* th_ids = malloc(nr_threads * sizeof(pthread_t));
		int i;
		for (i = 0; i < nr_threads; i++) {
			pthread_create(&th_ids[i], NULL, generate_points,
					(void*) &counts[i]);
		}

		// Wait for threads to finish
		for (i = 0; i < nr_threads; i++) {
			pthread_join(th_ids[i], NULL);
		}

		// Calculate PI
		double pi;
		for (i = 0; i < nr_threads; i++) {
			pi += counts[i];
		}
		pi /= nr_threads * nr_points;
		pi *= 4;
		printf("PI is %f\n", pi);

		//Free memory
		free(th_ids);
		free(counts);

		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

void* generate_points(void* param) {
	// Cast the thread id
	int *global_cell = (int*) param;

	// Generate random points
	int i, count = 0;
	float rand_x, rand_y;
	// initialize random number generator
	srand(time(NULL));
	for (i = 0; i < nr_points; i++) {
		generate_random(&rand_x, &rand_y);
		if (is_in_circle(rand_x, rand_y) == TRUE)
			count++;

	}

	// Copy the count value to the global_cell
	*global_cell = count;
	return (void*) TRUE;
}

void generate_random(float* x, float* y) {

	/* Ceff_x will be 1 in case the next randomly generated
	 * number is greater than RAND_MAX / 2, and -1 otherwise.
	 * Finally x will be coeff_x times the next random number,
	 * and same thing applies for coeff_y and y.
	 */
	int coeff_x = rand() > RAND_MAX / 2 ? 1 : -1;
	int coeff_y = rand() > RAND_MAX / 2 ? 1 : -1;

	*x = (float) rand() / (float) RAND_MAX * coeff_x;
	*y = (float) rand() / (float) RAND_MAX * coeff_y;
}

int is_in_circle(float x, float y) {
	if (x * x + y * y <= RADIUS * RADIUS)
		return TRUE;
	return FALSE;
}

