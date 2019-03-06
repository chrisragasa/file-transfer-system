'''
Author: Christopher Ragasa
Date: Mar 5, 2019
Program Description: ftclient.py is the client side of the file transfer system.
'''
import socket
import sys

MAX_LENGTH = 1024


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
    print("Connected to " + str(host) + " on port " + str(port))
    return s


def send_msg(sock, message):
    """Send string to a socket
   Args:
        sock: socket used to transmit the message
        message: message to be transmitted
    """
    sock.send(message.encode('utf-8'))


def is_valid_command(command):
    """Establish connection with the server
   Args:
        command: command passed by the user as an argument
   Returns:
        True if valid, False otherwise
    """
    return command == "-g" or command == "-l"


# def send_command(sock, command, data_port):


def main():
    # Validate correct usage
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        error("error: incorrect usage\nusage: python3 ftclient.py <server_host> <server_port> <command> <filename> <data_port>", 1)

    host = sys.argv[1]
    port = int(sys.argv[2])
    command = sys.argv[3]

    if len(sys.argv) == 5:
        data_port = int(sys.argv[4])

    elif len(sys.argv) == 6:
        filename = sys.argv[4]
        data_port = int(sys.argv[5])

    # Validate correct command
    if not is_valid_command(command):
        error("error: the only commands accepted are -g or -l", 1)

    # Connect to the server socket
    server_socket = connect(host, port)
    send_msg(server_socket, "testing")


if __name__ == "__main__":
    main()
