import socket
import subprocess
import time
import random
import sqlite3
import string
from configparser import ConfigParser
INTEGER_OVERFLOW = "INTEGER_OVERFLOW"
INTEGER_TYPE_ERROR = "INTEGER_TYPE_ERROR"
INVALID_FORMAT = "INVALID_FORMAT"
SUCCESS = "SUCCESS"
def get_random_string(length):
    letters = string.ascii_lowercase
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str
def send_message(host, port, message,prefix):
    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((host, port))
        client_socket.send((prefix + message).encode("utf8"))
        rec = client_socket.recv(1024)
        client_socket.close()
        return rec.decode()
    except Exception as e:
        print(f"Error connecting to the server: {e}")
server_host = "localhost"
configure = ConfigParser()
configure.read("../config/fconfig.ini")
server_port = configure.getint('address','port')
conn = sqlite3.connect('../db/messages.db')
#first section test : 
#making every table empty
cur = conn.cursor()
cur.execute("DELETE FROM MESSAGES;")
cur.execute("DELETE FROM NUMBERS;")
conn.commit()

#going to send some integer and text message to the server 
try:#starting the server program excutable file :
    c_server_process = subprocess.Popen(["../build/server"], stdout=subprocess.PIPE, universal_newlines=True, start_new_session=True)
    time.sleep(2)#wait for the server to stablish its connection and etc
except Exception as e:
    print(f"Error running the C server: {e}")
    
#testing true examples
test_messages_number = []
test_messages_text = []
number_of_tests = 1000
for i in range(number_of_tests):
    test_messages_number.append((random.randint(1,999999999)))
for i in range(number_of_tests):
    test_messages_text.append(get_random_string(random.randint(1,20)))
#sending all test requests
for message in test_messages_number:
    red = send_message(server_host, server_port, str(message),configure.get('prefix','p1') )
    print(f"Sent message to server: {message} , server reply : {red}")
    assert red == SUCCESS #checking each connection response
for message in test_messages_text:
    red = send_message(server_host, server_port, message,configure.get('prefix','p2'))
    print(f"Sent message to server: {message} , server reply : {red}")
    assert red == SUCCESS
#checking in the database 
res = cur.execute("SELECT count(*) FROM NUMBERS;")
assert res.fetchone()[0] == number_of_tests
res = cur.execute("SELECT count(*) FROM MESSAGES;")
assert res.fetchone()[0] == number_of_tests
res_number = cur.execute("SELECT (message) FROM NUMBERS")
it = 0
for row in res_number:
    assert row[0] == test_messages_number[it]
    it=it+1
it =0
res_message = cur.execute("SELECT (message) FROM MESSAGES")
for row in res_message:
    assert row[0] == test_messages_text[it]
    it+=1
#wrong format insertion
res = send_message(server_host,server_port,"test message","wrongprefix")
assert res == INVALID_FORMAT
#long integer insertion
res = send_message(server_host , server_port,"12345678910",configure.get('prefix','p1'))
assert res == INTEGER_OVERFLOW
#wrong integer type 
res = send_message(server_host,server_port,"435f435",configure.get('prefix','p1'))
assert res == INTEGER_TYPE_ERROR
c_server_process.terminate()#stop the server or it will run forever
c_server_process.wait()
cur.close()
conn.close()

