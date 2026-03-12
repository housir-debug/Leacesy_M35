class ChannelMonitor {
    constructor() {
        this.ws = null;
        this.channels = new Map();
        this.autoRefresh = false;
        this.refreshInterval = 180; // 180ms
        this.intervalId = null;
        this.initWebSocket();
        this.loadChannels();
        this.setupEventListeners();
    }

    async loadChannels() {
        try {
            const response = await fetch('/api/channels');
            const data = await response.json();
            this.channels.clear();
            data.channels.forEach(ch => {
                this.channels.set(ch.channel, ch);
                this.renderChannels();
            });
        } catch (error) {
            console.error('Failed to load channels:', error);
        }
    }

    renderChannels() {
        const container = document.getElementById('channels-container');
        if (!container) return;

        let html = '';
        const sortedChannels = Array.from(this.channels.entries()).sort((a, b) => a[0] - b[0]);
        
        sortedChannels.forEach(([id, ch]) => {
            const isEnabled = ch.enabled;
            const cvSetpoint = ch.cvSetpoint;
            const ccSetpoint = ch.ccSetpoint;
            const ovSetpoint = ch.ovSetpoint;
            const currentMode = ch.status_v;
            const unit = ch.current_unit ?'mA':'A';

            html += `
                <div class="channel-detailed-card ${isEnabled ? 'enabled' : 'disabled'}" 
                     data-channel="${id}"
                     onclick="channelMonitor.toggleChannel(${id})"
                     oncontextmenu="channelMonitor.showDetails(${id}); return false">
                    
                    <div class="channel-header">
                        <h3>CH ${id}</h3>
                        <div class="channel-dot ${isEnabled ? 'enabled' : 'disabled'}"></div>
                    </div>
                    
                    <div class="measurement-card">
                        <div class="measurement-row">
                            <span class="measurement-value voltage">${ch.voltage.toFixed(4)} V</span>
                        </div>
                        <div class="measurement-row">
                            <span class="measurement-value current">${ch.current.toFixed(4)} ${unit}</span>
                        </div>
                    </div>

                    <div class="setpoints-grid">
                        <div class="setpoint-row ${currentMode === 'CV' ? 'active' : ''}">
                            <span class="setpoint-label">CV</span>
                            <span class="setpoint-value">${cvSetpoint.toFixed(3)} V</span>
                            <span class="mode-indicator ${currentMode === 'CV' ? 'active' : ''}"></span>
                        </div>
                        
                        <div class="setpoint-row ${currentMode === 'CC' ? 'active' : ''}">
                            <span class="setpoint-label">CC</span>
                            <span class="setpoint-value">${ccSetpoint.toFixed(3)} A</span>
                            <span class="mode-indicator ${currentMode === 'CC' ? 'active' : ''}"></span>
                        </div>
                        
                        <div class="setpoint-row ${currentMode === 'OV' ? 'active' : ''}">
                            <span class="setpoint-label">OV</span>
                            <span class="setpoint-value">${ovSetpoint.toFixed(3)} V</span>
                            <span class="mode-indicator ${currentMode === 'OV' ? 'active' : ''}"></span>
                        </div>
                    </div>
                </div>
            `;
        });
        
        container.innerHTML = html;
    }

    setupEventListeners() {
        const refreshBtn = document.getElementById('refresh-button');
        if (refreshBtn) {
            refreshBtn.onclick = () => {
                if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                    this.ws.send(JSON.stringify({
                        type: 'channels_update'
                    }));
                }
            }
        }

        const autoRefreshCheck = document.getElementById('auto-refresh');
        if (autoRefreshCheck) {
            autoRefreshCheck.onchange = () => {
                this.autoRefresh = autoRefreshCheck.checked;
                if (this.autoRefresh) {
                    this.intervalId = setInterval(() => {
                        if (this.autoRefresh && this.ws && this.ws.readyState === WebSocket.OPEN) {
                            this.ws.send(JSON.stringify({
                                type: 'channels_update'
                            }));
                        }
                    }, this.refreshInterval);
                }else {
                    if (this.intervalId) {
                        clearInterval(this.intervalId);
                    }
                }
            }
        }
    }

    initWebSocket() {
        const wsUrl = `ws://${window.location.hostname}:8080`;
        console.log('Connecting to WebSocket:', wsUrl);
        
        this.ws = new WebSocket(wsUrl);
        this.ws.onopen = () => {
            console.log('WebSocket connected');
            document.getElementById('connection-status').textContent = 'Device: Connected';
        };

        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if (data.type === 'channels_response') {
                this.channels.clear();
                data.channels.forEach(ch => {
                    this.channels.set(ch.channel, ch);
                    this.renderChannels();
                });
            }
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected, reconnecting...');
            document.getElementById('connection-status').textContent = 'Device: Reconnecting...';
        };
    }

    /*showDetails(channelId) {
        console.log('Show details for channel:', channelId);
        // 这里可以弹出模态框显示详细设置
        // 或者跳转到详情页面
    }

    toggleChannel(channelId) {
        const ch = this.channels.get(channelId);
        if (ch) {
            ch.enabled = !ch.enabled;
            if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                this.ws.send(JSON.stringify({
                    type: 'channel_control',
                    channel: channelId,
                    action: ch.enabled
                }));
            } 
            this.renderChannels();
        }
    }*/

    destroy() {
        if (this.intervalId) {
            clearInterval(this.intervalId);
        }
        if (this.ws) {
            this.ws.close();
        }
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.channelMonitor = new ChannelMonitor();
});