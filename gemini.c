#include "deepshell.h"

bool fetch_gemini_models(const char *api_key, char ***models, int *model_count) {
    if (!api_key || !models || !model_count) {
        return false;
    }
    
    display_message("Fetching models from Gemini API...", COLOR_BLUE);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s/models?key=%s", GEMINI_API_BASE_URL, api_key);
    
    char *response = NULL;
    struct curl_slist *headers = create_headers();
    
    bool success = make_http_request(url, NULL, headers, &response);
    free_headers(headers);
    
    if (!success || !response) {
        display_message("Failed to fetch models from Gemini API.", COLOR_RED);
        return false;
    }
    
    // Parse JSON response
    json_object *json_obj = json_tokener_parse(response);
    free(response);
    
    if (!json_obj) {
        display_message("Failed to parse JSON response from Gemini API.", COLOR_RED);
        return false;
    }
    
    json_object *models_array;
    if (!json_object_object_get_ex(json_obj, "models", &models_array)) {
        display_message("Invalid response format from Gemini API.", COLOR_RED);
        json_object_put(json_obj);
        return false;
    }
    
    int array_length = json_object_array_length(models_array);
    *model_count = 0;
    *models = malloc(array_length * sizeof(char*));
    
    if (!*models) {
        json_object_put(json_obj);
        return false;
    }
    
    for (int i = 0; i < array_length; i++) {
        json_object *model_obj = json_object_array_get_idx(models_array, i);
        json_object *name_obj, *methods_array;
        
        if (json_object_object_get_ex(model_obj, "name", &name_obj) &&
            json_object_object_get_ex(model_obj, "supportedGenerationMethods", &methods_array)) {
            
            const char *model_name = json_object_get_string(name_obj);
            
            // Check if model supports generation and is not a chat/TTS model
            bool supports_generation = false;
            int methods_length = json_object_array_length(methods_array);
            
            for (int j = 0; j < methods_length; j++) {
                json_object *method = json_object_array_get_idx(methods_array, j);
                const char *method_str = json_object_get_string(method);
                if (strstr(method_str, "generateContent") || strstr(method_str, "generateAnswer")) {
                    supports_generation = true;
                    break;
                }
            }
            
            // Skip chat and TTS models
            if (supports_generation && 
                !strstr(model_name, "chat") && 
                !strstr(model_name, "tts")) {
                (*models)[*model_count] = strdup_safe(model_name);
                if ((*models)[*model_count]) {
                    (*model_count)++;
                }
            }
        }
    }
    
    json_object_put(json_obj);
    return true;
}

char* send_gemini_query(const char *api_key, const char *model_name,
                      const char *user_query, conversation_message_t *history,
                      int history_count, config_t *config) {
    if (!api_key || !model_name || !user_query || !config) {
        return NULL;
    }
    
    char status_message[MAX_RESPONSE_LEN];
    snprintf(status_message, sizeof(status_message), 
             "Using active LLM service: Gemini. Sending query to %s...", model_name);
    
    // Start concurrent progress animation
    start_progress_animation(status_message, config->show_progress_animation);
    
    // Create JSON payload
    json_object *payload = create_gemini_payload(model_name, user_query, history, history_count);
    if (!payload) {
        display_message("Failed to create request payload.", COLOR_RED);
        return NULL;
    }
    
    const char *json_string = json_object_to_json_string(payload);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s/%s:generateContent?key=%s", 
             GEMINI_API_BASE_URL, model_name, api_key);
    
    char *response = NULL;
    struct curl_slist *headers = create_headers();
    
    bool success = make_http_request(url, json_string, headers, &response);
    free_headers(headers);
    json_object_put(payload);
    
    if (!success || !response) {
        stop_progress_animation();
        display_message("Failed to send query to Gemini API.", COLOR_RED);
        return NULL;
    }
    
    // Parse response
    json_object *response_obj = json_tokener_parse(response);
    free(response);
    
    if (!response_obj) {
        stop_progress_animation();
        display_message("Failed to parse response from Gemini API.", COLOR_RED);
        return NULL;
    }
    
    char *response_text = extract_response_from_json(response_obj, LLM_SERVICE_GEMINI);
    json_object_put(response_obj);
    
    if (!response_text) {
        stop_progress_animation();
        display_message("Failed to extract response from Gemini API.", COLOR_RED);
        return NULL;
    }
    // Stop animation right before returning the text so caller can print immediately
    stop_progress_animation();
    
    // The caller is responsible for printing and freeing response_text
    return response_text;
}

