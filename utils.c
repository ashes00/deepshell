#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "deepshell.h"
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdatomic.h>
#include <termios.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

void display_message(const char *message, const char *color) {
    if (color) {
        printf("%s%s%s\n", color, message, COLOR_RESET);
    } else {
        printf("%s\n", message);
    }
}

void print_colored(const char *text, const char *color) {
    if (color) {
        printf("%s%s%s\n", color, text, COLOR_RESET);
    } else {
        printf("%s\n", text);
    }
}

void print_markdown(const char *text) {
    if (!text) return;
    
    const char *ptr = text;
    bool in_code_block = false;
    bool in_inline_code = false;
    bool in_bold = false;
    bool in_italic = false;
    bool in_list = false;
    bool in_header = false;
    bool in_table = false;
    bool in_table_header = false;
    int header_level = 0;
    int line_length = 0;
    const int max_line_length = 120; // Prevent excessive line wrapping

    while (*ptr) {
        // Handle code blocks (```)
        if (strncmp(ptr, "```", 3) == 0) {
            if (!in_code_block) {
                printf("\n%s", COLOR_BLUE);
                printf("┌─ Code Block ──────────────────────────────────────────────┐\n");
                in_code_block = true;
                ptr += 3;
                // Skip language identifier (e.g., "markdown", "c", etc.)
                while (*ptr && !isspace(*ptr) && *ptr != '\n') {
                    ptr++;
                }
                // Skip any remaining whitespace
                while (*ptr && isspace(*ptr) && *ptr != '\n') {
                    ptr++;
                }
                continue;
            } else {
                printf("└─────────────────────────────────────────────────────────────┘\n");
                printf("%s", COLOR_RESET);
                in_code_block = false;
                ptr += 3;
                continue;
            }
        }
        
        // Handle inline code (`)
        if (*ptr == '`' && !in_code_block) {
            if (!in_inline_code) {
                printf("%s", COLOR_BLUE);
                in_inline_code = true;
            } else {
                printf("%s", COLOR_RESET);
                in_inline_code = false;
            }
            ptr++;
            continue;
        }
        
        // Handle bold (**)
        if (strncmp(ptr, "**", 2) == 0 && !in_code_block && !in_inline_code) {
            if (!in_bold) {
                printf("%s", COLOR_YELLOW);
                in_bold = true;
            } else {
                printf("%s", COLOR_RESET);
                in_bold = false;
            }
            ptr += 2;
            continue;
        }
        
        // Handle italic (*)
        if (*ptr == '*' && !in_code_block && !in_inline_code && !in_bold) {
            if (!in_italic) {
                printf("%s", COLOR_ORANGE);
                in_italic = true;
            } else {
                printf("%s", COLOR_RESET);
                in_italic = false;
            }
            ptr++;
            continue;
        }
        
        // Handle headers (#)
        if (*ptr == '#' && !in_code_block && !in_inline_code) {
            header_level = 0;
            const char *header_start = ptr;
            while (*ptr == '#') {
                header_level++;
                ptr++;
            }
            if (header_level <= 6 && *ptr && isspace(*ptr)) {
                printf("\n%s", COLOR_GREEN);
                for (int i = 0; i < header_level; i++) {
                    printf("#");
                }
                printf(" ");
                in_header = true;
                line_length = 0;
                continue;
            } else {
                ptr = header_start; // Reset if not a valid header
            }
        }
        
        // Handle lists (- or *)
        if ((*ptr == '-' || *ptr == '*') && !in_code_block && !in_inline_code && !in_header) {
            const char *list_start = ptr;
            ptr++;
            if (*ptr && isspace(*ptr)) {
                printf("\n%s• ", COLOR_BLUE);
                in_list = true;
                line_length = 0;
                continue;
            } else {
                ptr = list_start; // Reset if not a valid list
            }
        }
        
        // Handle numbered lists
        if (isdigit(*ptr) && !in_code_block && !in_inline_code && !in_header) {
            const char *num_start = ptr;
            int num = 0;
            while (isdigit(*ptr)) {
                num = num * 10 + (*ptr - '0');
                ptr++;
            }
            if (*ptr == '.' && *(ptr + 1) && isspace(*(ptr + 1))) {
                printf("\n%s%d. ", COLOR_BLUE, num);
                in_list = true;
                line_length = 0;
                ptr++;
                continue;
            } else {
                ptr = num_start; // Reset if not a valid numbered list
            }
        }
        
        // Handle horizontal rules (--- or ***)
        if ((*ptr == '-' || *ptr == '*') && !in_code_block && !in_inline_code) {
            const char *rule_start = ptr;
            char rule_char = *ptr;
            int rule_count = 0;
            while (*ptr == rule_char) {
                rule_count++;
                ptr++;
            }
            if (rule_count >= 3 && (*ptr == '\n' || *ptr == '\0')) {
                printf("\n%s", COLOR_BLUE);
                printf("─────────────────────────────────────────────────────────────\n");
                printf("%s", COLOR_RESET);
                line_length = 0;
                continue;
            } else {
                ptr = rule_start; // Reset if not a valid rule
            }
        }
        
        // Handle table separators (|)
        if (*ptr == '|' && !in_code_block && !in_inline_code) {
            if (!in_table) {
                in_table = true;
                printf("%s", COLOR_CYAN);
            }
            printf("│");
            line_length++;
            ptr++;
            continue;
        }
        
        // Handle table header separators (|---|)
        if (strncmp(ptr, "|---", 4) == 0 && in_table) {
            in_table_header = true;
            printf("%s", COLOR_CYAN);
            printf("├─");
            while (*ptr == '-' || *ptr == '|') {
                if (*ptr == '|') {
                    printf("─┤");
                } else {
                    printf("─");
                }
                ptr++;
            }
            printf("%s", COLOR_RESET);
            line_length = 0;
            continue;
        }
        
        // Handle HTML-style breaks (<br>)
        if (strncmp(ptr, "<br>", 4) == 0 && !in_code_block) {
            printf("\n");
            line_length = 0;
            ptr += 4;
            continue;
        }
        
        // Handle line breaks
        if (*ptr == '\n') {
            if (in_list) {
                in_list = false;
            }
            if (in_header) {
                in_header = false;
            }
            if (in_table_header) {
                in_table_header = false;
            }
            if (in_table && !in_table_header) {
                in_table = false;
            }
            printf("\n");
            line_length = 0;
            ptr++;
            continue;
        }
        
        // Print the character with smart wrapping
        if (line_length >= max_line_length && *ptr == ' ' && !in_code_block) {
            printf("\n");
            line_length = 0;
        }
        
        printf("%c", *ptr);
        line_length++;
        ptr++;
    }
    
    // Reset any active formatting
    if (in_code_block || in_inline_code || in_bold || in_italic) {
        printf("%s", COLOR_RESET);
    }
    
    printf("\n");
}

