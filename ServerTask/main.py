import threading
from socket import *
from flask import Flask, render_template

app = Flask(__name__)

host = "127.0.0.1"
port = 1111
addr = (host, port)

connect_numbers = 0
clients = {}

@app.route('/')
def hello():
    return render_template("index.html", clients=clients, user_nums=connect_numbers)


# Socket Server Function
def run_socket_server():
    global connect_numbers
    host = "127.0.0.1"
    port = 1111
    addr = (host, port)
    Connection = socket(AF_INET, SOCK_STREAM)
    Connection.bind(addr)
    Connection.listen(100)

    while True:
        if connect_numbers == 0:
            print("wait connection...")

        print(f"Connections: {connect_numbers}")

        conn, addr = Connection.accept()
        print("addr client: ", addr)
        connect_numbers += 1

        conn.send(b"Hello from server")

        while True:
            try:
                data = conn.recv(16024)
                if not data:
                    break
                else:
                    if data == "heartbeat":
                        clients[addr[0]]['last_activity'] += 1
                    elif data.startswith(b"user:"):
                        clients[addr[0]] = {"user_data" : data}
                    print(data)
            except ConnectionResetError:
                print("Соеденение с клиентом прервано")
                break
        conn.close()
        connect_numbers -= 1

    Connection.close()


# Flask Server Function
def run_flask_server():
    app.run(debug=True, port=5000)


# Start Threads
socket_thread = threading.Thread(target=run_socket_server)

socket_thread.start()

# Start Flask server after socket server is running
run_flask_server()
