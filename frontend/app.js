/* ============================================================
   AlgoExchange — Frontend Logic
   ============================================================ */

// ---- State ----
let activeTab = 'trade';
let tradeType = 'limit'; // 'limit' or 'market'
let tradeCount = 0;

// ---- DOM References ----
const terminalOutput = document.getElementById('terminalOutput');
const loadingOverlay = document.getElementById('loadingOverlay');
const responsePanel = document.getElementById('responsePanel');
const responseContent = document.getElementById('responseContent');
const engineStatus = document.getElementById('engineStatus');
const toastContainer = document.getElementById('toastContainer');

// ============================================================
// Engine Status Polling
// ============================================================

async function checkEngineStatus() {
    try {
        const res = await fetch('/api/status');
        const data = await res.json();
        if (data.ready) {
            loadingOverlay.classList.add('hidden');
            engineStatus.innerHTML = '<span class="status-dot online"></span><span>Engine Online</span>';
            showToast('Engine ready — all systems operational', 'success');
        } else {
            setTimeout(checkEngineStatus, 500);
        }
    } catch (err) {
        setTimeout(checkEngineStatus, 1000);
    }
}
checkEngineStatus();

// ============================================================
// Toast Notifications
// ============================================================

function showToast(message, type = 'info') {
    const icons = { success: '✓', error: '✕', info: 'ℹ' };
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `<span>${icons[type] || ''}</span> ${message}`;
    toastContainer.appendChild(toast);

    setTimeout(() => {
        toast.classList.add('removing');
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// ============================================================
// Tab Navigation
// ============================================================

document.querySelectorAll('.nav-item').forEach(btn => {
    btn.addEventListener('click', () => {
        const tab = btn.dataset.tab;
        
        // Update sidebar
        document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
        btn.classList.add('active');

        // Update content
        document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
        document.getElementById(`tab-${tab}`).classList.add('active');

        // Update title
        const titles = { trade: 'Trade', orders: 'Orders', analytics: 'Analytics', tools: 'Tools', terminal: 'Terminal', settings: 'Settings' };
        document.getElementById('pageTitle').textContent = titles[tab] || tab;
        
        activeTab = tab;

        // Close response panel when switching tabs
        responsePanel.classList.remove('open');
    });
});

// ============================================================
// Trade Type Toggle (Limit / Market)
// ============================================================

document.querySelectorAll('.toggle-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.toggle-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        tradeType = btn.dataset.type;
        
        const priceGroup = document.getElementById('priceGroup');
        if (tradeType === 'market') {
            priceGroup.style.opacity = '0.3';
            priceGroup.querySelector('input').disabled = true;
        } else {
            priceGroup.style.opacity = '1';
            priceGroup.querySelector('input').disabled = false;
        }
    });
});

// ============================================================
// API Communication
// ============================================================

async function sendCommand(command) {
    if (!command.trim()) return;

    try {
        const res = await fetch('/api/command', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command })
        });

        const data = await res.json();

        if (data.error) {
            logToTerminal(command, `Error: ${data.error}`);
            showToast(`Error: ${data.error}`, 'error');
            showResponse(`Error: ${data.error}`);
        } else {
            logToTerminal(command, data.response);
            showResponse(data.response);

            // Track trade count for BUY/SELL commands
            if (command.startsWith('BUY') || command.startsWith('SELL') || command.startsWith('MARKET')) {
                tradeCount++;
                document.getElementById('tradeCountValue').textContent = tradeCount;
                
                const side = command.startsWith('SELL') || command.includes('SELL') ? 'sell' : 'buy';
                const sym = command.split(' ').find(t => /^[A-Z]{1,5}$/.test(t)) || '';
                showToast(`${side === 'buy' ? '🟢' : '🔴'} ${side.toUpperCase()} order placed for ${sym}`, 'success');
            }
        }
    } catch (err) {
        logToTerminal(command, `Network Error: ${err.message}`);
        showToast(`Network Error: ${err.message}`, 'error');
    }
}

// ============================================================
// Terminal Logging
// ============================================================

