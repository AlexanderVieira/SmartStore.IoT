#include <AsyncTCP.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
//#include <Arduino_JSON.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Domain Name with full URL Path for HTTP POST Request
const char* urlAPI = "http://6ed707e74609.ngrok.io/api/notificacao/enviar";

// Replace with your network credentials (STATION)
const char* ssidStation = "Alexander";
const char* passwordStation = "@lerCor#";

// ACCESS POINT credentials
const char* ssidAP = "ESP32-AP";
const char* passwordAP = "12345678";

// Wi-Fi channel for the access point (must match the sender channel)
#define CHAN_AP 2

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {  
  String carrinhoId;  
  String tagClienteId;
  String arrayTagProdutoId[10];
  
} struct_message;

// Create a struct_message called myData
struct_message myData;

AsyncWebServer server(80);
AsyncEventSource events("/events");

void postNotification() {    
  
  Serial.println("Posting JSON data to server...");
  delay(1000);
  // Block until we are able to connect to the WiFi access point
  if (WiFi.status() == WL_CONNECTED) {
     
    HTTPClient http;
    http.begin(urlAPI);    
    http.addHeader("Content-Type", "application/json");
        
    StaticJsonDocument<300> root;   
    //Add an objectJson     
    root["carrinhoId"] = myData.carrinhoId;
    root["tagClienteId"] = myData.tagClienteId;   
    JsonArray jsonArrayTagProduto = root.createNestedArray("arrayTagProdutoId");    
    
    //String tagsProduto;
    for(int i = 0; i < (sizeof(myData.arrayTagProdutoId)/sizeof(myData.arrayTagProdutoId[0])); i++){
      if(myData.arrayTagProdutoId[i] != ""){
        jsonArrayTagProduto.add(myData.arrayTagProdutoId[i]);
        myData.arrayTagProdutoId[i] = "";
        Serial.println(myData.arrayTagProdutoId[i]); 
      }         
    }    
    
    String requestBody;
    serializeJson(root,requestBody);
    Serial.println(requestBody);  

    http.addHeader("Content-Length", (String)requestBody.length());

    //int httpResponseCode;
    if(myData.arrayTagProdutoId[0] == ""){
      int httpResponseCode = http.POST(requestBody);
      Serial.println(httpResponseCode); 
      if(httpResponseCode > 0){
       
      String response = http.getString();
      //Serial.println(httpResponseCode);   
      Serial.println(response);
      // Free resources
      http.end();
     
    }
    else {
     
      Serial.printf("Error occurred while sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
       
    }
    } 
  }
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
 // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  
  memcpy(&myData, incomingData, sizeof(myData));

  //JSONVar board;
  StaticJsonDocument<300> board;
  board["carrinhoId"] = myData.carrinhoId;
  board["tagClienteId"] = myData.tagClienteId;
  JsonArray jsonArrayTag = board.createNestedArray("arrayTag");

   for(int i = 0; i < (sizeof(myData.arrayTagProdutoId)/sizeof(myData.arrayTagProdutoId[0])); i++){
      if(myData.arrayTagProdutoId[i] != ""){
        jsonArrayTag.add(myData.arrayTagProdutoId[i]);
        //myData.arrayTagProdutoId[i] = "";
        Serial.println(myData.arrayTagProdutoId[i]); 
      }         
   }    
  
  String jsonString;
  serializeJson(board,jsonString);
  
  //String jsonString = JSON.stringify(board);
  Serial.println(jsonString);
  events.send(jsonString.c_str(), "new_readings", millis());
  
  Serial.print("Packages received: "); 
  Serial.println(len); 
  Serial.print("Board Id received: ");  
  Serial.println(myData.carrinhoId);
  Serial.print("Tag Client received: ");
  Serial.println(myData.tagClienteId);
  Serial.println("Tags Products received: ");

  byte tamArray = (sizeof(myData.arrayTagProdutoId)/sizeof(myData.arrayTagProdutoId[0]));  
  
  for(byte i = 0; i < tamArray; i++){    
    Serial.println(myData.arrayTagProdutoId[i]);
  }    
  Serial.println();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW SMART STORE</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" 
        integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards 
    { 
      max-width: 700px; 
      margin: 0 auto; 
      display: grid; 
      grid-gap: 2rem; 
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); 
    }
    .reading { font-size: 2rem; }
    .packet { color: #bebebe; }
    .card.rc522 { color: #fd7e14; }
    #tabela { margin-block-end: 0.5em; }  
    td { text-align: center; }    
  </style>
</head>
<body>
  <div class="topnav">
    <h4>SMART STORE</h4>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card rc522">
        <h2><i class="fas fa-cart-arrow-down" style="font-size:36px"></i> CARRINHO ID: <span id="car001"></span></h2>
        <p><span class="reading">TAG Cliente: <span id="c001"></span></span></p>
        <!--<p><span class="reading">TAG Produto: <span id="p001"></span></span></p>-->
        <!--<p class="packet">CARRINHO ID: <span id="rt1"></span></p>-->
        <div id="tabela" class="reading" align="center"></div>
      </div>           
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  var arrayTag = obj["arrayTag"];
  //var count = arrayTag.length;

  //Create a HTML Table element.
        var table = document.createElement("TABLE");
        table.border = "0";
 
        //Get the count of columns.
        var columnCount = 1;
 
        //Add the header row.
        var row = table.insertRow(-1);
        for (var i = 0; i < columnCount; i++) {
            var headerCell = document.createElement("TH");
            headerCell.innerHTML = "TAG PRODUTOS";
            row.appendChild(headerCell);
        }
 
        //Add the data rows.
        for (var i = 0; i < arrayTag.length; i++) {
            row = table.insertRow(-1);
            for (var j = 0; j < columnCount; j++) {
                var cell = row.insertCell(-1);
                cell.innerHTML = arrayTag[i];
            }
        }
 
        var dvTable = document.getElementById("tabela");
        dvTable.innerHTML = "";
        dvTable.appendChild(table);
  
  document.getElementById("car"+obj.carrinhoId).innerHTML = obj.carrinhoId;
  document.getElementById("c"+obj.carrinhoId).innerHTML = obj.tagClienteId;  
  document.getElementById("p"+obj.carrinhoId).innerHTML = arrayTag[0];  
 }, false);
}
</script>
</body>
</html>)rawliteral";
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssidStation, passwordStation);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Set device as an access point
  WiFi.softAP(ssidAP, passwordAP, CHAN_AP, true);  

  Serial.print("Mac Address in Station: "); 
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to  
  esp_now_register_recv_cb(OnDataRecv);  

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
   
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }    
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}
 
void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  }
  if(myData.arrayTagProdutoId[0] != ""){
    postNotification();
  } 
}
