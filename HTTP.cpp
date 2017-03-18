#pragma once
#include "HTTP.hpp"
#include <stdlib.h>

bool GET_method(Request &request, Session &session);
bool POST_method(Request &request, Session &session);
int callback(void *values, int number_of_columns, char **data, char **columns);
void sha256(char *string, char outputBuffer[65]);
bool prepare_sql_command(Request &request, Session &session, std::string &sql);
void hash_password(Request &request);
void handle_response(Request &request, std::vector<std::vector<std::string>> &table, const bool status, Session &session);

#define OK true
#define NOT_OK false

/*------------------------------- GET METHOD (START) ------------------------------- */

bool GET_method(Request &request, Session &session)
{

    std::string response;
    std::string header = "HTTP/1.1 200 OK\r\nContent-Type: */*\r\nConnection: keep-alive\r\nContent-Length: ";
    std::ifstream fin;

    if (request.URI == "/api/heartbeat")
    {
        time_t current_time;
        time(&current_time);
        std::cout << "HEARTBEAT ESTE: " << session.get_heartbeat() << '\n';
        response = "{\"heartbeat\":" + std::to_string(session.get_heartbeat()) + ",\"timestamp\":\"\\/Date(" + std::to_string(current_time) + ")\\/\"}";
        header += std::to_string(response.size()) + "\r\n\r\n" + response;
        session.inc_heartbeat();
        SSL_write(session.ssl, header.c_str(), header.size());
    }
    else
    {
        if (request.URI == "/")
        {
            fin.open("html/VirtualSoc/index.html", std::ios::binary);
        }
        else
        {
            request.URI = "html/VirtualSoc" + request.URI;
            fin.open(request.URI, std::ios::binary);

            if (!fin.is_open())
            {
                std::cout << "failed to open\n";
                fin.open("html/VirtualSoc/index.html", std::ios::binary);
                if (!fin.is_open())
                {
                    std::cout << "critical error\n";
                }
            }
        }

        std::stringstream message;
        message << fin.rdbuf();
        message.seekp(0, std::ios::end);
        std::stringstream::pos_type len = message.tellp();
        message.seekp(0, std::ios::beg);
        response.reserve(5 + len + header.size());
        response = header + std::to_string(len) + "\r\n\r\n" + message.str();
        if (SSL_write(session.ssl, response.c_str(), response.size()) <= 0)
        {
            perror("[server]Eroare la write() catre client.\n");
            return 1;
        }
        else
            printf("[server]Mesajul a fost trasmis cu succes.\n");
        fin.close();
    }
    return 0;
}

/*------------------------------- GET METHOD (END) ------------------------------- */

/*------------------------------- POST METHOD (START) ------------------------------- */

bool POST_method(Request &request, Session &session)
{

    std::string response;
    std::string header = "HTTP/1.1 200 OK\r\nContent-Type: */*\r\nConnection: keep-alive\r\nContent-Length: ";

    sqlite3 *db;
    char *dbErrorMessage = 0;
    if (sqlite3_open("database/virtualsoc.db", &db))
    {
        perror("Open database error.\n");
        std::cout << sqlite3_errmsg(db) << '\n';
        SSL_write(session.ssl, "HTTP/1.1 500 Internal Server Error\r\n\r\n", strlen("HTTP/1.1 500 Internal Server Error\r\n\r\n"));
    }
    else
    {
        std::cout << "Database opened successfully\n";
    }

    std::vector<std::vector<std::string>> table;

    std::string sql;

    if (prepare_sql_command(request, session, sql))
    {
        if (sqlite3_exec(db, sql.c_str(), callback, (void *)&table, &dbErrorMessage) != SQLITE_OK)
        {
            perror("SQL error");
            std::cout << dbErrorMessage << '\n';
            sqlite3_free(dbErrorMessage);
            std::cout << "Error executing sql\n";
            handle_response(request, table, NOT_OK, session);
        }
        else
        {
            std::cout << "Operation done successfully\n";
            handle_response(request, table, OK, session);
        }
    }
    else
    {
        SSL_write(session.ssl, "HTTP/1.1 500 Internal Server Error\r\n\r\n", strlen("HTTP/1.1 500 Internal Server Error\r\n\r\n"));
    }

    sqlite3_close(db);
    return 0;
}

/*------------------------------- POST METHOD (END) ------------------------------- */

/*------------------------------- PREPARE SQL COMMAND (START) ------------------------------- */

