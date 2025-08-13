#ifndef DEEPSHELL_H
#define DEEPSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <ctype.h>

// Version
#define DEEPSHELL_VERSION "1.1.2"

// Configuration constants
#define CONFIG_DIR_NAME ".deepshell"
#define CONFIG_FILE_NAME "deepshell.conf"
#define MAX_PATH_LEN 1024
#define MAX_QUERY_LEN 4096
#define MAX_RESPONSE_LEN 8192
#define MAX_HISTORY_TURNS 50
#define MAX_API_KEY_LEN 256
#define MAX_NICKNAME_LEN 64
#define MAX_MODEL_NAME_LEN 128
#define MAX_SERVER_ADDR_LEN 256

// LLM Service constants
#define LLM_SERVICE_OLLAMA "ollama"
#define LLM_SERVICE_GEMINI "gemini"
#define MAX_SERVICES 10

// Color codes for terminal output
#define COLOR_RED     "\033[91m"
#define COLOR_GREEN   "\033[92m"
#define COLOR_YELLOW  "\033[93m"
#define COLOR_BLUE    "\033[94m"
#define COLOR_CYAN    "\033[96m"
#define COLOR_ORANGE  "\033[33m"
#define COLOR_RESET   "\033[0m"

// API endpoints
#define GEMINI_API_BASE_URL "https://generativelanguage.googleapis.com/v1beta"
#define OLLAMA_API_CHAT "/api/chat"
#define OLLAMA_API_TAGS "/api/tags"

// Structures
typedef struct {
    char nickname[MAX_NICKNAME_LEN];
    char key[MAX_API_KEY_LEN];
} api_key_t;

typedef struct {
    char server_address[MAX_SERVER_ADDR_LEN];
    char model[MAX_MODEL_NAME_LEN];
    bool render_markdown;
} ollama_config_t;

typedef struct {
    api_key_t api_keys[MAX_SERVICES];
    int api_key_count;
    char active_api_key_nickname[MAX_NICKNAME_LEN];
    char model[MAX_MODEL_NAME_LEN];
    bool render_markdown;
} gemini_config_t;

typedef struct {
    char role[16];  // "user" or "model"
    char content[MAX_RESPONSE_LEN];
} conversation_message_t;

typedef struct {
    char active_llm_service[32];
    char previous_active_llm_service[32];
    ollama_config_t ollama;
    gemini_config_t gemini;
    int interactive_history_limit;
    bool enable_streaming;
    bool show_progress_animation;
} config_t;

// Function prototypes

// Main functions
int main(int argc, char *argv[]);
void print_help(void);
void print_version(void);

// Configuration functions
char* get_config_path(void);
char* get_config_file_path(void);
config_t* load_config(void);
bool save_config(config_t *config);
config_t* get_default_config(void);
void free_config(config_t *config);

// Settings functions
bool setup_config(bool is_direct_flag_call);
bool configure_ollama_service(config_t *config);
bool configure_gemini_service(config_t *config);
bool manage_gemini_api_keys(config_t *config);
bool change_active_model(config_t *config);
bool switch_llm_service(config_t *config);
bool toggle_markdown_rendering(config_t *config);
bool toggle_streaming(config_t *config);
bool toggle_progress_animation(config_t *config);
bool set_history_limit(config_t *config);
bool delete_config_file(void);
void show_active_configuration(config_t *config);
bool jump_to_previous_llm(config_t *config);

// Ollama functions
bool fetch_ollama_models(const char *server_address, char ***models, int *model_count);
char* send_ollama_query(const char *server_address, const char *model_name, 
                      const char *user_query, conversation_message_t *history, 
                      int history_count, config_t *config);
json_object* create_ollama_payload(const char *model, const char *query, 
                                  conversation_message_t *history, int history_count);

// Gemini functions
bool fetch_gemini_models(const char *api_key, char ***models, int *model_count);
char* send_gemini_query(const char *api_key, const char *model_name,
                      const char *user_query, conversation_message_t *history,
                      int history_count, config_t *config);
char* get_active_gemini_key_value(const gemini_config_t *gemini_config, char *nickname);
bool add_gemini_api_key(gemini_config_t *gemini_config, const char *nickname, const char *key);
bool remove_gemini_api_key(gemini_config_t *gemini_config, const char *nickname);
bool set_active_gemini_api_key(gemini_config_t *gemini_config, const char *nickname);
json_object* create_gemini_payload(const char *model, const char *query,
                                  conversation_message_t *history, int history_count);

// Utility functions
void display_message(const char *message, const char *color);
void animate_progress(const char *status_text);
void animate_progress_conditional(const char *status_text, bool show_animation);

// Non-blocking progress animation controls
void start_progress_animation(const char *status_text, bool enable_animation);
void stop_progress_animation(void);
void print_colored(const char *text, const char *color);
void print_markdown(const char *text);
char* get_home_directory(void);
bool create_directory_if_not_exists(const char *path);
char* read_line(void);
void clear_screen(void);
bool is_valid_url(const char *url);
char* trim_whitespace(char *str);
char* strdup_safe(const char *str);
char* extract_response_from_json(json_object *json_obj, const char *service);

// HTTP functions
struct curl_slist* create_headers(void);
void free_headers(struct curl_slist *headers);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
bool make_http_request(const char *url, const char *post_data, 
                      struct curl_slist *headers, char **response);

// Interactive mode
bool start_interactive_session(config_t *config);

// Command line parsing
typedef struct {
    bool setup;
    bool interactive;
    bool query;
    bool delete_config;
    bool show_gemini_key;
    bool set_gemini_key;
    bool gemini_quota_check;
    bool switch_llm;
    bool show_config;
    bool jump_llm;
    bool model_change;
    bool version;
    bool help;
    bool no_animation;
    char *query_text;
} cli_args_t;

cli_args_t parse_arguments(int argc, char *argv[]);
void free_cli_args(cli_args_t *args);

#endif // DEEPSHELL_H 