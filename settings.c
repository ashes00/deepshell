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
        display_message("3. OpenRouter", COLOR_BLUE);
        display_message("4. Cancel", COLOR_BLUE);
        
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
            setup_success = configure_openrouter_service(config);
        } else if (service_choice == 4) {
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
            display_message("4. Manage OpenRouter API Key", COLOR_BLUE);
            display_message("5. Change Model for Active Service", COLOR_BLUE);
            display_message("6. Toggle Markdown Rendering for Active Service", COLOR_BLUE);
            display_message("7. Set Interactive History Limit", COLOR_BLUE);
            display_message("8. Toggle Response Streaming", COLOR_BLUE);
            display_message("9. Toggle Progress Animation", COLOR_BLUE);
            display_message("10. View Active Configuration", COLOR_BLUE);
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
                        display_message("3. OpenRouter", COLOR_BLUE);
                        if (config->openrouter.api_key_count > 0) {
                            display_message("   (already configured)", COLOR_YELLOW);
                        }
                        display_message("4. Cancel", COLOR_BLUE);
                        
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
                            setup_success = configure_openrouter_service(config);
                        } else if (choice == 4) {
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
                    // Manage OpenRouter API Key
                    if (manage_openrouter_api_key(config)) {
                        save_config(config);
                    }
                    break;
                case 5:
                    // Change Model for Active Service
                    if (change_active_model(config)) {
                        save_config(config);
                    }
                    break;
                case 6:
                    // Toggle Markdown Rendering for Active Service
                    if (toggle_markdown_rendering(config)) {
                        save_config(config);
                    }
                    break;
                case 7:
                    // Set Interactive History Limit
                    if (set_history_limit(config)) {
                        save_config(config);
                    }
                    break;
                case 8:
                    // Toggle Response Streaming
                    if (toggle_streaming(config)) {
                        save_config(config);
                    }
                    break;
                case 9:
                    // Toggle Progress Animation
                    if (toggle_progress_animation(config)) {
                        save_config(config);
                    }
                    break;
                case 10:
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
    } else if (strcmp(config->active_llm_service, LLM_SERVICE_OPENROUTER) == 0) {
        char nickname[MAX_NICKNAME_LEN];
        char *api_key = get_active_openrouter_key_value(&config->openrouter, nickname);
        if (!api_key) {
            display_message("No active OpenRouter API key configured.", COLOR_RED);
            return false;
        }
        
        openrouter_model_t *models = NULL;
        int model_count = 0;
        if (fetch_openrouter_models_detailed(api_key, &models, &model_count)) {
            int current_page = 0;
            const int models_per_page = 15;
            int total_pages = (model_count + models_per_page - 1) / models_per_page;
            
            while (true) {
                // Display header
                display_message("\n=== OpenRouter Models (FREE models listed first) ===", COLOR_GREEN);
                printf("Page %d of %d (%d total models)\n\n", current_page + 1, total_pages, model_count);
                
                // Calculate display range for current page
                int start_idx = current_page * models_per_page;
                int end_idx = start_idx + models_per_page;
                if (end_idx > model_count) end_idx = model_count;
                
                // Display models for current page
                for (int i = start_idx; i < end_idx; i++) {
                    bool is_current = (strcmp(models[i].id, config->openrouter.model) == 0);
                    const char *status_icon = models[i].is_free ? "🆓" : "💰";
                    
                    printf("  %2d. %s %s%s\n", 
                           i + 1, 
                           status_icon,
                           models[i].id, 
                           is_current ? " (current)" : "");
                }
                
                // Display navigation options
                printf("\nOptions:\n");
                printf("  Enter model number (1-%d) to select\n", model_count);
                if (current_page > 0) printf("  'p' for previous page\n");
                if (current_page < total_pages - 1) printf("  'n' for next page\n");
                printf("  'q' to quit without changing\n");
                
                display_message("\nYour choice: ", COLOR_YELLOW);
                char *choice = read_line();
                if (!choice) break;
                
                // Handle navigation
                if (strcmp(choice, "n") == 0 && current_page < total_pages - 1) {
                    current_page++;
                    free(choice);
                    continue;
                } else if (strcmp(choice, "p") == 0 && current_page > 0) {
                    current_page--;
                    free(choice);
                    continue;
                } else if (strcmp(choice, "q") == 0) {
                    free(choice);
                    break;
                }
                
                // Handle model selection
                int model_idx = atoi(choice) - 1;
                free(choice);
                
                if (model_idx >= 0 && model_idx < model_count) {
                    strncpy(config->openrouter.model, models[model_idx].id, MAX_MODEL_NAME_LEN - 1);
                    config->openrouter.model[MAX_MODEL_NAME_LEN - 1] = '\0';
                    
                    display_message("Model changed successfully to: ", COLOR_GREEN);
                    printf("%s %s\n", 
                           models[model_idx].is_free ? "🆓" : "💰",
                           models[model_idx].id);
                    break;
                } else {
                    display_message("Invalid selection. Try again.", COLOR_RED);
                }
            }
            
            // Free models
            free(models);
        } else {
            display_message("Failed to fetch models from OpenRouter. Please check your API key.", COLOR_RED);
            return false;
        }
    } else {
        display_message("Unknown active LLM service.", COLOR_RED);
        return false;
    }
    
    return true;
}

