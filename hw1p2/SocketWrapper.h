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
    
    const int INITIAL_BUF_SIZE = 4096;
    const int THRESHOLD = 1024;
    const int MAX_ROBOT_SIZE = 16 * 1024;
    const int MAX_PAGE_SIZE = 2 * 1024 * 1024;
    int allocatedSize = 0;
    int curPos = 0;
    char* buf = nullptr;
    
    Result<http_req> parseURL(char* url);
    Result<bool> checkHost(const char* host, ThreadSafeSet& known_hosts);
    Result<std::string> dnsLookup(sockaddr_in& server, const char* host);
    Result<bool> checkIP(const char* ip, ThreadSafeSet& known_ips);

    Result<bool> socketConnect(sockaddr_in& server);

    std::string httpRobotsStr();
    std::string httpRequestStr();

    // Send request
    Result<bool> sendRequest(std::string req_str);
    // Recv loop
    Result<std::string> read(int& num_bytes, int max_size);
   
    std::string requestURL(std::string url, ThreadSafeSet& known_hosts, ThreadSafeSet& known_ips);
    
    //Cleanup
    void cleanup();
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

Result<std::string> SocketWrapper::dnsLookup(sockaddr_in& server, const char* host) {
    output << "\t  Doing DNS... ";
    std::clock_t dns_start = std::clock();

    // if not a valid ip check host
    if (!(inet_pton(AF_INET, host, &(server.sin_addr)) == 1)) {        addrinfo hints = {};
        addrinfo* result = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(host, nullptr, &hints, &result) != 0) {
            output << "failed with " << WSAGetLastError() << std::endl;
            return { 1, "" };
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

    return { 0, ip };
}

Result<bool> SocketWrapper::checkIP(const char* ip, ThreadSafeSet& known_ips) {
    output << "\t  Checking IP uniqueness... ";
    if (known_ips.seen(ip)) {
        output << "failed" << std::endl;
        return Result<bool> {1, false};
    }
    
    output << "passed" << std::endl;
    return Result<bool> {0, true};
}

Result<bool> SocketWrapper::socketConnect(sockaddr_in& server) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        output << "socket() created an error: " << WSAGetLastError() << std::endl;
        return Result<bool>{1, false};
    }

    if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
        output << "failed to connect with " << WSAGetLastError() << std::endl;
        return Result<bool>{1, false};
    }

    return Result<bool>{0, true};
}

std::string SocketWrapper::httpRobotsStr() {
    std::string request = "/robots.txt";

    request = "HEAD " + request + " HTTP/1.0\r\n" + "User-agent: csce463crawler/1.2\r\n"
        + "Host: " + req.host + "\r\nConnection: close\r\n" + "\r\n";;

    return request;
}

std::string SocketWrapper::httpRequestStr() {
    std::string request;
    if (req.query != "") {
        request = "/" + req.path + "?" + req.query;
    }
    else {
        request = "/" + req.path;
    }

    request = "GET " + request + " HTTP/1.0\r\n" + "User-agent: csce463crawler/1.2\r\n"
        + "Host: " + req.host + "\r\nConnection: close\r\n" + "\r\n";

    return request;
}

Result<bool> SocketWrapper::sendRequest(std::string req_str) {
    size_t total_sent = 0;
    while (total_sent < req_str.size()) {
        int sent_bytes = send(sock, req_str.c_str() + total_sent, req_str.size() - total_sent, 0);
        if (sent_bytes == SOCKET_ERROR) {
            output << "failed with " << WSAGetLastError() << std::endl;
            return { 1, false };
        }

        total_sent += sent_bytes;
    }
    return { 0, true };
}