char* get_active_gemini_key_value(const gemini_config_t *gemini_config, char *nickname) {
    if (!gemini_config || !nickname) {
        return NULL;
    }
    
    const char *active_nickname = gemini_config->active_api_key_nickname;
    if (strlen(active_nickname) == 0) {
        return NULL;
    }
    
    strcpy(nickname, active_nickname);
    
    for (int i = 0; i < gemini_config->api_key_count; i++) {
        if (strcmp(gemini_config->api_keys[i].nickname, active_nickname) == 0) {
            return (char*)gemini_config->api_keys[i].key;
        }
    }
    
    return NULL;
}

bool add_gemini_api_key(gemini_config_t *gemini_config, const char *nickname, const char *key) {
    if (!gemini_config || !nickname || !key) {
        return false;
    }
    
    // Check if nickname already exists
    for (int i = 0; i < gemini_config->api_key_count; i++) {
        if (strcmp(gemini_config->api_keys[i].nickname, nickname) == 0) {
            display_message("Nickname already exists.", COLOR_RED);
            return false;
        }
    }
    
    // Check if we have space for another key
    if (gemini_config->api_key_count >= MAX_SERVICES) {
        display_message("Maximum number of API keys reached.", COLOR_RED);
        return false;
    }
    
    // Add the new key
    strcpy(gemini_config->api_keys[gemini_config->api_key_count].nickname, nickname);
    strcpy(gemini_config->api_keys[gemini_config->api_key_count].key, key);
    gemini_config->api_key_count++;
    
    // Set as active if it's the first key
    if (gemini_config->api_key_count == 1) {
        strcpy(gemini_config->active_api_key_nickname, nickname);
    }
    
    return true;
}

bool remove_gemini_api_key(gemini_config_t *gemini_config, const char *nickname) {
    if (!gemini_config || !nickname) {
        return false;
    }
    
    for (int i = 0; i < gemini_config->api_key_count; i++) {
        if (strcmp(gemini_config->api_keys[i].nickname, nickname) == 0) {
            // Remove the key by shifting remaining keys
            for (int j = i; j < gemini_config->api_key_count - 1; j++) {
                strcpy(gemini_config->api_keys[j].nickname, gemini_config->api_keys[j + 1].nickname);
                strcpy(gemini_config->api_keys[j].key, gemini_config->api_keys[j + 1].key);
            }
            gemini_config->api_key_count--;
            
            // Update active key if the removed key was active
            if (strcmp(gemini_config->active_api_key_nickname, nickname) == 0) {
                if (gemini_config->api_key_count > 0) {
                    strcpy(gemini_config->active_api_key_nickname, 
                          gemini_config->api_keys[0].nickname);
                } else {
                    gemini_config->active_api_key_nickname[0] = '\0';
                }
            }
            
            return true;
        }
    }
    
    return false;
}

bool set_active_gemini_api_key(gemini_config_t *gemini_config, const char *nickname) {
    if (!gemini_config || !nickname) {
        return false;
    }
    
    // Check if the nickname exists
    for (int i = 0; i < gemini_config->api_key_count; i++) {
        if (strcmp(gemini_config->api_keys[i].nickname, nickname) == 0) {
            strcpy(gemini_config->active_api_key_nickname, nickname);
            return true;
        }
    }
    
    return false;
}

json_object* create_gemini_payload(const char *model, const char *query,
                                  conversation_message_t *history, int history_count) {
    (void)model; // Suppress unused parameter warning
    json_object *payload = json_object_new_object();
    if (!payload) {
        return NULL;
    }
    
    // Add contents array
    json_object *contents = json_object_new_array();
    if (!contents) {
        json_object_put(payload);
        return NULL;
    }
    
    // Add conversation history if provided
    if (history && history_count > 0) {
        for (int i = 0; i < history_count; i++) {
            json_object *content = json_object_new_object();
            if (!content) {
                continue;
            }
            
            json_object *parts = json_object_new_array();
            if (!parts) {
                json_object_put(content);
                continue;
            }
            
            json_object *part = json_object_new_object();
            if (part) {
                json_object_object_add(part, "text", json_object_new_string(history[i].content));
                json_object_array_add(parts, part);
            }
            json_object_object_add(content, "parts", parts);
            // Add the required role field
            json_object_object_add(content, "role", json_object_new_string(history[i].role));
            json_object_array_add(contents, content);
        }
    }
    
    // Add current query
    json_object *current_content = json_object_new_object();
    if (current_content) {
        json_object *parts = json_object_new_array();
        if (parts) {
            json_object *part = json_object_new_object();
            if (part) {
                json_object_object_add(part, "text", json_object_new_string(query));
                json_object_array_add(parts, part);
            }
            json_object_object_add(current_content, "parts", parts);
        }
        // Always set role to 'user' for the current query
        json_object_object_add(current_content, "role", json_object_new_string("user"));
        json_object_array_add(contents, current_content);
    }
    
    json_object_object_add(payload, "contents", contents);
    
    return payload;
} 