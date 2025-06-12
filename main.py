import os
import sys
import json
import requests
import argparse
import threading
import time
from pathlib import Path

# Configuration constants
CONFIG_DIR_NAME = ".deepshell"
CONFIG_FILE_NAME = "deepshell.conf"
LLM_SERVICE_OLLAMA = "ollama"
LLM_SERVICE_GEMINI = "gemini"
SUPPORTED_LLM_SERVICES = [LLM_SERVICE_OLLAMA, LLM_SERVICE_GEMINI]

# Version
__version__ = "1.0"

# Gemini API Configuration
GEMINI_API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta"

# Define ANSI escape codes for various colors globally
COLORS = {
    "red": "\033[91m",
    "green": "\033[92m",
    "yellow": "\033[93m",
    "blue": "\033[94m",
    "orange": "\033[33m", # A darker yellow/orange
    "reset": "\033[0m"    # Resets text color to default
}

def get_config_path():
    """
    Returns the full path to the configuration directory and file.
    The config will be stored in a '.deepshell' folder within the user's home directory.
    """
    home_dir = Path.home()
    config_dir = home_dir / CONFIG_DIR_NAME
    config_file = config_dir / CONFIG_FILE_NAME
    return config_dir, config_file

def display_message(message, color_name=None):
    """
    Prints a message to the console with optional ANSI color codes.
    This provides visual feedback for status, errors, and success.
    """
    if color_name and color_name in COLORS:
        print(f"{COLORS[color_name]}{message}{COLORS['reset']}")
    else:
        print(message)

def fetch_ollama_models(server_address):
    """
    Fetches available models from the OLLAMA server's /api/tags endpoint.
    It handles various network and parsing errors gracefully.
    """
    display_message(f"Attempting to fetch models from {server_address}...", "blue")
    url = f"{server_address}/api/tags"
    try:
        # Send a GET request to the /api/tags endpoint with a timeout
        response = requests.get(url, timeout=10)
        # Raise an HTTPError for bad responses (4xx or 5xx status codes)
        response.raise_for_status()
        data = response.json()
        # Extract model names from the 'models' list in the JSON response
        models = [m['name'] for m in data.get('models', [])]
        return models
    except requests.exceptions.ConnectionError:
        display_message(f"Error: Could not connect to {server_address}. Please ensure the OLLAMA server is running and accessible.", "red")
        return None
    except requests.exceptions.Timeout:
        display_message(f"Error: Connection to {server_address} timed out. The server did not respond in time.", "red")
        return None
    except requests.exceptions.HTTPError as http_err:
        display_message(f"HTTP Error fetching models: {http_err} - Server response: {response.text}", "red")
        return None
    except json.JSONDecodeError:
        display_message(f"Error: Could not parse JSON response from {server_address}. Is this address pointing to an OLLAMA server?", "red")
        return None
    except Exception as e:
        display_message(f"An unexpected error occurred while fetching models: {e}", "red")
        return None

def fetch_gemini_models(api_key):
    """
    Fetches available models from the Gemini API.
    """
    display_message("Attempting to fetch Gemini models...", "blue")
    url = f"{GEMINI_API_BASE_URL}/models?key={api_key}"
    try:
        response = requests.get(url, timeout=20)
        response.raise_for_status()
        data = response.json()
        # We are interested in models that support 'generateContent'
        # and are not chat models for this simple query tool.
        # Gemini model names are like "models/gemini-pro"
        models = [
            m['name'] for m in data.get('models', [])
            if any(method in m.get('supportedGenerationMethods', []) for method in ['generateContent', 'generateAnswer'])
            and "chat" not in m['name'].lower()  # Exclude chat models (case-insensitive)
            and "tts" not in m['name'].lower()   # Exclude Text-To-Speech models (case-insensitive)
        ]
        if not models:
            display_message("No suitable Gemini models found. Ensure your API key is valid and has access to models like 'gemini-pro'.", "yellow")
            display_message(f"Full response from Gemini API: {json.dumps(data, indent=2)}", "yellow")
            return [] # Return empty list, not None, to distinguish from connection errors
        return models
    except requests.exceptions.ConnectionError:
        display_message("Error: Could not connect to Google API. Please check your internet connection.", "red")
        return None
    except requests.exceptions.Timeout:
        display_message("Error: Connection to Google API timed out.", "red")
        return None
    except requests.exceptions.HTTPError as http_err:
        error_details = "Unknown error"
        try:
            error_details = http_err.response.json().get('error', {}).get('message', http_err.response.text)
        except json.JSONDecodeError:
            error_details = http_err.response.text
        display_message(f"HTTP Error fetching Gemini models: {http_err} - Details: {error_details}", "red")
        if "API key not valid" in error_details:
            display_message("Please ensure your Gemini API key is correct.", "yellow")
        return None
    except json.JSONDecodeError:
        display_message("Error: Could not parse JSON response from Google API.", "red")
        return None
    except Exception as e:
        display_message(f"An unexpected error occurred while fetching Gemini models: {e}", "red")
        return None

def get_default_config_structure():
    """Returns the basic structure for a new configuration file."""
    return {
        "active_llm_service": None,
        "llm_services": {}
    }

def _setup_ollama_service(config_data, server_address_input_prompt="Enter OLLAMA Server IP/Hostname (e.g., localhost or 192.168.1.100): "):
    """Sets up or updates Ollama service configuration."""
    display_message("\n--- Ollama Service Setup ---", "green")
    existing_ollama_config = config_data.get("llm_services", {}).get(LLM_SERVICE_OLLAMA, {})
    current_server_address_val = existing_ollama_config.get("server_address", "")

    server_address = ""
    models = None

    while True:
        try:
            default_prompt = f" (current: {current_server_address_val})" if current_server_address_val else ""
            server_address_input = input(f"{server_address_input_prompt}{default_prompt}: ").strip()

            if not server_address_input and current_server_address_val:
                server_address_input = current_server_address_val # User pressed Enter, use current
            elif not server_address_input:
                display_message("Server address cannot be empty. Please try again.", "red")
                continue

            temp_server_address = ""
            if ":" in server_address_input:
                temp_server_address = server_address_input
            else:
                use_default_port_prompt = f"No port specified for '{server_address_input}'. Use default port 11434? (Y/n): "
                use_default_port = input(use_default_port_prompt).strip().lower()
                if use_default_port == 'y' or use_default_port == '':
                    temp_server_address = f"{server_address_input}:11434"
                else:
                    display_message("Please re-enter the server address including the port, or enter just the IP/hostname to be prompted for the default port again.", "yellow")
                    continue

            if not temp_server_address.startswith("http://") and not temp_server_address.startswith("https://"):
                temp_server_address = "http://" + temp_server_address

            models = fetch_ollama_models(temp_server_address)
            if models is None: # Connection or critical error
                # fetch_ollama_models already displayed an error
                # Ask if user wants to try again or skip
                retry = input("Failed to connect or fetch models. Try again with a different address? (Y/n): ").strip().lower()
                if retry == 'n':
                    return None # User chose not to retry, signal failure
                continue # Loop to re-ask for server address
            elif not models: # Connected but no models found
                display_message(f"No models found at {temp_server_address}. Please ensure models are available on the Ollama server.", "yellow")
                retry = input("Try again with a different address or check server? (Y/n to skip Ollama setup): ").strip().lower()
                if retry == 'n':
                    return None
                continue
            else:
                server_address = temp_server_address
                break
        except KeyboardInterrupt:
            display_message("\nOllama setup cancelled by user.", "yellow")
            return None # Signal cancellation

    display_message("\nAvailable Ollama Models:", "green")
    for i, model in enumerate(models):
        print(f"  {i+1}. {model}")

    chosen_model_index = -1
    current_ollama_model = existing_ollama_config.get("model")
    default_model_prompt = f" (current: {current_ollama_model})" if current_ollama_model and current_ollama_model in models else ""

    while not (0 <= chosen_model_index < len(models)):
        try:
            choice_prompt = f"Enter the number of the Ollama model you want to use (1-{len(models)}){default_model_prompt}: "
            choice = input(choice_prompt).strip()
            if not choice and current_ollama_model and current_ollama_model in models:
                chosen_model_index = models.index(current_ollama_model)
                display_message(f"Using current default model: {current_ollama_model}", "blue")
                break
            chosen_model_index = int(choice) - 1
            if not (0 <= chosen_model_index < len(models)):
                display_message("Invalid model number. Please enter a number from the list.", "red")
        except ValueError:
            display_message("Invalid input. Please enter a numerical value.", "red")
        except KeyboardInterrupt:
            display_message("\nModel selection cancelled by user.", "yellow")
            return None

    chosen_model = models[chosen_model_index]
    ollama_config = {
        "server_address": server_address,
        "model": chosen_model
    }
    if "llm_services" not in config_data:
        config_data["llm_services"] = {}
    config_data["llm_services"][LLM_SERVICE_OLLAMA] = ollama_config
    display_message(f"Ollama service configured with model: {chosen_model}", "green")
    return config_data


