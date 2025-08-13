#include "deepshell.h"

bool setup_config(bool is_direct_flag_call) {
    config_t *config = load_config();
    bool config_existed_before_load = (config != NULL);
    
    if (!config) {
        config = get_default_config();
        config_existed_before_load = false;
    }
    
    // Handle initial setup (like Python version)
    if (!config_existed_before_load && !is_direct_flag_call) {
        display_message("\n--- DeepShell Initial Configuration ---", COLOR_GREEN);
        display_message("No configuration file found. Let's set up your first LLM service.", COLOR_YELLOW);
        
        // Let user choose which service to configure initially
        display_message("\nWhich LLM service would you like to set up initially?", COLOR_BLUE);
        display_message("1. Ollama", COLOR_BLUE);
        display_message("2. Gemini", COLOR_BLUE);
        display_message("3. Cancel", COLOR_BLUE);
        
        display_message("\nEnter your choice (number): ", COLOR_YELLOW);
        char *choice = read_line();
        if (!choice) {
            display_message("Invalid input.", COLOR_RED);
            free_config(config);
            return false;
        }
        
        int service_choice = atoi(choice);
        free(choice);
        
        bool setup_success = false;
        if (service_choice == 1) {
            setup_success = configure_ollama_service(config);
        } else if (service_choice == 2) {
            setup_success = configure_gemini_service(config);
        } else if (service_choice == 3) {
            display_message("Initial setup cancelled. At least one service must be configured to use DeepShell.", COLOR_YELLOW);
            free_config(config);
            return false;
        } else {
            display_message("Invalid choice.", COLOR_RED);
            free_config(config);
            return false;
        }
        
        if (setup_success) {
            save_config(config);
            display_message("Initial setup complete!", COLOR_GREEN);
            free_config(config);
            return true;
        } else {
            display_message("Initial service setup failed. Exiting.", COLOR_RED);
            free_config(config);
            return false;
        }
    }
    
    // For direct flag calls (-s), show the full settings menu
    if (is_direct_flag_call) {
        display_message("\n--- DeepShell Configuration Setup ---", COLOR_GREEN);
        display_message("This will guide you through configuring your LLM services.", COLOR_BLUE);
        
        bool setup_completed = false;
        while (!setup_completed) {
            display_message("\n--- DeepShell Settings Menu ---", COLOR_GREEN);
            display_message("Settings Menu:", COLOR_GREEN);
            display_message("1. Manage LLM Services (Add/Reconfigure)", COLOR_BLUE);
            display_message("2. Switch Active LLM Service", COLOR_BLUE);
            display_message("3. Manage Gemini API Keys", COLOR_BLUE);
            display_message("4. Change Model for Active Service", COLOR_BLUE);
            display_message("5. Toggle Markdown Rendering for Active Service", COLOR_BLUE);
            display_message("6. Set Interactive History Limit", COLOR_BLUE);
            display_message("7. Toggle Response Streaming", COLOR_BLUE);
            display_message("8. Toggle Progress Animation", COLOR_BLUE);
            display_message("9. View Active Configuration", COLOR_BLUE);
            display_message("0. Delete Entire Configuration", COLOR_BLUE);
            display_message("X. Exit Settings", COLOR_BLUE);
            
            display_message("\nEnter your choice: ", COLOR_YELLOW);
            char *choice = read_line();
            if (!choice) {
                display_message("Invalid input.", COLOR_RED);
                continue;
            }
            
            int menu_choice = -1;
            bool is_exit = false;
            if (strlen(choice) == 1 && (choice[0] == 'x' || choice[0] == 'X')) {
                is_exit = true;
            } else {
                menu_choice = atoi(choice);
            }
            free(choice);

            if (is_exit) {
                setup_completed = true;
                continue;
            }

            switch (menu_choice) {
                case 1:
                    // Manage LLM Services (Add/Reconfigure)
                    {
                        display_message("\nWhich LLM service would you like to configure/reconfigure?", COLOR_BLUE);
                        display_message("1. Ollama", COLOR_BLUE);
                        if (strlen(config->ollama.server_address) > 0) {
                            display_message("   (already configured)", COLOR_YELLOW);
                        }
                        display_message("2. Gemini", COLOR_BLUE);
                        if (config->gemini.api_key_count > 0) {
                            display_message("   (already configured)", COLOR_YELLOW);
                        }
                        display_message("3. Cancel", COLOR_BLUE);
                        
                        display_message("\nEnter your choice (number): ", COLOR_YELLOW);
                        char *service_choice = read_line();
                        if (!service_choice) {
                            display_message("Invalid input.", COLOR_RED);
                            break;
                        }
                        
                        int choice = atoi(service_choice);
                        free(service_choice);
                        
                        bool setup_success = false;
                        if (choice == 1) {
                            setup_success = configure_ollama_service(config);
                        } else if (choice == 2) {
                            setup_success = configure_gemini_service(config);
                        } else if (choice == 3) {
                            display_message("Service configuration cancelled.", COLOR_YELLOW);
                        } else {
                            display_message("Invalid choice.", COLOR_RED);
                        }
                        
                        if (setup_success) {
                            save_config(config);
                        }
                    }
                    break;
                case 2:
                    // Switch Active LLM Service
                    if (switch_llm_service(config)) {
                        save_config(config);
                    }
                    break;
                case 3:
                    // Manage Gemini API Keys
                    manage_gemini_api_keys(config);
                    break;
                case 4:
                    // Change Model for Active Service
                    if (change_active_model(config)) {
                        save_config(config);
                    }
                    break;
                case 5:
                    // Toggle Markdown Rendering for Active Service
                    if (toggle_markdown_rendering(config)) {
                        save_config(config);
                    }
                    break;
                case 6:
                    // Set Interactive History Limit
                    if (set_history_limit(config)) {
                        save_config(config);
                    }
                    break;
                case 7:
                    // Toggle Response Streaming
                    if (toggle_streaming(config)) {
                        save_config(config);
                    }
                    break;
                case 8:
                    // Toggle Progress Animation
                    if (toggle_progress_animation(config)) {
                        save_config(config);
                    }
                    break;
                case 9:
                    // View Active Configuration
                    show_active_configuration(config);
                    break;
                case 0:
                    // Delete Entire Configuration
                    if (delete_config_file()) {
                        display_message("Configuration deleted. Exiting settings.", COLOR_GREEN);
                        free_config(config);
                        return false;
                    }
                    break;
                default:
                    display_message("Invalid choice. Please enter a number between 1 and 0, or X to exit.", COLOR_RED);
                    break;
            }
        }
        
        if (save_config(config)) {
            display_message("Configuration saved successfully!", COLOR_GREEN);
            free_config(config);
            return true;
        } else {
            display_message("Failed to save configuration.", COLOR_RED);
            free_config(config);
            return false;
        }
    }
    
    // For programmatic calls (not direct flag), just return success
    free_config(config);
    return true;
}