bool prepare_sql_command(Request &request, Session &session, std::string &sql)
{

    std::vector<std::pair<std::string, std::string>> request_content;
    std::cout << "let\'s the the PATH: " << request.URI << '\n';

    if (request.URI == "/register")
    {
        if (request.content_JSON.size() != 3)
            return NOT_OK;
        hash_password(request);
        sql = "INSERT INTO User ('username', 'password', 'u_visibility') VALUES (\'" + request.content_JSON["username"] + "\', \'" + request.content_JSON["password"] + "\', \'" + request.content_JSON["visibility"] + "\')";
        std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
    }
    else if (request.URI == "/login")
    {
        if (request.content_JSON.size() != 2)
            return NOT_OK;
        hash_password(request);
        sql = "SELECT u_id from User WHERE username = \'" + request.content_JSON["username"] + "\' AND password = \'" + request.content_JSON["password"] + "\'";
        std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
    }
    else if (request.URI.compare(0, 5, "/user") == 0)
    {
        if (request.content_JSON.size() != 1)
            return NOT_OK;
        if (session.get_login_status() == false)
        {
            sql = "SELECT p.p_content, p.p_date_created, p.p_visibility from Post p where p.u_id in(SELECT u_id from User where username = '" + request.content_JSON["username"] + "' and u_visibility = 'public')";
        }
        else if (request.content_JSON["username"] == session.connected_username)
        {
            sql = "SELECT p.p_content, p.p_date_created, p.p_visibility from Post p where p.u_id = " + std::to_string(session.get_user_id());
        }
        else
        {
            sql = "SELECT p.p_content, p.p_date_created, p.p_visibility from Post p where p.u_id = (SELECT u_id from User where username ='" + request.content_JSON["username"] + "') and (p.p_visibility = (SELECT friend_type from Relationship where u_id = p.u_id and friend_u_id = (SELECT u_id from User where username = '" + session.connected_username + "')) or p.p_visibility = 'public')";
        }
        std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
    }
    else if (request.URI == "/search")
    {
        if (request.content_JSON.size() != 1)
            return NOT_OK;
        if (session.get_login_status() == false)
        {
            sql = "SELECT username from User where u_visibility = 'public' and username like '%" + request.content_JSON["username"] + "%'";
        }
        else
        {
            sql = "SELECT username from User where username like '%" + request.content_JSON["username"] + "%'";
        }
        std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
    }
    else if (request.URI == "/post")
    {
        if (request.content_JSON.size() != 3)
            return NOT_OK;
        if (session.get_login_status() == true)
        {
            sql = "INSERT INTO Post ('p_content', 'u_id', 'p_date_created', 'p_visibility') values ('" + request.content_JSON["content"] + "','" +
                  std::to_string(session.get_user_id()) + "','" + request.content_JSON["date"] + "','" + request.content_JSON["visibility"] + "')";
            std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
        }
    }
    else if (request.URI == "/relationship")
    {
        if (request.content_JSON.size() != 2)
            return NOT_OK;
        if (session.get_login_status() == true)
        {
            sql = "INSERT into Relationship values('" + std::to_string(session.get_user_id()) + "',(select u_id from User where username = '" + request.content_JSON["username"] + "'), '" + request.content_JSON["friendType"] + "')";
            std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
        }
    }
    else if (request.URI == "/messages/receive")
    {
        if (request.content_JSON.size() != 1)
            return NOT_OK;
        if (session.get_login_status() == true)
        {
            sql = "SELECT m.message_body, m.sent_date from Messages m where (m.u_id_sender in(SELECT u_id from User where username = '" + request.content_JSON["username"] + "') or m.u_id_sender = '" + std::to_string(session.get_user_id()) + "')" + "and (m.u_id_receiver in(SELECT u_id from User where username = '" + request.content_JSON["username"] + "') or m.u_id_receiver = '" + std::to_string(session.get_user_id()) + "')";
            std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
        }
    }
    else if (request.URI == "/messages/send")
    {
        if (request.content_JSON.size() != 3)
            return NOT_OK;
        if (session.get_login_status() == true)
        {
            sql = "INSERT INTO Messages values('" + std::to_string(session.get_user_id()) + "',(select u_id from User where username = '" + request.content_JSON["username"] + "'),'" + request.content_JSON["content"] + "', '" + request.content_JSON["date"] + "')";
            std::cout << "The SQL command you\'re about to execute is: " << sql << std::endl;
        }
    }

    return OK;
}

/*------------------------------- PREPARE SQL COMMAND (END) ------------------------------- */

void hash_password(Request &request)
{
    char outputBuffer[65];
    sha256(&request.content_JSON["password"][0], outputBuffer);
    request.content_JSON["password"] = std::string(outputBuffer);
}

/*------------------------------- SHA256 HEXADECIMAL (START) ------------------------------- */

