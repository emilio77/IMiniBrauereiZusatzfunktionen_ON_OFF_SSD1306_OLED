// iMiniBrauereiZusatzfunktionen by emilio
// D1,D2: Optional WEMOS OLED

// Durch Setzen der Variable "ExterneSteuerung" kann die Brauerei auf dem Rechner gestartet, gestoppt oder pausiert werden
// t= keine Funktion
// s= Start
// p= Pause
// e= Stop

#include <ESP8266WiFi.h>                             //https://github.com/esp8266/Arduino
#include <EEPROM.h>                                  
#include <WiFiManager.h>                             //https://github.com/kentaylor/WiFiManager
#include <ESP8266WebServer.h>                        //http://www.wemos.cc/tutorial/get_started_in_arduino.html
#include <DoubleResetDetector.h>                     //https://github.com/datacute/DoubleResetDetector
#include "SSD1306Wire.h"                             //https://github.com/squix78/esp8266-oled-ssd1306 
// #include "SH1106Wire.h"                             //https://github.com/squix78/esp8266-oled-ssd1306 

#define Version "1.0.2_ESP+OLED"

#define deltaMeldungMillis 1000                      // Sendeintervall an die Brauerei in Millisekunden
#define DRD_TIMEOUT 10                               // Number of seconds after reset during which a subseqent reset will be considered a double reset.
#define DRD_ADDRESS 0                                // RTC Memory Address for the DoubleResetDetector to use
#define RESOLUTION 12                                // OneWire - 12bit resolution == 750ms update rate
#define OWinterval (800 / (1 << (12 - RESOLUTION))) 
#define OLED_RESET 0

SSD1306Wire  display(0x3c, D1, D2);
// SH1106Wire  display(0x3c, D1, D2);

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

IPAddress UDPip(192,168,178,255);                     // IP-Adresse an welche UDP-Nachrichten geschickt werden xx.xx.xx.255 = Alle Netzwerkteilnehmer die am Port horchen.
unsigned int answerPort = 5003;                       // Port auf den Temperaturen geschickt werden
unsigned int localPort = 5010;                        // Port auf dem gelesen wird
ESP8266WebServer server(80);                          // Webserver initialisieren auf Port 80
WiFiUDP Udp;

const int Malz = D8;
const int Muehle = D7;
const int Hopf_1 = D4;
const int Hopf_2 = D5;
const int Hopf_3 = D6;

String FunktionText1="",FunktionText2="",FunktionText3=""; 
int EinbringzeitMalz =0;
char ExterneSteuerung = 't';
char state[3] = "";
String str = "";

void Deklaration()        // Setup für Zusatzfunktionen
{
  pinMode(Malz, OUTPUT);
  pinMode(Muehle, OUTPUT); 
  pinMode(Hopf_1, OUTPUT); 
  pinMode(Hopf_2, OUTPUT); 
  pinMode(Hopf_3, OUTPUT); 
}
    
void Funktion1ON()           // Individuelle Funktion ON
{
  if (FunktionText1="Muehle AUS")
  { 
    digitalWrite(Muehle,HIGH);
    FunktionText1="Muehle EIN";
  }
}

void Funktion2ON()           // Individuelle Funktion ON
{
  if(EinbringzeitMalz !=25)      // Einstellung Malzeinbringzeit
  {
    EinbringzeitMalz++;
    digitalWrite(Malz,HIGH);
    FunktionText2="Malz ";
    FunktionText2+=EinbringzeitMalz;
  }
  else 
  {
    digitalWrite(Malz,LOW);
    FunktionText2="";
  } 
}

void Funktion3ON()           // Individuelle Funktion ON
{
  digitalWrite(Hopf_1,HIGH); 
  FunktionText3="Hopf_1 EIN";   
}

void Funktion4ON()           // Individuelle Funktion ON
{
  digitalWrite(Hopf_2,HIGH); 
  FunktionText3="Hopf_2 EIN";   
}

