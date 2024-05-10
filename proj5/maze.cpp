/* 
 * Rakesh Veetekat
 * CS 502 Project 5
 *
 * This project aims to synchronize the activity of threads, which are called rats
 * in this project, as they progress through a collection of rooms that make up a maze. 
 *
 * To compile the program, navigate to the directory of the project and run the 'make' command to
 * create the executable file 'maze'. Run the program in the following way:
 * './maze [numThreads] [algorithm_used]'
 * where numThreads refers to the number of rats and algorithm_used refers to whether
 * an in_order, distributed, or non-blocking algorithm was used to run the program.
 * 
 * The code for the distributed and non-blocking algorithms is included in the program,
 * but I could not get either of them to successfully run so I am hoping that the logic
 * behind the code will be reviewed as I could not debug the reason it would not run
 * successfully.
 * 
*/

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <vector>
#include <ctime>
#include <set>

#define MAXROOMS 8
#define MAXRATS 5

struct vbentry {
    int iRat; /* rat identifier */
    int tEntry; /* time of entry into room */
    int tDep; /* time of departure from room */
};

struct vbentry RoomVB[MAXROOMS][MAXRATS]; /* array of room visitors books */
int VisitorCount[MAXROOMS] = {0}; // to keep track of total number of visitors for each room
sem_t room_semaphores[MAXROOMS]; // semaphores for controlling access to each room
int total_traversal_time = 0; // Global total traversal time variable
int room_info[MAXROOMS][2]; // store the capacity and traversal times for each room
std::set<int> skippedRooms[MAXRATS];  // Set to keep track of skipped rooms for each rat

int num_rooms = 0;
int num_rats = 0;
int algorithm = 0;
time_t program_start_time = 0;

// Function to read room configurations from file
bool readRoomConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return false;
    }

    int capacity, delay;
    int room_index = 0;

    while (file >> capacity >> delay) {
        if (room_index >= MAXROOMS) {
            std::cerr << "Error: Too many rooms specified in the file." << std::endl;
            file.close();
            return false;
        }
        if (capacity <= 0 || delay <= 0) {
            std::cerr << "Error: Invalid room capacity or delay." << std::endl;
            file.close();
            return false;
        }
        // Store information about the rooms and initialize the room semaphores
        VisitorCount[room_index] = 0;
        sem_init(&room_semaphores[room_index], 0, capacity);
        room_info[room_index][0] = capacity;
        room_info[room_index][1] = delay;
        room_index++;
    }
    num_rooms = room_index;
    file.close();
    return true;
}

// Function to simulate rat traversal of a room
void traverseRoom(int room_id, int rat_id) {
    sleep(room_info[room_id][1]);
}

// Function to enter a room
void EnterRoom(int iRat, int iRoom) {
    sem_wait(&room_semaphores[iRoom]);
    RoomVB[iRoom][VisitorCount[iRoom]].iRat = iRat;
    RoomVB[iRoom][VisitorCount[iRoom]].tEntry = time(nullptr) - program_start_time;
    VisitorCount[iRoom]++;
}

// Function to leave a room
void LeaveRoom(int iRat, int iRoom, int tEnter) {
    RoomVB[iRoom][VisitorCount[iRoom] - 1].tDep = time(nullptr) - program_start_time;
    sem_post(&room_semaphores[iRoom]);
}

// Try to enter a room without blocking
int TryToEnterRoom(int iRat, int iRoom) {
    int value;
    sem_getvalue(&room_semaphores[iRoom], &value);
    if (value > 0) {
        EnterRoom(iRat, iRoom);
        return 0;
    }
    return -1;
}

