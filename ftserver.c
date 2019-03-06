/*
Author: Christopher Ragasa
Date: Mar 5, 2019
Program Description: ftserver.c is the server side of the file transfer system
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#define SIZE 75000

void error(const char *msg, int exitVal);

int main(int argc, char *argv[])
{
    int portNumber, textLength, keyLength, socketFD, newsocketFD, bytesSent, charsText, charsKey, pid, r, checkSockSending = 1, checkSockRecieving = 0;
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    char stringText[SIZE];
    char stringKey[SIZE];
    char cipherArray[SIZE];
    char readBuffer[512];
    socklen_t cliLength;

    memset(stringText, '\0', SIZE);  // Fill arrays with null terminators and clear garbage
    memset(stringKey, '\0', SIZE);   // Fill arrays with null terminators and clear garbage
    memset(cipherArray, '\0', SIZE); // Fill arrays with null terminators and clear garbage
    memset(readBuffer, '\0', 512);   // Fill arrays with null terminators and clear garbage

    /* Check for the correct number of arguments */
    if (argc < 2)
        error("ERROR: Incorrect number of arguments.\nSYNTAX: otp_enc_d port", 1);

    /* Port and Socket Setup */
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    portNumber = atoi(argv[1]);                 // Get the port number
    if (socketFD < 0)
    { // Check for socket creation error
        error("ERROR: server couldn't open the socket", 1);
    }
    memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    serverAddress.sin_family = AF_INET;                          // Create a network-capable socket
    serverAddress.sin_addr.s_addr = INADDR_ANY;                  // Any address is allowed for connection to this process
    serverAddress.sin_port = htons(portNumber);                  // Store the port number
    if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    { // Check for bind error
        error("error: server couldn't bind", 0);
    }
    listen(socketFD, 5);               // 5 concurrent connections
    cliLength = sizeof(clientAddress); // Set the length of the client address

    /* Listening Loop */
    while (1)
    {
        newsocketFD = accept(socketFD, (struct sockaddr *)&clientAddress, &cliLength); // Creates a new connected socket, returns it's new file descriptor
        if (newsocketFD < 0)
        { // Error if the socket couldn't be created
            error("error: server couldn't set up socket on accept", 1);
        }
        pid = fork(); // Create child
        if (pid < 0)
        {
            error("error: fork error", 1);
        }
        else if (pid == 0)
        { // I am the child
            printf("AYO");
            close(newsocketFD); // Close the socket
            close(socketFD);    // Close the socket
            exit(0);            // Child dies
        }
        else
        {
            close(newsocketFD); // Parent closes the new socket
        }
    }
}

/*
 * Function: error
 * ---------------------------- 
 *   Utility function that prints error message and exits program with status value
 *
 *   msg: error message to be printed
 *   exitVal: exit status value
 *
 *   post-conditions: exits the program 
 */
void error(const char *msg, int exitVal)
{
    fprintf(stderr, "%s\n", msg);
    exit(exitVal);
}