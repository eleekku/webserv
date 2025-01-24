#!/usr/bin/env python3

import time

# Output required headers to avoid a malformed response
#print("Content-Type: text/plain")
print()

# Start the infinite loop
i = 0
while True:
    print(f"Iteration {i}", flush=True)
    i += 1
    time.sleep(1)  # Add a small delay to avoid consuming 100% CPU