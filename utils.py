import sys
import threading
import time
from rich.console import Console

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

def _animate_progress(stop_event, message_length):
    """
    Displays a simple command-line progress animation (moving wave of blue dots).
    """
    # Initial delay to allow the primary status message (printed before this thread starts)
    # to be visible before the animation overwrites it.
    time.sleep(0.15)
    if message_length <= 0:
        message_length = 20
    dot_char = "●"
    empty_char = "○"
    direction = 1
    position = 0
    while not stop_event.is_set():
        progress_bar = [empty_char] * message_length
        if 0 <= position < message_length:
            progress_bar[position] = dot_char
        pattern = "".join(progress_bar)
        output_str = f"{COLORS['blue']}{pattern}{COLORS['reset']}" # Uses COLORS from this module
        sys.stdout.write(f"\r{output_str}")
        sys.stdout.flush()
        position += direction
        if position >= message_length -1 or position <= 0:
            direction *= -1
            position = max(0, min(position, message_length - 1))
        time.sleep(0.15)