bool configure_ollama_service(config_t *config) {
    display_message("\n--- Ollama Service Setup ---", COLOR_GREEN);
    
    char server_address[MAX_SERVER_ADDR_LEN];
    char current_server[MAX_SERVER_ADDR_LEN];
    strcpy(current_server, config->ollama.server_address);
    
    if (strlen(current_server) > 0) {
        display_message("Current server address: ", COLOR_BLUE);
        printf("%s\n", current_server);
    }
    
    display_message("Enter Ollama server address (e.g., http://localhost:11434): ", COLOR_YELLOW);
    char *input = read_line();
    if (!input) {
        return false;
    }
    
    if (strlen(input) == 0 && strlen(current_server) > 0) {
        strcpy(server_address, current_server);
    } else if (strlen(input) > 0) {
        strcpy(server_address, input);
        
        // Add http:// if not present
        if (strncmp(server_address, "http://", 7) != 0 && strncmp(server_address, "https://", 8) != 0) {
            char temp[MAX_SERVER_ADDR_LEN];
            if (snprintf(temp, sizeof(temp), "http://%s", server_address) >= (int)sizeof(temp)) {
                display_message("Server address too long.", COLOR_RED);
                return false;
            }
            strcpy(server_address, temp);
        }
    } else {
        display_message("Server address cannot be empty.", COLOR_RED);
        free(input);
        return false;
    }
    free(input);
    
    // Test connection and fetch models
    char **models = NULL;
    int model_count = 0;
    if (!fetch_ollama_models(server_address, &models, &model_count)) {
        display_message("Failed to connect to Ollama server or fetch models.", COLOR_RED);
        return false;
    }
    
    if (model_count == 0) {
        display_message("No models found on the Ollama server.", COLOR_YELLOW);
        for (int i = 0; i < model_count; i++) {
            free(models[i]);
        }
        free(models);
        return false;
    }
    
    // Display available models
    display_message("\nAvailable models:", COLOR_GREEN);
    for (int i = 0; i < model_count; i++) {
        printf("  %d. %s\n", i + 1, models[i]);
    }
    
    // Select model
    display_message("\nEnter the number of the model to use: ", COLOR_YELLOW);
    char *model_choice = read_line();
    if (!model_choice) {
        for (int i = 0; i < model_count; i++) {
            free(models[i]);
        }
        free(models);
        return false;
    }
    
    int choice = atoi(model_choice);
    free(model_choice);
    
    if (choice < 1 || choice > model_count) {
        display_message("Invalid model selection.", COLOR_RED);
        for (int i = 0; i < model_count; i++) {
            free(models[i]);
        }
        free(models);
        return false;
    }
    
    // Update configuration
    strcpy(config->ollama.server_address, server_address);
    strcpy(config->ollama.model, models[choice - 1]);
    config->ollama.render_markdown = true;
    
    // Set as active service if no active service or if it's the only one
    if (strlen(config->active_llm_service) == 0) {
        strcpy(config->active_llm_service, LLM_SERVICE_OLLAMA);
        display_message("Ollama set as active LLM service.", COLOR_GREEN);
    }
    
    display_message("Ollama service configured successfully!", COLOR_GREEN);
    
    // Cleanup
    for (int i = 0; i < model_count; i++) {
        free(models[i]);
    }
    free(models);
    
    return true;
}

