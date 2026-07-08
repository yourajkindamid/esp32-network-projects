#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <TinyGPS++.h>


const char* ssid = "RCCCCCC";
const char* password = ""; 


#define MAX_SPEED 240      
#define PWM_FREQ 1000
#define PWM_RES 8


#define IN1 23   
#define IN2 22   
#define IN3 21   
#define IN4 19   

#define TRIG_PIN 16
#define ECHO_PIN 17
unsigned long lastSonarRead = 0;
const unsigned long SONAR_READ_INTERVAL = 100;


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

int targetSpeedLeft = 0;  
int targetSpeedRight = 0; 
bool motorInvertLeft  = false; 
bool motorInvertRight = false; 


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ESP32 RC Pro</title>
<style>
  :root {
    --bg: #0a0e14;
    --panel: #131a24;
    --surface: #1a2332;
    --border: #2b3648;
    --accent: #9fef00;
    --accent-dim: rgba(159, 239, 0, 0.12);
    --text: #87a1ad;
    --text-light: #f2f6f8;
    --shadow-btn: 0 6px 0 0 #05070a;
    --shadow-btn-active: 0 0 0 0 transparent;
    --bg-rgb: 10, 14, 20;
    --panel-rgb: 19, 26, 36;
  }

  body.light-mode {
    --bg: #f4f6f8;
    --panel: #ffffff;
    --surface: #e9edf1;
    --border: #d3dae1;
    --accent: #6ea100;
    --accent-dim: rgba(110, 161, 0, 0.1);
    --text: #4b5563;
    --text-light: #10151a;
    --shadow-btn: 0 6px 0 0 #c2c9d1;
    --bg-rgb: 244, 246, 248;
    --panel-rgb: 255, 255, 255;
  }
  
  * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
  
  body {
    margin: 0; padding: 0; background-color: var(--bg); color: var(--text);
    font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
    overflow: hidden; height: 100vh; width: 100vw;
    display: flex; flex-direction: column; user-select: none;
    background-image: radial-gradient(var(--border) 1px, transparent 1px);
    background-size: 24px 24px;
    touch-action: none;
    transition: background-color 0.3s, color 0.3s;
  }

  /* --- HEADER --- */
  header {
    height: 64px; background: rgba(var(--bg-rgb), 0.85); backdrop-filter: blur(10px);
    display: flex; align-items: center; justify-content: space-between; padding: 0 16px;
    border-bottom: 1px solid var(--border); z-index: 50;
  }

  .header-left { display: flex; align-items: center; gap: 8px; }
  
  .status-dot {
    width: 36px; height: 36px; border-radius: 50%;
    display: flex; align-items: center; justify-content: center;
    background: rgba(239, 68, 68, 0.1); color: #ef4444;
    transition: 0.3s;
  }
  .status-dot.connected { background: var(--accent-dim); color: var(--accent); }

  .ip-box {
    display: none; background: var(--panel); border: 1px solid var(--border);
    border-radius: 8px; padding: 0 8px; height: 36px;
    align-items: center; width: 140px;
  }
  @media (min-width: 500px) { .ip-box { display: flex; } }
  
  .ip-input {
    background: transparent; border: none; outline: none;
    color: var(--text-light); font-family: 'Consolas', monospace; font-size: 12px; width: 100%;
  }

  .btn-connect {
    background: var(--accent); color: #0a0e14; border: none;
    height: 36px; padding: 0 15px; border-radius: 8px;
    font-weight: bold; font-size: 13px; cursor: pointer;
    box-shadow: 0 4px 10px var(--accent-dim);
    transition: 0.2s;
  }
  .btn-connect.disconnect { background: #ef4444; color: white; box-shadow: 0 4px 10px rgba(239, 68, 68, 0.2); }

  .header-right { display: flex; align-items: center; gap: 6px; }

  .btn-tool {
    background: var(--panel); border: 1px solid var(--border);
    color: var(--text); height: 36px; width: 36px;
    border-radius: 8px; display: flex; align-items: center; justify-content: center;
    cursor: pointer; transition: 0.2s;
  }
  .btn-tool.active { background: var(--accent); border-color: var(--accent); color: #0a0e14; }
  .btn-edit-text { width: auto; padding: 0 12px; gap: 6px; font-size: 12px; font-weight: bold; }

  /* --- GAME AREA --- */
  #game-area { flex: 1; position: relative; width: 100%; height: 100%; }
  .group { position: absolute; display: flex; align-items: center; justify-content: center; }

  .control-box {
    background: rgba(var(--panel-rgb), 0.85); border: 1px solid var(--border);
    backdrop-filter: blur(8px);
    border-radius: 40px; padding: 16px;
    display: flex; gap: 16px;
  }

  /* --- 3D BUTTONS --- */
  .btn-3d {
    width: 100px; height: 100px;
    background: var(--surface); border: 1px solid var(--border);
    border-radius: 24px; color: var(--text);
    display: flex; align-items: center; justify-content: center;
    font-size: 32px; cursor: pointer;
    box-shadow: var(--shadow-btn);
    transition: all 0.1s; touch-action: manipulation;
  }
  .btn-3d:active, .btn-3d.pressed {
    transform: translateY(6px); box-shadow: var(--shadow-btn-active);
    background: var(--accent); color: #0a0e14; border-color: var(--accent);
  }

  /* Log Box */
  #log-box {
    width: 300px; height: 180px;
    background: var(--panel); border: 1px solid var(--border);
    border-radius: 16px; display: flex; flex-direction: column;
    overflow: hidden;
  }

  #scanlog-box {
    width: 260px; height: 160px;
    background: var(--panel); border: 1px solid var(--border);
    border-radius: 16px; display: flex; flex-direction: column;
    overflow: hidden;
  }

  .log-header {
    height: 36px; border-bottom: 1px solid var(--border);
    display: flex; align-items: center; padding: 0 12px; gap: 8px;
    font-size: 11px; font-weight: bold; background: rgba(159, 239, 0, 0.04);
    color: var(--text-light); letter-spacing: 0.5px;
  }
  .log-content { flex: 1; padding: 12px; overflow-y: auto; font-family: 'Consolas', monospace; font-size: 11px; }
  .log-tx { color: var(--accent); } .log-err { color: #ef4444; } .log-suc { color: #22c55e; }

/* Sonar Panel */
  #sonar-box {
    width: 190px;
    background: var(--panel); border: 1px solid var(--border);
    border-radius: 16px; overflow: hidden;
  }
  .sonar-header {
    height: 36px; border-bottom: 1px solid var(--border);
    display: flex; align-items: center; gap: 8px; padding: 0 12px;
    font-size: 11px; font-weight: bold; background: rgba(159, 239, 0, 0.04);
    color: var(--text-light); letter-spacing: 0.5px;
  }
  .sonar-fix-dot {
    width: 8px; height: 8px; border-radius: 50%; background: #ef4444; margin-left: auto;
    transition: 0.3s;
  }
  .sonar-fix-dot.locked { background: #22c55e; box-shadow: 0 0 6px #22c55e; }
  .sonar-readout { padding: 10px 12px; font-family: 'Consolas', monospace; font-size: 11px; }
  .sonar-row { display: flex; justify-content: space-between; padding: 3px 0; color: var(--text); }
  .sonar-row span:last-child { color: var(--text-light); font-weight: bold; }

  /* Overlays */
  .resize-handle { width: 24px; height: 24px; background: var(--accent); border: 2px solid var(--bg); border-radius: 50%; position: absolute; bottom: -8px; right: -8px; display: none; z-index: 100; }
  .edit-overlay { position: absolute; inset: 0; border: 2px dashed var(--accent); background: var(--accent-dim); border-radius: 40px; pointer-events: none; display: none; z-index: 90; }
  .edit-mode .resize-handle, .edit-mode .edit-overlay { display: block; }
  .edit-mode .draggable { cursor: move; }

  @keyframes pulse { 50% { opacity: 0.5; } }

  /* --- RESPONSIVE / MOBILE --- */
  @media (max-width: 600px) {
    header { padding: 0 10px; height: 56px; }
    .btn-tool { height: 32px; width: 32px; }
    .btn-connect { height: 32px; padding: 0 12px; font-size: 12px; }
    .status-dot { width: 32px; height: 32px; }

    .btn-3d { width: 72px; height: 72px; border-radius: 18px; }
    .btn-3d svg { width: 28px; height: 28px; }
    .control-box { padding: 10px; gap: 10px; border-radius: 28px; }

    #grp-steer { left: 16px; bottom: 16px; }
    #grp-thro { right: 16px; bottom: 16px; }

    #log-box { width: 200px; height: 130px; }
    .log-header { height: 30px; font-size: 10px; }
    .log-content { padding: 8px; font-size: 10px; }

    #scanlog-box { width: 200px; height: 130px; }

    #sonar-box { width: 160px; }
    .sonar-header { height: 30px; font-size: 10px; }
    .sonar-readout { padding: 8px 10px; font-size: 10px; }
  }

  @media (max-width: 380px) {
    .btn-3d { width: 60px; height: 60px; }
    .btn-3d svg { width: 22px; height: 22px; }
    #log-box { width: 170px; height: 110px; }
  }
</style>
</head>
<body id="body-main">

<header>
  <div class="header-left">
    <div id="status-icon" class="status-dot">
      <!-- Default Disconnected Icon with Cross -->
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 1l22 22"/><path d="M16.72 11.06A10.94 10.94 0 0 1 19 12.55"/><path d="M5 12.55a10.94 10.94 0 0 1 5.17-2.39"/><path d="M10.71 5.05A16 16 0 0 1 22.58 9"/><path d="M1.42 9a15.91 15.91 0 0 1 4.7-2.88"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>
    </div>
    <div class="ip-box">
      <input type="text" id="ip-address" value="192.168.4.1" class="ip-input">
    </div>
    <button id="btn-connect" class="btn-connect" onclick="toggleConnection()">Connect</button>
  </div>
  
  <div class="header-right">
    <button id="btn-scan" class="btn-tool btn-edit-text" onclick="sendCmd('SCAN',1)" title="Start 360 degree sweep scan">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M12 6v6l4 2"/></svg>
      <span>Scan</span>
    </button>

    <button id="btn-theme" class="btn-tool" onclick="toggleTheme()" title="Toggle Theme">
      <svg id="theme-icon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <circle cx="12" cy="12" r="5"></circle>
        <line x1="12" y1="1" x2="12" y2="3"></line>
        <line x1="12" y1="21" x2="12" y2="23"></line>
        <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
        <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
        <line x1="1" y1="12" x2="3" y2="12"></line>
        <line x1="21" y1="12" x2="23" y2="12"></line>
        <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
        <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
      </svg>
    </button>

    <button id="btn-reset" class="btn-tool" style="display:none; color:#ef4444" onclick="resetLayout()">
       <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"/><path d="M3 3v5h5"/></svg>
    </button>
    <button id="btn-edit" class="btn-tool btn-edit-text" onclick="toggleEdit()">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>
      <span id="txt-edit">Edit</span>
    </button>
  </div>
</header>

<div id="game-area">
  <div id="grp-sonar" class="group draggable" style="top: 80px; left: 20px;">
    <div id="sonar-box">
      <div class="sonar-header">
        <span style="color:var(--accent)">◎</span> SONAR
        <div id="sonar-fix-dot" class="sonar-fix-dot"></div>
      </div>
      <div class="sonar-readout">
        <div class="sonar-row"><span>DISTANCE</span><span id="sonar-distance">--</span></div>
      </div>
    </div>
    <div class="edit-overlay" style="border-radius:16px"></div>
  </div>

  <div id="grp-log" class="group draggable" style="top: 50%; left: 50%; transform: translate(-50%, -50%);">
    <div id="log-box">
      <div class="log-header"><span style="color:var(--accent)">>_</span> SYSTEM LOG</div>
      <div id="log-content" class="log-content"><div class="log-msg" style="opacity:0.5">Ready...</div></div>
    </div>
    <div class="edit-overlay" style="border-radius:16px"></div>
    <div class="resize-handle" onmousedown="startResize(event, 'grp-log')" ontouchstart="startResize(event, 'grp-log')"></div>
  </div>

  <div id="grp-scanlog" class="group draggable" style="top: 80px; right: 20px;">
    <div id="scanlog-box">


      <div class="log-header">
        <span style="color:var(--accent)">⟳</span> SCAN LOG
        <button onclick="downloadScanData()" style="margin-left:auto; background:none; border:1px solid var(--border); color:var(--text); font-size:9px; font-weight:bold; padding:3px 8px; border-radius:6px; cursor:pointer; letter-spacing:0.5px;">SAVE</button>
      </div>      
      
      
      
      <div id="scanlog-content" class="log-content"><div class="log-msg" style="opacity:0.5">No scans yet...</div></div>
    </div>
    <div class="edit-overlay" style="border-radius:16px"></div>
    <div class="resize-handle" onmousedown="startResize(event, 'grp-scanlog')" ontouchstart="startResize(event, 'grp-scanlog')"></div>
  </div>

  <div id="grp-steer" class="group draggable" style="left: 40px; bottom: 40px;">
    <div class="control-box">
      <button class="btn-3d" data-cmd="LEFT"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 12H5"/><path d="M12 19l-7-7 7-7"/></svg></button>
      <button class="btn-3d" data-cmd="RIGHT"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14"/><path d="M12 5l7 7-7 7"/></svg></button>
    </div>
    <div class="edit-overlay"></div>
    <div class="resize-handle" onmousedown="startResize(event, 'grp-steer')" ontouchstart="startResize(event, 'grp-steer')"></div>
  </div>

  <div id="grp-thro" class="group draggable" style="right: 40px; bottom: 40px;">
    <div class="control-box" style="flex-direction: column;">
      <button class="btn-3d" data-cmd="FORWARD"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 19V5"/><path d="M5 12l7-7 7 7"/></svg></button>
      <button class="btn-3d" data-cmd="BACKWARD"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 5v14"/><path d="M19 12l-7 7-7-7"/></svg></button>
    </div>
    <div class="edit-overlay"></div>
    <div class="resize-handle" onmousedown="startResize(event, 'grp-thro')" ontouchstart="startResize(event, 'grp-thro')"></div>
  </div>
</div>

<script>
  let isConnected = false;
  let isEditMode = false;
  let websocket;
  let scanData = [];


function toggleTheme() {
  const body = document.getElementById('body-main');
  const isLight = body.classList.toggle('light-mode');
  localStorage.setItem('rc_theme', isLight ? 'light' : 'dark');
  updateThemeIcon(isLight);
}

function downloadScanData() {
    if (scanData.length === 0) return;
    const lines = scanData.map(p => `${p.angle}, ${p.distance}`).join('\n');
    const blob = new Blob([lines], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `scan_${Date.now()}.txt`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  function updateThemeIcon(isLight) {
    const icon = document.getElementById('theme-icon');
    if (isLight) {
      icon.innerHTML = '<path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>'; // Moon
    } else {
      icon.innerHTML = '<circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>'; // Sun
    }
  }

  function addLog(msg, type='info') {
    const box = document.getElementById('log-content');
    const div = document.createElement('div');
    div.className = 'log-msg ' + (type === 'tx' ? 'log-tx' : type === 'error' ? 'log-err' : type === 'success' ? 'log-suc' : '');
    div.innerHTML = `<span>[${new Date().toLocaleTimeString([],{hour12:false,hour:'2-digit',minute:'2-digit',second:'2-digit'})}]</span> ${msg}`;
    box.appendChild(div); box.scrollTop = box.scrollHeight;
  }

  function addScanLog(msg, type='info') {
    const box = document.getElementById('scanlog-content');
    const div = document.createElement('div');
    div.className = 'log-msg ' + (type === 'tx' ? 'log-tx' : type === 'error' ? 'log-err' : type === 'success' ? 'log-suc' : '');
    div.innerHTML = `<span>[${new Date().toLocaleTimeString([],{hour12:false,hour:'2-digit',minute:'2-digit',second:'2-digit'})}]</span> ${msg}`;
    box.appendChild(div); box.scrollTop = box.scrollHeight;
  }

  function toggleConnection() {
    if(isConnected) { websocket.close(); onDisconnect(); }
    else {
      const ip = document.getElementById('ip-address').value;
      websocket = new WebSocket(`ws://${ip}/ws`);
      websocket.onopen = () => { isConnected = true; updateUIState(true); addLog(`Connected to ${ip}`, 'success'); };
      websocket.onclose = () => onDisconnect();
      websocket.onerror = () => addLog('Connection Error', 'error');
      websocket.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          if (data.type === 'sonar') updateSonarUI(data);
          else if (data.type === 'scan_start') { scanData = []; addScanLog('Scan started', 'success'); }
          else if (data.type === 'scan_point') {
            if (data.ok) { addScanLog(`Scan ${data.angle}&deg; -&gt; ${data.distance} cm`); scanData.push({angle: data.angle, distance: data.distance}); }
            else addScanLog(`Scan ${data.angle}&deg; -&gt; no echo`, 'error');
          }
        else if (data.type === 'scan_complete') addScanLog('Scan complete', 'success');
        } catch(e) { /* ignore non-JSON messages */ }
      };
    }
  }

  function updateSonarUI(data) {
    const dot = document.getElementById('sonar-fix-dot');
    if (data.ok) {
      dot.classList.add('locked');
      document.getElementById('sonar-distance').innerText = data.distance.toFixed(1) + ' cm';
    } else {
      dot.classList.remove('locked');
      document.getElementById('sonar-distance').innerText = '--';
    }
  }

  function onDisconnect() { isConnected = false; updateUIState(false); addLog('Disconnected', 'error'); }

  function updateUIState(connected) {
    const btn = document.getElementById('btn-connect');
    const icon = document.getElementById('status-icon');
    if(connected) {
      btn.innerText = 'Disconnect'; 
      btn.classList.add('disconnect'); 
      icon.classList.add('connected');
      icon.innerHTML = '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>';
    } else {
      btn.innerText = 'Connect'; 
      btn.classList.remove('disconnect'); 
      icon.classList.remove('connected');
      icon.innerHTML = '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 1l22 22"/><path d="M16.72 11.06A10.94 10.94 0 0 1 19 12.55"/><path d="M5 12.55a10.94 10.94 0 0 1 5.17-2.39"/><path d="M10.71 5.05A16 16 0 0 1 22.58 9"/><path d="M1.42 9a15.91 15.91 0 0 1 4.7-2.88"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>';
    }
  }

  function sendCmd(cmd, val) {
    if(websocket && isConnected) {
      websocket.send(JSON.stringify({cmd: cmd, val: val}));
      if(val === 1) addLog(`TX: ${cmd}`, 'tx');
    }
  }

  document.querySelectorAll('.btn-3d').forEach(btn => {
    const cmd = btn.dataset.cmd;
    const press = (e) => { if(isEditMode) return; e.preventDefault(); btn.classList.add('pressed'); sendCmd(cmd, 1); };
    const release = (e) => { if(isEditMode) return; e.preventDefault(); btn.classList.remove('pressed'); sendCmd(cmd, 0); };
    btn.addEventListener('touchstart', press, {passive: false}); btn.addEventListener('touchend', release);
    btn.addEventListener('mousedown', press); btn.addEventListener('mouseup', release); btn.addEventListener('mouseleave', release);
  });

  function toggleEdit() {
    isEditMode = !isEditMode;
    document.body.classList.toggle('edit-mode');
    document.getElementById('btn-edit').classList.toggle('active', isEditMode);
    document.getElementById('txt-edit').innerText = isEditMode ? 'Done' : 'Edit';
    document.getElementById('btn-reset').style.display = isEditMode ? 'flex' : 'none';
    if(!isEditMode) saveLayout();
  }

  function saveLayout() {
    const layout = {};
    document.querySelectorAll('.draggable').forEach(el => {
      layout[el.id] = { left: el.style.left, top: el.style.top, transform: el.style.transform };
    });
    const logBox = document.getElementById('log-box');
    layout['size_log'] = { w: logBox.style.width, h: logBox.style.height };
    localStorage.setItem('rc_layout_v2', JSON.stringify(layout));
    const scanLogBox = document.getElementById('scanlog-box');
    layout['size_scanlog'] = { w: scanLogBox.style.width, h: scanLogBox.style.height };
  }

  function loadLayout() {
    const theme = localStorage.getItem('rc_theme');
    if(theme === 'light') { document.getElementById('body-main').classList.add('light-mode'); updateThemeIcon(true); }
    const data = localStorage.getItem('rc_layout_v2');
    if(!data) return;
    const layout = JSON.parse(data);
    for (const id in layout) {
      if(id === 'size_log') {
        const lb = document.getElementById('log-box');
        if(layout[id].w) lb.style.width = layout[id].w;
        if(layout[id].h) lb.style.height = layout[id].h;
        continue;
      }
      if(id === 'size_scanlog') {
        const slb = document.getElementById('scanlog-box');
        if(layout[id].w) slb.style.width = layout[id].w;
        if(layout[id].h) slb.style.height = layout[id].h;
        continue;
      }
      const el = document.getElementById(id);
      if(el) { el.style.left = layout[id].left; el.style.top = layout[id].top; el.style.transform = layout[id].transform; }
    }
  }

  function resetLayout() { if(confirm('Reset layout?')) { localStorage.removeItem('rc_layout_v2'); location.reload(); } }

  let dragItem = null, startX, startY, startLeft, startTop, isResizing = false;
  document.querySelectorAll('.draggable').forEach(el => {
    const startDrag = (e) => {
      if(!isEditMode || e.target.classList.contains('resize-handle')) return;
      e.preventDefault(); dragItem = el;
      const t = e.touches ? e.touches[0] : e;
      startX = t.clientX; startY = t.clientY; startLeft = el.offsetLeft; startTop = el.offsetTop;
      el.style.transform = 'none';
    };
    el.addEventListener('touchstart', startDrag, {passive:false}); el.addEventListener('mousedown', startDrag);
  });

  const onMove = (e) => {
    if(!dragItem || isResizing) return;
    const t = e.touches ? e.touches[0] : e;
    dragItem.style.left = (startLeft + t.clientX - startX) + 'px';
    dragItem.style.top = (startTop + t.clientY - startY) + 'px';
    dragItem.style.bottom = 'auto'; dragItem.style.right = 'auto';
  };
  window.addEventListener('mousemove', onMove);
  window.addEventListener('touchmove', onMove, {passive:false});
  window.addEventListener('mouseup', () => dragItem = null);
  window.addEventListener('touchend', () => dragItem = null);

  function startResize(e, id) {
    if(!isEditMode) return; e.stopPropagation(); isResizing = true;
    const t = e.touches ? e.touches[0] : e;
    const initX = t.clientX, initY = t.clientY;
    const el = document.getElementById(id);
    const box = (id === 'grp-log') ? document.getElementById('log-box')
               : (id === 'grp-scanlog') ? document.getElementById('scanlog-box')
               : el.querySelector('.btn-3d');
    const sW = box.offsetWidth, sH = box.offsetHeight;
    const doResize = (ev) => {
      const ct = ev.touches ? ev.touches[0] : ev;
      if(id === 'grp-log' || id === 'grp-scanlog') {
        box.style.width = Math.max(200, sW + ct.clientX - initX) + 'px';
        box.style.height = Math.max(100, sH + ct.clientY - initY) + 'px';
      } else {
        const size = Math.max(60, sW + ct.clientX - initX);
        el.querySelectorAll('.btn-3d').forEach(b => { 
          b.style.width = size + 'px'; b.style.height = size + 'px'; 
          const svg = b.querySelector('svg'); if(svg) { svg.setAttribute('width', size*0.4); svg.setAttribute('height', size*0.4); }
        });
      }
    };
    const stop = () => { 
      window.removeEventListener('mousemove', doResize); 
      window.removeEventListener('touchmove', doResize);
      isResizing = false; 
    };
    window.addEventListener('mousemove', doResize);
    window.addEventListener('touchmove', doResize, {passive:false});
    window.addEventListener('mouseup', stop);
    window.addEventListener('touchend', stop);
  }

  window.onload = loadLayout;
</script>
</body>
</html>
)rawliteral";


