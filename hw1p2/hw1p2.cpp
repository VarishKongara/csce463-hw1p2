// hw1p2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#pragma comment(lib, "ws2_32.lib")

#include "ThreadSafeSet.h"

#include <queue>
#include <thread>
#include <vector>
#include <fstream>


// Queue of urls to request
std::queue<std::string> parseUrlFile(std::string url_file) {
    std::queue<std::string> url_queue;
    std::ifstream file(url_file);

    if (!file.is_open()) {
        std::cout << "Unable to open file" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(file, line)) {
        url_queue.push(line);
    }

    file.close();
    return url_queue;
}

// Thread Safe Cout
void safePrint(std::string& str) {
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    std::cout << str << std::endl;
}

void requestUrls(std::queue<std::string>& url_queue, ThreadSafeSet& known_hosts, ThreadSafeSet& known_ip, SocketClass socket class) {
    static std::mutex queue_mutex;
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (url_queue.empty()) {
        return;
    }
    std::string url = url_queue.front();
    url_queue.pop();
    lock.unlock();

    SocketClass Parse Url
	Socket Class Check host and ip
	Socket Class Check create http request
	Socket Class send request
	Socket Class recv loop
	Socket Class return string
	Socker Class kill buffer

	// when thread finished requesting lock section std::cout, then get from queue
	// inside function
	safePrint(str);
}

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
        return EXIT_FAILURE;
    }
    if (errno == ERANGE || thread_long != 1) {
        // number is too big
        std::cout << "Invalid thread count" << std::endl;
        return EXIT_FAILURE;
    }
    if (*end_ptr != '\0') {
        std::cout << "Invalid thread count" << std::endl;
        return EXIT_FAILURE;
    }
    int thread_count = static_cast<int>(thread_long);
    std::string url_file = argv[2];

    // Read file and create a queue of urls to request
    std::queue<std::string> url_queue = parseUrlFile(url_file);

    // Create an atomic hash set of previous hosts and ip
    ThreadSafeSet known_hosts;
    ThreadSafeSet known_ips;

    // Initialize threads with a url, 
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(function, i);
    }

    for (std::thread& t : threads) {
        t.join();
    }

    return 0;
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