bool configure_gemini_service(config_t *config) {
    display_message("\n--- Gemini Service Setup ---", COLOR_GREEN);
    
    // First, manage API keys
    if (!manage_gemini_api_keys(config)) {
        display_message("Failed to configure Gemini API keys.", COLOR_RED);
        return false;
    }
    
    // Get active API key
    char nickname[MAX_NICKNAME_LEN];
    char *api_key = get_active_gemini_key_value(&config->gemini, nickname);
    if (!api_key) {
        display_message("No active API key configured.", COLOR_RED);
        return false;
    }
    
    // Fetch available models
    char **models = NULL;
    int model_count = 0;
    if (!fetch_gemini_models(api_key, &models, &model_count)) {
        display_message("Failed to fetch Gemini models.", COLOR_RED);
        return false;
    }
    
    if (model_count == 0) {
        display_message("No models found for Gemini API.", COLOR_YELLOW);
        for (int i = 0; i < model_count; i++) {
            free(models[i]);
        }
        free(models);
        return false;
    }
    
    // Display available models
    display_message("\nAvailable Gemini models:", COLOR_GREEN);
    for (int i = 0; i < model_count; i++) {
        printf("  %d. %s\n", i + 1, models[i]);
    }
    
    // Select model
    display_message("\nEnter the number of the model to use: ", COLOR_YELLOW);
    char *model_choice = read_line();
    if (!model_choice) {
        for (int i = 0; i < model_count; i++) {
            free(models[i]);
        }
        free(models);
        return false;
    }
    
    int choice = atoi(model_choice);
    free(model_choice);
    
    if (choice < 1 || choice > model_count) {
        display_message("Invalid model selection.", COLOR_RED);
        for (int i = 0; i < model_count; i++) {
            free(models[i]);
        }
        free(models);
        return false;
    }
    
    // Update configuration
    strcpy(config->gemini.model, models[choice - 1]);
    config->gemini.render_markdown = true;
    
    // Set as active service if no active service
    if (strlen(config->active_llm_service) == 0) {
        strcpy(config->active_llm_service, LLM_SERVICE_GEMINI);
        display_message("Gemini set as active LLM service.", COLOR_GREEN);
    }
    
    display_message("Gemini service configured successfully!", COLOR_GREEN);
    
    // Cleanup
    for (int i = 0; i < model_count; i++) {
        free(models[i]);
    }
    free(models);
    
    return true;
}

