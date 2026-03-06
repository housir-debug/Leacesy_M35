class SCPIConsole {
    constructor() {
        this.ws = null;
        this.commands = [];
        this.initWebSocket();
        this.loadCommands();
        this.setupAutocomplete();
    }

    initWebSocket() {
        const wsUrl = `ws://${window.location.hostname}:8080`;
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            document.getElementById('connection-status').textContent = 'WebSocket: Connected';
        };

        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if (data.type === 'scpi_response') {
                this.addToOutput(`Response: ${data.result}`, 'response');
            }
        };
    }

    async loadCommands() {
        try {
            const response = await fetch('/api/scpi_commands');
            const data = await response.json();
            this.commands = data.commands;
            this.renderCommandList();
        } catch (error) {
            console.error('Failed to load commands:', error);
        }
    }

    renderCommandList() {
        const list = document.getElementById('command-list');
        if (!list) return;

        this.commands.forEach(cmd => {
            const div = document.createElement('div');
            div.className = 'command-item';
            div.textContent = cmd;
            div.onclick = () => this.selectCommand(cmd);
            div.ondblclick = () => this.sendCommand(cmd);
            list.appendChild(div);
        });
    }

    selectCommand(cmd) {
        document.getElementById('command-input').value = cmd;
    }

    sendCommand(cmd) {
        if (!cmd) {
            cmd = document.getElementById('command-input').value;
        }
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

    setupAutocomplete() {
        const input = document.getElementById('command-input');
        input.addEventListener('input', () => {
            const val = input.value;
            if (!val) return;

            const matches = this.commands.filter(cmd => 
                cmd.toLowerCase().includes(val.toLowerCase())
            ).slice(0, 10);

            // 显示自动补全下拉框
            this.showAutocomplete(matches);
        });
    }

    showAutocomplete(matches) {
        // 实现自动补全UI
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.scpiConsole = new SCPIConsole();
    
    document.getElementById('send-button').onclick = () => {
        window.scpiConsole.sendCommand();
    };
});