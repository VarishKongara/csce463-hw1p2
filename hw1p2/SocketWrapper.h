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
    
    
    Result<http_req> parseURL(char* url);
   
    std::string requestURL(std::string url);
    

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
        }
    }

    std::string scheme(url, 7);
    if (scheme != "http://") {
        output << "failed with invalid scheme" << std::endl;
        error = 1;
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

std::string SocketWrapper::requestURL(std::string url) {
    output << "URL: " << url << std::endl;
    
    // Make a copy of the url and parse
    char* url_copy = new char[url.size() + 1];
    errno_t err = strcpy_s(url_copy, url.size() + 1, url.c_str());
    if (err != 0) {
        std::cout << "Failed to copy arguments" << std::endl;
    }
    Result<http_req> url_parse = parseURL(url_copy);
    if (url_parse.error) {
        return output.str();
    }
    req = url_parse.value;

    output << "host " << req.host << ", port " << req.port << std::endl;

    /*
    Socket Class Check host and ip
	Socket Class Check create http request
	Socket Class send request
	Socket Class recv loop
	Socket Class return string
	Socker Class kill buffer
    */

    return output.str();
}