Result<std::string> SocketWrapper::read(int& num_bytes, int max_size) {
    if (buf == nullptr) {
        buf = new char[INITIAL_BUF_SIZE];
        allocatedSize = INITIAL_BUF_SIZE;
        curPos = 0;
    }

    auto max_time = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        if (current_time >= max_time) {
            output << "failed with slow download" << std::endl;
            return Result<std::string>{1, ""};
        }

        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(max_time-current_time);

        fd_set fd;
        FD_ZERO(&fd);
        FD_SET(sock, &fd);
        timeval time;
        time.tv_sec = remaining.count() / 1000000;
        time.tv_usec = remaining.count() % 1000000;

        int ret = select(0, &fd, NULL, NULL, &time);
        if (ret > 0) {
            // leave space for null terminator by setting size to allocatedSize - curPos - 1
            int bytes = recv(sock, buf + curPos, allocatedSize - curPos - 1, 0);
            if (bytes == SOCKET_ERROR) {
                output << "failed with " << WSAGetLastError() << std::endl;
                return Result<std::string>{1, ""};
            }
            if (bytes == 0) {
                buf[curPos] = '\0';
                num_bytes = curPos;
                return Result<std::string>{0, buf};
            }

            curPos += bytes;
            if (curPos > max_size) {
                output << "failed with exceeding max" << std::endl;
                return Result<std::string>{1, ""};
            }
            if (allocatedSize - curPos < THRESHOLD) {
                if(allocatedSize * 2 > max_size) {
                    output << "failed with exceeding max" << std::endl;
                    return Result<std::string>{1, ""};
                }
                allocatedSize *= 2;
                char* new_buf = new char[allocatedSize];
                memcpy(new_buf, buf, curPos);
                delete[] buf;
                buf = new_buf;
            }
        }
        else if (ret == 0) {
            continue;
        }
        else {
            output << "failed with " << WSAGetLastError() << std::endl;
            return Result<std::string>{1, ""};
        }
    }

    return Result<std::string>{0, ""};  
}

