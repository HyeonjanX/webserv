#!/usr/bin/env python

import sys
import os

# Check the REQUEST_METHOD environment variable
request_method = os.environ.get("REQUEST_METHOD", "")

# Only read from stdin if the request method is POST
if request_method == "POST":
    input_data = sys.stdin.read()
else:
    input_data = ""

# Print headers
print("Content-Type: text/plain")
print("Content-Length: {}".format(len(input_data)))
print("")

# Print the read data
print(input_data)