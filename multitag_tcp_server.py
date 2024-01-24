import socket
import time
import json

# Helper Functions
def get_next_key(d, current_key):
    keys = list(d.keys())
    if current_key not in keys:
        raise ValueError("The provided key is not in the dictionary.")
    current_index = keys.index(current_key)
    next_index = (current_index + 1) % len(keys)
    return keys[next_index]

def request_measurements(connection, message):
    connection.send(bytes(message, 'utf-8'))
    print(f"Sent req msg to {connection}")

def current_milli_time():
    return round(time.time() * 1000)

# TCP Server Setup
server_ip = "192.168.1.115" 
server_port = 12345

tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tcp_socket.bind((server_ip, server_port))
tcp_socket.listen(1)

print(f"TCP server listening on {server_ip}:{server_port}")

tag_address = {}
start_time = 0
previous_id = None

while True:
    connection, client_address = tcp_socket.accept()
    try:
        print(f"Connection from {client_address}")

        while True:
            data = connection.recv(1024)
            if data:
                data = json.loads(data.decode('utf-8'))
                id = data['id']
                ip, port = client_address

                if id not in tag_address.keys():
                    print(f"New tag added. ID: {id} Address: {ip}:{port}")
                    tag_address[id] = client_address
                    if len(tag_address) == 1:
                        request_measurements(connection, "1")
                        start_time = current_milli_time()
                        previous_id = id
                else:
                    print(f"Received {len(data)} bytes from {ip}:{port}")
                    print(f"Data: {data}")
                    if client_address != tag_address[id]:
                        tag_address[id] = client_address
                    end_time = current_milli_time()
                    time_taken = end_time - start_time
                    print(f"Time taken (ms): {time_taken}")
                    start_time = current_milli_time()

                    next_id = get_next_key(tag_address, id)
                    next_ip, next_port = tag_address[next_id]
                    request_measurements(connection, "1")
                    previous_id = next_id
            else:
                break
    finally:
        connection.close()
