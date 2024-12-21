import tensorflow as tf
from tensorflow.keras.models import load_model
import cv2
import numpy as np
import time
import socket
import json

# 모델 로드
print("Loading the model...")
model = load_model("best_model.h5")
print("Model loaded successfully.")

# 서버 연결 설정
HOST = "192.168.199.29"
PORT = 8080

def predict_number(model, img):
    # MNIST 모델 입력 크기(28x28)로 리사이즈
    img_resized = cv2.resize(img, (28, 28))
    img_resized = img_resized / 255.0  # 정규화
    img_reshaped = img_resized.reshape(1, 28, 28)  # 모델 입력 형태로 변경

    # 모델 예측
    prediction = model.predict(img_reshaped, verbose=0)
    predicted_number = np.argmax(prediction)  # 가장 높은 확률의 숫자 선택
    return predicted_number, img_resized  # 예측 결과와 전처리된 이미지 반환


print("1")
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
    print("2")
    client_socket.connect((HOST, PORT))
    print("3")
    print(f"Connected to server at {HOST}:{PORT}")
    initial_data = {"class":3 }
    client_socket.sendall(json.dumps(initial_data).encode())
    print("3")
    print("Sent initial data: {\"class\": 3}")
    # 웹캠 설정
    cap = cv2.VideoCapture(0)
    saved_number = None  # 저장된 숫자를 담는 변수

    while True: 
        ret, frame = cap.read()
        if not ret:
            print("Failed to grab frame from webcam.")
            break

        # 프레임 전처리
        # gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)  # 흑백 변환
        # blurred = cv2.GaussianBlur(gray, (5, 5), 0)  # 가우시안 블러로 노이즈 제거
        # _, thresh = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)  # Otsu 이진화

        # height, width = thresh.shape
        # x_start, y_start = width // 2 - 100, height // 2 - 100
        # x_end, y_end = width // 2 + 100, height // 2 + 100

        # # 관심 영역(ROI) 설정
        # roi = thresh[y_start:y_end, x_start:x_end]
        # roi_resized = cv2.resize(roi, (28, 28))
        # roi_normalized = roi_resized / 255.0
        # roi_reshaped = roi_normalized.reshape(1, 28, 28, 1)

        # # 모델로 숫자 예측
        # predictions = model.predict(roi_reshaped, verbose=0)
        # filtered_predictions = [predictions[0][2], predictions[0][5], predictions[0][8]]  # 2, 5, 8만 고려
        # predicted_number = [2, 5, 8][np.argmax(filtered_predictions)]  # 가장 높은 확률의 숫자 선택

        # # 숫자가 1~9일 때만 출력
        # label = f"Detected: {predicted_number}"

        # # 사각형과 텍스트 추가
        # cv2.rectangle(frame, (x_start, y_start), (x_end, y_end), (0, 255, 0), 2)
        # cv2.putText(frame, label, (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        # 저장된 숫자 화면에 출력
        # if saved_number is not None:
        #     saved_label = f"Saved: {saved_number}"
        #     cv2.putText(frame, saved_label, (10, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)

        
        # # 결과 출력
        # cv2.imshow("Webcam", frame)   
        # cv2.imshow("Processed Frame", thresh)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)  # 흑백 변환
        blurred = cv2.GaussianBlur(gray, (5, 5), 0)  # 가우시안 블러로 노이즈 제거
        _, thresh = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)  # Otsu 이진화

        height, width = thresh.shape
        x_start, y_start = width // 2 - 100, height // 2 - 100
        x_end, y_end = width // 2 + 100, height // 2 + 100

        # 관심 영역(ROI) 설정
        roi = thresh[y_start:y_end, x_start:x_end]
        roi_resized = cv2.resize(roi, (28, 28))
        roi_normalized = roi_resized / 255.0
        roi_reshaped = roi_normalized.reshape(1, 28, 28, 1)

        # ROI 외 영역을 검은색으로 처리
        masked_frame = np.zeros_like(thresh)
        masked_frame[y_start:y_end, x_start:x_end] = roi

        # 모델로 숫자 예측
        # predictions = model.predict(roi_reshaped, verbose=0)
        # filtered_predictions = [predictions[0][2], predictions[0][5], predictions[0][8]]  # 2, 5, 8만 고려
        # predicted_number = [2, 5, 8][np.argmax(filtered_predictions)]  # 가장 높은 확률의 숫자 선택

        predicted_number, processed_img = predict_number(model, roi)

        # 숫자가 1~9일 때만 출력
        label = f"Detected: {predicted_number}"

        # 사각형과 텍스트 추가
        cv2.rectangle(frame, (x_start, y_start), (x_end, y_end), (0, 255, 0), 2)
        cv2.putText(frame, label, (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        # 저장된 숫자 화면에 출력
        if saved_number is not None:
            saved_label = f"Saved: {saved_number}"
            cv2.putText(frame, saved_label, (10, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)

        # 결과 출력
        cv2.imshow("Webcam", frame)
        cv2.imshow("Processed Frame", masked_frame)

        # 'w' 키를 눌렀을 때 2초 대기 후 숫자 저장
        key = cv2.waitKey(1) & 0xFF
        if key == ord('w'):
            print("Saving detected number...")
            time.sleep(2)  # 2초 대기
            saved_number = predicted_number
            print(saved_number)
            print(f"Number {saved_number} saved!")

            # 숫자 전송
            data = {"CarNumber": int(saved_number)}
            print(data)
            client_socket.sendall(json.dumps(data).encode())
            print(f"Sent: {saved_number}")

            # 서버로부터 응답 받기
            data = client_socket.recv(1024)
            print(f"Received from server: {data.decode()}")

        # 종료 조건
        if key == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

    if saved_number is not None:
        print(f"Final saved number: {saved_number}")