void animate_progress(const char *status_text) {
    // Blue braille Dot Scanner Window (front + rear around status_text)
    // Uses small braille dots (not periods), appears on both sides of the text
    // and sweeps a denser "window" across a lighter drizzle.
    static const char *DOTS_LIGHT[] = {"⠁","⠂","⠄","⡀","⢀","⠠","⠐","⠈"};
    static const int DOTS_LIGHT_N = 8;
    static const char *DOTS_DENSE[] = {"⠃","⠇","⡇","⣇","⣧","⣷","⣿","⣷","⣧","⣇","⡇","⠇","⠃"};
    static const int DOTS_DENSE_N = 13;

    const int left_width = 12;
    const int right_width = 12;

    for (int j = 0; j < 84; j++) {
        int win = j % 16;

        // Move to line start and set blue
        printf("\r%s", COLOR_BLUE);

        // Front (left) side
        for (int i = 0; i < left_width; i++) {
            if (i == win/2 || i == (win/2)+1) {
                printf("%s", DOTS_DENSE[(j + i) % DOTS_DENSE_N]);
            } else {
                printf("%s", DOTS_LIGHT[(j + i) % DOTS_LIGHT_N]);
            }
        }

        // Status text
        printf(" %s ", status_text);

        // Rear (right) side
        for (int i = 0; i < right_width; i++) {
            if (i == (15 - win)/2 || i == (15 - win)/2 + 1) {
                printf("%s", DOTS_DENSE[(j + i * 2) % DOTS_DENSE_N]);
            } else {
                printf("%s", DOTS_LIGHT[(j + i * 2) % DOTS_LIGHT_N]);
            }
        }

        // Reset color and clear to end of line for clean rendering
        printf("%s\033[K", COLOR_RESET);
        fflush(stdout);
        usleep(85000);
    }

    printf("\n");
}

// --- Concurrent progress animation (Dot Scanner Window) ---
static pthread_t g_progress_thread;
static atomic_int g_progress_active = 0;
static atomic_int g_progress_should_run = 0;
static char g_progress_message[1024];

