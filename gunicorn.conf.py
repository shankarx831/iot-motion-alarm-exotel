import os

# Get port from environment variable (Render sets this)
port = os.environ.get("PORT", "10000")
bind = f"0.0.0.0:{port}"

# Log configuration for verification
print(f"Gunicorn binding to: {bind}")

# Standard performance settings
workers = 1 
threads = 2
timeout = 120
