#!/usr/bin/env python3
"""
Simple test server to replace postman-echo.com for unit tests.
This server listens on an unprivileged port and echoes back JSON data.
"""
import json
import socket
import sys
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

class TestServerHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Read the request body
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)

        try:
            # Parse the JSON data
            json_data = json.loads(post_data.decode('utf-8'))

            # Send response back
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()

            # Echo back the original JSON
            response_data = json.dumps(json_data)
            self.wfile.write(response_data.encode('utf-8'))

        except json.JSONDecodeError as e:
            # Return error if JSON is invalid
            self.send_response(400)
            self.send_header('Content-type', 'application/json')
            self.end_headers()

            error_response = {"error": f"Invalid JSON: {str(e)}"}
            self.wfile.write(json.dumps(error_response).encode('utf-8'))

    def log_message(self, format, *args):
        # Suppress log messages
        pass

def start_server(port=8080):
    """Start the test server on specified port"""
    server = HTTPServer(('localhost', port), TestServerHandler)
    print(f"Test server started on port {port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("Server stopped")
        sys.exit(0)

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    start_server(port)