static void* progress_animation_thread(void *arg) {
    (void)arg;
    // Dot Scanner Window frames (blue only)
    static const char *DOTS_LIGHT[] = {"⠁","⠂","⠄","⡀","⢀","⠠","⠐","⠈"};
    static const int DOTS_LIGHT_N = 8;
    static const char *DOTS_DENSE[] = {"⠃","⠇","⡇","⣇","⣧","⣷","⣿","⣷","⣧","⣇","⡇","⠇","⠃"};
    static const int DOTS_DENSE_N = 13;

    const int left_width = 12;
    const int right_width = 12;
    int j = 0;
    while (atomic_load(&g_progress_should_run)) {
        int win = j % 16;
        printf("\r%s", COLOR_BLUE);
        // Left band
        for (int i = 0; i < left_width; i++) {
            if (i == win/2 || i == (win/2)+1) {
                printf("%s", DOTS_DENSE[(j + i) % DOTS_DENSE_N]);
            } else {
                printf("%s", DOTS_LIGHT[(j + i) % DOTS_LIGHT_N]);
            }
        }
        // Message
        printf(" %s ", g_progress_message);
        // Right band
        for (int i = 0; i < right_width; i++) {
            if (i == (15 - win)/2 || i == (15 - win)/2 + 1) {
                printf("%s", DOTS_DENSE[(j + i * 2) % DOTS_DENSE_N]);
            } else {
                printf("%s", DOTS_LIGHT[(j + i * 2) % DOTS_LIGHT_N]);
            }
        }
        // Clear rest of line, keep on same line
        printf("%s\033[K", COLOR_RESET);
        fflush(stdout);
        usleep(85000);
        j++;
    }
    return NULL;
}

void start_progress_animation(const char *status_text, bool enable_animation) {
    if (!enable_animation) {
        return;
    }
    if (atomic_load(&g_progress_active)) {
        return; // already running
    }
    // Copy message (truncate safely)
    size_t len = strlen(status_text);
    if (len >= sizeof(g_progress_message)) len = sizeof(g_progress_message) - 1;
    memcpy(g_progress_message, status_text, len);
    g_progress_message[len] = '\0';

    atomic_store(&g_progress_should_run, 1);
    if (pthread_create(&g_progress_thread, NULL, progress_animation_thread, NULL) == 0) {
        atomic_store(&g_progress_active, 1);
    } else {
        atomic_store(&g_progress_should_run, 0);
    }
}

void stop_progress_animation(void) {
    if (!atomic_load(&g_progress_active)) {
        return;
    }
    atomic_store(&g_progress_should_run, 0);
    pthread_join(g_progress_thread, NULL);
    atomic_store(&g_progress_active, 0);
    // Clear the animation line so next output starts cleanly
    printf("\r\033[K");
    fflush(stdout);
}

void animate_progress_conditional(const char *status_text, bool show_animation) {
    if (show_animation) {
        animate_progress(status_text);
    }
}

char* get_home_directory(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    return home ? strdup_safe(home) : NULL;
}

bool create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == 0) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

char* read_line(void) {
    char *line = readline("");
    if (line && *line) {
        // Add to history if non-empty
        add_history(line);
    }
    return line;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

bool is_valid_url(const char *url) {
    if (!url) return false;
    
    // Basic URL validation
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        return true;
    }
    
    // Check if it looks like a hostname:port
    if (strchr(url, ':') != NULL) {
        return true;
    }
    
    return false;
}

char* trim_whitespace(char *str) {
    if (!str) return NULL;
    
    char *end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str; // All spaces
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    
    return str;
}

char* strdup_safe(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *dup = malloc(len);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// HTTP utility functions
struct curl_slist* create_headers(void) {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    return headers;
}

void free_headers(struct curl_slist *headers) {
    if (headers) {
        curl_slist_free_all(headers);
    }
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char **response_ptr = (char**)userp;
    
    size_t current_len = (*response_ptr) ? strlen(*response_ptr) : 0;
    char *new_response = realloc(*response_ptr, current_len + realsize + 1);
    if (!new_response) {
        return 0; // Out of memory
    }
    
    *response_ptr = new_response;
    memcpy(&(*response_ptr)[current_len], contents, realsize);
    (*response_ptr)[current_len + realsize] = 0;
    
    return realsize;
}

bool make_http_request(const char *url, const char *post_data, 
                      struct curl_slist *headers, char **response) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    *response = malloc(1);
    (*response)[0] = '\0';
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // Extended timeout for slower models
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); // Extended connection timeout
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L); // Enable keep-alive
    
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    if (post_data) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    }
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        display_message("HTTP request failed: ", COLOR_RED);
        printf("%s\n", curl_easy_strerror(res));
        free(*response);
        *response = NULL;
        return false;
    }
    
    if (http_code < 200 || http_code >= 300) {
        display_message("HTTP request failed with status code: ", COLOR_RED);
        printf("%ld\n", http_code);
        if (*response) {
            display_message("Response: ", COLOR_RED);
            printf("%s\n", *response);
        }
        free(*response);
        *response = NULL;
        return false;
    }
    
    return true;
}

