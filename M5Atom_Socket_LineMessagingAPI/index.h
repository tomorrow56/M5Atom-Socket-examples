const char html[] = R"=====(
<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5Atom Socket</title>
<style>
body {
  margin: 0;
  padding: 20px;
  font-family: 'Arial', sans-serif;
  background-color: #f0f0f0;
  display: flex;
  flex-direction: column;
  align-items: center;
  min-height: 100vh;
}

.container {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 20px;
}

.device-info {
  background-color: #ffeaa7;
  border: 2px solid #ddd;
  border-radius: 15px;
  padding: 20px;
  width: 300px;
  box-shadow: 0 4px 8px rgba(0,0,0,0.1);
}

.device-name {
  font-size: 18px;
  font-weight: bold;
  margin-bottom: 10px;
  color: #2d3436;
}

.info-line {
  margin: 8px 0;
  font-size: 16px;
  color: #2d3436;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.info-line.ip {
  justify-content: flex-start;
  gap: 10px;
}

.measurement-line {
  margin: 8px 0;
  font-size: 16px;
  color: #2d3436;
  display: grid;
  grid-template-columns: 80px 1fr 20px;
  align-items: center;
}

.label {
  text-align: left;
}

.value {
  font-weight: bold;
  text-align: right;
}

.unit {
  text-align: left;
  margin-left: 5px;
}

.status-section {
  margin-top: 10px;
  display: flex;
  align-items: center;
  gap: 15px;
  justify-content: center;
}

.status-label {
  font-size: 18px;
  font-weight: bold;
  color: #2d3436;
}

.power-button {
  padding: 10px 20px;
  border: none;
  border-radius: 8px;
  font-size: 16px;
  font-weight: bold;
  cursor: pointer;
  transition: all 0.3s ease;
  min-width: 80px;
}

.power-button.on {
  background-color: #00b894;
  color: white;
}

.power-button.off {
  background-color: #636e72;
  color: white;
}

.power-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0,0,0,0.2);
}

.power-button:active {
  transform: translateY(0);
}

.loading {
  opacity: 0.6;
  cursor: not-allowed;
}

@media (max-width: 480px) {
  .device-info {
    width: 90%;
    margin: 10px;
  }
  
  .status-section {
    flex-direction: column;
    gap: 10px;
  }
  
  .measurement-line {
    grid-template-columns: 70px 1fr 20px;
  }
}
</style>
</head>
<body>
<div class="container">
  <div class="device-info">
    <div class="device-name" id="deviceName">%DEVICE_NAME%</div>
    <div class="info-line ip">
      <span>IP adrs:</span>
      <span class="value" id="ipAddress">取得中...</span>
    </div>
    <div class="measurement-line">
      <span class="label">Voltage:</span>
      <span class="value" id="voltage">--</span>
      <span class="unit">V</span>
    </div>
    <div class="measurement-line">
      <span class="label">Current:</span>
      <span class="value" id="current">--</span>
      <span class="unit">A</span>
    </div>
    <div class="measurement-line">
      <span class="label">Power:</span>
      <span class="value" id="power">--</span>
      <span class="unit">W</span>
    </div>
  </div>
  
  <div class="status-section">
    <span class="status-label">STATUS</span>
    <button id="powerButton" class="power-button on" onclick="togglePower()">ON</button>
  </div>
</div>

<script>
var httpRequest;
var currentState = false; // false = OFF, true = ON
var isLoading = false;

function updateDisplay(data) {
  // データの解析
  var parts = data.split('#');
  
  // 各部分から値を抽出
  parts.forEach(function(part) {
    if (part.includes('vol:')) {
      var voltage = part.replace('vol:<mark>', '').replace('</mark>V', '');
      document.getElementById('voltage').textContent = voltage;
    } else if (part.includes('current:')) {
      var current = part.replace('current:<mark>', '').replace('</mark>A', '');
      document.getElementById('current').textContent = current;
    } else if (part.includes('power:')) {
      var power = part.replace('power:<mark>', '').replace('</mark>W', '');
      document.getElementById('power').textContent = power;
    } else if (part.includes('state:')) {
      var state = part.replace('state:<mark>', '').replace('</mark>', '');
      currentState = (state === 'on');
      updateButtonState();
    }
  });
}

function updateButtonState() {
  var button = document.getElementById('powerButton');
  if (currentState) {
    button.textContent = 'ON';
    button.className = 'power-button on';
  } else {
    button.textContent = 'OFF';
    button.className = 'power-button off';
  }
  
  if (isLoading) {
    button.classList.add('loading');
  } else {
    button.classList.remove('loading');
  }
}

function handleResponse() {
  if (httpRequest.readyState === XMLHttpRequest.DONE) {
    isLoading = false;
    if (httpRequest.status === 200) {
      updateDisplay(httpRequest.responseText);
    }
    updateButtonState();
  }
}

function togglePower() {
  if (isLoading) return;
  
  isLoading = true;
  updateButtonState();
  
  var endpoint = currentState ? '/off' : '/on';
  
  if (window.XMLHttpRequest) {
    httpRequest = new XMLHttpRequest();
  } else if (window.ActiveXObject) {
    httpRequest = new ActiveXObject("Microsoft.XMLHTTP");
  }
  
  httpRequest.open("GET", endpoint, true);
  httpRequest.onreadystatechange = handleResponse;
  httpRequest.send();
}

function getData() {
  if (window.XMLHttpRequest) {
    httpRequest = new XMLHttpRequest();
  } else if (window.ActiveXObject) {
    httpRequest = new ActiveXObject("Microsoft.XMLHTTP");
  }
  
  httpRequest.open("GET", "/data", true);
  httpRequest.onreadystatechange = handleResponse;
  httpRequest.send();
}

// IPアドレスを取得して表示
function getIPAddress() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/ip", true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
      document.getElementById('ipAddress').textContent = xhr.responseText;
    }
  };
  xhr.send();
}

// 初期化
document.addEventListener('DOMContentLoaded', function() {
  getData();
  getIPAddress();
  
  // 定期的にデータを更新（5秒間隔）
  setInterval(getData, 5000);
});

function navigateToOTA() {
  window.location.href = '/update';
}
</script>
<div style="margin-top: 20px;">
  <button class="power-button on" onclick="navigateToOTA()">OTA Update</button>
</div>
</body>
</html>
)=====";