// Rat thread function
void* ratThread(void* arg) {
    int rat_id = *((int*)arg);
    int last_tDep = 0;

    if (algorithm == 1) {   // In-order algorithm
      for (int room_id = 0; room_id < num_rooms; ++room_id) {

          // Enter room
          EnterRoom(rat_id, room_id);

          // Traverse room
          traverseRoom(room_id, rat_id);

          // Leave room
          LeaveRoom(rat_id, room_id, RoomVB[room_id][VisitorCount[room_id] - 1].tEntry);
          last_tDep = RoomVB[room_id][VisitorCount[room_id] - 1].tDep;

      }

    } else if (algorithm = 2) {   // Distributed algorithm

      int currentRoom = 0;
      int counter;
      if (rat_id > num_rooms) {
        counter = rat_id % num_rooms;
      } else {
        counter = rat_id;
      }

      // Loop through the rooms starting with the number of the current rat
      for (counter; counter < counter + num_rooms; ++counter) {

          if (counter > num_rooms - 1) {
            currentRoom = counter - num_rooms - 1;
          } else {
            currentRoom = counter;
          }

          // Enter room
          EnterRoom(rat_id, currentRoom);

          // Traverse room
          traverseRoom(currentRoom, rat_id);

          // Leave room
          LeaveRoom(rat_id, currentRoom, RoomVB[currentRoom][VisitorCount[currentRoom] - 1].tEntry);
          last_tDep = RoomVB[currentRoom][VisitorCount[currentRoom] - 1].tDep;

      }

    } else if (algorithm = 3) {   // Non-blocking algorithm
      
      /**
       * This non-blocking algorithm works by allowing threads that can't enter rooms
       * to move onto the next room that they can enter. All of the rooms that they had
       * to skip have been put into a set. The thread will then loop through that set
       * to make sure that it passes through all of the rooms that it had skipped
       * when it was making its first pass through the rooms.
      */

        for (int room_id = 0; room_id < num_rooms; ++room_id) {

          // Try to enter room
          int result = TryToEnterRoom(rat_id, room_id);
          if (result == -1) {
            // Room is full, add it to skipped rooms
            skippedRooms[rat_id].insert(room_id);
            continue;
          }

          // Traverse room
          traverseRoom(room_id, rat_id);

          // Leave room
          LeaveRoom(rat_id, room_id, RoomVB[room_id][VisitorCount[room_id] - 1].tEntry);
          last_tDep = RoomVB[room_id][VisitorCount[room_id] - 1].tDep;

      }

      // Retry entering skipped rooms
      for (int skippedRoom : skippedRooms[rat_id]) {
          // Enter room
          EnterRoom(rat_id, skippedRoom);

          // Traverse room
          traverseRoom(skippedRoom, rat_id);

          // Leave room
          LeaveRoom(rat_id, skippedRoom, RoomVB[skippedRoom][VisitorCount[skippedRoom] - 1].tEntry);
      }

    }

    // Calculate traversal time for this rat
    int traversal_time = last_tDep - RoomVB[0][rat_id].tEntry;
    total_traversal_time += traversal_time;

    std::cout << "Rat " << rat_id << " completed maze in " << traversal_time << " seconds." << std::endl;

    return nullptr;
}

int main(int argc, char* argv[]) {

    // Store the start time of the program
    program_start_time = time(nullptr);

    // Notify the user of the arguments required to run the program
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <number_of_rats> <algorithm>" << std::endl;
        return 1;
    }

    // Store arguments
    num_rats = std::stoi(argv[1]);
    std::string algorithm_ch = argv[2];

    if (num_rats > MAXRATS) {
        std::cerr << "Error: Number of rats cannot exceed MAXRATS." << std::endl;
        return 1;
    }

    if (algorithm_ch == "i") {
      algorithm = 1;
    } else if (algorithm_ch == "d") {
      algorithm = 2;
    } else if (algorithm_ch == "n") {
      algorithm = 3;
    } else {
        std::cerr << "Error: Invalid algorithm. Use 'i' for in-order, 'd' for distributed, or 'n' for non-blocking." << std::endl;
        return 1;
    }

    // Read the rooms.txt file and gather all of the information needed from there
    if (!readRoomConfig("rooms.txt")) {
        return 1;
    }

    // Declare list of rat threads
    pthread_t rats[num_rats];
    int rat_ids[num_rats];

    // Create threads for list of rat threads
    for (int i = 0; i < num_rats; ++i) {
        rat_ids[i] = i;
        pthread_create(&rats[i], nullptr, ratThread, (void*)&rat_ids[i]);
    }

    for (int i = 0; i < num_rats; ++i) {
        pthread_join(rats[i], nullptr);
    }

    // Print room information
    for (int i = 0; i < num_rooms; ++i) {
        std::cout << "Room " << i << " [" << room_info[i][0] << " " << room_info[i][1] << "]: ";
        for (int j = 0; j < VisitorCount[i]; ++j) {
            std::cout << RoomVB[i][j].iRat << " " << RoomVB[i][j].tEntry << " " << RoomVB[i][j].tDep << "; ";
        }
        std::cout << std::endl;
    }

    // Print total traversal time and ideal time
    int ideal_time = 0;
    // ideal time will be number of rats * traversal time for each room
    for (int i = 0; i < num_rooms; ++i) {
        // ideal_time += std::stoi(argv[1]) * std::stoi(argv[1 + 1 + 1 + 1 + i * 2 + 1]);
        ideal_time += num_rats * room_info[i][1];
    }
    
    std::cout << "Total traversal time: " << total_traversal_time << " seconds, compared to ideal time: " << ideal_time << " seconds." << std::endl;
    return 0;

}