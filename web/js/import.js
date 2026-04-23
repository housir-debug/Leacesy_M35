class CSVImport {
    constructor() {
        this.ws = null;
        this.models = [];
        this.fileName = '';
        this.headers = [];
        this.csvData = [];
        this.expectedHeaders = ['SOC_%', 'OCV_V', 'IMP_ohm']; 
        this.loadModels();
        this.initWebSocket();
        this.setupEventListeners();
    }

    async loadModels() {
        try {
            const response = await fetch('/api/models');
            const data = await response.json();
            this.models = data.models || [];
            this.renderModels();
        } catch (error) {
            console.error('Failed to load models:', error);
            this.models = [];
        }
    }

    renderModels() {
        const container = document.getElementById('models-list');
        if (!container) return;

        container.innerHTML = this.models.map((model, index) => `
            <div class="model-card" data-index="${index}">
                <div class="model-name">${this.escapeHtml(model.name)}</div>
                <div class="model-actions">
                    <button class="export-btn" onclick="csvImport.exportModel(${index}); event.stopPropagation();">Export</button>
                    <button class="delete-btn" onclick="csvImport.deleteModel(${index}); event.stopPropagation();">Delete</button>
                </div>
            </div>
        `).join('');
    }
    async exportModel(index) {
        const model = this.models[index];
        if (!model) {
            this.showStatus('Model data not available', 'error');
            return;
        }

        const rows = [this.expectedHeaders.join(',')];
        model.data.forEach(ch => {
            const row = [
                ch.soc || 0,
                ch.ocv || 0,
                ch.esr || 0
            ];
            rows.push(row.join(','));
        });

        const blob = new Blob([rows.join('\n')], { type: 'text/csv;charset=utf-8;' });
        const link = document.createElement('a');
        const url = URL.createObjectURL(blob);
        link.setAttribute('href', url);
        link.setAttribute('download', `${model.name}.csv`);
        link.style.visibility = 'hidden';
        document.body.appendChild(link);
        link.click();
        URL.revokeObjectURL(url);
        document.body.removeChild(link);
        this.showStatus(`Exported "${model.name}"`, 'success');
    }
    async deleteModel(index) {
        const model = this.models[index];
        if (!model) {
            this.showStatus('Model data not available', 'error');
            return;
        }

        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            this.showStatus('WebSocket not connected', 'error');
            return;
        }

        try {
            this.ws.send(JSON.stringify({
                type: 'model_delete',
                name: model.name
            }));

            this.showStatus(`Delete "${model.name}"`, 'success');
        } catch (error) {
            console.error('Failed to delete model:', error);
            this.showStatus('Failed to delete model', 'error');
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

            if (data.type === 'model_delete_ack') {
                if (data.success) {
                    const index = this.models.findIndex(m => m.name === data.name);
                    if (index !== -1) {
                        this.models.splice(index, 1);
                        this.renderModels();
                        this.showStatus(`Deleted "${data.name}"`, 'success');
                    }
                }
            }
            else if (data.type === 'model_sync' && data.models) {
                this.models = data.models;
                this.renderModels();
            }
            else {
                this.showStatus(`Failed to delete "${data.type}": ${data.message}`, 'error');
            }
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected, reconnecting...');
            //setTimeout(() => this.initWebSocket(), 3000);// try reconnect every 3 seconds
        };
    }

    setupEventListeners() {
        const uploadZone = document.getElementById('upload-zone');
        const fileInput = document.getElementById('file-input');

        window.addEventListener('dragover', (e) => {
            // 检查是否是上传区域，如果不是则阻止默认行为
            if (!e.target.closest('#upload-zone')) {
                e.preventDefault();
            }
        });

        window.addEventListener('drop', (e) => {
            // 检查是否是上传区域，如果不是则阻止默认行为
            if (!e.target.closest('#upload-zone')) {
                e.preventDefault();
                return; // 不处理文件
            }
        });

        if (uploadZone && fileInput) {
            // 点击上传区域时触发文件选择对话框
            uploadZone.addEventListener('click', () => fileInput.click());
            fileInput.addEventListener('change', (e) => {
                const file = e.target.files[0];
                if (file) this.handleFile(file);
            });

            uploadZone.addEventListener('dragover', (e) => {
                e.preventDefault();// 必须！阻止浏览器默认行为
                uploadZone.classList.add('dragover');//等于悬停视觉效果
            });
            uploadZone.addEventListener('dragleave', () => {
                uploadZone.classList.remove('dragover');
            });

            // 拖放文件时处理上传
            uploadZone.addEventListener('drop', (e) => {
                e.preventDefault();
                uploadZone.classList.remove('dragover');
                const file = e.dataTransfer.files[0];
                if (file) this.handleFile(file);
            });
        }

        const sendButton = document.getElementById('send-button');
        if (sendButton) {
            sendButton.onclick = () => this.sendToDevice();
        }
        const clearButton = document.getElementById('clear-button');
        if (clearButton) {
            clearButton.onclick = () => {
                this.csvData = [];
                this.headers = [];
                this.fileName = '';

                const fileInput = document.getElementById('file-input');
                if (fileInput) fileInput.value = '';
                const previewCard = document.getElementById('preview-card');
                if (previewCard) previewCard.classList.remove('visible');
            }
        }

        const exportAllBtn = document.getElementById('export-all-btn');
        if (exportAllBtn) {
            exportAllBtn.onclick = () => {
                if (this.models.length === 0) {
                    this.showStatus('No models to export', 'error');
                    return;
                }

                this.models.forEach((model, index) => {
                    setTimeout(() => {
                        this.exportModel(index);
                    }, index * 200);
                });
                this.showStatus(`Exporting ${this.models.length} models...`, 'info');
            }
        }
    }

    // 处理文件
    handleFile(file) {
        if (!file.name.endsWith('.csv')) {
            this.showStatus('Please select a CSV file', 'error');
            return;
        }

        this.fileName = file.name;
        const reader = new FileReader();
        
        // 文件读取成功后回调函数
        reader.onload = (e) => {
            const content = e.target.result;
            const result = this.parseCSV(content);
            if (!result) return;

            const previewCard = document.getElementById('preview-card');
            if (previewCard) previewCard.classList.add('visible');

            const thead = document.getElementById('table-header');
            if (thead) {
                thead.innerHTML = `<tr>${this.headers.map(h => `<th>${this.escapeHtml(h)}</th>`).join('')}</tr>`;
            }

            const tbody = document.getElementById('table-body');
            if (tbody) {
                const previewRows = this.csvData.slice(0, 100);
                tbody.innerHTML = previewRows.map(row => 
                    `<tr>${row.map(cell => `<td>${this.escapeHtml(cell)}</td>`).join('')}</tr>`
                ).join('');

                if (this.csvData.length > 100) {
                    tbody.innerHTML += `<tr><td colspan="${this.headers.length}" style="text-align: center; color: #a0a0a0;">... and ${this.csvData.length - 100} more rows</td></tr>`;
                }
            }

            const fileInfo = document.getElementById('file-info');
            if (fileInfo) {
                fileInfo.textContent = `${file.name} • ${this.csvData.length} rows • ${this.headers.length} columns`;
            }
        };

        // 文件读取失败后回调函数
        reader.onerror = () => {
            this.showStatus('Failed to read file', 'error');
        };

        // 读取文件
        reader.readAsText(file);
    }
    parseCSV(content) {
        const lines = content.split(/\r?\n/).filter(line => {
            //console.log(`"${line}"`, `trim后: "${line.trim()}"`);
            const trimmed = line.trim();
            if (trimmed === '') return false;// 过滤掉完全空白的行
            if (/^[, ]+$/.test(trimmed)) return false;// 过滤掉只包含逗号或者空格的行
            return true;
        });

        // 基本验证：行数必须在1到100之间
        if (lines.length === 0 || lines.length > 100) {
            this.showStatus('CSV file is empty or overlong', 'error');
            return false;
        }

        this.headers = this.parseLine(lines[0]);
        if ( this.headers.length !== this.expectedHeaders.length) {
            this.showStatus(`Invalid header format. Expected: ${this.expectedHeaders.join(', ')}`, 'error');
            return false;
        }

        for (let i = 0; i < this.expectedHeaders.length; i++) {
            if (this.headers[i] !== this.expectedHeaders[i]) {
                this.showStatus(`Invalid header format. Expected: ${this.expectedHeaders.join(', ')}`, 'error');
                return false;
            }
        }
            
        this.csvData = [];
        const rawData = [];
        for (let i = 1; i < lines.length; i++) {
            const line = lines[i];
            const row = this.parseLine(line);

            // 检查字段数量是否正确，且没有空字段
            if (row === null) {
                this.showStatus(`Row ${i} contains empty field`, 'error');
                return false;
            }
            if (row.length !== this.expectedHeaders.length) {
                this.showStatus(`Row ${i} has ${row.length} fields, expected ${this.headers.length}`, 'error');
                return false;
            }

            // 验证数值范围
            const socValue = parseFloat(row[0]);
            if (socValue < 0 || socValue > 100) {
                this.showStatus(`Row ${i}, Column "SOC_%" value ${socValue} is out of range (0-100)`, 'error');
                return false;
            }
            const ocvValue = parseFloat(row[1]);
            if (ocvValue < 0.0 || ocvValue > 6.0) {
                this.showStatus(`Row ${i}, Column "OCV_V" value ${ocvValue} is out of range (0-6.0)`, 'error');
                return false;
            }
            const impValue = parseFloat(row[2]);
            if (impValue < 0 || impValue > 1.0) {
                this.showStatus(`Row ${i}, Column "IMP_ohm" value ${impValue} is out of range (0-1.0)`, 'error');
                return false;
            }
        
            rawData.push({
                soc: socValue,
                ocv: ocvValue,
                imp: impValue,
                rowIndex: i
            });
        }

        rawData.sort((a, b) => a.soc - b.soc);
         for (let i = 1; i < rawData.length; i++) {
            if (rawData[i].soc <= rawData[i-1].soc) {
                this.showStatus(`SOC values must be strictly increasing. Found ${rawData[i-1].soc} followed by ${rawData[i].soc}`, 'error');
                return false;
            }
        }

        if (rawData.length > 0) {
            const firstSoc = rawData[0].soc;
            const lastSoc = rawData[rawData.length - 1].soc;
            
            if (Math.abs(firstSoc - 0) > 0.001) {
                this.showStatus(`First SOC value must be 0.0, but got ${firstSoc}`, 'error');
                return false;
            }
            
            if (Math.abs(lastSoc - 100) > 0.001) {
                this.showStatus(`Last SOC value must be 100.0, but got ${lastSoc}`, 'error');
                return false;
            }
        }

        this.csvData = rawData.map(item => [
            item.soc.toString(),
            item.ocv.toString(),
            item.imp.toString()
        ]);
        this.showStatus(`Successfully loaded ${this.csvData.length} rows (sorted by SOC, from ${rawData[0].soc}% to ${rawData[rawData.length-1].soc}%)`, 'success');
        return true;
    }
    parseLine(line) {
        const result = [];
        let current = '';
        let inQuotes = false;

        // 逐字符解析，处理逗号和引号
        for (let i = 0; i < line.length; i++) {
            const char = line[i];
            
            if (char === '"') {
                inQuotes = !inQuotes;
            } else if (char === ',' && !inQuotes) {
                result.push(current.trim() === '' ? null : current.trim());
                current = '';
            } else {
                current += char;
            }
        }
        //处理最后一个字段
        result.push(current.trim() === '' ? null : current.trim());

        if (result.some(field => field === null)) {
            return null;
        }
        return result;
    }
    // 发送模型数据到设备
    async sendToDevice() {
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            this.showStatus('WebSocket not connected', 'error');
            return;
        }

        if (this.csvData.length === 0) {
            this.showStatus('No data to send', 'error');
            return;
        }
        this.showStatus(`Sending ${this.csvData.length} rows...`, 'info');

        try {
            const modelName = this.fileName.replace('.csv', '');
            const modelData = {
                name: modelName,
                data: this.csvData.map(row => ({
                    soc: parseFloat(row[0]),
                    ocv: parseFloat(row[1]),
                    imp: parseFloat(row[2])
                }))
            };

            this.ws.send(JSON.stringify({
                type: 'model_upload',
                content: modelData
            }));
            this.showStatus(`Model "${modelData.name}" sent successfully (${this.csvData.length} points)`, 'success'); 
        
        } catch (error) {
            console.error('Failed to send model:', error);
            this.showStatus('Failed to send model to device', 'error');
        }
    }
    
    // 辅助函数：转义HTML特殊字符，防止XSS攻击
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
    // 辅助函数：渲染状态消息
    showStatus(message, type) {
        const status = document.getElementById('status-message');
        if (!status) return;
        status.textContent = message;
        status.className = `status-message ${type}`;

        setTimeout(() => {
            if (status.textContent === message) {
                status.textContent = '';
                status.className = 'status-message';
            }
        }, 3000);//3秒后自动清除状态消息
    }

    destroy() {
        if (this.ws) {
            this.ws.close();
        }
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.csvImport = new CSVImport();
});
