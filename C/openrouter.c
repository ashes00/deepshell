#include "deepshell.h"

bool fetch_openrouter_models(const char *api_key, char ***models, int *model_count) {
    if (!api_key || !models || !model_count) {
        return false;
    }
    
    display_message("Fetching models from OpenRouter API...", COLOR_BLUE);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s%s", OPENROUTER_API_BASE_URL, OPENROUTER_API_MODELS);
    
    char *response = NULL;
    struct curl_slist *headers = create_headers();
    
    // Add authorization header
    char auth_header[MAX_API_KEY_LEN + 20];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    
    bool success = make_http_request(url, NULL, headers, &response);
    free_headers(headers);
    
    if (!success || !response) {
        display_message("Failed to fetch models from OpenRouter API.", COLOR_RED);
        return false;
    }
    
    // Parse JSON response
    json_object *json_obj = json_tokener_parse(response);
    free(response);
    
    if (!json_obj) {
        display_message("Failed to parse JSON response from OpenRouter API.", COLOR_RED);
        return false;
    }
    
    json_object *data_array;
    if (!json_object_object_get_ex(json_obj, "data", &data_array)) {
        display_message("Invalid response format from OpenRouter API.", COLOR_RED);
        json_object_put(json_obj);
        return false;
    }
    
    int array_length = json_object_array_length(data_array);
    *model_count = 0;
    *models = malloc(array_length * sizeof(char*));
    
    if (!*models) {
        json_object_put(json_obj);
        return false;
    }
    
    for (int i = 0; i < array_length; i++) {
        json_object *model_obj = json_object_array_get_idx(data_array, i);
        json_object *id_obj;
        
        if (json_object_object_get_ex(model_obj, "id", &id_obj)) {
            const char *model_id = json_object_get_string(id_obj);
            if (model_id && strlen(model_id) > 0) {
                (*models)[*model_count] = strdup_safe(model_id);
                if ((*models)[*model_count]) {
                    (*model_count)++;
                }
            }
        }
    }
    
    json_object_put(json_obj);
    
    if (*model_count > 0) {
        display_message("Models fetched successfully from OpenRouter.", COLOR_GREEN);
        return true;
    } else {
        display_message("No valid models found in OpenRouter response.", COLOR_YELLOW);
        free(*models);
        *models = NULL;
        return false;
    }
}

char* send_openrouter_query(const char *api_key, const char *model_name,
                           const char *user_query, conversation_message_t *history,
                           int history_count, config_t *config) {
    if (!api_key || !model_name || !user_query || !config) {
        return NULL;
    }
    
    // Start progress animation
    start_progress_animation("Querying OpenRouter API", config->show_progress_animation);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s%s", OPENROUTER_API_BASE_URL, OPENROUTER_API_CHAT);
    
    // Create the JSON payload
    json_object *payload = create_openrouter_payload(model_name, user_query, history, history_count);
    if (!payload) {
        stop_progress_animation();
        display_message("Failed to create OpenRouter request payload.", COLOR_RED);
        return NULL;
    }
    
    const char *json_string = json_object_to_json_string_ext(payload, JSON_C_TO_STRING_NOSLASHESCAPE);
    
    // Prepare headers
    struct curl_slist *headers = create_headers();
    
    // Add authorization header
    char auth_header[MAX_API_KEY_LEN + 20];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    
    // Add OpenRouter-specific headers for app attribution (optional but recommended)
    if (strlen(config->openrouter.site_url) > 0) {
        char referer_header[MAX_SERVER_ADDR_LEN + 20];
        snprintf(referer_header, sizeof(referer_header), "HTTP-Referer: %s", config->openrouter.site_url);
        headers = curl_slist_append(headers, referer_header);
    }
    
    if (strlen(config->openrouter.site_name) > 0) {
        char title_header[MAX_NICKNAME_LEN + 20];
        snprintf(title_header, sizeof(title_header), "X-Title: %s", config->openrouter.site_name);
        headers = curl_slist_append(headers, title_header);
    }
    
    char *response = NULL;
    bool success = make_http_request(url, json_string, headers, &response);
    
    // Cleanup
    json_object_put(payload);
    free_headers(headers);
    stop_progress_animation();
    
    if (!success || !response) {
        display_message("Failed to get response from OpenRouter API.", COLOR_RED);
        return NULL;
    }
    
    // Extract response from JSON
    char *extracted_response = extract_response_from_json(json_tokener_parse(response), LLM_SERVICE_OPENROUTER);
    free(response);
    
    if (!extracted_response) {
        display_message("Failed to extract response from OpenRouter API response.", COLOR_RED);
        return NULL;
    }
    
    return extracted_response;
}

json_object* create_openrouter_payload(const char *model, const char *query,
                                      conversation_message_t *history, int history_count) {
    if (!model || !query) {
        return NULL;
    }
    
    json_object *payload = json_object_new_object();
    json_object *model_obj = json_object_new_string(model);
    json_object *messages_array = json_object_new_array();
    
    // Add conversation history if provided
    if (history && history_count > 0) {
        for (int i = 0; i < history_count; i++) {
            json_object *message = json_object_new_object();
            
            // Convert role from internal format to OpenAI format
            const char *openai_role = strcmp(history[i].role, "model") == 0 ? "assistant" : history[i].role;
            json_object *role_obj = json_object_new_string(openai_role);
            json_object *content_obj = json_object_new_string(history[i].content);
            
            json_object_object_add(message, "role", role_obj);
            json_object_object_add(message, "content", content_obj);
            json_object_array_add(messages_array, message);
        }
    }
    
    // Add current user query
    json_object *user_message = json_object_new_object();
    json_object *user_role = json_object_new_string("user");
    json_object *user_content = json_object_new_string(query);
    
    json_object_object_add(user_message, "role", user_role);
    json_object_object_add(user_message, "content", user_content);
    json_object_array_add(messages_array, user_message);
    
    // Build the payload
    json_object_object_add(payload, "model", model_obj);
    json_object_object_add(payload, "messages", messages_array);
    
    // Add additional parameters for better responses
    json_object_object_add(payload, "max_tokens", json_object_new_int(4096));
    json_object_object_add(payload, "temperature", json_object_new_double(0.7));
    
    return payload;
}