def _find_key_by_nickname(nickname, keys_list):
    """Helper to find an API key dictionary by its nickname."""
    for item in keys_list:
        if item.get("nickname") == nickname:
            return item
    return None

def _get_key_value_by_nickname(nickname, keys_list):
    """Helper to get an API key's value by its nickname."""
    item = _find_key_by_nickname(nickname, keys_list)
    if item:
        return item.get("key")
    return None

def _get_active_gemini_key_value(gemini_config):
    """Retrieves the value of the active Gemini API key."""
    if not gemini_config:
        return None, None # No config, no key, no nickname
    
    active_nickname = gemini_config.get("active_api_key_nickname")
    api_keys_list = gemini_config.get("api_keys", [])
    
    if not active_nickname or not api_keys_list:
        return None, None # No active nickname or no keys list

    for key_info in api_keys_list:
        if key_info.get("nickname") == active_nickname:
            return key_info.get("key"), active_nickname
    return None, active_nickname # Nickname exists but key not found in list

def _manage_gemini_api_keys_interactive_menu(gemini_config_part):
    """
    Manages Gemini API keys interactively (add, set active).
    Modifies gemini_config_part in place.
    Returns:
        bool: True if an active key is set and user wants to proceed, False if cancelled or no active key.
        str: The nickname of the active API key, or None.
    """
    api_keys_list = gemini_config_part.get("api_keys", [])
    active_api_key_nickname = gemini_config_part.get("active_api_key_nickname")

    # Ensure api_keys_list is part of the config if not present
    if "api_keys" not in gemini_config_part:
        gemini_config_part["api_keys"] = api_keys_list

    # API Key Management Loop
    while True:
        display_message("\n--- Gemini API Key Management ---", "blue")
        if not api_keys_list:
            display_message("No Gemini API keys configured yet.", "yellow")
        else:
            display_message("Configured Gemini API Keys:", "yellow")
            # Determine the maximum nickname length for padding
            max_nick_len = 0
            if api_keys_list: # Ensure list is not empty
                max_nick_len = max(len(key_info.get('nickname', f'Key {idx+1}')) for idx, key_info in enumerate(api_keys_list))

            for i, key_info in enumerate(api_keys_list):
                nick = key_info.get('nickname', f'Key {i+1}')
                key_val = key_info.get('key', 'N/A')
                # Display the full key as requested
                display_key_val = key_val # No longer masked
                active_marker = " (active)" if nick == active_api_key_nickname else ""
                # Pad the nickname for alignment
                # The f-string {nick:<{max_nick_len}} left-aligns nick in a space of max_nick_len
                print(f"  {i+1}. {nick:<{max_nick_len}} - Key: {display_key_val}{active_marker}")

        print("\nOptions:")
        print("  1. Add a new API key")
        if api_keys_list:
            print("  2. Set an API key as active")
            print("  3. Remove an API key")
        # Condition for 'Continue' option: only if an active key is actually set and valid
        if active_api_key_nickname and _get_key_value_by_nickname(active_api_key_nickname, api_keys_list):
            print("  C. Continue / Confirm selection")
        print("  X. Cancel / Exit key management")

        choice = input("Enter your choice: ").strip().lower()

        if choice == '1': # Add new API key
            new_nickname = ""
            while not new_nickname: # Loop until a valid, unique nickname is provided
                new_nickname = input("Enter a unique nickname for this API key: ").strip()
                if not new_nickname:
                    display_message("Nickname cannot be empty.", "red")
                elif _find_key_by_nickname(new_nickname, api_keys_list): # Check for uniqueness
                    display_message(f"Nickname '{new_nickname}' already exists. Please choose another.", "red")
                    new_nickname = "" # Reset to re-prompt

            new_api_key_value = input("Enter the new Gemini API Key: ").strip()
            if not new_api_key_value:
                display_message("API Key value cannot be empty. Aborting add.", "red")
                continue
            
            api_keys_list.append({"nickname": new_nickname, "key": new_api_key_value})
            display_message(f"API key '{new_nickname}' added.", "green")
            if not active_api_key_nickname or len(api_keys_list) == 1: # Auto-activate if first key or no active key
                active_api_key_nickname = new_nickname
                gemini_config_part["active_api_key_nickname"] = active_api_key_nickname
                display_message(f"'{new_nickname}' has been set as the active API key.", "blue")

        elif choice == '2' and api_keys_list: # Set active API key
            if len(api_keys_list) == 1:
                active_api_key_nickname = api_keys_list[0]["nickname"]
                gemini_config_part["active_api_key_nickname"] = active_api_key_nickname
                display_message(f"'{active_api_key_nickname}' is already the only key and is active.", "blue")
                continue

            display_message("Select an API key to set as active:", "yellow")
            for i, key_info_display in enumerate(api_keys_list): # Renamed to avoid conflict
                print(f"  {i+1}. {key_info_display['nickname']}")
            
            try:
                key_choice_idx = int(input("Enter number of the key to activate: ").strip()) - 1
                if 0 <= key_choice_idx < len(api_keys_list):
                    active_api_key_nickname = api_keys_list[key_choice_idx]["nickname"]
                    gemini_config_part["active_api_key_nickname"] = active_api_key_nickname
                    display_message(f"API key '{active_api_key_nickname}' is now active.", "green")
                else:
                    display_message("Invalid selection.", "red")
            except ValueError:
                display_message("Invalid input. Please enter a number.", "red")

        elif choice == '3' and api_keys_list: # Remove an API key
            display_message("Select an API key to remove:", "yellow")
            for i, key_info_remove in enumerate(api_keys_list): # Renamed to avoid conflict
                print(f"  {i+1}. {key_info_remove['nickname']}")
            print(f"  {len(api_keys_list) + 1}. Cancel removal")
            
            try:
                key_choice_idx_remove = int(input("Enter number of the key to remove: ").strip()) - 1
                if 0 <= key_choice_idx_remove < len(api_keys_list):
                    removed_key_info = api_keys_list.pop(key_choice_idx_remove)
                    removed_nickname = removed_key_info['nickname']
                    display_message(f"API key '{removed_nickname}' has been removed.", "green")

                    # Store what was active before this removal operation
                    previously_active_nickname_before_removal_logic = active_api_key_nickname

                    # If the removed key was the active one, clear the active status for now
                    if removed_nickname == previously_active_nickname_before_removal_logic:
                        active_api_key_nickname = None
                        gemini_config_part["active_api_key_nickname"] = None
                    
                    # Now, handle auto-activation or messaging based on remaining keys
                    if len(api_keys_list) == 1:
                        # If only one key remains, it automatically becomes active
                        current_sole_key_nickname = api_keys_list[0]['nickname']
                        if active_api_key_nickname != current_sole_key_nickname: # Update if not already this one
                            active_api_key_nickname = current_sole_key_nickname
                            gemini_config_part["active_api_key_nickname"] = active_api_key_nickname
                            display_message(f"'{active_api_key_nickname}' is now the active API key as it's the only one remaining.", "blue")
                    elif not api_keys_list:
                        # No keys left, active_api_key_nickname is already None if the last one was removed.
                        display_message("All API keys have been removed. No active key is set.", "yellow")
                    elif removed_nickname == previously_active_nickname_before_removal_logic: 
                        # Multiple keys remain, and the one removed WAS the active one
                        display_message(f"The active API key ('{removed_nickname}') was removed. Please set a new active key from the remaining options.", "yellow")
                    # If a non-active key was removed and multiple keys (including the active one) remain, no further message needed.

                elif key_choice_idx_remove == len(api_keys_list): # Cancel option
                    display_message("Removal cancelled.", "yellow")
                else:
                    display_message("Invalid selection for removal.", "red")
            except ValueError:
                display_message("Invalid input. Please enter a numerical value.", "red")

        elif choice == 'c' and active_api_key_nickname and _get_key_value_by_nickname(active_api_key_nickname, api_keys_list):
            gemini_config_part["active_api_key_nickname"] = active_api_key_nickname # Ensure it's saved
            return True, active_api_key_nickname # Proceed
        elif choice == 'x':
            display_message("Gemini key management cancelled.", "yellow")
            # Return current active key status, even if none, to reflect state before cancellation
            return bool(active_api_key_nickname and _get_key_value_by_nickname(active_api_key_nickname, api_keys_list)), active_api_key_nickname
        else:
            display_message("Invalid choice. Please try again.", "red")

