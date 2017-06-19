# taken from http://www.piware.de/2011/01/creating-an-https-server-in-python/
# generate server.xml with the following command:
#    openssl req -new -x509 -keyout server.pem -out server.pem -days 365 -nodes
# run as follows:
#    python simple-https-server.py
# then in your browser, visit:
#    https://localhost:4443

import http.server
import socketserver
import ssl

PORT = 8443

Handler = http.server.SimpleHTTPRequestHandler

server = socketserver.TCPServer(("localhost", PORT), Handler)
server.socket = ssl.wrap_socket(server.socket, certfile='../server.pem', server_side=True)
with server:
    print("serving at port", PORT)
    server.serve_forever()