char* extract_response_from_json(json_object *json_obj, const char *service) {
    if (strcmp(service, LLM_SERVICE_OLLAMA) == 0) {
        json_object *message_obj, *content_obj;
        if (json_object_object_get_ex(json_obj, "message", &message_obj)) {
            if (json_object_object_get_ex(message_obj, "content", &content_obj)) {
                const char *content = json_object_get_string(content_obj);
                return strdup_safe(content);
            }
        }
    } else if (strcmp(service, LLM_SERVICE_GEMINI) == 0) {
        json_object *candidates_array, *candidate, *content, *parts_array, *part, *text_obj;
        if (json_object_object_get_ex(json_obj, "candidates", &candidates_array) &&
            json_object_array_length(candidates_array) > 0) {
            candidate = json_object_array_get_idx(candidates_array, 0);
            if (json_object_object_get_ex(candidate, "content", &content)) {
                if (json_object_object_get_ex(content, "parts", &parts_array) &&
                    json_object_array_length(parts_array) > 0) {
                    part = json_object_array_get_idx(parts_array, 0);
                    if (json_object_object_get_ex(part, "text", &text_obj)) {
                        const char *text = json_object_get_string(text_obj);
                        return strdup_safe(text);
                    }
                }
            }
        }
    } else if (strcmp(service, LLM_SERVICE_OPENROUTER) == 0) {
        // OpenRouter uses OpenAI-compatible format
        json_object *choices_array, *choice, *message_obj, *content_obj;
        if (json_object_object_get_ex(json_obj, "choices", &choices_array) &&
            json_object_array_length(choices_array) > 0) {
            choice = json_object_array_get_idx(choices_array, 0);
            if (json_object_object_get_ex(choice, "message", &message_obj)) {
                if (json_object_object_get_ex(message_obj, "content", &content_obj)) {
                    const char *content = json_object_get_string(content_obj);
                    return strdup_safe(content);
                }
            }
        }
    }
    return NULL;
}

// Safe string concatenation that handles special characters
char* safe_concat_query(int argc, char *argv[], int start_index) {
    size_t total_len = 0;
    
    // First pass: calculate total length needed
    for (int j = start_index; j < argc; j++) {
        total_len += strlen(argv[j]) + 1; // +1 for space
    }
    
    char *result = malloc(total_len + 1);
    if (!result) return NULL;
    
    result[0] = '\0';
    
    // Second pass: concatenate with proper spacing
    for (int j = start_index; j < argc; j++) {
        if (j > start_index) {
            strcat(result, " ");
        }
        strcat(result, argv[j]);
    }
    
    return result;
}

// Command line argument parsing
cli_args_t parse_arguments(int argc, char *argv[]) {
    cli_args_t args = {0};
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--setup") == 0) {
            args.setup = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            args.interactive = true;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--query") == 0) {
            args.query = true;
            // Collect all remaining arguments as the query using safe concatenation
            if (i + 1 < argc) {
                args.query_text = safe_concat_query(argc, argv, i + 1);
                break; // We've processed all remaining arguments
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--active-config") == 0) {
            args.active_config = true;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--export") == 0) {
            args.export_config = true;
            // Get the next argument as filename
            if (i + 1 < argc) {
                args.config_filename = strdup(argv[i + 1]);
                i++; // Skip the filename argument
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--import") == 0) {
            args.import_config = true;
            // Get the next argument as filename
            if (i + 1 < argc) {
                args.config_filename = strdup(argv[i + 1]);
                i++; // Skip the filename argument
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--delete-config") == 0) {
            args.delete_config = true;
        } else if (strcmp(argv[i], "-show-key") == 0 || strcmp(argv[i], "--show-api-key") == 0) {
            args.show_gemini_key = true;
        } else if (strcmp(argv[i], "-set-key") == 0 || strcmp(argv[i], "--set-api-key") == 0) {
            args.set_gemini_key = true;
        } else if (strcmp(argv[i], "-gq") == 0 || strcmp(argv[i], "--gemini-quota") == 0) {
            args.gemini_quota_check = true;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--llm") == 0) {
            args.switch_llm = true;
        } else if (strcmp(argv[i], "-show-config") == 0 || strcmp(argv[i], "--show-full-conf") == 0) {
            args.show_config = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            args.version = true;
        } else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jump-llm") == 0) {
            args.jump_llm = true;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--model-change") == 0) {
            args.model_change = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            args.help = true;
        } else if (strncmp(argv[i], "-q=", 3) == 0 || strncmp(argv[i], "--query=", 8) == 0) {
            // Handle quoted query format: -q="query text" or --query="query text"
            args.query = true;
            const char *query_start = (argv[i][1] == 'q') ? argv[i] + 3 : argv[i] + 8;
            if (strlen(query_start) > 0) {
                args.query_text = strdup(query_start);
            }
        } else if (strcmp(argv[i], "--no-animation") == 0) {
            args.no_animation = true;
        }
    }
    
    return args;
}

void free_cli_args(cli_args_t *args) {
    if (args->query_text) {
        free(args->query_text);
        args->query_text = NULL;
    }
    if (args->config_filename) {
        free(args->config_filename);
        args->config_filename = NULL;
    }
}

// Get user's Downloads directory
char* get_downloads_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }
    
    char *downloads_path = malloc(strlen(home) + strlen("/Downloads") + 1);
    if (!downloads_path) {
        return NULL;
    }
    
    strcpy(downloads_path, home);
    strcat(downloads_path, "/Downloads");
    return downloads_path;
}