bool switch_llm_service(config_t *config) {
    display_message("Switch active LLM service:", COLOR_GREEN);
    display_message("1. Ollama", COLOR_BLUE);
    display_message("2. Gemini", COLOR_BLUE);
    display_message("3. OpenRouter", COLOR_BLUE);
    
    display_message("Enter your choice (1-3): ", COLOR_YELLOW);
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
    } else if (service_choice == 3) {
        if (config->openrouter.api_key_count == 0) {
            display_message("OpenRouter not configured. Please configure it first.", COLOR_YELLOW);
            return false;
        }
        strcpy(config->previous_active_llm_service, config->active_llm_service);
        strcpy(config->active_llm_service, LLM_SERVICE_OPENROUTER);
        display_message("Switched to OpenRouter.", COLOR_GREEN);
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
    
    if (strcmp(config->active_llm_service, LLM_SERVICE_OPENROUTER) == 0 || strcmp(config->active_llm_service, "") == 0) {
        display_message("\n--- OpenRouter Configuration ---", COLOR_GREEN);
        display_message("  Active API Key: ", COLOR_BLUE);
        if (config->openrouter.api_key_count > 0) {
            printf("%s\n", config->openrouter.active_api_key_nickname);
        } else {
            printf("Not configured\n");
        }
        display_message("  Model: ", COLOR_BLUE);
        printf("%s\n", config->openrouter.model);
        display_message("  Site URL: ", COLOR_BLUE);
        printf("%s\n", config->openrouter.site_url);
        display_message("  Site Name: ", COLOR_BLUE);
        printf("%s\n", config->openrouter.site_name);
        display_message("  Markdown Rendering: ", COLOR_BLUE);
        printf("%s\n", config->openrouter.render_markdown ? "Enabled" : "Disabled");
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

bool configure_openrouter_service(config_t *config) {
    display_message("\n--- OpenRouter Service Setup ---", COLOR_GREEN);
    
    // Get API key
    display_message("Enter your OpenRouter API key: ", COLOR_YELLOW);
    char *api_key = read_line();
    if (!api_key || strlen(api_key) == 0) {
        display_message("API key is required for OpenRouter.", COLOR_RED);
        if (api_key) free(api_key);
        return false;
    }
    
    // Trim whitespace and validate
    char *trimmed_key = trim_whitespace(api_key);
    if (strlen(trimmed_key) == 0) {
        display_message("Invalid API key.", COLOR_RED);
        free(api_key);
        return false;
    }
    
    // Add the API key with a default nickname
    char default_nickname[MAX_NICKNAME_LEN];
    snprintf(default_nickname, sizeof(default_nickname), "default-%d", (int)time(NULL) % 1000);
    
    if (!add_openrouter_api_key(&config->openrouter, default_nickname, trimmed_key)) {
        display_message("Failed to add API key.", COLOR_RED);
        free(api_key);
        return false;
    }
    free(api_key);
    
    // Get optional site URL for attribution
    display_message("Enter your site URL (optional, for rankings): ", COLOR_YELLOW);
    char *site_url = read_line();
    if (site_url) {
        char *trimmed_url = trim_whitespace(site_url);
        strncpy(config->openrouter.site_url, trimmed_url, MAX_SERVER_ADDR_LEN - 1);
        config->openrouter.site_url[MAX_SERVER_ADDR_LEN - 1] = '\0';
        free(site_url);
    }
    
    // Get optional site name for attribution
    display_message("Enter your site name (optional, for rankings): ", COLOR_YELLOW);
    char *site_name = read_line();
    if (site_name) {
        char *trimmed_name = trim_whitespace(site_name);
        strncpy(config->openrouter.site_name, trimmed_name, MAX_NICKNAME_LEN - 1);
        config->openrouter.site_name[MAX_NICKNAME_LEN - 1] = '\0';
        free(site_name);
    }
    
    // Test the API key by fetching models
    openrouter_model_t *models = NULL;
    int model_count = 0;
    
    char nickname[MAX_NICKNAME_LEN];
    char *test_api_key = get_active_openrouter_key_value(&config->openrouter, nickname);
    if (!test_api_key || !fetch_openrouter_models_detailed(test_api_key, &models, &model_count)) {
        display_message("Failed to validate API key or fetch models. Please check your API key.", COLOR_RED);
        return false;
    }
    
    // Show available models and let user choose (first 10 free models, then 10 paid)
    display_message("\nTop recommended models:", COLOR_GREEN);
    int free_count = 0;
    int paid_count = 0;
    
    // Show free models first
    display_message("\n🆓 FREE Models:", COLOR_BLUE);
    for (int i = 0; i < model_count && free_count < 10; i++) {
        if (models[i].is_free) {
            printf("  %d. %s\n", free_count + 1, models[i].id);
            free_count++;
        }
    }
    
    // Show some paid models
    display_message("\n💰 PAID Models (top options):", COLOR_BLUE);
    for (int i = 0; i < model_count && paid_count < 10; i++) {
        if (!models[i].is_free) {
            printf("  %d. %s\n", free_count + paid_count + 1, models[i].id);
            paid_count++;
        }
    }
    
    printf("\nTotal models available: %d (showing %d recommended)\n", model_count, free_count + paid_count);
    
    display_message("\nEnter the number of your preferred model (or 0 for default): ", COLOR_YELLOW);
    char *model_choice = read_line();
    if (!model_choice) {
        free(models);
        return false;
    }
    
    int choice = atoi(model_choice);
    free(model_choice);
    
    if (choice > 0 && choice <= (free_count + paid_count)) {
        // Find the selected model in the original array
        int selected_idx = -1;
        int display_idx = 1;
        
        // Check free models first
        for (int i = 0; i < model_count && display_idx <= free_count; i++) {
            if (models[i].is_free) {
                if (display_idx == choice) {
                    selected_idx = i;
                    break;
                }
                display_idx++;
            }
        }
        
        // Check paid models if not found in free models
        if (selected_idx == -1) {
            for (int i = 0; i < model_count && display_idx <= (free_count + paid_count); i++) {
                if (!models[i].is_free) {
                    if (display_idx == choice) {
                        selected_idx = i;
                        break;
                    }
                    display_idx++;
                }
            }
        }
        
        if (selected_idx >= 0) {
            strncpy(config->openrouter.model, models[selected_idx].id, MAX_MODEL_NAME_LEN - 1);
            config->openrouter.model[MAX_MODEL_NAME_LEN - 1] = '\0';
        } else {
            strcpy(config->openrouter.model, "openai/gpt-4o-mini");
        }
    } else {
        // Use a sensible free default
        strcpy(config->openrouter.model, "microsoft/phi-3-mini-128k-instruct:free");
    }
    
    // Free models
    free(models);
    
    // Set as active service
    strcpy(config->previous_active_llm_service, config->active_llm_service);
    strcpy(config->active_llm_service, LLM_SERVICE_OPENROUTER);
    
    display_message("OpenRouter configured successfully!", COLOR_GREEN);
    display_message("Selected model: ", COLOR_BLUE);
    printf("%s\n", config->openrouter.model);
    
    return true;
}

bool manage_openrouter_api_key(config_t *config) {
    display_message("\n--- OpenRouter API Key Management ---", COLOR_GREEN);
    
    while (true) {
        display_message("\nCurrent API keys:", COLOR_BLUE);
        if (config->openrouter.api_key_count == 0) {
            display_message("No API keys configured.", COLOR_YELLOW);
        } else {
            for (int i = 0; i < config->openrouter.api_key_count; i++) {
                bool is_active = (strcmp(config->openrouter.api_keys[i].nickname, 
                                       config->openrouter.active_api_key_nickname) == 0);
                printf("  %d. %s%s\n", i + 1, config->openrouter.api_keys[i].nickname,
                       is_active ? " (active)" : "");
            }
        }
        
        display_message("\nOptions:", COLOR_BLUE);
        display_message("1. Add new API key", COLOR_BLUE);
        if (config->openrouter.api_key_count > 0) {
            display_message("2. Set API key as active", COLOR_BLUE);
            display_message("3. Remove API key", COLOR_BLUE);
        }
        display_message("0. Back to main menu", COLOR_BLUE);
        // Add C and X options as in Gemini version
        bool has_active = (config->openrouter.api_key_count > 0 && strlen(config->openrouter.active_api_key_nickname) > 0);
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
            display_message("OpenRouter key management cancelled.", COLOR_YELLOW);
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
                        if (nickname) free(nickname);
                        break;
                    }
                    
                    display_message("Enter the API key: ", COLOR_YELLOW);
                    char *api_key = read_line();
                    if (!api_key || strlen(api_key) == 0) {
                        display_message("API key cannot be empty.", COLOR_RED);
                        if (nickname) free(nickname);
                        if (api_key) free(api_key);
                        break;
                    }
                    
                    if (add_openrouter_api_key(&config->openrouter, nickname, api_key)) {
                        display_message("API key added successfully.", COLOR_GREEN);
                    } else {
                        display_message("Failed to add API key.", COLOR_RED);
                    }
                    
                    free(nickname);
                    free(api_key);
                }
                break;
            case 2:
                if (config->openrouter.api_key_count > 0) {
                    display_message("Select API key to activate:", COLOR_YELLOW);
                    for (int i = 0; i < config->openrouter.api_key_count; i++) {
                        printf("  %d. %s\n", i + 1, config->openrouter.api_keys[i].nickname);
                    }
                    
                    char *key_choice = read_line();
                    if (key_choice) {
                        int idx = atoi(key_choice) - 1;
                        if (idx >= 0 && idx < config->openrouter.api_key_count) {
                            if (set_active_openrouter_api_key(&config->openrouter, config->openrouter.api_keys[idx].nickname)) {
                                display_message("API key activated successfully.", COLOR_GREEN);
                            }
                        } else {
                            display_message("Invalid selection.", COLOR_RED);
                        }
                        free(key_choice);
                    }
                } else {
                    display_message("No API keys available.", COLOR_YELLOW);
                }
                break;
            case 3:
                if (config->openrouter.api_key_count > 0) {
                    display_message("Select API key to remove:", COLOR_YELLOW);
                    for (int i = 0; i < config->openrouter.api_key_count; i++) {
                        printf("  %d. %s\n", i + 1, config->openrouter.api_keys[i].nickname);
                    }
                    
                    char *key_choice = read_line();
                    if (key_choice) {
                        int idx = atoi(key_choice) - 1;
                        if (idx >= 0 && idx < config->openrouter.api_key_count) {
                            char *nickname = config->openrouter.api_keys[idx].nickname;
                            display_message("Are you sure you want to remove this API key? (y/N): ", COLOR_YELLOW);
                            char *confirm = read_line();
                            if (confirm && (confirm[0] == 'y' || confirm[0] == 'Y')) {
                                if (remove_openrouter_api_key(&config->openrouter, nickname)) {
                                    display_message("API key removed successfully.", COLOR_GREEN);
                                }
                            } else {
                                display_message("Removal cancelled.", COLOR_YELLOW);
                            }
                            if (confirm) free(confirm);
                        } else {
                            display_message("Invalid selection.", COLOR_RED);
                        }
                        free(key_choice);
                    }
                } else {
                    display_message("No API keys to remove.", COLOR_YELLOW);
                }
                break;
            default:
                display_message("Invalid choice.", COLOR_RED);
                break;
        }
    }
}

