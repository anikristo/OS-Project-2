/*
 ============================================================================
 Name        : t_indexgen.c
 Author      : Ani Kristo
 ============================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

// Constant definitions
#define MAX_THREADS 26
#define MIN_THREADS 1
#define MAX_LINE_SIZE 4096
#define MAX_WORD_SIZE 64

#define ASCII_a 97

#define TRUE 1
#define FALSE 0

// Structure definitions
struct lines_item {
	int line_nr;
	struct lines_item* next;
};

struct word_item {
	char word[MAX_WORD_SIZE];
	struct lines_item* line_nrs; // where the word was found in the input
	struct word_item* next;
};

// Function prototypes
void* generate_index(void*);
int read_input(char*);
int word_exists(struct word_item**, const char*, struct word_item**);
int line_exists(struct word_item*, int);
void mergesort(struct word_item**);
struct word_item* merge_sorted(struct word_item*, struct word_item*);
void split(struct word_item*, struct word_item**, struct word_item**);
void free_mem_word(struct word_item*);
void free_mem_lines(struct lines_item*);

// Global variables
int nr_threads = -1;
int word_thread_lookup[MAX_THREADS]; // MAX_THREADS = letters in the English alphabet
/*
 * This holds a list of pointer to lists of word_item structures
 * which are added while reading the input.
 *
 * The list pointer by the cell at index 0 will be processed by the first thread
 * ...
 * So on, until the last cell in this list will be processed by the last thread
 */
struct word_item** words_list;

