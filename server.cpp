/**
 * @project IPK-users-info
 * @file server.cpp
 * @author Petr Sopf (xsopfp00)
 * @brief Server for handling client requests (sends data from /etc/passwd)
 */
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
#include <sys/wait.h>
#include <signal.h>

/**
 * @brief Catching signals, waits for all proccesses to end.
 * @param n Signal number
 */
void SigCatcher(int n) {
    (void) n;
    wait3(nullptr, WNOHANG, nullptr);
}

int main(int argc, char *argv[]) {
    //Server variables
    int welcome_socket;
    struct sockaddr_in sa{};
    struct sockaddr_in sa_client{};
    char str[INET_ADDRSTRLEN];
    int port_number = 0;
    int c;

    //PWD.h variables
    struct passwd pwd{};
    struct passwd *result;
    size_t bufsize = 16384;
    char buf[bufsize];
    int s;

    //Parse arguments
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

    //Check for unknown argument
    if (optind < argc) {
        fprintf(stderr, "ERROR: Unknown argument!\n");
        exit(EXIT_FAILURE);
    }

    //Create welcome socket
    socklen_t sa_client_len = sizeof(sa_client);
    if ((welcome_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR: socket!\n");
        exit(EXIT_FAILURE);
    }

    //Set server variables
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(static_cast<uint16_t>(port_number));

    //Try to bind given port
    if (bind(welcome_socket, (struct sockaddr *) &sa, sizeof(sa))) {
        fprintf(stderr, "ERROR: bind!\n");
        exit(EXIT_FAILURE);
    }

    //Start listening on given port
    if ((listen(welcome_socket, 1)) < 0) {
        fprintf(stderr, "ERROR: listen!\n");
        exit(EXIT_FAILURE);
    }

    //Create signal catcher to wait for proccesses to end
    signal(SIGCHLD, SigCatcher);

    //Client connections loop
    while (true) {
        //Accept welcome socket
        int comm_socket = accept(welcome_socket, (struct sockaddr *) &sa_client, &sa_client_len);

        if (comm_socket <= 0) {
            break;
        }

        //Fork new child for multiclient server
        pid_t pid = fork();

        if (pid < 0) {
            perror("ERROR: fork");
            close(comm_socket);
            exit(EXIT_FAILURE);
        } else if (pid == 0) { //Inside child
            int child_pid = getpid();

            //Print client info
            if (inet_ntop(AF_INET, &sa_client.sin_addr, str, sizeof(str))) {
                printf("INFO: New connection (child %d):\n", child_pid);
                printf("INFO: Client address is %s\n", str);
                printf("INFO: Client port is %d\n", ntohs(sa_client.sin_port));
            }

            //Buffer for client's data
            char buff[1024];
            int res = 0;

            for (;;) {
                //Recieve data from client
                res = static_cast<int>(recv(comm_socket, buff, 1024, 0));
                if (res <= 0) {
                    break;
                }

                //Client arguments variables
                std::string login;
                bool nParameter = false;
                bool fParameter = false;
                bool lParameter = false;

                //Parse data obtained from client
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

                //Create string for output message
                std::string message;

                if (nParameter) { //Get login info
                    s = getpwnam_r(login.c_str(), &pwd, buf, bufsize, &result);

                    //Check for errors
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
                } else if (fParameter) { //Get login directory
                    s = getpwnam_r(login.c_str(), &pwd, buf, bufsize, &result);

                    //Check for errors
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
                } else if (lParameter) { //Get logins
                    //Rewind to the beginning of the password database
                    setpwent();

                    struct passwd *user;
                    //List through all records
                    while ((user = getpwent()) != nullptr) {
                        if ((!login.empty() &&
                             (strncmp(login.c_str(), user->pw_name, strlen(login.c_str())) == 0)) ||
                            login.empty()) { //If login is set, check if login is prefix of user's login
                            message += user->pw_name;
                            message += "\n";
                        }
                    }

                    //Close password database
                    endpwent();
                }

                std::string state; //String for server's actual state
                long int messageSize = strlen(message.c_str()); //Output message size
                long int actual = 0; //How many bytes of message were already sent

                //Loop for sending whole message
                while (actual < messageSize) {
                    //Send next 1023B of message to the client
                    memcpy(buff, message.c_str() + actual, 1023);
                    strcat(buff, "\0");
                    send(comm_socket, buff, 1024, 0);

                    actual += 1024;

                    if (actual >= messageSize) { //If whole message was sent, we cant signal client to close connection
                        state = "::CLOSE::";
                    } else {
                        state = "::CONTINUE::";
                    }

                    //Send state info
                    send(comm_socket, state.c_str(), 50, 0);
                }
            }
        }

        //Close connection
        close(comm_socket);
    }

}
