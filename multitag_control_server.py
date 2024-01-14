"""
Prototype server that handles multitag.

Tags needs to send an initial packet to register itself with the server.
"""

import socket
import time
import json
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
    msg = bytes("1", 'utf-8') # or whatever message
    socket.sendto(msg, addr)
    print(f"Sent req msg to {ip}:{port}")

def current_milli_time():
    return round(time.time() * 1000)

# Define the UDP server's IP address and port
server_ip = "192.168.0.212" 
server_port = 12345  # Replace with the port you want to listen on

# Create a UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the specified IP address and port
udp_socket.bind((server_ip, server_port))
udp_socket.settimeout(1)

print(f"UDP server listening on {server_ip}:{server_port}")

tag_address = {} # maps ip to port
start_time = 0

while True:
    try:
        data, (ip, port) = udp_socket.recvfrom(1024)
    except socket.timeout:
        print("Trying again!")
        continue
    
    if not data:
        continue

    data = json.loads(data.decode('utf-8'))
    id = data['id']
    # add to tag list if we haven't seen this tag before
    if id not in tag_address.keys():  
        print(f"New tag added. ID: {id} Address: {ip}:{port}")
        tag_address[id] = (ip,port)
        if len(tag_address) == 1:
            request_measurements(udp_socket, ip, port)
            start_time = current_milli_time()
    # have seen this before, and just got back a packet.
    else: 
        print(f"Received {len(data)} bytes from {ip}:{port}")
        print(f"Data: {data}")
        end_time = current_milli_time()
        time_taken = end_time - start_time
        print(f"Time taken (ms): {time_taken}")
        start_time = current_milli_time()

        next_id = get_next_key(tag_address, id)
        next_ip, next_port = tag_address[next_id]

        request_measurements(udp_socket, next_ip, next_port)
