import sys
import time
from rich.console import Console # rich_console is already defined here
from rich.progress import Progress, SpinnerColumn, TextColumn

# Define ANSI escape codes for various colors globally
COLORS = {
    "red": "\033[91m",
    "green": "\033[92m",
    "yellow": "\033[93m",
    "blue": "\033[94m",
    "orange": "\033[33m",
    "reset": "\033[0m"
}

# Global Rich Console instance
rich_console = Console()

# LLM Service Constants
LLM_SERVICE_OLLAMA = "ollama"
LLM_SERVICE_GEMINI = "gemini"
SUPPORTED_LLM_SERVICES = [LLM_SERVICE_OLLAMA, LLM_SERVICE_GEMINI]

def display_message(message, color_name=None, end='\n'):
    """
    Prints a message to the console with optional ANSI color codes.
    Allows specifying the end character (like print's end).
    """
    if color_name and color_name in COLORS:
        print(f"{COLORS[color_name]}{message}{COLORS['reset']}", end=end)
    else:
        print(message, end=end)

def _animate_progress(stop_event, status_text):
    """
    Displays a Rich-based progress animation with a spinner and custom text.
    The animation runs until `stop_event` is set.
    """
    with Progress(
        SpinnerColumn(),
        TextColumn(f"[bold blue]{status_text}"), # Use the passed status_text
        SpinnerColumn(),
        console=rich_console,
        transient=True, # Hides the progress bar upon completion
    ) as progress:
        progress.add_task("Waiting", total=None) # Indeterminate task
        # Keep the thread alive until stop_event is set
        while not stop_event.is_set():
            time.sleep(0.1) # Small sleep to prevent busy-waiting