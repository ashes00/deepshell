<div align="center">
  <h1>DeepShell</h1>
  <p><strong>Your Universal LLM Command-Line Interface</strong></p>
  
  <img src="images/deepshell-i.png" alt="DeepShell Interactive Mode" width="600" style="border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);">
</div>

[![C Version](https://img.shields.io/badge/C-Native-green.svg)](https://gcc.gnu.org/)
<!-- Add other badges as appropriate, e.g., license, build status, etc. -->

**DeepShell** is a powerful and versatile command-line program written in C that seamlessly blends the familiar environment of your local shell with the immense knowledge and capabilities of Large Language Models (LLMs). Imagine having direct access to the world's most advanced AI models—from local Ollama instances to cloud-based services like Google's Gemini—all unified within a single, efficient native binary.

Designed for developers, researchers, and power users, DeepShell abstracts away the complexity of API integrations. It offers a streamlined pathway to query both open-source and proprietary LLMs, transforming your command prompt into a conduit for deep AI intelligence with native performance.

## ✨ Features

*   **Multi-LLM Support:**
    *   Seamlessly connect to **Ollama** servers (local or remote).
    *   Integrate with the **Google Gemini API**.
    *   Connect to **OpenRouter.ai** for access to 200+ LLM models from various providers.
        *   Advanced model selection with pagination and sorting (free models first).
        *   Multi-key management with nicknames for different OpenRouter accounts.
*   **Conversational Memory & Customization:**
    *   Engage in multi-turn conversations using the **interactive mode** (`-i`).
    *   Set the conversation history limit (defaults to 25 turns).
    *   Toggle response streaming for immediate (plain-text) or complete (Markdown-rendered) output. Streaming is disabled by default to preserve formatting.
    *   Enable or disable Markdown rendering for each LLM service individually.
*   **Unified & Interactive Configuration:**
    *   A central, user-friendly settings menu (`-s`) guides you through all configuration tasks.
    *   Manages LLM service details, including server addresses (Ollama) and API keys (Gemini).
    *   Stores configuration securely in `~/.deepshell/deepshell.conf`.
*   **Flexible Service & Model Management:**
    *   Easily switch between configured LLM services (`-l`).
    *   Quickly jump back to the previously used LLM service (`-j`).
    *   List available models from your connected LLM service and change the default model per service (`-m`).
*   **Advanced API Key Management:**
    *   Store and manage multiple API keys with user-defined nicknames for both Gemini and OpenRouter.
    *   Unified key management interface (`-set-key`) supporting both services.
    *   Display the currently active API key for any LLM service (`-show-key`).
    *   Quick active configuration summary showing current LLM, model, and API key (`-a`).
    *   Gemini-specific quota checking and usage dashboard access (`-gq`).
*   **Configuration Backup & Migration:**
    *   Export complete configuration to encrypted files (`-b`) with password protection.
    *   Import configuration from backup files (`-c`) with confirmation prompts.
    *   Files saved to Downloads folder in secure binary format (unreadable as text).
    *   Future-proof design with version metadata for cross-version compatibility.
    *   Perfect for backing up settings, sharing configurations, or migrating between systems.
*   **Intuitive User Experience:**
    *   Send queries directly from your command line (`-q`).
         *   Beautiful Markdown rendering for LLM responses in the terminal with native C implementation.
    *   Engaging progress animation while waiting for the LLM.
    *   Clear, colored console output for enhanced readability.
    *   Well-formatted and alphabetized help messages (`-h`).

## 🛠️ Installation

### 📦 **Quick Install (Recommended)**

**For Ubuntu/Debian systems:**
```bash
# Download the appropriate .deb package for your system
wget https://github.com/ashes00/deepshell/releases/download/v1.3.1/deepshell-1.3.1-ubuntu-20.04.deb

# Install the package
sudo dpkg -i deepshell-1.3.1-ubuntu-20.04.deb

# Install any missing dependencies
sudo apt-get install -f
```

**For other Linux distributions:**
- Check the [releases page](https://github.com/ashes00/deepshell/releases) for pre-built packages
- Look for packages matching your distribution and architecture

### 🔨 **Build from Source**

1. **Prerequisites:**
    *   GCC compiler and build tools
    *   libcurl4-openssl-dev
    *   libjson-c-dev
    *   libreadline-dev

2. **Install Dependencies (Linux):**
    ```bash
    sudo apt-get update
    sudo apt-get install -y build-essential libcurl4-openssl-dev libjson-c-dev libreadline-dev
    ```

3. **Clone the Repository:**
    ```bash
    git clone https://github.com/ashes00/deepshell.git
    cd deepshell
    ```

4. **Build DeepShell:**
    ```bash
    make
    ```

5. **Run DeepShell:**
    ```bash
    ./deepshell [OPTIONS]
    ```

## 🏁 Getting Started: Initial Setup

The first time you run DeepShell, or anytime you want to manage settings, use the `-s` or `--setup` flag:

```bash
./deepshell -s
```

This launches a comprehensive, interactive menu that allows you to:
1.  **Add or Reconfigure LLM Services:**
    *   **For Ollama:** Enter your server address (e.g., `http://localhost:11434`) and select a default model from those available on your server.
    *   **For Gemini:** Manage your API keys (add, remove, set active) and select a default model from the Gemini API.
2.  **Switch** the active LLM service.
3.  **Change** the default model for the currently active service.
4.  **Manage** Gemini API keys specifically.
5.  **View** your current configuration or **delete** it entirely.
6.  **Toggle Markdown Rendering:** Enable or disable Markdown formatting for the active service's responses.
7.  **Set Interactive History Limit:** Change the number of conversation turns remembered in interactive mode.
8.  **Toggle Response Streaming:** Enable or disable streaming responses. (Note: Markdown is not supported in streaming mode).


Your settings will be saved to `~/.deepshell/deepshell.conf`.

## 💻 Usage & Command-Line Options

### Primary Usage

**Query the active LLM**
```bash
./deepshell -q "What are the benefits of using a CLI for LLM interaction?"
./deepshell --query "Write a function to calculate a factorial"
```

### LLM & Model Management

**Enter the main settings menu**
```bash
./deepshell -s (or --setup)
```

**Switch active service or configure services** (shortcut to a settings sub-menu)
```bash
./deepshell -l (or --llm)
```

**Quickly jump to the previously used LLM service**
```bash
./deepshell -j (or --jump-llm)
```

**Change the default model for the active service** (shortcut)
```bash
./deepshell -m (or --model-change)
```

### API Key Management

**Interactively manage API keys for LLM services** (Gemini or OpenRouter)
```bash
./deepshell -set-key (or --set-api-key)
```

**Show the active API key for current LLM service**
```bash
./deepshell -show-key (or --show-api-key)
```

**Quick summary of active LLM, model, and API key**
```bash
./deepshell -a (or --active-config)
```

**Check Gemini API key status and get quota info**
```bash
./deepshell -gq (or --gemini-quota)
```

### Configuration Backup & Migration

**Export configuration to encrypted backup file**
```bash
./deepshell -b mybackup.config (or --export mybackup.config)
```

**Import configuration from encrypted backup file**
```bash
./deepshell -c mybackup.config (or --import mybackup.config)
```

### Configuration & Info

**Display the currently active configuration details**
```bash
./deepshell -show-config (or --show-full-conf)
```

**Delete the entire configuration file** (use with caution!)
```bash
./deepshell -d (or --delete-config)
```

**Show the help message**
```bash
./deepshell -h (or --help)
```

**Show the program's version**
```bash
./deepshell -v (or --version)
```

**Start an interactive chat session**
```bash
./deepshell -i (or --interactive)
```

## ⚙️ Configuration File

DeepShell stores its configuration in a JSON file located at `~/.deepshell/deepshell.conf`. While you can view this file, it's recommended to manage settings through DeepShell's command-line options for safety and ease of use.

An example configuration might look like this:
```json
{
    "active_llm_service": "openrouter",
    "previous_active_llm_service": "gemini",
    "interactive_history_limit": 25,
    "enable_streaming": false,
    "show_progress_animation": true,
    "ollama": {
        "server_address": "http://localhost:11434",
        "model": "llama3:latest",
        "render_markdown": true
    },
    "gemini": {
        "api_keys": [
            {
                "nickname": "personal-key",
                "key": "AIza..."
            }
        ],
        "active_api_key_nickname": "personal-key",
        "model": "models/gemini-1.5-flash",
        "render_markdown": true
    },
    "openrouter": {
        "api_keys": [
            {
                "nickname": "work-account",
                "key": "sk-or-v1-..."
            }
        ],
        "active_api_key_nickname": "work-account",
        "model": "openai/gpt-4o",
        "site_url": "https://myproject.com",
        "site_name": "My Project",
        "render_markdown": true
    }
}
```

## 🚀 Performance Benefits

The C version offers several advantages over interpreted languages:
- **Faster startup time**: No interpreter overhead
- **Smaller executable**: Single binary with minimal dependencies
- **Lower memory usage**: More efficient memory management
- **Better system integration**: Native system calls
- **Extended timeouts**: 30-second request timeout for better compatibility with slower models

## 🤖 Supported LLMs

----
*   **Ollama:** Connect to any Ollama instance serving models like Llama, Mistral, etc.
*   **Google Gemini:** Access Gemini models (e.g., `gemini-1.5-pro`, `gemini-1.5-flash`) via the Google AI Studio API.
*   **OpenRouter.ai:** Access 200+ models from providers like OpenAI, Anthropic, Meta, Google, and more:
    *   GPT-4, GPT-3.5, Claude, Llama, Mixtral, Gemma, and many others
    *   Free and paid models with transparent pricing
    *   Advanced model browser with pagination and sorting (free models first)
    *   Multi-account support with API key nicknames

---

## ⚙️ Pro Tip

1.  Copy deepshell to your Environment path:
```bash
sudo cp deepshell /usr/local/bin/
```

2.  Create aliases for ds & dsq for quick keyboard actions:
```bash
nano .bashrc 
alias dsq="deepshell -q"
alias ds="deepshell"
alias dsi="deepshell -i"
```

3.  Save .bashrc file:
```bash
Ctrl+s & Ctrl+x
```

4.  Update your .bashrc file to use commands:
```bash
source .bashrc
```

5.  Use the alias `dsq` to quickly query the LLM:
```bash
dsq What is the best LLM?
```

6.  Use the alias `ds` to quickly access features with options:
```bash
ds -v
```

7.  Use the alias `dsi` to enter interactive mode:
```bash
dsi
```

Happy Querying!!!