std::string SocketWrapper::requestURL(std::string url, ThreadSafeSet& known_hosts, ThreadSafeSet& known_ips){
    output.str("");
    output.clear();
    output << "URL: " << url << std::endl;
    
    // Make a copy of the url and parse
    char* url_copy = new char[url.size() + 1];
    errno_t err = strcpy_s(url_copy, url.size() + 1, url.c_str());
    if (err != 0) {
        output << "Failed to copy arguments" << std::endl;
    }
    Result<http_req> url_parse = parseURL(url_copy);
    if (url_parse.error) {
        delete[] url_copy;
        return output.str();
    }
    req = url_parse.value;

    output << "host " << req.host << ", port " << req.port << std::endl;
    
    // Check if the host is duplicate
    Result<bool> host_check = checkHost(req.host.c_str(), known_hosts);
    if (host_check.error) {
        delete[] url_copy;
        return output.str();
    }

    // Perform dns lookup
    sockaddr_in server;
    Result<std::string> dns_result = dnsLookup(server, req.host.c_str());
    if (dns_result.error) {
        delete[] url_copy;
        return output.str();
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(req.port);

    // Check if the ip is dupicate
    Result<bool> ip_check = checkIP(dns_result.value.c_str(), known_ips);
    if (ip_check.error) {
        delete[] url_copy;
        return output.str();
    }

    // Connect a socket
    output << "\t  Connecting on robots... " << std::flush;
    std::clock_t conn_start = std::clock();
    Result<bool> socket = socketConnect(server);
    if (socket.error) {
        delete[] url_copy;
        return output.str();
    }
    std::clock_t conn_end = std::clock();
    double conn_duration = static_cast<double>(conn_end - conn_start) / (CLOCKS_PER_SEC / 1000);
    output << "done in " << static_cast<int>(conn_duration) << " ms" << std::endl;

    // Send request to robots
    std::string req_robots = httpRobotsStr();

    // Send robots request
    output << "\t  Loading... ";
    std::clock_t load_start = std::clock();
    Result<bool> send_req = sendRequest(req_robots);
    if (send_req.error) {
        delete[] url_copy;
        return output.str();
    }
    
    //read robots request
    int num_bytes = 0;
    Result<std::string>robots_response = read(num_bytes, MAX_ROBOT_SIZE);
    if (robots_response.error) {
        delete[] url_copy;
        return output.str();
    }

    std::string http_version;
    int status_code;
    std::istringstream stream(robots_response.value);
    if (!(stream >> http_version >> status_code)) {
        output << "failed with non-HTTP header (does not begin with HTTP/)" << std::endl;
        delete[] url_copy;
        return output.str();
    }
    else if (http_version.compare(0, 5, "HTTP/") != 0) {
        output << "failed with non-HTTP header (does not begin with HTTP/)" << std::endl;
        delete[] url_copy;
        return output.str();
    }
    std::clock_t load_end = std::clock();
    double load_duration = static_cast<double>(load_end - load_start) / (CLOCKS_PER_SEC / 1000);
    output << "done in " << static_cast<int>(load_duration) << " ms with " << num_bytes << " bytes" << std::endl;
    
    int close_sock = closesocket(sock);
    if (close_sock == SOCKET_ERROR) {
        output << "Failed to close socket with " << WSAGetLastError() << std::endl;
    }

    if (status_code < 400 || status_code > 499) {
        output << "\t  Verifying header... status code " << status_code << std::endl;
        delete[] url_copy;
        return output.str();
    }
    output << "\t  Verifying header... status code " << status_code << std::endl;
  
    curPos = 0;
    buf[curPos] = 0;

    // Connect a socket
    output << "\t* Connecting on page... " << std::flush;
    conn_start = std::clock();
    socket = socketConnect(server);
    if (socket.error) {
        delete[] url_copy;
        return output.str();
    }
    conn_end = std::clock();
    conn_duration = static_cast<double>(conn_end - conn_start) / (CLOCKS_PER_SEC / 1000);
    output << "done in " << static_cast<int>(conn_duration) << " ms" << std::endl;

    // Send request to page
    std::string req_str = httpRequestStr();

    // Send page request
    output << "\t  Loading... ";
    load_start = std::clock();
    send_req = sendRequest(req_str);
    if (send_req.error) {
        delete[] url_copy;
        return output.str();
    }
    
    //read page request
    num_bytes = 0;
    Result<std::string>request_response = read(num_bytes, MAX_PAGE_SIZE);
    if (request_response.error) {
        delete[] url_copy;
        return output.str();
    }

    std::string page_http_version = "";
    int page_status_code = 0;
    std::istringstream page_stream(request_response.value);
    //std::cout << request_response.value << std::endl << std::endl;
    if (!(page_stream >> page_http_version >> page_status_code)) {
        output << "failed with non-HTTP header (does not begin with HTTP/)" << std::endl;
        delete[] url_copy;
        return output.str();
    }
    else if (page_http_version.compare(0, 5, "HTTP/") != 0) {
        output << "failed with non-HTTP header (does not begin with HTTP/)" << std::endl;
        delete[] url_copy;
        return output.str();
    }
    load_end = std::clock();
    load_duration = static_cast<double>(load_end - load_start) / (CLOCKS_PER_SEC / 1000);
    output << "done in " << static_cast<int>(load_duration) << " ms with " << num_bytes << " bytes" << std::endl;
    
    close_sock = closesocket(sock);
    if (close_sock == SOCKET_ERROR) {
        output << "Failed to close socket with " << WSAGetLastError() << std::endl;
    }

    output << "\t  Verifying header... status code " << page_status_code << std::endl;

    // HTML Parsing
    std::string header;
    std::string body;
    std::string delimiter = "\r\n\r\n";
    size_t pos = request_response.value.find(delimiter);
    if (pos == std::string::npos) {
        delimiter = "\n\n";
        pos = request_response.value.find(delimiter);
    }
    if (pos != std::string::npos) {
        header = request_response.value.substr(0, pos);
        body = request_response.value.substr(pos + delimiter.length());
    }
    else {
        output << "Could not separate http header and body" << std::endl;
    }

    if (page_status_code >= 200 && page_status_code <= 299) {
        //parse html
        HTMLParserBase* parser = new HTMLParserBase;
        int nlinks;
        char* body_copy = new char[body.size() + 1];
        errno_t err = strcpy_s(body_copy, body.size() + 1, body.c_str());
        if (err != 0) {
            output << "Failed to copy html body" << std::endl;
        }

        std::clock_t parsing_start = std::clock();
        output << "\t+ Parsing page... ";
        char* linkBuffer = parser->Parse(body_copy, body.size(), url_copy, url.size(), &nlinks);
        delete[] body_copy;

        if (nlinks < 0) {
            output << "failed to parse with " << nlinks << std::endl;
            delete[] url_copy;
            return output.str();
        }

        std::clock_t parsing_end = std::clock();
        double parsing_duration = static_cast<double>(parsing_end - parsing_start) / (CLOCKS_PER_SEC / 1000);
        output << "done in " << static_cast<int>(parsing_duration) << " ms with " << nlinks << " links" << std::endl;

        delete parser;
    }

    delete[] url_copy;
    return output.str();
}

void SocketWrapper::cleanup() {
    if (allocatedSize > (32 * 1024)) {
        delete[] buf;
        buf = nullptr;
        allocatedSize = 0;
    }
    curPos = 0;
    if (buf == nullptr) {
        return;
    }
    buf[curPos] = '\0';
}
