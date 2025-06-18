import json
import sys
from pathlib import Path

from utils import display_message, LLM_SERVICE_OLLAMA, LLM_SERVICE_GEMINI, SUPPORTED_LLM_SERVICES
from ollama import _setup_ollama_service, fetch_ollama_models
from gemini import _setup_gemini_service, fetch_gemini_models, _get_active_gemini_key_value, _manage_gemini_api_keys_interactive_menu


# Configuration constants
CONFIG_DIR_NAME = ".deepshell"
CONFIG_FILE_NAME = "deepshell.conf"

def get_config_path():
    """
    Returns the full path to the configuration directory and file.
    The config will be stored in a '.deepshell' folder within the user's home directory.
    """
    home_dir = Path.home()
    config_dir = home_dir / CONFIG_DIR_NAME
    config_file = config_dir / CONFIG_FILE_NAME
    return config_dir, config_file

def get_default_config_structure():
    """Returns the basic structure for a new configuration file."""
    return {
        "active_llm_service": None,
        "llm_services": {}
    }

def load_config(config_file):
    """
    Loads configuration from the config file.
    Returns the config dictionary or None if the file is not found or invalid.
    """
    try:
        with open(config_file, 'r') as f:
            config = json.load(f)
        return config
    except FileNotFoundError:
        return None 
    except json.JSONDecodeError:
        display_message(f"Error: Invalid JSON format in config file {config_file}. Please delete it and run DeepShell again to reconfigure.", "red")
        sys.exit(1) 
    except Exception as e:
        display_message(f"Error loading configuration: {e}", "red")
        sys.exit(1)

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


def _select_service_to_configure(config_data, action_verb="configure"):
    """Helper to prompt user to select a service to configure/reconfigure."""
    display_message(f"\nWhich LLM service would you like to {action_verb}?", "blue")
    options = {}
    idx = 1
    for service_const in SUPPORTED_LLM_SERVICES:
        options[str(idx)] = service_const
        is_configured = service_const in config_data.get("llm_services", {})
        configured_marker = "(already configured)" if is_configured else ""
        display_message(f"  {idx}. {service_const.capitalize()} {configured_marker}")
        idx +=1
    
    options[str(idx)] = "cancel"
    display_message(f"  {idx}. Cancel / Back")

    while True:
        try:
            choice = input("Enter your choice (number): ").strip()
            if choice in options:
                if options[choice] == "cancel":
                    return None
                return options[choice]
            else:
                display_message("Invalid choice. Please select a number from the list.", "red")
        except KeyboardInterrupt:
            display_message("\nSelection cancelled.", "yellow")
            return None

def _configure_selected_service(service_const, config_data, config_file):
    """Calls the appropriate setup function and handles active service logic."""
    original_config_copy = json.loads(json.dumps(config_data)) # Deep copy for comparison
    
    if service_const == LLM_SERVICE_OLLAMA:
        updated_config_data = _setup_ollama_service(config_data) 
    elif service_const == LLM_SERVICE_GEMINI:
        updated_config_data = _setup_gemini_service(config_data) 
    else:
        return # Should not happen

    if updated_config_data: # If setup was successful (not cancelled)
        config_data = updated_config_data # Adopt the changes
        configured_services = list(config_data.get("llm_services", {}).keys())
        
        # Auto-activate if it's the first service or only service configured
        if service_const in config_data.get("llm_services", {}) and \
           (not config_data.get("active_llm_service") or len(configured_services) == 1 or config_data.get("active_llm_service") != service_const):
            current_active = config_data.get("active_llm_service")
            if current_active and current_active != service_const: # Store previous only if it's different and existed
                config_data["previous_active_llm_service"] = current_active
            config_data["active_llm_service"] = service_const
            display_message(f"\nSet {service_const.capitalize()} as the active LLM service.", "blue")
        
        # Save only if changes were made
        if config_data != original_config_copy:
            save_config(config_file, config_data)
        else:
            display_message(f"No changes made to {service_const.capitalize()} configuration.", "yellow")
    else:
        display_message(f"{service_const.capitalize()} setup was not completed or was cancelled.", "yellow")

