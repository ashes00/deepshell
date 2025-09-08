#include "deepshell.h"

// Logo functions for interactive mode
void print_interactive_logo() {
    // Clear screen and move to top
    printf("\033[2J\033[H");
    
    // Add some spacing
    printf("\n\n");
    
    // DeepShell ASCII Art Logo - actually spelling out "DeepShell"
    printf("%s", COLOR_CYAN);
    printf("    ██████╗ ███████╗███████╗██████╗ ███████╗██╗  ██╗███████╗██╗     ██╗     \n");
    printf("    ██╔══██╗██╔════╝██╔════╝██╔══██╗██╔════╝██║  ██║██╔════╝██║     ██║     \n");
    printf("    ██║  ██║█████╗  █████╗  ██████╔╝███████╗███████║█████╗  ██║     ██║     \n");
    printf("    ██║  ██║██╔══╝  ██╔══╝  ██╔═══╝ ╚════██║██╔══██║██╔══╝  ██║     ██║     \n");
    printf("    ██████╔╝███████╗███████╗██║     ███████║██║  ██║███████╗███████╗███████╗\n");
    printf("    ╚═════╝ ╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝\n");
    printf("%s", COLOR_RESET);
    
    printf("\n");
    
    // Subtitle with version info - PERFECT RECTANGLE
    printf("%s", COLOR_BLUE);
    printf("                    ╔══════════════════════════════════════════════════════════════╗\n");
    printf("                    ║                  AI-Powered Shell Interface                  ║\n");
    printf("                    ║                       Version %s                          ║\n", DEEPSHELL_VERSION);
    printf("                    ╚══════════════════════════════════════════════════════════════╝\n");
    printf("%s", COLOR_RESET);
    
    printf("\n");
    
    // Feature highlights
    printf("%s", COLOR_GREEN);
    printf("    ✨  Multi-LLM Support (Ollama, Gemini, and OpenRouter)    🔧  Interactive Mode    📝  Markdown Rendering\n");
    printf("    🚀  Streaming Responses                    ⚙️   Easy Configuration    💾  Conversation History\n");
    printf("%s", COLOR_RESET);
    
    printf("\n");
    
    // Status indicator
    printf("%s", COLOR_YELLOW);
    printf("    [*] DeepShell is ready to assist you with AI-powered interactions!\n");
    printf("%s", COLOR_RESET);
    
    printf("\n");
}

