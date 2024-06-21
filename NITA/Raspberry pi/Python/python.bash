# Install Python and Flask
# If you don't have Python and Flask installed, you can install them as follows:

sudo apt update
sudo apt install python3 python3-pip -y
pip3 install Flask


# First, create a new service file. You can do this with the following command:

sudo nano /etc/systemd/system/flask_app.service

# Define the service configuration:

[Unit]
Description=A simple Flask web application
After=network.target

[Service]
ExecStart=/usr/bin/python3 /path/to/your/app.py
WorkingDirectory=/path/to/your
User=pi
Environment="PATH=/usr/bin"
Restart=always

[Install]
WantedBy=multi-user.target


# Enable the service to start on boot:

sudo systemctl enable flask_app.service


# Start the service immediately:

sudo systemctl start flask_app.service


# Check the status of the service:

sudo systemctl status flask_app.service