def _handle_active_model_change(config_data, config_file):
    """Handles changing the model for the currently active service."""
    active_service = config_data.get("active_llm_service")
    if not active_service:
        display_message("No active LLM service. Please select or configure one first via the main settings menu.", "yellow")
        return

    service_config = config_data.get("llm_services", {}).get(active_service)
    if not service_config:
        display_message(f"Configuration for active service '{active_service}' not found.", "red")
        return

    display_message(f"\n--- Change Model for Active Service: {active_service.capitalize()} ---", "green")
    models = []
    if active_service == LLM_SERVICE_OLLAMA:
        server_address = service_config.get("server_address")
        if not server_address:
            display_message("Ollama server address not configured. Please reconfigure Ollama.", "red")
            return
        models = fetch_ollama_models(server_address)
    elif active_service == LLM_SERVICE_GEMINI:
        api_key, nick = _get_active_gemini_key_value(service_config)
        if not api_key:
            display_message("No active Gemini API key for model fetching. Please manage Gemini API keys.", "red")
            return
        models = fetch_gemini_models(api_key)

    if not models: # Covers None or empty list
        display_message(f"No models found for {active_service.capitalize()}. Cannot change model.", "yellow")
        return

    display_message(f"\nAvailable Models for {active_service.capitalize()}:", "green")
    display_names = [m.split('/')[-1] if active_service == LLM_SERVICE_GEMINI else m for m in models]
    current_model = service_config.get("model")
    for i, name in enumerate(display_names):
        is_current = (models[i] == current_model)
        print(f"  {i+1}. {name} {'(current default)' if is_current else ''}")

    try:
        choice = input(f"Enter number of the new default model (or 'c' to cancel): ").strip().lower()
        if choice == 'c':
            display_message("Model change cancelled.", "yellow")
            return
        idx = int(choice) - 1
        if 0 <= idx < len(models):
            if service_config.get("model") != models[idx]:
                service_config["model"] = models[idx]
                display_message(f"Default model for {active_service.capitalize()} set to {display_names[idx]}.", "green")
                save_config(config_file, config_data)
            else:
                display_message(f"Model for {active_service.capitalize()} is already {display_names[idx]}.", "yellow")
        else:
            display_message("Invalid selection.", "red")
    except ValueError:
        display_message("Invalid input. Please enter a number or 'c'.", "red")
    except KeyboardInterrupt:
        display_message("\nModel change cancelled.", "yellow")

def _handle_switch_active_llm(config_data, config_file):
    """Handles switching the active LLM service."""
    configured_services = list(config_data.get("llm_services", {}).keys())
    if not configured_services:
        display_message("No LLM services configured yet. Please configure a service first.", "yellow")
        return

    display_message("\n--- Switch Active LLM Service ---", "green")
    current_active = config_data.get("active_llm_service")
    options = {}
    idx = 1
    for service_name in configured_services:
        options[str(idx)] = service_name
        active_marker = " (currently active)" if service_name == current_active else ""
        print(f"  {idx}. {service_name.capitalize()}{active_marker}")
        idx += 1
    options[str(idx)] = "cancel"
    print(f"  {idx}. Cancel / Back")
    
    try:
        choice = input("Select service to make active (number): ").strip().lower()
        if choice in options:
            chosen_service = options[choice]
            if chosen_service == "cancel":
                display_message("Switch cancelled.", "yellow")
                return
            if config_data.get("active_llm_service") != chosen_service:
                current_active_for_previous = config_data.get("active_llm_service")
                config_data["active_llm_service"] = chosen_service
                if current_active_for_previous: # Store the one we are switching from
                    config_data["previous_active_llm_service"] = current_active_for_previous
                display_message(f"{chosen_service.capitalize()} is now the active LLM service.", "green")
                save_config(config_file, config_data)
            else:
                display_message(f"{chosen_service.capitalize()} is already the active LLM service.", "yellow")
        else:
            display_message("Invalid selection.", "red")
    except ValueError: 
        display_message("Invalid input.", "red")
    except KeyboardInterrupt:
        display_message("\nSwitch cancelled.", "yellow")

