class InstrumentApp {
    constructor() {
        this.ws = null;
        this.channels = new Map();
        this.initWebSocket();
        this.loadDeviceInfo();
    }

    initWebSocket() {
        // 关键：连接WebSocket服务器
        const wsUrl = `ws://${window.location.hostname}:8080`;
        console.log('Connecting to WebSocket:', wsUrl);
        
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            console.log('WebSocket connected');
            document.getElementById('connection-status').textContent = 'WebSocket: Connected';
        };

        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            this.handleWebSocketMessage(data);
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected, reconnecting...');
            document.getElementById('connection-status').textContent = 'WebSocket: Reconnecting...';
            setTimeout(() => this.initWebSocket(), 3000);
        };
    }

    handleWebSocketMessage(data) {
        if (data.type === 'channel_update') {
            this.updateChannel(data);
        } else if (data.type === 'initial_data' && data.channels) {
            data.channels.forEach(ch => this.updateChannel(ch));
        }
    }

    updateChannel(data) {
        this.channels.set(data.channel, data);
        this.renderChannels();
    }

    renderChannels() {
        const container = document.getElementById('channels-container');
        if (!container) return;

        let html = '';
        this.channels.forEach((ch, id) => {
            html += `
                <div class="channel-card">
                    <h3>Channel ${id}</h3>
                    <div class="voltage">${ch.voltage.toFixed(3)} V</div>
                    <div class="current">${ch.current.toFixed(3)} A</div>
                    <div class="status-${ch.status}">${ch.status}</div>
                </div>
            `;
        });
        container.innerHTML = html;
    }

    async loadDeviceInfo() {
        try {
            const response = await fetch('/api/device/info');
            const data = await response.json();
            document.getElementById('model').textContent = data.model || 'N/A';
            document.getElementById('serial').textContent = data.serialNumber || 'N/A';
            document.getElementById('firmware').textContent = data.firmwareVersion || 'N/A';
            document.getElementById('uptime').textContent = data.uptime || 'N/A';
        } catch (error) {
            console.error('Failed to load device info:', error);
        }
    }
}

// 页面加载后初始化
document.addEventListener('DOMContentLoaded', () => {
    window.app = new InstrumentApp();
});