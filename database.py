from models import db, AlarmStatus, MotionLog
from datetime import datetime, timedelta
import random

def init_db(app):
    """Initialize database with app context"""
    with app.app_context():
        db.create_all()
        seed_sample_data()

def seed_sample_data():
    """Add sample motion logs for demo purposes"""
    # Check if data already exists
    if MotionLog.query.first() is None:
        # Create sample logs for the past 7 days
        sample_logs = []
        statuses = ['Motion Detected', 'Motion Detected', 'Motion Detected', 
                   'False Alarm', 'Motion Detected']
        
        for i in range(15):
            random_days = random.randint(0, 7)
            random_hours = random.randint(0, 23)
            random_minutes = random.randint(0, 59)
            timestamp = datetime.utcnow() - timedelta(
                days=random_days, 
                hours=random_hours, 
                minutes=random_minutes
            )
            sample_logs.append(MotionLog(
                timestamp=timestamp,
                status=random.choice(statuses),
                call_made=random.choice([True, False])
            ))
        
        db.session.add_all(sample_logs)
        
        # Initialize alarm status (default: OFF)
        if AlarmStatus.query.first() is None:
            alarm_status = AlarmStatus(is_active=False)
            db.session.add(alarm_status)
        
        db.session.commit()
        print("Sample data initialized successfully!")
