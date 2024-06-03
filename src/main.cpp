#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <EEPROM.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <cmath>
#include <unordered_map>
#include <NewPing.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <EasyBuzzer.h>

// Access point config
const char *SSID = "287707_ASG2";
const char *PASSWORD = "2877073113";

// Timeout for attempting to connect network in milliseconds
const unsigned long CONNECTION_TIMEOUT_MS = 10 * 1000; // 10 seconds
unsigned long previousMillis = 0;

bool isScanning = false;

// Set web webServer port number to 80 and ws as web socket url
AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

// Minified Static files for web webServer
const char *html = R"###(
<!doctypehtml><html lang="en"><meta charset="UTF-8"><title>ESP TEST</title><meta name="viewport"content="width=device-width,initial-scale=1"><link rel="stylesheet"type="text/css"href="style.min.css"><header id="page-header"><h1>ESP Distance Sensing LED Web Server</h1></header><form id="configForm"method="post"action="/submitForm"class="d-flex flex-column"style="flex-grow: 1; position: relative;"><main><div class="card"><div class="collapse-toggle d-flex align-items-center justify-content-center card-header"data-toggle="#credentialForm"><h2 style="margin-right: auto">WiFi Credential</h2><span class="collapse-icon">&#9650;</span></div><div id="credentialForm"class="collapse-content"><div class="card-body"><button id="refreshNetworks"type="button"class="btn btn-primary">Refresh</button><table id="networkTable"><thead><tr><th scope="col">Available Networks<tbody></table><label for="ssid">SSID:</label><br><input id="ssid"name="ssid"required> <label for="deviceID">Device ID:</label><br><input id="deviceID"name="deviceID"required> <label for="password">Password:</label><br><input type="password"id="password"name="password"autocomplete="new=password"></div></div></div><div class="card"style="margin-top: 1.5rem"><div class="collapse-toggle d-flex align-items-center justify-content-center card-header"data-toggle="#ledControl"><h2 style="margin-right: auto">Control</h2><span class="collapse-icon">&#9650;</span></div><div id="ledControl"class="collapse-content"><div id="ledList"class="card-body"><div><span><b>Note:</b></span><ol><li>Trigger ranges must have value such that GREEN > YELLLOW > RED.<li>Press and release to toggle the device.<li>Long press 5 seconds and release to erase saved configuration.</ol></div><div class="d-flex align-items-center justify-content-center"><span>LED #1 - GPIO%RED%</span> <span class="brightness-box red">RED</span> <span>Brightness (0 - %MAXBRIGHTNESS%)</span><div class="slideContainer d-flex align-items-center justify-content-center"><input type="range"min="0"max="%MAXBRIGHTNESS%"value="%MAXBRIGHTNESS%"step="1"class="slider brightness-slider"name="ledBrightness0"> <input type="number"inputmode="numeric"min="0"max="%MAXBRIGHTNESS%"value="%MAXBRIGHTNESS%"step="1"class="brightness-number"></div><span>Trigger Range (%MINRANGE% - %MAXRANGE%)</span><div class="slideContainer d-flex align-items-center justify-content-center"><input type="range"min="%MINRANGE%"max="%MAXRANGE%"value="%MAXRANGE%"step="1"class="slider"name="ledTriggerRange0"> <input type="number"inputmode="numeric"min="%MINRANGE%"max="%MAXRANGE%"value="%MAXRANGE%"step="1"></div></div><div class="d-flex align-items-center justify-content-center"><span>LED #1 - GPIO%YELLOW%</span> <span class="brightness-box yellow">YELLOW</span> <span>Brightness (0 - %MAXBRIGHTNESS%)</span><div class="slideContainer d-flex align-items-center justify-content-center"><input type="range"min="0"max="%MAXBRIGHTNESS%"value="%MAXBRIGHTNESS%"step="1"class="slider brightness-slider"name="ledBrightness1"> <input type="number"inputmode="numeric"min="0"max="%MAXBRIGHTNESS%"value="%MAXBRIGHTNESS%"step="1"class="brightness-number"></div><span>Trigger Range (%MINRANGE% - %MAXRANGE%)</span><div class="slideContainer d-flex align-items-center justify-content-center"><input type="range"min="%MINRANGE%"max="%MAXRANGE%"value="%MAXRANGE%"step="1"class="slider"name="ledTriggerRange1"> <input type="number"inputmode="numeric"min="%MINRANGE%"max="%MAXRANGE%"value="%MAXRANGE%"step="1"></div></div><div class="d-flex align-items-center justify-content-center"><span>LED #1 - GPIO%GREEN%</span>12 <span class="brightness-box green">GREEN</span> <span>Brightness (0 - %MAXBRIGHTNESS%)</span><div class="slideContainer d-flex align-items-center justify-content-center"><input type="range"min="0"max="%MAXBRIGHTNESS%"value="%MAXBRIGHTNESS%"step="1"class="slider brightness-slider"name="ledBrightness2"> <input type="number"inputmode="numeric"min="0"max="%MAXBRIGHTNESS%"value="%MAXBRIGHTNESS%"step="1"class="brightness-number"></div><span>Trigger Range (%MINRANGE% - %MAXRANGE%)</span><div class="slideContainer d-flex align-items-center justify-content-center"><input type="range"min="%MINRANGE%"max="%MAXRANGE%"value="%MAXRANGE%"step="1"class="slider"name="ledTriggerRange2"> <input type="number"inputmode="numeric"min="%MINRANGE%"max="%MAXRANGE%"value="%MAXRANGE%"step="1"></div></div></div></div></div></main><div id="buttonContainer"class="d-flex justify-content-center"><button type="submit"class="btn btn-primary">Submit</button> <button type="reset"class="btn btn-danger">Reset</button></div></form><template id="networkTableRow"><tr><td><svg class="wifi-svg"viewBox="0 0 100 100"stroke="#000"stroke-width="7"xmlns="http://www.w3.org/2000/svg"><circle class="wifi-0"cx="50"cy="80"r="5"stroke="none"/><path class="wifi-1"d="M35 70q15-10 30 0"fill="none"/><path class="wifi-2"d="M25 60q25-20 50 0"fill="none"/><path class="wifi-3"d="M15 50q35-30 70 0"fill="none"/><path class="wifi-4"d="M5 40q45-40 90 0"fill="none"/><path class="wifi-none"d="M5 40q45-40 90 0L50 82.5Z"fill="none"stroke="none"/></svg> <span class="networkName"></span> <button type="button"class="btn btn-warning networkSelect">Select</button></template><template id="loadingNetwork"><tr class="h-100"><td class="justify-content-center flex-column h-100"><svg style="scale: 3; margin-bottom: 1rem"width="80"height="66px"viewBox="0 0 24 24"><path class="spinner_Uvk8"d="M12,1A11,11,0,1,0,23,12,11,11,0,0,0,12,1Zm0,20a9,9,0,1,1,9-9A9,9,0,0,1,12,21Z"transform="matrix(0 0 0 0 12 12)"/><path class="spinner_Uvk8 spinner_ypeD"d="M12,1A11,11,0,1,0,23,12,11,11,0,0,0,12,1Zm0,20a9,9,0,1,1,9-9A9,9,0,0,1,12,21Z"transform="matrix(0 0 0 0 12 12)"/><path class="spinner_Uvk8 spinner_y0Rj"d="M12,1A11,11,0,1,0,23,12,11,11,0,0,0,12,1Zm0,20a9,9,0,1,1,9-9A9,9,0,0,1,12,21Z"transform="matrix(0 0 0 0 12 12)"/></svg> Scanning for networks</template><script src="index.min.js"></script>
)###";

