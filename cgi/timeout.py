#!/usr/bin/env python

import sys
import os
import time

time.sleep(100)

request_method = os.environ.get("REQUEST_METHOD", "")

if request_method == "POST":
    input_data = ""
    while True:
        chunk = sys.stdin.read(1024) 
        if not chunk:
            break
        input_data += chunk

else:
    input_data = "............    조용하다     ............"

print("Content-Type: text/plain\r\n", end="")
print("Status: 200\r\n", end="")
print("Content-Length: {}\r\n\r\n".format(len(input_data)), end="")

print(input_data, end="")
