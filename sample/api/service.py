from http.server import HTTPServer, SimpleHTTPRequestHandler, test

class CORSRequestHandler(SimpleHTTPRequestHandler):
	def end_headers(self):
		self.send_header('Access-Control-Allow-Origin', '*')
		self.send_header('Cache-Control', 'no-cache')
		super().end_headers()

test(CORSRequestHandler, HTTPServer, port=8888)
