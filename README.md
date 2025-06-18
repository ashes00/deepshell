# DeepShell: Your Universal LLM Command-Line Interface

[![Python Version](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/)

<!-- Add other badges as appropriate, e.g., license, build status -->

**DeepShell**: Your Universal LLM Command-Line Interface

**DeepShell** is a powerful and versatile command-line program that seamlessly blends the familiar environment of your local shell with the immense knowledge and capabilities of Large Language Models (LLMs). Imagine having direct, remote access to the world's most advanced AI models—from local Ollama instances to cloud-based services like Google's Gemini—all unified within your terminal.

Designed for developers, researchers, and power users who demand precision, flexibility, and efficiency, DeepShell cuts through the complexity of API integrations. It offers a streamlined pathway to query both open-source and proprietary LLMs, transforming your command prompt into a conduit for deep AI intelligence. Whether you're generating code snippets, summarizing extensive documents, brainstorming complex ideas, or crafting creative text, DeepShell makes it effortless.

Say goodbye to juggling multiple tools! With DeepShell, your command line isn't just for local commands anymore; it's your portal to a new dimension of intelligent interaction, empowering you with efficient, configurable LLM access.

## ✨ Features

* **Multi-LLM Support:**
  * Seamlessly connect to **Ollama** servers (local or remote).
  * Integrate with **Google Gemini API**.
  * Easily switch between configured LLM services.
* **Interactive Setup & Configuration:**
  * User-friendly setup wizard (`-s`) to guide you through initial configuration.
  * Manages LLM service details, including server addresses (Ollama) and API keys (Gemini).
  * Stores configuration securely in `~/.deepshell/deepshell.conf`.
* **Flexible Model Management:**
  * List available models from your connected LLM service.
  * Set and change default models per service (`-model`).
* **Gemini API Key Management:**
  * Store and manage multiple Gemini API keys with user-defined nicknames.
  * Easily switch between active Gemini API keys (`-set-key`).
  * Display the currently active Gemini API key (`-show-key`).
  * Quick link to check your Gemini API usage (`-gq`).
* **Intuitive Querying:**
  * Send queries directly from your command line (`-q` or `--query`).
  * Engaging progress animation while waiting for LLM responses.
* **User-Friendly Interface:**
  * Clear, colored console output for enhanced readability.
  * Well-formatted and alphabetized help messages (`-h`).
  * View active configuration details (`-show-config`).
  * Option to delete your configuration (`-d`).

## 🚀 Why DeepShell?

* **Unified Experience:** Access different LLMs without context switching.
* **Efficiency:** Quickly query models and manage configurations from your terminal.
* **Customizable:** Tailor DeepShell to your preferred models and services.
* **Developer-Friendly:** Built with Python, making it easy to understand and extend.
* **Local First, Cloud Ready:** Perfect for local development with Ollama and scalable with cloud LLMs like Gemini.

## 🛠️ Installation

1. **Prerequisites:**
   
   * Python 3.7 or higher.
   * `pip` (Python package installer).

2. **Clone the Repository:**
   
   ```bash
   git clone <your-repository-url> # Replace with your actual repo URL
   cd deepshell
   ```

3. **Install Dependencies (If running the python file):**
   DeepShell uses the `requests` library for API communication, but all required modules will be listed in `modules.txt`.
   
   ```bash
   pip install requests chardet
   ```

4. **Run Python version:**

   ```bash
   python3 main.py
   ```

5. **Run Executable Binary Release :** 
   
   ```bash
   ./deepshell
   ```

## 🏁 Getting Started: Initial Setup

The first time you run DeepShell, or if you want to reconfigure, use the `-s` flag:

```bash
./deepshell -s (or --setup)
```

This will launch an interactive wizard that helps you:

1. Choose an LLM service to configure (Ollama or Gemini).
2. **For Ollama:** Enter your Ollama server address and select a default model.
3. **For Gemini:** Manage your API keys (add, set active) and select a default model.

Your settings will be saved to `~/.deepshell/deepshell.conf`.

## 💻 Usage & Command-Line Options

Here are some common ways to use DeepShell:

* **Querying the Active LLM:**
  
  ```bash
  ./deepshell -q (or --query) "What are the benefits of using a CLI for LLM interaction?"
  ```

* **Changing the Default Model for the Active Service:**
  
  ```bash
  ./deepshell -model (or --model-change)
  ```
  
  (This will list available models and prompt you to choose a new default.)

* **Switching or Configuring LLM Services:**
  
  ```bash
  ./deepshell -l (or --llm)
  ```
  
  (This allows you to switch between already configured services like Ollama and Gemini, or add/reconfigure one.)

* **Managing Gemini API Keys:**
  
  * Add a new key or set an existing one as active:
    
    ```bash
    ./deepshell -set-key (or --set-api-key)
    ```
  
  * Show the currently active Gemini API key nickname and value:
    
    ```bash
    ./deepshell -show-key (or --show-api-key)
    ```
  
  * Get information on checking your Gemini API quota:
    
    ```bash
    ./deepshell -gq (or --gemini-quota)
    ```

* **Viewing Active Configuration:**
  
  ```bash
  ./deepshell -show-config (or --show-full-config)
  ```

* **Getting Help:**
  
  ```bash
  ./deepshell -h (or --help)
  ```

* **Checking Version:**
  
  ```bash
  ./deepshell -v (or --version)
  ```

* **Deleting Configuration:** (Use with caution!)
  
  ```bash
  ./deepshell -d (or --delete-config)
  ```

## ⚙️ Configuration File

DeepShell stores its configuration in a JSON file located at `~/.deepshell/deepshell.conf`. While you can view this file, it's recommended to manage settings through DeepShell's command-line options for safety and ease of use.

The configuration includes:

* `active_llm_service`: The currently selected LLM service (e.g., "ollama" or "gemini").
* `llm_services`: A dictionary containing specific configurations for each service:
  * **Ollama:** `server_address` and default `model`.
  * **Gemini:** A list of `api_keys` (each with a `nickname` and `key` value), the `active_api_key_nickname`, and the default `model`.

## 🤖 Supported LLMs

* **Ollama:** Connect to any Ollama instance serving models like Llama, Mistral, etc.
* **Google Gemini:** Access Gemini models (e.g., `gemini-pro`) via the Google AI Studio API.

*(Support for more LLM providers can be added in the future!)*

---

We hope DeepShell enhances your productivity and makes interacting with LLMs a breeze!
If you have suggestions or encounter issues, please feel free to open an issue on the project repository.

Happy Hacking!
