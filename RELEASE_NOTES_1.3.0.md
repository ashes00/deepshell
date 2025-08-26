# 🚀 DeepShell v1.3.0 - Major Feature Release

## 🎯 Overview

We're excited to announce **DeepShell v1.3.0**, a major feature release that significantly expands DeepShell's capabilities with **OpenRouter.ai integration**, **secure configuration backup/migration**, and numerous enhancements that make DeepShell the most comprehensive LLM command-line interface available.

## ✨ New Features

### 🌐 OpenRouter.ai Integration
**Complete third LLM service with full feature parity**

- **200+ Model Access**: Connect to OpenAI, Anthropic, Meta, Google, Mistral, and many more providers through a single interface
- **Advanced Model Browser**: 
  - Paginated model selection (15 models per page)
  - Smart sorting: **Free models listed first**, then alphabetical
  - Navigate with `n` (next), `p` (previous), `q` (quit)
- **Multi-Key Management**: Store multiple OpenRouter API keys with custom nicknames
- **Full Feature Support**: All DeepShell features work seamlessly with OpenRouter
- **Free Model Support**: Easy access to free models like `openai/gpt-4o-mini:free`

### 🔐 Configuration Backup & Migration
**Enterprise-grade configuration management**

- **Encrypted Export** (`-b filename.config`):
  - Password-protected with confirmation
  - Secure binary format (completely unreadable as text)
  - Saves to Downloads folder automatically
  - Includes ALL settings, API keys, and configurations
- **Secure Import** (`-c filename.config`):
  - Password verification
  - Confirmation prompt before overwriting
  - Future-proof with version metadata
- **Perfect for**:
  - Backing up your complete DeepShell setup
  - Migrating between development machines
  - Sharing team configurations securely
  - Disaster recovery

### 🔑 Enhanced API Key Management
**Unified and powerful key management across all services**

- **Service-Agnostic Commands**: 
  - `-set-key`: Now manages both Gemini AND OpenRouter keys
  - `-show-key`: Shows active key for current LLM service
  - `-a` / `--active-config`: Quick summary of active LLM, model, and API key
- **Consistent UX**: OpenRouter key management now matches Gemini's interface exactly
- **Smart Validation**: Both services support nickname-based multi-key workflows

## 🛠️ Improvements & Fixes

### User Experience Enhancements
- **Alphabetized Help Menu**: All command-line options now sorted alphabetically for easier reference
- **Updated Interactive Logo**: Now properly shows "Multi-LLM Support (Ollama, Gemini, and OpenRouter)"
- **Improved Error Handling**: Better error messages and validation throughout
- **Enhanced Model Selection**: Improved pagination and user-friendly navigation

### Technical Improvements
- **Auto-Setup Bypass**: Export/import flags now correctly bypass automatic setup
- **Future-Proof Design**: Export format includes version metadata for cross-version compatibility
- **Binary Security**: Export files are true binary format, preventing accidental text viewing
- **Memory Management**: Enhanced memory handling and cleanup throughout

### Bug Fixes
- Fixed import flag being overridden by default configuration setup
- Resolved model selection issues for OpenRouter
- Corrected API key management inconsistencies
- Fixed various edge cases in configuration handling

## 📊 What's Included

### Complete Configuration Export
When you export your configuration, **everything** is included:
- All LLM service configurations (Ollama, Gemini, OpenRouter)
- All API keys with their nicknames
- Model selections for each service
- Interactive settings (history limit, streaming, animation)
- Markdown rendering preferences
- Server addresses and site attribution

### OpenRouter Model Categories
Access to major model families including:
- **OpenAI**: GPT-4, GPT-3.5 (including free variants)
- **Anthropic**: Claude 3.5 Sonnet, Claude 3 Haiku
- **Meta**: Llama 3.1, Llama 3.2 (including free versions)
- **Google**: Gemma 2, PaLM models
- **Mistral**: Mixtral, Mistral 7B
- **And 190+ more models** from various providers

## 🚀 Getting Started with New Features

### Try OpenRouter
```bash
# Setup OpenRouter
./deepshell -s
# Select option 1 (Manage LLM Services)
# Choose 3 (OpenRouter)

# Quick model change with new pagination
./deepshell -m
# Browse models with n/p navigation, free models listed first

# Manage multiple OpenRouter keys
./deepshell -set-key
# Choose 2 (OpenRouter)
```

### Backup Your Configuration
```bash
# Export your complete setup
./deepshell -b my-deepshell-backup.config
# Enter password twice for protection

# Import on another machine
./deepshell -c my-deepshell-backup.config
# Enter password and confirm overwrite
```

### Quick Status Check
```bash
# See your current setup at a glance
./deepshell -a
# Shows: LLM Service, Model, API Key (with nickname)
```

## 🔧 Migration Guide

### From v1.2.x
No breaking changes! Your existing configuration will work seamlessly. New features are additive.

### Recommended Actions
1. **Backup First**: `./deepshell -b v1-2-backup.config`
2. **Try OpenRouter**: Add it as a third LLM option
3. **Test New Commands**: Explore `-a` for quick status checks

## 🎯 What's Next

DeepShell v1.3.0 establishes a solid foundation for multi-LLM management with enterprise-grade backup capabilities. Future releases will focus on:
- Additional LLM service integrations
- Advanced conversation management features
- Enhanced interactive mode capabilities
- Performance optimizations

## 📚 Documentation

Full documentation with examples and tutorials is available in our [README.md](README.md).

## 🙏 Acknowledgments

Special thanks to the community for feature requests, bug reports, and testing that made this release possible.

---

**Download DeepShell v1.3.0** from the [releases page](https://github.com/ashes00/deepshell/releases) or build from source.

**Need Help?** Check our [README.md](README.md) or open an issue on GitHub.

Happy Querying! 🤖✨