// Execution starts here
int main(int argc, char** argv) {

	// Parse command line arguments
	if (argc < 4)
		puts("Insufficient parameters passed!\nMake sure you enter: "
				"indexgen_t <n> <infile> <outfile>");
	else if (argc > 4)
		puts("Too many parameters passed!\nMake sure you enter: "
				"indexgen_t <n> <infile> <outfile>");
	else if (atoi(argv[1]) < MIN_THREADS || atoi(argv[1]) > MAX_THREADS)
		printf("%s Make sure the first argument <n> "
				"is an integer between %d and %d!\n", argv[1], MIN_THREADS,
		MAX_THREADS);
	else {

		// Valid input was given
		nr_threads = atoi(argv[1]);

		/*
		 * Assign threads to some words according to the letter they start with
		 *
		 * At index 0 the cell will show the number of the worker for words that start with letter a
		 * At index 1 -> b
		 * ...
		 * At index 25 --> z
		 */
		int i, j, step = MAX_THREADS / nr_threads;
		for (i = 0; i < nr_threads; i++) {
			for (j = 0; j < step; j++) {
				word_thread_lookup[i * step + j] = i;
			}

		}

		// Add the last ENG_CHAR_CNT % n letter of the alphabet
		for (i = MAX_THREADS % nr_threads; i > 0; i--) {
			word_thread_lookup[MAX_THREADS - i] = nr_threads - 1;
		}

		// Initialize the words_list item
		words_list = malloc(sizeof(struct word_item*) * nr_threads);

		// Array to keep the thread IDs
		pthread_t *th_ids = malloc(sizeof(pthread_t) * nr_threads);

		// Read input
		if(read_input(argv[2])==-1){
			free(words_list);
			free(th_ids);
			exit(EXIT_FAILURE);
		}

		// Start threads
		int result;
		for (i = 0; i < nr_threads; i++) {
			result = pthread_create(&th_ids[i], NULL, generate_index,
					(void*) &words_list[i]);
			if (result) {
				printf("Return code: %d\n", result);
				exit(EXIT_FAILURE);
			}
		}

		for (i = 0; i < nr_threads; i++) {
			pthread_join(th_ids[i], NULL);
		}

		// Output the words_list in a file
		// Open file
		FILE* f = fopen(argv[3], "w");
		if (!f) {
			printf("Cannot create output file!\n");
			exit(EXIT_FAILURE);
		}

		struct word_item* w_iter;
		struct lines_item* l_iter;
		for (i = 0; i < nr_threads; i++) {
			for (w_iter = words_list[i]; w_iter != NULL; w_iter = w_iter->next) {
				fprintf(f, "%s ", w_iter->word);
				for (l_iter = w_iter->line_nrs; l_iter != NULL;
						l_iter = l_iter->next) {
					if (l_iter == w_iter->line_nrs)
						// First number
						fprintf(f, "%d", l_iter->line_nr);
					else
						fprintf(f, ", %d", l_iter->line_nr);
				}
				fprintf(f, "\n");
			}
		}

		fclose(f);

		// Free memory
		for(i = 0; i < nr_threads;i++){
			free_mem_word(words_list[i]);
		}
		free(words_list);
		free(th_ids);

		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

void* generate_index(void* param) {
	struct word_item** root = (struct word_item**) param;
	mergesort(root);
	return (void*) TRUE; // for the sake of returning something
}

int read_input(char* filename) {
	FILE* infile = fopen(filename, "r");

	// Open input file
	if (!infile) {
		perror("Cannot open input file! Make sure it exists.");
		return -1;
	} else {

		// Start reading the input
		int line_cnt = 0;
		char* cur_line = NULL;
		size_t length = 0;
		ssize_t read_size;
		while ((read_size = getline(&cur_line, &length, infile)) != -1) {

			// increment line count
			line_cnt++;

			// remove the end-of-line character
			if (length > 1 && cur_line[read_size - 1] == '\n') {
				cur_line[read_size - 1] = '\0';
				if (read_size > 1 && cur_line[read_size - 2] == '\r')
					cur_line[read_size - 2] = '\0';
			}

			// convert to lower case
			char* p;
			for (p = cur_line; *p; p++) {
				*p = tolower(*p);
			}

			// read words
			int initial_char_distance;
			struct word_item* word_node;

			char* dlmt = " ";
			char* word = strtok(cur_line, dlmt);

			while (word != NULL) {

				// Calculate the distance of the first letter of the word from the letter 'a'
				initial_char_distance = word[0] - ASCII_a;

				// Lookup the list where it should be working on
				struct word_item** respective_list =
						&words_list[word_thread_lookup[initial_char_distance]];

				// Check if the word already exists
				struct word_item* word_position = NULL;
				if (word_exists(respective_list, word, &word_position) == TRUE) {

					// Check if line number already exists
					if (line_exists(word_position, line_cnt) == FALSE) {
						// Add the line number to the line_nrs list
						struct lines_item* cur = word_position->line_nrs;
						// Special case: the line number is the smallest
						if (cur->line_nr > line_cnt) {
							// create new node
							struct lines_item* new_line_node = malloc(
									sizeof(struct lines_item));
							new_line_node->line_nr = line_cnt;
							new_line_node->next = cur;

							// assign it to the previous node
							word_position->line_nrs = new_line_node;
						} else {
							for (; cur != NULL; cur = cur->next) {
								if (cur->next == NULL
										|| cur->next->line_nr > line_cnt) {
									// insert here (keeping it sorted)

									// create new node
									struct lines_item* new_line_node = malloc(
											sizeof(struct lines_item));
									new_line_node->line_nr = line_cnt;
									new_line_node->next = cur->next;

									// assign it to the previous node
									cur->next = new_line_node;
									break;
								}
							}
						} // else if line number exists, then ignore it
					}
				} else { // Add the word node to the respective sublist of words_list
					// Initialize the word_node
					word_node = malloc(sizeof(struct word_item));
					word_node->line_nrs = malloc(sizeof(struct lines_item));
					word_node->line_nrs->line_nr = line_cnt; // copy the line number
					word_node->line_nrs->next = NULL;
					strcpy(word_node->word, word); // copy the word

					// Add this node to the respective list
					word_node->next = *respective_list;
					*respective_list = word_node;

				}

				// Read next word
				word = strtok(NULL, dlmt);
			}
		}

		free(cur_line);
	}
	fclose(infile);
	return 0;
}

int word_exists(struct word_item** root, const char* word,
		struct word_item** word_position) {
	if (root == NULL || *root == NULL || word == NULL) {
		return FALSE;
	} else {
		// check and return it in word_position
		struct word_item* cur;
		for (cur = *root; cur != NULL; cur = cur->next) {
			if (strcmp(cur->word, word) == 0) {
				*word_position = cur;
				return TRUE;
			}
		}
		return FALSE;
	}
}

int line_exists(struct word_item* word_position, int line) {
	if (word_position == NULL || word_position->line_nrs == NULL || line < 1) {
		return FALSE;
	} else {
		struct lines_item* cur;
		for (cur = word_position->line_nrs; cur != NULL; cur = cur->next)
			if (cur->line_nr == line)
				return TRUE;
		return FALSE;
	}
}
void mergesort(struct word_item** root) {
	struct word_item* head = *root;
	struct word_item* first;
	struct word_item* second;

	if ((head == NULL) || (head->next == NULL)) {
		return;
	}

	// Split head into 'first' and 'second' sublists
	split(head, &first, &second);

	// Sort the sublists
	mergesort(&first);
	mergesort(&second);

	*root = merge_sorted(first, second);
}

struct word_item* merge_sorted(struct word_item* first,
		struct word_item* second) {
	struct word_item* result = NULL;

	if (first == NULL)
		return (second);
	else if (second == NULL)
		return (first);

	if (strcmp(first->word, second->word) <= 0) {
		result = first;
		result->next = merge_sorted(first->next, second);
	} else {
		result = second;
		result->next = merge_sorted(first, second->next);
	}
	return (result);
}

void split(struct word_item* src, struct word_item** front,
		struct word_item** back) {
	struct word_item* fast;
	struct word_item* slow;
	if (src == NULL || src->next == NULL) {
		*front = src;
		*back = NULL;
	} else {
		slow = src;
		fast = src->next;

		while (fast != NULL) {
			fast = fast->next;
			if (fast != NULL) {
				slow = slow->next;
				fast = fast->next;
			}
		}

		*front = src;
		*back = slow->next;
		slow->next = NULL;
	}
}
void free_mem_word(struct word_item* root){

	struct word_item* to_be_deleted = NULL;
	while(root){
		to_be_deleted = root;
		root = root->next;
		free_mem_lines(to_be_deleted->line_nrs);
		to_be_deleted->line_nrs = NULL;
		to_be_deleted->next = NULL;
		free(to_be_deleted);
	}
}

void free_mem_lines(struct lines_item* root){

	struct lines_item* to_be_deleted = NULL;
	while(root){
		to_be_deleted = root;
		root = root->next;
		to_be_deleted->next = NULL;
		free(to_be_deleted);
	}
}

