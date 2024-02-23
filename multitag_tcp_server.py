import socket
import time
import json
import threading
from collections import defaultdict

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

def handle_connection(connection, client_address, last_update_timestamp, lock):
    connection.settimeout(5)
    last_timestamp = None
    print(f"Connection from {client_address}")
    data = connection.recv(1024)
    if not data:
        print("No data received.")
        connection.close()
        return
    id = json.loads(data.decode('utf-8'))['id']

    while True:
        with lock:
            try:
                connection.sendall(bytes("1", 'utf-8')) # request data
                data = connection.recv(1024)
            except:
                print(f"Timeout occurred by {id}.")
                connection.close()
                return
            if not data:
                break
            
            data = json.loads(data.decode('utf-8'))
            id = data['id']
            current_timestamp = time.time()
            last_timestamp = last_update_timestamp[id]
            time_difference = (current_timestamp - last_timestamp)
            frequency = 1 / time_difference

            print(f"Update frequency for ID {id}: {frequency} Hz")
            print(f"Data: {data}")

            last_update_timestamp[id] = current_timestamp

# TCP Server Setup
server_ip = "192.168.0.219" 
server_port = 12345

tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tcp_socket.bind((server_ip, server_port))
tcp_socket.listen(5)  # Listen for multiple connections

print(f"TCP server listening on {server_ip}:{server_port}")

last_update_timestamp = defaultdict(int)
lock = threading.Lock()

try:
    while True:
        connection, client_address = tcp_socket.accept()
        client_thread = threading.Thread(target=handle_connection, args=(connection, client_address, last_update_timestamp, lock))
        client_thread.start()
except KeyboardInterrupt:
    print("Server is shutting down.")
finally:
    tcp_socket.close()
