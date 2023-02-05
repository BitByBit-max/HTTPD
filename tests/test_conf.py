import subprocess as sp
import requests
import socket
import time
from http.client import HTTPResponse

def launch_server(args, configs):
    http_proc = sp.Popen(["./httpd"] + args + configs)
    time.sleep(0.5)
    return http_proc

def test_dry_run():
    http_proc = launch_server(["--dry-run"], ["src/server.conf"])
    res = http_proc.wait()
    assert 0 == res

def test_request():
    httpd_proc = launch_server([], ["src/server.conf"])
    ip = "0.0.0.0"
    port = "4243"
    response = requests.head(f"http://{ip}:{port}/")
    assert 200 == response.status_code

def send_head(ip, port, target):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((ip, int(port)))
    s.send(f"HEAD {target} HTTP/1.1\r\nHost:{ip}:{port}\r\nContent_Length:{ip}:\r\n\r\n".encode())
    return s

def test_socket():
    httpd_proc = launch_server([], ["src/server.conf"])
    ip = "0.0.0.0"
    port = "4243"
    s = send_head(ip, port, "/")
    response = HTTPResponse(s)
    response.begin()
    assert response.status == 200

def send_get_segmented(ip, port, target):
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.connect((ip, int(port)))
      s.send(f"GET {target} HTTP/1.1\r\n".encode())
      time.sleep(0.5)
      s.send(f"Host:{ip}:{port}\r\n".encode())
      time.sleep(0.5)
      s.send(f"Content_Length: 0\r\n\r\n".encode())
      return s

def test_epoll():
    httpd_proc = launch_server([], ["src/server.conf"])
    ip = "0.0.0.0"
    port = "4243"
    s = send_get_segmented(ip, port, "/")
    response = HTTPResponse(s)
    response.begin()
    time.sleep(0.5)
    assert response.status == 200