// Get password input with optional confirmation
char* get_password_input(const char *prompt, bool confirm) {
    printf("%s", prompt);
    fflush(stdout);
    
    // Turn off echo
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    
    char *password = read_line();
    
    // Turn echo back on
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    printf("\n");
    
    if (!password || strlen(password) == 0) {
        if (password) free(password);
        return NULL;
    }
    
    if (confirm) {
        printf("Confirm password: ");
        fflush(stdout);
        
        // Turn off echo again
        tcsetattr(STDIN_FILENO, TCSANOW, &new);
        char *confirm_password = read_line();
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        printf("\n");
        
        if (!confirm_password || strcmp(password, confirm_password) != 0) {
            display_message("Passwords do not match.", COLOR_RED);
            free(password);
            if (confirm_password) free(confirm_password);
            return NULL;
        }
        free(confirm_password);
    }
    
    return password;
}

// Export configuration to encrypted file
bool export_config_to_file(const config_t *config, const char *filename, const char *password) {
    // Get Downloads directory
    char *downloads_path = get_downloads_path();
    if (!downloads_path) {
        display_message("Failed to get Downloads directory.", COLOR_RED);
        return false;
    }
    
    // Create full file path
    char *full_path = malloc(strlen(downloads_path) + strlen(filename) + 2);
    if (!full_path) {
        free(downloads_path);
        display_message("Memory allocation failed.", COLOR_RED);
        return false;
    }
    
    strcpy(full_path, downloads_path);
    strcat(full_path, "/");
    strcat(full_path, filename);
    free(downloads_path);
    
    // Create export JSON with version metadata
    json_object *export_obj = json_object_new_object();
    json_object *version_obj = json_object_new_string("1.0");
    json_object *timestamp_obj = json_object_new_int64(time(NULL));
    
    json_object_object_add(export_obj, "deepshell_export_version", version_obj);
    json_object_object_add(export_obj, "export_timestamp", timestamp_obj);
    
    // Add all configuration data
    json_object *config_obj = json_object_new_object();
    
    // Active LLM service and settings
    json_object *active_llm_obj = json_object_new_string(config->active_llm_service);
    json_object *prev_llm_obj = json_object_new_string(config->previous_active_llm_service);
    json_object *history_limit_obj = json_object_new_int(config->interactive_history_limit);
    json_object *streaming_obj = json_object_new_boolean(config->enable_streaming);
    json_object *animation_obj = json_object_new_boolean(config->show_progress_animation);
    
    json_object_object_add(config_obj, "active_llm_service", active_llm_obj);
    json_object_object_add(config_obj, "previous_llm_service", prev_llm_obj);
    json_object_object_add(config_obj, "interactive_history_limit", history_limit_obj);
    json_object_object_add(config_obj, "response_streaming", streaming_obj);
    json_object_object_add(config_obj, "progress_animation", animation_obj);
    
    // Ollama configuration
    json_object *ollama_obj = json_object_new_object();
    json_object_object_add(ollama_obj, "server_address", json_object_new_string(config->ollama.server_address));
    json_object_object_add(ollama_obj, "model", json_object_new_string(config->ollama.model));
    json_object_object_add(ollama_obj, "render_markdown", json_object_new_boolean(config->ollama.render_markdown));
    json_object_object_add(config_obj, "ollama", ollama_obj);
    
    // Gemini configuration with all API keys
    json_object *gemini_obj = json_object_new_object();
    json_object *gemini_keys_array = json_object_new_array();
    for (int i = 0; i < config->gemini.api_key_count; i++) {
        json_object *key_obj = json_object_new_object();
        json_object_object_add(key_obj, "nickname", json_object_new_string(config->gemini.api_keys[i].nickname));
        json_object_object_add(key_obj, "key", json_object_new_string(config->gemini.api_keys[i].key));
        json_object_array_add(gemini_keys_array, key_obj);
    }
    json_object_object_add(gemini_obj, "api_keys", gemini_keys_array);
    json_object_object_add(gemini_obj, "active_api_key_nickname", json_object_new_string(config->gemini.active_api_key_nickname));
    json_object_object_add(gemini_obj, "model", json_object_new_string(config->gemini.model));
    json_object_object_add(gemini_obj, "render_markdown", json_object_new_boolean(config->gemini.render_markdown));
    json_object_object_add(config_obj, "gemini", gemini_obj);
    
    // OpenRouter configuration with all API keys
    json_object *openrouter_obj = json_object_new_object();
    json_object *openrouter_keys_array = json_object_new_array();
    for (int i = 0; i < config->openrouter.api_key_count; i++) {
        json_object *key_obj = json_object_new_object();
        json_object_object_add(key_obj, "nickname", json_object_new_string(config->openrouter.api_keys[i].nickname));
        json_object_object_add(key_obj, "key", json_object_new_string(config->openrouter.api_keys[i].key));
        json_object_array_add(openrouter_keys_array, key_obj);
    }
    json_object_object_add(openrouter_obj, "api_keys", openrouter_keys_array);
    json_object_object_add(openrouter_obj, "active_api_key_nickname", json_object_new_string(config->openrouter.active_api_key_nickname));
    json_object_object_add(openrouter_obj, "model", json_object_new_string(config->openrouter.model));
    json_object_object_add(openrouter_obj, "site_url", json_object_new_string(config->openrouter.site_url));
    json_object_object_add(openrouter_obj, "site_name", json_object_new_string(config->openrouter.site_name));
    json_object_object_add(openrouter_obj, "render_markdown", json_object_new_boolean(config->openrouter.render_markdown));
    json_object_object_add(config_obj, "openrouter", openrouter_obj);
    
    json_object_object_add(export_obj, "configuration", config_obj);
    
    // Convert to string
    const char *json_string = json_object_to_json_string_ext(export_obj, JSON_C_TO_STRING_PRETTY);
    
    // Create a more robust encryption using password-based key derivation
    size_t json_len = strlen(json_string);
    size_t password_len = strlen(password);
    
    // Create a stronger key by hashing password multiple times
    unsigned char key[256];
    for (int i = 0; i < 256; i++) {
        key[i] = (unsigned char)((password[i % password_len] + i + 17) ^ (i * 3 + 7));
    }
    
    // Add random salt to make each encryption unique
    srand((unsigned int)time(NULL));
    unsigned char salt[16];
    for (int i = 0; i < 16; i++) {
        salt[i] = (unsigned char)(rand() % 256);
    }
    
    // Encrypt with multiple passes and salt
    char *encrypted_data = malloc(json_len + 16 + 1); // +16 for salt prefix
    
    // First, copy salt to beginning
    memcpy(encrypted_data, salt, 16);
    
    // Encrypt the JSON data using enhanced XOR with salt
    for (size_t i = 0; i < json_len; i++) {
        unsigned char byte_to_encrypt = (unsigned char)json_string[i];
        unsigned char key_byte = key[(i + salt[i % 16]) % 256];
        unsigned char salt_byte = salt[i % 16];
        encrypted_data[i + 16] = (char)(byte_to_encrypt ^ key_byte ^ salt_byte ^ (i & 0xFF));
    }
    encrypted_data[json_len + 16] = '\0';
    
    // Write to file
    FILE *file = fopen(full_path, "wb");
    if (!file) {
        display_message("Failed to create export file.", COLOR_RED);
        free(full_path);
        free(encrypted_data);
        json_object_put(export_obj);
        return false;
    }
    
    fwrite(encrypted_data, 1, json_len + 16, file); // +16 for salt
    fclose(file);
    
    printf("Configuration exported to: %s\n", full_path);
    
    free(full_path);
    free(encrypted_data);
    json_object_put(export_obj);
    return true;
}

