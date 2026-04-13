# 2WD Bluetooth Car — Walkthrough

## Architecture

```
Phone Browser ──WiFi──▶ Flask Server (Laptop/RPi) ──BT Serial──▶ HC-05 ──▶ Arduino Uno ──▶ L298N Motors
                                                   ◀── Distance data ◀──────────────────── HC-SR04
```

## Web UI Preview

![2WD Car Controller UI](/Users/pasan/.gemini/antigravity/brain/3ee2c34d-4355-4f48-a731-68db9626b8a6/controller_web_ui_1774973632084.png)

## Files Created/Modified

| File | Purpose |
|------|---------|
| [platformio.ini](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/platformio.ini) | Arduino Uno target config |
| [motor_control.h](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/include/motor_control.h) | L298N motor driver (analogWrite PWM) |
| [ultrasonic.h](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/include/ultrasonic.h) | HC-SR04 distance sensor |
| [main.cpp](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/src/main.cpp) | Arduino firmware — command parsing & motor control |
| [server.py](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/bridge/server.py) | Python Flask + PySerial bridge |
| [controller.html](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/bridge/templates/controller.html) | Web UI (dark theme, D-pad, speed slider, distance) |
| [requirements.txt](file:///Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth/bridge/requirements.txt) | Python dependencies |

## Build Verification

✅ `pio run` — **SUCCESS** (RAM: 22.8%, Flash: 20.7%)

## How to Use

### 1. Upload firmware to Arduino
```bash
cd /Volumes/Pasan_s_SSD/PlatformIO/Projects/4WD_Bluetooth
pio run -t upload
```

### 2. Pair HC-05 with your laptop/RPi
- **Linux/RPi**: `sudo rfcomm bind 0 <HC-05_MAC_ADDRESS>` → port is `/dev/rfcomm0`
- **macOS**: Pair via System Preferences → Bluetooth → port is [/dev/tty.HC-05](file:///dev/tty.HC-05)
- **Windows**: Pair via Settings → port shows in Device Manager as `COM3` etc.

### 3. Start the bridge server
```bash
cd bridge
pip install -r requirements.txt
python3 server.py --port /dev/rfcomm0   # adjust port for your OS
```

### 4. Control from phone
- Connect your phone to the **same WiFi network** as the laptop/RPi
- Open `http://<laptop-ip>:5000` in Chrome
- Use the D-pad to drive! (**Auto-stops** when you release the button)

## Key Features

- **Touch-optimized D-pad** with auto-stop on release for safety
- **Speed slider** (80–255 PWM range)
- **Live distance** from front ultrasonic sensor (color-coded: 🔴<15cm, 🟡<40cm, 🟢>40cm)
- **USB Serial debug** — commands also work via Arduino Serial Monitor for testing without Bluetooth
