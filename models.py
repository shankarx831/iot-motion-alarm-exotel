from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

db = SQLAlchemy()

class AlarmStatus(db.Model):
    """
    Stores the current alarm status (ON/OFF).
    Uses a single row approach for simplicity.
    """
    id = db.Column(db.Integer, primary_key=True)
    is_active = db.Column(db.Boolean, default=False, nullable=False)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    def to_dict(self):
        return {
            'id': self.id,
            'is_active': self.is_active,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None
        }

class MotionLog(db.Model):
    """
    Stores all motion detection events.
    Each row represents a single motion detection event.
    """
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow, nullable=False)
    status = db.Column(db.String(50), default='Motion Detected', nullable=False)
    call_made = db.Column(db.Boolean, default=False)
    
    def to_dict(self):
        return {
            'id': self.id,
            'timestamp': self.timestamp.isoformat() if self.timestamp else None,
            'status': self.status,
            'call_made': self.call_made
        }