def _setup_gemini_service(config_data):
    """Sets up or updates Gemini service configuration."""
    display_message("\n--- Gemini Service Setup ---", "green")
    display_message("You'll need a Gemini API key.", "yellow")
    display_message("You can obtain one from Google AI Studio: https://aistudio.google.com/app/apikey", "blue")

    # Ensure Gemini service config structure exists
    if LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}):
        if "llm_services" not in config_data:
            config_data["llm_services"] = {}
        config_data["llm_services"][LLM_SERVICE_GEMINI] = {"api_keys": [], "active_api_key_nickname": None, "model": None}
    
    gemini_config_part = config_data["llm_services"][LLM_SERVICE_GEMINI]

    proceed_with_active_key, active_api_key_nickname = _manage_gemini_api_keys_interactive_menu(gemini_config_part)

    if not proceed_with_active_key or not active_api_key_nickname:
        display_message("No active Gemini API key selected or setup cancelled. Cannot proceed with model selection.", "yellow")
        return None # Indicates setup was not fully completed or was cancelled

    current_active_api_key_value = _get_key_value_by_nickname(active_api_key_nickname, gemini_config_part["api_keys"])
    if not current_active_api_key_value: # Should be caught by proceed_with_active_key
        # This block seems to have a copy-paste error from _get_key_value_by_nickname
        # It should just be an error message if current_active_api_key_value is None
        # For example:
        # display_message("Error: Active API key value could not be retrieved. Cannot proceed.", "red")
        # return None
        # The original code here was:
        # for item in keys_list:
        #     if item.get("nickname") == nickname:
        #         return item.get("key")
        # return None
        # This is incorrect in this context.
        # The following is also incorrect as it's an infinite loop.
        # while True:
        display_message("Error: No active API key found. Cannot proceed.", "red")
        return None

    display_message(f"\nFetching models using active API key: '{active_api_key_nickname}'", "blue")
    models = None
    while True:
        try:
            models = fetch_gemini_models(current_active_api_key_value)
            if models is None: # Connection or critical error (e.g. invalid key)
                retry = input("Failed to fetch Gemini models. Check API key and connection. Try again? (Y/n): ").strip().lower()
                if retry == 'n':
                    return None # User chose not to retry
                continue
            elif not models: # Connected, key might be valid, but no suitable models
                display_message(f"No suitable models found for API key '{active_api_key_nickname}'. You might need to enable models in your Google Cloud project or use a different key.", "yellow")
                retry = input("Manage API keys again or skip Gemini setup? (M to manage keys / S to skip): ").strip().lower()
                if retry == 'n': # 's' was intended for skip
                    return None
                elif retry == 'm': # Go back to API key management loop
                    # This requires restructuring or a goto-like mechanism.
                    # For now, returning None and letting user re-run setup is simpler.
                    display_message("Please re-run setup to manage API keys or select a different option.", "yellow")
                    return None # Or re-enter the _manage_gemini_api_keys_interactive_menu if structured differently
                continue # Re-tries fetching with the same key, or user can cancel the retry prompt.
            else:
                break
        except KeyboardInterrupt:
            display_message("\nGemini setup cancelled by user.", "yellow")
            return None

    display_message("\nAvailable Gemini Models (e.g., models/gemini-pro):", "green")
    display_models = [m.split('/')[-1] for m in models] # e.g. "gemini-pro"
    for i, model_display_name in enumerate(display_models):
        print(f"  {i+1}. {model_display_name} (Full ID: {models[i]})")

    chosen_model_index = -1
    current_gemini_model_full_name = gemini_config_part.get("model") # Stored as full name

    default_model_prompt = ""
    if current_gemini_model_full_name and current_gemini_model_full_name in models:
        try:
            current_model_display_name = current_gemini_model_full_name.split('/')[-1]
            default_model_prompt = f" (current: {current_model_display_name})"
        except: # Should not happen if format is models/name
             default_model_prompt = f" (current: {current_gemini_model_full_name})"

    while not (0 <= chosen_model_index < len(models)):
        try:
            choice_prompt = f"Enter the number of the Gemini model you want to use (1-{len(models)}){default_model_prompt}: "
            choice = input(choice_prompt).strip()

            if not choice and current_gemini_model_full_name and current_gemini_model_full_name in models:
                chosen_model_index = models.index(current_gemini_model_full_name)
                display_message(f"Using current default model: {models[chosen_model_index].split('/')[-1]}", "blue")
                break

            chosen_model_index = int(choice) - 1
            if not (0 <= chosen_model_index < len(models)):
                display_message("Invalid model number. Please enter a number from the list.", "red")
        except ValueError:
            display_message("Invalid input. Please enter a numerical value.", "red")
        except KeyboardInterrupt:
            display_message("\nModel selection cancelled by user.", "yellow")
            return None

    chosen_model_full_name = models[chosen_model_index] # Store the full "models/name"
    # Update the model in the gemini_config_part which is a reference to config_data's Gemini section
    gemini_config_part["model"] = chosen_model_full_name
    # active_api_key_nickname and api_keys are already updated in gemini_config_part by _manage_gemini_api_keys_interactive_menu

    # No need to reassign config_data["llm_services"][LLM_SERVICE_GEMINI] as gemini_config_part is a direct reference

    display_message(f"Gemini service configured with model: {chosen_model_full_name.split('/')[-1]}", "green")
    return config_data