bool start_interactive_session(config_t *config) {
    if (!config) {
        return false;
    }
    
    // Display the DeepShell logo for interactive mode
    print_interactive_logo();
    
    display_message("\n--- DeepShell Interactive Mode ---", COLOR_GREEN);
    display_message("Type 'help' to see all available commands.", COLOR_YELLOW);
    
    // Check if we have a valid active service
    if (strlen(config->active_llm_service) == 0) {
        display_message("No active LLM service configured.", COLOR_RED);
        return false;
    }
    
    // Validate service configuration
    if (strcmp(config->active_llm_service, LLM_SERVICE_OLLAMA) == 0) {
        if (strlen(config->ollama.server_address) == 0) {
            display_message("Ollama server address not configured.", COLOR_RED);
            return false;
        }
    } else if (strcmp(config->active_llm_service, LLM_SERVICE_GEMINI) == 0) {
        char nickname[MAX_NICKNAME_LEN];
        char *api_key = get_active_gemini_key_value(&config->gemini, nickname);
        if (!api_key) {
            display_message("No active Gemini API key found.", COLOR_RED);
            return false;
        }
    } else if (strcmp(config->active_llm_service, LLM_SERVICE_OPENROUTER) == 0) {
        char nickname[MAX_NICKNAME_LEN];
        char *api_key = get_active_openrouter_key_value(&config->openrouter, nickname);
        if (!api_key) {
            display_message("No active OpenRouter API key found.", COLOR_RED);
            return false;
        }
    } else {
        display_message("Unknown active LLM service.", COLOR_RED);
        return false;
    }
    
    // Initialize conversation history
    conversation_message_t *history = malloc(config->interactive_history_limit * 2 * sizeof(conversation_message_t));
    if (!history) {
        display_message("Failed to allocate conversation history.", COLOR_RED);
        return false;
    }
    
    int history_count = 0;
    int max_history_items = config->interactive_history_limit * 2;
    
    while (true) {
        printf("%s> %s", COLOR_BLUE, COLOR_RESET);
        fflush(stdout);
        
        char *user_input = read_line();
        if (!user_input) {
            display_message("Failed to read input.", COLOR_RED);
            break;
        }
        
        // Check for exit commands
        if (strcmp(user_input, "exit") == 0 || strcmp(user_input, "quit") == 0) {
            display_message("Exiting interactive mode.", COLOR_BLUE);
            free(user_input);
            break;
        }
        
        // Check for save command
        if (strncmp(user_input, "save", 4) == 0) {
            // Check if there's any conversation history (at least one model response)
            bool has_model_response = false;
            for (int i = 0; i < history_count; i++) {
                if (strcmp(history[i].role, "model") == 0) {
                    has_model_response = true;
                    break;
                }
            }
            
            if (!has_model_response) {
                display_message("Please search before saving. No previous response found.", COLOR_YELLOW);
                free(user_input);
                continue;
            }
            
            // Parse filename from command
            char *filename = NULL;
            if (strlen(user_input) > 4) {
                // Skip "save" and any following spaces
                char *filename_start = user_input + 4;
                while (*filename_start == ' ') {
                    filename_start++;
                }
                if (strlen(filename_start) > 0) {
                    filename = strdup_safe(filename_start);
                }
            }
            
            // If no filename provided, ask for it
            if (!filename) {
                display_message("Enter filename (.md extension will be added automatically): ", COLOR_YELLOW);
                filename = read_line();
                if (!filename || strlen(filename) == 0) {
                    display_message("Save cancelled.", COLOR_YELLOW);
                    if (filename) free(filename);
                    free(user_input);
                    continue;
                }
            }
            
            // Auto-append .md extension if not provided
            if (filename && strlen(filename) > 0) {
                // Check if filename already ends with .md
                size_t len = strlen(filename);
                if (len < 3 || strcmp(filename + len - 3, ".md") != 0) {
                    // Append .md extension
                    char *new_filename = malloc(len + 4);
                    if (new_filename) {
                        strcpy(new_filename, filename);
                        strcat(new_filename, ".md");
                        free(filename);
                        filename = new_filename;
                    }
                }
            }
            
            // Validate filename
            if (!is_valid_filename(filename)) {
                display_message("Invalid filename. Please enter a valid filename: ", COLOR_RED);
                free(filename);
                filename = read_line();
                if (!filename || strlen(filename) == 0) {
                    display_message("Save cancelled.", COLOR_YELLOW);
                    if (filename) free(filename);
                    free(user_input);
                    continue;
                }
                
                // Auto-append .md extension for the new filename too
                size_t len = strlen(filename);
                if (len < 3 || strcmp(filename + len - 3, ".md") != 0) {
                    char *new_filename = malloc(len + 4);
                    if (new_filename) {
                        strcpy(new_filename, filename);
                        strcat(new_filename, ".md");
                        free(filename);
                        filename = new_filename;
                    }
                }
                
                if (!is_valid_filename(filename)) {
                    display_message("Save cancelled.", COLOR_YELLOW);
                    if (filename) free(filename);
                    free(user_input);
                    continue;
                }
            }
            
            // Find the last model response
            char *last_response = NULL;
            for (int i = history_count - 1; i >= 0; i--) {
                if (strcmp(history[i].role, "model") == 0) {
                    last_response = history[i].content;
                    break;
                }
            }
            
            if (last_response) {
                if (save_response_to_file(last_response, filename)) {
                    // Success message already printed by save_response_to_file
                } else {
                    display_message("Save failed.", COLOR_RED);
                }
            } else {
                display_message("No model response found to save.", COLOR_RED);
            }
            
            free(filename);
            free(user_input);
            continue;
        }
        
        // Check for open command
        if (strncmp(user_input, "open", 4) == 0) {
            // Parse filepath from command
            char *filepath = NULL;
            if (strlen(user_input) > 4) {
                // Skip "open" and any following spaces
                char *filepath_start = user_input + 4;
                while (*filepath_start == ' ') {
                    filepath_start++;
                }
                if (strlen(filepath_start) > 0) {
                    filepath = strdup_safe(filepath_start);
                }
            }
            
            // If no filepath provided, ask for it
            if (!filepath) {
                display_message("Enter file path: ", COLOR_YELLOW);
                filepath = read_line();
                if (!filepath || strlen(filepath) == 0) {
                    display_message("Open cancelled.", COLOR_YELLOW);
                    if (filepath) free(filepath);
                    free(user_input);
                    continue;
                }
            }
            
            // Read file content first to check if it exists and is readable
            char *file_content = read_file_content(filepath);
            if (!file_content) {
                free(filepath);
                free(user_input);
                continue;
            }
            
            // Check if file is text-based (only after successful read)
            if (!is_text_file(filepath)) {
                display_message("Error: File is not a text-based file (binary detected).", COLOR_RED);
                free(file_content);
                free(filepath);
                free(user_input);
                continue;
            }
            
            // Prompt user for instructions
            display_message("Enter instructions for the LLM about this data:", COLOR_YELLOW);
            char *user_instructions = read_line();
            if (!user_instructions || strlen(user_instructions) == 0) {
                // Use default instructions if none provided
                user_instructions = strdup("Please analyze this data");
            }
            
            // Combine user instructions with file content
            size_t total_len = strlen(user_instructions) + strlen(file_content) + 10; // Extra space for separators
            char *combined_prompt = malloc(total_len);
            if (!combined_prompt) {
                display_message("Error: Not enough memory to process file.", COLOR_RED);
                free(user_instructions);
                free(file_content);
                free(filepath);
                free(user_input);
                continue;
            }
            
            // Format: [User instructions]\n\n[File content]
            strcpy(combined_prompt, user_instructions);
            strcat(combined_prompt, "\n\n");
            strcat(combined_prompt, file_content);
            
            // Add user instructions as separate history entry
            if (history_count < max_history_items) {
                strcpy(history[history_count].role, "user");
                strncpy(history[history_count].content, user_instructions, MAX_RESPONSE_LEN - 1);
                history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                history_count++;
            }
            
            // Add file content as separate history entry
            if (history_count < max_history_items) {
                strcpy(history[history_count].role, "user");
                strncpy(history[history_count].content, file_content, MAX_RESPONSE_LEN - 1);
                history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                history_count++;
            }
            
            // Process the combined prompt as a query
            char *response_text = NULL;
            
            if (strcmp(config->active_llm_service, LLM_SERVICE_OLLAMA) == 0) {
                response_text = send_ollama_query(config->ollama.server_address, 
                                        config->ollama.model, combined_prompt, 
                                        history, history_count, config);
            } else if (strcmp(config->active_llm_service, LLM_SERVICE_GEMINI) == 0) {
                char nickname[MAX_NICKNAME_LEN];
                char *api_key = get_active_gemini_key_value(&config->gemini, nickname);
                if (api_key) {
                    response_text = send_gemini_query(api_key, config->gemini.model, 
                                            combined_prompt, history, history_count, config);
                } else {
                    display_message("No active Gemini API key found.", COLOR_RED);
                }
            } else if (strcmp(config->active_llm_service, LLM_SERVICE_OPENROUTER) == 0) {
                char nickname[MAX_NICKNAME_LEN];
                char *api_key = get_active_openrouter_key_value(&config->openrouter, nickname);
                if (api_key) {
                    response_text = send_openrouter_query(api_key, 
                                            config->openrouter.model, combined_prompt, 
                                            history, history_count, config);
                } else {
                    display_message("No active OpenRouter API key found.", COLOR_RED);
                }
            } else {
                display_message("No active LLM service configured.", COLOR_RED);
            }
            
            // Display response
            if (response_text) {
                if (config->enable_streaming) {
                    display_message("Response:", COLOR_GREEN);
                    printf("%s\n", response_text);
                } else {
                    display_message("Response:", COLOR_GREEN);
                    print_markdown(response_text);
                }
                
                // Add model response to history
                if (history_count < max_history_items) {
                    strcpy(history[history_count].role, "model");
                    strncpy(history[history_count].content, response_text, MAX_RESPONSE_LEN - 1);
                    history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                    history_count++;
                }
                free(response_text);
            }
            
            free(combined_prompt);
            free(user_instructions);
            free(file_content);
            free(filepath);
            free(user_input);
            continue;
        }
        
        // Check for help command
        if (strncmp(user_input, "help", 4) == 0) {
            display_interactive_help();
            free(user_input);
            continue;
        }
        
        // Skip empty input
        if (strlen(user_input) == 0) {
            free(user_input);
            continue;
        }
        
        // Trim history if it's too long
        if (max_history_items > 0 && history_count >= max_history_items) {
            // Keep the last N-1 pairs to make room for the new one
            for (int i = 0; i < history_count - 2; i++) {
                strcpy(history[i].role, history[i + 2].role);
                strcpy(history[i].content, history[i + 2].content);
            }
            history_count -= 2;
        }
        
        // Send query to appropriate service
        bool query_success = false;
        
        if (strcmp(config->active_llm_service, LLM_SERVICE_OLLAMA) == 0) {
            char *response_text = send_ollama_query(config->ollama.server_address, 
                                            config->ollama.model, user_input, 
                                            history, history_count, config);
            query_success = (response_text != NULL);
            if (query_success) {
                // Print response
                display_message("--- Ollama Response ---", COLOR_GREEN);
                if (config->ollama.render_markdown) {
                    print_markdown(response_text);
                } else {
                    printf("%s\n", response_text);
                }
                display_message("-----------------------", COLOR_GREEN);
            }
            if (response_text) {
                // Add model response to history
                if (history_count < max_history_items) {
                    strcpy(history[history_count].role, "model");
                    strncpy(history[history_count].content, response_text, MAX_RESPONSE_LEN - 1);
                    history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                    history_count++;
                }
                free(response_text);
            }
        } else if (strcmp(config->active_llm_service, LLM_SERVICE_GEMINI) == 0) {
            char nickname[MAX_NICKNAME_LEN];
            char *api_key = get_active_gemini_key_value(&config->gemini, nickname);
            if (api_key) {
                char *response_text = send_gemini_query(api_key, config->gemini.model, 
                                                user_input, history, history_count, config);
                query_success = (response_text != NULL);
                if (query_success) {
                    display_message("--- Gemini Response ---", COLOR_GREEN);
                    if (config->gemini.render_markdown) {
                        print_markdown(response_text);
                    } else {
                        printf("%s\n", response_text);
                    }
                    display_message("------------------------", COLOR_GREEN);
                }
                if (response_text) {
                    // Add model response to history
                    if (history_count < max_history_items) {
                        strcpy(history[history_count].role, "model");
                        strncpy(history[history_count].content, response_text, MAX_RESPONSE_LEN - 1);
                        history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                        history_count++;
                    }
                    free(response_text);
                }
            }
        } else if (strcmp(config->active_llm_service, LLM_SERVICE_OPENROUTER) == 0) {
            char nickname[MAX_NICKNAME_LEN];
            char *api_key = get_active_openrouter_key_value(&config->openrouter, nickname);
            if (api_key) {
                char *response_text = send_openrouter_query(api_key, 
                                                config->openrouter.model, user_input, 
                                                history, history_count, config);
            query_success = (response_text != NULL);
            if (query_success) {
                // Print response
                display_message("--- OpenRouter Response ---", COLOR_GREEN);
                if (config->openrouter.render_markdown) {
                    print_markdown(response_text);
                } else {
                    printf("%s\n", response_text);
                }
                display_message("---------------------------", COLOR_GREEN);
            }
            if (response_text) {
                // Add model response to history
                if (history_count < max_history_items) {
                    strcpy(history[history_count].role, "model");
                    strncpy(history[history_count].content, response_text, MAX_RESPONSE_LEN - 1);
                    history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                    history_count++;
                }
                free(response_text);
            }
            }
        }
        
        if (query_success) {
            // Add to history
            if (history_count < max_history_items) {
                strcpy(history[history_count].role, "user");
                strncpy(history[history_count].content, user_input, MAX_RESPONSE_LEN - 1);
                history[history_count].content[MAX_RESPONSE_LEN - 1] = '\0';
                history_count++;
            }
        } else {
            display_message("Failed to get response from LLM.", COLOR_RED);
        }
        
        free(user_input);
    }
    
    free(history);
    return true;
} 