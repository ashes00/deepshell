import json
import requests
import threading
import time
import sys

from utils import display_message, _animate_progress, rich_console, LLM_SERVICE_OLLAMA
from rich.markdown import Markdown

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
                retry = input("Failed to connect or fetch models. Try again with a different address? (Y/n): ").strip().lower()
                if retry == 'n':
                    return None 
                continue 
            elif not models: 
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
            return None

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
        "model": chosen_model,
        "render_markdown": True # Always enable Markdown rendering by default
    }
    
    if "llm_services" not in config_data:
        config_data["llm_services"] = {}
    
    config_data["llm_services"][LLM_SERVICE_OLLAMA] = ollama_config
    display_message(f"Ollama service configured with model: {chosen_model}", "green")
    return config_data

def send_ollama_query(server_address, model_name, user_query, active_service_name_display, ollama_service_config):
    """
    Sends the user's query to the OLLAMA server's /api/chat endpoint
    and prints only the relevant response content.
    ollama_service_config is the specific configuration part for Ollama service.
    """
    sending_message = (
        f"Using active LLM service: {active_service_name_display}. "
        f"Sending query to {server_address} (Model: {model_name})..."
    )
    # display_message(sending_message, "blue", end='') # Old way
    display_message(sending_message, "blue") # New way, prints newline by default
    sys.stdout.flush() # Ensure the message is displayed before animation starts
    message_len_for_animation = len(sending_message)

    url = f"{server_address}/api/chat"
    payload = {
        "model": model_name,
        "messages": [{"role": "user", "content": user_query}],
        "stream": False 
    }

    stop_event = threading.Event()
    animation_thread = threading.Thread(target=_animate_progress, args=(stop_event, message_len_for_animation))
    animation_thread.daemon = True 
    animation_thread.start()

    response_data = None
    error_message_to_display = None 
    http_response_obj = None 

    try:
        http_response_obj = requests.post(url, json=payload, timeout=120)
        http_response_obj.raise_for_status()
        response_data = http_response_obj.json()
    except requests.exceptions.ConnectionError:
        error_message_to_display = f"Connection Error: Could not connect to {server_address}. Is the server running and accessible?"
    except requests.exceptions.Timeout:
        error_message_to_display = f"Request Timeout: The server at {server_address} did not respond in time."
    except requests.exceptions.HTTPError as e:
        error_message_to_display = f"HTTP Error during query: {e} - Server response: {getattr(e.response, 'text', 'No response text')}"
        http_response_obj = e.response 
    except json.JSONDecodeError:
        resp_text = getattr(http_response_obj, 'text', 'No response object available for text.') if http_response_obj else 'No response object.'
        error_message_to_display = f"JSON Decode Error: Could not parse response from server. Response text: {resp_text}"
    except Exception as e:
        error_message_to_display = f"An unexpected error occurred during query: {e}"
    finally:
        stop_event.set()
        animation_thread.join(timeout=0.5)
        sys.stdout.write("\r" + " " * message_len_for_animation + "\r")
        sys.stdout.flush()

    if error_message_to_display:
        display_message(error_message_to_display, "red")
    elif response_data:
        if "message" in response_data and "content" in response_data["message"]:
            ollama_text_response = response_data["message"]["content"].strip()
            render_markdown_enabled = ollama_service_config.get("render_markdown", True) 

            display_message("--- OLLAMA Response ---", "green") 
            if render_markdown_enabled:
                try:
                    # Assuming rich_console and Markdown are imported/available
                    rich_console.print(Markdown(ollama_text_response))
                except Exception as e: 
                    display_message(f"Rich Markdown rendering failed: {e}. Falling back to plain text.", "orange")
                    print(ollama_text_response)
            else:
                print(ollama_text_response)
            display_message("-----------------------", "green") 
        else:
            display_message(f"Error: Unexpected response format from server. Full response: {json.dumps(response_data, indent=2)}", "orange")