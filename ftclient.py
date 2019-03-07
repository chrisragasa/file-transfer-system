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
    """Send message to a socket
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

def get_IP():
    """Get the local IP address
   Args:
        Local IP address
   Resources:
    https://stackoverflow.com/questions/166506/finding-local-ip-addresses-using-pythons-stdlib
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    hostname = s.getsockname()[0]
    s.close()
    return hostname


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

    # Validate correct command
    if not is_valid_command(command):
        error("error: the only commands accepted are -g or -l", 1)

    # Connect to the server socket
    sock = connect(host, port)

    # Send the command
    send_msg(sock, command)
    response = sock.recv(MAX_LENGTH)
    print(response)

    # Send the port number
    send_msg(sock, data_port)
    response = sock.recv(MAX_LENGTH)
    print(response)

    # Send client IP address
    send_msg(sock, get_IP())
    response = sock.recv(MAX_LENGTH)
    print(response)

    # Create socket (SOCK_STREAM means a TCP socket)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("", int(data_port)))
    s.listen(1)

    # accept() returns conn (socket object) and addr (address bound to socket on other end of conn)
    conn, addr = s.accept()
    print("Established connection with " + str(addr) + "...")
    '''
    # line = conn.recv(MAX_LENGTH).decode('UTF-8').rstrip('\x00')
    # format from buffer to integer
    dir_lines_count = int(conn.recv(MAX_LENGTH).decode('UTF-8').rstrip('\x00'))
    print("lines count: " + str(dir_lines_count))
    line = conn.recv(MAX_LENGTH).decode('UTF-8').rstrip('\x00')
    print(line)

    count = 0
    while count < dir_lines_count:
        line = conn.recv(MAX_LENGTH).decode('UTF-8').rstrip('\x00')
        print(line)
        count += 1
    '''
    conn.close()


if __name__ == "__main__":
    main()
