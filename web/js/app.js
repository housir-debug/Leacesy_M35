class InstrumentApp {
    constructor() {
        this.ws = null;
        this.commands = [];
        this.initWebSocket();
        this.loadDeviceInfo();
        this.loadCommands();
        this.setupEventListeners();
    }

    async loadDeviceInfo() {
        try {
            const response = await fetch('/api/device/info');
            const data = await response.json();
            document.getElementById('Brand').textContent = data.Brand || 'N/A';
            document.getElementById('model').textContent = data.model || 'N/A';
            document.getElementById('serial').textContent = data.serialNumber || 'N/A';
            document.getElementById('firmware').textContent = data.firmwareVersion || 'N/A';
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
                    //this.sendCommand();//需要添加通道号等等
                };
                list.appendChild(div);
            });
        } catch (error) {
            console.error('Failed to load commands:', error);
        }
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
                window.app.sendCommand();
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
            if (data.type === 'scpi_response') {
                this.addToOutput(`Response: ${data.result}`, 'response');
            }
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected, reconnecting...');
            document.getElementById('connection-status').textContent = 'Device: Reconnecting...';
            //setTimeout(() => this.initWebSocket(), 3000);
        };
    }

    sendCommand() {
        const cmd  = document.getElementById('command-input').value;
        if (!cmd) return;

        this.addToOutput(`> ${cmd}`, 'command');
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({
                type: 'scpi_command',
                command: cmd
            }));
        }

        document.getElementById('command-input').value = '';
    }
    
    addToOutput(text, type) {
        const output = document.getElementById('output');
        const div = document.createElement('div');
        div.className = `output-line ${type}`;
        div.textContent = text;
        output.appendChild(div);
        output.scrollTop = output.scrollHeight;
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
