/*
** Date: 20/01/2023 | Time: 02:47:55
** Author: Jay KUmar Thakur
** Rollno: 20CS30024
*/

#include <stdio.h> 
#include <dirent.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

const int packetSize = 37;


// this function sends the data to the client  
// in form of packets of desired length
void Send(int sockfd, char *send_buf) {
    int len = strlen(send_buf);

    int idx = 0;

    while (idx < len - packetSize) {

        send(sockfd, &send_buf[idx], packetSize, 0);
        idx += packetSize;
    }

    send(sockfd, &send_buf[idx], len - idx + 1, 0);
}


// this fucntion reads the data sent by the clients
// in the form of packets and stops receiving packets 
// when '\0' is encountered
void Recv(int sockfd, char **recv_buf) {
    
    int bufsize = 100000;
    if (*recv_buf != NULL) {

        free(*recv_buf);
    }
    
    *recv_buf = NULL;
    int recvBufSize = 0;
    
    char *temp_buf = (char *) malloc(sizeof(char) * bufsize);

    int readingIsAllowed = 1, idx = 0;
    
    int len = 0;

    while (readingIsAllowed) {

        len = recv(sockfd, temp_buf, bufsize, 0);

        if (temp_buf[len - 1] == '\0') {
            readingIsAllowed = 0;
        }

        *recv_buf = realloc(*recv_buf, idx + len);

        char *tmp = *recv_buf + idx;

        for (int i = 0; i < len; i++) {

            *(tmp++) = temp_buf[i];
        }

        idx += len;
    }

}

// utility function that searches for a particular username
// in the users.txt file
int search (FILE *fileptr, char *username) {

    char x[30];
    /* assumes no word exceeds length of 30 */
    while (fscanf(fileptr, " %30s", x) == 1) {
        if (strcmp(x, username) == 0) return 1;
    }
    return 0;
}


// Utility function to process the commands
void process_cmd(char *s, char **result) {
    
    // free up unwanted memory allocated before
    if (*result != NULL) {
        free (*result);
    }
    *result = NULL;
    
    char *word = strtok(s, " ");

    if (strcmp(word, "pwd") == 0) {
        
        word = strtok(NULL, " ");

        // Assuming current working directory info fits under 256 byte 
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            
            *result = strdup("####");
        }
        else {

            *result = strdup(cwd);
        }
    }

    else if (strcmp(word, "cd") == 0) {
        
        word = strtok(NULL, " ");

        // if user enters "cd" then it should change its directory
        // to home directory
        if (word == NULL) {
            
            word = getenv("HOME");
        }
        
        // if there is error in changing the directory 
        // or more than 1 arguments is given by the user
        if (chdir(word) != 0 || (word = strtok(NULL, " ")) != NULL) {

            *result = strdup("####");
        }
        else {
            // successfully changed the directory
            *result = strdup("");
        }
        
    }

    else if (strcmp(word, "dir") == 0) {

        word = strtok(NULL, " ");
        int dirNameWrite = 1;
        *result = strdup("");

        if (word == NULL) {
            
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                
                *result = strdup("####");
                return;
            }

            word = strdup(cwd);

            dirNameWrite = 0;
        }

        
        // this loop is for checking each directory
        do {

            DIR *dir;
            struct dirent *entry;

            if ((dir = opendir(word)) == NULL) {
                
                free(*result);

                // if any argument provided by the user is wrong 
                // then we raise the error
                *result = strdup("####");
                return;
            }

            char *tmp;

            int initialLength = strlen(*result);

            *result = realloc(*result, initialLength + strlen(word) + 3);

            tmp = *result + initialLength;

            // printing the name of the directory at the top (if required!)
            if (dirNameWrite) {
                for (int i = 0; i < strlen(word); i++) {

                    *(tmp++) = word[i];
                }

                *(tmp++) = ':';
                *(tmp++) = '\n';
                *(tmp) = '\0';
            }

            // after that we print all its file and subdirectory names
            while ((entry = readdir(dir)) != NULL) {
                
                initialLength = strlen(*result);

                *result = realloc(*result, initialLength + strlen(entry->d_name) + 2);
                
                tmp = *result + initialLength;

                for (int i = 0; i < strlen(entry->d_name); i++) {

                    *(tmp++) = entry->d_name[i];
                }

                *(tmp++) = ' ';
                *(tmp) = '\0';

            }

            initialLength = strlen(*result);
            
            *result = realloc(*result, initialLength + 2);

            tmp = *result + initialLength;

            *(tmp++) = '\n';
            *(tmp) = '\0';

            closedir(dir);

        } while ((word = strtok(NULL, " ")) != NULL);
    }

    else {
        // if none of the above arguments are matched then
        // it is error in command syntax
        *result = strdup("$$$$");
    }
}


int main() {

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {

        perror("Socket Creation failed!");
        exit(0);
    }

    printf("Socket created successfully!\n");

    struct sockaddr_in ClientAddress, ServerAddress;

    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(20000);
    ServerAddress.sin_addr.s_addr = INADDR_ANY;

    int bindStatus;

    bindStatus = bind(sockfd, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress));

    if (bindStatus < 0) {

        perror("Bind() failed!");
        exit(0);
    }

    printf("Bind() executed successfully!\n");


    listen(sockfd, 5);

    while (1) {

        int clientAddressLength = sizeof(ClientAddress);

        int newsockfd = accept(sockfd, (struct sockaddr *) &ClientAddress, &clientAddressLength);
    
        if (newsockfd < 0) {

            perror("Accept failed!");
            exit(0);
        }

        if (fork() == 0) {
            
            close(sockfd);

            // sending the LOGIN string and expecting the 
            // username from the client side
            char *loginString = strdup("LOGIN: ");

            Send(newsockfd, loginString);

            char *username;
            Recv(newsockfd, &username);

                FILE *fileptr = fopen("users.txt", "r");

                // if users.txt file not found then we raise the error
                if (!fileptr) {

                    perror("users file not found!");
                    exit(0);
                }


            loginString = strdup("NOT-FOUND");
            if (search(fileptr, username)) {

                loginString = strdup("FOUND");
            }

            // sending the login status back to the client
            Send(newsockfd, loginString);

            if (strcmp(loginString, "FOUND") == 0) {
                
                char *command = NULL;
                char *result = NULL;

                Recv(newsockfd, &command);

                // while user doesn't sends exit command we process the commands
                while (strcmp(command, "exit") != 0) {

                    // process command
                    process_cmd(command, &result);
                    
                    // send the result to the user
                    Send(newsockfd, result);

                    // recieve another command from the user
                    Recv(newsockfd, &command);
                }

            }


            close(newsockfd);
            exit(0);

        }

        close(newsockfd);
    }
    
    return 0;
}