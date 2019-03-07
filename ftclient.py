'''
Author: Christopher Ragasa
Date: Mar 5, 2019
Program Description: ftclient.py is the client side of the file transfer system.
'''
import socket
import sys
import os

MAX_LENGTH = 1024
CHUNK_SIZE = 1024


def error(msg, value):
    """Utility function that handles errors
   Args:
       msg: error message
       value: exit value
   Returns:
       N/A
    """
    print(msg)
    exit(value)


def connect(host, port):
    """Establish connection with the server
   Args:
        host: server hostname
        port: server port number
   Returns:
        socket object
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((host, port))
    except:
        error("error: error connecting to port " + str(port), 1)
    print("Connected to " + str(host) + " on port " + str(port) + "...")
    return s


def send_msg(sock, message):
    """Send message to a socket
   Args:
        sock: socket used to transmit the message
        message: message to be transmitted
    """
    sock.send(message.encode('utf-8'))

# def send_command(sock, command, data_port):


def get_IP():
    """Get the local IP address
   Returns:
        local IP address
   Resources:
    https://stackoverflow.com/questions/166506/finding-local-ip-addresses-using-pythons-stdlib
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    hostname = s.getsockname()[0]
    s.close()
    return hostname


def validate_arguments(h, p, c, d):
    """Validate user arguments
   Args:
        h: hostname
        p: port number
        c: command
        d: data port number
        f: filename
    """
    # Validate hostname
    hostnames = ["flip1.engr.oregonstate.edu",
                 "flip2.engr.oregonstate.edu",
                 "flip3.engr.oregonstate.edu",
                 "localhost"]
    if h not in hostnames:
        error("error: invalid hostname - an OSU flip server must be used", 1)

    if c != '-l' and c != '-g':
        error("error: only commands '-l' and '-g' are allowed", 1)

    # Validate port number
    if int(p) < 1024 or int(p) > 65535:
        error("error: use a port number within range [1024, 65535]", 1)

    # Validate data port number
    if int(d) < 1024 or int(d) > 65535:
        error("error: use a port number within range [1024, 65535]", 1)


def main():

    # Validate correct usage
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        error("error: incorrect usage\nusage: python3 ftclient.py <server_host> <server_port> <command> <filename> <data_port>", 1)

    host = sys.argv[1]
    port = int(sys.argv[2])
    command = sys.argv[3]

    if len(sys.argv) == 5:
        data_port = sys.argv[4]

    elif len(sys.argv) == 6:
        filename = sys.argv[4]
        data_port = sys.argv[5]

    validate_arguments(host, port, command, data_port)

    # Connect to the server socket
    sock = connect(host, port)

    # send the command
    send_msg(sock, command)
    response = sock.recv(MAX_LENGTH)
    if not response:
        error("error: no ack received", 1)

    # send the port number
    send_msg(sock, data_port)
    response = sock.recv(MAX_LENGTH)
    if not response:
        error("error: no ack received", 1)

    # send client IP address
    send_msg(sock, get_IP())
    response = sock.recv(MAX_LENGTH)
    if not response:
        error("error: no ack received", 1)

    # send the filename
    if command == "-g" and filename:
        send_msg(sock, filename)
        response = sock.recv(MAX_LENGTH)
        if not response:
            error("error: no ack received", 1)

    # create socket (SOCK_STREAM means a TCP socket)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("", int(data_port)))
    s.listen(1)

    # accept() returns conn (socket object) and addr (address bound to socket on other end of conn)
    conn, addr = s.accept()
    print("Data transfer initiated on " + str(addr) + "...")
    data = ""
    while True:
        # reads data chunk from the socket in batches using method recv() until it returns an empty string
        datachunk = conn.recv(CHUNK_SIZE)
        if not datachunk:
            break  # no more data coming in, so break out of the while loop
        # add chunk to your already collected data
        data += datachunk.decode('UTF-8')
    data = data.rstrip('\x00')

    # print the remote directory
    if command == "-l":
        print(data)

    # transfer the file from remote directory to local
    elif command == "-g":
        # validate file exists on server side
        if data == "File not found.":
            print(data)
        else:
            if os.path.exists(filename):
                ext = 1
                while os.path.exists(filename + str(ext)):
                    ext += 1
                fh = open(filename + str(ext), "w")
                fh.write(data)
                fh.close()
            else:
                fh = open(filename, "w")
                fh.write(data)
                fh.close()
            print("Transfer complete.")

    conn.close()


if __name__ == "__main__":
    main()
