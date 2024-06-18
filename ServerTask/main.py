import threading
from socket import *
from flask import Flask, render_template, jsonify
import queue
import time

app = Flask(__name__)

host = "127.0.0.1"
port = 1111
addr = (host, port)

connect_numbers = 0
machine = []
ip = []
last_activity = 0
picture_size = ""

data_queue = queue.Queue()


@app.route('/')
def hello():
    global connect_numbers, machine, ip, last_activity
    if not data_queue.empty():
        data = data_queue.get()
        print(data)
        connect_numbers = data['user_nums']
        ip = data['ip']
        machine = data['machine']
        last_activity = data['last_activity']
    return render_template("index.html", ip=ip, machine=machine, last_activity=last_activity,
                           user_nums=connect_numbers)


@app.route("/get_data")
def get_data():
    global connect_numbers, machine, ip, last_activity
    try:
        data = data_queue.get(block=True, timeout=1)
    except queue.Empty:
        data = {"user_nums": 0, "ip": [], "machine": [], "last_activity": 0}
    return jsonify(data)


# Socket Server Function
def run_socket_server():
    global connect_numbers, machine, ip, last_activity, picture_size
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
                    print(data)
                    if data == b"heartbeat":
                        last_activity += 1
                    elif data.startswith(b"machine:"):
                        machine.append(data[8:].decode())
                    elif data.startswith(b"ip:"):
                        if data == b"ip:heartbeat":
                            pass
                        else:
                            ip.append(data[3:].decode())
                    elif data.startswith(b"sizePic:"):
                        picture_size = data
                        size_bytes = picture_size[8:]
                        picture_size = int(size_bytes)
                    elif data.startswith(b"screenshot:"):
                        receive_data = b""
                        while len(receive_data) < int(picture_size):
                            dat = conn.recv(1024)
                            if not dat:
                                break
                            receive_data += dat
                        with open("screenshot.bmp", "wb") as f:
                            f.write(receive_data)
                        print("Изображение сохранено в файл screenshot.bmp")
                    if data:
                        data_queue.put({
                            "ip": ip,
                            "machine": machine,
                            "last_activity": last_activity,
                            "user_nums": connect_numbers
                        })

                    print("Данные добавлены в очередь:", data_queue.qsize())
            except ConnectionResetError:
                print(*machine, *ip, last_activity)
                print("Соеденение с клиентом прервано")
                break

        time.sleep(0.1)

        conn.close()
        connect_numbers -= 1

    Connection.close()


def run_flask_server():
    app.run(debug=True, port=5000)


socket_thread = threading.Thread(target=run_socket_server)

socket_thread.start()

run_flask_server()
