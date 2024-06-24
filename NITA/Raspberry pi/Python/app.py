from flask import Flask, redirect, render_template_string, request, jsonify
import os
import ipaddress
import sqlite3
import requests

app = Flask(__name__)
app.secret_key = os.urandom(24)  # Needed for flash messages

ESP32_IP = 'http://192.168.1.139:80'  # Replace with the public IP or domain name
LOCAL_IP = 'http://192.168.1.139:80'  # Replace with the local IP and port of your Raspberry Pi

# Define your local network range(s)
local_networks = [
    ipaddress.IPv4Network('192.168.0.0/16'),
    ipaddress.IPv4Network('10.0.0.0/8'),
    # Add more networks if necessary
]

DATABASE_PATH = '/home/pi/licenta.db'

def get_client_ip():
    """
    Try to get the correct client IP address, considering reverse proxies.
    
    This function checks if the request comes from a reverse proxy (e.g., Nginx).
    If the request is from a reverse proxy, it gets the first IP address in the list of 
    IP addresses provided by the reverse proxy in the HTTP_X_FORWARDED_FOR header.
    If the request is not from a reverse proxy, it gets the direct remote address
    from the REMOTE_ADDR header. If neither of these headers are present, it falls back
    to Flask's provided method to get the client IP address.
    
    Returns:
        str: The client IP address.
    """
    # Check if the request comes from a reverse proxy
    if request.environ.get('HTTP_X_FORWARDED_FOR'):
        # If behind a proxy, get the first address in the list
        return request.environ['HTTP_X_FORWARDED_FOR'].split(',')[0]
    elif request.environ.get('REMOTE_ADDR'):
        # Otherwise, get the direct remote address
        return request.environ['REMOTE_ADDR']
    else:
        return request.remote_addr  # Fallback to Flask's provided method

def is_same_network(ip):
    """
    Check if the given IP address is within any of the defined local networks.
    
    Args:
        ip (str): The IP address to check.
        
    Returns:
        bool: True if the IP address is within any of the local networks, False otherwise.
    """
    try:
        # Convert the IP address to IPv4Address object
        user_ip = ipaddress.IPv4Address(ip)
        
        # Iterate over the defined local networks
        for network in local_networks:
            
            # Check if the IP address is within the network
            if user_ip in network:
                return True
    
    # If the IP address is invalid, ignore it and continue to the next iteration
    except ipaddress.AddressValueError:
        pass
    
    # If the IP address is not within any of the local networks, return False
    return False

@app.route('/')
def home():
    """Renders the home page with a menu and buttons for each POST endpoint."""
    menu = '''
    <div class="menu">
        <a href="#">Home</a>
        <a href="#">About</a>
        <a href="#">Contact</a>
    </div>
    '''

    button = '''
    <div class="button-container">
        <form action="/redirect_to_stream" method="post">
            <button type="submit" class="button">Start Video Stream</button>
        </form>
    </div>
    '''

    flash_message = '''
    <div class="flash-message">
        {% if message %}
        <p>{{ message }}</p>
        {% endif %}
    </div>
    '''

    form_receive_data = '''
    <div>
        <form action="/receive_data" method="post">
            <textarea name="data" rows="4" cols="50" placeholder="Enter data to send"></textarea><br>
            <button type="submit">Send Data</button>
        </form>
    </div>
    '''

    form_add_device = '''
    <div>
        <form action="/api/add_device" method="post">
            <input type="text" name="name" placeholder="Enter device name"><br>
            <input type="text" name="domain" placeholder="Enter device domain"><br>
            <button type="submit">Add Device</button>
        </form>
    </div>
    '''

    form_post_door_data = '''
    <div>
        <form action="/api/post_door_data" method="post">
            <button type="submit">Post Door Data</button>
        </form>
    </div>
    '''
    get_all_devices_data = '''
    <div>
        <form action="/api/get_all_devices_data" method="get">
            <button type="submit">Get All Devices Data</button>
        </form>
    </div>
    '''

    template = f'''
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP32 Camera Stream</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
            }}
            .menu {{
                margin-bottom: 20px;
            }}
            .menu a {{
                margin-right: 10px;
                text-decoration: none;
                color: #000;
            }}
            .button-container {{
                margin-top: 20px;
            }}
            .button {{
                padding: 10px 20px;
                font-size: 16px;
                background-color: #007BFF;
                color: #FFF;
                border: none;
                cursor: pointer;
            }}
            .button:hover {{
                background-color: #0056b3;
            }}
            .flash-message {{
                color: red;
            }}
            form {{
                margin-bottom: 20px;
            }}
        </style>
    </head>
    <body>
        {menu}
        {flash_message}
        {button}
        {form_receive_data}
        {form_add_device}
        {form_post_door_data}
        {get_all_devices_data}
    </body>
    </html>
    '''

    return render_template_string(template)