bool manage_gemini_api_keys(config_t *config) {
    display_message("\n--- Gemini API Key Management ---", COLOR_GREEN);
    
    while (true) {
        display_message("\nCurrent API keys:", COLOR_BLUE);
        if (config->gemini.api_key_count == 0) {
            display_message("No API keys configured.", COLOR_YELLOW);
        } else {
            for (int i = 0; i < config->gemini.api_key_count; i++) {
                bool is_active = (strcmp(config->gemini.api_keys[i].nickname, 
                                       config->gemini.active_api_key_nickname) == 0);
                printf("  %d. %s%s\n", i + 1, config->gemini.api_keys[i].nickname,
                       is_active ? " (active)" : "");
            }
        }
        
        display_message("\nOptions:", COLOR_BLUE);
        display_message("1. Add new API key", COLOR_BLUE);
        if (config->gemini.api_key_count > 0) {
            display_message("2. Set API key as active", COLOR_BLUE);
            display_message("3. Remove API key", COLOR_BLUE);
        }
        display_message("0. Back to main menu", COLOR_BLUE);
        // Add C and X options as in Python version
        bool has_active = (config->gemini.api_key_count > 0 && strlen(config->gemini.active_api_key_nickname) > 0);
        if (has_active) {
            display_message("C. Continue / Confirm selection", COLOR_BLUE);
        }
        display_message("X. Cancel / Exit key management", COLOR_BLUE);
        
        display_message("\nEnter your choice: ", COLOR_YELLOW);
        char *choice = read_line();
        if (!choice) {
            continue;
        }
        
        int menu_choice = -1;
        bool is_cancel = false, is_confirm = false;
        if (strlen(choice) == 1) {
            if (choice[0] == 'x' || choice[0] == 'X') {
                is_cancel = true;
            } else if ((choice[0] == 'c' || choice[0] == 'C') && has_active) {
                is_confirm = true;
            } else {
                menu_choice = atoi(choice);
            }
        } else {
            menu_choice = atoi(choice);
        }
        free(choice);

        if (is_cancel) {
            display_message("Gemini key management cancelled.", COLOR_YELLOW);
            return has_active;
        }
        if (is_confirm) {
            return true;
        }

        switch (menu_choice) {
            case 0:
                return true;
            case 1:
                {
                    display_message("Enter nickname for the API key: ", COLOR_YELLOW);
                    char *nickname = read_line();
                    if (!nickname || strlen(nickname) == 0) {
                        display_message("Nickname cannot be empty.", COLOR_RED);
                        free(nickname);
                        break;
                    }
                    
                    display_message("Enter the API key: ", COLOR_YELLOW);
                    char *api_key = read_line();
                    if (!api_key || strlen(api_key) == 0) {
                        display_message("API key cannot be empty.", COLOR_RED);
                        free(nickname);
                        free(api_key);
                        break;
                    }
                    
                    if (add_gemini_api_key(&config->gemini, nickname, api_key)) {
                        display_message("API key added successfully.", COLOR_GREEN);
                    } else {
                        display_message("Failed to add API key.", COLOR_RED);
                    }
                    
                    free(nickname);
                    free(api_key);
                }
                break;
            case 2:
                if (config->gemini.api_key_count > 0) {
                    display_message("Select API key to activate:", COLOR_YELLOW);
                    for (int i = 0; i < config->gemini.api_key_count; i++) {
                        printf("  %d. %s\n", i + 1, config->gemini.api_keys[i].nickname);
                    }
                    
                    char *key_choice = read_line();
                    if (key_choice) {
                        int key_idx = atoi(key_choice) - 1;
                        free(key_choice);
                        
                        if (key_idx >= 0 && key_idx < config->gemini.api_key_count) {
                            if (set_active_gemini_api_key(&config->gemini, config->gemini.api_keys[key_idx].nickname)) {
                                display_message("API key activated successfully.", COLOR_GREEN);
                            } else {
                                display_message("Failed to activate API key.", COLOR_RED);
                            }
                        } else {
                            display_message("Invalid selection.", COLOR_RED);
                        }
                    }
                }
                break;
            case 3:
                if (config->gemini.api_key_count > 0) {
                    display_message("Select API key to remove:", COLOR_YELLOW);
                    for (int i = 0; i < config->gemini.api_key_count; i++) {
                        printf("  %d. %s\n", i + 1, config->gemini.api_keys[i].nickname);
                    }
                    
                    char *remove_choice = read_line();
                    if (remove_choice) {
                        int remove_idx = atoi(remove_choice) - 1;
                        free(remove_choice);
                        
                        if (remove_idx >= 0 && remove_idx < config->gemini.api_key_count) {
                            if (remove_gemini_api_key(&config->gemini, config->gemini.api_keys[remove_idx].nickname)) {
                                display_message("API key removed successfully.", COLOR_GREEN);
                            } else {
                                display_message("Failed to remove API key.", COLOR_RED);
                            }
                        } else {
                            display_message("Invalid selection.", COLOR_RED);
                        }
                    }
                }
                break;
            default:
                display_message("Invalid choice.", COLOR_RED);
                break;
        }
    }
}