const float OBSTACLE_THRESHOLD_CM = 25.0;
const unsigned long AVOID_TURN_DURATION = 250; 

bool avoiding = false;
unsigned long avoidStartTime = 0;
bool connected = false;



#define SCAN_STEP_DEG       15.0   // degrees rotated per step
#define SCAN_TOTAL_DEG      360.0  // total sweep (360 = full circle)
#define TURN_MS_PER_DEG     8.0    // ms of full-speed turn per deg
#define SCAN_SETTLE_MS      150    // pause after stopping, before first ping
#define SCAN_SAMPLES        3      // sonar pings averaged per angle
#define SCAN_SAMPLE_GAP_MS  60     // gap between repeated pings at the same angle

enum ScanState { SCAN_IDLE, SCAN_TURN, SCAN_SETTLE, SCAN_MEASURE };
ScanState scanState = SCAN_IDLE;

float scanHeading = 0;             // relative heading since scan start, degrees
int   scanSamplesTaken = 0;
int   scanSampleFails = 0;
float scanSampleSum = 0;
unsigned long scanStepStartTime = 0;

void startScan();
void updateScan();
float getDistanceCM();
void setMotorSpeed(int speedL, int speedR);

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
              AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("Client Connected");
    connected = true;
  }

  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      String msg = (char*)data;
      String cmd = "";
      int val = 0;

      int cmdStart = msg.indexOf("\"cmd\":\"") + 7;
      int cmdEnd   = msg.indexOf("\"", cmdStart);
      cmd = msg.substring(cmdStart, cmdEnd);

      int valStart = msg.indexOf("\"val\":") + 6;
      int valEnd   = msg.indexOf("}", valStart);
      val = msg.substring(valStart, valEnd).toInt();

      if (cmd == "RIGHT") {
        if (val) { targetSpeedLeft = MAX_SPEED; targetSpeedRight = -MAX_SPEED; }        //INVERTED CONTROLS
        else { targetSpeedLeft = 0; targetSpeedRight = 0; }
      }
      else if (cmd == "LEFT") {
        if (val) { targetSpeedLeft = -MAX_SPEED; targetSpeedRight = MAX_SPEED; }
        else { targetSpeedLeft = 0; targetSpeedRight = 0; }
      }
      else if (cmd == "FORWARD") {
        if (val) { targetSpeedLeft = -MAX_SPEED; targetSpeedRight = -MAX_SPEED; }
        else { targetSpeedLeft = 0; targetSpeedRight = 0; }
      }
      else if (cmd == "BACKWARD") {
        if (val) { targetSpeedLeft = MAX_SPEED; targetSpeedRight = MAX_SPEED; }
        else { targetSpeedLeft = 0; targetSpeedRight = 0; }
      }
      else if (cmd == "SCAN") {
        if (val && scanState == SCAN_IDLE) {
          targetSpeedLeft = 0;
          targetSpeedRight = 0;
          startScan();
        }
      }      
    }
  }

  else if (type != WS_EVT_CONNECT) {
    Serial.println("Client disconnected");
    connected = false;
  }
}

