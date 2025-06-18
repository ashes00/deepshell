import json
import requests
import threading
import time
import sys

from utils import display_message, _animate_progress, rich_console, LLM_SERVICE_GEMINI
from rich.markdown import Markdown

GEMINI_API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta"

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
        models = [
            m['name'] for m in data.get('models', [])
            if any(method in m.get('supportedGenerationMethods', []) for method in ['generateContent', 'generateAnswer'])
            and "chat" not in m['name'].lower()
            and "tts" not in m['name'].lower()
        ]
        if not models:
            display_message("No suitable Gemini models found. Ensure your API key is valid and has access to models like 'gemini-pro'.", "yellow")
            display_message(f"Full response from Gemini API: {json.dumps(data, indent=2)}", "yellow")
            return []
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
        return None, None 
    
    active_nickname = gemini_config.get("active_api_key_nickname")
    api_keys_list = gemini_config.get("api_keys", [])
    
    if not active_nickname or not api_keys_list:
        return None, None 

    for key_info in api_keys_list:
        if key_info.get("nickname") == active_nickname:
            return key_info.get("key"), active_nickname
    return None, active_nickname 

def _manage_gemini_api_keys_interactive_menu(gemini_config_part):
    """
    Manages Gemini API keys interactively (add, remove, set active).
    Modifies gemini_config_part in place.
    Returns:
        bool: True if an active key is set and user wants to proceed, False if cancelled or no active key.
        str: The nickname of the active API key, or None.
    """
    api_keys_list = gemini_config_part.get("api_keys", [])
    active_api_key_nickname = gemini_config_part.get("active_api_key_nickname")

    if "api_keys" not in gemini_config_part:
        gemini_config_part["api_keys"] = api_keys_list

    while True:
        display_message("\n--- Gemini API Key Management ---", "blue")
        if not api_keys_list:
            display_message("No Gemini API keys configured yet.", "yellow")
        else:
            display_message("Configured Gemini API Keys:", "yellow")
            max_nick_len = 0
            if api_keys_list: 
                max_nick_len = max(len(key_info.get('nickname', f'Key {idx+1}')) for idx, key_info in enumerate(api_keys_list))

            for i, key_info in enumerate(api_keys_list):
                nick = key_info.get('nickname', f'Key {i+1}')
                key_val = key_info.get('key', 'N/A')
                display_key_val = key_val 
                active_marker = " (active)" if nick == active_api_key_nickname else ""
                print(f"  {i+1}. {nick:<{max_nick_len}} - Key: {display_key_val}{active_marker}")

        print("\nOptions:")
        print("  1. Add a new API key")
        if api_keys_list:
            print("  2. Set an API key as active")
            print("  3. Remove an API key")
        if active_api_key_nickname and _get_key_value_by_nickname(active_api_key_nickname, api_keys_list):
            print("  C. Continue / Confirm selection")
        print("  X. Cancel / Exit key management")

        choice = input("Enter your choice: ").strip().lower()

        if choice == '1': 
            new_nickname = ""
            while not new_nickname: 
                new_nickname = input("Enter a unique nickname for this API key: ").strip()
                if not new_nickname:
                    display_message("Nickname cannot be empty.", "red")
                elif _find_key_by_nickname(new_nickname, api_keys_list): 
                    display_message(f"Nickname '{new_nickname}' already exists. Please choose another.", "red")
                    new_nickname = "" 

            new_api_key_value = input("Enter the new Gemini API Key: ").strip()
            if not new_api_key_value:
                display_message("API Key value cannot be empty. Aborting add.", "red")
                continue
            
            api_keys_list.append({"nickname": new_nickname, "key": new_api_key_value})
            display_message(f"API key '{new_nickname}' added.", "green")

            # Ask if the user wants to make the new key active
            make_active_choice = input(f"Make '{new_nickname}' the active API key? (Y/n): ").strip().lower()
            if make_active_choice == 'y' or make_active_choice == '':
                active_api_key_nickname = new_nickname # Update local variable for current menu session
                gemini_config_part["active_api_key_nickname"] = new_nickname # Update config
                display_message(f"'{new_nickname}' has been set as the active API key.", "blue")

        elif choice == '2' and api_keys_list: 
            if len(api_keys_list) == 1:
                active_api_key_nickname = api_keys_list[0]["nickname"]
                gemini_config_part["active_api_key_nickname"] = active_api_key_nickname
                display_message(f"'{active_api_key_nickname}' is already the only key and is active.", "blue")
                continue

            display_message("Select an API key to set as active:", "yellow")
            for i, key_info_display in enumerate(api_keys_list): 
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

        elif choice == '3' and api_keys_list: 
            display_message("Select an API key to remove:", "yellow")
            for i, key_info_remove in enumerate(api_keys_list): 
                print(f"  {i+1}. {key_info_remove['nickname']}")
            print(f"  {len(api_keys_list) + 1}. Cancel removal")
            
            try:
                key_choice_idx_remove = int(input("Enter number of the key to remove: ").strip()) - 1
                if 0 <= key_choice_idx_remove < len(api_keys_list):
                    removed_key_info = api_keys_list.pop(key_choice_idx_remove)
                    removed_nickname = removed_key_info['nickname']
                    display_message(f"API key '{removed_nickname}' has been removed.", "green")

                    previously_active_nickname_before_removal_logic = active_api_key_nickname

                    if removed_nickname == previously_active_nickname_before_removal_logic:
                        active_api_key_nickname = None
                        gemini_config_part["active_api_key_nickname"] = None
                    
                    if len(api_keys_list) == 1:
                        current_sole_key_nickname = api_keys_list[0]['nickname']
                        if active_api_key_nickname != current_sole_key_nickname: 
                            active_api_key_nickname = current_sole_key_nickname
                            gemini_config_part["active_api_key_nickname"] = active_api_key_nickname
                            display_message(f"'{active_api_key_nickname}' is now the active API key as it's the only one remaining.", "blue")
                    elif not api_keys_list:
                        display_message("All API keys have been removed. No active key is set.", "yellow")
                    elif removed_nickname == previously_active_nickname_before_removal_logic: 
                        display_message(f"The active API key ('{removed_nickname}') was removed. Please set a new active key from the remaining options.", "yellow")

                elif key_choice_idx_remove == len(api_keys_list): 
                    display_message("Removal cancelled.", "yellow")
                else:
                    display_message("Invalid selection for removal.", "red")
            except ValueError:
                display_message("Invalid input. Please enter a numerical value.", "red")

        elif choice == 'c' and active_api_key_nickname and _get_key_value_by_nickname(active_api_key_nickname, api_keys_list):
            gemini_config_part["active_api_key_nickname"] = active_api_key_nickname 
            return True, active_api_key_nickname 
        elif choice == 'x':
            display_message("Gemini key management cancelled.", "yellow")
            return bool(active_api_key_nickname and _get_key_value_by_nickname(active_api_key_nickname, api_keys_list)), active_api_key_nickname
        else:
            display_message("Invalid choice. Please try again.", "red")