const char *css = R"###(
@keyframes spinner_otJF{0%{transform:translate(12px,12px) scale(0);opacity:1}75%,to{transform:translate(0,0) scale(1);opacity:0}}:root{--primary:#0d6efd;--primary-rgb:13, 110, 253;--card-border-radius:12px}*{box-sizing:border-box}html{font-size:16px;height:100%}body{display:flex;flex-direction:column;min-height:100%}main{padding:1.4rem 2rem}body,h1,h2,h3,h4,h5,h6{margin:0}h1{font-size:calc(2rem + 1vw)}h2{font-size:calc(1.5rem + 1vw)}button,input,label,li,p,span,td,th{font-size:calc(.5rem + 1vw)}label{font-weight:600}input{width:100%;padding:5px}#ledList span,input,label{margin-block:.5rem}table{width:100%;border-collapse:collapse;border:2px solid #8c8c8c;font-family:sans-serif;font-size:.8rem;letter-spacing:1px}tfoot,thead{background-color:#e4f0f5}td,th{border:1px solid #a0a0a0;padding:8px 10px}td:last-of-type{text-align:center}tbody>tr:nth-of-type(even){background-color:#edeef2}.h-100{height:100%}.flex-column{flex-direction:column}.d-flex{display:flex}.align-items-center{align-items:center}.justify-content-center{justify-content:center}.visibility-hidden{visibility:hidden}.opacity-30{opacity:30%}.card{border-radius:10px;overflow:visible}.card .card-header{background-color:var(--primary);color:#fff;cursor:pointer;padding:1rem 2rem;border-radius:var(--card-border-radius) var(--card-border-radius)0 0}.card .card-header:hover{background-color:rgba(var(--primary-rgb),.7)}.card .card-body{padding:1.5rem 2rem;border:#000 1px solid;border-radius:0 0 var(--card-border-radius) var(--card-border-radius);overflow:visible}.collapse-content{overflow:hidden;padding:0;max-height:0;transition:max-height .2s ease-out}.collapsible .collapse-icon{font-size:1.25em}.btn{display:inline-block;font-weight:400;color:#212529;text-align:center;vertical-align:middle;user-select:none;background-color:transparent;border:1px solid transparent;padding:.375rem .75rem;font-size:1rem;line-height:1.5;border-radius:.25rem;transition:color .15s ease-in-out,background-color .15s ease-in-out,border-color .15s ease-in-out,box-shadow .15s ease-in-out}.btn:hover{color:#212529;text-decoration:none}.btn.focus,.btn:focus{outline:0;box-shadow:0 0 0 .2rem rgba(0,123,255,.25)}.btn.disabled,.btn:disabled{opacity:.65}.btn-primary{color:#fff;background-color:#007bff;border-color:#007bff}.btn-primary:hover{color:#fff;background-color:#0056b3;border-color:#0056b3}.btn-primary.focus,.btn-primary:focus{box-shadow:0 0 0 .2rem rgba(38,143,255,.5)}.btn-primary.disabled,.btn-primary:disabled{background-color:#007bff;border-color:#007bff}.btn-danger{color:#fff;background-color:#dc3545;border-color:#dc3545}.btn-danger:hover{color:#fff;background-color:#c82333;border-color:#bd2130}.btn-danger.focus,.btn-danger:focus{box-shadow:0 0 0 .2rem rgba(225,83,97,.5)}.btn-danger.disabled,.btn-danger:disabled{background-color:#dc3545;border-color:#dc3545}.btn-warning{color:#212529;background-color:#ffc107;border-color:#ffc107}.btn-warning:hover{color:#212529;background-color:#e0a800;border-color:#d39e00}.btn-warning.focus,.btn-warning:focus{box-shadow:0 0 0 .2rem rgba(255,193,7,.5)}.btn-warning.disabled,.btn-warning:disabled{background-color:#ffc107;border-color:#ffc107}#page-header{background-color:var(--primary);margin-bottom:1rem;color:#fff;padding:2rem}#ledList{display:grid;grid-template-columns:1fr;gap:10px}#ledList :first-child{grid-column:span 1}#ledList>*{box-shadow:0 .125rem .25rem rgba(0,0,0,.15);flex-direction:column;width:100%;padding:1rem 1.5rem}#ledList .slideContainer{width:90%}#ledList .slider{-webkit-appearance:none;width:80%;height:.25rem;border-radius:5px;background:#d3d3d3;outline:0;opacity:.7;-webkit-transition:opacity .15s ease-in-out;transition:opacity .15s ease-in-out}#ledList input[type=number]{width:5ch;border-radius:10px;margin:0}#ledList .slider:hover{opacity:1}#ledList .slider::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:1.25rem;height:1.25rem;border-radius:100%;background:var(--primary);cursor:pointer}#ledList .brightness-box{padding:.5rem 1rem;border-radius:10px}#ledList .red{background-color:red;color:#fff}#ledList .yellow{background-color:#ff0}#ledList .green{background-color:green;color:#fff}#buttonContainer{margin-top:auto;background-color:#fff;padding-block:1rem;position:sticky;bottom:0;gap:10px;box-shadow:0 0 0 .15rem rgba(0,0,0,.15)}#networkTable{margin-bottom:1rem}#networkTable svg{width:1.5rem}#networkTable td{display:flex;align-items:center;gap:5px;box-sizing:content-box}#networkTable tbody,#networkTable th,#networkTable thead,#networkTable tr{display:block;width:100%}#networkTable tbody{height:200px;overflow-y:auto;overflow-x:hidden}#refreshNetworks{border-bottom-left-radius:0;border-bottom-right-radius:0;float:right}.networkSelect{margin-left:auto;font-weight:700}.spinner_Uvk8{animation:spinner_otJF 1.6s cubic-bezier(.52,.6,.25,.99) infinite}.spinner_ypeD{animation-delay:.2s}.spinner_y0Rj{animation-delay:.4s}@media only screen and (min-width:767px){#ledList{grid-template-columns:repeat(3,1fr)}#ledList :first-child{grid-column:span 3}}
)###";