void Funktion5ON()           // Individuelle Funktion ON
{
  digitalWrite(Hopf_3,HIGH); 
  FunktionText3="Hopf_3 EIN";   
}

void Funktion6ON()           // Individuelle Funktion ON
{

}

void Funktion7ON()           // Individuelle Funktion ON
{

}

void Funktion8ON()           // Individuelle Funktion ON
{

}

void Funktion9ON()           // Individuelle Funktion ON
{

}

void Funktion10ON()           // Individuelle Funktion ON
{
  
}

void Funktion1OFF()           // Individuelle Funktion OFF
{ 
  if (FunktionText1="Muehle EIN")
  { 
    digitalWrite(Muehle,LOW);
    FunktionText1="Muehle AUS";
  } 
}

void Funktion2OFF()           // Individuelle Funktion OFF
{
  if(EinbringzeitMalz !=0)      // Einstellung Malzeinbringzeit
  {
    digitalWrite(Malz,LOW);
    FunktionText2="";
    EinbringzeitMalz=0;
  }  
}

void Funktion3OFF()           // Individuelle Funktion OFF
{
  digitalWrite(Hopf_1,LOW); 
  FunktionText3="";   
}

void Funktion4OFF()           // Individuelle Funktion OFF
{
  digitalWrite(Hopf_2,LOW); 
  FunktionText3="";  
}

void Funktion5OFF()           // Individuelle Funktion OFF
{
  digitalWrite(Hopf_3,LOW); 
  FunktionText3="";  
}

void Funktion6OFF()           // Individuelle Funktion OFF
{
  
}

void Funktion7OFF()           // Individuelle Funktion OFF
{
  
}

void Funktion8OFF()           // Individuelle Funktion OFF
{
  
}

void Funktion9OFF()           // Individuelle Funktion OFF
{
  
}

void Funktion10OFF()           // Individuelle Funktion OFF
{
  
}

void noFunktion()
{
  digitalWrite(Malz,LOW);
  digitalWrite(Muehle,LOW);
  if (state[1]!='o') {EinbringzeitMalz = 0;}
}

const int PIN_LED = D4;                                // Controls the onboard LED.

char charVal[8];
char packetBuffer[24];                                 // buffer to hold incoming packet,
char temprec[24] = "";
char relais[5] = "";

boolean Funktionslog[10];
bool initialConfig = false;                             // Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
unsigned long jetztMillis = 0, letzteInMillis = 0, letzteOfflineMillis = 0, displayMillis=0, letzteSendMillis = 0;
 
const unsigned char officon[] =
{
  0b01100000, 0b00000000, //          ##     
  0b11100000, 0b00000000, //         ###     
  0b11000000, 0b00000001, //        ###      
  0b11000000, 0b00000001, //        ###      
  0b11100110, 0b00000001, //        ####  ## 
  0b11111110, 0b00000011, //       ######### 
  0b11111100, 0b00000111, //      #########  
  0b11111000, 0b00001111, //     #########   
  0b11000000, 0b00011111, //    #######      
  0b10000000, 0b00111111, //   #######       
  0b00000000, 0b01111111, //  #######        
  0b00000000, 0b01111110, //  ######         
  0b00000000, 0b01111100, //  #####          
  0b00000000, 0b00111000, //   ###           
  0b00000000, 0b00000000, //    
  0b00000000, 0b00000000, // 
};

const unsigned char playicon[] =
{
  0b00000000, 0b00000000, //   
  0b00000000, 0b00000000, //     
  0b01000000, 0b00000000, //      # 
  0b11000000, 0b00000001, //      ###  
  0b11000000, 0b00000111, //      #####   
  0b11000000, 0b00011111, //      #######    
  0b11000000, 0b00111111, //      ########     
  0b11000000, 0b00011111, //      ####### 
  0b11000000, 0b00000111, //      #####   
  0b11000000, 0b00000001, //      ###  
  0b01000000, 0b00000000, //      # 
  0b00000000, 0b00000000, //        
  0b00000000, 0b00000000, //   
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //  
};

