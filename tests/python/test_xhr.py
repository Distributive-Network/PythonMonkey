from http.server import HTTPServer, BaseHTTPRequestHandler
import pythonmonkey as pm
import threading
import asyncio
import json

def test_xhr():
  class TestHTTPRequestHandler(BaseHTTPRequestHandler):
    def log_request(self, *args) -> None:
      return
    
    def do_GET(self):
      self.send_response(200)
      self.end_headers()
      self.wfile.write(b"get response")
    
    def do_POST(self):
      self.send_response(200)
      self.send_header('Content-Type', 'application/json')
      self.end_headers()
      length = int(self.headers.get('Content-Length'))
      json_string = self.rfile.read(length).decode("utf-8")
      parameter_dict = json.loads(json_string)
      parameter_dict["User-Agent"] = self.headers['User-Agent']
      data = json.dumps(parameter_dict).encode("utf-8")
      self.wfile.write(data)
    
  httpd = HTTPServer(('localhost', 4001), TestHTTPRequestHandler)
  thread = threading.Thread(target = httpd.serve_forever)
  thread.daemon = True
  thread.start()
  
  async def async_fn():
    assert "get response" == await pm.eval("""
      new Promise(function (resolve, reject) {
        let xhr = new XMLHttpRequest();
        xhr.open('GET', 'http://localhost:4001');
          
        xhr.onload = function ()
        {
          if (this.status >= 200 && this.status < 300)
          {
            resolve(this.response);
          }
          else
          {
            reject(new Error(JSON.stringify({
              status: this.status,
              statusText: this.statusText
            })));
          }
        };
        
        xhr.onerror = function (ev)
        {
          reject(ev.error);
        };
        xhr.send();
      });
      """)

    post_result = await pm.eval("""
      new Promise(function (resolve, reject) 
      {
        let xhr = new XMLHttpRequest();
        xhr.open('POST', 'http://localhost:4001');

        xhr.onload = function () 
        {
          if (this.status >= 200 && this.status < 300) 
          {
            resolve(this.response);
          }
          else 
          {
            reject(new Error(JSON.stringify({
              status: this.status,
              statusText: this.statusText
            })));
          }
        };

        xhr.onerror = function (ev) 
        {
          console.log(ev)
          reject(ev.error);
        };

        xhr.send(JSON.stringify({fromPM: "snakesandmonkeys"}));
      })
      """)
    
    result_json = json.loads(post_result)
    assert result_json["fromPM"] == "snakesandmonkeys"
    assert result_json["User-Agent"].startswith("Python/")
    httpd.shutdown()
  asyncio.run(async_fn())