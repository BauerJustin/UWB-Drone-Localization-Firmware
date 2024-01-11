"""

"""

import socket
import time
# Helper Functions

# get next key of the dict, returns the first key if it reaches the end
def get_next_key(d, current_key):
    keys = list(d.keys())
    if current_key not in keys:
        raise ValueError("The provided key is not in the dictionary.")
    current_index = keys.index(current_key)
    next_index = (current_index + 1) % len(keys)
    return keys[next_index]

def request_measurements(socket, ip, port):
    addr = (ip, port)
    msg = "1"
    socket.sendto(msg, addr)
    print(f"Sent req msg to {ip}:{port}")

def current_milli_time():
    return round(time.time() * 1000)

# Define the UDP server's IP address and port
server_ip = "192.168.0.219" 
server_port = 12345  # Replace with the port you want to listen on

# Create a UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the specified IP address and port
udp_socket.bind((server_ip, server_port))

print(f"UDP server listening on {server_ip}:{server_port}")

tag_address = {} # maps ip to port
start_time = 0

while True:
    data, (ip, port) = udp_socket.recvfrom(1024)

    # add to tag list if we haven't seen this tag before
    if ip not in tag_address.keys():
        print(f"New tag added. Address: {ip}:{port}")
        tag_address[ip] = port
        if len(tag_address) is 1:
            request_measurements(udp_socket, ip, port)
            start_time = current_milli_time()
    # have seen this before, and just got back a packet.
    else: 
        print(f"Received {len(data)} bytes from {ip}:{port}")
        print(f"Data: {data.decode('utf-8')}")
        end_time = current_milli_time()
        time_taken = end_time - start_time
        print(f"Time taken (ms): {time_taken}")
        start_time = current_milli_time()

        next_ip = get_next_key(tag_address, ip)
        next_port = tag_address[next_ip]

        request_measurements(udp_socket, next_ip, next_port)
