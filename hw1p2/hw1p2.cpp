// hw1p2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "ThreadSafeSet.h"

#include <queue>
#include <thread>


int main(int argc, char** argv)
{
    // Parse Command Line arguments
    if (argc < 3) {
        std::cout << "Not enough arguments" << std::endl;
        return EXIT_FAILURE;
    }
    if (argc > 3) {
        std::cout << "Too many arguments" << std::endl;
        return EXIT_FAILURE;
    }

    char* thread_str = argv[1];
    char* end_ptr;
    long int thread_long = strtol(thread_str, &end_ptr, 10);
    if (thread_str == end_ptr) {
        // number could not be parsed
        std::cout << "Invalid thread count" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (errno == ERANGE || thread_long != 1) {
        // number is too big
        std::cout << "Invalid thread count" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (*end_ptr != '\0') {
        std::cout << "Invalid thread count" << std::endl;
        exit(EXIT_FAILURE);
    }
    int thread_count = static_cast<int>(thread_long);
    std::string url_file = argv[2];

    // Read file and create a queue of urls to request
    std::queue<std::string> url_queue = parseUrlFile(url_file);

    // Create an atomic hash set of previous hosts and ip
    ThreadSafeSet known_hosts;
    ThreadSafeSet known_ips;

    // Initialize threads with a url, 
    // when thread finished requesting lock section std::cout, then get from queue
    // inside function
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
