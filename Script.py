import cv2
from ultralytics import YOLO

# =========================
# CONFIG
# =========================
STREAM_URL = "http://10.82.237.71:81/stream"  # ESP32 stream
MODEL_PATH = r"C:\Users\messe\Desktop\FireShield.pt"

# =========================
# LOAD MODEL
# =========================
model = YOLO(MODEL_PATH)

print("Model loaded successfully")

# =========================
# OPEN STREAM
# =========================
cap = cv2.VideoCapture(STREAM_URL)

if not cap.isOpened():
    print("Error: Cannot open ESP32 stream")
    exit()

print("Connected to ESP32-CAM")

# =========================
# LOOP
# =========================
while True:
    ret, frame = cap.read()

    if not ret:
        print("Failed to grab frame")
        break

    # =========================
    # INFERENCE
    # =========================
    results = model(frame)

    # =========================
    # DRAW RESULTS
    # =========================
    for result in results:
        for box in result.boxes:
            x1, y1, x2, y2 = map(int, box.xyxy[0])
            conf = float(box.conf[0])
            cls = int(box.cls[0])

            label = f"Fire {conf:.2f}"

            # red box for fire
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
            cv2.putText(frame, label, (x1, y1 - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

    # =========================
    # SHOW VIDEO
    # =========================
    cv2.imshow("Fire Detection (PyTorch)", frame)

    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()