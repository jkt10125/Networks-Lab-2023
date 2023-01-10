/*
** Date: 08/01/2023 | Time: 15:15:36
** Name: Jay Kumar Thakur
** Rollno: 20CS30024
*/

/* Assumptions made:

    1. the result of the expression can be stored completely in the
        string of size bufsize without any overflow.
    2. the packets recived by the server is of size smaller than 
        bufsize.
    3. server stops reciving the packets if end of string '\0' is 
        encountered.
    4. It should be noted that we are dealing with floating point 
        numbers hence the calculated answer will lie within the 
        5% error range of the expected answer.

*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int bufsize = 10000; // MAX BUFFER LIMIT

const int zero = 48; // CONSTANT CHARACTER 0

// checking whether the char is a digit or not
int isNum(char c) {
    int a = c - zero;
    return (a >= 0 && a <= 9);
}

// utility function to execute the given operation
void doOperation(float *ans, float *num, char op) {
    
    switch (op) {
    case '+':
        *ans += *num;
        break;

    case '-':
        *ans -= *num;
        break;

    case '*':
        *ans *= *num;
        break;

    case '/':
        *ans /= *num;
        break;

    default:
        printf("Invalid Operation! %c\n", op);
    }
}

// Exp function to process the expression string
float Exp(char *s, int len, int *idx) {
    
    float ans = 0.0; // variable to store the final answer of the function
    float num = 0.0; // variable to store the current number of the function
    
    int precn = 10; // precision variable to add the fractional digit to num
    int isDec = 0; // flag which tells whether decimal is encountered or not
    
    char op = '+'; // operation variable


    while (*idx < len) {
        
        // adding the digit to the num if it is a number
        if (isNum(s[*idx])) {
            
            int digit = s[*idx] - zero;
            
            if (isDec) {
                
                num += digit * 1.0 / precn;
                precn *= 10;
            }
            else {
                
                num *= 10;
                num += digit;
            }
        }

        // trigger the isDec flag if a point is encountered
        else if (s[*idx] == '.') {
            
            isDec = 1;
        }

        // recursive callig the function if a '(' is encountered
        else if (s[*idx] == '(') {
            
            (*idx)++;
            num = Exp(s, len, idx);
        }

        // if it is a space, ignore it
        else if (s[*idx] == ' ');
        
        // do the operation and update the operator
        else {
            
            doOperation(&ans, &num, op);
            
            num = 0.0;
            isDec = 0;
            precn = 10;

            if (s[*idx] == ')') {
                
                (*idx)++;
                return ans;
            }

            else {
                
                op = s[*idx];
            }
        }

        (*idx)++;
    }

    // processing the last number and updating the answer
    doOperation(&ans, &num, op);

    return ans;
}


// this fucntion reads the data sent by the clients
// in the form of packets and stops receiving packets 
// when '\0' is encountered
void Recv(int sockfd, char **recv_buf) {
    
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

        for (int i = 0; i < len; i++) {

            char *tmp = *recv_buf + idx + i;
            *tmp = temp_buf[i];
            // recv_buf[idx + i] = temp_buf[i];
        }

        idx += len;
    }

}

int main() {
    
    int sockfd; // variable for socket creation
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // checking for socket creation failures
    if (sockfd < 0) {
        
        perror("Socket creation failed!");
        exit(0);
    }
    else {
        
        printf("Socket successfully created!\n");
    }

    struct sockaddr_in ClintAddress, ServerAddress;

    // setting the serveraddress specifiations
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons (20000);
    ServerAddress.sin_addr.s_addr = INADDR_ANY;

    int bindStatus;
    // binding with local address
    bindStatus = bind(sockfd, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress));

    //checking for binding failures
    if (bindStatus < 0) {
        
        perror("Local Address bind failed!");
        exit(0);
    }
    else {
        
        printf("bind() executed successfully!\n");
    }

    // a max of 5 clients can wait in the queue if server is busy
    listen(sockfd, 5);  

    int clientNumber = 0; // variable to count the number of clients connected so far
    
    while (1) {
        
        int clintAddressLength = sizeof(ClintAddress);

        // making an accept call to the client
        int newsockfd = accept(sockfd, (struct sockaddr *) &ClintAddress, &clintAddressLength);

        // checking for the accept failures
        if (newsockfd < 0) {
            
            perror("Accept failed!");
            exit(0);
        }
        else {
            
            printf("New client connected! #%d\n", ++clientNumber);
        }

        // setting up the send and receive buffer
        char *send_buf = (char *) malloc(sizeof(char) * bufsize);
        
        // char *recv_buf = (char *) malloc(sizeof(char) * bufsize);

        char *recv_buf = (char *) malloc(sizeof(char));

        recv_buf[0] = '\0';
        int receivedLength = 0;

        // while the client does not sends "-1" the connection remains valid
        while (strcmp (recv_buf, "-1")) {
            
            // receivedLength = recv(newsockfd, recv_buf, bufsize, 0);

            Recv(newsockfd, &recv_buf);

            printf("--> %s\n", recv_buf);

            // if the received string is "-1" then send GoodBye to client
            if (strcmp (recv_buf, "-1") == 0) {
                strcpy(send_buf, "Good Bye!");
                send_buf[9] = '\0';
            }

            // else evaluate the expression and put it in send buffer
            else {
                int idx = 0;
                gcvt(Exp(recv_buf, strlen(recv_buf), &idx), 10, send_buf);
            }
            
            // sending the response to the user
            send(newsockfd, send_buf, strlen(send_buf) + 1, 0);

        }

        // closing the new socket with a message of disconnection
        close(newsockfd);

        printf("Connection closed!\n");
    
    }

    return 0;
}