// Code referenced from Beej's Guide to Network Programming

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "dir.h"
#include "usage.h"

struct timeval timeout;
fd_set readfds;
int fdmax;
int sockfd; 
int datasockfd;
int new_datasockfd;
struct sockaddr server_addr; 
struct sockaddr_storage client_addr;
struct sockaddr_storage data_addr;
struct addrinfo * servinfo;
struct addrinfo hints;
struct addrinfo * p;
socklen_t addr_size;
socklen_t data_addr_size;
int new_fd; 
int is_logged_in;
int data_connection_open;
char current_directory[512] = {'\0'}; 
char root_dir[512];

// Return index within string
int return_char_index(char * str) {
    return strcspn(str, "\r\n");
}

// Send data 
int send_data(char * data) {
    printf("--> %s\n", data);
    return send(new_fd, data, strlen(data), 0);
}

// Receive data
int recv_data(char * buf) {
    memset(buf, 0, strlen(buf));
    int status = recv(new_fd, buf, strlen(buf), 0);
    printf("<-- %s", buf);
    return status;
}

// Send Response
int send_resp(int code) {
    char * response;
    switch (code) {
        case 125:
            response = "125 Data connection already open; transfer starting.\r\n";
            break;
        case 200:
            response = "200 Command okay.\r\n";
            break;
        case 220:
            response = "220 Service ready for new user.\r\n";
            break;
        case 221:
            response = "221 Service closing control connection.\r\n";
            break;
        case 226:
            response = "226 Closing data connection.\r\n";
            break;
        case 230:
            response = "230 User logged in, proceed.\r\n";
            break;
        case 250:
            response = "250 Requested file action okay, completed.\r\n";
            break;
        case 425:
            response = "425 Can't open data connection.\r\n";
            break;
        case 426:
            response = "426 Connection closed; transfer aborted.\r\n";
            break;
        case 421:
            response = "421 Timeout. Closing data connection.\r\n";
            break;
        case 430:
            response = "430 Invalid username or password.\r\n";
            break;
        case 500:
            response = "500 Syntax error, command unrecognized and the requested action did not take place.\r\n";
            break;
        case 501:
            response = "501 Syntax error in parameters or arguments.\r\n";
            break;
        case 503:
            response = "503 Bad sequence of commands.\r\n";
            break;
        case 504:
            response = "504 Command not implemented for that parameter.\r\n";
            break;
        case 530:
            response = "530 Not logged in.\r\n";
            break;
        case 550:
            response = "550 Requested action not taken. File or directory unavailable. (e.g. file not found, no access).\r\n";
            break;
        default:
            response = "500 Syntax error, command unrecognized and the requested action did not take place.\r\n";
            break;
    }
    return send_data(response);
}

int open_data_connection() {
    // Check for existing data connection
    if (!data_connection_open) {
        return -1;
    }

    data_addr_size = sizeof data_addr;

    // Set length of timeout in second, microseconds
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    // Timeout to accept connection 
    if (select(fdmax + 1, &readfds, NULL, NULL, &timeout) <= 0) {
        send_resp(421);
        // Clean out our state variables
        close(new_datasockfd);
        close(datasockfd);
        data_connection_open = 0;
        FD_ZERO(&readfds);
        return -10; 
    }
    
    // accept socket connection 
    new_datasockfd = accept(datasockfd, (struct sockaddr *) &data_addr, &data_addr_size);
    // Discard the data socket in our FD_SET, no longer need to keep track of timeout for this one
    FD_ZERO(&readfds);

    printf("Server: Incoming connection on data socket.\n");
    return 0;
}

// USER COMMAND
void user_command(char * input) {
    if (is_logged_in == 1) {
        send_resp(503);
    } else {
        char * username = strsep(&input, " ");
        // Check that at least one arg present
        if (username != NULL) {
            // Check for correct num of args
            int count = 1;
            while (strsep(&input, " ") != NULL) {
                count ++;
            }
            if (count == 1) {
                // Only user can be "cs317"
                username[return_char_index(username)] = '\0';
                if (strcmp("cs317", username) == 0) {
                    is_logged_in = 1;
                    send_resp(230);
                } else {
                    send_resp(430);
                }
            } else {
                send_resp(501); 
            }
        } 
        else {
            send_resp(501);
        }
    }
}

