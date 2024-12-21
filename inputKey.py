import win32gui
import win32con
import win32api
import time
import socket
import json 
HOST = "192.168.199.29"
PORT = 8080

def find_window(title):
    # 특정 프로그램 창의 핸들 가져오기
    hwnd = win32gui.FindWindow(None, title)
    if hwnd == 0:
        print(f"Window with title '{title}' not found!")
        return None
    return hwnd

def send_key_to_window(hwnd, key):
    # 창 활성화
    win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
    win32gui.SetForegroundWindow(hwnd)

    # 키 입력 시뮬레이션
    vk_code = ord(key.upper())  # 'w'의 가상 키코드
    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk_code, 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_KEYUP, vk_code, 0)

if __name__ == "__main__":
    # 프로그램 창 제목 (작업 관리자에서 확인 가능)
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((HOST, PORT))
        print(f"Connected to server at {HOST}:{PORT}")
        initial_data = {"class": 5}
        client_socket.sendall(json.dumps(initial_data).encode())
        print("Sent initial data: {\"class\": 5}")
        data = client_socket.recv(1024)
        print(f"Received from server: {data.decode()}")

        
        window_title = "WebCam"  # 예: 메모장 창 제목
        hwnd = find_window(window_title)
        print("find window")
        while(1):
            if client_socket.recv(1024):
                print(f"Sending 'W' to '{window_title}'...")
                send_key_to_window(hwnd, 'w')
