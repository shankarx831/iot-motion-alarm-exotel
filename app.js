// ==================== Configuration ====================
const API_BASE_URL = '/api';

// ==================== DOM Elements ====================
const loginPage = document.getElementById('loginPage');
const dashboardPage = document.getElementById('dashboardPage');
const loginForm = document.getElementById('loginForm');
const loginError = document.getElementById('loginError');
const logoutBtn = document.getElementById('logoutBtn');
const alarmToggle = document.getElementById('alarmToggle');
const toggleLabel = document.getElementById('toggleLabel');
const alarmStatus = document.getElementById('alarmStatus');
const statusMessage = document.getElementById('statusMessage');
const motionLogs = document.getElementById('motionLogs');
const refreshBtn = document.getElementById('refreshBtn');
const totalMotion = document.getElementById('totalMotion');
const callsMade = document.getElementById('callsMade');
const lastEvent = document.getElementById('lastEvent');

// ==================== Authentication ====================

loginForm.addEventListener('submit', async (e) => {
    e.preventDefault();

    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const response = await fetch(`${API_BASE_URL}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });

        const data = await response.json();

        if (data.success) {
            sessionStorage.setItem('isLoggedIn', 'true');
            showDashboard();
        } else {
            loginError.textContent = data.message;
        }
    } catch (error) {
        loginError.textContent = 'Connection error. Please try again.';
        console.error('Login error:', error);
    }
});

logoutBtn.addEventListener('click', () => {
    sessionStorage.removeItem('isLoggedIn');
    showLogin();
});

function showLogin() {
    loginPage.classList.add('active');
    dashboardPage.classList.remove('active');
    loginForm.reset();
    loginError.textContent = '';
}

function showDashboard() {
    loginPage.classList.remove('active');
    dashboardPage.classList.add('active');
    loadAlarmStatus();
    loadMotionLogs();
}

// ==================== Alarm Control ====================

alarmToggle.addEventListener('change', async (e) => {
    const isActive = e.target.checked;

    try {
        const response = await fetch(`${API_BASE_URL}/alarm/toggle`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ is_active: isActive })
        });

        const data = await response.json();

        if (data.success) {
            updateAlarmUI(isActive);
        }
    } catch (error) {
        console.error('Toggle error:', error);
        e.target.checked = !isActive; // Revert on error
    }
});

async function loadAlarmStatus() {
    try {
        const response = await fetch(`${API_BASE_URL}/alarm/status`);
        const data = await response.json();

        if (data.success) {
            alarmToggle.checked = data.data.is_active;
            updateAlarmUI(data.data.is_active);
        }
    } catch (error) {
        console.error('Load status error:', error);
    }
}

function updateAlarmUI(isActive) {
    if (isActive) {
        alarmStatus.textContent = 'ACTIVE';
        alarmStatus.classList.remove('inactive');
        alarmStatus.classList.add('active');
        statusMessage.textContent = 'Alarm is monitoring for motion';
        toggleLabel.textContent = 'Turn Alarm OFF';
    } else {
        alarmStatus.textContent = 'INACTIVE';
        alarmStatus.classList.remove('active');
        alarmStatus.classList.add('inactive');
        statusMessage.textContent = 'Alarm is currently turned off';
        toggleLabel.textContent = 'Turn Alarm ON';
    }
}

// ==================== Motion Logs ====================

async function loadMotionLogs() {
    try {
        const response = await fetch(`${API_BASE_URL}/motion/logs`);
        const data = await response.json();

        if (data.success) {
            displayLogs(data.data);
            updateStats(data.data);
        }
    } catch (error) {
        console.error('Load logs error:', error);
        motionLogs.innerHTML = '<p class="error">Failed to load logs</p>';
    }
}

function displayLogs(logs) {
    if (logs.length === 0) {
        motionLogs.innerHTML = '<p class="loading">No motion events recorded</p>';
        return;
    }

    motionLogs.innerHTML = logs.map(log => `
        <div class="log-item">
            <div class="log-status">
                <span class="log-indicator ${log.call_made ? 'call' : ''}"></span>
                <span>${log.status}</span>
            </div>
            <span class="log-time">${formatTimestamp(log.timestamp)}</span>
        </div>
    `).join('');
}

function formatTimestamp(isoString) {
    const date = new Date(isoString);
    return date.toLocaleString('en-US', {
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit'
    });
}

function updateStats(logs) {
    totalMotion.textContent = logs.length;
    callsMade.textContent = logs.filter(log => log.call_made).length;

    if (logs.length > 0) {
        const latest = new Date(logs[0].timestamp);
        const now = new Date();
        const diffMs = now - latest;
        const diffMins = Math.floor(diffMs / 60000);

        if (diffMins < 1) {
            lastEvent.textContent = 'Just now';
        } else if (diffMins < 60) {
            lastEvent.textContent = `${diffMins}m ago`;
        } else {
            const diffHours = Math.floor(diffMins / 60);
            lastEvent.textContent = `${diffHours}h ago`;
        }
    }
}

refreshBtn.addEventListener('click', () => {
    loadMotionLogs();
    loadAlarmStatus();
});

// ==================== Initialize ====================

// Check if already logged in
if (sessionStorage.getItem('isLoggedIn') === 'true') {
    showDashboard();
} else {
    showLogin();
}

// Auto-refresh logs every 30 seconds
setInterval(() => {
    if (sessionStorage.getItem('isLoggedIn') === 'true') {
        loadMotionLogs();
        loadAlarmStatus();
    }
}, 30000);
