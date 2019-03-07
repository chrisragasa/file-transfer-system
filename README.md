# file-transfer-system

This program is a simple file transfer system, i.e., create a file transfer server and a file transfer client.

The objectives of this program are to:

1. Implement 2-connection client-server network application
2. Practice using the `sockets` API
3. Refresh programming skills in Python and C

Software Requirements:

1. Server must be written in C
2. Client must be written in Python
3. Programs must run on an OSU flip server
4. Program can not use `sendfile` or any other predefined function that makes this trivial
5. Program should be able to send a complete text file -- not required to handle "out of memory" error

## Compile

In project directory, ensure proper permissions are set on `compileall`:

```bash
$ chmod +x compileall
```

To compile, run the following script in the project directory:

```bash
$ ./compileall
```

## Usage

### 1) Starting the server

```bash
$ ./ftserver <port number>
```

### 2) Using the client

```bash
$ python3 ftclient.py <server_host> <server_port> <command> <filename> <data_port>
```

## Example Program Usage

#### Start the server on flip1.engr.oregonstate.edu:3555

```bash
flip1 ~/file-transfer-system $ ./ftserver 3555
```

#### Using the client in a different directory on flip2.engr.oregonstate.edu, list the server's directory

```bash
flip2 ~/test-dir $ python3 ftclient.py flip1.engr.oregonstate.edu 3555 -l 8555
Connected to flip1.engr.oregonstate.edu on port 3555...
Data transfer initiated on ('128.193.54.168', 41184)...
.
..
README.md
compileall
ftserver.c
ftserver
ftclient.py
textfile-small
textfile-large
```

#### Using the client in a different directory on flip2.engr.oregonstate.edu, get textfile-large from the server's directory

```bash
flip2 ~/test-dir $ ls
ftclient.py

flip2 ~/test-dir $ python3 ftclient.py flip1.engr.oregonstate.edu 3555 -g textfile-large 8555
Connected to flip1.engr.oregonstate.edu on port 3555...
Data transfer initiated on ('128.193.54.168', 41900)...
Transfer complete.

flip2 ~/test-dir $ ls
ftclient.py textfile-large
```

## Resources

- http://docs.python.org/release/2.6.5/library/internet.html
- https://docs.python.org/3/library/socket.html
- https://beej.us/guide/bgnet/html/multi/index.html
- https://stackoverflow.com/questions/53285659/how-can-i-wait-until-i-receive-data-using-a-python-socket
