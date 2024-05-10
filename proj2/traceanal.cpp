/* 
 * Rakesh Veetekat
 * CS 502 Project 2
 *
 * This program has been implemented to analyze the output of the strace command
 * in the Linux Operating System. 
 *
 * To compile the program, navigate to the directory of the project and run the 'make' command to
 * create the executable file 'traceanal'. The 'traceanal' file can be run with the following commands:
 * "./traceanal < ls.slog"
 * "./traceanal < ls.slog | sort"
 * "./traceanal < ls.slog | sort -nrk 2"
 * "./traceanal seq < ls.slog"
 * "./traceanal seq < ls.slog | sort"
 * 
 * The ls.slog needs to be in the project directory before 'traceanal' can be run. I've created
 * a program called 'simpleProgram' which I used to observe program execution without the use
 * of system calls and the log file I created from running strace on that program is in simpleProgram.slog.
*/

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string.h>
using namespace std;

int main(int argc, char *argv[]) {

    unordered_map<string, int> syscallCounts;
    int totalSyscalls = 0;
    bool systemCallPairs = false;
    string line;

    // Check to see if the 'seq' argument has been included
    if (argc > 1) {
        if (strcmp(argv[1],"seq") == 0) {
            systemCallPairs = true;
        }
    }

    if (systemCallPairs) {
        // Print system call pairs if the 'seq' argument was used
        unordered_map<string, int> syscallSubsequentCounts;
        string previousSyscall = "tempString";
        while (getline(cin, line)) {
            if (line.find("(") != string::npos) {
                // Extract the system call name
                size_t start = 0;
                size_t end = line.find("(");
                string syscall = line.substr(start, end);
                totalSyscalls++;
                syscallCounts[syscall]++;
                
                // Now, pair the  previous system call with the current system call and add to hashmap
                if (strcmp(previousSyscall.c_str(), "tempString") != 0) {
                    // We can add this pair to the hashmap
                    string syscallPair = previousSyscall + ":" + syscall;
                    syscallSubsequentCounts[syscallPair]++;
                }
                previousSyscall = syscall;
            }
        }
    
        // Count the unique system calls
        int uniqueSyscalls = syscallCounts.size();
        cout << "AAA: " << totalSyscalls << " invoked system call instances from " << uniqueSyscalls << " unique system calls" << endl;

        // Print all of the unique system calls, the number of times they were used, and the system call pairs
        for (auto n = syscallCounts.begin(); n != syscallCounts.end(); n++) {
            string firstSyscall = n->first;
            int firstCount = n->second;
            cout << firstSyscall << " " << firstCount << endl;
            for (auto i = syscallSubsequentCounts.begin(); i != syscallSubsequentCounts.end(); i++) {
                string secondSyscall = i->first;
                int secondCount = i->second;
                // Check if the first portion of the syscall pair matches the overarching system call
                if (strcmp(secondSyscall.substr(0,firstSyscall.size()).c_str(), firstSyscall.c_str()) == 0) {
                    cout << "   " << secondSyscall << " " << secondCount << endl;
                }
            }
        }

    } else {
        // Analyze slog file normally without printing system call pairs
        while (getline(cin, line)) {
            if (line.find("(") != string::npos) {
                // Extract the system call name
                size_t start = 0;
                size_t end = line.find("(");
                string syscall = line.substr(start, end);
                totalSyscalls++;
                syscallCounts[syscall]++;
            }
        }

        // Count the unique system calls
        int uniqueSyscalls = syscallCounts.size();
        cout << "AAA: " << totalSyscalls << " invoked system call instances from " << uniqueSyscalls << " unique system calls" << endl;

        // Print all of unique system calls and the number of times they were used
        for (auto i = syscallCounts.begin(); i != syscallCounts.end(); i++) {
            cout << i->first << " " << i->second << endl;
        }

    }

    return 0;

}