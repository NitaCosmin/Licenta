#!/bin/bash

# Fetch the public IP address
PUBLIC_IP=$(curl -s http://checkip.amazonaws.com)

# Define the Nginx configuration file path
NGINX_CONFIG="/etc/nginx/nginx.conf"

# Backup the original configuration file
cp $NGINX_CONFIG $NGINX_CONFIG.bak

# Use sed to replace the server_name placeholder with the actual public IP address
sed -i "s/server_name  localhost;/server_name  $PUBLIC_IP;/" $NGINX_CONFIG

# Restart Nginx to apply the new configuration
sudo systemctl restart nginx


# Add this to crontab -e to run the script on reboot
# @reboot /path/to/your/script.sh
# 0 * * * * /path/to/your/script.sh

