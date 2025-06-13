import subprocess
import os
import sys
import shutil # Import shutil for cross-platform directory removal

# Define virtual environment name and path
env_name = "myenv"
env_path = os.path.join(os.getcwd(), env_name)
current_python_executable = sys.executable
modules_file = "modules.txt" # Define the name of the file containing modules

print(f"--- Setting up Nuitka Development Environment ---")

# Routine to check and remove existing venv
if os.path.exists(env_path):
    print(f"Existing virtual environment '{env_name}' found at {env_path}.")
    print(f"Attempting to remove existing '{env_name}' folder...")

    # Deactivation warning: A Python script cannot deactivate the virtual environment
    # in the parent shell that called it. This message is for user awareness.
    print("NOTE: If 'myenv' was active in your current shell, please deactivate it manually after this script runs.")

    try:
        # Use shutil.rmtree for cross-platform directory removal.
        # This is generally safer and more robust than calling OS-specific shell commands
        # like rmdir or rm -rf directly via subprocess for directory removal.
        shutil.rmtree(env_path)
        print(f"Dev environment '{env_name}' has been removed.")
        # EXIT HERE AFTER SUCCESSFUL REMOVAL, AS REQUESTED
        sys.exit(0)
    except OSError as e:
        print(f"Error removing existing virtual environment: {e}")
        print("Please ensure you have permissions to delete the folder and it's not in use.")
        sys.exit(1) # Exit if removal fails

# 1. Create virtual environment
print(f"\n1. Creating virtual environment: '{env_name}'...")
try:
    subprocess.run([current_python_executable, "-m", "venv", env_name], check=True)
    print(f"   Virtual environment '{env_name}' created successfully at {env_path}")
except subprocess.CalledProcessError as e:
    print(f"Error creating virtual environment: {e}")
    sys.exit(1) # Exit if venv creation fails

# Determine the correct path to the Python executable within the new virtual environment.
# This is OS-dependent (Scripts on Windows, bin on Linux/macOS).
if sys.platform == "win32":
    venv_python = os.path.join(env_path, "Scripts", "python.exe")
    # activate_script is no longer needed here for constructing the final output strings
else:
    venv_python = os.path.join(env_path, "bin", "python")
    # activate_script is no longer needed here for constructing the final output strings

# Read modules from modules.txt
packages_to_install = []
if os.path.exists(modules_file):
    print(f"\nReading modules from '{modules_file}'...")
    with open(modules_file, 'r') as f:
        for line in f:
            package = line.strip()
            if package and not package.startswith('#'): # Ignore empty lines and comments
                packages_to_install.append(package)
    if not packages_to_install:
        print(f"No modules found in '{modules_file}'.")
else:
    print(f"Warning: '{modules_file}' not found. No additional modules will be installed.")

# Install modules
if packages_to_install:
    print(f"\nInstalling modules into '{env_name}'...")
    for package in packages_to_install:
        print(f"   Installing '{package}'...")
        try:
            subprocess.run([venv_python, "-m", "pip", "install", package], check=True)
            print(f"     '{package}' installed.")
        except subprocess.CalledProcessError as e:
            print(f"Error installing '{package}': {e}")
            sys.exit(1) # Exit if any package installation fails
else:
    print("No modules to install (besides base venv).")

print("\n--- Setup Complete ---")
print("Ready to compile with Nuitka!")
print(f"\nTo activate your environment, run:")

# Define platform-specific relative activation commands using env_name (e.g., "myenv")
# These strings are tailored for copy-pasting into the respective shells.
activate_cmd_linux = f"source {env_name}/bin/activate"
activate_cmd_win_cmd = f"{env_name}\\Scripts\\activate.bat"
activate_cmd_win_ps = f".\\{env_name}\\Scripts\\Activate.ps1" # Using .\\ for PowerShell relative execution

print(f"  Linux/macOS: {activate_cmd_linux}")
print(f"  Windows (Cmd): {activate_cmd_win_cmd}")
print(f"  Windows (PowerShell): {activate_cmd_win_ps}")