float lastDistance = -1;

float getDistanceCM()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    if (duration == 0)
        return -1;  
    
    lastDistance = duration * 0.0343 / 2.0;

    return duration * 0.0343 / 2.0;
}


void setMotorSpeed(int speedL, int speedR) {
  if (speedL > 0) { ledcWrite(IN1, speedL); ledcWrite(IN2, 0); } 
  else if (speedL < 0) { ledcWrite(IN1, 0); ledcWrite(IN2, -speedL); } 
  else { ledcWrite(IN1, 0); ledcWrite(IN2, 0); }

  if (speedR > 0) { ledcWrite(IN3, speedR); ledcWrite(IN4, 0); } 
  else if (speedR < 0) { ledcWrite(IN3, 0); ledcWrite(IN4, -speedR); } 
  else { ledcWrite(IN3, 0); ledcWrite(IN4, 0); }
}



void startScan() {
  scanHeading = 0;
  scanSamplesTaken = 0;
  scanSampleFails = 0;
  scanSampleSum = 0;
  scanState = SCAN_SETTLE;      // settle briefly, then take the heading-0 reading
  scanStepStartTime = millis();
  ws.textAll("{\"type\":\"scan_start\"}");
}

void updateScan() {
  switch (scanState) {

    case SCAN_IDLE:
      return;

    case SCAN_TURN: {
      setMotorSpeed(MAX_SPEED, -MAX_SPEED);   // spin in place
      unsigned long turnDuration = (unsigned long)(SCAN_STEP_DEG * TURN_MS_PER_DEG);
      if (millis() - scanStepStartTime >= turnDuration) {
        setMotorSpeed(0, 0);
        scanHeading += SCAN_STEP_DEG;
        scanState = SCAN_SETTLE;
        scanStepStartTime = millis();
      }
      break;
    }

    case SCAN_SETTLE: {
      setMotorSpeed(0, 0);
      if (millis() - scanStepStartTime >= SCAN_SETTLE_MS) {
        scanSamplesTaken = 0;
        scanSampleFails = 0;
        scanSampleSum = 0;
        scanState = SCAN_MEASURE;
        scanStepStartTime = millis();
      }
      break;
    }

    case SCAN_MEASURE: {
      setMotorSpeed(0, 0);
      if (millis() - scanStepStartTime >= SCAN_SAMPLE_GAP_MS) {
        float d = getDistanceCM();
        if (d > 0) scanSampleSum += d;
        else scanSampleFails++;
        scanSamplesTaken++;
        scanStepStartTime = millis();

        if (scanSamplesTaken >= SCAN_SAMPLES) {
          int validSamples = SCAN_SAMPLES - scanSampleFails;
          String json;
          if (validSamples > 0) {
            float avgDist = scanSampleSum / validSamples;
            json = "{\"type\":\"scan_point\",\"angle\":" + String(scanHeading, 1) +
                   ",\"distance\":" + String(avgDist, 1) + ",\"ok\":true}";
          } else {
            json = "{\"type\":\"scan_point\",\"angle\":" + String(scanHeading, 1) +
                   ",\"ok\":false}";
          }
          ws.textAll(json);

          if (scanHeading + 0.01 >= SCAN_TOTAL_DEG) {
            scanState = SCAN_IDLE;
            ws.textAll("{\"type\":\"scan_complete\"}");
          } else {
            scanState = SCAN_TURN;
            scanStepStartTime = millis();
          }
        }
      }
      break;
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); digitalWrite(IN1, LOW);
  pinMode(IN2, OUTPUT); digitalWrite(IN2, LOW);
  pinMode(IN3, OUTPUT); digitalWrite(IN3, LOW);
  pinMode(IN4, OUTPUT); digitalWrite(IN4, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT); 

  digitalWrite(TRIG_PIN, LOW);

  ledcAttach(IN1, PWM_FREQ, PWM_RES);
  ledcAttach(IN2, PWM_FREQ, PWM_RES);
  ledcAttach(IN3, PWM_FREQ, PWM_RES);
  ledcAttach(IN4, PWM_FREQ, PWM_RES);

  WiFi.softAP(ssid, password);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html); });
  server.onNotFound([](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html); });
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();
}


