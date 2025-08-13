#include "deepshell.h"

char* get_config_path(void) {
    char *home = get_home_directory();
    if (!home) {
        return NULL;
    }
    
    char *config_path = malloc(strlen(home) + strlen(CONFIG_DIR_NAME) + 2);
    if (!config_path) {
        free(home);
        return NULL;
    }
    
    sprintf(config_path, "%s/%s", home, CONFIG_DIR_NAME);
    free(home);
    return config_path;
}

char* get_config_file_path(void) {
    char *config_dir = get_config_path();
    if (!config_dir) {
        return NULL;
    }
    
    char *config_file = malloc(strlen(config_dir) + strlen(CONFIG_FILE_NAME) + 2);
    if (!config_file) {
        free(config_dir);
        return NULL;
    }
    
    sprintf(config_file, "%s/%s", config_dir, CONFIG_FILE_NAME);
    free(config_dir);
    return config_file;
}

config_t* get_default_config(void) {
    config_t *config = malloc(sizeof(config_t));
    if (!config) {
        return NULL;
    }
    
    memset(config, 0, sizeof(config_t));
    config->interactive_history_limit = 25;
    config->enable_streaming = false;
    config->show_progress_animation = true;
    config->gemini.render_markdown = true;
    config->ollama.render_markdown = true;
    
    return config;
}

