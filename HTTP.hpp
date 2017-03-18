#ifndef HTTP_hpp
#define HTTP_hpp

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <arpa/inet.h>
#include <cstdio>
#include <sys/stat.h>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <fstream>
#include <thread>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sstream>
#include <sqlite3.h>
#include <vector>
#include <cstddef>
#include <openssl/sha.h>
#include <ctime>

class Request
{
  public:
    std::string method;
    std::string URI;
    std::string query;
    std::string version;
    std::unordered_map<std::string, std::string> request_options;
    std::string content;
    std::unordered_map<std::string, std::string> content_JSON;
    Request();
    void show_options();
    void parse_message(char &message);
    void resolve_URI_query();
    void advanced_message_parser(char *original_message);
    void parse_JSON();
};

Request::Request()
{
    request_options.rehash(10);
}

void Request::show_options()
{
    for (auto it = request_options.begin(); it != request_options.end(); ++it)
    {
        std::cout << it->first << ":" << it->second << '\n';
    }
    std::cout << std::endl;
    for (auto it = content_JSON.begin(); it != content_JSON.end(); ++it)
    {
        std::cout << it->first << ":" << it->second << '\n';
    }
    std::cout << std::endl;
}

void Request::parse_message(char &message)
{
    method = strtok(&message, " ");
    URI = strtok(NULL, " ");
    version = strtok(NULL, "\n");
    version.pop_back(); // erase remaining \r at tbe end of the line
    char *name = strtok(NULL, ": ");
    char *value = strtok(NULL, "\n");
    value[strlen(value) - 1] = '\0'; // erases ramaining \r at the end of line
    while (name != NULL && value != NULL)
    {
        request_options.insert(std::pair<std::string, std::string>((std::string)name, (std::string)value));
        name = strtok(NULL, ": ");
        value = strtok(NULL, "\n");
        if (value != NULL) // prevents reference outside of memory
        {
            value[strlen(value) - 1] = '\0'; // erases ramaining \r at the end of line
        }
    }
    resolve_URI_query();
}

void Request::advanced_message_parser(char *original_message)
{
    std::string message, header;
    message.assign(original_message);
    std::size_t found = message.find("\r\n\r\n");
    header = message.substr(0, found + 4);
    char *c_header = new char[header.size() + 1];
    strcpy(c_header, header.c_str());
    c_header[header.size()] = '\0';
    parse_message(*c_header);
    if (found + 4 < message.size())
    {
        this->content = message.substr(found + 4, message.size());
        parse_JSON();
    }
    else
    {
        this->content = "";
        this->content_JSON.clear();
    }
}

void Request::parse_JSON()
{
    this->content_JSON.clear();
    std::stringstream ss(content);
    ss.seekg(2);
    std::string name, value;
    while (ss)
    {
        std::getline(ss, name, '\"');
        ss.seekg(ss.tellg() + long(2));
        std::getline(ss, value, '\"');
        ss.seekg(ss.tellg() + long(2));
        std::pair<std::string, std::string> name_value_pair(name, value);
        this->content_JSON.insert(name_value_pair);
        if (ss.peek() == '}')
            break;
    }
}

void Request::resolve_URI_query()
{
    std::size_t found = URI.find_first_of('?');
    if (found != std::string::npos)
    {
        std::stringstream ss(URI);
        std::getline(ss, URI, '?');
        std::getline(ss, query, '\r');
    }
}

class Session
{
  private:
    bool login_status;
    int user_id;
    unsigned int heartbeat;

  public:
    std::string connected_username;
    SSL *ssl;
    Session(SSL *ssl);
    int get_user_id();
    int set_user_id(int id);
    void set_login_status(bool status);
    bool get_login_status();
    void inc_heartbeat();
    unsigned int get_heartbeat();
};

Session::Session(SSL *ssl)
{
    login_status = false;
    heartbeat = 1;
    this->ssl = ssl;
}

bool Session::get_login_status()
{
    return this->login_status;
}

int Session::get_user_id()
{
    return this->user_id;
}
int Session::set_user_id(int id)
{
    this->user_id = id;
}
void Session::inc_heartbeat()
{
    this->heartbeat++;
}

unsigned int Session::get_heartbeat()
{
    return this->heartbeat;
}

void Session::set_login_status(bool status)
{
    login_status = status;
}
#endif /* HTTP_hpp */
