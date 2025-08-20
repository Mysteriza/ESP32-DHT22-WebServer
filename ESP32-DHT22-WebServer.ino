#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include "DHT.h"

#define DHT_PIN 4
#define DHTTYPE DHT22

const char* AP_SSID="ESP32-Weather";
const char* AP_PASS="weather123";
const IPAddress AP_IP(192,168,4,1);
const IPAddress AP_GW(192,168,4,1);
const IPAddress AP_MASK(255,255,255,0);

WebServer server(80);
DHT dht(DHT_PIN, DHTTYPE);

volatile float curT=NAN, curH=NAN, curHI=NAN;
volatile bool sensorOK=false;
uint32_t lastRead=0, lastAPGuard=0, lastWdt=0, lastDbg=0;

const uint32_t READ_INTERVAL=10000;
const int LOG_MAX=120;
String logs[LOG_MAX];
int logHead=0,logCount=0;

void addLog(const String& s){ logs[logHead]=s; logHead=(logHead+1)%LOG_MAX; if(logCount<LOG_MAX) logCount++; Serial.println(s); }
String buildLogsText(){ String out; int n=logCount; out.reserve(n*24); int start=(logHead-logCount+LOG_MAX)%LOG_MAX; for(int i=0;i<n;i++){ int idx=(start+i)%LOG_MAX; out+=logs[idx]; out+='\n'; } return out; }

String formatUptime(){ unsigned long ms=millis(); unsigned long s=ms/1000; unsigned int d=s/86400; s%=86400; byte h=s/3600; s%=3600; byte m=s/60; s%=60; char buf[32]; snprintf(buf,sizeof(buf),"%ud %02u:%02u:%02lu",d,h,m,s); return String(buf); }

void startAP(){ WiFi.mode(WIFI_AP); WiFi.setSleep(false); esp_wifi_set_ps(WIFI_PS_NONE); WiFi.softAPConfig(AP_IP,AP_GW,AP_MASK); WiFi.softAP(AP_SSID,AP_PASS,6,0,4); }
bool apActive(){ if(WiFi.getMode()!=WIFI_MODE_AP) return false; IPAddress ip=WiFi.softAPIP(); return ip[0]|ip[1]|ip[2]|ip[3]; }
void ensureAP(){ if(!apActive()) startAP(); }