def save_config(config_file_path, config_data_to_save):
    """
    Saves the provided configuration data to the specified file.
    Returns True on success, False on failure.
    """
    try:
        with open(config_file_path, 'w') as f:
            json.dump(config_data_to_save, f, indent=4)
        display_message(f"\nConfiguration saved to {config_file_path}", "green")
        return True
    except Exception as e:
        display_message(f"Error saving configuration: {e}", "red")
        return False

def setup_config(config_dir, config_file, service_to_setup=None, is_initial_setup=False):
    """
    Manages the setup or update of LLM service configurations.
    """
    config_dir.mkdir(parents=True, exist_ok=True)
    config_data = load_config(config_file)
    if config_data is None:
        config_data = get_default_config_structure()
        is_initial_setup = True # Force this if config was missing

    if is_initial_setup:
        display_message("\n--- DeepShell Initial Configuration Setup ---", "green")
        display_message("It looks like this is your first time or the configuration is missing/empty.", "yellow")
        display_message(f"We'll guide you to set up an LLM service. Config will be at: {config_file}", "yellow")

    if not service_to_setup:
        display_message("\nWhich LLM service would you like to configure?", "blue")
        options = {
            "1": {"name": "Ollama", "value": LLM_SERVICE_OLLAMA},
            "2": {"name": "Gemini (Google AI)", "value": LLM_SERVICE_GEMINI}
        }
        for key, opt in options.items():
            is_configured = opt["value"] in config_data.get("llm_services", {})
            configured_marker = "(already configured)" if is_configured else ""
            print(f"  {key}. {opt['name']} {configured_marker}")

        while True:
            try:
                choice = input("Enter your choice (number): ").strip()
                if choice in options:
                    service_to_setup = options[choice]["value"]
                    break
                else:
                    display_message("Invalid choice. Please select a number from the list.", "red")
            except KeyboardInterrupt:
                display_message("\nSetup cancelled by user.", "yellow")
                return config_data # Return current config, might be partially modified or empty

    updated_config_data = None # Use a different variable name to avoid confusion with outer scope config_data
    if service_to_setup == LLM_SERVICE_OLLAMA:
        # _setup_ollama_service modifies config_data directly if successful
        if _setup_ollama_service(config_data):
             updated_config_data = config_data
    elif service_to_setup == LLM_SERVICE_GEMINI:
        # _setup_gemini_service modifies config_data directly if successful
        if _setup_gemini_service(config_data):
            updated_config_data = config_data
    else:
        display_message(f"Unsupported service type: {service_to_setup}", "red")
        return config_data # Return original data if service is unknown

    if updated_config_data is None: # Setup for the chosen service was cancelled or failed
        display_message(f"{service_to_setup.capitalize()} setup was not completed.", "yellow")
        return config_data 

    config_data = updated_config_data # Adopt the changes

    configured_services = list(config_data.get("llm_services", {}).keys())
    if service_to_setup in configured_services:
        if not config_data.get("active_llm_service") or len(configured_services) == 1:
            config_data["active_llm_service"] = service_to_setup
            display_message(f"\nSet {service_to_setup.capitalize()} as the active LLM service.", "blue")

    if save_config(config_file, config_data):
        return config_data
    return None # Indicate save failure

def load_config(config_file):
    """
    Loads configuration from the gollama.conf file.
    Returns the config dictionary or None if the file is not found or invalid.
    """
    try:
        with open(config_file, 'r') as f:
            
            config = json.load(f)
        return config
    except FileNotFoundError:
        return None # Return None to indicate config needs to be set up
    except json.JSONDecodeError:
        display_message(f"Error: Invalid JSON format in config file {config_file}. Please delete it and run DeepShell again to reconfigure.", "red")
        sys.exit(1) # Exit if config file is corrupted
    except Exception as e:
        display_message(f"Error loading configuration: {e}", "red")
        sys.exit(1)

def change_active_model_config(config_dir, config_file):
    """
    Allows the user to change the default model for the active LLM service.
    """
    display_message("\n--- Change Default Model for Active Service ---", "green")
    config_data = load_config(config_file)

    if not config_data or not config_data.get("active_llm_service"):
        display_message("No active LLM service configured. Please run --setup or --llm to configure and select a service.", "red")
        if config_data is None: # No config file at all
            run_setup = input("No configuration found. Run initial setup now? (Y/n): ").strip().lower()
            if run_setup == 'y' or run_setup == '':
                config_data = setup_config(config_dir, config_file, is_initial_setup=True)
                if not config_data or not config_data.get("active_llm_service"):
                    display_message("Setup incomplete or no active service selected. Cannot change model.", "red")
                    sys.exit(1)
            else:
                sys.exit(1)
        else: # Config exists but no active service
            sys.exit(1)

    active_service = config_data["active_llm_service"]
    service_config = config_data.get("llm_services", {}).get(active_service)

    if not service_config:
        display_message(f"Configuration for active service '{active_service}' not found. Please run --setup or --llm.", "red")
        sys.exit(1)

    display_message(f"Changing model for active service: {active_service.capitalize()}", "blue")

    models = []
    if active_service == LLM_SERVICE_OLLAMA:
        server_address = service_config.get("server_address")
        if not server_address:
            display_message("Ollama server address not configured for the active service. Please re-run setup for Ollama.", "red")
            sys.exit(1)
        display_message(f"Fetching Ollama models from: {server_address}", "blue")
        models = fetch_ollama_models(server_address)
    elif active_service == LLM_SERVICE_GEMINI:
        active_api_key_value, active_nickname = _get_active_gemini_key_value(service_config)
        if not active_api_key_value:
            display_message("No active Gemini API key configured or found. Please re-run setup for Gemini.", "red")
            sys.exit(1)

        display_message(f"Fetching Gemini models (using active key: '{active_nickname}')...", "blue")
        models = fetch_gemini_models(active_api_key_value)

    if models is None or not models:
        display_message(f"Could not retrieve models for {active_service.capitalize()}. Cannot change model.", "red")
        display_message("Please check service configuration (server/API key) and connectivity.", "yellow")
        sys.exit(1)

    display_message(f"\nAvailable Models for {active_service.capitalize()}:", "green")
    display_names = models
    if active_service == LLM_SERVICE_GEMINI: # Gemini models are "models/name", show "name"
        display_names = [m.split('/')[-1] for m in models]

    # Loop through the models. 'models' contains the actual identifiers.
    # 'display_names' contains what should be shown to the user.
    for i, model_identifier in enumerate(models): # model_identifier is models[i]
        is_current = (models[i] == service_config.get("model"))
        print(f"  {i+1}. {display_names[i]} {'(current default)' if is_current else ''}")

    chosen_model_index = -1
    while not (0 <= chosen_model_index < len(models)):
        try:
            choice = input(f"Enter the number of the new default model for {active_service.capitalize()} (1-{len(models)}): ").strip()
            chosen_model_index = int(choice) - 1
            if not (0 <= chosen_model_index < len(models)):
                display_message("Invalid model number. Please enter a number from the list.", "red")
        except ValueError:
            display_message("Invalid input. Please enter a numerical value.", "red")
        except KeyboardInterrupt:
            display_message("\nModel selection cancelled by user.", "yellow")
            sys.exit(0)

    new_model_name = models[chosen_model_index] # For Gemini, this is "models/name"
    config_data["llm_services"][active_service]["model"] = new_model_name

    if save_config(config_file, config_data):
        display_model_show_name = new_model_name.split('/')[-1] if active_service == LLM_SERVICE_GEMINI else new_model_name
        display_message(f"\nDefault model for {active_service.capitalize()} successfully changed to: {display_model_show_name}", "green")
    else:
        sys.exit(1)

