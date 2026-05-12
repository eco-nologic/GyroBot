import subprocess
import argparse
import sys
import os
import shutil

def get_pio_command():
    """Locate the platformio executable."""
    # 1. Check if it's already in the system PATH
    executable = shutil.which("platformio") or shutil.which("pio")
    if executable:
        return executable
    # 2. Check common Windows path (PlatformIO installed via VS Code)
    home = os.path.expanduser("~")
    pio_path = os.path.join(home, ".platformio", "penv", "Scripts", "platformio.exe")
    if os.path.exists(pio_path):
        return pio_path
    return "platformio" # Fallback to name

def main():
    parser = argparse.ArgumentParser(description="GiRobot Batch Uploader: Firmware + Filesystem")
    parser.add_argument("--port", help="Specify serial port (e.g., COM3)")
    parser.add_argument("--env", default="esp32_real", help="PlatformIO environment (default: esp32_real)")
    args = parser.parse_args()

    # Ensure we are in the project root (one level up from scripts/)
    current_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(current_dir)
    # If platformio.ini isn't here, we are likely in a subdirectory
    if not os.path.exists("platformio.ini"):
        os.chdir("..")

    def run_pio(target_name):
        pio_cmd = get_pio_command()
        cmd = [pio_cmd, "run", "-e", args.env, "--target", target_name]
        if args.port:
            cmd.extend(["--upload-port", args.port])
        
        print(f"\n--- Executing: {' '.join(cmd)} ---")
        # shell=True is required on Windows to find the platformio command
        result = subprocess.run(cmd, shell=True)
        return result.returncode == 0

    print(f"🚀 Starting GiRobot Deployment to environment: {args.env}")

    # 1. Upload Firmware
    print("\n📦 Step 1/2: Uploading Firmware...")
    if not run_pio("upload"):
        print("\n❌ Error: Firmware upload failed.")
        sys.exit(1)

    # 2. Upload Filesystem (LittleFS)
    print("\n📂 Step 2/2: Uploading Filesystem Image (Data folder)...")
    if not run_pio("uploadfs"):
        print("\n❌ Error: Filesystem upload failed. Ensure the 'data' folder exists.")
        sys.exit(1)

    print("\n✨ Success! Firmware and Web Dashboard have been deployed.")
    print("🔌 Connect to 'RobotWifi' and navigate to http://192.168.4.1")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nStopped by user.")