const char *js = R"###(
document.addEventListener("DOMContentLoaded",function(event){let websocket;const networkTable=document.querySelector("#networkTable");const tableBody=networkTable.querySelector("tbody");const tableCollapse=document.querySelector("#credentialForm");const refreshButton=document.querySelector("#refreshNetworks");const template=document.querySelector("#networkTableRow");let closing=false;function initWebSocket(restart){websocket=new WebSocket("ws://"+window.location.hostname+"/ws");websocket.onopen=function(){console.log("WebSocket connection opened");websocket.send("scan")};websocket.onclose=function(){console.log("WebSocket connection closed");if(!restart&&!closing){alert("Web socket disconnected. Please click refresh to restart auto network scanning.")}refreshButton.disabled=false};const ssidInput=document.querySelector("#ssid");websocket.onmessage=function(event){console.log("Received data: "+event.data);let networks=JSON.parse(event.data);tableBody.replaceChildren();for(let key in networks){const strength=networks[key];let level=0;switch(true){case strength>-30:level=4;break;case strength>-67:level=3;break;case strength>-70:level=2;break;case strength>-80:level=1;break}const elm=template.content.cloneNode(true).firstElementChild;for(let i=4-level;i>0;i--){elm.querySelector(".wifi-"+(4-i+1)).classList.add("opacity-30")}elm.querySelector(".networkName").textContent=key;tableBody.appendChild(elm)}document.querySelectorAll(".networkSelect").forEach(elm=>{elm.addEventListener("click",evt=>{ssidInput.value=elm.previousElementSibling.textContent})});tableCollapse.style.maxHeight=tableCollapse.scrollHeight+"px";refreshButton.disabled=false;return false}}const loadingNetwork=document.querySelector("#loadingNetwork");refreshButton.addEventListener("click",evt=>{initWebSocket(false);evt.target.disabled=true;tableBody.replaceChildren();tableBody.appendChild(loadingNetwork.content.cloneNode(true).firstElementChild)});refreshButton.dispatchEvent(new Event("click"));let coll=document.querySelectorAll(".collapse-toggle");coll.forEach(elm=>{elm.addEventListener("click",toggleCollapse);setTimeout(()=>{elm.dispatchEvent(new Event("click"))},250)});function toggleCollapse(){this.classList.toggle("active");let target=document.querySelector(this.dataset.toggle);if(target.style.maxHeight){target.style.overflow="hidden";target.style.maxHeight=null}else{target.style.overflow="visible";target.style.maxHeight=target.scrollHeight+"px"}let icon=this.querySelector(".collapse-icon");if(icon){icon.innerHTML=icon.innerHTML===String.fromCharCode(9650)?"&#x25BC;":"&#x25B2;"}}function updateBrightness(elm,percentage){elm.style.filter="brightness("+percentage+"%)"}document.querySelectorAll(".slideContainer").forEach(elm=>{elm.children[0].addEventListener("input",evt=>{evt.target.nextElementSibling.value=evt.target.value;if(evt.target.classList.contains("brightness-slider")){updateBrightness(elm.parentElement.querySelector(".brightness-box"),evt.target.value/255*100)}});elm.children[1].addEventListener("input",evt=>{evt.target.previousElementSibling.value=evt.target.value;if(evt.target.value.length>1){evt.target.value=Math.round(evt.target.value.replace(/\D/g,""));if(parseInt(evt.target.value)<parseInt(evt.target.min)){evt.target.value=evt.target.min}if(parseInt(evt.target.value)>parseInt(evt.target.max)){evt.target.value=evt.target.max}}if(evt.target.classList.contains("brightness-number")){updateBrightness(elm.parentElement.querySelector(".brightness-box"),evt.target.value/255*100)}})});function formInputReadonly(form,readonly){form.querySelectorAll("input").forEach(input=>{input.readOnly=readonly})}document.querySelector("#configForm").addEventListener("submit",async evt=>{evt.preventDefault();formInputReadonly(evt.target,true);let formData=new FormData(evt.target);let object={};formData.forEach((value,key)=>object[key]=value);let json=JSON.stringify(object);console.log(json);try{alert("Form submitted. Validation may take up to max 1 minute.");const response=await fetch("/submitForm",{method:"POST",mode:"cors",body:json,headers:{Accept:"application/json","Content-Type":"application/json"}});const data=await response.json();console.log(data);alert(data?"Configuration is saved and web server will be closed.":"Invalid configuration. Please check again.");if(data){closing=true;window.location.href="about:home"}}catch(e){alert("Error:"+e);console.error("Error:",e)}finally{formInputReadonly(evt.target,false)}})});
)###";