def _handle_gemini_key_management(config_data, config_file):
    """Handles interactive Gemini API key management."""
    if LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}):
        display_message("Gemini service not configured. Configure it first to manage API keys.", "yellow")
        choice = input("Configure Gemini service now? (Y/n): ").strip().lower()
        if choice == 'y' or choice == '':
            _configure_selected_service(LLM_SERVICE_GEMINI, config_data, config_file)
            if LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}): 
                return 
        else:
            return

    gemini_config_part = config_data["llm_services"][LLM_SERVICE_GEMINI]
    if "api_keys" not in gemini_config_part: gemini_config_part["api_keys"] = []
    if "active_api_key_nickname" not in gemini_config_part: gemini_config_part["active_api_key_nickname"] = None
    
    original_keys_config = json.loads(json.dumps(gemini_config_part)) # Deep copy for comparison
    
    proceed, _ = _manage_gemini_api_keys_interactive_menu(gemini_config_part) 
    
    if gemini_config_part != original_keys_config: # Save if changes were made
        save_config(config_file, config_data)
    elif not proceed and (gemini_config_part.get("active_api_key_nickname") is None and not gemini_config_part.get("api_keys")):
        # If user cancelled and no keys are configured, no need to save or message "no changes"
        pass
    else:
        display_message("No changes made to Gemini API keys.", "yellow")