config_t* load_config(void) {
    char *config_file_path = get_config_file_path();
    if (!config_file_path) {
        return NULL;
    }
    
    FILE *file = fopen(config_file_path, "r");
    if (!file) {
        free(config_file_path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        free(config_file_path);
        return NULL;
    }
    
    // Read file content
    char *json_content = malloc(file_size + 1);
    if (!json_content) {
        fclose(file);
        free(config_file_path);
        return NULL;
    }
    
    size_t bytes_read = fread(json_content, 1, file_size, file);
    json_content[bytes_read] = '\0';
    fclose(file);
    
    // Parse JSON
    json_object *json_obj = json_tokener_parse(json_content);
    free(json_content);
    
    if (!json_obj) {
        display_message("Error: Invalid JSON format in config file. Please delete it and run DeepShell again to reconfigure.", COLOR_RED);
        free(config_file_path);
        return NULL;
    }
    
    // Create config structure
    config_t *config = get_default_config();
    if (!config) {
        json_object_put(json_obj);
        free(config_file_path);
        return NULL;
    }
    
    // Parse JSON into config structure
    json_object *active_service, *previous_service, *llm_services, *ollama_config, *gemini_config;
    json_object *interactive_limit, *enable_streaming;
    
    if (json_object_object_get_ex(json_obj, "active_llm_service", &active_service)) {
        strncpy(config->active_llm_service, json_object_get_string(active_service), 31);
        config->active_llm_service[31] = '\0';
    }
    
    if (json_object_object_get_ex(json_obj, "previous_active_llm_service", &previous_service)) {
        strncpy(config->previous_active_llm_service, json_object_get_string(previous_service), 31);
        config->previous_active_llm_service[31] = '\0';
    }
    
    if (json_object_object_get_ex(json_obj, "interactive_history_limit", &interactive_limit)) {
        config->interactive_history_limit = json_object_get_int(interactive_limit);
    }
    
    if (json_object_object_get_ex(json_obj, "enable_streaming", &enable_streaming)) {
        config->enable_streaming = json_object_get_boolean(enable_streaming);
    }
    
    json_object *show_progress_animation;
    if (json_object_object_get_ex(json_obj, "show_progress_animation", &show_progress_animation)) {
        config->show_progress_animation = json_object_get_boolean(show_progress_animation);
    }
    
    if (json_object_object_get_ex(json_obj, "llm_services", &llm_services)) {
        // Parse Ollama config
        if (json_object_object_get_ex(llm_services, LLM_SERVICE_OLLAMA, &ollama_config)) {
            json_object *server_addr, *model, *render_md;
            
            if (json_object_object_get_ex(ollama_config, "server_address", &server_addr)) {
                strncpy(config->ollama.server_address, json_object_get_string(server_addr), MAX_SERVER_ADDR_LEN - 1);
                config->ollama.server_address[MAX_SERVER_ADDR_LEN - 1] = '\0';
            }
            
            if (json_object_object_get_ex(ollama_config, "model", &model)) {
                strncpy(config->ollama.model, json_object_get_string(model), MAX_MODEL_NAME_LEN - 1);
                config->ollama.model[MAX_MODEL_NAME_LEN - 1] = '\0';
            }
            
            if (json_object_object_get_ex(ollama_config, "render_markdown", &render_md)) {
                config->ollama.render_markdown = json_object_get_boolean(render_md);
            }
        }
        
        // Parse Gemini config
        if (json_object_object_get_ex(llm_services, LLM_SERVICE_GEMINI, &gemini_config)) {
            json_object *api_keys, *active_key, *model, *render_md;
            
            if (json_object_object_get_ex(gemini_config, "api_keys", &api_keys)) {
                int key_count = json_object_array_length(api_keys);
                config->gemini.api_key_count = key_count > MAX_SERVICES ? MAX_SERVICES : key_count;
                
                for (int i = 0; i < config->gemini.api_key_count; i++) {
                    json_object *key_obj = json_object_array_get_idx(api_keys, i);
                    json_object *nickname, *key;
                    
                    if (json_object_object_get_ex(key_obj, "nickname", &nickname)) {
                        strncpy(config->gemini.api_keys[i].nickname, json_object_get_string(nickname), MAX_NICKNAME_LEN - 1);
                        config->gemini.api_keys[i].nickname[MAX_NICKNAME_LEN - 1] = '\0';
                    }
                    
                    if (json_object_object_get_ex(key_obj, "key", &key)) {
                        strncpy(config->gemini.api_keys[i].key, json_object_get_string(key), MAX_API_KEY_LEN - 1);
                        config->gemini.api_keys[i].key[MAX_API_KEY_LEN - 1] = '\0';
                    }
                }
            }
            
            if (json_object_object_get_ex(gemini_config, "active_api_key_nickname", &active_key)) {
                strncpy(config->gemini.active_api_key_nickname, json_object_get_string(active_key), MAX_NICKNAME_LEN - 1);
                config->gemini.active_api_key_nickname[MAX_NICKNAME_LEN - 1] = '\0';
            }
            
            if (json_object_object_get_ex(gemini_config, "model", &model)) {
                strncpy(config->gemini.model, json_object_get_string(model), MAX_MODEL_NAME_LEN - 1);
                config->gemini.model[MAX_MODEL_NAME_LEN - 1] = '\0';
            }
            
            if (json_object_object_get_ex(gemini_config, "render_markdown", &render_md)) {
                config->gemini.render_markdown = json_object_get_boolean(render_md);
            }
        }
    }
    
    json_object_put(json_obj);
    free(config_file_path);
    return config;
}

bool save_config(config_t *config) {
    if (!config) {
        return false;
    }
    
    char *config_dir = get_config_path();
    char *config_file_path = get_config_file_path();
    
    if (!config_dir || !config_file_path) {
        free(config_dir);
        free(config_file_path);
        return false;
    }
    
    // Create config directory if it doesn't exist
    if (!create_directory_if_not_exists(config_dir)) {
        free(config_dir);
        free(config_file_path);
        return false;
    }
    
    // Create JSON object
    json_object *json_obj = json_object_new_object();
    
    // Add basic fields
    json_object_object_add(json_obj, "active_llm_service", 
                         json_object_new_string(config->active_llm_service));
    json_object_object_add(json_obj, "previous_active_llm_service", 
                         json_object_new_string(config->previous_active_llm_service));
    json_object_object_add(json_obj, "interactive_history_limit", 
                         json_object_new_int(config->interactive_history_limit));
    json_object_object_add(json_obj, "enable_streaming", 
                         json_object_new_boolean(config->enable_streaming));
    json_object_object_add(json_obj, "show_progress_animation", 
                         json_object_new_boolean(config->show_progress_animation));
    
    // Create llm_services object
    json_object *llm_services = json_object_new_object();
    
    // Add Ollama config
    json_object *ollama_config = json_object_new_object();
    json_object_object_add(ollama_config, "server_address", 
                          json_object_new_string(config->ollama.server_address));
    json_object_object_add(ollama_config, "model", 
                          json_object_new_string(config->ollama.model));
    json_object_object_add(ollama_config, "render_markdown", 
                          json_object_new_boolean(config->ollama.render_markdown));
    json_object_object_add(llm_services, LLM_SERVICE_OLLAMA, ollama_config);
    
    // Add Gemini config
    json_object *gemini_config = json_object_new_object();
    json_object_object_add(gemini_config, "active_api_key_nickname", 
                          json_object_new_string(config->gemini.active_api_key_nickname));
    json_object_object_add(gemini_config, "model", 
                          json_object_new_string(config->gemini.model));
    json_object_object_add(gemini_config, "render_markdown", 
                          json_object_new_boolean(config->gemini.render_markdown));
    
    // Add API keys array
    json_object *api_keys = json_object_new_array();
    for (int i = 0; i < config->gemini.api_key_count; i++) {
        json_object *key_obj = json_object_new_object();
        json_object_object_add(key_obj, "nickname", 
                             json_object_new_string(config->gemini.api_keys[i].nickname));
        json_object_object_add(key_obj, "key", 
                             json_object_new_string(config->gemini.api_keys[i].key));
        json_object_array_add(api_keys, key_obj);
    }
    json_object_object_add(gemini_config, "api_keys", api_keys);
    json_object_object_add(llm_services, LLM_SERVICE_GEMINI, gemini_config);
    
    json_object_object_add(json_obj, "llm_services", llm_services);
    
    // Write to file
    FILE *file = fopen(config_file_path, "w");
    if (!file) {
        json_object_put(json_obj);
        free(config_dir);
        free(config_file_path);
        return false;
    }
    
    const char *json_string = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE);
    fprintf(file, "%s\n", json_string);
    fclose(file);
    
    display_message("Configuration saved successfully.", COLOR_GREEN);
    
    json_object_put(json_obj);
    free(config_dir);
    free(config_file_path);
    return true;
}

void free_config(config_t *config) {
    if (config) {
        free(config);
    }
}

bool delete_config_file(void) {
    char *config_file_path = get_config_file_path();
    if (!config_file_path) {
        return false;
    }
    
    display_message("Are you sure you want to delete the configuration file? (y/N): ", COLOR_YELLOW);
    char *response = read_line();
    if (!response) {
        free(config_file_path);
        return false;
    }
    
    bool confirmed = (strlen(response) > 0 && 
                     (response[0] == 'y' || response[0] == 'Y'));
    free(response);
    
    if (!confirmed) {
        display_message("Configuration deletion cancelled.", COLOR_YELLOW);
        free(config_file_path);
        return false;
    }
    
    int result = remove(config_file_path);
    free(config_file_path);
    
    return result == 0;
} 