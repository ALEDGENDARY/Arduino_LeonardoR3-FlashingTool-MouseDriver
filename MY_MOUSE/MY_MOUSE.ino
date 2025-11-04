#include <SPI.h>
#include <Mouse.h>
#include <Usb.h>
#include <hiduniversal.h>
#include <usbhub.h>

USB Usb;
HIDUniversal Hid(&Usb);

class MouseRptParser : public HIDReportParser {
private:
  uint8_t oldButtons = 0;
  int scrollAccumulator = 0;
  
  // SCROLL SETTINGS
  const int SCROLL_SENSITIVITY = 2;
  const int SCROLL_THRESHOLD = 1;
  
public:
  virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    if (len == 7) {
      uint8_t buttons = buf[1];  // ALL buttons are in byte 1
      
      // Movement (working great!)
      int16_t moveX = (int16_t)((buf[3] << 8) | buf[2]);
      int16_t moveY = (int16_t)((buf[5] << 8) | buf[4]);
      int8_t scroll = (int8_t)buf[6];
      
      // Handle movement
      if (moveX != 0 || moveY != 0) {
        int8_t moveX8 = constrain(moveX, -127, 127);
        int8_t moveY8 = constrain(moveY, -127, 127);
        Mouse.move(moveX8, moveY8, 0);
      }
      
      // SUPER SMOOTH SCROLL WHEEL
      if (scroll != 0) {
        scrollAccumulator += scroll * SCROLL_SENSITIVITY;
        if (abs(scrollAccumulator) >= SCROLL_THRESHOLD) {
          Mouse.move(0, 0, scrollAccumulator);
          scrollAccumulator = 0;
        }
      }
      
      // Handle ALL buttons including side buttons
      handleAllButtons(buttons, oldButtons);
      oldButtons = buttons;
    }
  }
  
private:
  void handleAllButtons(uint8_t newButtons, uint8_t oldButtons) {
    // Left button (bit 0) - 0x01
    if ((newButtons & 0x01) && !(oldButtons & 0x01)) {
      Mouse.press(MOUSE_LEFT);
      Serial.println("🟦 LEFT CLICK");
    }
    if (!(newButtons & 0x01) && (oldButtons & 0x01)) {
      Mouse.release(MOUSE_LEFT);
    }
    
    // Right button (bit 1) - 0x02
    if ((newButtons & 0x02) && !(oldButtons & 0x02)) {
      Mouse.press(MOUSE_RIGHT);
      Serial.println("🟥 RIGHT CLICK");
    }
    if (!(newButtons & 0x02) && (oldButtons & 0x02)) {
      Mouse.release(MOUSE_RIGHT);
    }
    
    // Middle button (bit 2) - 0x04
    if ((newButtons & 0x04) && !(oldButtons & 0x04)) {
      Mouse.press(MOUSE_MIDDLE);
      Serial.println("🟨 MIDDLE CLICK");
    }
    if (!(newButtons & 0x04) && (oldButtons & 0x04)) {
      Mouse.release(MOUSE_MIDDLE);
    }
    
    // Side Button 1 (bit 3) - 0x08
    if ((newButtons & 0x08) && !(oldButtons & 0x08)) {
      Serial.println("⬅️ SIDE BUTTON 1 DETECTED");
      // The mouse is sending the side button signal
      // If browser doesn't respond, it's because the mouse report 
      // doesn't include standard browser navigation commands
    }
    
    // Side Button 2 (bit 4) - 0x10
    if ((newButtons & 0x10) && !(oldButtons & 0x10)) {
      Serial.println("➡️ SIDE BUTTON 2 DETECTED");
      // The mouse is sending the side button signal
    }
  }
};

MouseRptParser Prs;

void setup() {
  Serial.begin(115200);
  Serial.println("=== PURE MOUSE CONTROLLER ===");
  Serial.println("Side buttons detected directly from mouse data");
  Serial.println("If browser doesn't navigate, it's a mouse protocol issue");
  Serial.println("======================================================");
  
  if (Usb.Init() == -1) Serial.println("USB Host failed");
  delay(200);
  
  Hid.SetReportParser(0, &Prs);
  Mouse.begin();
  
  Serial.println("READY - Side buttons are being detected");
  Serial.println("Check if your browser responds to them");
}

void loop() {
  Usb.Task();
  delay(5);
}
