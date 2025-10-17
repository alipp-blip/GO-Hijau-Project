# Updated level2.py - Compatible with new GoHijau API
import serial
import time
import json
import requests

UNO_PORT = '/dev/ttyUSB0'
MEGA_PORT = '/dev/ttyACM0'
BAUD_RATE = 9600
mega_ser = serial.Serial(MEGA_PORT, BAUD_RATE, timeout=1)
uno_ser = serial.Serial(UNO_PORT, BAUD_RATE, timeout=1)
QR_VALIDATE_URL = "https://services.gohijau.org/api/Qr/verify"
FINAL_SUBMIT_URL = "https://services.gohijau.org/api/Qr/complete"
TOKEN = None
time.sleep(2)

def send_overflow_warning():
    print("📢 Sending overflow warning to server...")
    payload = {
        "token": TOKEN,
        "message": "Overflow detected. Please close the door to continue."
    }
    try:
        response = requests.post("https://services.gohijau.org/api/Qr/overflow", json=payload)
        print(f"📨 Overflow API Response: {response.status_code} - {response.text}")
    except Exception as e:
        print(f"❌ Failed to send overflow message: {e}")


def get_weight_from_uno(samples=5):
    weights = []
    uno_ser.reset_input_buffer()
    time.sleep(0.2)

    for _ in range(samples):
        try:
            line = uno_ser.readline().decode().strip()
            if line:
                print(f"🟡 Uno says: {line}")
                weight = float(line)
                weights.append(weight)
        except Exception as e:
            print(f"⚠️ Read failed: {e}")
        time.sleep(0.2)

    if not weights:
        print("⚠️ No valid weights collected.")
        return None

    # Step 1: Median
    weights.sort()
    median = weights[len(weights) // 2]

    # Step 2: Filter outliers (any sample differing by >0.1kg from median)
    filtered = [w for w in weights if abs(w - median) <= 0.1]

    if not filtered:
        print("⚠️ All samples were outliers. Using unfiltered average.")
        filtered = weights

    avg_weight = round(sum(filtered) / len(filtered), 3)
    print(f"⚖️ Averaged Stable Weight: {avg_weight} kg (filtered from {weights})")
    return avg_weight



def send_to_arduino(command):
    mega_ser.write((command + '\n').encode())
    time.sleep(0.1)
    return mega_ser.readline().decode().strip()


def wait_for_qr():
    print("📷 Waiting for QR scan...")
    while True:
        scanned = input("Scan QR: ")
        if scanned:
            return scanned

def send_qr_to_api(token):
    global TOKEN
    headers = {'Content-Type': 'application/json'}
    payload = {"token": token}
    try:
        print(f"📨 Sending token to API: {payload}")
        response = requests.post(QR_VALIDATE_URL, json=payload, headers=headers)
        print(f"🔁 API Response: {response.status_code} - {response.text}")

        if response.status_code == 200:
            data = response.json()
            if data.get("success") == True:
                TOKEN = token
                return True
            else:
                print("❌ QR rejected by server (invalid token).")
        else:
            print("❌ Unexpected response code from API.")

    except Exception as e:
        print(f"❌ API Error: {e}")

    return False


def wait_for_pouring():
    print("🚪 Door unlocked. Begin pouring...")
    transfer_started = False

    while True:
        try:
            line = mega_ser.readline().decode().strip()
            if line:
                print(f"📟 Mega says: {line}")

                if "overflow_small_tank" in line:
                    print("⚠️ Overflow detected!")
                    send_to_arduino("LOCK")
                    send_overflow_warning()
                    return 0.0  # Early exit

                if "start_transfer" in line:
                    print("🔄 Transfer started.")
                    transfer_started = True
                    break  # ✅ Immediately start reading weight
        except Exception as e:
            print(f"⚠️ Error: {e}")
        time.sleep(0.1)

def monitor_pump_until_threshold():
    """Continuously read weight and stop pump when target is reached."""
    while True:
        weight = get_weight_from_uno()
        print(f"⚖️ Current Weight: {weight} kg")
        if weight is not None and weight <= 0.25:
            print("✅ Weight threshold met. Sending LOCK...")
            send_to_arduino("LOCK")
            return weight
        time.sleep(1)
        
def average_weight_from_uno(samples=3):
    weights = []
    for _ in range(samples):
        weight = get_weight_from_uno()
        if weight is not None:
            weights.append(weight)
        time.sleep(0.5)
    return sum(weights) / len(weights) if weights else 0.0


def monitor_and_stop_pump():
    while True:
        weight = get_stable_weight(samples=5)
        if weight is not None:
            print(f"⚖️ Average Weight While Pumping: {weight:.3f} kg")
            if weight <= 0.2:
                print("✅ Threshold reached. Sending LOCK to stop pump.")
                send_to_arduino("LOCK")
                break
        time.sleep(1)

        
def get_stable_weight(samples=5):
    weights = []
    for _ in range(samples):
        try:
            line = uno_ser.readline().decode().strip()
            print(f"📉 Pumping weight: {line}")
            weight = float(line)
            weights.append(weight)
        except:
            continue
        time.sleep(0.2)  # Small delay between samples
    if weights:
        return round(sum(weights) / len(weights), 3)
    return None







def send_final_data(oil_amount):
    print("📤 Sending final data to server...")
    payload = {
        "token": TOKEN,
        "oilAmount": round(oil_amount, 2)
    }
    try:
        response = requests.post(FINAL_SUBMIT_URL, json=payload)
        print(f"✅ Server Response: {response.status_code} - {response.text}")
    except Exception as e:
        print(f"❌ Failed to send final data: {e}")

def determine_user_door(token):
    # Example logic stub — update with actual logic if needed
    return 2 if token.endswith("2") else 3 if token.endswith("3") else 1

def main():
    while True:
        print("📷 Waiting for QR scan...")
        token = wait_for_qr()
        print(f"Scan QR: {token}")

        print(f"📨 Sending token to API: {{'token': '{token}'}}")
        if not send_qr_to_api(token):
            print("❌ Invalid QR token.")
            continue
        print("🟢 QR Validated.")

        # Determine which door to unlock based on QR
        send_to_arduino("unlock")
        print("🚪 Door unlocked. Waiting for user to open...")

        # Step 1: Wait for door_opened (60s)
        door_opened = False
        start_time = time.time()
        timeout_open = 60

        while time.time() - start_time < timeout_open:
            if mega_ser.in_waiting:
                msg = mega_ser.readline().decode().strip()
                if msg == "door_opened":
                    print("📟 Mega says: door opened")
                    door_opened = True
                    break
            time.sleep(0.2)

        if not door_opened:
            print("⏰ Timeout: Door not opened after QR. Returning to QR scan...")
            send_to_arduino("LOCK")
            send_final_data(0.0)
            continue

        # ✅ Step 2: Wait for door_closed with NO timeout
        print("⏳ Waiting for user to close the door...")

        while True:
            if mega_ser.in_waiting:
                msg = mega_ser.readline().decode().strip()
                if msg == "door_closed":
                    print("📟 Mega says: door closed")

                    # Step 3: Capture weight from Uno
                    print("⚖️ Capturing stable weight from Uno...")
                    weight = get_weight_from_uno()
                    if weight is not None:
                        print(f"📊 Final Oil Amount: {weight:.2f} kg")
                        send_final_data(weight)
                    else:
                        print("⚠️ Could not read weight from Uno!")
                    break

                elif msg.startswith("overflow"):
                    print(f"⚠️ Overflow detected: {msg}")
                    send_to_arduino("LOCK")
                    send_overflow_warning()
                    break
            time.sleep(0.2)

        # Step 4: Wait for "start_transfer" from Mega
        while True:
            if mega_ser.in_waiting:
                msg = mega_ser.readline().decode().strip()
                if msg == "start_transfer":
                    print("📟 Mega says: start_transfer")
                    print("🔄 Transfer started.")
                    break
            time.sleep(0.2)

        # Step 5: Monitor until weight threshold
        monitor_and_stop_pump()

        print("✅ Cycle complete. 🔁 Ready for next user...\n")




if __name__ == "__main__":
    try:
        while True:
            main()
            print("🔁 Ready for next user...\n")
            time.sleep(2)
    except KeyboardInterrupt:
        print("\n🛑 Program interrupted. Closing ports...")
        mega_ser.close()
        uno_ser.close()