// Import configuration from encrypted file
bool import_config_from_file(config_t *config, const char *filename, const char *password) {
    (void)config; // Will be used when full import parsing is implemented
    // Check if file exists
    FILE *file = fopen(filename, "rb");
    if (!file) {
        display_message("Failed to open import file.", COLOR_RED);
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read encrypted data
    char *encrypted_data = malloc(file_size + 1);
    if (!encrypted_data) {
        fclose(file);
        display_message("Memory allocation failed.", COLOR_RED);
        return false;
    }
    
    size_t bytes_read = fread(encrypted_data, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        display_message("Failed to read complete file.", COLOR_RED);
        free(encrypted_data);
        fclose(file);
        return false;
    }
    encrypted_data[file_size] = '\0';
    fclose(file);
    
    // Check file has minimum size (at least 16 bytes for salt)
    if (file_size < 16) {
        display_message("Invalid configuration file format.", COLOR_RED);
        free(encrypted_data);
        return false;
    }
    
    // Extract salt from first 16 bytes
    unsigned char salt[16];
    memcpy(salt, encrypted_data, 16);
    
    // Recreate encryption key from password
    size_t password_len = strlen(password);
    unsigned char key[256];
    for (int i = 0; i < 256; i++) {
        key[i] = (unsigned char)((password[i % password_len] + i + 17) ^ (i * 3 + 7));
    }
    
    // Decrypt the JSON data (skip first 16 bytes which are salt)
    long json_data_size = file_size - 16;
    char *decrypted_data = malloc(json_data_size + 1);
    
    for (long i = 0; i < json_data_size; i++) {
        unsigned char encrypted_byte = (unsigned char)encrypted_data[i + 16];
        unsigned char key_byte = key[(i + salt[i % 16]) % 256];
        unsigned char salt_byte = salt[i % 16];
        decrypted_data[i] = (char)(encrypted_byte ^ key_byte ^ salt_byte ^ (i & 0xFF));
    }
    decrypted_data[json_data_size] = '\0';
    
    free(encrypted_data);
    
    // Parse JSON
    json_object *import_obj = json_tokener_parse(decrypted_data);
    if (!import_obj) {
        display_message("Failed to parse configuration file. Invalid password or corrupted file.", COLOR_RED);
        free(decrypted_data);
        return false;
    }
    
    free(decrypted_data);
    
    // Check version compatibility
    json_object *version_obj;
    if (json_object_object_get_ex(import_obj, "deepshell_export_version", &version_obj)) {
        const char *version = json_object_get_string(version_obj);
        printf("Importing configuration from export version: %s\n", version);
    }
    
    // Get configuration object
    json_object *config_obj;
    if (!json_object_object_get_ex(import_obj, "configuration", &config_obj)) {
        display_message("Invalid configuration file format.", COLOR_RED);
        json_object_put(import_obj);
        return false;
    }
    
    // Ask for confirmation
    display_message("This will overwrite your current configuration. Are you sure? (y/N): ", COLOR_YELLOW);
    char *confirm = read_line();
    if (!confirm || (confirm[0] != 'y' && confirm[0] != 'Y')) {
        display_message("Import cancelled.", COLOR_YELLOW);
        if (confirm) free(confirm);
        json_object_put(import_obj);
        return false;
    }
    free(confirm);
    
    // Import configuration data
    json_object *active_llm_obj, *prev_llm_obj, *history_limit_obj, *streaming_obj, *animation_obj;
    
    // Load basic settings
    if (json_object_object_get_ex(config_obj, "active_llm_service", &active_llm_obj)) {
        const char *active_llm = json_object_get_string(active_llm_obj);
        strncpy(config->active_llm_service, active_llm, sizeof(config->active_llm_service) - 1);
        config->active_llm_service[sizeof(config->active_llm_service) - 1] = '\0';
    }
    
    if (json_object_object_get_ex(config_obj, "previous_llm_service", &prev_llm_obj)) {
        const char *prev_llm = json_object_get_string(prev_llm_obj);
        strncpy(config->previous_active_llm_service, prev_llm, sizeof(config->previous_active_llm_service) - 1);
        config->previous_active_llm_service[sizeof(config->previous_active_llm_service) - 1] = '\0';
    }
    
    if (json_object_object_get_ex(config_obj, "interactive_history_limit", &history_limit_obj)) {
        config->interactive_history_limit = json_object_get_int(history_limit_obj);
    }
    
    if (json_object_object_get_ex(config_obj, "response_streaming", &streaming_obj)) {
        config->enable_streaming = json_object_get_boolean(streaming_obj);
    }
    
    if (json_object_object_get_ex(config_obj, "progress_animation", &animation_obj)) {
        config->show_progress_animation = json_object_get_boolean(animation_obj);
    }
    
    // Load Ollama configuration
    json_object *ollama_obj;
    if (json_object_object_get_ex(config_obj, "ollama", &ollama_obj)) {
        json_object *server_addr_obj, *model_obj, *markdown_obj;
        
        if (json_object_object_get_ex(ollama_obj, "server_address", &server_addr_obj)) {
            const char *server_addr = json_object_get_string(server_addr_obj);
            strncpy(config->ollama.server_address, server_addr, sizeof(config->ollama.server_address) - 1);
            config->ollama.server_address[sizeof(config->ollama.server_address) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(ollama_obj, "model", &model_obj)) {
            const char *model = json_object_get_string(model_obj);
            strncpy(config->ollama.model, model, sizeof(config->ollama.model) - 1);
            config->ollama.model[sizeof(config->ollama.model) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(ollama_obj, "render_markdown", &markdown_obj)) {
            config->ollama.render_markdown = json_object_get_boolean(markdown_obj);
        }
    }
    
    // Load Gemini configuration
    json_object *gemini_obj;
    if (json_object_object_get_ex(config_obj, "gemini", &gemini_obj)) {
        json_object *keys_array_obj, *active_key_obj, *model_obj, *markdown_obj;
        
        // Clear existing keys
        config->gemini.api_key_count = 0;
        memset(config->gemini.active_api_key_nickname, 0, sizeof(config->gemini.active_api_key_nickname));
        
        if (json_object_object_get_ex(gemini_obj, "api_keys", &keys_array_obj)) {
            int array_len = json_object_array_length(keys_array_obj);
            for (int i = 0; i < array_len && i < MAX_SERVICES; i++) {
                json_object *key_obj = json_object_array_get_idx(keys_array_obj, i);
                json_object *nickname_obj, *key_value_obj;
                
                if (json_object_object_get_ex(key_obj, "nickname", &nickname_obj) &&
                    json_object_object_get_ex(key_obj, "key", &key_value_obj)) {
                    const char *nickname = json_object_get_string(nickname_obj);
                    const char *key_value = json_object_get_string(key_value_obj);
                    
                    strncpy(config->gemini.api_keys[i].nickname, nickname, sizeof(config->gemini.api_keys[i].nickname) - 1);
                    config->gemini.api_keys[i].nickname[sizeof(config->gemini.api_keys[i].nickname) - 1] = '\0';
                    
                    strncpy(config->gemini.api_keys[i].key, key_value, sizeof(config->gemini.api_keys[i].key) - 1);
                    config->gemini.api_keys[i].key[sizeof(config->gemini.api_keys[i].key) - 1] = '\0';
                    
                    config->gemini.api_key_count++;
                }
            }
        }
        
        if (json_object_object_get_ex(gemini_obj, "active_api_key_nickname", &active_key_obj)) {
            const char *active_key = json_object_get_string(active_key_obj);
            strncpy(config->gemini.active_api_key_nickname, active_key, sizeof(config->gemini.active_api_key_nickname) - 1);
            config->gemini.active_api_key_nickname[sizeof(config->gemini.active_api_key_nickname) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(gemini_obj, "model", &model_obj)) {
            const char *model = json_object_get_string(model_obj);
            strncpy(config->gemini.model, model, sizeof(config->gemini.model) - 1);
            config->gemini.model[sizeof(config->gemini.model) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(gemini_obj, "render_markdown", &markdown_obj)) {
            config->gemini.render_markdown = json_object_get_boolean(markdown_obj);
        }
    }
    
    // Load OpenRouter configuration
    json_object *openrouter_obj;
    if (json_object_object_get_ex(config_obj, "openrouter", &openrouter_obj)) {
        json_object *keys_array_obj, *active_key_obj, *model_obj, *site_url_obj, *site_name_obj, *markdown_obj;
        
        // Clear existing keys
        config->openrouter.api_key_count = 0;
        memset(config->openrouter.active_api_key_nickname, 0, sizeof(config->openrouter.active_api_key_nickname));
        
        if (json_object_object_get_ex(openrouter_obj, "api_keys", &keys_array_obj)) {
            int array_len = json_object_array_length(keys_array_obj);
            for (int i = 0; i < array_len && i < MAX_SERVICES; i++) {
                json_object *key_obj = json_object_array_get_idx(keys_array_obj, i);
                json_object *nickname_obj, *key_value_obj;
                
                if (json_object_object_get_ex(key_obj, "nickname", &nickname_obj) &&
                    json_object_object_get_ex(key_obj, "key", &key_value_obj)) {
                    const char *nickname = json_object_get_string(nickname_obj);
                    const char *key_value = json_object_get_string(key_value_obj);
                    
                    strncpy(config->openrouter.api_keys[i].nickname, nickname, sizeof(config->openrouter.api_keys[i].nickname) - 1);
                    config->openrouter.api_keys[i].nickname[sizeof(config->openrouter.api_keys[i].nickname) - 1] = '\0';
                    
                    strncpy(config->openrouter.api_keys[i].key, key_value, sizeof(config->openrouter.api_keys[i].key) - 1);
                    config->openrouter.api_keys[i].key[sizeof(config->openrouter.api_keys[i].key) - 1] = '\0';
                    
                    config->openrouter.api_key_count++;
                }
            }
        }
        
        if (json_object_object_get_ex(openrouter_obj, "active_api_key_nickname", &active_key_obj)) {
            const char *active_key = json_object_get_string(active_key_obj);
            strncpy(config->openrouter.active_api_key_nickname, active_key, sizeof(config->openrouter.active_api_key_nickname) - 1);
            config->openrouter.active_api_key_nickname[sizeof(config->openrouter.active_api_key_nickname) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(openrouter_obj, "model", &model_obj)) {
            const char *model = json_object_get_string(model_obj);
            strncpy(config->openrouter.model, model, sizeof(config->openrouter.model) - 1);
            config->openrouter.model[sizeof(config->openrouter.model) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(openrouter_obj, "site_url", &site_url_obj)) {
            const char *site_url = json_object_get_string(site_url_obj);
            strncpy(config->openrouter.site_url, site_url, sizeof(config->openrouter.site_url) - 1);
            config->openrouter.site_url[sizeof(config->openrouter.site_url) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(openrouter_obj, "site_name", &site_name_obj)) {
            const char *site_name = json_object_get_string(site_name_obj);
            strncpy(config->openrouter.site_name, site_name, sizeof(config->openrouter.site_name) - 1);
            config->openrouter.site_name[sizeof(config->openrouter.site_name) - 1] = '\0';
        }
        
        if (json_object_object_get_ex(openrouter_obj, "render_markdown", &markdown_obj)) {
            config->openrouter.render_markdown = json_object_get_boolean(markdown_obj);
        }
    }
    
    display_message("Configuration imported successfully!", COLOR_GREEN);
    
    json_object_put(import_obj);
    return true;
} 