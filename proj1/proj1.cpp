/* 
 * Rakesh Veetekat
 * CS 502 Project 1 
 *
 * This program has been implemented to behave like a command shell that reads in user commands
 * and executed those commands in a child process that is forked off.
 *
 * To compile the program, navigate to the directory of the project and run the 'make' command to
 * create the executable file 'doit'. The 'doit' file can either be run with a command or
 * with no arguments, and it should work similarily to a shell program if no arguments are given.
 * Also, statistics are listed after any command is executed through the program.
 * 
 * I've included a sample text file to use for the word count command called foo.txt. It has been
 * utilized in the testcases.txt file where I used the script command to show different test cases I
 * used to test the doit executable.
 *
*/

#include <iostream>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <vector>
using namespace std;

// Process struct
struct process {
	int pid;
	long startTime;
	string cmd;
};
vector<struct process> children;	// use vector to store children stemming from original shell
char *prompt = (char *)malloc(32 * sizeof(char));	// prompt that will be shown to the user when the program is run
int ampsand = 0;	// will be used if there's a background process

// Print the required statistics after running the current command
void printStatistics(long start) {
	// Calculate the times required for the statistics
	struct rusage usage;
	struct timeval userT, systemT, wallclockT;
	gettimeofday(&wallclockT, NULL);
	long end = (wallclockT.tv_sec * 1000) + (wallclockT.tv_usec / 1000);
	if(getrusage(RUSAGE_CHILDREN, &usage) != 0) {
		cerr << "Error occurred while getting usage\n";
	}
	userT = usage.ru_utime;
	systemT = usage.ru_stime;
	double userTime = ((userT.tv_sec * 1000) + (userT.tv_usec / 1000));
	double systemTime = ((systemT.tv_sec * 1000) + (systemT.tv_usec / 1000));
	double wallClockTime = end - start;

	// Print out all of the statistics required at this moment
	cout << "\n";
	cout << "-Statistics-\n";
	cout << "*** System time = " << systemTime << " milliseconds\n";
	cout << "*** User time = " << userTime << " milliseconds\n";
	cout << "*** Wall clock time = " << wallClockTime << " milliseconds\n";
	cout << "*** Involuntary context switches = " << usage.ru_nivcsw << "\n";
	cout << "*** Voluntary context switches = " << usage.ru_nvcsw << "\n";
	cout << "*** Page faults requiring I/O = " << usage.ru_majflt << "\n";
	cout << "*** Page faults serviced without I/O = " << usage.ru_minflt << "\n";
	cout << "*** Maximum resident set size = " << usage.ru_maxrss << " kilobytes\n";
}

// This function will fork off a child process in order to run the command that the user has input
int execute(char ** args) {
	long start;
	int pid;
	struct timeval wallclockT;
	gettimeofday(&wallclockT, NULL);
	start = ((wallclockT.tv_sec * 1000) + (wallclockT.tv_usec / 1000));

	// fork off a child process
	pid = fork();
		
	// determine if this process is the child or parent
	if (pid < 0) {
		cerr << "Error while forking\n";
		exit(1);
	} else if (pid == 0) {
		// child
		execvp(args[0], args);
		cerr << "Error while using execvp\n";
		exit(1);
	} else {
		// parent, wait for child
		// Ampersand not used, this is not a background process
		if (!ampsand) {
			wait(0);
			printStatistics(start);
			return 0;
		} else {
			// Ampersand used, this is the background process
			process child = {pid, start, args[0]};
			// Add new child to children vector
			// The vector will be used to check if background processes have finished by the next user input
			children.push_back(child);
			cout << "[" << children.size() << "] " << children.back().pid << "\n";
			return 0;
		}
	}
}

// This function will make sure that the shell will only exit once all child processes have finished running
void checkBackgroundProcesses() {
	if (children.size() > 0) {	// wait for children for finish
		if (children.size() == 1) {
			cout << "Cannot exit. There is " << children.size() << " background process that needs to finish" << "\n";
		} else {
			cout << "Cannot exit. There are " << children.size() << " background processes that need to finish" << "\n";
		}
		// Loop through children to see which background processes have finished
		for (long n = 0; n < children.size(); n++) {
			int *statusPtr;
			pid_t result = waitpid(children.at(n).pid, statusPtr, 0);
			if (result > 0) {	// child finished
				cout << "[" << n + 1 << "] " << children.at(n).pid << " Completed\n";
				printStatistics(children.at(n).startTime);
			}
		}
	}
	exit(0);
}

int main(int argc, char *argv[]) {
	int i;
	char **args = (char **)malloc(32 * sizeof(char *));	// Can only have up to 32 distinct arguments
	
	// Initalize the prompt that appears when the user runs the program
	prompt[0] = '=';
	prompt[1] = '=';
	prompt[2] = '>';
	prompt[3] = '\0';

	// Read and execute the command line argument provided by the user
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			args[i - 1] = argv[i];
		}
		args[argc - 1] = NULL;
		execute(args);
	} else {
		// No command line argument presented by the user
		// The program should start to behave like a shell now
		// Continuosly loop through the user's input and only exit when given the 'exit' command by the user
		while (1) {

			// Print the default prompt for the shell
			cout << prompt << " ";
			char line[128];
			char *token;
			int currentToken = 0;
			// Read the input provided by the user
			cin.getline(line, 128);

			// Check to see if any background processes finished after each user command
			for (long n = 0; n < children.size(); n++) {
				int *statusPtr;
				pid_t result = waitpid(children.at(n).pid, statusPtr, WNOHANG);
				if (result > 0) {
					// Child process exited, can let the user know
					cout << "[" << n + 1 << "] " << children.at(n).pid << " Completed\n";
					printStatistics(children.at(n).startTime);
					children.erase(children.begin() + n);
				}
			}

			// Split the input line into tokens to parse through the arguments
			token = strtok(line, " ");
			while (token != NULL) {
				args[currentToken] = token;
				token = strtok(NULL, " ");	
				currentToken++;
			}

			// Check if an ampersand is in the input
			if (strcmp(args[currentToken - 1], "&") == 0) {
				args[currentToken - 1] = NULL;
				ampsand = 1;
			} else {
				args[currentToken] = NULL;
				ampsand = 0;
			}

			// Check to see if the user is using the 'exit', 'cd', 'set prompt', or 'jobs' command
			if (strcmp(args[0], "exit") == 0) {
				// Check to see if there are background processes before program exits
				checkBackgroundProcesses();
			} else if (strcmp(args[0], "cd") == 0 && args[1] != NULL)  {
				if (chdir(args[1]) != 0) {
					// No directory given with cd command
					cerr << "Error while using cd\n";
				}
			} else if (strcmp(args[0], "set") == 0 && strcmp(args[1], "prompt") == 0 && strcmp(args[2], "=") == 0 && args[3] != NULL) {
				// Change default prompt to prompt given by user
				strcpy(prompt, args[3]);
			} else if (strcmp(args[0], "jobs") == 0) {
				if (children.size() == 0) {
					// Inform user that there are no background processes
					cout << "No jobs are running in the background\n";
				} else {
					// Print out all of the background processes
					for (long n = 0; n < children.size(); n++) {
						cout << "[" << n + 1 << "] " << children[n].pid << " " << children[n].cmd << "\n";
					}
				}
			} else {
				// No processing is required for other commands read by the shell
				execute(args);
			}
		}
	}
}