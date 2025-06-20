<div align="center">
  <h1>DeepShell</h1>
  <p><strong>Your Universal LLM Command-Line Interface</strong></p>
</div>

[![Python Version](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/)
<!-- Add other badges as appropriate, e.g., license, build status, etc. -->

**DeepShell** is a powerful and versatile command-line program that seamlessly blends the familiar environment of your local shell with the immense knowledge and capabilities of Large Language Models (LLMs). Imagine having direct access to the world's most advanced AI models—from local Ollama instances to cloud-based services like Google's Gemini—all unified within a single, efficient terminal interface.

Designed for developers, researchers, and power users, DeepShell abstracts away the complexity of API integrations. It offers a streamlined pathway to query both open-source and proprietary LLMs, transforming your command prompt into a conduit for deep AI intelligence.

## ✨ Features

*   **Multi-LLM Support:**
    *   Seamlessly connect to **Ollama** servers (local or remote).
    *   Integrate with the **Google Gemini API**.
*   **Conversational Memory:**
    *   Engage in multi-turn conversations using the new **interactive mode** (`-i`).
    *   The model remembers the context of the last 10 turns of your conversation.
*   **Unified & Interactive Configuration:**
    *   A central, user-friendly settings menu (`-s`) guides you through all configuration tasks.
    *   Manages LLM service details, including server addresses (Ollama) and API keys (Gemini).
    *   Stores configuration securely in `~/.deepshell/deepshell.conf`.
*   **Flexible Service & Model Management:**
    *   Easily switch between configured LLM services (`-l`).
    *   Quickly jump back to the previously used LLM service (`-j`).
    *   List available models from your connected LLM service and change the default model per service (`-m`).
*   **Advanced Gemini API Key Management:**
    *   Store and manage multiple Gemini API keys with user-defined nicknames.
    *   Easily add new keys or set an active key from your stored list (`-set-key`).
    *   Display the currently active Gemini API key's nickname and value (`-show-key`).
    *   Quickly check your Gemini API key status and get a link to your usage dashboard (`-gq`).
*   **Intuitive User Experience:**
    *   Send queries directly from your command line (`-q`).
    *   Beautiful Markdown rendering for LLM responses in the terminal, powered by `rich`.
    *   Engaging progress animation while waiting for the LLM.
    *   Clear, colored console output for enhanced readability.
    *   Well-formatted and alphabetized help messages (`-h`).

## 🛠️ Installation

1. **Prerequisites:**
    *   Python 3.7 or higher.
    *   `pip` (Python package installer).

2. **Clone the Repository:**
    ```bash
    git clone https://github.com/ashes00/deepshell.git
    cd deepshell
    ```

3. **Install Dependencies:**
    The required Python modules are listed in `modules.txt`. You can install them manually or use the provided development setup script.
    ```bash
    pip install -r <(grep -vE "^\s*#|^\s*$" modules.txt)
    ```

4. **Run DeepShell:**
    *   **From source:**
        ```bash
        python3 main.py [OPTIONS]
        ```
    *   **As an executable** (if you've built one):
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

Your settings will be saved to `~/.deepshell/deepshell.conf`.

## 💻 Usage & Command-Line Options

### Primary Usage

**Query the active LLM**
```bash
./deepshell -q "What are the benefits of using a CLI for LLM interaction?"
./deepshell --query "Write a python function to calculate a factorial"
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

### Gemini-Specific Commands

**Interactively manage Gemini API keys** (add, remove, set active)
```bash
./deepshell -set-key (or --set-api-key)
```

**Show the active Gemini API key nickname and value**
```bash
./deepshell -show-key (or --show-api-key)
```

**Check Gemini API key status and get quota info**
```bash
./deepshell -gq (or --gemini-quota)
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

## ⚙️ Configuration File

DeepShell stores its configuration in a JSON file located at `~/.deepshell/deepshell.conf`. While you can view this file, it's recommended to manage settings through DeepShell's command-line options for safety and ease of use.

An example configuration might look like this:
```json
{
    "active_llm_service": "gemini",
    "previous_active_llm_service": "ollama",
    "llm_services": {
        "ollama": {
            "server_address": "http://localhost:11434",
            "model": "llama3:latest",
            "render_markdown": true
        },
        "gemini": {
            "api_keys": [
                {
                    "nickname": "personal-key",
                    "key": "BIsa8y..."
                }
            ],
            "active_api_key_nickname": "personal-key",
            "model": "models/gemini-1.5-flash",
            "render_markdown": true
        }
    }
}
```

## 🤖 Supported LLMs

----
*   **Ollama:** Connect to any Ollama instance serving models like Llama, Mistral, etc.
*   **Google Gemini:** Access Gemini models (e.g., `gemini-1.5-pro`, `gemini-1.5-flash`) via the Google AI Studio API.

---

## ⚙️ Pro Tip

1.  Copy deepshell to your Environment path:
```bash
nano .bashrc 
export PATH=$PATH:/home/user/APPS-DIR
```
2.  Create an aliases for ds & dsq for quick keyboard actions.
```bash
nano .bashrc 
alias ds="deepshell"
alias dsq="deepshell -q"
```
3.  Save .bashrc file.
```bash
Ctrl+s & Ctrl+x
```
4. Update your .bashrc file to use commands
```bash
source .bashrc
```
5.  Us the alias to quickly query the LLM
```bash
dsq What is the best LLM?
```
6.  Use the alias to quickly access features
```bash
ds -v
```

Happy Querying!