// Variables to store form post request and its data for long polling
AsyncWebServerRequest *formRequest;
JsonDocument formData;

// Processing form event flag
bool isProcessingForm = false;

// Setup for ultrasonic distance sensor (HC-SR04)
const uint8_t ECHO_PIN = 22;
const uint8_t TRIG_PIN = 23;
const unsigned int MAX_SENSING_RANGE_CM = 400;
const unsigned int MIN_SENSING_RANGE_CM = 2;
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_SENSING_RANGE_CM);

// Setup for buzzer
const uint8_t BUZZER_PIN = 4;
const uint BUZZER_FREQUENCY = 1000;
const uint BASED_PAUSE_DURATION = 100;
const uint PAUSE_DURATION_INCREMENT = 250;

// Setup for DHT11
const uint8_t DHT_PIN = 33;
const uint8_t DHT_TYPE = DHT11;
DHT dht(DHT_PIN, DHT_TYPE);

// LED GPIOs pins
const std::vector<String> LED_ORDER_VECTOR = {"RED", "YELLOW", "GREEN"};
const std::unordered_map<std::string, uint8_t> LED = {
        {std::string("RED"),    25},
        {std::string("YELLOW"), 26},
        {std::string("GREEN"),  27},
};

// Setup for LEDs' brightness control
const unsigned int LED_FREQUENCY = 5000;
const int RESOLUTION = 8;
const ulong MAX_BRIGHTNESS = pow(2, RESOLUTION) - 1;
const ulong DEFAULT_BRIGHTNESS = 200;
const std::unordered_map<std::string, uint> DEFAULT_LED_TRIGGER_RANGE = {
        {std::string("RED"),    50},
        {std::string("YELLOW"), 100},
        {std::string("GREEN"),  400},
};