bool change_active_model(config_t *config) {
    if (strlen(config->active_llm_service) == 0) {
        display_message("No active LLM service.", COLOR_YELLOW);
        return false;
    }
    
    display_message("Change model for active service: ", COLOR_GREEN);
    printf("%s\n", config->active_llm_service);
    
    if (strcmp(config->active_llm_service, LLM_SERVICE_OLLAMA) == 0) {
        char **models = NULL;
        int model_count = 0;
        if (fetch_ollama_models(config->ollama.server_address, &models, &model_count)) {
            display_message("Available models:", COLOR_GREEN);
            for (int i = 0; i < model_count; i++) {
                bool is_current = (strcmp(models[i], config->ollama.model) == 0);
                printf("  %d. %s%s\n", i + 1, models[i], is_current ? " (current)" : "");
            }
            
            display_message("Enter model number: ", COLOR_YELLOW);
            char *choice = read_line();
            if (choice) {
                int model_idx = atoi(choice) - 1;
                free(choice);
                
                if (model_idx >= 0 && model_idx < model_count) {
                    strcpy(config->ollama.model, models[model_idx]);
                    display_message("Model changed successfully.", COLOR_GREEN);
                } else {
                    display_message("Invalid model selection.", COLOR_RED);
                }
            }
            
            for (int i = 0; i < model_count; i++) {
                free(models[i]);
            }
            free(models);
        }
    } else if (strcmp(config->active_llm_service, LLM_SERVICE_GEMINI) == 0) {
        char nickname[MAX_NICKNAME_LEN];
        char *api_key = get_active_gemini_key_value(&config->gemini, nickname);
        if (api_key) {
            char **models = NULL;
            int model_count = 0;
            if (fetch_gemini_models(api_key, &models, &model_count)) {
                display_message("Available models:", COLOR_GREEN);
                for (int i = 0; i < model_count; i++) {
                    bool is_current = (strcmp(models[i], config->gemini.model) == 0);
                    printf("  %d. %s%s\n", i + 1, models[i], is_current ? " (current)" : "");
                }
                
                display_message("Enter model number: ", COLOR_YELLOW);
                char *choice = read_line();
                if (choice) {
                    int model_idx = atoi(choice) - 1;
                    free(choice);
                    
                    if (model_idx >= 0 && model_idx < model_count) {
                        strcpy(config->gemini.model, models[model_idx]);
                        display_message("Model changed successfully.", COLOR_GREEN);
                    } else {
                        display_message("Invalid model selection.", COLOR_RED);
                    }
                }
                
                for (int i = 0; i < model_count; i++) {
                    free(models[i]);
                }
                free(models);
            }
        } else {
            display_message("No active API key found.", COLOR_RED);
        }
    }
    
    return true;
}

bool switch_llm_service(config_t *config) {
    display_message("Switch active LLM service:", COLOR_GREEN);
    display_message("1. Ollama", COLOR_BLUE);
    display_message("2. Gemini", COLOR_BLUE);
    
    display_message("Enter your choice (1-2): ", COLOR_YELLOW);
    char *choice = read_line();
    if (!choice) {
        return false;
    }
    
    int service_choice = atoi(choice);
    free(choice);
    
    if (service_choice == 1) {
        if (strlen(config->ollama.server_address) == 0) {
            display_message("Ollama not configured. Please configure it first.", COLOR_YELLOW);
            return false;
        }
        strcpy(config->previous_active_llm_service, config->active_llm_service);
        strcpy(config->active_llm_service, LLM_SERVICE_OLLAMA);
        display_message("Switched to Ollama.", COLOR_GREEN);
    } else if (service_choice == 2) {
        if (config->gemini.api_key_count == 0) {
            display_message("Gemini not configured. Please configure it first.", COLOR_YELLOW);
            return false;
        }
        strcpy(config->previous_active_llm_service, config->active_llm_service);
        strcpy(config->active_llm_service, LLM_SERVICE_GEMINI);
        display_message("Switched to Gemini.", COLOR_GREEN);
    } else {
        display_message("Invalid choice.", COLOR_RED);
        return false;
    }
    
    return true;
}