def switch_llm_service(config_dir, config_file):
    """Allows the user to switch the active LLM service or configure a new one."""
    display_message("\n--- Switch/Configure LLM Service ---", "green")
    config_data = load_config(config_file)
    if config_data is None:
        config_data = get_default_config_structure()

    configured_services = list(config_data.get("llm_services", {}).keys())
    current_active_service = config_data.get("active_llm_service")

    options = {}
    next_opt_num = 1

    if configured_services:
        display_message("Currently configured LLM services:", "blue")
        for service_name in configured_services:
            options[str(next_opt_num)] = {"name": service_name.capitalize(), "action": "switch", "value": service_name}
            active_marker = "(active)" if service_name == current_active_service else ""
            print(f"  {next_opt_num}. Switch to {service_name.capitalize()} {active_marker}")
            next_opt_num += 1
    else:
        display_message("No LLM services are currently configured.", "yellow")

    display_message("\nOther options:", "blue")
    for service_type_const in SUPPORTED_LLM_SERVICES:
        action_verb = "Reconfigure" if service_type_const in configured_services else "Configure"
        options[str(next_opt_num)] = {"name": f"{action_verb} {service_type_const.capitalize()}", "action": "setup", "value": service_type_const}
        print(f"  {next_opt_num}. {action_verb} {service_type_const.capitalize()}")
        next_opt_num += 1
    
    options[str(next_opt_num)] = {"name": "Cancel", "action": "cancel", "value": None}
    print(f"  {next_opt_num}. Cancel")

    while True:
        try:
            choice = input("Enter your choice (number): ").strip()
            if choice in options:
                selected_option = options[choice]
                break
            else:
                display_message("Invalid choice. Please select a number from the list.", "red")
        except KeyboardInterrupt:
            display_message("\nOperation cancelled by user.", "yellow")
            sys.exit(0)

    action = selected_option["action"]
    value = selected_option["value"]

    if action == "cancel":
        display_message("Operation cancelled.", "yellow")
        sys.exit(0)
    elif action == "switch":
        config_data["active_llm_service"] = value
        display_message(f"Switched active LLM service to: {value.capitalize()}", "green")
    elif action == "setup":
        # Pass current config_data to setup_config so it can be modified
        new_config_data = setup_config(config_dir, config_file, service_to_setup=value)
        if new_config_data and value in new_config_data.get("llm_services", {}):
            config_data = new_config_data # Update local config_data with changes from setup
            # If the newly setup/reconfigured service is not active, ask to make it active
            if config_data.get("active_llm_service") != value:
                activate_choice = input(f"Make {value.capitalize()} the active LLM service? (Y/n): ").strip().lower()
                if activate_choice == 'y' or activate_choice == '':
                    config_data["active_llm_service"] = value
                    display_message(f"Set {value.capitalize()} as active LLM service.", "green")
        else:
            display_message(f"Setup for {value.capitalize()} was not completed or failed to save.", "yellow")
            sys.exit(1) # Exit if setup failed critically

    # Save any changes to active_llm_service or from a successful setup
    if not save_config(config_file, config_data):
        sys.exit(1) # Exit if save failed

def delete_config_file(config_file):
    """
    Prompts the user for confirmation and deletes the configuration file if confirmed.
    """
    display_message("\n--- Delete Configuration File ---", "orange")
    if not config_file.exists():
        display_message(f"Configuration file not found at {config_file}. Nothing to delete.", "yellow")
        sys.exit(0)

    try:
        confirm = input(f"Are you sure you want to delete the configuration file at {config_file}? (Y/n): ").strip().lower()
        if confirm == 'y' or confirm == '': # Default to 'yes' if user presses Enter
            try:
                config_file.unlink() # Deletes the file
                display_message(f"Configuration file {config_file} has been deleted.", "green")
            except OSError as e:
                display_message(f"Error deleting configuration file: {e}", "red")
                sys.exit(1)
        else:
            display_message("Deletion cancelled by user.", "yellow")
        sys.exit(0) # Exit after attempting deletion or cancellation
    except KeyboardInterrupt:
        display_message("\nDeletion process cancelled by user. Exiting.", "yellow")
        sys.exit(0)
    except Exception as e: # Catch any other unexpected errors
        display_message(f"An unexpected error occurred during the delete confirmation: {e}", "red")
        sys.exit(1)

def _animate_progress(stop_event, message_length):
    """
    Displays a simple command-line progress animation (moving wave of blue dots).
    This function is intended to be run in a separate thread.
    The animation width will match the provided message_length.
    """
    if message_length <= 0: # Safety check
        message_length = 20 # Default to a reasonable length if 0 or negative

    dot_char = "●"
    empty_char = "○"
    
    idx = 0
    direction = 1 # 1 for right, -1 for left
    position = 0

    while not stop_event.is_set():
        # Create the animation string
        progress_bar = [empty_char] * message_length
        if 0 <= position < message_length:
            progress_bar[position] = dot_char
        
        pattern = "".join(progress_bar)
        # Using blue color for the wave
        output_str = f"{COLORS['blue']}{pattern}{COLORS['reset']}"
        
        # No need for extra spaces if pattern itself is message_length
        sys.stdout.write(f"\r{output_str}") 
        sys.stdout.flush()
        
        position += direction
        if position >= message_length -1 or position <= 0:
            direction *= -1 # Reverse direction
            # Ensure position stays within bounds after reversing
            position = max(0, min(position, message_length - 1))
            
        time.sleep(0.15) # Adjust for desired speed