// Setup for touch sensor
const uint8_t TOUCH_PIN = 34;
const ulong LONG_PRESS_DURATION = 5 * 1000;
ulong pressedTime;
enum TOUCH_STATE {
    RELEASED,
    PRESSING,
    SHORT_PRESSED,
    LONG_PRESSED
} touchState;

const ulong WAKE_UP_TIMER_INTERVAL = 5 * 60 * 1000000;

// IO event flag
bool isIOStarted = false;

const size_t EEPROM_SIZE = 512;

// Variable for EEPROM stored data
JsonDocument savedData;

// IO config valid frag
bool isIOConfigValid;


void initEEPROM() {
    Serial.println("\nInitialize EEPROM emulation");
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("EEPROM initialization failed. Restarting ESP...");
        ESP.restart();
    }
    Serial.println("EEPROM initialization successfully\n");
}

bool connectWiFi(const String &ssid, const String &password, uint attempts) {
    Serial.print("\nAttempt to connect SSID: " + ssid + " | password: " + password);
    unsigned char status;
    for (uint i = attempts; i > 0 && touchState < PRESSING; --i) {
        Serial.print(".");
        WiFi.begin(ssid, password);
        status = WiFi.waitForConnectResult(CONNECTION_TIMEOUT_MS);
        if (status == WL_CONNECTED) {
            break;
        }
        WiFi.disconnect(true, true);
    }
    Serial.println();
    return status == WL_CONNECTED;
}

