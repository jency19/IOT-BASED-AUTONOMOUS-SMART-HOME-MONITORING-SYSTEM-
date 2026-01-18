#include <WiFi.h>
#include <WebServer.h>

// ---------------- ACCESS POINT ----------------
const char *ap_ssid = "ESP32-Dashboard";
const char *ap_password = "12345678";  // min 8 chars

// ---------------- GAS SENSOR ----------------
int gasAnalog = 34;
int buzzerPin = 27;
int gasThreshold = 1500;
int gasValue = 0;

// ---------------- PIR SENSOR ----------------
#define PIR_PIN 13
#define PIR_LED 25
int pirValue = 0;

// ---------------- SOUND SENSOR ----------------
#define SOUND_DO 32
#define LED_PIN 2
#define RELAY_PIN 18
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 300;
int lastSound = LOW;
bool ledOn = false;   // ✅ changed Fan → LED

// ---------------- ULTRASONIC ----------------
#define TRIG_PIN 14
#define ECHO_PIN 33
#define STEP_ALERT_LED 19
const int safeDistance = 10;
long distanceCm = 0;

// ---------------- TIMERS ----------------
unsigned long lastGasCheck = 0;
unsigned long lastPirCheck = 0;
unsigned long lastUltraCheck = 0;
const unsigned long sensorInterval = 500;
const unsigned long ultraInterval = 200;

// ---------------- Web Server ----------------
WebServer server(80);