function logToTerminal(command, response) {
    const time = new Date().toLocaleTimeString();

    if (command) {
        terminalOutput.innerHTML += `\n<span style="color:#818cf8">[${time}]</span> <span style="color:#6366f1">AlgoExchange&gt;</span> <span style="color:#e2e8f0">${escapeHtml(command)}</span>\n`;
    }

    if (response) {
        terminalOutput.innerHTML += colorizeOutput(escapeHtml(response)) + '\n';
    }

    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function colorizeOutput(text) {
    // Highlight key words in the terminal output
    text = text.replace(/\b(BUY)\b/g, '<span style="color:#22c55e;font-weight:600">$1</span>');
    text = text.replace(/\b(SELL)\b/g, '<span style="color:#ef4444;font-weight:600">$1</span>');
    text = text.replace(/\b(TRADE|MATCHED|FILLED|EXECUTED)\b/gi, '<span style="color:#f59e0b">$1</span>');
    text = text.replace(/\b(ERROR|Error|INVALID)\b/gi, '<span style="color:#ef4444">$1</span>');
    text = text.replace(/(Order #\d+)/g, '<span style="color:#818cf8">$1</span>');
    text = text.replace(/(\d+\.\d{2})/g, '<span style="color:#67e8f9">$1</span>');
    return text;
}

// ============================================================
// Response Panel (for non-terminal tabs)
// ============================================================

function showResponse(text) {
    if (activeTab !== 'terminal') {
        responseContent.innerHTML = colorizeOutput(escapeHtml(text));
        responsePanel.classList.add('open');
        responseContent.scrollTop = responseContent.scrollHeight;
    }
}

document.getElementById('closeResponseBtn').addEventListener('click', () => {
    responsePanel.classList.remove('open');
});

// ============================================================
// Search
// ============================================================

document.getElementById('searchBtn').addEventListener('click', doSearch);
document.getElementById('searchInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') doSearch();
});

function doSearch() {
    const symbol = document.getElementById('searchInput').value.trim();
    if (!symbol) return;
    
    // Show inline results
    fetch('/api/command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command: `SEARCH_SYMBOL ${symbol}` })
    })
    .then(r => r.json())
    .then(data => {
        const resultsDiv = document.getElementById('searchResults');
        if (data.response) {
            resultsDiv.textContent = data.response;
            resultsDiv.classList.remove('hidden');
            logToTerminal(`SEARCH_SYMBOL ${symbol}`, data.response);
        }
    });
}

// ============================================================
// Trade Execution
// ============================================================

function getTradeData() {
    return {
        symbol: document.getElementById('tradeSymbol').value.toUpperCase().trim(),
        qty: document.getElementById('tradeQty').value,
        price: document.getElementById('tradePrice').value
    };
}

document.getElementById('buyBtn').addEventListener('click', () => {
    const { symbol, qty, price } = getTradeData();
    if (!symbol || !qty) return showToast('Fill in Symbol and Quantity', 'error');
    if (tradeType === 'limit') {
        if (!price) return showToast('Limit orders require a price', 'error');
        sendCommand(`BUY ${symbol} ${qty} ${price}`);
    } else {
        sendCommand(`MARKET BUY ${symbol} ${qty}`);
    }
});

document.getElementById('sellBtn').addEventListener('click', () => {
    const { symbol, qty, price } = getTradeData();
    if (!symbol || !qty) return showToast('Fill in Symbol and Quantity', 'error');
    if (tradeType === 'limit') {
        if (!price) return showToast('Limit orders require a price', 'error');
        sendCommand(`SELL ${symbol} ${qty} ${price}`);
    } else {
        sendCommand(`MARKET SELL ${symbol} ${qty}`);
    }
});

// ============================================================
// Order Management
// ============================================================

document.getElementById('cancelOrderBtn').addEventListener('click', () => {
    const orderId = document.getElementById('manageOrderId').value;
    if (!orderId) return showToast('Enter an Order ID', 'error');
    sendCommand(`CANCEL ${orderId}`);
});

document.getElementById('modifyOrderBtn').addEventListener('click', () => {
    const orderId = document.getElementById('modifyOrderId').value;
    const side = document.getElementById('manageSide').value;
    const qty = document.getElementById('manageQty').value;
    const price = document.getElementById('managePrice').value;
    if (!orderId || !qty || !price) return showToast('Fill in all modify fields', 'error');
    sendCommand(`MODIFY ${orderId} ${side} ${qty} ${price}`);
});