def setup_config(config_dir, config_file, jump_to=None, is_direct_flag_call=False):
    """
    Main entry point for all configuration tasks.
    Presents a menu or jumps to a specific section if `jump_to` is provided.
    If `is_direct_flag_call` is True, exits after the specific action unless it's the main setup (-s).
    """
    config_dir.mkdir(parents=True, exist_ok=True)
    config_data = load_config(config_file)
    config_existed_before_load = config_data is not None

    if config_data is None:
        config_data = get_default_config_structure()
        config_existed_before_load = False # Explicitly set after creating default

    if not config_existed_before_load and (is_direct_flag_call or not jump_to): # True initial setup
        display_message("\n--- DeepShell Initial Configuration ---", "green")
        display_message("No configuration file found. Let's set up your first LLM service.", "yellow")
        
        service_to_init = _select_service_to_configure(config_data, action_verb="set up initially")
        if service_to_init:
            _configure_selected_service(service_to_init, config_data, config_file)
            # Reload config_data after potential save in _configure_selected_service
            current_config_data_check = load_config(config_file)
            if current_config_data_check:
                config_data = current_config_data_check
            
            if service_to_init not in config_data.get("llm_services", {}):
                 display_message("Initial service setup failed. Exiting.", "red")
                 sys.exit(1)
            # After initial setup, previous_active_llm_service will be None, which is fine.
            # The first "jump" won't be possible until a second service is activated.
        else: 
            display_message("Initial setup cancelled. At least one service must be configured to use DeepShell.", "yellow")
            sys.exit(0)
        
        if jump_to and is_direct_flag_call: # e.g. --model-change on first run
             pass # Fall through to jump_to handling
        elif is_direct_flag_call and not jump_to: # -s was used, initial setup done, proceed to menu
            display_message("\nInitial setup complete. Entering main settings menu...", "blue")
        elif not is_direct_flag_call and not jump_to: # Programmatic call during query, config was missing
            return config_data 

    # Handle direct jumps from CLI flags
    if jump_to:
        if jump_to == "model_change":
            _handle_active_model_change(config_data, config_file)
        elif jump_to == "llm_management": 
            display_message("\n--- LLM Service Management ---", "green")
            print("  1. Configure/Reconfigure an LLM Service")
            print("  2. Switch Active LLM Service")
            print("  B. Back/Cancel")
            sub_choice = input("Your choice: ").strip().lower()
            if sub_choice == '1':
                service_to_manage = _select_service_to_configure(config_data, action_verb="configure/reconfigure")
                if service_to_manage: _configure_selected_service(service_to_manage, config_data, config_file)
            elif sub_choice == '2':
                _handle_switch_active_llm(config_data, config_file)
            elif sub_choice != 'b':
                display_message("Invalid choice.", "red")
        elif jump_to == "gemini_keys":
            _handle_gemini_key_management(config_data, config_file)
        elif jump_to == "show_config":
            show_active_configuration(config_dir, config_file) # This exits
        elif jump_to == "delete_config":
            delete_config_file(config_file) # This exits
        
        if is_direct_flag_call: 
            return config_data 

    # Main settings menu loop (only if -s was used, or if initial setup via -s just completed)
    if is_direct_flag_call and not jump_to: 
        if not config_existed_before_load: # If we just did initial setup via -s
            pass # Message already printed
        else: # -s used on existing config
            display_message("\n--- DeepShell Settings Menu ---", "green")

        while True:
            # Reload config at the start of each menu loop iteration to reflect changes
            current_config_data_check = load_config(config_file)
            if current_config_data_check:
                config_data = current_config_data_check
            else: # Config was deleted from within the menu
                display_message("Configuration file seems to have been deleted. Exiting settings.", "orange")
                break

            print("\nSettings Menu:")
            
            gemini_configured = LLM_SERVICE_GEMINI in config_data.get("llm_services", {})
            menu_options = []
            menu_options.append({"key": "1", "text": "Manage LLM Services (Add/Reconfigure)", "action": "manage_services"})
            menu_options.append({"key": "2", "text": "Switch Active LLM Service", "action": "switch_active"})
            
            next_key = 3
            if gemini_configured:
                menu_options.append({"key": str(next_key), "text": "Manage Gemini API Keys", "action": "gemini_keys"})
                next_key += 1
            
            menu_options.append({"key": str(next_key), "text": "Change Model for Active Service", "action": "model_change"})
            next_key += 1
            menu_options.append({"key": str(next_key), "text": "View Active Configuration", "action": "show_config"})
            next_key += 1
            menu_options.append({"key": str(next_key), "text": "Delete Entire Configuration", "action": "delete_config"})
            
            for opt in menu_options:
                print(f"  {opt['key']}. {opt['text']}")
            print("  X. Exit Settings")
            
            choice = input("Enter your choice: ").strip().lower()
            action_to_take = None
            for opt in menu_options:
                if choice == opt['key'].lower():
                    action_to_take = opt['action']
                    break
            
            if choice == 'x':
                display_message("Exiting settings.", "blue")
                break

            if action_to_take == "manage_services":
                service_to_manage = _select_service_to_configure(config_data, action_verb="configure/reconfigure")
                if service_to_manage:
                    _configure_selected_service(service_to_manage, config_data, config_file)
            elif action_to_take == "switch_active":
                _handle_switch_active_llm(config_data, config_file)
            elif action_to_take == "gemini_keys" and gemini_configured:
                _handle_gemini_key_management(config_data, config_file)
            elif action_to_take == "model_change":
                _handle_active_model_change(config_data, config_file)
            elif action_to_take == "show_config":
                show_active_configuration(config_dir, config_file) 
            elif action_to_take == "delete_config":
                delete_config_file(config_file) 
            elif action_to_take is None and choice != 'x': # Invalid choice not matching 'x'
                display_message("Invalid choice.", "red")
            
    return config_data