// ---------------- HELPER: Ultrasonic ----------------
long readUltrasonicCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// ---------------- HTML PAGE ----------------
// ---------------- HTML PAGE ----------------
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>ESP32 Multi-Sensor Dashboard</title>";
  page += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  page += "<script src='https://cdn.jsdelivr.net/npm/chartjs-plugin-datalabels'></script>";
  page += "<style>";
  page += "body{font-family:Poppins,Arial;background:#0f172a;color:#f8fafc;margin:0;padding:20px;}";
  page += "h1{text-align:center;color:#38bdf8;margin-bottom:20px;}";
  page += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;}";
  page += ".card{background:#1e293b;padding:20px;border-radius:16px;box-shadow:0 4px 10px rgba(0,0,0,0.4);transition:0.3s;}";
  page += ".card:hover{transform:translateY(-6px);}h2{margin-top:0;font-size:18px;color:#facc15;}";
  page += ".badge{padding:6px 12px;border-radius:12px;font-weight:bold;} .ok{background:#16a34a;} .alert{background:#dc2626;} .info{background:#2563eb;}";
  page += "canvas{max-width:100%;}";
  page += ".gauge{width:200px;height:200px;margin:auto;}";
  page += "</style>";

  // ---------- JS ----------
  page += "<script>let gasData=[],ultraData=[],labels=[];let waterLevel=0;";
  page += "function updateCharts(){gasChart.data.labels=labels;gasChart.data.datasets[0].data=gasData;ultraChart.data.labels=labels;ultraChart.data.datasets[0].data=ultraData;gasChart.update();ultraChart.update();gaugeChart.data.datasets[0].data=[waterLevel,100-waterLevel];gaugeChart.update();}";
  page += "setInterval(()=>{fetch('/data').then(r=>r.json()).then(d=>{";
  page += "document.getElementById('gas').innerHTML=d.gas;";
  page += "document.getElementById('gasstatus').innerHTML=d.gasStatus;";
  page += "document.getElementById('pir').innerHTML=d.pir;";
  page += "document.getElementById('sound').innerHTML=d.sound;";
  page += "document.getElementById('ultra').innerHTML=d.ultra;";
  page += "document.getElementById('led').innerHTML=d.led;";
  page += "document.getElementById('time').innerHTML=new Date().toLocaleTimeString();";
  page += "labels.push('');if(labels.length>20){labels.shift();gasData.shift();ultraData.shift();}";
  page += "gasData.push(d.gas);ultraData.push(d.ultra);";
  page += "let maxDepth=50;waterLevel=Math.max(0,Math.min(100,100-((d.ultra/maxDepth)*100)));";
  page += "updateCharts();";
  page += "});},1000);</script></head><body>";

  // ---------- BODY ----------
  page += "<h1>🌙 ESP32 Multi-Sensor Dashboard</h1>";
  page += "<p style='text-align:center'>Last Update: <span id='time'>--</span></p>";
  page += "<div class='grid'>";

  page += "<div class='card'><h2>Gas Sensor</h2><p>Value: <span id='gas'>--</span></p><p>Status: <span id='gasstatus' class='badge'>--</span></p><canvas id='gasChart'></canvas></div>";
  page += "<div class='card'><h2>PIR Motion</h2><p><span id='pir' class='badge info'>--</span></p></div>";
  page += "<div class='card'><h2>Sound Sensor</h2><p><span id='sound' class='badge info'>--</span></p></div>";
  page += "<div class='card'><h2>Ultrasonic Distance</h2><p><span id='ultra'>--</span> cm</p><canvas id='ultraChart'></canvas></div>";
  page += "<div class='card'><h2>Water Level</h2><div class='gauge'><canvas id='gaugeChart'></canvas></div><p><b><span id='water'>--</span>%</b></p></div>";
  page += "<div class='card'><h2>LED (Sound Controlled)</h2><p>Status: <span id='led' class='badge'>--</span></p></div>";

  page += "</div>";

  // ---------- CHARTS ----------
  page += "<script>";
  page += "const gasCtx=document.getElementById('gasChart').getContext('2d');";
  page += "const ultraCtx=document.getElementById('ultraChart').getContext('2d');";
  page += "var gasChart=new Chart(gasCtx,{type:'line',data:{labels:[],datasets:[{label:'Gas Value',data:[],borderColor:'#ef4444',backgroundColor:'rgba(239,68,68,0.3)',fill:true}]},options:{scales:{y:{beginAtZero:true}}}});";
  page += "var ultraChart=new Chart(ultraCtx,{type:'line',data:{labels:[],datasets:[{label:'Distance (cm)',data:[],borderColor:'#3b82f6',backgroundColor:'rgba(59,130,246,0.3)',fill:true}]},options:{scales:{y:{beginAtZero:true}}}});";

  // Gauge chart
  page += "const gaugeCtx=document.getElementById('gaugeChart').getContext('2d');";
  page += "var gaugeChart=new Chart(gaugeCtx,{type:'doughnut',data:{labels:['Water','Empty'],datasets:[{data:[0,100],backgroundColor:['#22c55e','#1e293b'],borderWidth:0}]},options:{rotation:-90,circumference:180,plugins:{legend:{display:false},datalabels:{display:true}}}});";
  page += "</script>";

  page += "</body></html>";
  return page;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(PIR_PIN, INPUT_PULLUP);
  pinMode(PIR_LED, OUTPUT);
  pinMode(SOUND_DO, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(STEP_ALERT_LED, OUTPUT);

  digitalWrite(STEP_ALERT_LED, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);

  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Access Point started!");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send(200, "text/html", htmlPage());
  });

  server.on("/data", []() {
    String gasStatus = (gasValue > gasThreshold) ? "<span class='badge alert'>ALERT!</span>" : "<span class='badge ok'>Normal</span>";
    String pirStatus = (pirValue == HIGH) ? "Motion Detected" : "No Motion";
    String soundStatus = ledOn ? "Clap Detected (LED ON)" : "No Clap (LED OFF)";
    String ledStatus = ledOn ? "<span class='badge ok'>ON</span>" : "<span class='badge alert'>OFF</span>";

    String json = "{";
    json += "\"gas\":" + String(gasValue) + ",";
    json += "\"gasStatus\":\"" + gasStatus + "\",";
    json += "\"pir\":\"" + pirStatus + "\",";
    json += "\"sound\":\"" + soundStatus + "\",";
    json += "\"ultra\":" + String(distanceCm) + ",";
    json += "\"led\":\"" + ledStatus + "\"}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

// ---------------- LOOP ----------------
void loop() {
  unsigned long now = millis();
  server.handleClient();

  // Sound (toggle LED)
  int state = digitalRead(SOUND_DO);
  if (state == HIGH && lastSound == LOW && (now - lastDebounce) > debounceDelay) {
    ledOn = !ledOn;
    digitalWrite(RELAY_PIN, ledOn ? LOW : HIGH);
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    lastDebounce = now;
  }
  lastSound = state;

  // Gas
  if (now - lastGasCheck >= sensorInterval) {
    gasValue = analogRead(gasAnalog);
    digitalWrite(buzzerPin, (gasValue > gasThreshold) ? HIGH : LOW);
    lastGasCheck = now;
  }

  // PIR
  if (now - lastPirCheck >= sensorInterval) {
    pirValue = digitalRead(PIR_PIN);
    digitalWrite(PIR_LED, pirValue == HIGH ? HIGH : LOW);
    lastPirCheck = now;
  }

  // Ultrasonic
  if (now - lastUltraCheck >= ultraInterval) {
    distanceCm = readUltrasonicCm();
    digitalWrite(STEP_ALERT_LED, (distanceCm > 0 && distanceCm <= safeDistance) ? HIGH : LOW);
    lastUltraCheck = now;
  }
}