def send_ollama_query(server_address, model_name, user_query):
    """
    Sends the user's query to the OLLAMA server's /api/chat endpoint
    and prints only the relevant response content.
    """
    sending_message = f"Sending query to {server_address} (Model: {model_name})..."
    display_message(f"\n{sending_message}", "blue")
    message_len_for_animation = len(sending_message)

    url = f"{server_address}/api/chat"
    payload = {
        "model": model_name,
        "messages": [{"role": "user", "content": user_query}],
        "stream": False # Set to false as per requirement for single response
    }

    stop_event = threading.Event()
    animation_thread = threading.Thread(target=_animate_progress, args=(stop_event, message_len_for_animation))
    animation_thread.daemon = True # Ensure thread exits when main program does
    animation_thread.start()

    response_data = None
    error_occurred = False
    http_response_obj = None # To store the response object for error messages
    # Specific error variables to hold exception instances
    _connection_err_for_display = None
    _timeout_err_for_display = None
    _http_err_for_display = None
    _json_decode_err_for_display = None
    _generic_err_for_display = None


    try:
        http_response_obj = requests.post(url, json=payload, timeout=120) 
        http_response_obj.raise_for_status()
        response_data = http_response_obj.json()
    except requests.exceptions.ConnectionError as e:
        error_occurred = True
        _connection_err_for_display = e
    except requests.exceptions.Timeout as e:
        error_occurred = True
        _timeout_err_for_display = e
    except requests.exceptions.HTTPError as e:
        error_occurred = True
        _http_err_for_display = e
    except json.JSONDecodeError as e:
        error_occurred = True
        _json_decode_err_for_display = e
        # http_response_obj might be available here if status was 200 but content was not JSON
    except Exception as e:
        error_occurred = True
        _generic_err_for_display = e 
    finally:
        stop_event.set()
        animation_thread.join(timeout=0.5) 
        sys.stdout.write("\r" + " " * message_len_for_animation + "\r") 
        sys.stdout.flush()

    if error_occurred:
        if _connection_err_for_display:
            display_message(f"Connection Error: Could not connect to {server_address}. Is the server running and accessible?", "red")
        elif _timeout_err_for_display:
            display_message(f"Request Timeout: The server at {server_address} did not respond in time.", "red")
        elif _http_err_for_display:
            display_message(f"HTTP Error during query: {_http_err_for_display} - Server response: {getattr(_http_err_for_display.response, 'text', 'No response text')}", "red")
        elif _json_decode_err_for_display:
            resp_text = getattr(http_response_obj, 'text', 'No response object available for text.') if http_response_obj else 'No response object.'
            display_message(f"JSON Decode Error: Could not parse response from server. Response text: {resp_text}", "red")
        elif _generic_err_for_display:
             display_message(f"An unexpected error occurred during query: {_generic_err_for_display}", "red")
        else: # Should not be reached if one of the above is true
            display_message("An unspecified error occurred during the query.", "red")

    elif response_data: 
        if "message" in response_data and "content" in response_data["message"]:
            display_message("\n--- OLLAMA Response ---", "green")
            print(response_data["message"]["content"].strip())
            display_message("\n-----------------------", "green")
        else:
            display_message(f"Error: Unexpected response format from server. Full response: {json.dumps(response_data, indent=2)}", "orange")


def send_gemini_query(api_key, model_name, user_query):
    """
    Sends the user's query to the Gemini API and prints the response.
    Model name should be the full "models/gemini-pro" style name.
    """
    sending_message = f"Sending query to Gemini (Model: {model_name.split('/')[-1]})..."
    display_message(f"\n{sending_message}", "blue")
    message_len_for_animation = len(sending_message)

    if not model_name.startswith("models/"):
        model_name_for_api = f"models/{model_name}"
    else:
        model_name_for_api = model_name

    url = f"{GEMINI_API_BASE_URL}/{model_name_for_api}:generateContent?key={api_key}"
    payload = {
        "contents": [{"parts":[{"text": user_query}]}]
    }

    stop_event = threading.Event()
    animation_thread = threading.Thread(target=_animate_progress, args=(stop_event, message_len_for_animation))
    animation_thread.daemon = True
    animation_thread.start()

    response_data = None
    error_message_to_display = None # Use a single variable for the error message
    try:
        response = requests.post(url, json=payload, timeout=120)
        response.raise_for_status() 
        response_data = response.json()
    except requests.exceptions.RequestException as e:
        error_message_to_display = f"Error during Gemini query: {e}"
        if hasattr(e, 'response') and e.response is not None:
            try:
                err_details = e.response.json().get("error", {}).get("message", e.response.text)
                error_message_to_display += f" - Server detail: {err_details}"
            except json.JSONDecodeError:
                error_message_to_display += f" - Server response: {e.response.text}"
    except json.JSONDecodeError:
        error_message_to_display = "Error: Could not parse JSON response from Gemini API."
    finally:
        stop_event.set()
        animation_thread.join(timeout=0.5)
        sys.stdout.write("\r" + " " * message_len_for_animation + "\r")
        sys.stdout.flush()

    if response_data and 'candidates' in response_data and response_data['candidates'] and \
       'content' in response_data['candidates'][0] and \
       'parts' in response_data['candidates'][0]['content'] and response_data['candidates'][0]['content']['parts'] and \
       'text' in response_data['candidates'][0]['content']['parts'][0]:
        display_message("\n--- Gemini Response ---", "green")
        print(response_data['candidates'][0]['content']['parts'][0]['text'].strip())
        display_message("\n-----------------------", "green")
    elif error_message_to_display: 
        display_message(error_message_to_display, "red")
    elif not response_data and not error_message_to_display: # No specific error caught, but no data
        display_message("Error: Received an empty or unexpected response from Gemini API.", "orange")
    elif response_data: # Data received but not in expected format
        display_message(f"Error: Unexpected response format from Gemini. Full response: {json.dumps(response_data, indent=2)}", "orange")


def check_gemini_quota_status(config_dir, config_file):
    """
    Checks the status of the configured Gemini API key and displays general quota information.
    """
    display_message("\n--- Gemini API Key & Quota Info ---", "green")
    config_data = load_config(config_file)

    if not config_data or not config_data.get("active_llm_service"):
        display_message("No active LLM service configured. Please run --setup or --llm first.", "red")
        if config_data is None: 
            run_setup = input("No configuration found. Run initial setup now? (Y/n): ").strip().lower()
            if run_setup == 'y' or run_setup == '':
                config_data = setup_config(config_dir, config_file, is_initial_setup=True)
                if not config_data or not config_data.get("active_llm_service"):
                    display_message("Setup incomplete. Exiting.", "red")
                    sys.exit(1)
            else:
                sys.exit(1)
        else: 
            sys.exit(1) 

    active_service = config_data.get("active_llm_service")
    gemini_config = config_data.get("llm_services", {}).get(LLM_SERVICE_GEMINI, {})

    if active_service != LLM_SERVICE_GEMINI:
        display_message(f"The active LLM service is '{active_service.capitalize()}', not Gemini.", "yellow")
        display_message("This option is for the Gemini service. Switch services using --llm or configure Gemini via --setup.", "yellow")
        sys.exit(0)
    
    active_api_key_value, active_nickname = _get_active_gemini_key_value(gemini_config)

    if not active_api_key_value:
        display_message("Gemini service is active, but no active API key is configured or found.", "red")
        display_message("Please (re)configure the Gemini service using --setup or --llm.", "yellow")
        sys.exit(1)

    display_message(f"Active Gemini API Key Nickname: {active_nickname}", "blue")
    display_message(f"Active Gemini API Key Value   : {active_api_key_value}", "blue")

    fetch_gemini_models(active_api_key_value)

    display_message("To check your Gemini API usage, please visit:", "blue") 
    display_message("https://aistudio.google.com/app/usage", "yellow")