bool validateIOConfig(JsonDocument doc) {
    // Any value that must be smaller first LED (RED) trigger range
    double previousLEDRange = MIN_SENSING_RANGE_CM - 1;
    for (int i = 0; i < LED.size(); ++i) {
        ulong brightness = doc[String("ledBrightness") + i].as<ulong>();
        double triggerRange = doc[String("ledTriggerRange") + i].as<double>();

        if (brightness > MAX_BRIGHTNESS || triggerRange < MIN_SENSING_RANGE_CM ||
            triggerRange > MAX_SENSING_RANGE_CM || triggerRange <= previousLEDRange) {
            // Last condition ensures trigger ranges follows GREEN > YELLOW > RED as order of iteration is RED -> YELLOW -> GREEN
            return false;
        }
        previousLEDRange = triggerRange;
    }
    return true;
}

String processor(const String &var) {
    if (var == "MAXBRIGHTNESS") {
        return String(MAX_BRIGHTNESS);
    } else if (var == "MINRANGE") {
        return String(MIN_SENSING_RANGE_CM);
    } else if (var == "MAXRANGE") {
        return String(MAX_SENSING_RANGE_CM);
    } else {
        return String(LED.at(var.c_str()), 0);
    }
    return {};
}

void handleSubmit(AsyncWebServerRequest *request, JsonVariant &json) {
    // Timeout need to be adjusted for long polling to avoid stored request being discarded internally
    request->client()->setRxTimeout(60000);

    Serial.println("\nReceive request of form data");
    Serial.println("Processing submitted form...");

    if (json.is<JsonArray>()) {
        formData = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        formData = json.as<JsonObject>();
    }

    // Store request for long polling (response only after form processing finish)
    formRequest = request;

    String jsonData;
    serializeJson(formData, jsonData);
    Serial.println("Receive data:\n" + jsonData);
    Serial.println("\nValidating configuration in background...");

    // Trigger processing form event in loop
    isProcessingForm = true;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    auto *info = (AwsFrameInfo *) arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char *) data;
        if (!isScanning && message == "scan") {
            Serial.println("\nStarting WiFi scan...\n");
            // Trigger constantly network scanning in loop
            WiFi.scanNetworks(true, false, true, 75U);
            isScanning = true;
        }
    }
}

void
onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.println("\nWebSocket client connected");
            break;
        case WS_EVT_DISCONNECT:
            Serial.println("\nWebSocket client disconnected");
            // Stop constantly network scanning after no connected client
            isScanning = false;
            WiFi.scanDelete();
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initAPWebServer() {
    Serial.println("\nSetting up access point...");
    WiFi.softAP(SSID, PASSWORD, 1, 0, 1);
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("IP Address for AP: %s\n", ip.toString().c_str());
    // Serve html on client connected
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Web Server Client has connected");
        request->send_P(200, "text/html", html, processor);
    });
    // Serve stylesheet for web page
    webServer.on("/style.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/css", css);
    });
    // Serve javascript for web page
    webServer.on("/index.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/javascript", js);
    });
    // Handle form as json
    auto *handler = new AsyncCallbackJsonWebHandler("/submitForm", handleSubmit);
    webServer.addHandler(handler);
    ws.onEvent(onEvent);
    // Handle websocket events triggered by web server
    webServer.addHandler(&ws);
    webServer.begin();
    Serial.println("\nWaiting for client connection...");
}