const unsigned char pauseicon[] =
{
  0b00000000, 0b00000000, // 
  0b00000000, 0b00000000, //   
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b10011100, 0b00000011, //       ###  ### 
  0b00000000, 0b00000000, //
  0b00000000, 0b00000000, //   
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //                 
};

const unsigned char stopicon[] =
{
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, // 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b11000000, 0b00111111, //       ######## 
  0b00000000, 0b00000000, // 
  0b00000000, 0b00000000, //   
  0b00000000, 0b00000000, //   
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //                 
};

const unsigned char kreis_voll[] =
{
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, // 
  0b00000000, 0b00000000, //       
  0b00000000, 0b00000000, //
  0b11111110, 0b00000001, //      ######## 
  0b11111111, 0b00000011, //     ##########
  0b11111111, 0b00000011, //     ########## 
  0b11111111, 0b00000011, //     ########## 
  0b11111111, 0b00000011, //     ########## 
  0b11111111, 0b00000011, //     ########## 
  0b11111111, 0b00000011, //     ##########
  0b11111110, 0b00000001, //      ########
  0b00000000, 0b00000000, //      
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //   
};

const unsigned char kreis_leer[] =
{
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, // 
  0b00000000, 0b00000000, //       
  0b00000000, 0b00000000, //
  0b11111110, 0b00000001, //      ######## 
  0b11111111, 0b00000011, //     ##########
  0b00000011, 0b00000011, //     ##      ## 
  0b00000011, 0b00000011, //     ##      ## 
  0b00000011, 0b00000011, //     ##      ## 
  0b00000011, 0b00000011, //     ##      ## 
  0b11111111, 0b00000011, //     ##########
  0b11111110, 0b00000001, //      ########
  0b00000000, 0b00000000, //      
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //         
  0b00000000, 0b00000000, //   
}; 