// Comparison function for sorting models (free first, then alphabetical)
static int compare_models(const void *a, const void *b) {
    const openrouter_model_t *model_a = (const openrouter_model_t *)a;
    const openrouter_model_t *model_b = (const openrouter_model_t *)b;
    
    // Free models come first
    if (model_a->is_free && !model_b->is_free) return -1;
    if (!model_a->is_free && model_b->is_free) return 1;
    
    // Then sort alphabetically by ID
    return strcmp(model_a->id, model_b->id);
}

bool fetch_openrouter_models_detailed(const char *api_key, openrouter_model_t **models, int *model_count) {
    if (!api_key || !models || !model_count) {
        return false;
    }
    
    display_message("Fetching detailed models from OpenRouter API...", COLOR_BLUE);
    
    char url[MAX_PATH_LEN];
    snprintf(url, sizeof(url), "%s%s", OPENROUTER_API_BASE_URL, OPENROUTER_API_MODELS);
    
    char *response = NULL;
    struct curl_slist *headers = create_headers();
    
    // Add authorization header
    char auth_header[MAX_API_KEY_LEN + 20];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    
    bool success = make_http_request(url, NULL, headers, &response);
    free_headers(headers);
    
    if (!success || !response) {
        display_message("Failed to fetch models from OpenRouter API.", COLOR_RED);
        return false;
    }
    
    // Parse JSON response
    json_object *json_obj = json_tokener_parse(response);
    free(response);
    
    if (!json_obj) {
        display_message("Failed to parse JSON response from OpenRouter API.", COLOR_RED);
        return false;
    }
    
    json_object *data_array;
    if (!json_object_object_get_ex(json_obj, "data", &data_array)) {
        display_message("Invalid response format from OpenRouter API.", COLOR_RED);
        json_object_put(json_obj);
        return false;
    }
    
    int array_length = json_object_array_length(data_array);
    *model_count = 0;
    *models = malloc(array_length * sizeof(openrouter_model_t));
    
    if (!*models) {
        json_object_put(json_obj);
        return false;
    }
    
    for (int i = 0; i < array_length; i++) {
        json_object *model_obj = json_object_array_get_idx(data_array, i);
        json_object *id_obj, *name_obj, *pricing_obj;
        
        if (json_object_object_get_ex(model_obj, "id", &id_obj)) {
            const char *model_id = json_object_get_string(id_obj);
            if (model_id && strlen(model_id) > 0) {
                // Copy model ID
                strncpy((*models)[*model_count].id, model_id, MAX_MODEL_NAME_LEN - 1);
                (*models)[*model_count].id[MAX_MODEL_NAME_LEN - 1] = '\0';
                
                // Get model name (fallback to ID if no name)
                if (json_object_object_get_ex(model_obj, "name", &name_obj)) {
                    const char *model_name = json_object_get_string(name_obj);
                    strncpy((*models)[*model_count].name, model_name, MAX_MODEL_NAME_LEN - 1);
                } else {
                    strncpy((*models)[*model_count].name, model_id, MAX_MODEL_NAME_LEN - 1);
                }
                (*models)[*model_count].name[MAX_MODEL_NAME_LEN - 1] = '\0';
                
                // Check pricing to determine if free
                (*models)[*model_count].is_free = false;
                (*models)[*model_count].price_per_token = 0.0;
                
                if (json_object_object_get_ex(model_obj, "pricing", &pricing_obj)) {
                    json_object *prompt_obj, *completion_obj;
                    if (json_object_object_get_ex(pricing_obj, "prompt", &prompt_obj) &&
                        json_object_object_get_ex(pricing_obj, "completion", &completion_obj)) {
                        
                        const char *prompt_price = json_object_get_string(prompt_obj);
                        const char *completion_price = json_object_get_string(completion_obj);
                        
                        // Model is free if both prompt and completion prices are "0"
                        if (prompt_price && completion_price &&
                            strcmp(prompt_price, "0") == 0 && strcmp(completion_price, "0") == 0) {
                            (*models)[*model_count].is_free = true;
                        } else if (prompt_price) {
                            (*models)[*model_count].price_per_token = atof(prompt_price);
                        }
                    }
                }
                
                (*model_count)++;
            }
        }
    }
    
    json_object_put(json_obj);
    
    if (*model_count > 0) {
        // Sort models: free first, then alphabetically
        qsort(*models, *model_count, sizeof(openrouter_model_t), compare_models);
        
        display_message("Models fetched and sorted successfully from OpenRouter.", COLOR_GREEN);
        return true;
    } else {
        display_message("No valid models found in OpenRouter response.", COLOR_YELLOW);
        free(*models);
        *models = NULL;
        return false;
    }
}
