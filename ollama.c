#include "deepshell.h"

bool fetch_ollama_models(const char *server_address, char ***models, int *model_count) {
    if (!server_address || !models || !model_count) {
        return false;
    }
    
    display_message("Fetching models from Ollama server...", COLOR_BLUE);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s/api/tags", server_address);
    
    char *response = NULL;
    struct curl_slist *headers = create_headers();
    
    bool success = make_http_request(url, NULL, headers, &response);
    free_headers(headers);
    
    if (!success || !response) {
        display_message("Failed to fetch models from Ollama server.", COLOR_RED);
        return false;
    }
    
    // Parse JSON response
    json_object *json_obj = json_tokener_parse(response);
    free(response);
    
    if (!json_obj) {
        display_message("Failed to parse JSON response from Ollama server.", COLOR_RED);
        return false;
    }
    
    json_object *models_array;
    if (!json_object_object_get_ex(json_obj, "models", &models_array)) {
        display_message("Invalid response format from Ollama server.", COLOR_RED);
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
        json_object *name_obj;
        
        if (json_object_object_get_ex(model_obj, "name", &name_obj)) {
            const char *model_name = json_object_get_string(name_obj);
            (*models)[*model_count] = strdup_safe(model_name);
            if ((*models)[*model_count]) {
                (*model_count)++;
            }
        }
    }
    
    json_object_put(json_obj);
    return true;
}

char* send_ollama_query(const char *server_address, const char *model_name, 
                      const char *user_query, conversation_message_t *history, 
                      int history_count, config_t *config) {
    if (!server_address || !model_name || !user_query || !config) {
        return NULL;
    }
    
    char status_message[MAX_RESPONSE_LEN];
    snprintf(status_message, sizeof(status_message), 
             "Using active LLM service: Ollama. Sending query to %s...", server_address);
    
    // Start concurrent progress animation
    start_progress_animation(status_message, config->show_progress_animation);
    
    // Create JSON payload
    json_object *payload = create_ollama_payload(model_name, user_query, history, history_count);
    if (!payload) {
        display_message("Failed to create request payload.", COLOR_RED);
        return NULL;
    }
    
    const char *json_string = json_object_to_json_string(payload);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s%s", server_address, OLLAMA_API_CHAT);
    
    char *response = NULL;
    struct curl_slist *headers = create_headers();
    
    bool success = make_http_request(url, json_string, headers, &response);
    free_headers(headers);
    json_object_put(payload);
    
    if (!success || !response) {
        stop_progress_animation();
        display_message("Failed to send query to Ollama server.", COLOR_RED);
        return NULL;
    }
    
    // Parse response
    json_object *response_obj = json_tokener_parse(response);
    free(response);
    
    if (!response_obj) {
        stop_progress_animation();
        display_message("Failed to parse response from Ollama server.", COLOR_RED);
        return NULL;
    }
    
    char *response_text = extract_response_from_json(response_obj, LLM_SERVICE_OLLAMA);
    json_object_put(response_obj);
    
    if (!response_text) {
        stop_progress_animation();
        display_message("Failed to extract response from Ollama server.", COLOR_RED);
        return NULL;
    }
    // Stop animation so the caller can print response immediately
    stop_progress_animation();
    
    // The caller is responsible for printing and freeing response_text
    return response_text;
}

json_object* create_ollama_payload(const char *model, const char *query, 
                                  conversation_message_t *history, int history_count) {
    json_object *payload = json_object_new_object();
    if (!payload) {
        return NULL;
    }
    
    // Add model
    json_object_object_add(payload, "model", json_object_new_string(model));
    
    // Add messages array
    json_object *messages = json_object_new_array();
    if (!messages) {
        json_object_put(payload);
        return NULL;
    }
    
    // Add conversation history
    for (int i = 0; i < history_count; i++) {
        json_object *message = json_object_new_object();
        if (!message) {
            continue;
        }
        
        const char *role = (strcmp(history[i].role, "model") == 0) ? "assistant" : "user";
        json_object_object_add(message, "role", json_object_new_string(role));
        json_object_object_add(message, "content", json_object_new_string(history[i].content));
        json_object_array_add(messages, message);
    }
    
    // Add current query
    json_object *current_message = json_object_new_object();
    if (current_message) {
        json_object_object_add(current_message, "role", json_object_new_string("user"));
        json_object_object_add(current_message, "content", json_object_new_string(query));
        json_object_array_add(messages, current_message);
    }
    
    json_object_object_add(payload, "messages", messages);
    
    // Add streaming option
    json_object_object_add(payload, "stream", json_object_new_boolean(false));
    
    return payload;
} 