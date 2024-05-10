/* 
 * Rakesh Veetekat
 * CS 502 Project 6
 *
 * This project aims to compare the performance of standard file I/O that uses the read()
 * system call with memory-mapped I/O that uses the mmap() system call. 
 *
 * To compile the program, navigate to the directory of the project and run the 'make' command to
 * create the executable file 'proj6'. To run, use:
 * "./proj6 srcfile size mmap pn"
 * Where srcfile is the input file, size is the chunk size, and pn denotes the
 * number of threads for the program to use. p tells the program to use threads, while
 * n signals the number of threads to use.
 * 
 * I've tested this project with 3 input files, which are the test.txt, moby.txt, and
 * proj6.c files. I've also included the doit program from project 1 in order to create
 * the performance analysis.
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>

#define MAX_THREADS 16
#define MAX_CHUNK_SIZE 8192

// Struct to hold data for each thread
struct ThreadData {
    char *start;         // Starting address of the chunk of memory to process
    size_t size;         // Size of the chunk of memory
    int num_strings;     // Number of strings found in the chunk
    size_t max_length;   // Length of the longest string found in the chunk
};

// Function used by each thread to process a chunk of memory
void *process_chunk(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
    char *ptr = data->start;
    char *end = ptr + data->size;
    int num_strings = 0;
    size_t max_length = 0;
    
    // Loop through the chunk of memory
    while (ptr < end) {
        size_t length = 0;
	   // Find printable strings within the chunk
        while (ptr < end && isprint(*ptr)) {
            length++;
            ptr++;
        }
	   // If a string of length at least 4 is found, update counters
        if (length >= 4) {
            num_strings++;
            if (length > max_length)
                max_length = length;
        }
	   // Skip non-printable characters
        while (ptr < end && !isprint(*ptr)) {
            ptr++;
        }
    }
    // Update thread's data with the results
    data->num_strings = num_strings;
    data->max_length = max_length;
    return NULL;
}

int main(int argc, char *argv[]) {
	// Check command line arguments
    	if (argc < 2 || argc > 5) {
		fprintf(stderr, "Usage: %s <filename> [size|mmap] [p<num_threads>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    
	char *filename = argv[1];
	int use_mmap = 0;
	int num_threads = 1;
	int chunk_size = 1024;
	
	// Parse arguments
	if (argc >= 3) {	// Chunk size argument
		int chunk_size = atoi(argv[2]);
		if (chunk_size <= 0 || chunk_size > MAX_CHUNK_SIZE) {
			fprintf(stderr, "Invalid chunk size. Using default chunk size (1024).\n");
		}
	}

	if (argc >= 4) {	// mmap argument
		if (strcmp(argv[3], "mmap") == 0) {
			use_mmap = 1;
		}
	}
	
	if (argc == 5) {	// number of threads argument
		if (argv[4][0] == 'p' && isdigit(argv[4][1])) {
			num_threads = atoi(&argv[4][1]);
			// printf("Number of threads: %d threads.\n", num_threads);
			if (num_threads <= 0 || num_threads > MAX_THREADS) {
				fprintf(stderr, "Invalid number of threads. Using default (1 thread).\n");
				num_threads = 1;
			}
		} else {
			fprintf(stderr, "Invalid command line argument for number of threads. Using default (1 thread).\n");
		}
	}
	
	// Open the file
	int fd;
	struct stat file_stat;
	if ((fd = open(filename, O_RDONLY)) < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	
	// Get file size
	if (fstat(fd, &file_stat) < 0) {
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	
	size_t file_size = file_stat.st_size;
	int num_chunks = (file_size + chunk_size - 1) / chunk_size;
	size_t remaining_bytes = file_size;
	pthread_t threads[MAX_THREADS];
	struct ThreadData thread_data[MAX_THREADS];
	
	// Read the file using mmap or standard file I/O based on user's input
	if (use_mmap) {
		char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (file_data == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}
		
		size_t chunk_start = 0;

		// Divide the file into chunks for parallel processing
		for (int i = 0; i < num_threads; i++) {
			size_t chunk_end = chunk_start + (remaining_bytes + num_threads - i - 1) / (num_threads - i);
			size_t chunk_size = chunk_end - chunk_start;
			
			thread_data[i].start = &file_data[chunk_start];
			thread_data[i].size = chunk_size;
			
			// Create a thread to process each chunk
			if (pthread_create(&threads[i], NULL, process_chunk, (void *)&thread_data[i]) != 0) {
				perror("pthread_create");
				exit(EXIT_FAILURE);
			}
			
			chunk_start = chunk_end;
			remaining_bytes -= chunk_size;
		}
		
		// Wait for all threads to finish
		for (int i = 0; i < num_threads; i++) {
			if (pthread_join(threads[i], NULL) != 0) {
				perror("pthread_join");
				exit(EXIT_FAILURE);
			}
		}

		// Unmap the memory
		munmap(file_data, file_size);

	} else {
		// Allocate buffer for standard file I/O
		char *buffer = malloc(chunk_size);
		if (!buffer) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		
		// Read the file in chunks
		for (int i = 0; i < num_chunks; i++) {
			size_t bytes_to_read = (i == num_chunks - 1) ? remaining_bytes : chunk_size;
			ssize_t bytes_read = read(fd, buffer, bytes_to_read);
			if (bytes_read < 0) {
				perror("read");
				exit(EXIT_FAILURE);
			}
			
			thread_data[0].start = buffer;
			thread_data[0].size = bytes_read;
			
			// Process the chunk
			process_chunk((void *)&thread_data[0]);
			
			remaining_bytes -= bytes_read;
		}
		
		// Free the buffer
		free(buffer);

	}
	
	// Aggregate results from all threads
	int total_strings = 0;
	size_t max_length = 0;
	for (int i = 0; i < num_threads; i++) {
		total_strings += thread_data[i].num_strings;
		if (thread_data[i].max_length > max_length)
			max_length = thread_data[i].max_length;
	}
	
	// Print results
	printf("File size: %lu bytes.\n", file_size);
	printf("Strings of at least length 4: %d\n", total_strings);
	printf("Maximum string length: %zu bytes\n", max_length);
	
	// Close the file
	close(fd);
	
	return 0;
}