void onTouch() {
    const int sensorReading = digitalRead(TOUCH_PIN);
    if (touchState != PRESSING && sensorReading == HIGH) {
        pressedTime = millis();
        touchState = PRESSING;
    }
    if (touchState == PRESSING && sensorReading == LOW) {
        touchState = (millis() - pressedTime >= LONG_PRESS_DURATION) ? LONG_PRESSED : SHORT_PRESSED;
    }
}

void setup() {


    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n\nSerial monitor is ready");
    initEEPROM();
    Serial.println("Attempt to read saved configurations from EEPROM...");
    WiFi.setAutoReconnect(false);
    // Data is assumed to be a map json initially
    String jsonConfig = EEPROM.readString(0);
    DeserializationError error = deserializeJson(savedData, jsonConfig);
    WiFiClass::mode(WIFI_AP_STA);

    attachInterrupt(TOUCH_PIN, onTouch, CHANGE);
    esp_sleep_enable_ext0_wakeup((gpio_num_t) TOUCH_PIN, HIGH);
    esp_sleep_enable_timer_wakeup(WAKE_UP_TIMER_INTERVAL);

    // Check json validity
    if (!error) {
        Serial.println("Configurations have been read successfully.");
        Serial.println("Read data: \n" + jsonConfig);

        // 3 attempts = max waiting time of 6 * CONNECTION_TIMEOUT_MS seconds
        if (connectWiFi(savedData["ssid"], savedData["password"], 6) && touchState == RELEASED) {
            Serial.println("Network is successfully connected");
            // Setup all devices (DHT, ultrasonic, buzzer, LEDs, touch sensor)
            dht.begin();
            // INPUT_PULLDOWN to avoid electrical noise
            pinMode(TOUCH_PIN, INPUT_PULLDOWN);
            EasyBuzzer.setPin(BUZZER_PIN);
            EasyBuzzer.beep(BUZZER_FREQUENCY, 1, 0, 10, BASED_PAUSE_DURATION, 0, NULL);

            // Use PMW channels of 1 to 3 because 0 is used by EasyBuzzer by default
            for (int i = 1; i <= LED_ORDER_VECTOR.size(); ++i) {
                const String color = LED_ORDER_VECTOR[i - 1];
                Serial.println("Setup LED " + color + " (GPIO" + LED.at(color.c_str()) + ") at channel " + i);
                ledcSetup(i, LED_FREQUENCY, RESOLUTION);
                ledcAttachPin(LED.at(color.c_str()), i);
            }

            isIOConfigValid = validateIOConfig(savedData);
            Serial.print("\nConfiguration validity: ");
            Serial.print(isIOConfigValid ? "true, using read data" : "false, using default values");

            // Trigger IO events in loop
            isIOStarted = true;
        } else {
            Serial.println(
                    "Connection timeout. Starting a web webServer for uploading new configuration (or restart the device to trigger reconnect)...");
            initAPWebServer();
        }

    } else {
        WiFi.disconnect(true, true);
        Serial.println("No valid configuration available. Starting a web server for uploading new configuration...");
        initAPWebServer();
    }
}

double getDistance() {
    double pingTime = sonar.ping();
    const float temperature = dht.readTemperature();
    const float humidity = dht.readHumidity();
    if (isnormal(temperature) && isnormal(humidity)) {
        // Compensate speed of sounds with relative humidity and temperature
        const double soundSpeed = 331.4 + (temperature * 0.6 + 0.0124 * humidity);
        const double distance = (pingTime * (soundSpeed / 2)) / 10000;
        return distance;
    }
    return -1;
}

