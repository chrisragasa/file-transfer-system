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
#include <dirent.h>

#define SIZE 75000
#define LARGE_SIZE 200000

void error(const char *msg, int exitVal);
int setupSocket(int portNumber, char *hostname);

int getCommand(char *commandBuffer);
int getDirectory(char *directoryStr);
int isValidFile(char *fileStr);

int main(int argc, char *argv[])
{
    int portNumber, dataPort, socketFD, newsocketFD, datasockFD, bytesSent, pid, bytesRecv, charsText, lines;
    struct sockaddr_in serverAddress;
    char commandBuffer[SIZE];
    char portBuffer[SIZE];
    char ipBuffer[SIZE];
    char fileBuffer[SIZE];

    char dirStr[LARGE_SIZE];
    char fileStr[LARGE_SIZE];

    char *confirm = "OK";
    char *test = "TEST";

    memset(commandBuffer, '\0', SIZE); // Fill arrays with null terminators and clear garbage
    memset(portBuffer, '\0', SIZE);    // Fill arrays with null terminators and clear garbage
    memset(ipBuffer, '\0', SIZE);      // Fill arrays with null terminators and clear garbage
    memset(fileBuffer, '\0', SIZE);    // Fill arrays with null terminators and clear garbage
    memset(dirStr, '\0', LARGE_SIZE);  // Fill arrays with null terminators and clear garbage
    memset(fileStr, '\0', LARGE_SIZE); // Fill arrays with null terminators and clear garbage

    /* Check for the correct number of arguments */
    if (argc < 2)
        error("ERROR: Incorrect number of arguments.\nSYNTAX: otp_enc_d port", 1);

    //Port and Socket Setup
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
    listen(socketFD, 5); // 5 concurrent connections
    // Listening Loop
    while (1)
    {
        newsocketFD = accept(socketFD, NULL, NULL); // Creates a new connected socket, returns it's new file descriptor
        if (newsocketFD < 0)
        {
            error("error: server couldn't set up socket on accept", 1);
        }
        pid = fork();
        if (pid < 0)
        {
            error("error: fork error", 1);
        }
        else if (pid == 0)
        {

            //Receive the command
            memset(commandBuffer, '\0', SIZE);
            bytesRecv = recv(newsocketFD, commandBuffer, SIZE - 1, 0);
            if (bytesRecv < 0)
            {
                error("error: server can't read from the socket", 1);
            }
            send(newsocketFD, confirm, strlen(confirm), 0);

            //Receive the port number for data transfer
            memset(portBuffer, '\0', SIZE);
            bytesRecv = recv(newsocketFD, portBuffer, SIZE - 1, 0);
            if (bytesRecv < 0)
            {
                error("error: server can't read from the socket", 1);
            }
            send(newsocketFD, confirm, strlen(confirm), 0);

            //Receive the client's IP address
            memset(ipBuffer, '\0', SIZE);
            bytesRecv = recv(newsocketFD, ipBuffer, SIZE - 1, 0);
            if (bytesRecv < 0)
            {
                error("error: server can't read from the socket", 1);
            }
            send(newsocketFD, confirm, strlen(confirm), 0);

            //Receive the file name
            if (getCommand(commandBuffer) == 2)
            {
                memset(fileBuffer, '\0', SIZE);
                bytesRecv = recv(newsocketFD, fileBuffer, SIZE - 1, 0);
                if (bytesRecv < 0)
                {
                    error("error: server can't read from the socket", 1);
                }
                send(newsocketFD, confirm, strlen(confirm), 0);
            }

            printf("command buffer:%s\n", commandBuffer);
            printf("port buffer:%d\n", atoi(portBuffer));
            printf("ip buffer:%s\n", ipBuffer);
            printf("file buffer:%s\n", fileBuffer);

            printf("command:%d\n", getCommand(commandBuffer));

            // Setup data socket
            datasockFD = setupSocket(atoi(portBuffer), ipBuffer);
            //send(datasockFD, test, strlen(test), 0);

            if (getCommand(commandBuffer) == 1)
            {
                // Store directory contents in buffer
                getDirectory(dirStr);

                // Send to the client
                bytesSent = 0; // Keep track of the bytes sent
                charsText = send(datasockFD, dirStr, LARGE_SIZE - 1, 0);
                bytesSent = bytesSent + charsText; // Keep track of the bytes sent
                if (charsText < 0)
                {
                    error("ERROR: server can't send encryption to socket", 1);
                }
                // While the total amount of bytes sent does not equal the size of the message
                while (bytesSent < LARGE_SIZE - 1)
                {
                    charsText = send(datasockFD, &dirStr[bytesSent], SIZE - (bytesSent - 1), 0); // Send the bytes that haven't been sent yet
                    bytesSent = bytesSent + charsText;                                           // Keep track of the bytes sent
                }
            }
            else if (getCommand(commandBuffer) == 2)
            {
                // Check if the file exists
                if (isValidFile(fileBuffer))
                {
                    printf("IS VALID FILE");
                }
                else
                {
                    printf("IS NOT VALID FILE");
                }
            }

            /*
            sprintf(dirLinesBuffer, "%d", getDirectory()); // Convert return value from getDirectory (int) to a string for buffer usage
            send(datasockFD, dirLinesBuffer, sizeof(dirLinesBuffer), 0);
            sendDirectory(datasockFD);
            */

            close(newsocketFD); // Close the socket
            close(datasockFD);  // Close the socket
            close(socketFD);    // Close the socket
            exit(0);            // Child dies
        }
        else
        {
            close(newsocketFD); // Parent closes the new socket
        }
    }
    return 0;
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

int getDirectory(char *directoryStr)
{
    int lines = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_type == DT_REG || dir->d_type == DT_DIR) // If the directory is a regular file or another directory
            {
                strcat(directoryStr, dir->d_name);
                strcat(directoryStr, "\n");
                lines++;
            }
        }
        closedir(d);
    }
    return lines;
}

int isValidFile(char *fileStr)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_type == DT_REG || dir->d_type == DT_DIR) // If the directory is a regular file or another directory
            {
                if (strcmp(fileStr, dir->d_name) == 0)
                    return 1;
            }
        }
        closedir(d);
    }
    return 0;
}

int setupSocket(int portNumber, char *hostname)
{
    int socketFD;                                     // Socket file descriptor
    struct sockaddr_in serverAddress;                 // Server address struct
    struct hostent *server = gethostbyname(hostname); // Get host IP
    if (server == NULL)
        error("error: host was not found", 1);

    /* Open the socket */
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Returns a socket descriptor that we can use in later system calls. returns -1 on error.
    if (socketFD < 0)
    {
        error("error: opening socket", 1);
    }

    /* Set up server address struct */
    memset((char *)&serverAddress, '\0', sizeof(serverAddress));                             // Clear out the address struct
    serverAddress.sin_family = AF_INET;                                                      // Create a network-capable socket
    bcopy((char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length); // Copy serv_addr ip into server->h_addr
    serverAddress.sin_port = htons(portNumber);                                              // Store the port number

    /* Connect to server */
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        error("error: connecting to socket", 1); // Connection error
    }

    return socketFD;
}

int getCommand(char *commandBuffer)
{
    // Return 1 if require listing
    if (strcmp("-l", commandBuffer) == 0)
    {
        return 1;
    }
    // Return 2 if require file transfer
    if (strcmp("-g", commandBuffer) == 0)
    {
        return 2;
    }
    return -1;
}