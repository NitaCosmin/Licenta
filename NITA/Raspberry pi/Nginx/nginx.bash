#NGINX Commands Documentation
#Basic Commands
# Start NGINX
# To start the NGINX service:

sudo systemctl start nginx


# Stop NGINX
# To stop the NGINX service:

sudo systemctl stop nginx


# Restart NGINX
# To restart the NGINX service (this stops and then starts the service):

sudo systemctl restart nginx


# Reload NGINX
# To reload the NGINX configuration without stopping the service:

sudo systemctl reload nginx


# Enable NGINX
# To enable NGINX to start at boot:

sudo systemctl enable nginx


# Disable NGINX
# To disable NGINX from starting at boot:

sudo systemctl disable nginx

# File the configuration

sudo nano /etc/nginx/sites-enabled/default


# Configuration Testing
# Test Configuration
# To test the NGINX configuration for syntax errors:

sudo nginx -t


# Test Configuration with Specific File
# To test a specific NGINX configuration file:

sudo nginx -t -c /path/to/nginx.conf


# Logs
# Access Logs
# Default location of access logs:

/var/log/nginx/access.log

sudo tail -f /var/log/nginx/error.log


# Error Logs
# Default location of error logs:

/var/log/nginx/error.log


# Advanced Commands
# Check NGINX Status
# To check the status of the NGINX service:

sudo systemctl status nginx


# View NGINX Version
# To display the version of NGINX:

nginx -v


# To display the compiled configuration parameters:

nginx -V

#Compile the configuration parameters

sudo ln -s /etc/nginx/sites-enabled/default etc/nginx/sites-enabled


# Remun the configuration parameters

sudo rm /etc/nginx/sites-enabled/default 


#Install a Web Server

sudo apt install nginx


#Install FFmpeg

sudo apt install ffmpeg