void loop() {

    if (touchState == LONG_PRESSED) {
        EasyBuzzer.stopBeep();
        EEPROM.put(0, 0);
        EEPROM.commit();
        Serial.println("\nErase saved configuration and restarting ESP...");
        esp_deep_sleep(1);
    } else if (touchState == SHORT_PRESSED) {
        Serial.println("\nEntering sleep mode (wake up on touch)...");
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        EasyBuzzer.stopBeep();
        delay(1000);
        esp_deep_sleep_start();
    }

    if (isIOStarted) {
        unsigned long currentMillis = millis();
        if (WiFiClass::status() == WL_CONNECTED) {
            EasyBuzzer.update();
            const double distance = getDistance();
            int pauseDuration = BASED_PAUSE_DURATION;
            bool isTrigger = false;
            // Trigger only one LED depending on range
            for (int i = 0; i < LED_ORDER_VECTOR.size(); - ++i) {
                const double triggerRange = isIOConfigValid ?
                                            savedData[String("ledTriggerRange") + i] :
                                            DEFAULT_LED_TRIGGER_RANGE.at(LED_ORDER_VECTOR.at(i).c_str());
                if (!isTrigger) {
                    // If previous LED triggered, set self to 0 brightness
                    if (distance <= triggerRange) {
                        const ulong brightness = isIOConfigValid ?
                                                 savedData[String("ledBrightness") + i] :
                                                 DEFAULT_BRIGHTNESS;
                        ledcWrite(i + 1, brightness);
                        EasyBuzzer.setPauseDuration(pauseDuration);
                        isTrigger = true;
                    } else {
                        ledcWrite(i + 1, 0);
                    }
                    // Closer range level, lower frequency of buzzer
                    pauseDuration += PAUSE_DURATION_INCREMENT;
                } else {
                    ledcWrite(i + 1, 0);
                }
            }
            previousMillis = currentMillis;
            delay(60);
        } else {
            Serial.println("\n\nNetwork disconnected during runtime");
            for (int i = 0; i < LED_ORDER_VECTOR.size(); - ++i) {
                ledcWrite(i + 1, DEFAULT_BRIGHTNESS);
            }
            EasyBuzzer.stopBeep();
            // Recoonection network for 10 attempts, if failed goes into light sleep mode
            if (connectWiFi(savedData["ssid"], savedData["password"], 10)) {
                Serial.println("\nReestablish connect successfully.");
                EasyBuzzer.beep(BUZZER_FREQUENCY, 1, 0, 10, BASED_PAUSE_DURATION, 0, NULL);
            } else {
                Serial.println("\nConnection timeout. Entering light sleep mode...");
                esp_sleep_enable_timer_wakeup(WAKE_UP_TIMER_INTERVAL);
                delay(1000);
                esp_light_sleep_start();
                touchState = RELEASED;
            }
        }
    }

    // Stop iterate network scanning on processing form
    if (isProcessingForm) {
        isProcessingForm = false;
        const bool isConnected = connectWiFi(formData["ssid"], formData["password"], 6);
        WiFi.disconnect(false, true); // Erase ssid and password in case AP ip change due to connected station
        if (isConnected && validateIOConfig(formData)) {
            Serial.println("\nValidation completed. Configuration is valid");
            formRequest->send(200, "application/json", "true");
            delay(10000);
            String data;
            serializeJson(formData, data);
            EEPROM.writeString(0, data);
            if (EEPROM.commit()) {
                Serial.println("\nConfiguration is successfully saved. Restarting ESP");
                esp_deep_sleep(1);
            }
        } else {
            Serial.println("\nValidation completed. Configuration is not valid");
            formRequest->send(200, "application/json", "false");
        }
        formData.clear();
        formRequest = nullptr;
    } else {
        if (isScanning && WiFi.scanComplete() >= 0) {
            JsonDocument doc;
            for (int i = 0; i < WiFi.scanComplete(); i++) {
                doc[WiFi.SSID(i)] = WiFi.RSSI(i);
            }

            String json;
            serializeJson(doc, json);
            WiFi.scanDelete();
            ws.textAll(json);
            WiFi.scanNetworks(true, false, true, 75U);
            delay(150);
        }
    }
}