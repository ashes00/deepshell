#include "deepshell.h"

int main(int argc, char *argv[]) {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Parse command line arguments
    cli_args_t args = parse_arguments(argc, argv);
    
    // Handle help and version flags
    if (args.help) {
        print_help();
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.version) {
        print_version();
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    // Load configuration
    config_t *config = load_config();
    bool config_loaded = (config != NULL);
    if (!config) {
        config = get_default_config();
    }
    
    // Check if we need to run setup automatically (like Python version)
    bool needs_setup = false;
    if (!config_loaded) {
        // No config file found - setup needed for any command except help/version
        if (!args.help && !args.version) {
            needs_setup = true;
        }
    } else if (config && strlen(config->active_llm_service) == 0) {
        // Config exists but no active LLM service - setup needed for any command except help/version
        if (!args.help && !args.version) {
            needs_setup = true;
        }
    }
    
    // Auto-start setup if needed (following Python version behavior)
    if (needs_setup && !args.setup && !args.delete_config && !args.help && !args.version) {
        display_message("Configuration not found or no active LLM service. Running setup...", COLOR_YELLOW);
        if (!setup_config(false)) {
            display_message("Setup incomplete. Exiting.", COLOR_RED);
            free_config(config);
            free_cli_args(&args);
            curl_global_cleanup();
            return 1;
        }
        // Reload config after setup
        free_config(config);
        config = load_config();
        if (!config) {
            display_message("Failed to load configuration after setup.", COLOR_RED);
            free_cli_args(&args);
            curl_global_cleanup();
            return 1;
        }
        
        // If setup was run and no query was provided, exit gracefully (like Python version)
        if (args.query_text == NULL && !args.interactive) {
            display_message("Setup complete. You can now run the program with a query or other options.", COLOR_GREEN);
            free_config(config);
            free_cli_args(&args);
            curl_global_cleanup();
            return 0;
        }
    }
    
    // Handle different command line options
    if (args.setup) {
        if (!setup_config(true)) {
            display_message("Setup failed or was cancelled.", COLOR_RED);
            free_config(config);
            free_cli_args(&args);
            curl_global_cleanup();
            return 1;
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.delete_config) {
        if (delete_config_file()) {
            display_message("Configuration file deleted successfully.", COLOR_GREEN);
        } else {
            display_message("Failed to delete configuration file.", COLOR_RED);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.show_config) {
        show_active_configuration(config);
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.switch_llm) {
        if (!switch_llm_service(config)) {
            display_message("Failed to switch LLM service.", COLOR_RED);
        } else {
            save_config(config);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.jump_llm) {
        if (!jump_to_previous_llm(config)) {
            display_message("No previous LLM service to jump to.", COLOR_YELLOW);
        } else {
            save_config(config);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.model_change) {
        if (!change_active_model(config)) {
            display_message("Failed to change model.", COLOR_RED);
        } else {
            save_config(config);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.set_gemini_key) {
        if (!manage_gemini_api_keys(config)) {
            display_message("Failed to manage Gemini API keys.", COLOR_RED);
        } else {
            save_config(config);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.show_gemini_key) {
        char nickname[MAX_NICKNAME_LEN];
        char *key_value = get_active_gemini_key_value(&config->gemini, nickname);
        if (key_value) {
            display_message("--- Active Gemini API Key ---", COLOR_GREEN);
            display_message("Active Nickname: ", COLOR_BLUE);
            printf("%s\n", nickname);
            display_message("API Key Value  : ", COLOR_BLUE);
            printf("%s\n", key_value);
        } else {
            display_message("No active Gemini API key is set.", COLOR_YELLOW);
            display_message("Use --set-api-key or --setup to configure Gemini API keys.", COLOR_YELLOW);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.gemini_quota_check) {
        char nickname[MAX_NICKNAME_LEN];
        char *key_value = get_active_gemini_key_value(&config->gemini, nickname);
        if (key_value) {
            display_message("--- Gemini API Key & Quota Info ---", COLOR_GREEN);
            display_message("Active Gemini API Key Nickname: ", COLOR_BLUE);
            printf("%s\n", nickname);
            display_message("Active Gemini API Key Value   : ", COLOR_BLUE);
            printf("%s\n", key_value);
            
            // Try to fetch models to test the key
            char **models = NULL;
            int model_count = 0;
            if (fetch_gemini_models(key_value, &models, &model_count)) {
                display_message("API key is valid and can access models.", COLOR_GREEN);
                // Free models
                for (int i = 0; i < model_count; i++) {
                    free(models[i]);
                }
                free(models);
            } else {
                display_message("API key may be invalid or have insufficient permissions.", COLOR_RED);
            }
            
            display_message("To check your Gemini API usage, please visit:", COLOR_BLUE);
            display_message("https://aistudio.google.com/app/usage", COLOR_YELLOW);
        } else {
            display_message("No active Gemini API key found.", COLOR_RED);
            display_message("Please configure Gemini API keys using --setup or --set-api-key.", COLOR_YELLOW);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    if (args.interactive) {
        if (!start_interactive_session(config)) {
            display_message("Interactive session failed.", COLOR_RED);
        }
        free_config(config);
        free_cli_args(&args);
        curl_global_cleanup();
        return 0;
    }
    
    // Handle query
    if (args.query) {
        // Temporarily disable animation if --no-animation flag is used
        bool original_animation_setting = config->show_progress_animation;
        if (args.no_animation) {
            config->show_progress_animation = false;
        }
        
        // Send query to appropriate service
        if (strcmp(config->active_llm_service, LLM_SERVICE_OLLAMA) == 0) {
            if (strlen(config->ollama.server_address) == 0) {
                display_message("Ollama server address not configured. Please run --setup or --llm to configure Ollama.", COLOR_RED);
                free_config(config);
                free_cli_args(&args);
                curl_global_cleanup();
                return 1;
            }
            char *response_text = send_ollama_query(config->ollama.server_address, config->ollama.model, 
                            args.query_text, NULL, 0, config);
            if (response_text) {
                display_message("--- Ollama Response ---", COLOR_GREEN);
                if (config->ollama.render_markdown) {
                    print_markdown(response_text);
                } else {
                    printf("%s\n", response_text);
                }
                display_message("-----------------------", COLOR_GREEN);
                free(response_text);
            }
        } else if (strcmp(config->active_llm_service, LLM_SERVICE_GEMINI) == 0) {
            char nickname[MAX_NICKNAME_LEN];
            char *api_key = get_active_gemini_key_value(&config->gemini, nickname);
            if (!api_key) {
                display_message("No active Gemini API key configured or found. Please run --setup or --llm to configure Gemini.", COLOR_RED);
                free_config(config);
                free_cli_args(&args);
                curl_global_cleanup();
                return 1;
            }
            char *response_text = send_gemini_query(api_key, config->gemini.model, args.query_text, 
                            NULL, 0, config);
            if (response_text) {
                display_message("--- Gemini Response ---", COLOR_GREEN);
                if (config->gemini.render_markdown) {
                    print_markdown(response_text);
                } else {
                    printf("%s\n", response_text);
                }
                display_message("------------------------", COLOR_GREEN);
                free(response_text);
            }
        } else {
            display_message("No active LLM service configured or configuration is corrupted.", COLOR_RED);
            display_message("Please run --setup or --llm to select/configure a service.", COLOR_YELLOW);
            free_config(config);
            free_cli_args(&args);
            curl_global_cleanup();
            return 1;
        }
        
        // Restore original animation setting if it was temporarily changed
        if (args.no_animation) {
            config->show_progress_animation = original_animation_setting;
        }
    } else {
        // No query provided and no other action flags
        print_help();
        display_message("\nError: A query is required (use -q YOUR_QUERY) if not using other action flags.", COLOR_RED);
    }
    
    // Cleanup
    free_config(config);
    free_cli_args(&args);
    curl_global_cleanup();
    return 0;
}

void print_help(void) {
    printf("DeepShell: Query an LLM service from the command line.\n\n");
    printf("Usage: deepshell [OPTIONS] -q QUERY\n\n");
    printf("Options:\n");
    printf("  -s, --setup              Run the interactive configuration setup process\n");
    printf("  -q, --query QUERY        The query to send to the LLM\n");
    printf("  -i, --interactive        Start an interactive chat session\n");
    printf("  -d, --delete-config      Delete the configuration file after confirmation\n");
    printf("  -show-key, --show-api-key\n");
    printf("                           Show the active Gemini API key nickname and value\n");
    printf("  -set-key, --set-api-key  Interactively set/manage Gemini API keys\n");
    printf("  -gq, --gemini-quota      Check Gemini API key status and quota information\n");
    printf("  -l, --llm                Switch the active LLM service or configure services\n");
    printf("  -show-config, --show-full-conf\n");
    printf("                           Display the currently active LLM configuration details\n");
    printf("  -v, --version            Show program's version number and exit\n");
    printf("  -j, --jump-llm           Quickly switch to the previously used LLM service\n");
    printf("  -m, --model-change       Change the default model for the active LLM service\n");
    printf("      --no-animation       Disable progress animation for this run\n");
    printf("  -h, --help               Show this help message and exit\n\n");
    printf("Examples:\n");
    printf("  deepshell -q \"What is the capital of France?\"\n");
    printf("  deepshell -q \"Tell me about adam sandler's movies\"\n");
    printf("  deepshell --no-animation -q \"Quick query without animation\"\n");
    printf("  deepshell -s\n");
    printf("  deepshell -i\n");
    printf("\nNote: Use quotes around queries containing spaces or special characters.\n");
}

void print_version(void) {
    printf("DeepShell Version %s\n", DEEPSHELL_VERSION);
} 