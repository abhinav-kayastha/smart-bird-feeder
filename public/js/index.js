async function fetchLatestState() {
    try {
        const response = await fetch('/latest-state');
        const data = await response.json();
        document.getElementById('device-state').textContent = `State: ${data.state}`;
        document.getElementById('battery-status').textContent = `Battery: ${data.battery}%`;
    } catch (error) {
        console.error('Error fetching latest state:', error);
        document.getElementById('device-state').textContent = 'Error fetching state';
        document.getElementById('battery-status').textContent = 'Error fetching battery status';
    }
}

async function sendCommand(data) {
    try {
        const response = await fetch('/send-command', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ data })
        });
        const result = await response.json();
        alert(result.message);
    } catch (error) {
        console.error('Error sending command:', error);
        alert('Error sending command');
    }
}

// Fetch the latest state when the page loads
window.onload = fetchLatestState;

// WebSocket connection to receive real-time updates
const socket = new WebSocket('ws://' + window.location.host);

socket.onmessage = function(event) {
    const message = JSON.parse(event.data);
    if (message.type === 'new-data') {
        fetchLatestState();
    }
};