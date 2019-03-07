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
void readFile(char *fileName, char *string);
void getClientCommandLine(char *cBuff, char *pBuff, char *ipBuff, char *fBuff, int sz, int sock);
void sendData(int sock, char *data, int sz);

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
    memset(dirStr, '\0', LARGE_SIZE);
    memset(fileStr, '\0', LARGE_SIZE);

    if (argc < 2)
        error("error: Incorrect number of arguments.\nSYNTAX: ftserver <port number>", 1);

    //Port and Socket Setup
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    portNumber = atoi(argv[1]);                 // Get the port number
    if (socketFD < 0)
    { // Check for socket creation error
        error("error: server couldn't open the socket", 1);
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
            // Get client command line arguments
            getClientCommandLine(commandBuffer, portBuffer, ipBuffer, fileBuffer, SIZE, newsocketFD);

            // Setup data socket
            datasockFD = setupSocket(atoi(portBuffer), ipBuffer);

            // Send directory contents to client
            if (getCommand(commandBuffer) == 1)
            {
                getDirectory(dirStr);
                sendData(datasockFD, dirStr, LARGE_SIZE);
            }
            else if (getCommand(commandBuffer) == 2)
            {
                // If file exists, send file to client
                if (isValidFile(fileBuffer))
                {
                    readFile(fileBuffer, fileStr);
                    sendData(datasockFD, fileStr, LARGE_SIZE);
                }
                // Otherwise, send file not found error to client
                else
                {
                    strcpy(fileStr, "File not found.");
                    sendData(datasockFD, fileStr, LARGE_SIZE);
                }
            }

            close(newsocketFD);
            close(datasockFD);
            close(socketFD);
            exit(0); // Child dies
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
 *   post-conditions: exits the program with status value
 */
void error(const char *msg, int exitVal)
{
    fprintf(stderr, "%s\n", msg);
    exit(exitVal);
}

/*
 * Function: getDirectory
 * ---------------------------- 
 *   Gets the current working directory and copies it to a string
 * 
 *   char: buffer/string where the directory contents will be copied to
 * 
 *   returns: number of files in the working directory
 */
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

/*
 * Function: isValidFile
 * ---------------------------- 
 *   Checks if the working directory contains a file
 * 
 *   fileStr: name of the file
 * 
 *   returns: 1 if the working dir contains the file, 0 otherwise
 */
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

/*
 * Function: setupSocket
 * ---------------------------- 
 *   Setup socket for data connection
 *
 *   portNumber: port number
 *   hostname: host name   
 *
 *   returns: socket file descriptor 
 */
int setupSocket(int portNumber, char *hostname)
{
    int socketFD;                                     // Socket file descriptor
    struct sockaddr_in serverAddress;                 // Server address struct
    struct hostent *server = gethostbyname(hostname); // Get host IP
    if (server == NULL)
        error("error: host was not found", 1);

    // Open the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Returns a socket descriptor that we can use in later system calls. returns -1 on error.
    if (socketFD < 0)
    {
        error("error: opening socket", 1);
    }

    // Set up server address struct
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

/*
 * Function: getCommand
 * ---------------------------- 
 *   Get the command type passed from the client
 *
 *   commandBuffer: buffer where the command is stored
 *
 *   returns: socket file descriptor 
 */
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

/*
 * Function: readFile
 * ---------------------------- 
 *   Reads the contents of a file into a string
 *
 *   fileName: file name
 *   string: string where the file contents will be stored  
 *
 *   post-conditions: the contents of the file will be copied into string
 */
void readFile(char *fileName, char *string)
{
    char *buffer = 0;
    long length;
    FILE *f = fopen(fileName, "rb");

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer)
        {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }

    strcpy(string, buffer);
}

/*
 * Function: getClientCommandLine
 * ---------------------------- 
 *   Reads the arguments of command line from client
 *
 *   fileName: file name
 *   string: string where the file contents will be stored  
 *
 *   post-conditions: the contents of the file will be copied into string
 */
void getClientCommandLine(char *cBuff, char *pBuff, char *ipBuff, char *fBuff, int sz, int sock)
{
    char *confirm = "OK";
    int bytesRecv;

    //Receive the command
    memset(cBuff, '\0', sz);
    bytesRecv = recv(sock, cBuff, sz - 1, 0);
    if (bytesRecv < 0)
    {
        error("error: server can't read from the socket", 1);
    }
    send(sock, confirm, strlen(confirm), 0);

    //Receive the port number for data transfer
    memset(pBuff, '\0', sz);
    bytesRecv = recv(sock, pBuff, sz - 1, 0);
    if (bytesRecv < 0)
    {
        error("error: server can't read from the socket", 1);
    }
    send(sock, confirm, strlen(confirm), 0);

    //Receive the client's IP address
    memset(ipBuff, '\0', sz);
    bytesRecv = recv(sock, ipBuff, sz - 1, 0);
    if (bytesRecv < 0)
    {
        error("error: server can't read from the socket", 1);
    }
    send(sock, confirm, strlen(confirm), 0);

    //Receive the file name
    if (getCommand(cBuff) == 2)
    {
        memset(fBuff, '\0', sz);
        bytesRecv = recv(sock, fBuff, sz - 1, 0);
        if (bytesRecv < 0)
        {
            error("error: server can't read from the socket", 1);
        }
        send(sock, confirm, strlen(confirm), 0);
    }
    printf("Data received from client...\n");
    printf("    IP buffer: %s\n", cBuff);
    printf("    port buffer: %d\n", atoi(pBuff));
    printf("    file buffer: %s\n", fBuff);
    printf("    command buffer: %s\n", cBuff);
    printf("    command code: %d\n", getCommand(cBuff));
}

void sendData(int sock, char *data, int sz)
{
    int bytesSent, charsText;
    bytesSent = 0;
    charsText = send(sock, data, sz - 1, 0);
    bytesSent = bytesSent + charsText;
    if (charsText < 0)
    {
        error("error: server can't send encryption to socket", 1);
    }
    while (bytesSent < sz - 1)
    {
        charsText = send(sock, &data[bytesSent], sz - (bytesSent - 1), 0); // Send the bytes that haven't been sent yet
        bytesSent = bytesSent + charsText;
    }
}