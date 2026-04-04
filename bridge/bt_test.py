#!/usr/bin/env python3
"""
Advanced HC-05 Bluetooth diagnostic — tries multiple connection approaches
to find what works on macOS.
"""
import serial
import time
import sys
import os
import subprocess

PORT = "/dev/cu.HC-05"
BAUD = 9600

def test_connection(label, **serial_kwargs):
    """Try a specific serial configuration and report results."""
    print(f"\n{'='*60}")
    print(f"  TEST: {label}")
    print(f"{'='*60}")
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=2, **serial_kwargs)
        time.sleep(2)
    except Exception as e:
        print(f"  ❌ Failed to open: {e}")
        return False

    # Print status
    try:
        print(f"  Port open: {ser.is_open}")
        print(f"  CTS={ser.cts} DSR={ser.dsr} CD={ser.cd} RI={ser.ri}")
    except:
        pass

    # Toggle DTR/RTS to kick-start connection
    try:
        ser.dtr = False
        ser.rts = False
        time.sleep(0.5)
        ser.dtr = True
        ser.rts = True
        time.sleep(0.5)
        print(f"  Toggled DTR/RTS")
    except Exception as e:
        print(f"  Could not toggle DTR/RTS: {e}")

    # Flush buffers
    try:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
    except:
        pass

    # Try sending first (sometimes HC-05 needs a write to start)
    try:
        ser.write(b"PING\n")
        ser.flush()
        print(f"  Sent 'PING'")
    except Exception as e:
        print(f"  Write error: {e}")

    time.sleep(1)

    # Listen for data
    print(f"  Listening for 6 seconds...")
    rx_count = 0
    start = time.time()
    while time.time() - start < 6:
        data = ser.readline()
        if data:
            rx_count += 1
            text = data.decode('utf-8', errors='ignore').strip()
            print(f"  ✅ RX: '{text}'")
            if rx_count >= 3:
                break
        else:
            # Try reading raw bytes instead of lines
            raw = ser.read(ser.in_waiting or 1)
            if raw:
                rx_count += 1
                print(f"  ✅ RX (raw): {raw}")

    # Try sending a motor command
    if rx_count > 0:
        print(f"  Sending 'F200' command...")
        ser.write(b"F200\n")
        ser.flush()
        time.sleep(1)
        resp = ser.readline()
        if resp:
            print(f"  ✅ Response: {resp.decode('utf-8', errors='ignore').strip()}")

    ser.close()
    
    if rx_count > 0:
        print(f"\n  ✅ SUCCESS — received {rx_count} messages!")
        return True
    else:
        print(f"\n  ❌ No data received")
        return False


def main():
    print("HC-05 Bluetooth Advanced Diagnostic")
    print(f"Port: {PORT}  Baud: {BAUD}")
    print()

    # Check if port exists
    if not os.path.exists(PORT):
        print(f"❌ Port {PORT} does not exist!")
        print("   HC-05 may not be paired. Go to System Settings > Bluetooth and pair it.")
        
        # List available Bluetooth serial ports
        result = subprocess.run(["ls", "/dev/cu.HC*", "/dev/tty.HC*"], 
                              capture_output=True, text=True, shell=False)
        # Try with glob
        import glob
        bt_ports = glob.glob("/dev/cu.HC*") + glob.glob("/dev/tty.HC*") + glob.glob("/dev/cu.*Bluetooth*") + glob.glob("/dev/tty.*Bluetooth*")
        if bt_ports:
            print(f"   Found Bluetooth ports: {bt_ports}")
        else:
            print(f"   No Bluetooth serial ports found at all.")
            # List all serial ports
            all_ports = glob.glob("/dev/cu.*") + glob.glob("/dev/tty.*")
            print(f"   All serial ports: {[p for p in all_ports if 'Bluetooth' in p or 'HC' in p or 'BT' in p]}")
        sys.exit(1)

    # Test 1: Default settings
    if test_connection("Default (no flow control)"):
        return

    # Test 2: With hardware flow control
    if test_connection("Hardware flow control (RTS/CTS)", rtscts=True):
        return

    # Test 3: With DSR/DTR flow control
    if test_connection("DSR/DTR flow control", dsrdtr=True):
        return

    # Test 4: With exclusive access
    try:
        if test_connection("Exclusive access", exclusive=True):
            return
    except:
        print("  (exclusive mode not supported)")

    # Test 5: With XON/XOFF software flow control
    if test_connection("Software flow control (XON/XOFF)", xonxoff=True):
        return

    # Test 6: Different baud rates (maybe HC-05 was reconfigured)
    for baud in [38400, 57600, 115200]:
        print(f"\n{'='*60}")
        print(f"  TEST: Baud rate {baud}")
        print(f"{'='*60}")
        try:
            ser = serial.Serial(PORT, baud, timeout=2)
            time.sleep(1)
            ser.write(b"PING\n")
            ser.flush()
            time.sleep(1)
            data = ser.readline()
            if data:
                print(f"  ✅ Got data at {baud} baud: {data}")
                ser.close()
                return
            else:
                print(f"  ❌ No data at {baud}")
            ser.close()
        except Exception as e:
            print(f"  ❌ Error: {e}")

    print(f"""
╔══════════════════════════════════════════════════════════════╗
║  ALL TESTS FAILED — NO DATA FLOWS                           ║
║                                                              ║
║  The HC-05 LED shows 'connected' but macOS isn't passing     ║
║  data through the serial port.                               ║
║                                                              ║
║  NEXT STEPS:                                                 ║
║                                                              ║
║  1. Test with screen (raw terminal):                         ║
║     screen /dev/cu.HC-05 9600                                ║
║     (Ctrl-A then K to quit)                                  ║
║     If screen shows D15.9 messages, the issue is PySerial.   ║
║                                                              ║
║  2. Check HC-05 wiring:                                      ║
║     HC-05 TXD → Arduino pin 10 (SoftwareSerial RX)          ║
║     HC-05 RXD → Arduino pin 11 (SoftwareSerial TX)          ║
║     HC-05 VCC → 5V,  GND → GND                              ║
║     HC-05 KEY/EN → leave disconnected (NOT HIGH)             ║
║                                                              ║
║  3. Try removing and re-pairing HC-05:                       ║
║     System Settings > Bluetooth > forget HC-05               ║
║     Power cycle HC-05, then re-pair (PIN: 1234)              ║
║                                                              ║
║  4. Reset Bluetooth on Mac:                                  ║
║     Hold Shift+Option, click Bluetooth menu bar icon          ║
║     > Debug > Reset Bluetooth Module                         ║
║                                                              ║
║  5. Test HC-05 itself with a phone BT terminal app:          ║
║     Use "Serial Bluetooth Terminal" (Android) to connect      ║
║     and verify data flows from Arduino                       ║
╚══════════════════════════════════════════════════════════════╝
""")


if __name__ == "__main__":
    main()