void updateSonar() {
  if (millis() - lastSonarRead < SONAR_READ_INTERVAL) return;
  lastSonarRead = millis();

  float distance = getDistanceCM();
  String json;
  if (distance >= 0) {
    json = "{\"type\":\"sonar\",\"ok\":true,\"distance\":" + String(distance, 1) + "}";
  }

   else {
    json = "{\"type\":\"sonar\",\"ok\":false}";
  }
  ws.textAll(json);
}


void handleObstacleAvoidance() {
  if (!avoiding && lastDistance > 0 && lastDistance <= OBSTACLE_THRESHOLD_CM) {     //getting lastDistance from getDistanceCM and comparing it to the set threshold for turning
    avoiding = true;
    avoidStartTime = millis();      //when the rc car starts turning to avoid
  }

  if (avoiding) {
    setMotorSpeed(MAX_SPEED, -MAX_SPEED); 
    if (millis() - avoidStartTime >= AVOID_TURN_DURATION) {
      avoiding = false;
      targetSpeedLeft = 0;
      targetSpeedRight = 0;
    }
  } else {
    setMotorSpeed(targetSpeedLeft, targetSpeedRight); 
  }
}

void loop() {
  ws.cleanupClients();
  dnsServer.processNextRequest();

  if (scanState != SCAN_IDLE) {
    updateScan();
  } else {
    updateSonar();
  if (connected == true)
    handleObstacleAvoidance();
    }
}