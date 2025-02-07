#!/usr/bin/env python3
import sys
import os

# Debugging output to stderr
sys.stderr.write("Starting upload script\n")

# Read the request body from stdin
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
sys.stderr.write(f"Content length: {content_length}\n")
body = sys.stdin.read(content_length)
sys.stderr.write(f"Body read: {body}\n")

# Define a file where we store the received data
upload_dir = "cgi_uploads"
os.makedirs(upload_dir, exist_ok=True)  # Ensure the directory exists
file_path = os.path.join(upload_dir, "uploaded_data.txt")

# Save body to file
try:
    with open(file_path, "a") as f:
        f.write(body + "\n")  # Append new data with a newline
    sys.stderr.write(f"Data written to {file_path}\n")
except Exception as e:
    sys.stderr.write(f"Error writing to file: {e}\n")