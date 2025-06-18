import os
import sys
import argparse
import textwrap 

from settings import (
    get_config_path, load_config, save_config, setup_config, change_active_model_config,
    switch_llm_service, delete_config_file, show_active_configuration
) 
from ollama import send_ollama_query
from gemini import (
    send_gemini_query, 
    _get_active_gemini_key_value,
    fetch_gemini_models, # Added for moved functions
    _manage_gemini_api_keys_interactive_menu # Added for moved functions
)
from utils import display_message, LLM_SERVICE_OLLAMA, LLM_SERVICE_GEMINI # Import constants from utils
from settings import jump_to_previous_llm # Import the new function

# Version
__version__ = "1.0.3"

class CustomHelpFormatter(argparse.RawTextHelpFormatter):
    """Custom formatter for argparse help messages to adjust spacing."""
    def __init__(self, prog, indent_increment=2, max_help_position=50, width=None):
        super().__init__(prog, indent_increment, max_help_position, width)

    def _split_lines(self, text, width):
        """
        Splits help text into lines.
        """
        raw_lines = text.splitlines()
        
        final_wrapped_lines = []
        for line in raw_lines:
            if not line:
                final_wrapped_lines.append('')
                continue
            sub_lines = textwrap.wrap(line, width)
            final_wrapped_lines.extend(sub_lines if sub_lines else [line]) 
        return final_wrapped_lines

    def add_arguments(self, actions):
        """
        Adds argument descriptions to the help message, sorted alphabetically for optional arguments.
        """
        if not actions:
            super().add_arguments(actions)
            return

        actions_to_format = list(actions) 

        if actions_to_format[0].option_strings:
            actions_to_format.sort(key=lambda action: action.option_strings[0])
        super().add_arguments(actions_to_format)


# Functions moved from gemini.py to break circular import
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
                # Pass is_direct_flag_call=False as this is a programmatic setup, not from -s flag
                config_data = setup_config(config_dir, config_file, is_direct_flag_call=False) 
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

    fetch_gemini_models(active_api_key_value) # Uses fetch_gemini_models from gemini.py

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
    # This function is now a shortcut to the main settings menu
    setup_config(config_dir, config_file, jump_to="gemini_keys", is_direct_flag_call=True)
    sys.exit(0)


def main():
    """
    Main function to parse arguments, manage configuration, and execute the query.
    """
    try:
        parser = argparse.ArgumentParser(
            description="DeepShell: Query an LLM service from the command line.",
            formatter_class=CustomHelpFormatter 
        )
        parser.add_argument(
            "-s", "--setup",
            action="store_true",
            help="Run the interactive configuration setup process."
        )
        parser.add_argument(
            "-model", "--model-change", 
            action="store_true",
            dest="model_change", 
            help="Change the default model for the active LLM service."
        )
        parser.add_argument(
            "-q", "--query",
            nargs='+', 
            metavar="QUERY",
            help="The query to send to the LLM. All text following this flag will be treated as the query.\n"
                 "Example: deepshell -q What is the capital of France?"
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
            "-set-key", "--set-api-key", 
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
            "-show-config", "--show-full-conf", 
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
        parser.add_argument(
            '-j', '--jump-llm',
            action='store_true',
            dest="jump_llm",
            help="Quickly switch to the previously used LLM service."
        )
        args = parser.parse_args()

        config_dir, config_file = get_config_path()

        if args.setup:
            setup_config(config_dir, config_file, is_direct_flag_call=True) 
            sys.exit(0)

        if args.model_change:
            # change_active_model_config(config_dir, config_file) # Old direct call
            setup_config(config_dir, config_file, jump_to="model_change", is_direct_flag_call=True)
            sys.exit(0)
        
        if args.switch_llm:
            # switch_llm_service(config_dir, config_file) # Old direct call
            setup_config(config_dir, config_file, jump_to="llm_management", is_direct_flag_call=True)
            sys.exit(0)
        
        if args.delete_config:
            # delete_config_file(config_file) # Old direct call
            setup_config(config_dir, config_file, jump_to="delete_config", is_direct_flag_call=True)
            sys.exit(0) 
        
        if args.gemini_quota_check:
            check_gemini_quota_status(config_dir, config_file)
            sys.exit(0)
        
        if args.show_gemini_key:
            show_active_gemini_api_key(config_dir, config_file)
            sys.exit(0)

        if args.set_gemini_key:
            set_gemini_api_key_interactive(config_dir, config_file)
            sys.exit(0)
        
        if args.show_config:
            # show_active_configuration(config_dir, config_file) # Old direct call
            setup_config(config_dir, config_file, jump_to="show_config", is_direct_flag_call=True)
            sys.exit(0)
        
        if args.jump_llm:
            jump_to_previous_llm(config_dir, config_file)
            sys.exit(0)

        config_data = load_config(config_file)
        user_query_list = args.query
        user_query = " ".join(user_query_list) if user_query_list else None

        if config_data is None or not config_data.get("active_llm_service"):
            display_message("Configuration not found or no active LLM service. Running setup...", "yellow")
            # Programmatic call, not from -s flag, so is_direct_flag_call=False
            config_data = setup_config(config_dir, config_file, is_direct_flag_call=False) 
            if not config_data or not config_data.get("active_llm_service"):
                display_message("Setup incomplete or no active LLM service selected. Exiting.", "red")
                sys.exit(1)
            if not user_query: # If setup was run and no query was provided, exit gracefully
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

        active_service_name_display = active_service_name.capitalize()

        if active_service_name == LLM_SERVICE_OLLAMA:
            server_address = service_config.get("server_address")
            if not server_address:
                display_message("Ollama server address not configured. Please run --setup or --llm to configure Ollama.", "red")
                sys.exit(1)
            send_ollama_query(server_address, model_name, user_query, active_service_name_display, service_config)
        elif active_service_name == LLM_SERVICE_GEMINI:
            active_api_key_value, active_nickname = _get_active_gemini_key_value(service_config)
            if not active_api_key_value:
                display_message("No active Gemini API key configured or found. Please run --setup or --llm to configure Gemini.", "red")
                sys.exit(1)
            send_gemini_query(
                active_api_key_value, model_name, user_query,
                active_service_name_display,
                active_nickname,
                service_config 
            )

    except KeyboardInterrupt:
        display_message("\nProgram interrupted by user. Exiting gracefully.", "yellow")
        sys.exit(0)

if __name__ == "__main__":
    main()