// MODE COMMAND
void mode_command(char * input) {
    if (is_logged_in == 1) {
        char * mode = strsep(&input, " ");
        // Check for at least one arg
        if (mode != NULL) {
            // Check for correct num of args
            int count = 1;
            while (strsep(&input, " ") != NULL) {
                count ++;
            }
            if (count == 1) {
                mode[return_char_index(mode)] = '\0';
                if (strcmp("S", mode) == 0){
                    send_resp(200);
                } else {
                    send_resp(504);
                }
            } else {
                send_resp(501);
            }
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// TYPE COMMAND
void type_command(char * input) {
    if (is_logged_in == 1) {
        char * type = strsep(&input, " ");
        // Check for at least one arg
        if (type != NULL) {
            // Check for correct num of args
            int count = 1;
            while (strsep(&input, " ") != NULL) {
                count ++;
            }
            if (count == 1) {
                type[return_char_index(type)] = '\0';
                if (strcmp("I", type) == 0 || strcmp("A", type) == 0) {
                    send_resp(200);
                } else {
                    send_resp(504);
                }
            } else {
                send_resp(501);
            }
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// STRU COMMAND
void stru_command(char * input) {
    if (is_logged_in == 1) {
        char * stru = strsep(&input, " ");
        // Check for at least one arg
        if (stru != NULL) {
            // Check for correct num of args
            int count = 1;
            while (strsep(&input, " ") != NULL) {
                count ++;
            }
            if (count == 1) {
                stru[return_char_index(stru)] = '\0';
                if (strcmp("F", stru) == 0) {
                    send_resp(200);
                } else {
                    send_resp(504);
                }
            } else {
                send_resp(501);
            }
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// CDUP COMMAND
void cdup_command(char * input) {
    if (is_logged_in == 1) {
        char * cdup  = strsep(&input, " ");
        // Check for no args 
        if (cdup == NULL) {
            // check that we aren't in root directory
            if (strcmp(root_dir, current_directory) == 0) {
                send_resp(550);
            } else {
                chdir("../");
                getcwd(current_directory, 512);
                printf("Current directory is now %s\n", current_directory);
                send_resp(200);
            }
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// CWD COMMAND
void cwd_command(char * input) {
    if (is_logged_in == 1) {
        char * cwd = strsep(&input, " ");
        // Check for at least one arg
        if (cwd != NULL) {
            // Check for correct num of args
            int count = 1;
            while (strsep(&input, " ") != NULL) {
                count ++;
            }
            if (count == 1) {
                cwd[return_char_index(cwd)] = '\0';
                // check that it is valid directory
                if ((strncmp(cwd, "./", 2) == 0) || (strncmp(cwd, "../", 3) == 0)) {
                    send_resp(550);
                } else if (strstr(cwd, "../")) {
                    send_resp(550);
                } else {
                    if (chdir(cwd) == 0) {
                        getcwd(current_directory, 512);
                        send_resp(200);
                        printf("Current directory is now %s\n", current_directory);
                    }
                    else {
                        send_resp(550);
                    }
                }
            }
            else {
                send_resp(501);
            }
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// RETR COMMAND
void retr_command(char * input) {
    if (is_logged_in == 1) {
        char * retr = strsep(&input, " ");
        // Check for at least one arg
        if (retr != NULL) {
            // Check for correct num of args
            int count = 1;
            while (strsep(&input, " ") != NULL) {
                count ++;
            }
            if (count == 1) {
                // Check that PASV was called first otherwise bad sequence
                if (data_connection_open) {
                    // Parse the desired filename
                    retr[return_char_index(retr)] = '\0';
                    FILE * file_fd;
                    // Open the file
                    if ((file_fd = fopen(retr, "r")) == NULL) {
                        printf("Server: Error opening file, not found or no permission.\n");
                        close(datasockfd);
                        data_connection_open = 0;
                        send_resp(550);
                        return;
                    }
                    // Seek to the end to determine filesize
                    fseek(file_fd, 0, SEEK_END);
                    long filesize = ftell(file_fd);
                    rewind(file_fd);
                    // Make a buffer and read the file into the buffer
                    char file_buf[filesize];
                    bzero(file_buf, filesize);
                    fread(file_buf, filesize, 1, file_fd);
                    // Establish data socket connection
                    int odc = open_data_connection();
                    if (odc < 0) {
                        if (odc == -10) {
                            data_connection_open = 0;
                            fclose(file_fd);
                            return;
                        }
                        send_resp(425);
                        perror("Server: Error accepting connection on data socket.\n");
                        close(datasockfd);
                        data_connection_open = 0;
                        fclose(file_fd);
                        return;
                    }
                    printf("Server: Sending file %s, filesize %ld\n", retr, filesize);
                    send_resp(125);
                    // Begin sending file
                    if (send(new_datasockfd, file_buf, filesize, 0) < 0) {
                        perror("Server: Error sending file.\n");
                        close(new_datasockfd);
                        close(datasockfd);
                        data_connection_open = 0;
                        fclose(file_fd);
                        return;
                    }
                    close(new_datasockfd);
                    close(datasockfd);
                    data_connection_open = 0;
                    fclose(file_fd);
                    send_resp(226);
                    printf("Server: File %s successfully sent.\n", retr);
                } else {
                    send_resp(425);
                    return;
                }
            } else {
                send_resp(501);
            }
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// PASV COMMAND
void pasv_command(char * input) {
    if (is_logged_in == 1) {
        char * pasv  = strsep(&input, " ");
        // Check for no args 
        if (pasv == NULL) {
            // Existing data socket is open, close it
            if (data_connection_open) {
                close(datasockfd);
            }

            // No easy way of getting the operating system to pick an unused port for data connection using the Beej guide method
            // Use the "old way" of creating a socket
            if ((datasockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                perror("Server: Error creating data socket\n");
                send_resp(425);
                return;
            }

            struct sockaddr_in data_addr;
            memset(&data_addr, 0, sizeof(data_addr));
            data_addr.sin_family = AF_INET;
            data_addr.sin_addr.s_addr = INADDR_ANY;
            // Set port to 0 to get the OS to assign a random port
            data_addr.sin_port = 0;

            if (bind(datasockfd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
                perror("Server: Error binding data socket\n");
                send_resp(425);
                return;
            }

            if (listen(datasockfd, 10) < 0) {
                perror("Server: Error listening on data socket\n");
                send_resp(425);
                return;
            }

            // Temporary sockaddr_in struct to hold IP and port data
            struct sockaddr_in temp_addr;
            socklen_t length = sizeof(temp_addr);

            // Get socket info from original control connection
            getsockname(new_fd, (struct sockaddr *)&temp_addr, &length);
            // Get IP from control connection
            unsigned int ip = temp_addr.sin_addr.s_addr;
            // printf("Data IP: %s\n", inet_ntoa(temp_addr.sin_addr.s_addr));

            // Get socket info from new data connection
            getsockname(datasockfd, (struct sockaddr *)&temp_addr, &length);
            // Get the port that was randomly assigned
            unsigned int port = ntohs(temp_addr.sin_port);

            // Split the IP into 4 pieces
            unsigned int ip1 = ip & 0xff;
            unsigned int ip2 = (ip >> 8) & 0xff;
            unsigned int ip3 = (ip >> 16) & 0xff;
            unsigned int ip4 = (ip >> 24) & 0xff;

            // Split the port into 2 pieces
            unsigned int port1 = port / 256;
            unsigned int port2 = port % 256;

            char buf[2048];
            sprintf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", ip1, ip2, ip3, ip4, port1, port2);
            send_data((char *)&buf);
            data_connection_open = 1;
            // Insert this new data socket into an FD_SET to keep track of timeouts
            FD_SET(datasockfd, &readfds);
            fdmax = datasockfd;
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

// NLST COMMAND
void nlst_command(char * input) {
    if (is_logged_in == 1) {
        char * nlst  = strsep(&input, " ");
        // Check for no args 
        if (nlst == NULL) {
            // Check if PASV was called
            if (data_connection_open == 0) {
                send_resp(425);
                return;
            }
            int odc = open_data_connection();
            if (odc < 0) {
                if (odc == -10) {
                    data_connection_open = 0;
                    return;
                }
                send_resp(425);
                perror("Server: Error accepting connection on data socket.\n");
                close(datasockfd);
                data_connection_open = 0;
                return;
            }
            send_resp(125);
            if (listFiles(new_datasockfd, (char *)&current_directory) < 0) {
                send_resp(550);
                printf("Server: Permission denied.\n");
                close(new_datasockfd);
                close(datasockfd);
                data_connection_open = 0;
                return;
            }
            send_resp(226);
            data_connection_open = 0;
            close(new_datasockfd);
            close(datasockfd);
        } else {
            send_resp(501);
        }
    } else {
        send_resp(530);
    }
}

//Listening to socket
int begin_listening() {

    // check if there is server 
    if (p == NULL) {
        fprintf(stderr, "Server: failed to bind.\n");
        return 1;
    }

    int result = listen(sockfd, 10);
    if (result != 0) {
        printf("Server: Error binding socket.\n");
        return 1;
    }

    // Clear our state variables
    is_logged_in = 0;
    data_connection_open = 0;
    getcwd(current_directory, 512);
    strcpy(root_dir, current_directory);

    char buf[2048]; // MTU of Ethernet is 1500

    // Main accept loop
    while (1) {

        // Clear our state variables
        FD_ZERO(&readfds);
        is_logged_in = 0;
        data_connection_open = 0;
        chdir(root_dir);
        strcpy(current_directory, root_dir);

        printf("Server: now waiting for connections.\n");

        // fill all of memory with 0s
        memset(&buf, 0, sizeof(buf)); 
        addr_size = sizeof client_addr;

        // accept socket connection 
        new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size);
        printf("Server: Incoming connection.\n");

        if (new_fd == -1) {
            printf("Server: Error accepting connection.\n");
            continue;
        }

        // Response is less than 1K bytes, do not need to resend unless error
        send_resp(220); 
        printf("Server: New connection accepted.\n");

        while (1) {
            // Add '\0' to end of buffer to avoid overflow when reading later
            memset(&buf, 0, sizeof(buf));
            result = recv(new_fd, &buf, sizeof(buf), 0);
            buf[result] = '\0'; 

            // If -1, connection was closed by client
            if (result <= -1) {
                printf("Server: Connection ended by client.\n");
                break;
            }
            // To avoid infinite loop
            if (buf[0] != 0) {  
                printf("<-- %s\n", buf);
                printf("\n");
                char * input = strdup(buf);
                char * ptr = input;
                char * command = strsep(&input, " ");
                
                // Convert all letters of command from response to upper-case
                int i;
                for (i =0; i < strlen(command); i++ ) {
                    command[i] = toupper(command[i]);
                }
                
                // USER COMMAND
                if (strncmp("USER", command, 4) == 0) {
                    user_command(input);
                    continue;
                }
                // QUIT COMMAND
                else if (strncmp("QUIT", command, 4) == 0) {
                    send_resp(221);
                    is_logged_in = 0;
                    close(new_fd);
                    if (data_connection_open) {
                        close(new_datasockfd);
                        close(datasockfd);
                    }
                    break;
                } 
                // CWD COMMAND
                else if (strncmp("CWD", command, 3) == 0) {
                    cwd_command(input);
                    continue;
                } 
                // CDUP COMMAND
                else if (strncmp("CDUP", command, 4) == 0) {
                    cdup_command(input);
                    continue;
                }
                // TYPE
                else if (strncmp("TYPE", command, 4) == 0) {
                    type_command(input);
                    continue;
                } 
                // MODE
                else if (strncmp("MODE", command, 4) == 0) {
                    mode_command(input);
                    continue;
                } 
                // STRU
                else if (strncmp("STRU", command, 4) == 0) {
                    stru_command(input);
                    continue;
                } 
                // RETR
                else if (strncmp("RETR", command, 4) == 0) {
                    retr_command(input);
                    continue;
                } 
                // PASV
                else if (strncmp("PASV", command, 4) == 0) {
                    pasv_command(input);
                    continue;
                }  
                // NLST
                else if (strncmp("NLST", command, 4) == 0) {
                    nlst_command(input);
                    continue;
                }                 
                else {
                    send_resp(500);
                    continue;
                }

                ptr = NULL;
                free(ptr);
            }

            close(new_fd);
            memset(&buf, 0, sizeof(buf)); 
        }
        close(new_datasockfd);
        close(datasockfd);
        close(new_fd);
        data_connection_open = 0; 
    }
    return 0; 
}

int main(int argc, char **argv) {

    // This is some sample code feel free to delete it
    // This is the main program for the thread version of nc
    
    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    // Set length of timeout in second, microseconds
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    // Clear all in socket set
    FD_ZERO(&readfds);

    // Set up a "helper" struct hints to help with socket binding
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 
    int yes = 1; 

    // Fill out our servinfo struct using our helper struct
    int result;
    result = getaddrinfo(NULL, argv[1], &hints, &servinfo);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return 1;
    }

    // Create a socket using our servinfo struct
    // Loop through results and bind to first port we can
    for (p = servinfo; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // Set up socket to allow reuse of port- yes is buf where result stored, sizeof(int)) is sizeof yes
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes ,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Bind our socket to the specified port
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }
    
    freeaddrinfo(servinfo); // info stored in p; free servinfo struct to clear mem 

    // Begin listening on the socket
    result = begin_listening();

    // Main listening loop returned, free our p struct to clear memory before returning
    freeaddrinfo(p);

    return result;
    
    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor 
    // returned for the ftp server's data connection
    
    printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;
}
