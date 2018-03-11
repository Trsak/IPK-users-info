#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cctype>
#include <cstring>
#include <string>

#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    int client_socket, port_number = 0, bytestx, bytesrx, bytesrx2;
    const char *server_hostname = nullptr;
    char *login;
    struct hostent *server;
    struct sockaddr_in server_address{};

    std::string messageToSend;
    bool portArg = false, nArg = false, fArg = false, lArg = false;
    int c;
    opterr = 0;

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

    if (server_hostname == nullptr) {
        fprintf(stderr, "ERROR: Missing host (-h) argument!\n");
        exit(EXIT_FAILURE);
    } else if (!portArg) {
        fprintf(stderr, "ERROR: Missing port (-p) argument!\n");
        exit(EXIT_FAILURE);
    }

    if (optind >= argc && !lArg) {
        fprintf(stderr, "ERROR: Missing login parameter!\n");
        exit(EXIT_FAILURE);
    }

    if (lArg && optind >= argc) {
        login = const_cast<char *>("");
    }
    else {
        login = argv[optind++];
    }

    if (optind < argc) {
        fprintf(stderr, "ERROR: Unknown argument!\n");
        exit(EXIT_FAILURE);
    }

    /* 2. ziskani adresy serveru pomoci DNS */
    if ((server = gethostbyname(server_hostname)) == nullptr) {
        fprintf(stderr, "ERROR: no such host as %s\n", server_hostname);
        exit(EXIT_FAILURE);
    }

    /* 3. nalezeni IP adresy serveru a inicializace struktury server_address */
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy(server->h_addr, (char *) &server_address.sin_addr.s_addr, static_cast<size_t>(server->h_length));
    server_address.sin_port = htons(static_cast<uint16_t>(port_number));

    /* Vytvoreni soketu */
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }

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

    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0) {
        perror("ERROR: connect");
        exit(EXIT_FAILURE);
    }

    char message[BUFSIZE];
    char state[BUFSIZE];
    strcpy(message, messageToSend.c_str());

    /* odeslani zpravy na server */
    bytestx = static_cast<int>(send(client_socket, message, strlen(message) + 1, 0));
    if (bytestx < 0)
        perror("ERROR in sendto");

    /* prijeti odpovedi a jeji vypsani */
    while (true) {
        bytesrx = static_cast<int>(recv(client_socket, message, BUFSIZE, 0));
        bytesrx2 = static_cast<int>(recv(client_socket, state, 50, 0));
        if (bytesrx < 0 || bytesrx2 < 0)
            perror("ERROR in recvfrom");

        printf("%s", message);

        if (strcmp(state, "::CLOSE::") == 0) {
            break;
        }
    }

    close(client_socket);
    return 0;
}
