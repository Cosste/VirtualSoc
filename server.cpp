#ifndef server_hpp
#define server_hpp

#include "HTTP.hpp"
#include "HTTP.cpp"
#include <string.h>
#include <stdio.h>

// port used
#define PORT 2024

// buffer size
#define RESPONSE_BUFFER 8192
#define RECEIVE_BUFFER 4096

/* FUNCTIONS */

int starting_server(int &socket_descriptor, struct sockaddr_in &server, struct sockaddr_in &from);
void service(int &socket_descriptor, struct sockaddr_in &server, struct sockaddr_in &from);
void request_header_parser(char &message, Request &request);
short int request_parser(Request &request, Session &session);
bool send_response(Request &request, Session &session);
bool connect_session(int client, SSL *ssl);
void init_openssl();
void cleanup_openssl();
SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);

/*------------------------------- PREPARING AND STARTING SERVER (BEGIN) ------------------------------- */

int starting_server(int &socket_descriptor, struct sockaddr_in &server, struct sockaddr_in &from)
{
    init_openssl(); //initializare openssl
    //crearea socketului
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error raised by socket().\n");
        return errno;
    }
    //pregatirea structurile de date
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = (short)htons(PORT);

    //atasarea socketului
    if (bind(socket_descriptor, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Error raised by bind().\n");
        return errno;
    }

    //serverul incepe sa astepte clienti

    if (listen(socket_descriptor, 100) == -1)
    {
        perror("Error raised by listen().\n");
        return errno;
    }

    return 0;
}

/*------------------------------- PREPARING AND STARTING SERVER (END) ------------------------------- */

/*------------------------------- SERVER'S MAIN SERVICE (BEGIN) ------------------------------- */

void service(int &socket_descriptor, struct sockaddr_in &server, struct sockaddr_in &from)
{
    SSL_CTX *ctx;
    ctx = create_context();
    configure_context(ctx);
    while (1)
    {
        int client;
        socklen_t length = sizeof(from);

        std::cout << "Waiting on port: " << PORT << std::endl;

        client = accept(socket_descriptor, (struct sockaddr *)&from, &length);
        if (client < 0)
        {
            perror("Error raised by accept().\n");
            continue;
        }
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);
        if (SSL_accept(ssl) <= 0)
        {
            std::cout << "Error accept();\n";
            ERR_print_errors_fp(stderr);
            continue;
        }
        std::thread serv(connect_session, client, ssl);
        serv.detach();
    }
}

/*------------------------------- SERVER'S MAIN SERVICE (END) ------------------------------- */

/*------------------------------- STARTING A CONNECTION SESSION (BEGIN) ------------------------------- */

bool connect_session(int client, SSL *ssl)
{
    Request request;
    Session session(ssl);
    while (1)
    {
        if (session.get_login_status())
        {
            std::cout << "User with ID " << session.get_user_id() << " is connected\n";
        }
        else
        {
            std::cout << "This connection is not linked to any user\n";
        }
        bool request_parser_return = request_parser(request, session);
        std::cout << "request parser returned " << request_parser_return << '\n';
        if (request_parser_return != 0)
        {
            std::cout << std::endl;
            SSL_free(ssl);
            close(client);
            return 1;
        }

        if (send_response(request, session) == 1)
        {
            SSL_free(ssl);
            close(client);
            return 1;
        }
        if (request.request_options["Connection"] != " keep-alive")
        {
            std::cout << "Browser doesn't support keep-alive protocol.\n";
            break;
        }
    }

    std::cout << "Connection ended due to keep-alive protocol unssuported.\n";
    SSL_free(ssl);
    close(client);
    return 0;
}

/*------------------------------- STARTING A CONNECTION SESSION (END) ------------------------------- */

/*------------------------------- HTTP REQUEST PARSER (BEGIN) ------------------------------- */

short int request_parser(Request &request, Session &session)
{

    char message[RECEIVE_BUFFER];

    bzero(message, RECEIVE_BUFFER);

    std::cout << "Waiting for message" << std::endl;
    int ssl_return = SSL_read(session.ssl, message, RECEIVE_BUFFER);

    if (ssl_return <= 0)
    {
        perror("Error raised by read() from client (client dissconeccted).\n");
        return -1;
    }
    request.advanced_message_parser(message);
    std::cout << request.method << '\n';
    std::cout << request.URI << '\n';
    std::cout << request.query << '\n';
    std::cout << request.version << '\n';
    request.show_options();
    std::cout << request.content << '\n';
    return 0;
}

/*------------------------------- HTTP REQUEST PARSER (END) ------------------------------- */

/*------------------------------- HTTP SEND RESPONSE (START) ------------------------------- */

bool send_response(Request &request, Session &session)
{
    if (request.method == "GET")
    {
        GET_method(request, session);
    }
    else if (request.method == "POST")
    {
        POST_method(request, session);
    }
    return 0;
}

/*------------------------------- HTTP SEND RESPONSE (END) ------------------------------- */

/*------------------------------- SSL (START) ------------------------------- */
void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) < 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) < 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}
/*------------------------------- SSL (END) ------------------------------- */

#endif /* server_hpp */