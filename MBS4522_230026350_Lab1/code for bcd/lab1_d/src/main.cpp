#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with network credentials
const char* ssid = "KOHA";
const char* password = "Jimmy1ac";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";
//set pin locatation
#define DHTPIN 12     // Digital pin connected to the DHT sensor
#define LDRPIN 36
#define LEDPIN 13

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT22     // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
int br = 0;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 3 seconds
const long interval = 3000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: right;
    }
    body  {
      background-image:url("https://images5.alphacoders.com/133/1332407.png");
      background-repeat:no-repeat;
      background-attachment:fixed;
      background-size:100%% 100%%;
    }
    TLE{ 
      font-size: 50px; 
      font-style: oblique;
      color: white;
    }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .TT {
    font-family: Georgia, serif;
    font-size: 25px;
    font-style: oblique, bold;
    }
    span{
      color : white;
    }
    sup{
      color : white;
    }
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #FF0000; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #0FFE00}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
  <title>ESP32 Server</title>
  <TLE>ESP_32 Server with DHT22, LDR & LED</TLE> 
  
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="TT" style="color:#EF39FE;">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>

  <p>
    <i class="fas fa-tint" style="color:#0049FF;"></i> 
    <span class="TT" style="color:#009EFF;">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%%</sup>
  </p>
  
  <p>
    <i class="far fa-sun" style="color:#FFF700;"></i>
    <span class="TT" style="color:#F7FF00;">BRIGHTNESS</span>
    <span id="brightness">%BRIGHTNESS%</span>
    <sup class="units">%%</sup>
  </p>
  %BUTTONPLACEHOLDER%
</body style="color:#3EC1FA;">

<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 3000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 3000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("brightness").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/brightness", true);
  xhttp.send();
}, 3000 ) ;

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}

</script>
</html>)rawliteral";

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}
// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  else if(var == "BRIGHTNESS"){
    return String(br);
  }

  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4 style = color:white;font-size:20px><i class=\"far fa-lightbulb\" style=\"color:#FFF700;font-size:40px;\"></i>LED_Output</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"13\" " + outputState(13) + "><span class=\"slider\"></span></label>";
    return buttons;
  }

  return String();
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    digitalWrite(13, inputMessage2.toInt());
    request->send(200, "text/plain", "OK");
  });
  
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print EPS32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(br).c_str());
  });

  // Start server
  server.begin();
}
 
void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.print("temperature: ");
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.print("Humi: ");
      Serial.println(h);
    }

    int raw= analogRead(LDRPIN);
    int newbr = analogRead(LDRPIN);
        
    br = map(newbr, 0, 4095, 0, 100);
    Serial.print(br);
    Serial.println("%");
    Serial.println(raw);
    
  } 
}