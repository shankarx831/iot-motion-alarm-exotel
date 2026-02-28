#!/bin/bash

# Configuration
export PORT=5001

# Twilio Configuration (Optional: fill these or use a .env file)
# export TWILIO_ACCOUNT_SID='your_sid_here'
# export TWILIO_AUTH_TOKEN='your_token_here'
# export TWILIO_PHONE_NUMBER='your_twilio_number'
# export OWNER_PHONE_NUMBER='your_phone_number'

echo "Starting IoT Alarm System Backend on port $PORT..."
echo "Access the dashboard at http://localhost:$PORT"

# Run the application
python3 app.py
