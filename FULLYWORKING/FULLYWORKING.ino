#include <SPI.h>
#include <Mouse.h>
#include <Keyboard.h>
#include <Usb.h>
#include <hiduniversal.h>
#include <usbhub.h>

USB Usb;
HIDUniversal Hid(&Usb);

// Custom HID Report Structure
typedef struct {
  uint8_t buttons;
  int8_t x;
  int8_t y;
  int8_t wheel;
} MouseReport;

class MouseRptParser : public HIDReportParser {
private:
  uint8_t lastButtonState = 0;
  int scrollAccumulator = 0;
  bool backPressed = false;
  bool forwardPressed = false;
  
  // SCROLL SETTINGS
  const int SCROLL_SENSITIVITY = 2;
  const int SCROLL_THRESHOLD = 1;
  
  // SIDE BUTTON CODES
  const uint8_t BACK_BUTTON_CODE = 0x08;
  const uint8_t FORWARD_BUTTON_CODE = 0x10;
  
public:
  virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    if (len == 7) {
      uint8_t clickType = buf[1];
      
      // Movement from physical mouse
      int16_t moveX = (int16_t)((buf[3] << 8) | buf[2]);
      int16_t moveY = (int16_t)((buf[5] << 8) | buf[4]);
      int8_t scroll = (int8_t)buf[6];
      
      // Handle movement
      if (moveX != 0 || moveY != 0) {
        int8_t moveX8 = constrain(moveX, -127, 127);
        int8_t moveY8 = constrain(moveY, -127, 127);
        Mouse.move(moveX8, moveY8, 0);
      }
      
      // Scroll wheel
      if (scroll != 0) {
        scrollAccumulator += scroll * SCROLL_SENSITIVITY;
        if (abs(scrollAccumulator) >= SCROLL_THRESHOLD) {
          Mouse.move(0, 0, scrollAccumulator);
          scrollAccumulator = 0;
        }
      }
      
      // Handle buttons
      handleButtonClicks(clickType);
    }
  }
  
private:
  void handleButtonClicks(uint8_t clickType) {
    // Handle standard mouse buttons
    if (clickType == 0x00) {
      Mouse.release(MOUSE_LEFT);
      Mouse.release(MOUSE_RIGHT);
      Mouse.release(MOUSE_MIDDLE);
    } else {
      if (clickType & 0x01) Mouse.press(MOUSE_LEFT);
      else Mouse.release(MOUSE_LEFT);
      
      if (clickType & 0x02) Mouse.press(MOUSE_RIGHT);
      else Mouse.release(MOUSE_RIGHT);
      
      if (clickType & 0x04) Mouse.press(MOUSE_MIDDLE);
      else Mouse.release(MOUSE_MIDDLE);
    }
    
    // Handle side buttons with keyboard shortcuts
    handleSideButtons(clickType);
  }
  
  void handleSideButtons(uint8_t clickType) {
    // Back button - Browser Back (Alt + Left Arrow)
    if ((clickType & BACK_BUTTON_CODE) && !backPressed) {
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(0xD8);
      delay(10);
      Keyboard.releaseAll();
      backPressed = true;
    } else if (!(clickType & BACK_BUTTON_CODE)) {
      backPressed = false;
    }
    
    // Forward button - Browser Forward (Alt + Right Arrow)
    if ((clickType & FORWARD_BUTTON_CODE) && !forwardPressed) {
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(0xD7);
      delay(10);
      Keyboard.releaseAll();
      forwardPressed = true;
    } else if (!(clickType & FORWARD_BUTTON_CODE)) {
      forwardPressed = false;
    }
  }
};

MouseRptParser Prs;

// ===== HID FEATURE REPORT FOR PYTHON CONTROL =====
class CustomHIDReportParser : public HIDReportParser {
public:
  virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    // This receives custom HID reports from Python
    if (len >= 4 && buf[0] == 0x02) { // Custom report ID
      int8_t moveX = (int8_t)buf[1];
      int8_t moveY = (int8_t)buf[2];
      uint8_t buttons = buf[3];
      
      // Move mouse
      if (moveX != 0 || moveY != 0) {
        Mouse.move(moveX, moveY, 0);
      }
      
      // Handle buttons from Python
      if (buttons & 0x01) Mouse.press(MOUSE_LEFT);
      else Mouse.release(MOUSE_LEFT);
      
      if (buttons & 0x02) Mouse.press(MOUSE_RIGHT);
      else Mouse.release(MOUSE_RIGHT);
      
      if (buttons & 0x04) Mouse.press(MOUSE_MIDDLE);
      else Mouse.release(MOUSE_MIDDLE);
    }
  }
};

CustomHIDReportParser CustomPrs;

void setup() {
  // No Serial.begin() - HID only mode!
  
  if (Usb.Init() == -1) {
    // USB Host failed
    return;
  }
  delay(200);
  
  // Set up parsers for both physical mouse and Python control
  Hid.SetReportParser(0, &Prs);
  // Use a different report ID for Python control
  Hid.SetReportParser(1, &CustomPrs);
  
  Mouse.begin();
  Keyboard.begin();
  
  // Arduino now appears as pure HID device, no COM port
}

void loop() {
  Usb.Task();
  delay(5);
}