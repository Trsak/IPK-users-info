#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pwd.h>

int main(int argc, char *argv[]) {
    int welcome_socket;
    struct sockaddr_in6 sa{};
    struct sockaddr_in6 sa_client{};
    char str[INET6_ADDRSTRLEN];
    int port_number = 0;
    int c;
    opterr = 0;

    //PWD
    struct passwd pwd{};
    struct passwd *result;
    char *buf;
    size_t bufsize;
    int s;

    //https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
    while ((c = getopt(argc, argv, "p:")) != -1) {
        switch (c) {
            case 'p':
                char *ptr;
                port_number = static_cast<int>(strtol(optarg, &ptr, 10));
                if (strlen(ptr) != 0) {
                    fprintf(stderr, "ERROR: Port must be integer!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case '?':
                if (optopt == 'p') {
                    fprintf(stderr, "ERROR: Option -p requires an argument.\n");
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

    if (optind < argc) {
        fprintf(stderr, "ERROR: Unknown argument!\n");
        exit(EXIT_FAILURE);
    }

    socklen_t sa_client_len = sizeof(sa_client);
    if ((welcome_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR: socket!\n");
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = in6addr_any;
    sa.sin6_port = htons(static_cast<uint16_t>(port_number));


    if (bind(welcome_socket, (struct sockaddr *) &sa, sizeof(sa))) {
        fprintf(stderr, "ERROR: bind!\n");
        exit(EXIT_FAILURE);
    }

    if ((listen(welcome_socket, 1)) < 0) {
        fprintf(stderr, "ERROR: listen!\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int comm_socket = accept(welcome_socket, (struct sockaddr *) &sa_client, &sa_client_len);
        if (comm_socket > 0) {
            if (inet_ntop(AF_INET6, &sa_client.sin6_addr, str, sizeof(str))) {
                printf("INFO: New connection:\n");
                printf("INFO: Client address is %s\n", str);
                printf("INFO: Client port is %d\n", ntohs(sa_client.sin6_port));
            }

            char buff[1024];
            int res = 0;
            for (;;) {
                res = static_cast<int>(recv(comm_socket, buff, 1024, 0));
                if (res <= 0)
                    break;

                std::string login;
                bool nParameter = false;
                bool fParameter = false;
                bool lParameter = false;

                char *recievedPart = strtok(buff, "::");
                while (recievedPart != nullptr) {
                    if (strcmp(recievedPart, "n") == 0) {
                        nParameter = true;
                    } else if (strcmp(recievedPart, "f") == 0) {
                        fParameter = true;
                    } else if (strcmp(recievedPart, "l") == 0) {
                        lParameter = true;
                    } else {
                        login = recievedPart;
                    }

                    recievedPart = strtok(nullptr, "::");
                }

                bufsize = static_cast<size_t>(sysconf(_SC_GETPW_R_SIZE_MAX));
                if (bufsize < 0)          /* Value was indeterminate */
                    bufsize = 16384;        /* Should be more than enough */

                buf = (char *) malloc(bufsize);
                if (buf == nullptr) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }

                std::string message;

                if (nParameter) {
                    s = getpwnam_r(login.c_str(), &pwd, buf, bufsize, &result);

                    if (result == nullptr) {
                        if (s == 0)
                            message = "ERROR: User not found.\n";
                        else {
                            message = "ERROR: getpwnam_r error.\n";
                        }
                    } else {
                        message = pwd.pw_gecos;
                        message += "\n";
                    }
                } else if (fParameter) {
                    s = getpwnam_r(login.c_str(), &pwd, buf, bufsize, &result);

                    if (result == nullptr) {
                        if (s == 0)
                            message = "ERROR: User not found.\n";
                        else {
                            message = "ERROR: getpwnam_r error.\n";
                        }
                    } else {
                        message = pwd.pw_dir;
                        message += "\n";
                    }
                } else if (lParameter) {
                    struct passwd *user;
                    while ((user = getpwent()) != nullptr) {
                        message += user->pw_name;
                        message += "\n";
                    }
                }

                send(comm_socket, message.c_str(), strlen(message.c_str()) + 1, 0);
                send(comm_socket, "::CLOSE::", 50, 0);
            }
        } else {
            printf(".");
        }

        printf("Connection to %s closed\n", str);
        close(comm_socket);
    }
}