def _setup_gemini_service(config_data):
    """
    Sets up or updates Gemini service configuration. This includes managing API keys,
    selecting a model, and choosing whether to render responses as Markdown.
    """
    display_message("\n--- Gemini Service Setup ---", "green")
    display_message("You'll need a Gemini API key.", "yellow")
    display_message("You can obtain one from Google AI Studio: https://aistudio.google.com/app/apikey", "blue")

    if LLM_SERVICE_GEMINI not in config_data.get("llm_services", {}):
        if "llm_services" not in config_data:
            config_data["llm_services"] = {}
        config_data["llm_services"][LLM_SERVICE_GEMINI] = {
            "api_keys": [], 
            "active_api_key_nickname": None, 
            "model": None,
            "render_markdown": True 
        }
    
    gemini_config_part = config_data["llm_services"][LLM_SERVICE_GEMINI]

    if "render_markdown" not in gemini_config_part:
        gemini_config_part["render_markdown"] = True 

    proceed_with_active_key, active_api_key_nickname = _manage_gemini_api_keys_interactive_menu(gemini_config_part)

    if not proceed_with_active_key or not active_api_key_nickname:
        display_message("No active Gemini API key selected or setup cancelled. Cannot proceed with model selection.", "yellow")
        return None 

    current_active_api_key_value = _get_key_value_by_nickname(active_api_key_nickname, gemini_config_part["api_keys"])
    if not current_active_api_key_value: 
        display_message(f"Error: Active API key '{active_api_key_nickname}' value not found. Cannot proceed.", "red")
        return None

    display_message(f"\nFetching models using active API key: '{active_api_key_nickname}'", "blue")
    models = None
    while True:
        try:
            models = fetch_gemini_models(current_active_api_key_value)
            if models is None: 
                retry = input("Failed to fetch Gemini models. Check API key and connection. Try again? (Y/n): ").strip().lower()
                if retry == 'n':
                    return None 
                continue
            elif not models: 
                display_message(f"No suitable models found for API key '{active_api_key_nickname}'. You might need to enable models in your Google Cloud project or use a different key.", "yellow")
                retry = input("Manage API keys again or skip Gemini setup? (M to manage keys / S to skip): ").strip().lower()
                if retry == 's': 
                    return None
                elif retry == 'm': 
                    display_message("Please re-run setup to manage API keys or select a different option.", "yellow")
                    return None 
                continue 
            else:
                break
        except KeyboardInterrupt:
            display_message("\nGemini setup cancelled by user.", "yellow")
            return None

    display_message("\nAvailable Gemini Models (e.g., models/gemini-pro):", "green")
    display_models = [m.split('/')[-1] for m in models] 
    for i, model_display_name in enumerate(display_models):
        print(f"  {i+1}. {model_display_name} (Full ID: {models[i]})")

    chosen_model_index = -1
    current_gemini_model_full_name = gemini_config_part.get("model") 

    default_model_prompt = ""
    if current_gemini_model_full_name and current_gemini_model_full_name in models:
        try:
            current_model_display_name = current_gemini_model_full_name.split('/')[-1]
            default_model_prompt = f" (current: {current_model_display_name})"
        except: 
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

    chosen_model_full_name = models[chosen_model_index] 
    gemini_config_part["model"] = chosen_model_full_name
    # Ensure render_markdown is True, removing the interactive prompt
    gemini_config_part["render_markdown"] = True
    display_message(f"Gemini service configured with model: {chosen_model_full_name.split('/')[-1]}", "green")
    return config_data