void sha256(char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

/*------------------------------- SHA256 HEXADECIMAL (END) ------------------------------- */

/*------------------------------- CALLBACK FOR EACH ELEMENT OF DATABSE CALL(START) ------------------------------- */

int callback(void *values, int number_of_columns, char **data, char **columns)
{
    std::vector<std::string> row;
    std::vector<std::vector<std::string>> *table = static_cast<std::vector<std::vector<std::string>> *>(values);
    for (int i = 0; i < number_of_columns; i++)
    {
        row.push_back(data[i] ? data[i] : "NULL");
    }
    table->push_back(row);
    return 0;
}
/*------------------------------- CALLBACK FOR EACH ELEMENT OF DATABSE CALL(END) ------------------------------- */

/*------------------------------- HANDLER BASED ON URI AND DATABASE RETURNED TABLE (START) ------------------------------- */

void handle_response(Request &request, std::vector<std::vector<std::string>> &table, const bool status, Session &session)
{
    std::string header, message, response;
    if (request.URI == "/register")
    {
        if (status)
        {
            message = "{\"message\": \"Registration OK, please log in\", \"uri\": \"/login\"}";
            header = "HTTP/1.1 201 Created\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
        }
        else
        {
            message = "{\"message\": \"Username is already taken, please choose another\"}";
            header = "HTTP/1.1 409 Conflict\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
        }
    }
    else if (request.URI == "/login")
    {
        std::cout << "Deci am mers pe ramura de logiiiiiiiin\n";
        if (status && table.size() != 0)
        {
            std::cout << "hopa, se pare ca e si ok totul\n";
            message = "{\"message\": \"Login was succesful\",\"uri\": \"/user/" + request.content_JSON["username"] + "\"}";
            header = "HTTP/1.1 201 Created\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
            session.set_login_status(true);
            session.set_user_id(std::stoi(table[0][0]));
            session.connected_username = request.content_JSON["username"];
        }
        else
        {
            std::cout << "lucrurile nu arata prea roz\n";
            message = "{\"message\": \"Username or password is not correct\"}";
            header = "HTTP/1.1 401 Unauthorized\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
        }
    }
    else if (request.URI.compare(0, 5, "/user") == 0)
    {
        if (status == OK)
        {
            if (table.size() > 0)
            {

                message += "[{\"content\":\"" + table[0][0] + "\",\"date\":\"" + table[0][1] + "\", \"visibility\":\"" + table[0][2] + "\"}";
                for (int i = 1; i < table.size(); ++i)
                {
                    message += ",{\"content\":\"" + table[i][0] + "\",\"date\":\"" + table[i][1] + "\", \"visibility\":\"" + table[i][2] + "\"}";
                }
                message += "]";
                header = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
                response = header + message;
            }
        }
        else
        {
            message = "[{\"warning\": \"An internal error occured\"}]";
            header = "HTTP/1.1 500 Internal Server Error\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
        }

        std::cout << "the content we write back is : " << response << '\n';
        SSL_write(session.ssl, response.c_str(), response.size());
    }
    else if (request.URI == "/search")
    {
        if (table.size() > 0)
        {
            message += "[{\"content\" : \"" + table[0][0] + "\"}";
            for (int i = 1; i < table.size(); ++i)
            {
                message += ",{\"content\" : \"" + table[i][0] + "\"}";
            }
            message += "]";
        }
        std::cout << "the content we write back is : " << message << '\n';
        header = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
        response = header + message;
        SSL_write(session.ssl, response.c_str(), response.size());
    }
    else if (request.URI == "/post")
    {
        if (status == OK)
        {
            message = "{\"message\": \"Post was succesfuly inserted in database\"}";
            header = "HTTP/1.1 201 Created\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
        }
        else
        {
            header = "HTTP/1.1 500 Internal Server Error\r\nConnection: keep-alive\r\n\r\n";
            SSL_write(session.ssl, header.c_str(), response.size());
        }
    }
    else if (request.URI == "/relationship")
    {
        if (status == OK)
        {
            message = "{\"message\": \"Relationship was succesfuly inserted in database\"}";
            header = "HTTP/1.1 201 Created\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
        }
        else
        {
            header = "HTTP/1.1 500 Internal Server Error\r\nConnection: keep-alive\r\n\r\n";
            SSL_write(session.ssl, header.c_str(), response.size());
        }
    }
    else if (request.URI == "/messages/receive")
    {
        if (status == OK)
        {
            if (table.size() > 0)
            {

                message += "[{\"content\":\"" + table[0][0] + "\",\"date\":\"" + table[0][1] + "\"}";
                for (int i = 1; i < table.size(); ++i)
                {
                    message += ",{\"content\":\"" + table[i][0] + "\",\"date\":\"" + table[i][1] + "\"}";
                }
                message += "]";
                header = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
                response = header + message;
            }
            else
            {
                message = "[{\"warning\": \"An internal error occured\"}]";
                header = "HTTP/1.1 500 Internal Server Error\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
                response = header + message;
            }
            SSL_write(session.ssl, response.c_str(), response.size());
        }
    }
    else if (request.URI == "/messages/receive")
    {
        if (status == OK)
        {
            message = "{\"message\": \"Message was succesfuly inserted in database\"}";
            header = "HTTP/1.1 201 Created\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(message.size()) + "\r\n\r\n";
            response = header + message;
            SSL_write(session.ssl, response.c_str(), response.size());
        }
        else
        {
            header = "HTTP/1.1 500 Internal Server Error\r\nConnection: keep-alive\r\n\r\n";
            SSL_write(session.ssl, header.c_str(), response.size());
        }
    }
    std::cout << "database query returned:\n";
    for (auto i = 0; i < table.size(); ++i)
    {
        for (auto j = 0; j < table[i].size(); ++j)
        {
            std::cout << table[i][j] << ' ';
        }
        std::cout << std::endl;
    }
}

/*------------------------------- HANDLER BASED ON URI AND DATABASE RETURNED TABLE (END) ------------------------------- */