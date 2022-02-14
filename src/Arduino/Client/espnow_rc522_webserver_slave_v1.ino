#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
#define SS_1_PIN 05
#define SS_2_PIN 21
#define RST_PIN 22
#define BUZZ_PIN 25
#define NR_OF_READERS 2
#define I2C_SDA 33
#define I2C_SCL 32

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

// Create MFRC522 instance.
MFRC522 mfrc522[NR_OF_READERS]; 

//The gateway access point credentials
const char* ssid = "ESP32-AP";
const char* password = "12345678";

// Set your CARRINHO ID (ESP32 Slave #1 = CARRINHO_ID 001, ESP32 Slave #2 = CARRINHO_ID 002, etc)
const String carrinhoId = "001";
const String tagClienteId = "E12B6320";
byte contador = 0;
byte flagInicio = 0; 
byte countTag = 0; 

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//Wi-Fi channel (must match the gateway wi-fi channel as an access point)
#define CHAN_AP 2

// set the LCD number of columns and rows
uint8_t lcdColumns = 16;
uint8_t lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Structure example to send data
typedef struct struct_message {  
  String carrinhoId;     
  String tagClienteId;
  String arrayTagProdutoId[10];
  
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail"); 
  
  for(byte i = 0; i < (sizeof(myData.arrayTagProdutoId)/sizeof(myData.arrayTagProdutoId[0])); i++){    
    Serial.println(myData.arrayTagProdutoId[i]);
    if(myData.arrayTagProdutoId[i] != ""){
        myData.arrayTagProdutoId[i] = "";
    }
  } 
  contador = 0;  
}

void beep()
{
  digitalWrite(BUZZ_PIN, HIGH);
  delay(5000); 
  digitalWrite(BUZZ_PIN, LOW); 
}

void inicializarDisplay(){
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
}

void exibirMensagemDisplay(String msg1, String msg2){

  lcd.setBacklight(HIGH);
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print message
  lcd.print(msg1);  

  if(msg2 != ""){

    // set cursor to first column, second row
    lcd.setCursor(0,1);
    lcd.print(msg2);    

  } 
  delay(5000);
  lcd.clear();
  lcd.setBacklight(LOW); 
}

void inicializarRFID(){
    
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    delay(10);
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
  }
  String msg1 = "Aproxime o seu"; 
  String msg2 = "cartao cliente";
  exibirMensagemDisplay(msg1, msg2);  
}
 
void setup() {
  
  //pinMode(BUZZ_PIN, OUTPUT);
  //digitalWrite(BUZZ_PIN, LOW);

  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Init Serial Monitor
  Serial.begin(115200);

  SPI.begin(); // Initiate  SPI bus 

  inicializarDisplay(); 
 
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Access Point...");
  }

  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  inicializarRFID();
  Serial.println();

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = CHAN_AP;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }  
   
}
 
void loop() { 
   
   readerCards();
  
}

void esp_now_slave_send(struct_message myData){  
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(2000);

}

String getTag(byte *buffer, byte bufferSize, uint8_t myReader){
   
   String content = ""; 
   for (byte i = 0; i < bufferSize; i++) {
      
     Serial.print(buffer[i] < 0x10 ? " 0" : " ");
     Serial.print(buffer[i], HEX);
     content.concat(String(mfrc522[myReader].uid.uidByte[i], HEX));

  } 
  return content; 
}

void readerCards(){

  String productTag = "";
  String clientTag = "";  
  
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {

    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
    delay(3000);
    
    // Look for new cards    
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
        Serial.print(F("Reader "));
        Serial.print(reader);
        
        // Show some details of the PICC (that is: the tag/card)
        Serial.print(F(": Card UID:"));
        if(reader == 0){

          productTag = getTag(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size, reader);
          
        }else{

          clientTag = getTag(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size, reader);
          
        }
                
        if(clientTag != ""){
          countTag++;
          Serial.println(String(countTag));
          clientTag.toUpperCase();          
          Serial.println("TAG CLIENTE: " + clientTag); 
        }
        
        if(countTag == 0){
           Serial.println();
           Serial.println("Count#0: " + String(countTag));
           String msg1 = "Carrinho fechado";
           String msg2 = "indisponível.";
           exibirMensagemDisplay(msg1, msg2);           
           String msg3 = "Aproxime o seu"; 
           String msg4 = "cartao cliente";
           exibirMensagemDisplay(msg3, msg4);                  
        }
        
        if(countTag == 1){
          Serial.println();
          Serial.println("Count#1: " + String(countTag));
          if(productTag != ""){
            //beep();
            productTag.toUpperCase();
            myData.arrayTagProdutoId[contador++] = productTag;            
            String msg1 = "Produto";
            String msg2 = "adicionado...";
            exibirMensagemDisplay(msg1, msg2);                    
            Serial.println("TAG PRODUTO: " + productTag);
          } 
       }      
    }    
  }   
  
  if (clientTag == tagClienteId)
  {              
     if(flagInicio == 0){       
       
       //beep();
       flagInicio = 1;       
       String msg1 = "Bem-vindo!";
       String msg2 = "Boas compras!";
       exibirMensagemDisplay(msg1, msg2);            
      
     }else{
       
       //beep();       
       flagInicio = 0;
       countTag = 0;       
       String msg1 = "Enviando pedido";
       String msg2 = "para o caixa...";
       exibirMensagemDisplay(msg1, msg2);         
       myData.carrinhoId = carrinhoId;
       myData.tagClienteId = clientTag;   
       esp_now_slave_send(myData);  
      
     } 
  }
}
