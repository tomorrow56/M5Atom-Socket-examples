const char settings_html[] = R"=====(
<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5Atom Socket Settings</title>
<style>
  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #0c1426 0%, #1a2332 50%, #0f1b2e 100%);
    color: white;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    padding: 20px;
  }
  .container {
    background-color: #1a2332;
    padding: 30px;
    border-radius: 10px;
    box-shadow: 0 0 20px rgba(0, 188, 212, 0.4);
    width: 100%;
    max-width: 500px;
    text-align: center;
  }
  h1 {
    color: #00bcd4;
    margin-bottom: 30px;
    font-size: 24px;
  }
  .setting-item {
    margin-bottom: 20px;
    text-align: left;
  }
  label {
    display: block;
    margin-bottom: 8px;
    color: #64b5f6;
    font-size: 14px;
    text-transform: uppercase;
    letter-spacing: 1px;
  }
  input[type="number"], input[type="text"] {
    width: calc(100% - 20px);
    padding: 10px;
    border: 1px solid #00bcd4;
    border-radius: 5px;
    background-color: #0f1b2e;
    color: white;
    font-size: 16px;
    outline: none;
  }
  input[type="number"]:focus, input[type="text"]:focus {
    border-color: #64b5f6;
    box-shadow: 0 0 8px rgba(100, 181, 246, 0.6);
  }
  input[type="number"]:disabled, input[type="text"]:disabled {
    background-color: #2a3441;
    border-color: #636e72;
    color: #636e72;
    cursor: not-allowed;
  }
  .checkbox-container {
    display: flex;
    align-items: center;
    margin-bottom: 15px;
    gap: 10px;
  }
  .checkbox-wrapper {
    position: relative;
    display: inline-block;
    flex-shrink: 0;
  }
  input[type="checkbox"] {
    opacity: 0;
    position: absolute;
    cursor: pointer;
    height: 20px;
    width: 20px;
  }
  .checkmark {
    position: relative;
    display: block;
    height: 20px;
    width: 20px;
    background-color: #0f1b2e;
    border: 2px solid #00bcd4;
    border-radius: 3px;
    transition: all 0.3s ease;
  }
  .checkbox-wrapper:hover input ~ .checkmark {
    background-color: rgba(0, 188, 212, 0.1);
  }
  .checkbox-wrapper input:checked ~ .checkmark {
    background-color: #00bcd4;
    border-color: #00bcd4;
  }
  .checkmark:after {
    content: "";
    position: absolute;
    display: none;
  }
  .checkbox-wrapper input:checked ~ .checkmark:after {
    display: block;
  }
  .checkbox-wrapper .checkmark:after {
    left: 6px;
    top: 2px;
    width: 5px;
    height: 10px;
    border: solid white;
    border-width: 0 3px 3px 0;
    transform: rotate(45deg);
  }
  .checkbox-label {
    color: #64b5f6;
    font-size: 14px;
    text-transform: uppercase;
    letter-spacing: 1px;
    cursor: pointer;
    margin: 0;
    line-height: 20px;
    user-select: none;
  }
  .ambient-section {
    border: 1px solid #636e72;
    border-radius: 8px;
    padding: 15px;
    margin-bottom: 20px;
    background-color: rgba(15, 27, 46, 0.5);
  }
  .ambient-section.enabled {
    border-color: #00bcd4;
    background-color: rgba(0, 188, 212, 0.1);
  }
  .section-title {
    color: #00bcd4;
    font-size: 16px;
    font-weight: 500;
    margin-bottom: 15px;
    text-transform: uppercase;
    letter-spacing: 1px;
  }
  .api-button-group {
    margin-top: 20px;
    margin-bottom: 10px;
    display: flex;
    justify-content: center;
  }
  .button-group {
    margin-top: 30px;
    display: flex;
    justify-content: space-around;
  }
  button {
    padding: 10px 20px;
    border: none;
    border-radius: 25px;
    font-size: 16px;
    cursor: pointer;
    transition: all 0.3s ease;
    text-transform: uppercase;
    letter-spacing: 1px;
  }
  button.ok {
    background: linear-gradient(135deg, #00bcd4 0%, #64b5f6 100%);
    color: white;
    box-shadow: 0 0 15px rgba(0, 188, 212, 0.4);
  }
  button.cancel {
    background-color: #636e72;
    color: white;
  }
  button.api-key {
    background: linear-gradient(135deg, #4caf50 0%, #66bb6a 100%);
    color: white;
    box-shadow: 0 0 15px rgba(76, 175, 80, 0.4);
    padding: 12px 24px;
  }
  button:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 20px rgba(0, 188, 212, 0.6);
  }
</style>
</head>
<body>
  <div class="container">
    <h1>Settings</h1>
    
    <div class="setting-item">
      <label for="deviceName">Device Name</label>
      <input type="text" id="deviceName" value="M5Atom Socket" maxlength="31">
    </div>
    
    <div class="setting-item">
      <label for="currentTH">Current Threshold (A)</label>
      <input type="number" id="currentTH" step="0.01" value="0.3">
    </div>
    
    <div class="setting-item">
      <label for="powerOffDuration">Power Off Duration (minutes)</label>
      <input type="number" id="powerOffDuration" step="1" value="60">
    </div>
    
    <div class="setting-item">
      <div class="checkbox-container">
        <div class="checkbox-wrapper">
          <input type="checkbox" id="autoOffEnabled">
          <span class="checkmark"></span>
        </div>
        <label for="autoOffEnabled" class="checkbox-label">Enable Auto OFF</label>
      </div>
    </div>
    
    <div class="ambient-section" id="ambientSection">
      <div class="section-title">Ambient Settings</div>
      
      <div class="checkbox-container">
        <div class="checkbox-wrapper">
          <input type="checkbox" id="ambientEnabled">
          <span class="checkmark"></span>
        </div>
        <label for="ambientEnabled" class="checkbox-label">Enable Ambient</label>
      </div>
      
      <div class="setting-item">
        <label for="ambientChannelId">Channel ID</label>
        <input type="number" id="ambientChannelId" value="0" disabled>
      </div>
      
      <div class="setting-item">
        <label for="ambientWriteKey">Write Key</label>
        <input type="text" id="ambientWriteKey" value="" maxlength="32" disabled>
      </div>
    </div>
    
    <div class="api-button-group">
      <button class="api-key" onclick="navigateToApiKey()">LINE API KEY</button>
    </div>
    
    <div class="button-group">
      <button class="ok" onclick="saveSettings()">OK</button>
      <button class="cancel" onclick="cancelSettings()">Cancel</button>
    </div>
  </div>

  <script>
    function updateAmbientInputs() {
      const enabled = document.getElementById('ambientEnabled').checked;
      const channelInput = document.getElementById('ambientChannelId');
      const keyInput = document.getElementById('ambientWriteKey');
      const section = document.getElementById('ambientSection');
      
      channelInput.disabled = !enabled;
      keyInput.disabled = !enabled;
      
      if (enabled) {
        section.classList.add('enabled');
      } else {
        section.classList.remove('enabled');
      }
    }

    function loadSettings() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/getsettings", true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
          var settings = JSON.parse(xhr.responseText);
          document.getElementById("deviceName").value = settings.deviceName || "M5Atom Socket";
          document.getElementById("currentTH").value = settings.currentTH;
          document.getElementById("powerOffDuration").value = settings.powerOffDuration;
          document.getElementById("autoOffEnabled").checked = settings.autoOffEnabled || false;
          document.getElementById("ambientEnabled").checked = settings.ambientEnabled || false;
          document.getElementById("ambientChannelId").value = settings.ambientChannelId || 0;
          document.getElementById("ambientWriteKey").value = settings.ambientWriteKey || "";
          updateAmbientInputs();
        }
      };
      xhr.send();
    }

    function saveSettings() {
      var deviceName = document.getElementById("deviceName").value;
      var currentTH = document.getElementById("currentTH").value;
      var powerOffDuration = document.getElementById("powerOffDuration").value;
      var autoOffEnabled = document.getElementById("autoOffEnabled").checked;
      var ambientEnabled = document.getElementById("ambientEnabled").checked;
      var ambientChannelId = document.getElementById("ambientChannelId").value;
      var ambientWriteKey = document.getElementById("ambientWriteKey").value;
      
      var xhr = new XMLHttpRequest();
      xhr.open("POST", "/savesettings", true);
      xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
      xhr.onreadystatechange = function() {
        if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
          window.location.href = "/"; // Return to main page
        }
      };
      
      var params = "deviceName=" + encodeURIComponent(deviceName) +
                   "&currentTH=" + currentTH +
                   "&powerOffDuration=" + powerOffDuration +
                   "&autoOffEnabled=" + autoOffEnabled +
                   "&ambientEnabled=" + ambientEnabled +
                   "&ambientChannelId=" + ambientChannelId +
                   "&ambientWriteKey=" + encodeURIComponent(ambientWriteKey);
      
      xhr.send(params);
    }

    function cancelSettings() {
      window.location.href = "/"; // Return to main page
    }

    function navigateToApiKey() {
      window.location.href = "/apikey"; // Navigate to LINE API KEY confirmation page
    }

    // Add event listeners
    document.addEventListener("DOMContentLoaded", function() {
      loadSettings();
      document.getElementById('ambientEnabled').addEventListener('change', updateAmbientInputs);
    });
  </script>
</body>
</html>
)=====";


