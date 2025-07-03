const char html[] = R"=====(
<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5Atom Socket</title>
<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #0c1426 0%, #1a2332 50%, #0f1b2e 100%);
  color: white;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 20px;
  overflow: hidden;
}

.main-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 40px;
  max-width: 900px;
  width: 100%;
}

.measurements-container {
  display: flex;
  align-items: center;
  gap: 60px;
  justify-content: center;
}

.left-measurements {
  display: flex;
  flex-direction: column;
  gap: 30px;
}

.measurement-item {
  position: relative;
  width: 280px;
}

.measurement-label {
  font-size: 14px;
  font-weight: 300;
  letter-spacing: 2px;
  text-transform: uppercase;
  color: #64b5f6;
  margin-bottom: 8px;
  opacity: 0.8;
  text-align: center;
}

.measurement-display {
  display: flex;
  align-items: baseline;
  gap: 8px;
  justify-content: center;
}

.measurement-value {
  font-size: 4rem;
  font-weight: 300;
  line-height: 1;
  color: #ffffff;
  text-shadow: 0 0 20px rgba(100, 181, 246, 0.3);
}

.measurement-unit {
  font-size: 2rem;
  font-weight: 300;
  color: #64b5f6;
}

.voltage-line {
  height: 3px;
  background: linear-gradient(90deg, #00bcd4 0%, #64b5f6 100%);
  margin-top: 15px;
  border-radius: 2px;
  box-shadow: 0 0 10px rgba(0, 188, 212, 0.5);
}

.current-line {
  height: 3px;
  background: linear-gradient(90deg, #00bcd4 0%, #64b5f6 100%);
  margin-top: 15px;
  border-radius: 2px;
  box-shadow: 0 0 10px rgba(0, 188, 212, 0.5);
}

.power-section {
  display: flex;
  justify-content: center;
  align-items: center;
}

.power-circle {
  position: relative;
  width: 160px;
  height: 160px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.circle-outer {
  position: absolute;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: conic-gradient(from 0deg, #00bcd4 0%, #64b5f6 100%);
  padding: 3px;
}

.circle-middle {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: linear-gradient(135deg, #1a2332 0%, #0c1426 100%);
  padding: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.circle-inner {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: conic-gradient(from 0deg, #00bcd4 0%, #64b5f6 100%);
  padding: 3px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.circle-content {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: linear-gradient(135deg, #1a2332 0%, #0c1426 100%);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.power-value {
  font-size: 2rem;
  font-weight: 300;
  color: #ffffff;
  text-shadow: 0 0 20px rgba(100, 181, 246, 0.3);
}

.power-unit {
  font-size: 2rem;
  font-weight: 300;
  color: #64b5f6;
  margin-top: 3px;
}

.status-section {
  display: flex;
  align-items: center;
  gap: 20px;
  margin-top: 20px;
}

.status-label {
  font-size: 16px;
  font-weight: 300;
  letter-spacing: 1px;
  color: #64b5f6;
  opacity: 0.8;
}

.power-button {
  padding: 12px 24px;
  border: 2px solid #00bcd4;
  border-radius: 25px;
  background: transparent;
  color: #00bcd4;
  font-size: 16px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.3s ease;
  text-transform: uppercase;
  letter-spacing: 1px;
  min-width: 80px;
}

.power-button.on {
  background: linear-gradient(135deg, #00bcd4 0%, #64b5f6 100%);
  color: white;
  box-shadow: 0 0 20px rgba(0, 188, 212, 0.4);
}

.power-button.off {
  background: transparent;
  color: #636e72;
  border-color: #636e72;
}

.power-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 20px rgba(0, 188, 212, 0.6);
}

.device-info {
  position: absolute;
  top: 30px;
  left: 30px;
  font-size: 14px;
  color: #64b5f6;
  opacity: 0.7;
}

.device-info #deviceName {
  font-size: 20px;
  font-weight: 500;
  margin-bottom: 5px;
}

.glow-effect {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 300px;
  height: 300px;
  background: radial-gradient(circle, rgba(100, 181, 246, 0.1) 0%, transparent 70%);
  border-radius: 50%;
  animation: pulse 3s ease-in-out infinite;
  z-index: -1;
}

@keyframes pulse {
  0%, 100% {
    opacity: 0.3;
    transform: translate(-50%, -50%) scale(1);
  }
  50% {
    opacity: 0.6;
    transform: translate(-50%, -50%) scale(1.1);
  }
}

.ota-section {
  margin-top: 30px;
}

.ota-button {
  padding: 8px 16px;
  border: 1px solid #64b5f6;
  border-radius: 20px;
  background: transparent;
  color: #64b5f6;
  font-size: 12px;
  font-weight: 400;
  cursor: pointer;
  transition: all 0.3s ease;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.ota-button:hover {
  background: rgba(100, 181, 246, 0.1);
  transform: translateY(-1px);
}

@media (max-width: 768px) {
  .measurements-container {
    flex-direction: column;
    gap: 30px;
    align-items: center;
  }
  
  .measurement-item {
    width: 250px;
    text-align: center;
  }
  
  .measurement-value {
    font-size: 3rem;
  }
  
  .power-circle {
    width: 140px;
    height: 140px;
  }
  
  .power-value {
    font-size: 1.8rem;
  }
  
  .power-unit {
    font-size: 1.8rem;
  }
  
  .device-info {
    position: relative;
    top: auto;
    left: auto;
    text-align: center;
    margin-bottom: 20px;
  }
}
</style>
</head>
<body>
<div class="glow-effect"></div>

<div class="device-info">
  <div id="deviceName">%DEVICE_NAME%</div>
  <div>IP: <span id="ipAddress">取得中...</span></div>
</div>

<div class="main-container">
  <div class="measurements-container">
    <div class="left-measurements">
      <div class="measurement-item">
        <div class="measurement-label">VOLTAGE</div>
        <div class="measurement-display">
          <div class="measurement-value" id="voltage">--</div>
          <span class="measurement-unit">V</span>
        </div>
        <div class="voltage-line"></div>
      </div>
      
      <div class="measurement-item">
        <div class="measurement-label">CURRENT</div>
        <div class="measurement-display">
          <div class="measurement-value" id="current">--</div>
          <span class="measurement-unit">A</span>
        </div>
        <div class="current-line"></div>
      </div>
    </div>
    
    <div class="power-section">
      <div class="power-circle">
        <div class="circle-outer">
          <div class="circle-middle">
            <div class="circle-inner">
              <div class="circle-content">
                <div class="power-value" id="power">--</div>
                <div class="power-unit">W</div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
  
  <div class="status-section">
    <span class="status-label">STATUS</span>
    <button id="powerButton" class="power-button on" onclick="togglePower()">ON</button>
  </div>
  
  <div class="ota-section">
    <button class="ota-button" onclick="navigateToOTA()">OTA Update</button>
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
    button.style.opacity = '0.6';
    button.style.cursor = 'not-allowed';
  } else {
    button.style.opacity = '1';
    button.style.cursor = 'pointer';
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
</body>
</html>
)=====";

