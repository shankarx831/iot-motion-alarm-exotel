import os
import requests
from datetime import datetime, timedelta
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
from dotenv import load_dotenv
from models import db, AlarmStatus, MotionLog
from database import init_db

# Load environment variables
load_dotenv()

# Initialize Flask app
app = Flask(__name__)

# Configuration
app.config['SQLALCHEMY_DATABASE_URI'] = os.environ.get(
    'DATABASE_URL', 
    'sqlite:///alarm.db'
).replace('postgres://', 'postgresql://')  # Heroku fix
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

# Initialize extensions
db.init_app(app)
CORS(app)  # Allow requests from GitHub Pages

# Exotel Configuration
EXOTEL_API_KEY = os.environ.get('EXOTEL_API_KEY')
EXOTEL_API_TOKEN = os.environ.get('EXOTEL_API_TOKEN')
EXOTEL_ACCOUNT_SID = os.environ.get('EXOTEL_ACCOUNT_SID')
EXOTEL_SUBDOMAIN = os.environ.get('EXOTEL_SUBDOMAIN', 'api.exotel.com')
EXOTEL_VIRTUAL_NUMBER = os.environ.get('EXOTEL_VIRTUAL_NUMBER')  # Your Exophone
EXOTEL_APP_ID = os.environ.get('EXOTEL_APP_ID') # ID of your Exotel Flow to play message
OWNER_PHONE_NUMBER = os.environ.get('OWNER_PHONE_NUMBER')
OWNER_PHONE_NUMBER_2 = os.environ.get('OWNER_PHONE_NUMBER_2')

# Rate limiting: Track last call time
last_call_time = None
CALL_COOLDOWN_SECONDS = 60

# Hardcoded credentials for demo (use environment variables in production)
ADMIN_USERNAME = "admin"
ADMIN_PASSWORD = "password123"


def check_auth(username, password):
    """Simple authentication check"""
    return username == ADMIN_USERNAME and password == ADMIN_PASSWORD


def make_phone_call(phone_number=None):
    """Make a phone call using Exotel with rate limiting"""
    global last_call_time
    
    # Use default owner number if none provided
    if not phone_number:
        phone_number = OWNER_PHONE_NUMBER
    
    # Rate limiting check
    if last_call_time:
        time_since_last = datetime.utcnow() - last_call_time
        if time_since_last.total_seconds() < CALL_COOLDOWN_SECONDS:
            remaining = CALL_COOLDOWN_SECONDS - time_since_last.total_seconds()
            print(f"Rate limit: {remaining:.0f}s remaining")
            return False, f"Rate limited. Try again in {remaining:.0f} seconds"
    
    try:
        # Exotel Connect Call API
        url = f"https://{EXOTEL_API_KEY}:{EXOTEL_API_TOKEN}@{EXOTEL_SUBDOMAIN}/v1/Accounts/{EXOTEL_ACCOUNT_SID}/Calls/connect.json"
        
        exotel_app_url = f"https://my.exotel.com/{EXOTEL_ACCOUNT_SID}/exoml/start_voice/{EXOTEL_APP_ID}"
        
        payload = {
            'From': phone_number,
            'CallerId': EXOTEL_VIRTUAL_NUMBER,
            'Url': exotel_app_url,
            'CallType': 'trans'
        }
        
        response = requests.post(url, data=payload)
        result = response.json()
        print(f"Exotel response JSON: {result}")
        
        if response.status_code == 200:
            last_call_time = datetime.utcnow()
            call_sid = result.get('Call', {}).get('Sid', 'Unknown')
            print(f"Call initiated: {call_sid}")
            return True, f"Call initiated successfully. SID: {call_sid}"
        else:
            error_msg = result.get('RestException', {}).get('Message', 'Exotel API failed')
            print(f"Exotel error: {error_msg}")
            return False, f"Failed to make call: {error_msg}"
            
    except Exception as e:
        print(f"Exotel request error: {str(e)}")
        return False, f"Failed to make call: {str(e)}"


# ==================== API ROUTES ====================

@app.route('/api/login', methods=['POST'])
def login():
    """Authenticate user and return success/failure"""
    data = request.get_json()
    username = data.get('username', '')
    password = data.get('password', '')
    
    if check_auth(username, password):
        return jsonify({
            'success': True, 
            'message': 'Login successful'
        })
    return jsonify({
        'success': False, 
        'message': 'Invalid credentials'
    }), 401


@app.route('/api/alarm/status', methods=['GET'])
def get_alarm_status():
    """Get current alarm status"""
    alarm = AlarmStatus.query.first()
    if alarm:
        return jsonify({
            'success': True,
            'data': alarm.to_dict()
        })
    return jsonify({
        'success': True,
        'data': {'is_active': False}
    })


@app.route('/api/alarm/toggle', methods=['POST'])
def toggle_alarm():
    """Toggle alarm ON/OFF"""
    data = request.get_json()
    is_active = data.get('is_active', False)
    
    alarm = AlarmStatus.query.first()
    if not alarm:
        alarm = AlarmStatus(is_active=is_active)
        db.session.add(alarm)
    else:
        alarm.is_active = is_active
    
    db.session.commit()
    
    status_text = "activated" if is_active else "deactivated"
    return jsonify({
        'success': True,
        'message': f'Alarm {status_text}',
        'data': alarm.to_dict()
    })


@app.route('/api/motion/logs', methods=['GET'])
def get_motion_logs():
    """Get all motion logs, ordered by most recent first"""
    logs = MotionLog.query.order_by(MotionLog.timestamp.desc()).limit(100).all()
    return jsonify({
        'success': True,
        'data': [log.to_dict() for log in logs]
    })


@app.route('/api/motion/detect', methods=['POST'])
def motion_detected():
    """
    Endpoint for ESP32 to report motion detection (Primary Number).
    """
    return process_motion_detection(OWNER_PHONE_NUMBER, "Motion Detected")


@app.route('/api/motion/detect2', methods=['POST'])
def motion_detected_2():
    """
    Endpoint for ESP32 to report motion detection (Secondary Number).
    """
    return process_motion_detection(OWNER_PHONE_NUMBER_2, "Motion Detected (Secondary)")


def process_motion_detection(phone_number, log_status):
    """Genetic processor for motion detection"""
    # Get current alarm status
    alarm = AlarmStatus.query.first()
    
    # Check if alarm is active
    if not alarm or not alarm.is_active:
        # Log motion but don't trigger call
        log = MotionLog(status=f'{log_status} (Alarm OFF)')
        db.session.add(log)
        db.session.commit()
        return jsonify({
            'success': True,
            'message': 'Motion logged, alarm is OFF',
            'call_triggered': False
        })
    
    # Alarm is ON - make phone call
    call_success, call_message = make_phone_call(phone_number)
    
    # Log the motion event
    log = MotionLog(
        status=log_status,
        call_made=call_success
    )
    db.session.add(log)
    db.session.commit()
    
    return jsonify({
        'success': True,
        'message': call_message,
        'call_triggered': call_success,
        'log_id': log.id
    })


@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint for monitoring"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.utcnow().isoformat()
    })


# ==================== STATIC FILE ROUTES ====================

@app.route('/')
def serve_index():
    """Serve the main index.html file"""
    return send_from_directory('.', 'index.html')


@app.route('/<path:path>')
def serve_static(path):
    """Serve other static files like CSS and JS"""
    return send_from_directory('.', path)


# ==================== INITIALIZATION ====================

# Initialize database (outside main block so it runs under Gunicorn)
init_db(app)

if __name__ == '__main__':
    # Run development server
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port, debug=True)
