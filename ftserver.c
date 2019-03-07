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
void readFile(char *fileName, char *buff);
void getClientCommandLine(char *cBuff, char *pBuff, char *ipBuff, char *fBuff, int sz, int sock);
void sendData(int sock, char *data, int sz);
void validateArgs(int argc, char *argv[]);
int setupSocket(int portNumber, char *hostname);
int getCommand(char *commandBuffer);
int getDirectory(char *directoryStr);
int isValidFile(char *fileStr);
int initSocket(int port);

int main(int argc, char *argv[])
{
    int socketFD, newsocketFD, datasockFD, pid;
    char commandBuffer[SIZE];
    char portBuffer[SIZE];
    char ipBuffer[SIZE];
    char fileBuffer[SIZE];
    char dirStr[LARGE_SIZE];
    char fileStr[LARGE_SIZE];

    // Validate correct usage (user arguments)
    validateArgs(argc, argv);

    // Socket initialization
    socketFD = initSocket(atoi(argv[1]));

    // Listening Loop
    listen(socketFD, 5);
    while (1)
    {
        // Creates a new connected socket, accept() returns it's new file descriptor
        newsocketFD = accept(socketFD, NULL, NULL);
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

            // Setup data transfer socket
            datasockFD = setupSocket(atoi(portBuffer), ipBuffer);

            // Send directory contents to client
            if (getCommand(commandBuffer) == 1)
            {
                memset(dirStr, '\0', LARGE_SIZE);
                getDirectory(dirStr);
                sendData(datasockFD, dirStr, LARGE_SIZE);
            }
            // Transfer file to client
            else if (getCommand(commandBuffer) == 2)
            {
                // If file exists, send file to client
                if (isValidFile(fileBuffer))
                {
                    memset(fileStr, '\0', LARGE_SIZE);
                    readFile(fileBuffer, fileStr);
                    sendData(datasockFD, fileStr, LARGE_SIZE);
                }
                // Otherwise, send file not found error to client
                else
                {
                    memset(fileStr, '\0', LARGE_SIZE);
                    strcpy(fileStr, "File not found.");
                    sendData(datasockFD, fileStr, LARGE_SIZE);
                }
            }
            close(newsocketFD);
            close(datasockFD);
            close(socketFD);
            exit(0);
        }
        else
        {
            close(newsocketFD);
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
 *   Gets the current working directory and copies it to a buffer
 * 
 *   char: buffer where directory contents will be copied to
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
 *   Checks if the working directory contains the file passed as an argument
 * 
 *   fileStr: filename
 * 
 *   returns: 1 if the working directory contains the file, 0 otherwise
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
 *   Setup socket for data connection with client
 *
 *   portNumber: client port number (for data transfer)
 *   hostname: client host name   
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

    // Connect to server
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
 *   commandBuffer: buffer where command is stored
 *
 *   returns: 1 if directory listing cmd, 2 if file transfer cmd, -1 otherwise
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
 *   Reads the contents of a file into a buffer
 *
 *   fileName: file name
 *   buff: buffer where file contents will be stored  
 *
 *   post-conditions: the contents of the file will be copied into buffer
 */
void readFile(char *fileName, char *buff)
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

    strcpy(buff, buffer);
}

/*
 * Function: getClientCommandLine
 * ---------------------------- 
 *   Reads the arguments from the client command line
 *
 *   cBuff: command buffer
 *   pBuff: port number (for data transfer) buffer
 *   ipBuff: IP address buffer
 *   fBuff: filename buffer
 *   sz: buffer size
 *   sock: socket file descriptor
 *
 *   post-conditions: client command line arguments are stored into their server side buffers
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

/*
 * Function: sendData
 * ---------------------------- 
 *   Send data from server to client
 *
 *   sock: socket file descriptor
 *   data: buffer where data to be transfer is stored
 *   sz: buffer size
 *
 *   post-conditions: data is sent from server to client
 */
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
    printf("Data sent to client...\n");
}

/*
 * Function: initSocket
 * ---------------------------- 
 *   Create server socket
 *
 *   port: port number (defined on server side) 
 *
 *   returns: socket file descriptor
 */
int initSocket(int port)
{
    int s;
    struct sockaddr_in serverAddress;
    s = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (s < 0)
    { // Check for socket creation error
        error("error: server couldn't open the socket", 1);
    }
    memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    serverAddress.sin_family = AF_INET;                          // Create a network-capable socket
    serverAddress.sin_addr.s_addr = INADDR_ANY;                  // Any address is allowed for connection to this process
    serverAddress.sin_port = htons(port);                        // Store the port number
    if (bind(s, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    { // Check for bind error
        error("error: server couldn't bind", 0);
    }
    return s;
}

/*
 * Function: validateArgs
 * ---------------------------- 
 *   Validate correct program usage
 *
 *   argc: C command line arguments (number of arguments passed)
 *   argv: C command line arguments (pointer array which points to each argument passed to program)
 * 
 *   post-conditions: error thrown and program exits if usage is invalid
 */
void validateArgs(int argc, char *argv[])
{
    if (argc != 2)
        error("error: incorrect usage\nusage: ftserver <port number>", 1);

    if (atoi(argv[1]) < 1024 || atoi(argv[1]) > 65535)
        error("error: use a port number within range [1024, 65535]", 1);
}