// ============================================================
// Orderbook
// ============================================================

document.getElementById('showOrderbookBtn').addEventListener('click', () => {
    const symbol = document.getElementById('obSymbol').value.toUpperCase().trim();
    sendCommand(symbol ? `SHOW_ORDERBOOK ${symbol}` : 'SHOW_ORDERBOOK');
});

// ============================================================
// Analytics
// ============================================================

document.getElementById('priceHistoryBtn').addEventListener('click', () => {
    const sym = document.getElementById('histSymbol').value.toUpperCase().trim();
    if (!sym) return showToast('Enter a symbol', 'error');
    sendCommand(`PRICE_HISTORY ${sym}`);
});

document.getElementById('rangeQueryBtn').addEventListener('click', () => {
    const sym = document.getElementById('rqSymbol').value.toUpperCase().trim();
    const type = document.getElementById('rqType').value;
    const left = document.getElementById('rqLeft').value;
    const right = document.getElementById('rqRight').value;
    if (!sym || left === '' || right === '') return showToast('Fill in all range query fields', 'error');
    sendCommand(`RANGE_QUERY ${sym} ${type} ${left} ${right}`);
});

document.getElementById('depPathBtn').addEventListener('click', () => {
    const s1 = document.getElementById('depSym1').value.toUpperCase().trim();
    const s2 = document.getElementById('depSym2').value.toUpperCase().trim();
    if (!s1 || !s2) return showToast('Enter both symbols', 'error');
    sendCommand(`DEPENDENCY_PATH ${s1} ${s2}`);
});

document.getElementById('optimizeBtn').addEventListener('click', () => {
    const side = document.getElementById('optSide').value;
    const sym = document.getElementById('optSymbol').value.toUpperCase().trim();
    const qty = document.getElementById('optQty').value;
    if (!sym || !qty) return showToast('Fill in symbol and quantity', 'error');
    sendCommand(`OPTIMIZE_ORDER ${side} ${sym} ${qty}`);
});

// ============================================================
// Quick Actions
// ============================================================

document.getElementById('topGainersBtn').addEventListener('click', () => sendCommand('TOP_GAINERS'));
document.getElementById('topLosersBtn').addEventListener('click', () => sendCommand('TOP_LOSERS'));
document.getElementById('showTradesBtn').addEventListener('click', () => sendCommand('SHOW_TRADES'));
document.getElementById('performanceBtn').addEventListener('click', () => sendCommand('PERFORMANCE'));
document.getElementById('refreshMarketBtn').addEventListener('click', () => sendCommand('MARKET_SUMMARY'));
document.getElementById('helpBtn').addEventListener('click', () => sendCommand('HELP'));
document.getElementById('marketSummaryBtn').addEventListener('click', () => sendCommand('MARKET_SUMMARY'));

// Analytics Dashboard buttons
document.getElementById('analyticsGainersBtn').addEventListener('click', () => sendCommand('TOP_GAINERS'));
document.getElementById('analyticsLosersBtn').addEventListener('click', () => sendCommand('TOP_LOSERS'));
document.getElementById('analyticsSummaryBtn').addEventListener('click', () => sendCommand('MARKET_SUMMARY'));
document.getElementById('analyticsPerformanceBtn').addEventListener('click', () => sendCommand('PERFORMANCE'));

// ============================================================
// Stress Test
// ============================================================

document.getElementById('stressTestBtn').addEventListener('click', () => {
    const count = document.getElementById('stressCount').value || '10000';
    showToast(`🚀 Launching stress test with ${count} orders...`, 'info');
    sendCommand(`STRESS_TEST ${count}`);
});

// ============================================================
// Custom Terminal Command
// ============================================================

document.getElementById('sendCommandBtn').addEventListener('click', () => {
    const input = document.getElementById('commandInput');
    if (input.value.trim()) {
        sendCommand(input.value.trim());
        input.value = '';
    }
});

document.getElementById('commandInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        const input = document.getElementById('commandInput');
        if (input.value.trim()) {
            sendCommand(input.value.trim());
            input.value = '';
        }
    }
});