// OpenRouter API key management functions (matching Gemini's functionality)
char* get_active_openrouter_key_value(const openrouter_config_t *openrouter_config, char *nickname) {
    if (!openrouter_config || !nickname) {
        return NULL;
    }
    
    const char *active_nickname = openrouter_config->active_api_key_nickname;
    if (strlen(active_nickname) == 0) {
        return NULL;
    }
    
    strcpy(nickname, active_nickname);
    
    for (int i = 0; i < openrouter_config->api_key_count; i++) {
        if (strcmp(openrouter_config->api_keys[i].nickname, active_nickname) == 0) {
            return (char*)openrouter_config->api_keys[i].key;
        }
    }
    
    return NULL;
}

bool add_openrouter_api_key(openrouter_config_t *openrouter_config, const char *nickname, const char *key) {
    if (!openrouter_config || !nickname || !key) {
        return false;
    }
    
    // Check if nickname already exists
    for (int i = 0; i < openrouter_config->api_key_count; i++) {
        if (strcmp(openrouter_config->api_keys[i].nickname, nickname) == 0) {
            display_message("Nickname already exists.", COLOR_RED);
            return false;
        }
    }
    
    // Check if we have space for another key
    if (openrouter_config->api_key_count >= MAX_SERVICES) {
        display_message("Maximum number of API keys reached.", COLOR_RED);
        return false;
    }
    
    // Add the new key
    strcpy(openrouter_config->api_keys[openrouter_config->api_key_count].nickname, nickname);
    strcpy(openrouter_config->api_keys[openrouter_config->api_key_count].key, key);
    openrouter_config->api_key_count++;
    
    // Set as active if it's the first key
    if (openrouter_config->api_key_count == 1) {
        strcpy(openrouter_config->active_api_key_nickname, nickname);
    }
    
    return true;
}