def show_active_gemini_api_key(config_dir, config_file):
    """Displays the active Gemini API key's nickname and full value."""
    display_message("\n--- Active Gemini API Key ---", "green")
    config_data = load_config(config_file)

    if not config_data or LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}):
        display_message("Gemini service not configured. Use --setup or --llm to configure it.", "yellow")
        sys.exit(1)

    gemini_config = config_data["llm_services"][LLM_SERVICE_GEMINI]
    active_key_value, active_nickname = _get_active_gemini_key_value(gemini_config)

    if active_key_value and active_nickname:
        display_message(f"Active Nickname: {active_nickname}", "blue")
        display_message(f"API Key Value  : {active_key_value}", "blue")
    elif active_nickname:
        display_message(f"Active Nickname: {active_nickname} (but key value not found in list!)", "orange")
        display_message("Please reconfigure Gemini API keys using --set-api-key or --setup.", "yellow")
    else:
        display_message("No active Gemini API key is set.", "yellow")
        display_message("Use --set-api-key or --setup to configure Gemini API keys.", "yellow")
    sys.exit(0)

def set_gemini_api_key_interactive(config_dir, config_file):
    """Allows interactive management (add/set active) of Gemini API keys."""
    display_message("\n--- Set/Manage Gemini API Keys ---", "green")
    config_data = load_config(config_file)
    if config_data is None:
        display_message("No configuration file found. Running initial setup for Gemini first...", "yellow")
        config_data = setup_config(config_dir, config_file, service_to_setup=LLM_SERVICE_GEMINI, is_initial_setup=True)
        if not config_data or LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}):
            display_message("Gemini setup failed or was cancelled. Cannot manage keys.", "red")
            sys.exit(1)

    if LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}):
        if "llm_services" not in config_data: 
            config_data["llm_services"] = {}
        config_data["llm_services"][LLM_SERVICE_GEMINI] = {"api_keys": [], "active_api_key_nickname": None, "model": None}

    gemini_config_part = config_data["llm_services"][LLM_SERVICE_GEMINI]
    # Ensure api_keys and active_api_key_nickname exist in gemini_config_part before passing
    if "api_keys" not in gemini_config_part:
        gemini_config_part["api_keys"] = []
    if "active_api_key_nickname" not in gemini_config_part:
        gemini_config_part["active_api_key_nickname"] = None

    proceed, _ = _manage_gemini_api_keys_interactive_menu(gemini_config_part)

    # Save if the menu indicates changes were made or user confirmed
    # The menu itself modifies gemini_config_part which is a reference to config_data's section
    if proceed or gemini_config_part.get("active_api_key_nickname") is not None or gemini_config_part.get("api_keys"):
        save_config(config_file, config_data)
    sys.exit(0)

def show_active_configuration(config_dir, config_file):
    """Displays the details of the currently active LLM configuration."""
    display_message("\n--- Active DeepShell Configuration ---", "green")
    config_data = load_config(config_file)

    if not config_data:
        display_message("No configuration file found. Please run --setup.", "yellow")
        sys.exit(1)

    # Determine the maximum label length for alignment
    labels = [
        "Active LLM Service:",
        "Active Model:",
        "Ollama Server:",
        "API Key Nickname:",
        "API Key Value:"
    ]
    max_label_len = max(len(label) for label in labels)

    active_service_name = config_data.get("active_llm_service")
    if not active_service_name:
        display_message("No active LLM service is set in the configuration.", "yellow")
        display_message("Use --llm to select or configure an LLM service.", "yellow")
        sys.exit(1)

    display_message(f"{'Active LLM Service:':<{max_label_len}} {active_service_name.capitalize()}", "blue")

    service_config = config_data.get("llm_services", {}).get(active_service_name)
    if not service_config:
        display_message(f"Configuration details for '{active_service_name}' are missing.", "orange")
        display_message("Please reconfigure this service using --setup or --llm.", "yellow")
        sys.exit(1)

    model_name = service_config.get("model")
    if model_name:
        # For Gemini, model_name is "models/gemini-pro", for Ollama it's just "llama2"
        display_model = model_name.split('/')[-1] if active_service_name == LLM_SERVICE_GEMINI else model_name
        full_id_display = f" (Full ID: {model_name})" if active_service_name == LLM_SERVICE_GEMINI and '/' in model_name else ""
        display_message(f"{'Active Model:':<{max_label_len}} {display_model}{full_id_display}", "blue")
    else:
        display_message(f"{'Active Model:':<{max_label_len}} Not set", "orange")

    if active_service_name == LLM_SERVICE_OLLAMA:
        server_address = service_config.get("server_address", "Not set")
        display_message(f"{'Ollama Server:':<{max_label_len}} {server_address}", "blue")
        # Example of splitting host and port if desired:
        # try:
        #     parsed_url = urlparse(server_address)
        #     display_message(f"{'Ollama Host:':<{max_label_len}} {parsed_url.hostname}", "blue")
        #     display_message(f"{'Ollama Port:':<{max_label_len}} {parsed_url.port}", "blue")
        # except:
        #     display_message(f"{'Ollama Server:':<{max_label_len}} {server_address} (could not parse host/port)", "blue")


    elif active_service_name == LLM_SERVICE_GEMINI:
        active_key_value, active_nickname = _get_active_gemini_key_value(service_config)
        if active_nickname and active_key_value:
            display_message(f"{'API Key Nickname:':<{max_label_len}} {active_nickname}", "blue")
            display_message(f"{'API Key Value:':<{max_label_len}} {active_key_value}", "blue")
        elif active_nickname: # Key value might be missing if list was tampered
            display_message(f"{'API Key Nickname:':<{max_label_len}} {active_nickname} (Key value missing!)", "orange")
        else:
            display_message(f"{'API Key:':<{max_label_len}} Not set", "orange")
    sys.exit(0)

import textwrap # Add this import