def send_gemini_query(api_key, model_name, user_query, active_service_name_display, api_key_nickname_display, gemini_service_config):
    """
    Sends the user's query to the Gemini API and prints the response.
    Model name should be the full "models/gemini-pro" style name.
    If Markdown rendering is enabled in the service configuration, the response
    will be formatted using the `rich` library.
    gemini_service_config is the specific configuration part for Gemini service.
    """
    model_display_name = model_name.split('/')[-1]
    sending_message = (
        f"Using active LLM service: {active_service_name_display} (API Key: '{api_key_nickname_display}'). "
        f"Sending query (Model: {model_display_name})..."
    )
    # display_message(sending_message, "blue", end='') # Old way
    display_message(sending_message, "blue") # New way, prints newline by default
    sys.stdout.flush() # Ensure the message is displayed before animation starts
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
    error_message_to_display = None 
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
        gemini_text_response = response_data['candidates'][0]['content']['parts'][0]['text'].strip()
        render_markdown_enabled = gemini_service_config.get("render_markdown", True) 

        display_message("--- Gemini Response ---", "green") 
        if render_markdown_enabled:
            try:
                # Assuming rich_console and Markdown are imported/available
                rich_console.print(Markdown(gemini_text_response))
            except Exception as e: 
                display_message(f"Rich Markdown rendering failed: {e}. Falling back to plain text.", "orange")
                print(gemini_text_response)
        else:
            print(gemini_text_response)
        display_message("-----------------------", "green") 
    elif error_message_to_display: 
        display_message(error_message_to_display, "red")
    elif not response_data and not error_message_to_display: 
        display_message("Error: Received an empty or unexpected response from Gemini API.", "orange")
    elif response_data: 
        display_message(f"Error: Unexpected response format from Gemini. Full response: {json.dumps(response_data, indent=2)}", "orange")
