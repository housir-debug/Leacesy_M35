class ChannelMonitor {
    constructor() {
        this.ws = null;
        this.channels = new Map();
        this.autoRefresh = true;
        this.initWebSocket();
        this.loadChannels();
        this.setupEventListeners();
    }

    initWebSocket() {
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
            if (this.autoRefresh) {
                setTimeout(() => this.initWebSocket(), 3000);
            }
        };
    }

    handleWebSocketMessage(data) {
        if (data.type === 'channel_update') {
            this.updateChannel(data);
        } else if (data.type === 'initial_data' && data.channels) {
            data.channels.forEach(ch => this.updateChannel(ch));
        }
    }

    async loadChannels() {
        try {
            const response = await fetch('/api/channels');
            const data = await response.json();
            data.channels.forEach(ch => this.updateChannel(ch));
            this.updateTimestamp(data.timestamp);
        } catch (error) {
            console.error('Failed to load channels:', error);
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
        // 按通道号排序
        const sortedChannels = Array.from(this.channels.entries()).sort((a, b) => a[0] - b[0]);
        
        sortedChannels.forEach(([id, ch]) => {
            const statusClass = this.getStatusClass(ch.status);
            html += `
                <div class="channel-detailed-card">
                    <div class="channel-header">
                        <h3>Channel ${id}</h3>
                        <span class="channel-status ${statusClass}">${ch.status}</span>
                    </div>
                    <div class="channel-measurements">
                        <div class="measurement-group">
                            <div class="measurement-label">Voltage</div>
                            <div class="measurement-value voltage">${ch.voltage.toFixed(3)} V</div>
                            <div class="measurement-bar">
                                <div class="bar-fill" style="width: ${this.calculatePercentage(ch.voltage, 0, 30)}%"></div>
                            </div>
                        </div>
                        <div class="measurement-group">
                            <div class="measurement-label">Current</div>
                            <div class="measurement-value current">${ch.current.toFixed(3)} A</div>
                            <div class="measurement-bar">
                                <div class="bar-fill" style="width: ${this.calculatePercentage(ch.current, 0, 5)}%"></div>
                            </div>
                        </div>
                    </div>
                    <div class="channel-footer">
                        <span class="power">Power: ${(ch.voltage * ch.current).toFixed(3)} W</span>
                        <button class="channel-control" onclick="channelMonitor.controlChannel(${id}, 'toggle')">
                            Toggle
                        </button>
                    </div>
                </div>
            `;
        });
        
        container.innerHTML = html;
    }

    getStatusClass(status) {
        switch(status.toLowerCase()) {
            case 'normal': return 'status-normal';
            case 'warning': return 'status-warning';
            case 'alert': return 'status-alert';
            default: return 'status-normal';
        }
    }

    calculatePercentage(value, min, max) {
        return Math.min(100, Math.max(0, ((value - min) / (max - min)) * 100));
    }

    updateTimestamp(timestamp) {
        const tsElement = document.getElementById('timestamp');
        if (tsElement && timestamp) {
            const date = new Date(timestamp);
            tsElement.textContent = date.toLocaleString();
        }
    }

    async refresh() {
        await this.loadChannels();
    }

    toggleAutoRefresh() {
        this.autoRefresh = document.getElementById('auto-refresh').checked;
        if (this.autoRefresh && this.ws?.readyState !== WebSocket.OPEN) {
            this.initWebSocket();
        }
    }

    controlChannel(channel, action) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({
                type: 'channel_control',
                channel: channel,
                action: action
            }));
        } else {
            console.error('WebSocket not connected');
        }
    }

    setupEventListeners() {
        const refreshBtn = document.getElementById('refresh-button');
        if (refreshBtn) {
            refreshBtn.onclick = () => this.refresh();
        }

        const autoRefreshCheck = document.getElementById('auto-refresh');
        if (autoRefreshCheck) {
            autoRefreshCheck.onchange = () => this.toggleAutoRefresh();
        }
    }
}

// 页面加载后初始化
document.addEventListener('DOMContentLoaded', () => {
    window.channelMonitor = new ChannelMonitor();
});