void DisplayOut() {

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setColor(WHITE);

  if (state[1]=='o') { display.drawXbm(111, 0, 16, 16, officon);}        
  else if (state[1]=='x') { display.drawXbm(111, 0, 16, 16, stopicon);}
  else if (state[1]=='y') { display.drawXbm(111, 0, 16, 16, playicon);}
  else if (state[1]=='z') { display.drawXbm(111, 0, 16, 16, pauseicon);}

  if (jetztMillis<displayMillis+5000)
  {
    if (Funktionslog[1]==true) { display.drawXbm(0, 40, 16, 16, kreis_voll); } else { display.drawXbm(0, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[2]==true) { display.drawXbm(12, 40, 16, 16, kreis_voll); } else { display.drawXbm(12, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[3]==true) { display.drawXbm(25, 40, 16, 16, kreis_voll); } else { display.drawXbm(25, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[4]==true) { display.drawXbm(36, 40, 16, 16, kreis_voll); } else { display.drawXbm(38, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[5]==true) { display.drawXbm(51, 40, 16, 16, kreis_voll); } else { display.drawXbm(51, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[6]==true) { display.drawXbm(64, 40, 16, 16, kreis_voll); } else { display.drawXbm(64, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[7]==true) { display.drawXbm(77, 40, 16, 16, kreis_voll); } else { display.drawXbm(77, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[8]==true) { display.drawXbm(90, 40, 16, 16, kreis_voll); } else { display.drawXbm(90, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[9]==true) { display.drawXbm(103, 40, 16, 16, kreis_voll); } else { display.drawXbm(103, 40, 16, 16, kreis_leer); }  
    if (Funktionslog[10]==true) { display.drawXbm(117, 40, 16, 16, kreis_voll); } else { display.drawXbm(117, 40, 16, 16, kreis_leer); }  

    display.drawString(0, 0, "Funktion");
    display.drawString(0, 20, "1 2 3 4 5 6 7 8 9 0");
    display.display();
  }
  else if (state[1]=='o') { displayMillis = jetztMillis; }  
  else if (jetztMillis<displayMillis+9000 & ( FunktionText1!="" || FunktionText2!="" || FunktionText3!="" ) )
  {
    display.clear();
    display.drawString(0, 0, FunktionText1);
    display.drawString(0, 22, FunktionText2);
    display.drawString(0, 44, FunktionText3);
    display.display();
  }
  else { displayMillis = jetztMillis; } 
}

void UDPOut() {
  Udp.beginPacket(UDPip, answerPort);
  Udp.print("T9999");
  Udp.print(ExterneSteuerung);
  Udp.println();
  Udp.endPacket();
}

void UDPRead()
{
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    for (int schleife = 0; schleife < 23; schleife++) { temprec[schleife] = ' '; }
    // read the packet into packetBufffer
    Udp.read(packetBuffer, packetSize);
    for (int schleife = 0; schleife < 23; schleife++) { temprec[schleife] = packetBuffer[schleife]; }
    letzteInMillis = millis();
    Serial.print("Udp Packet received"); 
    packetAuswertung();
  }
}    

void OfflineCheck()
{
  if (jetztMillis > letzteInMillis+10000) 
  {
    if (jetztMillis > letzteOfflineMillis+1000) {Serial.print("offline"); letzteOfflineMillis=jetztMillis; } 
    state[1]='o';
    for (int i=1; i<=10; i++) {Funktionslog[i]=false;}
  }
}

void packetAuswertung()
{
  int temp = 0;
  int temp2 = 0;
  if ((temprec[0]=='C') && (temprec[18]=='c'))             // Begin der Decodierung des seriellen Strings  
  { 
    temp=(int)temprec[1];
    if ( temp < 0 ) { temp = 256 + temp; }
    if ( temp > 7) {relais[4]='A';temp=temp-8;} else {relais[4]='a';} 
    if ( temp > 3) {relais[3]='P';temp=temp-4;} else {relais[3]='p';} 
    if ( temp > 1) {relais[2]='R';temp=temp-2;} else {relais[2]='r';}
    if ( temp > 0) {relais[1]='H';temp=temp-1;} else {relais[1]='h';}   

    temp=(int)temprec[2];
    if ( temp < 0 ) { temp = 256 + temp; }
    if ( temp > 127) {temp=temp-128;}  
    if ( temp > 63) {temp=temp-64;}
    if ( temp > 31) {temp=temp-32;}    
    if ( temp > 15) {temp=temp-16;}  
    if ( temp > 7) {temp=temp-8;}  
    if ( temp > 3) {state[1]='x';temp=temp-4;} 
    else if ( temp > 1) {state[1]='z';temp=temp-2;}  
    else if ( temp > 0) {state[1]='y';temp=temp-1;}    
    
    temp=(int)temprec[6];
    if ( temp < 0 ) { temp = 256 + temp; }  
    temp2=(int)temprec[7];
    if ( temp2 < 0 ) { temp2 = 256 + temp2; }
    for (int i=1; i<=10; i++) {Funktionslog[i]=false;}
    if ( temp > 127) {Funktionslog[1]=true;temp=temp-128;} 
    if ( temp > 63) {Funktionslog[2]=true;temp=temp-64;} 
    if ( temp > 31) {Funktionslog[3]=true;temp=temp-32;} 
    if ( temp > 15) {Funktionslog[4]=true;temp=temp-16;}    
    if ( temp > 7) {Funktionslog[5]=true;temp=temp-8;}  
    if ( temp > 3) {Funktionslog[6]=true;temp=temp-4;} 
    if ( temp > 1) {Funktionslog[7]=true;temp=temp-2;}  
    if ( temp > 0) {Funktionslog[8]=true;temp=temp-1;}  
    if ( temp2 > 1) {Funktionslog[9]=true;temp2=temp2-2;}    
    if ( temp2 > 0) {Funktionslog[10]=true;temp2=temp2-1;}  
  }
}

void Funktionsstart()
{
  bool NoFunktion = false;
  if ( Funktionslog[1] == true ) { Funktion1ON(); NoFunktion = true; } else { Funktion1OFF();}
  if ( Funktionslog[2] == true ) { Funktion2ON(); NoFunktion = true; } else { Funktion2OFF();}
  if ( Funktionslog[3] == true ) { Funktion3ON(); NoFunktion = true; } else { Funktion3OFF();}
  if ( Funktionslog[4] == true ) { Funktion4ON(); NoFunktion = true; } else { Funktion4OFF();}
  if ( Funktionslog[5] == true ) { Funktion5ON(); NoFunktion = true; } else { Funktion5OFF();}
  if ( Funktionslog[6] == true ) { Funktion6ON(); NoFunktion = true; } else { Funktion6OFF();}
  if ( Funktionslog[7] == true ) { Funktion7ON(); NoFunktion = true; } else { Funktion7OFF();}
  if ( Funktionslog[8] == true ) { Funktion8ON(); NoFunktion = true; } else { Funktion8OFF();}
  if ( Funktionslog[9] == true ) { Funktion9ON(); NoFunktion = true; } else { Funktion9OFF();}
  if ( Funktionslog[10] == true ) { Funktion10ON(); NoFunktion = true; } else { Funktion10OFF();}
  if ( NoFunktion == false ) { noFunktion(); FunktionText1 = ""; FunktionText2 = ""; FunktionText3 = ""; }
}

void ReadSettings() {
  EEPROM.begin(512);
  EEPROM.get(0, localPort);
  EEPROM.get(20, answerPort);
  EEPROM.commit();
  EEPROM.end();
}  

void WriteSettings() {
  EEPROM.begin(512);
  EEPROM.put(0, localPort);
  EEPROM.put(20, answerPort);
  EEPROM.end();    
}

void setup() {
  pinMode(PIN_LED, OUTPUT);       // Im folgenden werden die Pins als I/O definiert
  Deklaration();
  
  Serial.begin(19200);
  Serial.println("\n");
  Serial.println("Starting");
  Serial.print("iMiniBrauereiZusatzfunktionen V");  
  Serial.println(Version);

  // Initialising the UI will init the display too.
  display.init();
  // display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  
  WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
  WiFi.setOutputPower(20.5);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  ReadSettings();

  WiFi.printDiag(Serial);            //Remove this line if you do not want to see WiFi password printed

  if (WiFi.SSID()==""){
    Serial.println("We haven't got any access point credentials, so get them now");   
    initialConfig = true;
  }
 
  if (drd.detectDoubleReset()) {
    Serial.println("Double Reset Detected");
    initialConfig = true;
  }
  
  if (initialConfig) {
    Serial.println("Starting configuration portal.");
    digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
    char convertedValue[5];

    sprintf(convertedValue, "%d", answerPort);
    WiFiManagerParameter p_answerPort("answerPort", "send temperature on UDP Port", convertedValue, 5);
    
    sprintf(convertedValue, "%d", localPort);
    WiFiManagerParameter p_localPort("localPort", "receive relais state on UDP Port", convertedValue, 5);

    WiFiManager wifiManager;
    wifiManager.setBreakAfterConfig(true);
    wifiManager.addParameter(&p_answerPort);
    wifiManager.addParameter(&p_localPort);
    wifiManager.setConfigPortalTimeout(300);

    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
    
    answerPort = atoi(p_answerPort.getValue());
    localPort = atoi(p_localPort.getValue());

    WriteSettings();

    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

  }
   
  digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
  
  unsigned long startedAt = millis();
  Serial.print("After waiting ");
  int connRes = WiFi.waitForConnectResult();
  float waited = (millis()- startedAt);
  Serial.print(waited/1000);
  Serial.print(" secs in setup() connection result is ");
  Serial.println(connRes);
  if (WiFi.status()!=WL_CONNECTED){
    Serial.println("failed to connect, finishing setup anyway");
  } else{
    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());
    Udp.begin(localPort);
    Serial.print("UDP-IN port: ");
    Serial.println(localPort);
    UDPip=WiFi.localIP();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setColor(WHITE);
    display.drawString(0, 0, "IP-Adresse:");
    display.drawHorizontalLine(0, 28, 128);
    str=String(UDPip[0])+"."+String(UDPip[1])+"."+String(UDPip[2])+"."+String(UDPip[3]);
    display.drawString(0, 40, str); 
    display.display();
    UDPip[3]=255;
    Serial.print("UDP-OUT port: ");
    Serial.println(answerPort); 
    delay(8000);  
    server.on("/", Hauptseite);
    server.begin();                          // HTTP-Server starten
  }
}

void loop() {

  drd.loop();
    
  jetztMillis = millis();

  server.handleClient(); // auf HTTP-Anfragen warten

  if (WiFi.status()!=WL_CONNECTED){
    WiFi.reconnect();
    Serial.println("lost connection");
    delay(5000);
    Udp.begin(localPort);
    server.on("/", Hauptseite);
    server.begin();            // HTTP-Server starten
  } else{
    if(jetztMillis - letzteSendMillis > deltaMeldungMillis)
    {
      Funktionsstart();
      if ( ExterneSteuerung=='s' || ExterneSteuerung=='p' ||ExterneSteuerung=='e' ) {UDPOut();}
      letzteSendMillis = jetztMillis;
    }
    UDPRead();
    OfflineCheck();
    DisplayOut();
    Hauptseite();
    if (jetztMillis < 100000000) {wdt_reset();}             // WatchDog Reset  
  }  
}

void Hauptseite()
{
  char dummy[8];
  String Antwort = "";
  Antwort += "<meta http-equiv='refresh' content='5'/>";
  Antwort += "<font face=";
  Antwort += char(34);
  Antwort += "Courier New";
  Antwort += char(34);
  Antwort += ">";
   
  Antwort += "<b>Zusatzfunktionsstatus: </b>\n</br>Funktion 1:&nbsp;&nbsp;";
  if (Funktionslog[1] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 2:&nbsp;&nbsp;";
  if (Funktionslog[2] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 3:&nbsp;&nbsp;";
  if (Funktionslog[3] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 4:&nbsp;&nbsp;";
  if (Funktionslog[4] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 5:&nbsp;&nbsp;";
  if (Funktionslog[5] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 6:&nbsp;&nbsp;";
  if (Funktionslog[6] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 7:&nbsp;&nbsp;";
  if (Funktionslog[7] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 8:&nbsp;&nbsp;";
  if (Funktionslog[8] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 9:&nbsp;&nbsp;";
  if (Funktionslog[9] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="Funktion 10:&nbsp;";
  if (Funktionslog[10] == true) { Antwort +="Ein\n</br>"; } else { Antwort +="Aus\n</br>"; }
  Antwort +="\n</br><b>Brauereistatus: </b>\n</br>";
  if (state[1]=='o') { Antwort +="OFFLINE "; }        
  else if (state[1]=='x') { Antwort +="INAKTIV"; }
  else if (state[1]=='y') { Antwort +="AKTIV"; }
  else if (state[1]=='z') { Antwort +="PAUSIERT"; }
  Antwort +="\n</br>";      
  Antwort +="</br>Verbunden mit: ";
  Antwort +=WiFi.SSID(); 
  Antwort +="</br>Signalstaerke: ";
  Antwort +=WiFi.RSSI(); 
  Antwort +="dBm  </br>";
  Antwort +="</br>IP-Adresse: ";
  IPAddress IP = WiFi.localIP();
  Antwort += IP[0];
  Antwort += ".";
  Antwort += IP[1];
  Antwort += ".";
  Antwort += IP[2];
  Antwort += ".";
  Antwort += IP[3];
  Antwort +="</br>";
  Antwort +="</br>UDP-IN port: ";
  Antwort +=localPort; 
  Antwort +="</br>UDP-OUT port: ";
  Antwort +=answerPort; 
  Antwort +="</br></br>";
  Antwort += "</font>";
  server.send ( 300, "text/html", Antwort );
}

   
 
 
