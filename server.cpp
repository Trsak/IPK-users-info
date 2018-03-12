#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pwd.h>

int main(int argc, char *argv[]) {
    int welcome_socket;
    struct sockaddr_in sa{};
    struct sockaddr_in sa_client{};
    char str[INET_ADDRSTRLEN];
    int port_number = 0;
    int c;
    opterr = 0;

    //PWD
    struct passwd pwd{};
    struct passwd *result;
    char buf[16384];
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
    if ((welcome_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR: socket!\n");
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(static_cast<uint16_t>(port_number));


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
            if (inet_ntop(AF_INET, &sa_client.sin_addr, str, sizeof(str))) {
                printf("INFO: New connection:\n");
                printf("INFO: Client address is %s\n", str);
                printf("INFO: Client port is %d\n", ntohs(sa_client.sin_port));
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

                bufsize = 16384;

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
                    setpwent();
                    struct passwd *user;

                    while ((user = getpwent()) != nullptr) {
                        if ((!login.empty() && (strncmp(login.c_str(), user->pw_name, strlen(login.c_str())) == 0)) ||
                            login.empty()) {
                            message += user->pw_name;
                            message += "\n";
                        }
                    }

                    endpwent();
                }

                std::string ending;

                long int messageSize = strlen(message.c_str());
                long int actual = 0;

                while (actual < messageSize) {
                    memcpy(buff, message.c_str() + actual, 1023);
                    strcat(buff, "\0");
                    send(comm_socket, buff, 1024, 0);

                    actual += 1024;

                    if (actual >= messageSize) {
                        ending = "::CLOSE::";
                    } else {
                        ending = "";
                    }
                    send(comm_socket, ending.c_str(), 50, 0);
                }
            }
        } else {
            printf(".");
        }

        printf("Connection to %s closed\n", str);
        close(comm_socket);
    }
}
