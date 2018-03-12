/**
 * @project IPK-users-info
 * @file client.cpp
 * @author Petr Sopf (xsopfp00)
 * @brief Client for getting data from /etc/passwd
 */
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    //Connection variables
    int client_socket, port_number = 0, bytestx, bytesrx, bytesrx2;
    const char *server_hostname = nullptr;
    struct hostent *server;
    struct sockaddr_in server_address{};

    //Argument variables
    char *login;
    bool portArg = false, nArg = false, fArg = false, lArg = false;
    int c;

    //Parse arguments
    while ((c = getopt(argc, argv, "h:p:nfl")) != -1) {
        switch (c) {
            case 'h':
                server_hostname = optarg;
                break;
            case 'p':
                char *ptr;
                port_number = static_cast<int>(strtol(optarg, &ptr, 10));
                if (strlen(ptr) != 0) {
                    fprintf(stderr, "ERROR: Port must be integer!\n");
                    exit(EXIT_FAILURE);
                }
                portArg = true;
                break;
            case 'n':
                nArg = true;
                break;
            case 'f':
                fArg = true;
                break;
            case 'l':
                lArg = true;
                break;
            case '?':
                if (optopt == 'p' || optopt == 'h') {
                    fprintf(stderr, "ERROR: Option -%c requires an argument.\n", optopt);
                    exit(EXIT_FAILURE);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "ERROR: Unknown option `-%c'.\n", optopt);
                    exit(EXIT_FAILURE);
                } else {
                    fprintf(stderr, "ERROR: Unknown option character `\\x%x'.\n", optopt);
                    exit(EXIT_FAILURE);
                }
            default:
                abort();
        }
    }

    //Check for required arguments
    if (server_hostname == nullptr) {
        fprintf(stderr, "ERROR: Missing host (-h) argument!\n");
        exit(EXIT_FAILURE);
    } else if (!portArg) {
        fprintf(stderr, "ERROR: Missing port (-p) argument!\n");
        exit(EXIT_FAILURE);
    }

    //Check if only one of f, n, l parameters were used
    int argsN = nArg + fArg + lArg;
    if (argsN != 1) {
        fprintf(stderr, "ERROR: You can only use -f or -n or -l parameters!\n");
        exit(EXIT_FAILURE);
    }

    //If n or l parameters are used, we have to check for login argument
    if (optind >= argc && !lArg) {
        fprintf(stderr, "ERROR: Missing login parameter!\n");
        exit(EXIT_FAILURE);
    }

    //Fill login variable with argument, if not set, then fill it with empty string.
    if (lArg && optind >= argc) {
        login = const_cast<char *>("");
    } else {
        login = argv[optind++];
    }

    //Check for any unknown arguments
    if (optind < argc) {
        fprintf(stderr, "ERROR: Unknown argument!\n");
        exit(EXIT_FAILURE);
    }

    //Get IP address and check if host exists
    if ((server = gethostbyname(server_hostname)) == nullptr) {
        fprintf(stderr, "ERROR: Unknown host: %s!\n", server_hostname);
        exit(EXIT_FAILURE);
    }

    //Find server IP address and initialize server_address
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy(server->h_addr, (char *) &server_address.sin_addr.s_addr, static_cast<size_t>(server->h_length));
    server_address.sin_port = htons(static_cast<uint16_t>(port_number));

    //Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }

    //Create "welcome" message
    std::string messageToSend;
    messageToSend = login;

    if (nArg) {
        messageToSend += "::n";
    }

    if (fArg) {
        messageToSend += "::f";
    }

    if (lArg) {
        messageToSend += "::l";
    }

    //Connect to server
    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0) {
        perror("ERROR: connect");
        exit(EXIT_FAILURE);
    }

    //Communication variables
    char message[BUFSIZE]; //Buffer for server messages
    char state[BUFSIZE]; //Buffer for server states
    char errorMessage[] = "ERROR"; //Error string
    char *errorCheck = nullptr; //String for error checking


    strcpy(message, messageToSend.c_str());

    //Send "welcome" message to server
    bytestx = static_cast<int>(send(client_socket, message, strlen(message) + 1, 0));
    if (bytestx < 0) {
        perror("ERROR: sendto");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    //Wait for server's answer and print message
    while (true) {
        bytesrx = static_cast<int>(recv(client_socket, message, BUFSIZE, 0)); //Recieve message
        bytesrx2 = static_cast<int>(recv(client_socket, state, 50, 0)); //Recieve server status
        if (bytesrx < 0 || bytesrx2 < 0) {
            perror("ERROR: recvfrom");
            exit(EXIT_FAILURE);
        }

        //Check if message contains error
        errorCheck = strstr(message, errorMessage);
        if (errorCheck) {
            fprintf(stderr, "%s", message);
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        //Print server message
        printf("%s", message);

        //Check if server wants to close connection
        if (strcmp(state, "::CLOSE::") == 0) {
            break;
        }
    }

    close(client_socket);
    return 0;
}
