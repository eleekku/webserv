#!/usr/bin/env python3
import sys
import os


# Read the request body from stdin
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length)

# Define a file where we store the received data
upload_dir = "cgi-bin/cgi_uploads"
os.makedirs(upload_dir, exist_ok=True)  # Ensure the directory exists
file_path = os.path.join(upload_dir, "uploaded_data.txt")

# Save body to file
with open(file_path, "a") as f:
    f.write(body + "\n")  # Append new data with a newline