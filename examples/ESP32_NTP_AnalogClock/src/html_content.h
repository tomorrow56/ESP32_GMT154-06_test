#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

// テスト用の簡素なHTML
const char TEST_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Test</title>
</head>
<body>
  <h1>ESP32 Web Server Test</h1>
  <p>This is a test page.</p>
  <p>If you can see this, the web server is working correctly.</p>
  <a href="/">Go to main page</a>
</body>
</html>
)rawliteral";

// 設定ページのHTMLコンテンツ
const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>ESP32 Analog Clock Setup</title>
  <style>
    body { font-family: Arial, Helvetica, sans-serif; background-color: #f2f2f2; margin: 0; padding: 20px; color: #333; }
    h1 { color: #0066cc; text-align: center; }
    .card { background-color: white; box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2); border-radius: 5px; padding: 20px; margin-bottom: 20px; }
    .button { background-color: #0066cc; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 4px; }
    input[type=text], input[type=password] { width: 100%; padding: 12px 20px; margin: 8px 0; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
    label { font-weight: bold; }
    .success { color: green; }
    .error { color: red; }
  </style>
</head>
<body>
  <h1>ESP32 Analog Clock Setup</h1>
  
  <div class="card">
    <h2>WiFi Settings</h2>
    <form id="wifiForm">
      <label for="ssid">WiFi SSID:</label>
      <input type="text" id="ssid" name="ssid" value="%SSID%">
      <label for="password">WiFi Password:</label>
      <input type="password" id="password" name="password" value="%PASSWORD%">
      <button type="submit" class="button">Save WiFi Settings</button>
    </form>
    <p id="wifiMessage"></p>
  </div>

  <div class="card">
    <h2>OTA Update Settings</h2>
    <form id="otaForm">
      <label for="otaUsername">Username:</label>
      <input type="text" id="otaUsername" name="otaUsername" value="%OTA_USERNAME%">
      <label for="otaPassword">Password:</label>
      <input type="password" id="otaPassword" name="otaPassword" value="%OTA_PASSWORD%">
      <button type="submit" class="button">Save OTA Settings</button>
    </form>
    <p id="otaMessage"></p>
  </div>

  <div class="card">
    <h2>Device Control</h2>
    <button id="restartBtn" class="button">Restart</button>
    <p id="restartMessage"></p>
  </div>

  <script>
    document.getElementById('wifiForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const ssid = document.getElementById('ssid').value;
      const password = document.getElementById('password').value;
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/save-wifi');
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onload = function() {
        if (xhr.status === 200) {
          document.getElementById('wifiMessage').innerHTML = '<span class="success">WiFi settings saved. Please restart the device.</span>';
        } else {
          document.getElementById('wifiMessage').innerHTML = '<span class="error">Error: ' + xhr.responseText + '</span>';
        }
      };
      xhr.send(JSON.stringify({ssid: ssid, password: password}));
    });

    document.getElementById('otaForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const otaUsername = document.getElementById('otaUsername').value;
      const otaPassword = document.getElementById('otaPassword').value;
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/save-ota');
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onload = function() {
        if (xhr.status === 200) {
          document.getElementById('otaMessage').innerHTML = '<span class="success">OTA settings saved.</span>';
        } else {
          document.getElementById('otaMessage').innerHTML = '<span class="error">Error: ' + xhr.responseText + '</span>';
        }
      };
      xhr.send(JSON.stringify({otaUsername: otaUsername, otaPassword: otaPassword}));
    });

    document.getElementById('restartBtn').addEventListener('click', function() {
      if (confirm('Are you sure you want to restart?')) {
        document.getElementById('restartMessage').innerHTML = '<span class="success">Restarting... Please wait</span>';
        const xhr = new XMLHttpRequest();
        xhr.open('GET', '/restart');
        xhr.send();
      }
    });
  </script>
</body>
</html>
)rawliteral";

// メインページのHTMLコンテンツ（通常モード時）
const char MAIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Analog Clock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial;
      margin: 20px;
      text-align: center;
    }
    .btn {
      background: #0066cc;
      color: white;
      padding: 10px 20px;
      text-decoration: none;
      border-radius: 4px;
      display: inline-block;
      margin: 10px;
    }
  </style>
</head>
<body>
  <h1>ESP32 NTP Analog Clock</h1>
  <p>Current Status: Running</p>
  <p>SSID: %SSID%</p>
  <p>IP Address: %IP%</p>
  <a href='/update' class='btn'>Go to OTA Update</a>
  <a href='/setup' class='btn'>WiFi Settings</a>
  <a href='/restart' class='btn'>Restart</a>
</body>
</html>
)rawliteral";

// 再起動ページのHTMLコンテンツ
const char RESTART_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <title>再起動中</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial;
      margin: 20px;
      text-align: center;
    }
  </style>
</head>
<body>
  <h1>再起動中...</h1>
  <p>3秒後にトップページに移動します</p>
  <script>
    setTimeout(function(){
      window.location.href='/';
    }, 3000);
  </script>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H
