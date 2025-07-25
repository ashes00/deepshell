# DeepShell C Version

This is the C implementation of DeepShell, a command-line interface for interacting with Large Language Models (LLMs). This version provides the same functionality as the Python version but is written in C for better performance and smaller executable size.

## Features

- **Multi-LLM Support**: Connect to Ollama servers and Google Gemini API
- **Interactive Mode**: Chat with LLMs in an interactive session
- **Configuration Management**: Easy setup and management of LLM services
- **Conversation History**: Maintain context across multiple queries
- **Markdown Rendering**: Beautiful output formatting
- **Cross-platform**: Works on Linux and Windows (with MSYS2/Mingw64)

## Prerequisites

### Linux
```bash
sudo apt-get update
sudo apt-get install -y build-essential libcurl4-openssl-dev libjson-c-dev
```

### Windows (MSYS2/Mingw64)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-curl mingw-w64-x86_64-json-c
```

## Building

### Linux
```bash
cd C-Version
make
```

### Windows (MSYS2/Mingw64)
```bash
cd C-Version
make
```

### Development Build (with debug info)
```bash
make debug
```

### Release Build
```bash
make release
```

## Installation

After building, you can install the executable:

```bash
# Copy to a directory in your PATH
sudo cp deepshell /usr/local/bin/

# Or add the current directory to your PATH
export PATH=$PATH:$(pwd)
```

## Usage

### Initial Setup

The first time you run DeepShell, you'll need to configure your LLM services:

```bash
./deepshell -s
```

This will guide you through:
1. Configuring Ollama server connection
2. Setting up Gemini API keys
3. Selecting default models
4. Configuring other options

### Basic Usage

**Query an LLM:**
```bash
./deepshell -q "What is the capital of France?"
```

**Interactive mode:**
```bash
./deepshell -i
```

**Show help:**
```bash
./deepshell -h
```

### Command Line Options

- `-s, --setup`: Run interactive configuration setup
- `-q, --query QUERY`: Send a query to the active LLM
- `-i, --interactive`: Start an interactive chat session
- `-l, --llm`: Switch or configure LLM services
- `-model, --model-change`: Change the default model for active service
- `-set-key, --set-api-key`: Manage Gemini API keys
- `-show-key, --show-api-key`: Show active Gemini API key
- `-gq, --gemini-quota`: Check Gemini API quota
- `-show-config, --show-full-conf`: Display current configuration
- `-j, --jump-llm`: Switch to previously used LLM service
- `-d, --delete-config`: Delete configuration file
- `-v, --version`: Show version information
- `-h, --help`: Show help message

## Configuration

DeepShell stores its configuration in `~/.deepshell/deepshell.conf`. The configuration includes:

- Active LLM service
- Server addresses and API keys
- Default models for each service
- Markdown rendering preferences
- Interactive session settings

## Supported LLMs

### Ollama
- Connect to any Ollama server (local or remote)
- Supports all Ollama models (Llama, Mistral, etc.)
- Automatic model discovery

### Google Gemini
- Access to Gemini models via Google AI Studio API
- Multiple API key management
- Quota monitoring

## Examples

### Setup Ollama
```bash
./deepshell -s
# Choose option 1 (Configure Ollama service)
# Enter server address: http://localhost:11434
# Select a model from the list
```

### Setup Gemini
```bash
./deepshell -s
# Choose option 2 (Configure Gemini service)
# Add your API key with a nickname
# Select a model from the list
```

### Quick Query
```bash
./deepshell -q "Write a C function to calculate factorial"
```

### Interactive Session
```bash
./deepshell -i
# Type your questions and get responses
# Type 'exit' to quit
```

## Troubleshooting

### Build Issues
- Ensure you have the required development libraries installed
- On Windows, make sure you're using MSYS2/Mingw64 environment
- Check that libcurl and libjson-c are properly installed

### Runtime Issues
- Verify your Ollama server is running and accessible
- Check that your Gemini API key is valid
- Ensure network connectivity for API calls

### Configuration Issues
- Delete the config file (`~/.deepshell/deepshell.conf`) and run setup again
- Check file permissions for the config directory

## Performance

The C version offers several advantages over the Python version:
- **Faster startup time**: No Python interpreter overhead
- **Smaller executable**: Single binary with minimal dependencies
- **Lower memory usage**: More efficient memory management
- **Better system integration**: Native system calls

## Development

### Project Structure
```
C-Version/
├── Makefile          # Build configuration
├── deepshell.h       # Main header file
├── main.c           # Command-line interface
├── utils.c          # Utility functions
├── config.c         # Configuration management
├── settings.c       # Interactive setup
├── ollama.c         # Ollama API integration
├── gemini.c         # Gemini API integration
├── interactive.c    # Interactive mode
└── README.md        # This file
```

### Adding New Features
1. Add function prototypes to `deepshell.h`
2. Implement functions in appropriate `.c` files
3. Update the Makefile if adding new source files
4. Test thoroughly with different LLM services

## License

This project follows the same license as the original Python version.

## Contributing

Contributions are welcome! Please ensure your code:
- Follows the existing coding style
- Includes proper error handling
- Is tested on both Linux and Windows
- Maintains compatibility with existing features 