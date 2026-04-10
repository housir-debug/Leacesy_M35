class InstrumentApp {
    constructor() {
        this.ws = null;
        this.commands = [];
        this.loadDeviceInfo();
        this.loadCommands();
        this.initWebSocket();
        this.setupEventListeners();
    }

    async loadDeviceInfo() {
        try {
            const info = await fetch('/api/device/info');
            const data = await info.json();
            document.getElementById('model').textContent = data.model || 'N/A';
            document.getElementById('serial').textContent = data.serial || 'N/A';
            document.getElementById('software').textContent = data.software || 'N/A';
            document.getElementById('hardware').textContent = data.hardware || 'N/A';
        } catch (error) {
            console.error('Failed to load device info:', error);
        }
    }

    async loadCommands() {
        try {
            const list = document.getElementById('command-list');
            if (!list) return;
            const response = await fetch('/api/scpi_commands');
            const data = await response.json();
            this.commands = data.commands;

            this.commands.forEach(cmd => {
                const div = document.createElement('div');
                div.className = 'command-item';
                div.textContent = cmd;
                div.ondblclick = () => {
                    document.getElementById('command-input').value = cmd;
                };
                list.appendChild(div);
            });
        } catch (error) {
            console.error('Failed to load SCPIcommands:', error);
        }
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
            if (data.type === 'scpi_response') {
                this.addToOutput(`> ${data.result}`, 'response');
            }
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected, reconnecting...');
            //setTimeout(() => this.initWebSocket(), 3000);// try reconnect every 3 seconds
        };
    }

    setupEventListeners() {
        const input = document.getElementById('command-input');
        if (input) {
            input.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    this.sendCommand();
                }
            });
        }

        const sendBtn = document.getElementById('send-button');
        if (sendBtn) {
            sendBtn.onclick = () => {
                this.sendCommand();
            };
        }

        const clearButton = document.getElementById('clear-button');
        if (clearButton) {
            clearButton.onclick = () => {
                const output = document.getElementById('output');
                if (output) {
                    output.innerHTML = '';
                }
            };
        }
    }
    
    sendCommand() {
        const cmd  = document.getElementById('command-input').value;
        if (!cmd) return;
        this.addToOutput(`> ${cmd}`, 'command');
        document.getElementById('command-input').value = '';

        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({
                type: 'scpi_command',
                command: cmd
            }));
        }else{
            this.addToOutput('WebSocket is not connected. Please refresh the page and try again.', 'error');
        }
    }
    
    addToOutput(text, type) {
        const output = document.getElementById('output');
        const div = document.createElement('div');
        div.className = `output-line ${type}`;
        div.textContent = text;
        output.appendChild(div);
        output.scrollTop = output.scrollHeight; // 自动滚动到底部
    }

    destroy() {
        if (this.ws) {
            this.ws.close();
        }
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.app = new InstrumentApp();
});