void readSensor(){
  if(millis()-lastRead<READ_INTERVAL) return;
  lastRead=millis();
  float h=dht.readHumidity();
  float t=dht.readTemperature();
  if(!isnan(h)&&!isnan(t)){ curT=t; curH=h; curHI=dht.computeHeatIndex(t,h,false); sensorOK=true; addLog(String("DHT22: OK  T=")+String(curT,1)+"C  H="+String(curH,1)+"%  HI="+String(curHI,1)+"C"); }
  else{ sensorOK=false; addLog("DHT22: ERROR"); }
}

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Weather</title>
<style>
:root{--bg:#0f1221;--card:#151935;--text:#e9edf1;--muted:#9aa3b2;--accent:#5ee1a5;--accent2:#6ea8ff;--warn:#ffb86b}
*{box-sizing:border-box}body{margin:0;background:linear-gradient(135deg,#0b0f1a,#121737 40%,#161b3c);color:var(--text);font-family:Inter,system-ui,Segoe UI,Roboto,Arial,sans-serif}
.container{max-width:1000px;margin:0 auto;padding:20px}
.header{display:flex;flex-direction:column;align-items:center;gap:6px;margin-bottom:12px;text-align:center}
.h-title{font-size:22px;font-weight:900;letter-spacing:.2px}
.h-sub{font-size:13px;color:var(--muted)}
.row{display:flex;gap:8px;flex-wrap:wrap;justify-content:center}
.pill{padding:6px 10px;border-radius:999px;background:#0e1330;border:1px solid #2a3170;color:#a9b6ff;font-size:12px}
.grid{display:grid;grid-template-columns:repeat(12,1fr);gap:14px}
.card{grid-column:span 12;background:var(--card);border:1px solid #24294e;border-radius:16px;padding:14px;box-shadow:0 10px 25px rgba(0,0,0,.25)}
.kpis{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
.kpi{background:#0e1330;border:1px solid #22285a;border-radius:14px;padding:16px;display:flex;flex-direction:column;min-height:120px}
.kpi h3{margin:0;font-size:13px;color:var(--muted);font-weight:800;text-align:center}
.val{margin-top:8px;font-weight:900;text-align:center;font-size:clamp(36px,9vw,56px);line-height:1.05;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.unit{margin-top:2px;text-align:center;font-size:clamp(14px,3.5vw,18px);color:#cfd6ff;font-weight:700}
.section-title{font-size:15px;font-weight:900;margin:6px 0 10px;color:#cbd6ff;text-align:center}
.kv{display:grid;grid-template-columns:1fr auto;gap:8px;font-size:14px}
.k{color:#9aa3b2}.v{font-weight:800}
.term{height:220px;overflow:auto;background:#0b1030;border:1px solid #2a3170;border-radius:12px;padding:12px;color:#cbd6ff;font-family:ui-monospace,Menlo,Consolas,monospace;white-space:pre-wrap}
.btn{appearance:none;border:1px solid #2a3170;background:#0e1330;color:#dce6ff;padding:9px 12px;border-radius:10px;font-weight:800;cursor:pointer;font-size:12px}
.btn:active{transform:translateY(1px)}
.footer{margin-top:8px;display:flex;justify-content:space-between;color:var(--muted);font-size:12px;gap:8px;flex-wrap:wrap}
@media (min-width:720px){.card.sys{grid-column:span 12}.card.termwrap{grid-column:span 12}}
</style></head>
<body>
<div class="container">
  <div class="header">
    <div class="h-title">ESP32 Weather Station</div>
    <div class="h-sub" id="sub">AP: — | IP: — | Clients: —</div>
    <div class="row"><span class="pill" id="upt">Uptime: —</span><span class="pill" id="heap">Heap: —</span><span class="pill" id="sen">Sensor: —</span></div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="kpis">
        <div class="kpi"><h3>Temperature</h3><div class="val" id="t">—</div><div class="unit">°C</div></div>
        <div class="kpi"><h3>Humidity</h3><div class="val" id="h">—</div><div class="unit">% RH</div></div>
        <div class="kpi"><h3>Heat Index</h3><div class="val" id="hi">—</div><div class="unit">°C</div></div>
      </div>
      <div class="footer"><div id="status">Status: —</div><div id="time">Last update: —</div></div>
    </div>

    <div class="card sys">
      <div class="section-title">System Stats</div>
      <div class="kv"><div class="k">Chip</div><div class="v" id="chip">—</div></div>
      <div class="kv"><div class="k">Cores</div><div class="v" id="cores">—</div></div>
      <div class="kv"><div class="k">Revision</div><div class="v" id="rev">—</div></div>
      <div class="kv"><div class="k">CPU Freq</div><div class="v" id="mhz">—</div></div>
      <div class="kv"><div class="k">Flash</div><div class="v" id="flash">—</div></div>
      <div class="kv"><div class="k">Heap Total</div><div class="v" id="heapTot">—</div></div>
      <div class="kv"><div class="k">Heap Used</div><div class="v" id="heapUsed">—</div></div>
      <div class="kv"><div class="k">Heap Free</div><div class="v" id="heapFree">—</div></div>
    </div>

    <div class="card termwrap">
      <div class="section-title" style="display:flex;justify-content:space-between;align-items:center">
        <span>Serial Monitor</span>
        <button class="btn" id="clear">Clear</button>
      </div>
      <div class="term" id="term"></div>
    </div>
  </div>
</div>
<script>
const el=(id)=>document.getElementById(id);
function safeVal(n){if(!Number.isFinite(n))return "—"; if(Math.abs(n)>1000) return "ERR"; return Math.round(n*10)/10}
async function fetchStatus(){
  try{
    const r=await fetch('/api/status'); const j=await r.json();
    el('t').textContent=safeVal(j.t); el('h').textContent=safeVal(j.h); el('hi').textContent=safeVal(j.hi);
    el('upt').textContent='Uptime: '+j.uptime; el('heap').textContent='Heap: '+j.heap_free_kb+' KB';
    el('sen').textContent='Sensor: '+(j.sensor_ok?'ONLINE':'OFFLINE');
    el('sub').textContent='AP: '+j.ap.ssid+' | IP: '+j.ap.ip+' | Clients: '+j.ap.clients;
    el('status').textContent='Status: '+j.msg; el('time').textContent='Last update: '+new Date().toLocaleTimeString();
    el('chip').textContent=j.sys.chip; el('cores').textContent=j.sys.cores; el('rev').textContent=j.sys.rev; el('mhz').textContent=j.sys.mhz+' MHz';
    el('flash').textContent=j.sys.flash_mb+' MB';
    el('heapTot').textContent=j.heap_total_kb+' KB'; el('heapFree').textContent=j.heap_free_kb+' KB';
    el('heapUsed').textContent=(j.heap_total_kb-j.heap_free_kb)+' KB';
  }catch(e){el('status').textContent='Status: fetch error'}
}
async function fetchLogs(){
  try{
    const r=await fetch('/api/logs'); const txt=await r.text();
    const term=el('term'); const atBottom=term.scrollTop+term.clientHeight>=term.scrollHeight-4;
    term.textContent=txt.trimEnd(); if(atBottom) term.scrollTop=term.scrollHeight;
  }catch(e){}
}
async function clearLogs(){ try{ await fetch('/api/clear',{method:'POST'}); el('term').textContent=''; }catch(e){} }
el('clear').addEventListener('click',clearLogs);
setInterval(fetchStatus,10000);
setInterval(fetchLogs,10000);
fetchStatus(); fetchLogs();
</script>
</body></html>
)HTML";

void handleRoot(){ server.send_P(200,"text/html",INDEX_HTML); }

void handleStatus(){
  size_t heapTotKB=ESP.getHeapSize()/1024;
  size_t heapFreeKB=ESP.getFreeHeap()/1024;
  String json; json.reserve(560);
  json+="{\"t\":"; json+=String(curT,1);
  json+=",\"h\":"; json+=String(curH,1);
  json+=",\"hi\":"; json+=String(curHI,1);
  json+=",\"uptime\":\""; json+=formatUptime(); json+="\"";
  json+=",\"heap_total_kb\":"; json+=heapTotKB;
  json+=",\"heap_free_kb\":"; json+=heapFreeKB;
  json+=",\"ap\":{";
  json+="\"ssid\":\""; json+=AP_SSID; json+="\"";
  json+=",\"ip\":\""; json+=WiFi.softAPIP().toString(); json+="\"";
  json+=",\"clients\":"; json+=WiFi.softAPgetStationNum(); json+="}";
  json+=",\"sys\":{";
  json+="\"chip\":\""; json+=ESP.getChipModel(); json+="\"";
  json+=",\"cores\":"; json+=ESP.getChipCores();
  json+=",\"rev\":"; json+=ESP.getChipRevision();
  json+=",\"mhz\":"; json+=ESP.getCpuFreqMHz();
  json+=",\"flash_mb\":"; json+=ESP.getFlashChipSize()/1024/1024; json+="}";
  json+=",\"sensor_ok\":"; json+=(sensorOK?1:0);
  json+=",\"msg\":\"ok\"}";
  server.send(200,"application/json",json);
}

void handleLogs(){ String txt=buildLogsText(); server.send(200,"text/plain",txt); }
void handleClear(){ logHead=0; logCount=0; server.send(200,"text/plain","OK"); }

void setup(){
  Serial.begin(115200);
  dht.begin();
  startAP();
  server.on("/",handleRoot);
  server.on("/api/status",handleStatus);
  server.on("/api/logs",handleLogs);
  server.on("/api/clear",HTTP_POST,handleClear);
  server.begin();
  esp_task_wdt_config_t twdt_config={ .timeout_ms=10000, .idle_core_mask=(1<<portNUM_PROCESSORS)-1, .trigger_panic=true };
  esp_task_wdt_init(&twdt_config);
  esp_task_wdt_add(NULL);
}

void loop(){
  server.handleClient();
  readSensor();
  if(millis()-lastAPGuard>3000){ lastAPGuard=millis(); ensureAP(); }
  if(millis()-lastWdt>1000){ lastWdt=millis(); esp_task_wdt_reset(); }
  if(millis()-lastDbg>10000){ lastDbg=millis(); }
  delay(1);
}
