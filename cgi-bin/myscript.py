#!/usr/bin/env python3

import os
import sys
from urllib.parse import parse_qs

def main():

    # Get the query string from the environment
    query_string = os.environ.get("QUERY_STRING", "")
    
    # Parse the query string
    query_params = parse_qs(query_string)

    # Generate a response based on the query string
    if query_params:
        response = "You sent the following query parameters:\n"
        for key, values in query_params.items():
            response += f"{key}: {', '.join(values)}\n"
    else:
        response = "No query parameters received.\n"

    # Output the response
    print(response)

if __name__ == "__main__":
    main()