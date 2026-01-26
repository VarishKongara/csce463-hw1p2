#pragma once

#include "pch.h"
#include <sstream>

template <typename T>
struct Result {
    int error;
    T value;
};

struct http_req {
    std::string scheme;
    std::string host;
    int port = 80;
    std::string path;
    std::string query;
};

struct SocketWrapper {
    std::string url;

    std::stringstream output;
    http_req req;

    SOCKET sock;
    
    
    Result<http_req> parseURL(char* url);
    Result<bool> checkHost(const char* host, ThreadSafeSet& known_hosts);
    Result<bool> dnsLookup(sockaddr_in& server, const char* host);
    Result<bool> checkIP(const char* url);

    // Create socket connection
    Result<bool> connect(const char* url);
    // Send request
    Result<bool> sendRequest(const char* url);
    // Recv loop
    Result<bool> read();
    // Cleanup
   
    std::string requestURL(std::string url, ThreadSafeSet& known_hosts, ThreadSafeSet& known_ips);
    

};

Result<http_req> SocketWrapper::parseURL(char* url) {
    output << "\t  Parsing URL... ";

    http_req request;
    int error = 0;
    
    // Extract Scheme
    for (int i = 0; i < 7; i++) {
        if (url[i] == '\0') {
            output << "failed with invalid scheme" << std::endl;
            error = 1;
            return { 1, {} };
        }
    }

    std::string scheme(url, 7);
    if (scheme != "http://") {
        output << "failed with invalid scheme" << std::endl;
        error = 1;
        return { 1, {} };
    }

    request.scheme = "http://";
    
    url += 7;

    // Find # (fragment) using strchr() and trucate
    char* fragment = strchr(url, '#');
    if (fragment != nullptr) {
        fragment[0] = '\0';
    }

    // Find ?, extract query, and trucate
    char* query = strchr(url, '?');
    if (query != nullptr) {
        query[0] = '\0';
        ++query;
        request.query = query;
    }

    // Find /, extract path, and truncate
    char* path = strchr(url, '/');
    if (path != nullptr) {
        path[0] = '\0';
        ++path;
        request.path = path;
    }
    // Find :, extract port and truncate
    errno = 0;
    char* port = strchr(url, ':');
    if (port != nullptr) {
        port[0] = '\0';
        ++port;

        char* end_ptr;
        long int port_long = strtol(port, &end_ptr, 10);
        if (port == end_ptr) {
            // number could not be parsed
            output << "failed with invalid port" << std::endl;
            error = 1;
        }
        if (errno == ERANGE || port_long <= 0 || port_long > 65535) {
            // number is too big
            output << "failed with invalid port" << std::endl;
            error = 1;
        }
        if (*end_ptr != '\0') {
            output << "failed with invalid port" << std::endl;
            error = 1;
        }
        request.port = static_cast<int>(port_long);
    }

    request.host = url;

    return Result<http_req>{ error, request};
}

Result<bool> SocketWrapper::checkHost(const char* host, ThreadSafeSet& known_hosts) {
    output << "\t  Checking host uniqueness... ";
    if (known_hosts.seen(host)) {
        output << "failed" << std::endl;
        return Result<bool> {1, false};
    }
    
    output << "passed" << std::endl;
    return Result<bool> {0, true};
}

Result<bool> SocketWrapper::dnsLookup(sockaddr_in& server, const char* host) {
    output << "\t  Doing DNS... ";
    std::clock_t dns_start = std::clock();

    // if not a valid ip check host
    if (!(inet_pton(AF_INET, host, &(server.sin_addr)) == 1)) {        addrinfo hints = {};
        addrinfo* result = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(host, nullptr, &hints, &result) != 0) {
            output << "failed with " << WSAGetLastError() << std::endl;
            return { 1, false };
        }

        struct sockaddr_in* ipv4 = (struct sockaddr_in*)result->ai_addr;
        server.sin_addr = ipv4->sin_addr;

        freeaddrinfo(result);
    }

    std::clock_t dns_end = std::clock();
    double dns_duration = static_cast<double>(dns_end - dns_start) / (CLOCKS_PER_SEC / 1000);
    char ip_buffer[INET_ADDRSTRLEN] = {};
    const char* ip = inet_ntop(AF_INET, &(server.sin_addr), ip_buffer, INET_ADDRSTRLEN);
    output << "done in " << static_cast<int>(dns_duration) << " ms, found " << ip << std::endl;
}

std::string SocketWrapper::requestURL(std::string url, ThreadSafeSet& known_hosts, ThreadSafeSet& known_ips) {
    output << "URL: " << url << std::endl;
    
    // Make a copy of the url and parse
    char* url_copy = new char[url.size() + 1];
    errno_t err = strcpy_s(url_copy, url.size() + 1, url.c_str());
    if (err != 0) {
        output << "Failed to copy arguments" << std::endl;
    }
    Result<http_req> url_parse = parseURL(url_copy);
    if (url_parse.error) {
        return output.str();
    }
    req = url_parse.value;
    delete[] url_copy;

    output << "host " << req.host << ", port " << req.port << std::endl;
    
    // Check if the host is duplicate
    Result<bool> host_check = checkHost(req.host.c_str(), known_hosts);
    if (host_check.error) {
        return output.str();
    }

    // Perform dns lookup
    sockaddr_in server;
    Result<bool> dns_result = dnsLookup(server, req.host.c_str());

    server.sin_family = AF_INET;
    server.sin_port = htons(req.port);

    /*
    Socket Class Check host and ip
	Socket Class Check create http request
	Socket Class send request
	Socket Class recv loop
	Socket Class return string
	Socket Class kill buffer
    */

    return output.str();
}