document.getElementById('clearLogBtn').addEventListener('click', () => {
    terminalOutput.innerHTML = '<span style="color:#64748b">Terminal cleared.</span>\n';
});

// ============================================================
// Settings — Reset Engine
// ============================================================

async function resetEngine(clean = false) {
    const action = clean ? 'Clean & Reset' : 'Reset';
    const confirmed = confirm(`Are you sure you want to ${action} the engine? All in-memory data will be lost.`);
    if (!confirmed) return;

    showToast(`🔄 ${action}ting engine...`, 'info');
    engineStatus.innerHTML = '<span class="status-dot offline"></span><span>Restarting...</span>';

    try {
        const res = await fetch('/api/reset', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ clean })
        });
        const data = await res.json();

        if (data.status === 'restarted') {
            tradeCount = 0;
            document.getElementById('tradeCountValue').textContent = '0';
            terminalOutput.innerHTML = '<span style="color:#64748b">Engine restarted. Terminal cleared.</span>\n';
            engineStatus.innerHTML = '<span class="status-dot online"></span><span>Engine Online</span>';
            showToast(`✅ Engine ${action.toLowerCase()}ed successfully`, 'success');
        } else {
            showToast(`Error: ${data.error || 'Unknown error'}`, 'error');
        }
    } catch (err) {
        showToast(`Network Error: ${err.message}`, 'error');
    }
}

document.getElementById('resetEngineBtn').addEventListener('click', () => resetEngine(false));
document.getElementById('dropResetBtn').addEventListener('click', () => resetEngine(true));

// ============================================================
// Graphs Tab
// ============================================================

document.getElementById('drawGraphBtn').addEventListener('click', () => {
    const symbol = document.getElementById('graphSymbol').value.toUpperCase().trim();
    if (!symbol) return showToast('Enter a symbol to graph', 'error');
    drawGraph(symbol);
});

async function drawGraph(symbol) {
    const btn = document.getElementById('drawGraphBtn');
    btn.disabled = true;
    btn.textContent = 'Drawing...';
    
    try {
        const res = await fetch('/api/command', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command: `PRICE_HISTORY ${symbol}` })
        });
        const data = await res.json();
        
        if (data.error || !data.response.includes('Price History:')) {
            showToast(`No trades found for ${symbol}`, 'error');
            document.getElementById('graphEmptyState').textContent = `No trades found for ${symbol} yet`;
            document.getElementById('graphEmptyState').style.display = 'flex';
            
            // Clear any old graph
            const canvas = document.getElementById('priceChart');
            canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
            
            btn.disabled = false;
            btn.textContent = 'Draw Graph';
            return;
        }

        // Parse prices from output format: [  0]     185.50
        const regex = /\[\s*\d+\]\s+(\d+\.\d+)/g;
        let prices = [];
        let match;
        while ((match = regex.exec(data.response)) !== null) {
            prices.push(parseFloat(match[1]));
        }

        if (prices.length === 0) {
            showToast(`No trades found for ${symbol}`, 'error');
            document.getElementById('graphEmptyState').textContent = `No trades found for ${symbol} yet`;
            document.getElementById('graphEmptyState').style.display = 'flex';
            
            // Clear any old graph
            const canvas = document.getElementById('priceChart');
            canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
            
            btn.disabled = false;
            btn.textContent = 'Draw Graph';
            return;
        }

        // Keep at most 3,000 prices (last 3000)
        if (prices.length > 3000) {
            prices = prices.slice(prices.length - 3000);
        }
        
        const originalCount = prices.length;

        // Downsample to 100 points for a cleaner visual graph
        const MAX_POINTS = 100;
        if (prices.length > MAX_POINTS) {
            const smoothed = [];
            const chunkSize = Math.ceil(prices.length / MAX_POINTS);
            for (let i = 0; i < prices.length; i += chunkSize) {
                const chunk = prices.slice(i, i + chunkSize);
                const avg = chunk.reduce((a, b) => a + b, 0) / chunk.length;
                smoothed.push(avg);
            }
            prices = smoothed;
        }

        renderCanvasChart(prices, symbol, originalCount);

    } catch (err) {
        showToast(`Error: ${err.message}`, 'error');
    }

    btn.disabled = false;
    btn.textContent = 'Draw Graph';
}

