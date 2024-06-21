from flask import Flask, redirect, render_template_string, request
import os
import ipaddress

app = Flask(__name__)
app.secret_key = os.urandom(24)  # Needed for flash messages

ESP32_IP = 'https://lucky-winning-piranha.ngrok-free.app'  # Replace with the public IP or domain name
LOCAL_IP = 'http://192.168.1.139:80'  # Replace with the local IP and port of your Raspberry Pi

# Define your local network range(s)
local_networks = [
    ipaddress.IPv4Network('192.168.0.0/16'),
    ipaddress.IPv4Network('10.0.0.0/8'),
    # Add more networks if necessary
]

def get_client_ip():
    """Try to get the correct client IP address, considering reverse proxies."""
    if request.environ.get('HTTP_X_FORWARDED_FOR'):
        # If behind a proxy, get the first address in the list
        return request.environ['HTTP_X_FORWARDED_FOR'].split(',')[0]
    elif request.environ.get('REMOTE_ADDR'):
        # Otherwise, get the direct remote address
        return request.environ['REMOTE_ADDR']
    else:
        return request.remote_addr  # Fallback to Flask's provided method

def is_same_network(ip):
    try:
        user_ip = ipaddress.IPv4Address(ip)
        for network in local_networks:
            if user_ip in network:
                return True
    except ipaddress.AddressValueError:
        pass  # Handle invalid IP address gracefully
    return False

@app.route('/')
def index():
    return render_template_string('''
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP32 Camera Stream</title>
        <style>
            body {
                font-family: Arial, sans-serif;
            }
            .menu {
                margin-bottom: 20px;
            }
            .menu a {
                margin-right: 10px;
                text-decoration: none;
                color: #000;
            }
            .button-container {
                margin-top: 20px;
            }
            .button {
                padding: 10px 20px;
                font-size: 16px;
                background-color: #007BFF;
                color: #FFF;
                border: none;
                cursor: pointer;
            }
            .button:hover {
                background-color: #0056b3;
            }
            .flash-message {
                color: red;
            }
        </style>
    </head>
    <body>
        <div class="menu">
            <a href="#">Home</a>
            <a href="#">About</a>
            <a href="#">Contact</a>
        </div>
        <div class="flash-message">
            {% if message %}
            <p>{{ message }}</p>
            {% endif %}
        </div>
        <div class="button-container">
            <form action="/redirect_to_stream" method="post">
                <button type="submit" class="button">Start Video Stream</button>
            </form>
        </div>
    </body>
    </html>
    ''')

@app.route('/redirect_to_stream', methods=['POST'])
def redirect_to_stream():
    user_ip = get_client_ip()
    if is_same_network(user_ip):
        # User is on the same network
        return redirect(LOCAL_IP)
    else:
        # User is not on the same network
        return redirect(ESP32_IP)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
