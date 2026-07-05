import http.server
import socketserver
import subprocess
import json
import threading
import queue
import time
import os

PORT = 8000
DIRECTORY = "frontend"

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)
        
    def do_POST(self):
        if self.path == '/api/command':
            if not engine_manager.is_ready:
                self.send_error_response("Engine is still starting up...")
                return
                
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                data = json.loads(post_data.decode('utf-8'))
                cmd = data.get('command', '')
                
                # Send command to engine
                response = engine_manager.send_command(cmd)
                
                self.send_success_response({'response': response})
            except Exception as e:
                self.send_error_response(str(e))
        elif self.path == '/api/reset':
            try:
                content_length = int(self.headers.get('Content-Length', 0))
                post_data = self.rfile.read(content_length) if content_length else b'{}'
                data = json.loads(post_data.decode('utf-8')) if post_data else {}
                clean_files = data.get('clean', False)
                
                engine_manager.restart(clean_files)
                self.send_success_response({'status': 'restarted', 'clean': clean_files})
            except Exception as e:
                self.send_error_response(str(e))
        else:
            self.send_error(404, "Not Found")
            
    def do_GET(self):
        if self.path == '/api/status':
            self.send_success_response({'ready': engine_manager.is_ready})
        else:
            super().do_GET()

    def send_success_response(self, payload):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(payload).encode('utf-8'))

    def send_error_response(self, error_msg):
        self.send_response(500)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps({'error': error_msg}).encode('utf-8'))


class EngineManager:
    def __init__(self, exe_path):
        self.exe_path = exe_path
        self.process = None
        self.lock = threading.Lock()
        self.is_ready = False
        
    def start(self):
        # Start the C++ executable
        self.process = subprocess.Popen(
            [self.exe_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding='utf-8',
            errors='replace',
            bufsize=1, # Line buffered
            cwd=os.path.dirname(self.exe_path)
        )
        
        # Read the initial startup banner until the first prompt
        startup_log = self._read_until_prompt()
        self.is_ready = True
        print("\nEngine is ready!")
        
    def _read_until_prompt(self):
        # Read character by character until "AlgoExchange> "
        buffer = []
        prompt = "AlgoExchange> "
        prompt_len = len(prompt)
        
        while True:
            char = self.process.stdout.read(1)
            if not char:
                break
            buffer.append(char)
            # Check if the end of the buffer matches the prompt
            if len(buffer) >= prompt_len:
                if "".join(buffer[-prompt_len:]) == prompt:
                    # We hit the prompt, break out
                    return "".join(buffer[:-prompt_len])
        return "".join(buffer)
                    
    def send_command(self, cmd):
        with self.lock:
            # Write command
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()
            
            # Read output until next prompt
            response = self._read_until_prompt()
            return response.strip()
    
    def restart(self, clean_files=False):
        with self.lock:
            self.is_ready = False
            print("Restarting engine...")
            
            # Gracefully exit old process
            try:
                if clean_files:
                    self.process.stdin.write("DROP_EXIT\n")
                else:
                    self.process.stdin.write("EXIT\n")
                self.process.stdin.flush()
                self.process.wait(timeout=5)
            except:
                self.process.kill()
            
        # Start fresh (outside lock so _read_until_prompt can work)
        self.start()
        print("Engine restarted successfully!")

if __name__ == '__main__':
    # Ensure frontend directory exists
    if not os.path.exists(DIRECTORY):
        os.makedirs(DIRECTORY)

    exe = os.path.join(os.path.abspath(os.path.dirname(__file__)), "executable", "AlgoExchange.exe")
    if not os.path.exists(exe):
        print(f"Error: Executable not found at {exe}")
        exit(1)

    global engine_manager
    engine_manager = EngineManager(exe)
    
    # Start engine in a background thread so the HTTP server can start immediately
    print(f"Starting Engine: {exe} (in background)")
    threading.Thread(target=engine_manager.start, daemon=True).start()
    
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"Serving UI at http://localhost:{PORT}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")
            if engine_manager.is_ready:
                engine_manager.send_command("EXIT")
            httpd.server_close()