bool toggle_markdown_rendering(config_t *config) {
    if (strlen(config->active_llm_service) == 0) {
        display_message("No active LLM service.", COLOR_YELLOW);
        return false;
    }
    
    if (strcmp(config->active_llm_service, LLM_SERVICE_OLLAMA) == 0) {
        config->ollama.render_markdown = !config->ollama.render_markdown;
        display_message("Markdown rendering for Ollama: ", COLOR_GREEN);
        printf("%s\n", config->ollama.render_markdown ? "Enabled" : "Disabled");
    } else if (strcmp(config->active_llm_service, LLM_SERVICE_GEMINI) == 0) {
        config->gemini.render_markdown = !config->gemini.render_markdown;
        display_message("Markdown rendering for Gemini: ", COLOR_GREEN);
        printf("%s\n", config->gemini.render_markdown ? "Enabled" : "Disabled");
    }
    
    return true;
}

bool toggle_streaming(config_t *config) {
    config->enable_streaming = !config->enable_streaming;
    display_message("Response streaming: ", COLOR_GREEN);
    printf("%s\n", config->enable_streaming ? "Enabled" : "Disabled");
    return true;
}

bool set_history_limit(config_t *config) {
    display_message("Current history limit: ", COLOR_BLUE);
    printf("%d\n", config->interactive_history_limit);
    
    display_message("Enter new history limit (1-50): ", COLOR_YELLOW);
    char *input = read_line();
    if (!input) {
        return false;
    }
    
    int new_limit = atoi(input);
    free(input);
    
    if (new_limit >= 1 && new_limit <= 50) {
        config->interactive_history_limit = new_limit;
        display_message("History limit updated successfully.", COLOR_GREEN);
        return true;
    } else {
        display_message("Invalid history limit. Must be between 1 and 50.", COLOR_RED);
        return false;
    }
}

void show_active_configuration(config_t *config) {
    display_message("\n--- Current Configuration ---", COLOR_GREEN);
    
    display_message("Active LLM Service: ", COLOR_BLUE);
    printf("%s\n", strlen(config->active_llm_service) > 0 ? config->active_llm_service : "None");
    
    display_message("Previous LLM Service: ", COLOR_BLUE);
    printf("%s\n", strlen(config->previous_active_llm_service) > 0 ? config->previous_active_llm_service : "None");
    
    display_message("Interactive History Limit: ", COLOR_BLUE);
    printf("%d\n", config->interactive_history_limit);
    
    display_message("Response Streaming: ", COLOR_BLUE);
    printf("%s\n", config->enable_streaming ? "Enabled" : "Disabled");
    
    display_message("Progress Animation: ", COLOR_BLUE);
    printf("%s\n", config->show_progress_animation ? "Enabled" : "Disabled");
    
    if (strlen(config->ollama.server_address) > 0) {
        display_message("\nOllama Configuration:", COLOR_GREEN);
        display_message("  Server Address: ", COLOR_BLUE);
        printf("%s\n", config->ollama.server_address);
        display_message("  Model: ", COLOR_BLUE);
        printf("%s\n", config->ollama.model);
        display_message("  Markdown Rendering: ", COLOR_BLUE);
        printf("%s\n", config->ollama.render_markdown ? "Enabled" : "Disabled");
    }
    
    if (config->gemini.api_key_count > 0) {
        display_message("\nGemini Configuration:", COLOR_GREEN);
        display_message("  Active API Key: ", COLOR_BLUE);
        printf("%s\n", config->gemini.active_api_key_nickname);
        display_message("  Model: ", COLOR_BLUE);
        printf("%s\n", config->gemini.model);
        display_message("  Markdown Rendering: ", COLOR_BLUE);
        printf("%s\n", config->gemini.render_markdown ? "Enabled" : "Disabled");
        display_message("  API Keys: ", COLOR_BLUE);
        printf("%d configured\n", config->gemini.api_key_count);
    }
}

bool jump_to_previous_llm(config_t *config) {
    if (strlen(config->previous_active_llm_service) == 0) {
        display_message("No previous LLM service to jump to.", COLOR_YELLOW);
        return false;
    }
    // Swap active and previous
    char temp[32];
    strcpy(temp, config->active_llm_service);
    strcpy(config->active_llm_service, config->previous_active_llm_service);
    strcpy(config->previous_active_llm_service, temp);
    display_message("Jumped to previous LLM service: ", COLOR_GREEN);
    printf("%s\n", config->active_llm_service);
    return true;
}

bool toggle_progress_animation(config_t *config) {
    config->show_progress_animation = !config->show_progress_animation;
    display_message("Progress animation: ", COLOR_GREEN);
    printf("%s\n", config->show_progress_animation ? "Enabled" : "Disabled");
    return true;
} 