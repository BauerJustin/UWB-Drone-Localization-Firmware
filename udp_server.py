import socket

# Define the UDP server's IP address and port
server_ip = "192.168.0.212"  # Listen on all available network interfaces
server_port = 12345  # Replace with the port you want to listen on

# Create a UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the specified IP address and port
udp_socket.bind((server_ip, server_port))

print(f"UDP server listening on {server_ip}:{server_port}")

while True:
    data, client_address = udp_socket.recvfrom(1024)  # Adjust the buffer size as needed
    print(f"Received {len(data)} bytes from {client_address[0]}:{client_address[1]}")
    print(f"Data: {data.decode('utf-8')}")

# Close the socket when done (usually, you would use a signal or exception to exit the loop gracefully)
udp_socket.close()