class CustomHelpFormatter(argparse.RawTextHelpFormatter):
    """Custom formatter for argparse help messages to adjust spacing."""
    def __init__(self, prog, indent_increment=2, max_help_position=50, width=None):
        # Increase max_help_position to give more space to options
        super().__init__(prog, indent_increment, max_help_position, width)

    def _split_lines(self, text, width):
        """
        Splits help text into lines.

        This method first splits the text by explicit newlines (respecting
        RawTextHelpFormatter behavior). Then, for each resulting line, if it's
        too long for the given 'width', it's wrapped using textwrap.
        The 'width' here is the available width for the help text column.
        """
        # Split by explicit newlines first
        raw_lines = text.splitlines()
        
        final_wrapped_lines = []
        for line in raw_lines:
            # If the raw line is empty, it's an intentional blank line from \n\n, preserve it.
            if not line:
                final_wrapped_lines.append('')
                continue

            # Wrap this (potentially long) line using textwrap.
            # textwrap.wrap returns a list of strings.
            # If line is empty or all whitespace, textwrap.wrap might return an empty list or list of space segments.
            # If line has content, it will be wrapped.
            sub_lines = textwrap.wrap(line, width)
            final_wrapped_lines.extend(sub_lines if sub_lines else [line]) # Add original line if wrap results in empty
        return final_wrapped_lines

    def add_arguments(self, actions):
        """
        Adds argument descriptions to the help message, sorted alphabetically for optional arguments.
        """
        if not actions:
            super().add_arguments(actions)
            return

        actions_to_format = list(actions) # Work with a copy

        # Heuristic: if the first action in the group has option strings,
        # assume it's a group of optional arguments and sort them.
        # Positional arguments (which have empty action.option_strings)
        # will retain their defined order within their group.
        if actions_to_format[0].option_strings:
            # Sort by the first option string (e.g., '-s' from ['-s', '--setup']).
            # This provides a natural alphabetical sort for CLI options.
            actions_to_format.sort(key=lambda action: action.option_strings[0])

        # Call the superclass's add_arguments with the (potentially sorted) list.
        # This will then call self.add_argument for each action, which uses _format_action.
        super().add_arguments(actions_to_format)

def main():
    """
    Main function to parse arguments, manage configuration, and execute the query.
    """
    try:
        # Set up argument parser for command-line input
        # Use the custom formatter
        parser = argparse.ArgumentParser(
            description="DeepShell: Query an OLLAMA server from the command line.",
            formatter_class=CustomHelpFormatter 
        )
        parser.add_argument(
            "-s", "--setup",
            action="store_true",
            help="Run the interactive configuration setup process, (re)creating the config file."
        )
        parser.add_argument(
            "-model", "--model-change", # Changed --model to -model, kept --model-change
            action="store_true",
            dest="model_change", # Ensure it maps to args.model_change
            help="Change the default model for the active LLM service."
        )
        parser.add_argument(
            "-q", "--query",
            nargs='+', # Captures one or more arguments into a list
            metavar="QUERY",
            help="The query to send to the OLLAMA server. All text following this flag will be treated as the query.\n"
                 "Example: python main.py -q What is the capital of France?"
        )
        parser.add_argument(
            "-d", "--delete-config",
            action="store_true",
            help="Delete the configuration file after confirmation."
        )
        parser.add_argument(
            "-show-key", "--show-api-key",
            action="store_true",
            dest="show_gemini_key",
            help="Show the active Gemini API key nickname and value."
        )
        parser.add_argument(
            "-set-key", "--set-api-key", # Corrected "--set-apt-key" to "--set-api-key"
            action="store_true",
            dest="set_gemini_key",
            help="Interactively set/manage Gemini API keys (add new, set active)."
        )
        parser.add_argument(
            "-gq", "--gemini-quota",
            action="store_true",
            dest="gemini_quota_check",
            help="Check Gemini API key status and display general quota information (for active Gemini service)."
        )
        parser.add_argument(
            "-l", "--llm",
            action="store_true",
            dest="switch_llm",
            help="Switch the active LLM service or configure LLM services."
        )
        parser.add_argument(
            "-show-config", "--show-full-conf", # New flag
            action="store_true",
            dest="show_config",
            help="Display the currently active LLM configuration details."
        )
        parser.add_argument(
            "-v", "--version",
            action="version",
            version=f"DeepShell Version {__version__}",
            help="Show program's version number and exit."
        )
        args = parser.parse_args()

        # Get paths for the config directory and file
        config_dir, config_file = get_config_path()

        # Handle --setup flag: run setup and exit
        if args.setup:
            setup_config(config_dir, config_file, is_initial_setup=True) # Force initial setup flow
            sys.exit(0)

        # Handle --model-change flag
        if args.model_change:
            change_active_model_config(config_dir, config_file)
            sys.exit(0)
        
        # Handle --llm flag
        if args.switch_llm:
            switch_llm_service(config_dir, config_file)
            sys.exit(0)
        
        # Handle --delete-config flag
        if args.delete_config:
            delete_config_file(config_file)
            sys.exit(0) 
        
        # Handle --gemini-quota flag
        if args.gemini_quota_check:
            check_gemini_quota_status(config_dir, config_file)
            sys.exit(0)
        
        # Handle --show-api-key
        if args.show_gemini_key:
            show_active_gemini_api_key(config_dir, config_file)
            sys.exit(0)

        # Handle --set-api-key
        if args.set_gemini_key:
            set_gemini_api_key_interactive(config_dir, config_file)
            sys.exit(0)
        
        # Handle --show-config flag
        if args.show_config:
            show_active_configuration(config_dir, config_file)
            sys.exit(0)

        # If not running an action flag, attempt to load existing configuration
        config_data = load_config(config_file)
        user_query_list = args.query
        user_query = " ".join(user_query_list) if user_query_list else None

        if config_data is None or not config_data.get("active_llm_service"):
            display_message("Configuration not found or no active LLM service. Running setup...", "yellow")
            config_data = setup_config(config_dir, config_file, is_initial_setup=True)
            if not config_data or not config_data.get("active_llm_service"):
                display_message("Setup incomplete or no active LLM service selected. Exiting.", "red")
                sys.exit(1)
            if not user_query:
                display_message("Setup complete. You can now run the program with a query or other options.", "green")
                sys.exit(0)

        if not user_query: 
            parser.print_help()
            display_message("\nError: A query is required (use -q YOUR_QUERY) if not using other action flags.", "red")
            sys.exit(1)

        active_service_name = config_data.get("active_llm_service")
        if not active_service_name or active_service_name not in config_data.get("llm_services", {}):
            display_message("No active LLM service configured or configuration is corrupted.", "red")
            display_message("Please run --setup or --llm to select/configure a service.", "yellow")
            sys.exit(1)

        service_config = config_data["llm_services"][active_service_name]
        model_name = service_config.get("model")

        if not model_name:
            display_message(f"No default model configured for the active service '{active_service_name}'.", "red")
            display_message("Please run --model-change or re-run --setup / --llm for this service.", "yellow")
            sys.exit(1)

        display_message(f"Using active LLM service: {active_service_name.capitalize()}", "blue")

        if active_service_name == LLM_SERVICE_OLLAMA:
            server_address = service_config.get("server_address")
            if not server_address:
                display_message("Ollama server address not configured. Please run --setup or --llm to configure Ollama.", "red")
                sys.exit(1)
            send_ollama_query(server_address, model_name, user_query)
        elif active_service_name == LLM_SERVICE_GEMINI:
            active_api_key_value, active_nickname = _get_active_gemini_key_value(service_config)
            if not active_api_key_value:
                display_message("No active Gemini API key configured or found. Please run --setup or --llm to configure Gemini.", "red")
                sys.exit(1)
            display_message(f"(Using API key: '{active_nickname}')", "blue")
            send_gemini_query(active_api_key_value, model_name, user_query)

    except KeyboardInterrupt:
        display_message("\nProgram interrupted by user. Exiting gracefully.", "yellow")
        sys.exit(0)

if __name__ == "__main__":
    main()
