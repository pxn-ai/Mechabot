#!/usr/bin/env python3
"""
2WD Car — Bluetooth Serial to Web UI Bridge Server

This script runs on a laptop or Raspberry Pi that is paired with
the HC-05 Bluetooth module on the Arduino Uno. It:
  1. Opens the HC-05's serial port (Bluetooth SPP)
  2. Serves a Web UI on the local network
  3. Bridges HTTP commands ↔ Bluetooth serial

Usage:
  1. Pair your laptop/RPi with the HC-05 module
  2. Find the serial port:
       - Linux/RPi:  /dev/rfcomm0  (run: sudo rfcomm bind 0 <HC05_MAC>)
       - macOS:      /dev/cu.HC-05  (check System Preferences > Bluetooth)
       - Windows:    COM3, COM4, etc. (check Device Manager)
  3. Run:  python3 server.py              (defaults to /dev/cu.HC-05)
     Or:   python3 server.py --port /dev/rfcomm0
  4. Open http://<bridge-ip>:5000 on your phone browser
"""

import argparse
import json
import socket
import threading
import time
import sys

from flask import Flask, render_template, jsonify, request
import serial

# ──────────────────────────────────────────────
# Globals
# ──────────────────────────────────────────────
app = Flask(__name__)
bt_serial = None
bt_lock = threading.Lock()
latest_distance = {"distance": -1.0, "timestamp": 0}
server_info = {"ip": "0.0.0.0", "port": 5000}


def get_local_ip():
    """Get the local network IP address of this machine."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


# ──────────────────────────────────────────────
# Bluetooth Serial Reader Thread
# ──────────────────────────────────────────────
def bt_reader_thread():
    """Continuously reads from HC-05 and parses distance values."""
    global latest_distance
    read_count = 0
    while True:
        try:
            if bt_serial and bt_serial.is_open:
                raw = bt_serial.readline()
                line = raw.decode("utf-8", errors="ignore").strip()
                read_count += 1
                if line:
                    print(f"[BT-RX] '{line}'")
                    if line.startswith("D"):
                        try:
                            val = float(line[1:])
                            latest_distance = {
                                "distance": val,
                                "timestamp": time.time()
                            }
                        except ValueError:
                            print(f"[BT-RX] Could not parse distance from: '{line}'")
                else:
                    # Empty read (timeout) — log every 10th to avoid spam
                    if read_count % 10 == 0:
                        print(f"[BT-RX] No data (read #{read_count}, timeout)")
        except serial.SerialException:
            print("[BT] Serial connection lost, retrying...")
            time.sleep(2)
        except Exception as e:
            print(f"[BT] Error: {e}")
            time.sleep(0.1)


def send_bt_command(cmd: str):
    """Send a command string to the Arduino via HC-05."""
    try:
        with bt_lock:
            if bt_serial and bt_serial.is_open:
                data = (cmd + "\n").encode("utf-8")
                bt_serial.write(data)
                bt_serial.flush()
                print(f"[BT-TX] Sent: '{cmd}'")
                return True
            else:
                print(f"[BT-TX] ERROR: Serial port not open!")
    except Exception as e:
        print(f"[BT-TX] Send error: {e}")
    return False


# ──────────────────────────────────────────────
# Flask Routes
# ──────────────────────────────────────────────
@app.route("/")
def index():
    return render_template("controller.html",
                           server_ip=server_info["ip"],
                           server_port=server_info["port"])


@app.route("/forward")
def forward():
    speed = request.args.get("speed", "180")
    print(f"[CMD] /forward speed={speed}")
    ok = send_bt_command(f"F{speed}")
    return jsonify({"status": "ok" if ok else "error", "action": "forward", "speed": speed})


@app.route("/backward")
def backward():
    speed = request.args.get("speed", "180")
    print(f"[CMD] /backward speed={speed}")
    ok = send_bt_command(f"B{speed}")
    return jsonify({"status": "ok" if ok else "error", "action": "backward", "speed": speed})


@app.route("/left")
def left():
    speed = request.args.get("speed", "180")
    print(f"[CMD] /left speed={speed}")
    ok = send_bt_command(f"L{speed}")
    return jsonify({"status": "ok" if ok else "error", "action": "left", "speed": speed})


@app.route("/right")
def right():
    speed = request.args.get("speed", "180")
    print(f"[CMD] /right speed={speed}")
    ok = send_bt_command(f"R{speed}")
    return jsonify({"status": "ok" if ok else "error", "action": "right", "speed": speed})


@app.route("/stop")
def stop():
    print(f"[CMD] /stop")
    ok = send_bt_command("S")
    return jsonify({"status": "ok" if ok else "error", "action": "stop"})


@app.route("/distance")
def distance():
    return jsonify(latest_distance)


@app.route("/info")
def info():
    return jsonify(server_info)


# ──────────────────────────────────────────────
# Entry Point
# ──────────────────────────────────────────────
def main():
    global bt_serial

    parser = argparse.ArgumentParser(description="2WD Car Bluetooth Bridge Server")
    parser.add_argument("--port", "-p", default="/dev/cu.HC-05",
                        help="Bluetooth serial port (default: /dev/cu.HC-05)")
    parser.add_argument("--baud", "-b", type=int, default=9600,
                        help="Baud rate (default: 9600)")
    parser.add_argument("--host", default="0.0.0.0",
                        help="Web server host (default: 0.0.0.0)")
    parser.add_argument("--web-port", "-w", type=int, default=10000,
                        help="Web server port (default: 10000)")
    args = parser.parse_args()

    # Connect to HC-05 via Bluetooth serial
    print(f"[BT] Connecting to {args.port} at {args.baud} baud...")
    try:
        bt_serial = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)  # Wait for Arduino reset after serial connect
        print(f"[BT] Connected to {args.port}")
    except serial.SerialException as e:
        print(f"[BT] ERROR: Could not open {args.port}: {e}")
        print("     Make sure the HC-05 is paired and the port is correct.")
        sys.exit(1)

    # Start background reader thread
    reader = threading.Thread(target=bt_reader_thread, daemon=True)
    reader.start()

    # Detect local IP
    local_ip = get_local_ip()
    server_info["ip"] = local_ip
    server_info["port"] = args.web_port

    # Start web server
    print(f"[WEB] Starting server on http://{local_ip}:{args.web_port}")
    print(f"[WEB] Open this URL on your phone (same WiFi network)")
    app.run(host=args.host, port=args.web_port, debug=False)


if __name__ == "__main__":
    main()