@app.route('/api/redirect_to_stream', methods=['POST'])
def redirect_to_stream():
    """
    Redirects the user to the appropriate stream based on their IP address.

    This function checks if the user is on the same network as the Raspberry Pi.
    If they are, it redirects them to the local IP address of the camera stream.
    Otherwise, it redirects them to the public IP address of the ESP32 running
    the camera stream.

    Returns:
        Response: The redirect response object.
    """
    # Get the user's IP address
    user_ip = get_client_ip()

    # Check if the user is on the same network
    if is_same_network(user_ip):
        # User is on the same network
        return redirect(LOCAL_IP)
    else:
        # User is not on the same network
        return redirect(ESP32_IP)

@app.route('/receive_data', methods=['POST'])
def receive_data():
    if request.method == 'POST':
        data = request.get_json()
        if not data or 'name' not in data or 'domain' not in data:
            return "Name and domain are required", 400

        name = data['name']
        domain = data['domain']

        conn = sqlite3.connect(DATABASE_PATH)
        cursor = conn.cursor()

        cursor.execute('''
            CREATE TABLE IF NOT EXISTS devices (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                domain TEXT NOT NULL
            )
        ''')

        cursor.execute('INSERT INTO devices (name, domain) VALUES (?, ?)', (name, domain))
        conn.commit()
        conn.close()

        return "Data received and added to database successfully", 201

@app.route('/api/post_door_data', methods=['POST'])
def post_door_data():
    return 'Hello, World!'

@app.route('/api/get_all_devices_data',methods=['GET'])
def get_all_devices_data():
    conn = sqlite3.connect(DATABASE_PATH)
    cursor = conn.cursor()
    cursor.execute('''SELECT * FROM DEVICES''')
    details = cursor.fetchall()
    conn.close()
    return jsonify(details)

@app.route('/api/add_device', methods=['POST'])
def add_device():
    name = request.form.get('name')
    domain = request.form.get('domain')

    if not name or not domain:
        return "Name and domain are required", 400

    conn = sqlite3.connect(DATABASE_PATH)
    cursor = conn.cursor()

    # Ensure the devices table exists
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS devices (
            id INTEGER PRIMARY KEY,
            name TEXT UNIQUE NOT NULL,
            domain TEXT NOT NULL
        )
    ''')

    # Check if the device already exists
    cursor.execute('SELECT * FROM devices WHERE name = ?', (name,))
    device = cursor.fetchone()

    if device:
        # Update the domain if the device exists
        cursor.execute('UPDATE devices SET domain = ? WHERE name = ?', (domain, name))
    else:
        # Insert a new device if it doesn't exist
        cursor.execute('INSERT INTO devices (name, domain) VALUES (?, ?)', (name, domain))

    conn.commit()
    conn.close()

    if device:
        return "Device updated successfully", 200
    else:
        return "Device added successfully", 201

@app.route('/api/get_devices_details', methods=['GET'])
def get_devices_details():
    conn = sqlite3.connect(DATABASE_PATH)
    cursor = conn.cursor()

    cursor.execute('SELECT domain FROM devices')
    devices = cursor.fetchall()
    conn.close()

    details = []

    for (domain,) in devices:
        try:
            response = requests.get(domain)
            details.append(response.text)
        except requests.RequestException as e:
            details.append(f"Error reaching {domain}: {str(e)}")

    return jsonify(details)

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000)