function renderCanvasChart(prices, symbol, originalCount) {
    const canvas = document.getElementById('priceChart');
    const ctx = canvas.getContext('2d');
    const parent = canvas.parentElement;
    
    // Set actual canvas resolution to match its displayed size
    canvas.width = parent.clientWidth;
    canvas.height = parent.clientHeight;
    
    document.getElementById('graphEmptyState').style.display = 'none';

    // Dimensions and padding
    const width = canvas.width;
    const height = canvas.height;
    const padTop = 60; // Increased to give title more room
    const padBottom = 30;
    const padLeft = 60;
    const padRight = 20;

    const graphWidth = width - padLeft - padRight;
    const graphHeight = height - padTop - padBottom;

    // Data limits with 10% margin so it doesn't touch the very top/bottom
    const rawMin = Math.min(...prices);
    const rawMax = Math.max(...prices);
    const rawRange = rawMax - rawMin || 1;
    const margin = rawRange * 0.1; 
    
    const minPrice = rawMin - margin;
    const maxPrice = rawMax + margin;
    const priceRange = maxPrice - minPrice || 1; 

    // Clear canvas
    ctx.clearRect(0, 0, width, height);

    // Determine color based on trend
    const isUptrend = prices[prices.length - 1] >= prices[0];
    const lineColor = isUptrend ? '#22c55e' : '#ef4444'; // Green or Red
    const glowColor = isUptrend ? 'rgba(34, 197, 94, 0.4)' : 'rgba(239, 68, 68, 0.4)';
    const gradientTop = isUptrend ? 'rgba(34, 197, 94, 0.2)' : 'rgba(239, 68, 68, 0.2)';

    // 1. Draw Grid Lines & Y-Axis Labels
    ctx.font = '12px "JetBrains Mono", monospace';
    ctx.fillStyle = '#94a3b8';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    ctx.lineWidth = 1;
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';

    const steps = 5;
    for (let i = 0; i <= steps; i++) {
        const y = padTop + graphHeight - (i / steps) * graphHeight;
        const val = minPrice + (i / steps) * priceRange;
        
        // Draw grid line
        ctx.beginPath();
        ctx.moveTo(padLeft, y);
        ctx.lineTo(width - padRight, y);
        ctx.stroke();

        // Draw label
        ctx.fillText(val.toFixed(2), padLeft - 10, y);
    }

    // 2. Map coordinates
    const getX = (index) => padLeft + (index / (prices.length - 1 || 1)) * graphWidth;
    const getY = (price) => padTop + graphHeight - ((price - minPrice) / priceRange) * graphHeight;

    // 3. Draw gradient fill
    const fillGradient = ctx.createLinearGradient(0, padTop, 0, height - padBottom);
    fillGradient.addColorStop(0, gradientTop);
    fillGradient.addColorStop(1, 'rgba(0,0,0,0)');

    ctx.beginPath();
    ctx.moveTo(getX(0), getY(prices[0]));
    for (let i = 1; i < prices.length; i++) {
        ctx.lineTo(getX(i), getY(prices[i]));
    }
    ctx.lineTo(getX(prices.length - 1), height - padBottom);
    ctx.lineTo(getX(0), height - padBottom);
    ctx.closePath();
    ctx.fillStyle = fillGradient;
    ctx.fill();

    // 4. Draw the actual line
    ctx.beginPath();
    ctx.moveTo(getX(0), getY(prices[0]));
    for (let i = 1; i < prices.length; i++) {
        ctx.lineTo(getX(i), getY(prices[i]));
    }
    
    ctx.strokeStyle = lineColor;
    ctx.lineWidth = 1.5; // Thinner line since 1000 points is very dense
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    
    // Add glow
    ctx.shadowColor = glowColor;
    ctx.shadowBlur = 8;
    ctx.shadowOffsetY = 2;
    
    ctx.stroke();
    
    // Remove shadow for title
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur = 0;

    // 5. Title
    ctx.font = '600 14px Inter, sans-serif';
    ctx.fillStyle = '#f1f5f9';
    ctx.textAlign = 'left';
    ctx.fillText(`${symbol} Price History (Last ${originalCount} Trades)`, padLeft, 30);
}