def jump_to_previous_llm(config_dir, config_file):
    """Switches to the previously active LLM service."""
    config_data = load_config(config_file)
    if not config_data:
        display_message("Configuration not found. Please run setup (`-s`) first.", "red")
        sys.exit(1)

    current_active_service = config_data.get("active_llm_service")
    previous_active_service = config_data.get("previous_active_llm_service")

    if not previous_active_service:
        display_message("No previously active LLM service recorded to jump to.", "yellow")
        display_message("Hint: Switch services at least once using the settings menu (`-s` then option 2, or `-l`) to enable jumping.", "yellow")
        sys.exit(0)

    if previous_active_service not in config_data.get("llm_services", {}):
        display_message(f"The previously active service '{previous_active_service}' is no longer configured.", "orange")
        config_data["previous_active_llm_service"] = None # Clear invalid previous
        save_config(config_file, config_data)
        sys.exit(1)

    if current_active_service == previous_active_service:
        display_message(f"Already using '{current_active_service.capitalize()}'. No jump performed.", "yellow")
        sys.exit(0)

    # Perform the jump
    config_data["active_llm_service"] = previous_active_service
    config_data["previous_active_llm_service"] = current_active_service # The current becomes the new previous

    if save_config(config_file, config_data):
        display_message(f"Jumped to LLM service: {previous_active_service.capitalize()}", "green")
    sys.exit(0)

def change_active_model_config(config_dir, config_file):
    """
    Allows the user to change the default model for the active LLM service.
    This is now a wrapper for setup_config.
    """
    setup_config(config_dir, config_file, jump_to="model_change", is_direct_flag_call=True)


def switch_llm_service(config_dir, config_file):
    """
    Allows the user to switch the active LLM service or configure a new one.
    This is now a wrapper for setup_config.
    """
    setup_config(config_dir, config_file, jump_to="llm_management", is_direct_flag_call=True)

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
        if confirm == 'y' or confirm == '': 
            try:
                config_file.unlink() 
                display_message(f"Configuration file {config_file} has been deleted.", "green")
            except OSError as e:
                display_message(f"Error deleting configuration file: {e}", "red")
                sys.exit(1)
        else:
            display_message("Deletion cancelled by user.", "yellow")
        sys.exit(0) 
    except KeyboardInterrupt:
        display_message("\nDeletion process cancelled by user. Exiting.", "yellow")
        sys.exit(0)
    except Exception as e: 
        display_message(f"An unexpected error occurred during the delete confirmation: {e}", "red")
        sys.exit(1)

def show_active_configuration(config_dir, config_file):
    """Displays the details of the currently active LLM configuration."""
    display_message("\n--- Active DeepShell Configuration ---", "green")
    config_data = load_config(config_file)

    if not config_data:
        display_message("No configuration file found. Please run --setup.", "yellow")
        sys.exit(1)

    labels = [
        "Active LLM Service:",
        "Active Model:",
        "Ollama Server:",
        "API Key Nickname:",
        "API Key Value:",
        "Render Markdown:" 
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
        display_model = model_name.split('/')[-1] if active_service_name == LLM_SERVICE_GEMINI else model_name
        full_id_display = f" (Full ID: {model_name})" if active_service_name == LLM_SERVICE_GEMINI and '/' in model_name else ""
        display_message(f"{'Active Model:':<{max_label_len}} {display_model}{full_id_display}", "blue")
    else:
        display_message(f"{'Active Model:':<{max_label_len}} Not set", "orange")

    if active_service_name == LLM_SERVICE_OLLAMA:
        server_address = service_config.get("server_address", "Not set")
        display_message(f"{'Ollama Server:':<{max_label_len}} {server_address}", "blue")
    elif active_service_name == LLM_SERVICE_GEMINI:
        active_key_value, active_nickname = _get_active_gemini_key_value(service_config) 
        if active_nickname and active_key_value:
            display_message(f"{'API Key Nickname:':<{max_label_len}} {active_nickname}", "blue")
            display_message(f"{'API Key Value:':<{max_label_len}} {active_key_value}", "blue")
        elif active_nickname: 
            display_message(f"{'API Key Nickname:':<{max_label_len}} {active_nickname} (Key value missing!)", "orange")
        else:
            display_message(f"{'API Key:':<{max_label_len}} Not set", "orange")
        
    render_md_for_active_service = service_config.get("render_markdown", True) 
    display_message(f"{'Render Markdown:':<{max_label_len}} {'Enabled' if render_md_for_active_service else 'Disabled'}", "blue")

    sys.exit(0)
