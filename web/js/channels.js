class ChannelMonitor {
    constructor() {
        this.ws = null;
        this.channels = new Map();
        //this.channelHistory = new Map();
        //this.maxHistoryPoints = 30;
        this.autoRefresh = false;
        this.refreshInterval = 180;
        this.intervalId = null;
        this.loadChannels();
        this.initWebSocket();
        this.setupEventListeners();
    }

    async loadChannels() {
        try {
            const response = await fetch('/api/channels')
            const data = await response.json();
            this.channels.clear();
            data.channels.forEach(ch => {
                this.channels.set(ch.channel, ch);
                /*if (!this.channelHistory.has(ch.channel)) {
                    this.channelHistory.set(ch.channel, {
                        voltage: [],
                        current: []
                    });
                }*/
            });
            this.renderChannels();
        } catch (error) {
            console.error('Failed to load channels:', error);
        }
    }

    renderChannels() {
        const container = document.getElementById('channels-container');
        if (!container) return;
        let html = '';
        // 排序 channels Map，确保按照 channel ID 顺序渲染
        const sortedChannels = Array.from(this.channels.entries()).sort((a, b) => a[0] - b[0]);
        
        sortedChannels.forEach(([id, ch]) => {
            //this.updateHistory(id, ch.voltage, ch.current);
            // 防止 current_unit 或 status 字段异常导致的错误，添加默认值和长度检查
            const isUnitNormal = ch.current_unit && (ch.current_unit === "mA" || ch.current_unit === "A");
            const unit = isUnitNormal ? ch.current_unit : "A";
            const isStatusNormal = ch.status && ch.status.length === 16;
            const cvMode = isStatusNormal ? ch.status.charAt(14) === "1" : false;
            const ccMode = isStatusNormal ? ch.status.charAt(13) === "1" : false;
            const ovpMode = isStatusNormal ? ch.status.charAt(11) === "1" : false;

            html += `
                <div class="channel-detailed-card ${ch.isOutput ? 'enabled' : 'disabled'}"
                     data-channel="${id}"
                     onclick="channelMonitor.toggleChannel(${id})"
                     oncontextmenu="channelMonitor.showDetails(${id}); return false">
                    
                    <div class="channel-header">
                        <h3>CH ${id}</h3>
                        <div class="channel-dot ${ch.isOutput ? 'enabled' : 'disabled'}"></div>
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
                        <div class="setpoint-row ${cvMode ? 'active' : ''}">
                            <span class="setpoint-label">CV</span>
                            <span class="setpoint-value">${ch.cvSetpoint.toFixed(3)} V</span>
                            <span class="mode-indicator ${cvMode ? 'active' : ''}"></span>
                        </div>
                        
                        <div class="setpoint-row ${ccMode ? 'active' : ''}">
                            <span class="setpoint-label">CC</span>
                            <span class="setpoint-value">${ch.ccSetpoint.toFixed(3)} A</span>
                            <span class="mode-indicator ${ccMode ? 'active' : ''}"></span>
                        </div>
                        
                        <div class="setpoint-row ${ovpMode ? 'active' : ''}">
                            <span class="setpoint-label">OV</span>
                            <span class="setpoint-value">${ch.ovSetpoint.toFixed(3)} V</span>
                            <span class="mode-indicator ${ovpMode ? 'active' : ''}"></span>
                        </div>
                    </div>
                </div>
            `;
        });
        
        container.innerHTML = html;
    }

    updateHistory(channelId, voltage, current) {
        const history = this.channelHistory.get(channelId);
        //将新的电压值和电流值添加到数组末尾
        history.voltage.push(voltage);
        history.current.push(current);
        //如果数组长度超过最大历史点数，则删除最旧的值（数组开头）
        if (history.voltage.length > this.maxHistoryPoints) {
            history.voltage.shift();
            history.current.shift();
        }
    }

    toggleChannel(channelId) {
        //点击事件处理器
        console.log('Toggle channel:', channelId);
    }

    showDetails(channelId) {
        //右键点击事件处理器
        console.log('Show details for channel:', channelId);
    }
    
    initWebSocket() {
        const wsUrl = `ws://${window.location.hostname}:8080`;
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            console.log('Connecting to Leacesy Instrument WebSocket:', wsUrl);
        };

        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            // handle Back-end information
            if (data.type === 'channels_response') {
                this.channels.clear();
                data.channels.forEach(ch => {
                    this.channels.set(ch.channel, ch);
                });
                this.renderChannels();
            }
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected, reconnecting...');
            //setTimeout(() => this.initWebSocket(), 3000);// try reconnect every 3 seconds
        };
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
            };
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
                } else {
                    if (this.intervalId) {
                        clearInterval(this.intervalId);
                    }
                }
            };
        }
    }

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