bool remove_openrouter_api_key(openrouter_config_t *openrouter_config, const char *nickname) {
    if (!openrouter_config || !nickname) {
        return false;
    }
    
    // Find the key to remove
    int found_index = -1;
    for (int i = 0; i < openrouter_config->api_key_count; i++) {
        if (strcmp(openrouter_config->api_keys[i].nickname, nickname) == 0) {
            found_index = i;
            break;
        }
    }
    
    if (found_index == -1) {
        display_message("API key not found.", COLOR_RED);
        return false;
    }
    
    // Shift remaining keys
    for (int i = found_index; i < openrouter_config->api_key_count - 1; i++) {
        strcpy(openrouter_config->api_keys[i].nickname, openrouter_config->api_keys[i + 1].nickname);
        strcpy(openrouter_config->api_keys[i].key, openrouter_config->api_keys[i + 1].key);
    }
    
    openrouter_config->api_key_count--;
    
    // Clear the active key if it was the one removed
    if (strcmp(openrouter_config->active_api_key_nickname, nickname) == 0) {
        if (openrouter_config->api_key_count > 0) {
            // Set the first remaining key as active
            strcpy(openrouter_config->active_api_key_nickname, openrouter_config->api_keys[0].nickname);
        } else {
            // No keys left
            memset(openrouter_config->active_api_key_nickname, 0, MAX_NICKNAME_LEN);
        }
    }
    
    return true;
}

bool set_active_openrouter_api_key(openrouter_config_t *openrouter_config, const char *nickname) {
    if (!openrouter_config || !nickname) {
        return false;
    }
    
    // Check if the nickname exists
    for (int i = 0; i < openrouter_config->api_key_count; i++) {
        if (strcmp(openrouter_config->api_keys[i].nickname, nickname) == 0) {
            strcpy(openrouter_config->active_api_key_nickname, nickname);
            return true;
        }
    }
    
    display_message("API key nickname not found.", COLOR_RED);
    return false;
} 