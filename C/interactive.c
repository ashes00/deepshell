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
    display_message("Type 'exit' or 'quit' to end the session.", COLOR_YELLOW);
    
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