import socket
import subprocess
import time

def send_message(host, port, message):
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((host, port))
        client_socket.send(message.encode("utf8"))
        client_socket.close()
    except Exception as e:
        print(f"Error connecting to the server: {e}")

server_host = "localhost"
server_port = 3030

try:
    c_server_process = subprocess.Popen(["./server"], stdout=subprocess.PIPE, universal_newlines=True, start_new_session=True)
    time.sleep(1)
except Exception as e:
    print(f"Error running the C server: {e}")

test_messages = [
    "fup45556",
    "fup122774",
    "fup7898981",
    "fup34895",
]

for message in test_messages:
    send_message(server_host, server_port, message)
    print(f"Sent message to server: {message}")
    time.sleep(1) 


time.sleep(4)

c_server_process.terminate()
c_server_process.wait()

print("C Server Output:")
for line in c_server_process.stdout:
    print(line.strip())
