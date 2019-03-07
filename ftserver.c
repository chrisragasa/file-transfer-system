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

void error(const char *msg, int exitVal);
int setupSocket(int portNumber, char *hostname);
void sendDirectory(int socketFD);

char **createDirArray(int size);
int getDirectory(char **directoryArray);

int main(int argc, char *argv[])
{
    int portNumber, dataPort, socketFD, newsocketFD, datasockFD, bytesSent, pid, bytesRecv, lines;
    struct sockaddr_in serverAddress;
    char commandBuffer[SIZE];
    char portBuffer[SIZE];
    char ipBuffer[SIZE];
    char dirLinesBuffer[SIZE];
    char *confirm = "OK";
    char *test = "TEST";

    memset(commandBuffer, '\0', SIZE);  // Fill arrays with null terminators and clear garbage
    memset(portBuffer, '\0', SIZE);     // Fill arrays with null terminators and clear garbage
    memset(ipBuffer, '\0', SIZE);       // Fill arrays with null terminators and clear garbage
    memset(dirLinesBuffer, '\0', SIZE); // Fill arrays with null terminators and clear garbage

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
    /* Listening Loop */
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

            printf("%s\n", commandBuffer);
            printf("%d\n", atoi(portBuffer));
            printf("%s\n", ipBuffer);

            // Setup data socket
            datasockFD = setupSocket(atoi(portBuffer), ipBuffer);
            //send(datasockFD, test, strlen(test), 0);

            // Store directory in array
            char **dirArray = createDirArray(SIZE);
            lines = getDirectory(dirArray);

            int i = 0;
            while (i < lines)
            {
                printf("%s\n", dirArray[i]);
                i++;
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

void sendDirectory(int socketFD)
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
                send(socketFD, dir->d_name, strlen(dir->d_name), 0);
                send(socketFD, "\n", 1, 0);
                //printf("%s\n", dir->d_name);
            }
        }
        closedir(d);
    }
}

int getDirectory(char **directoryArray)
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
                strcpy(directoryArray[lines], dir->d_name);
                lines++;
            }
        }
        closedir(d);
    }
    return lines;
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

char **createDirArray(int size)
{
    char **array = malloc(size * sizeof(char *));
    int i;
    for (i = 0; i < size; i++)
    {
        array[i] = malloc(100 * sizeof(char));
        memset(array[i], '\0', sizeof(array[i]));
    }
    return array;
}