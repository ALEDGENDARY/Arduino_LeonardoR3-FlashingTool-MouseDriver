import cv2
import numpy as np
import serial
import time
import dxcam

class ColorAimbot:
    def __init__(self):
        self.serial_conn = None
        self.camera = None
        self.running = False
        
        self.target_color_lower1 = np.array([145, 115, 125])
        self.target_color_upper1 = np.array([150, 200, 255])
        
        self.target_color_lower2 = np.array([30, 165, 159])
        self.target_color_upper2 = np.array([31, 255, 255])
        
        self.fov_size = 100
        self.setup_serial()
        self.setup_camera()

    def setup_serial(self):
        for port_num in range(2, 11):
            port_name = f'COM{port_num}'
            try:
                self.serial_conn = serial.Serial(port_name, 115200, timeout=1)
                print(f"Connected to {port_name}")
                time.sleep(2)
                break
            except:
                continue
        
        if not self.serial_conn:
            print("No COM port found")
            return

    def setup_camera(self):
        try:
            self.camera = dxcam.create()
            if self.camera:
                self.camera.start(target_fps=144)
                print("DXCAM started")
        except Exception as e:
            print(f"Camera error: {e}")

    def send_mouse_move(self, x, y):
        if self.serial_conn:
            command = f"MOVE {x},{y}\n"
            try:
                self.serial_conn.write(command.encode())
                print(f"Mouse move: {x}, {y}")
            except Exception as e:
                print(f"Serial error: {e}")

    def process_frame(self, frame):
        if frame is None:
            return None, None
        
        height, width = frame.shape[:2]
        center_x, center_y = width // 2, height // 2
        
        fov_left = center_x - self.fov_size // 2
        fov_top = center_y - self.fov_size // 2
        
        fov_region = frame[fov_top:fov_top+self.fov_size, fov_left:fov_left+self.fov_size]
        
        hsv = cv2.cvtColor(fov_region, cv2.COLOR_BGR2HSV)
        
        mask1 = cv2.inRange(hsv, self.target_color_lower1, self.target_color_upper1)
        mask2 = cv2.inRange(hsv, self.target_color_lower2, self.target_color_upper2)
        
        kernel = np.ones((2, 2), np.uint8)
        mask1 = cv2.dilate(mask1, kernel, iterations=2)
        mask2 = cv2.dilate(mask2, kernel, iterations=2)
        
        combined_mask = cv2.bitwise_or(mask1, mask2)
        
        target_x, target_y = None, None
        
        contours, _ = cv2.findContours(combined_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        if contours:
            largest_contour = max(contours, key=cv2.contourArea)
            if cv2.contourArea(largest_contour) > 5:
                x, y, w, h = cv2.boundingRect(largest_contour)
                top_center_x = x + w // 2
                top_center_y = y
                target_x = top_center_x - self.fov_size // 2
                target_y = top_center_y - self.fov_size // 2
        
        return target_x, target_y

    def run(self):
        if not self.camera or not self.serial_conn:
            print("Initialization failed")
            return
        
        self.running = True
        print("Starting auto-aim...")
        
        try:
            while self.running:
                frame = self.camera.get_latest_frame()
                
                if frame is not None:
                    target_x, target_y = self.process_frame(frame)
                    
                    if target_x is not None and target_y is not None:
                        self.send_mouse_move(int(target_x), int(target_y))
                
                time.sleep(0.001)
                
        except KeyboardInterrupt:
            print("Stopping...")
        
        self.cleanup()

    def cleanup(self):
        self.running = False
        if self.camera:
            self.camera.stop()
        if self.serial_conn:
            self.serial_conn.close()

if __name__ == "__main__":
    aimbot = ColorAimbot()
    aimbot.run()