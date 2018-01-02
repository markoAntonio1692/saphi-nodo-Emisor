///////////////////////////////////////////////////////////////////////
//SAPHI NODO EMISOR V 0.2.1:                                          //
//                                                                    //
//-MODO AP                                                            //
//-WEBSERVER(config ssid y passwd para conexion wifi)                 //
//-MQTT(envio y recepcion a broker local y nube)                      //
//-JSON                                                               //
//-PANTALLA OLED                                                      //
//-MULTIPLEXOR ANALOGICO(16 entrasas para sensores analogicos 0-3.3v) //
//-MODO BAJO CONSUMO CONTROLADO                                       //
//-sensor dht22
////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <ArduinoJson.h>
#include "SSD1306.h"
#include "MUX74HC4067.h"
SSD1306  display(0x3C, 4, 5);
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>

//SALIDAS
#define led2 10
#define led  D4
#define scl  D1
#define sda  D2
#define scl  D1
#define S0   D3
#define S1   D5
#define S2   D6
#define S3   D8
#define WAKE-UP   D0
//ENTRADAS
#define B1   D7



/////////////PARAMETROS DE CONCFIGURACION DE LA RED WIFI
//const char* ssid = "HUAWEI-1605"; 
//const char* password = "75985170";
const char* mqtt_server = "broker.shiftr.io";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////AUTENTICACION BROKER MQTT
const char clientID[]="ESP8266-saphi-004";  //identificador unico de para cada dispositivo iot
const char username[]="saphi_server";       //usuario configurado en broker
const char passwords[]="saphi_server";      //contraseña usuario broker
const char willTopic[]="status";
int willQoS=1 ;                             //0-1-2
int  willRetain=0 ;                         //0-1  //si se activa o no la retencion de data
const char willMessage[]="desconectado";    //mensaje cuando device este desconectado de broker
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////OTRAS VARIABLES
int adc=0;
int humedad=0;
float voltaje=0;
int conec_err_mqtt=0;
int conec_err_wifi=0;
int aux_ret_desde_web=0;
int a_sleep;  //SEGUNDOS EN MODO ESLEEP    
int t_sleep_s; //ACTIVAR O DESACTIVAR MODO BAJO CONSUMO
int ener_reg;

/////////////////OPCIONAL, CONFIGURACION IP STATICA
//IPAddress staticIP316_10(192, 168, 8, 200);
//IPAddress gateway316_10(192, 168, 8, 1);
//IPAddress subnet316_10(255, 255, 255, 0);

const char* ssid = "test";
const char* passphrase = "test";
String st;
String content;
int statusCode;

//dht sensor
int pinDHT22 = 2;
SimpleDHT22 dht22;

ESP8266WebServer server(80);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastMsg2 = 0;
char msg[50];
int value = 0;
char data[256];
int sensores[18];
//pines multiplexor
//              EN  S0  S1  S2  S3
MUX74HC4067 mux(50, D3, D5, D6, D8);



void setup() {
  pinMode(led2,OUTPUT);
  digitalWrite(led2,HIGH);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); //desconecta funcion wifi
    mux.signalPin(A0, INPUT, ANALOG);
  Serial.begin(115200);
  pinMode(led, OUTPUT);   
  pinMode(S0, OUTPUT);  
  pinMode(S1, OUTPUT);   
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(B1,INPUT_PULLUP);  
  //for(int i=0; i<=10; i++){ digitalWrite(led,!digitalRead(led)); delay(50);}
  digitalWrite(led,HIGH);
  
  Serial.println("");
  
  //ENTRAR A MODO AP o MODO NODO SAPHI
  if(digitalRead(D7)==LOW)
  {
      delay(500);
      //init_psk();
      display.init();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 0, "Modo AP...");
      display.display();
   
      Serial.println("Modo AP...");
   
      delay(500);
      ap_wifi();
    }else{
        display.init();
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, "Iniciando");
        display.display();
        delay(500);
      }
  /*********************************************************/
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

//CONECTANDOSE A WIFI
void setup_wifi(){
    EEPROM.begin(512);
    delay(10);
    Serial.println();
    Serial.println();
    Serial.println("Startup");
    // read eeprom for ssid and pass
    Serial.println("Reading EEPROM ssid");
    String esid;
    for (int i = 0; i < 32; ++i)
      {
        esid += char(EEPROM.read(i));
      }
    Serial.print("SSID: ");
    Serial.println(esid);
    Serial.println("Reading EEPROM pass");
    String epass = "";
    for (int i = 32; i < 96; ++i)
      {
        epass += char(EEPROM.read(i));
      }
    Serial.print("PASS: ");
    Serial.println(epass);  
    if ( esid.length() > 1 ) {
        Serial.println();
        Serial.print("Conectado a red wifi2:  ");
        Serial.println(esid.c_str());
        WiFi.begin(esid.c_str(), epass.c_str());
    }
    
  //delay(10);
        //display.init();
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_10);
        display.clear();
        display.drawStringMaxWidth(0, 0, 128,"Conectando a Red WiFi : " );
        display.drawStringMaxWidth(0, 10, 128,esid.c_str() );
        //display.clear();
      
  //WiFi.begin(ssid, password);
  int x=0;int y=15;
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
          x=x+2;
          display.drawStringMaxWidth(x, y, 128,". " );
          display.display();
          if(x==120){x=0; y=y+2;}
           
          conec_err_wifi++;
          //espera 180 segundo sino se conecta, se reinician
          if(conec_err_wifi>=180)
          {
            display.drawStringMaxWidth(0,48 , 128,"Reiniciando...");
            display.display();
            Serial.println("Reiniciando...");
            delay(1000);
            ESP.restart();
          }
  }
  
   //CONFIGURAR IP ESTATICA SI SE REQUIERE....
   // WiFi.config(staticIP316_10, gateway316_10, subnet316_10); //ip estatica, comentar si solo usamos dhcp
   
    Serial.println("");
    Serial.println("Conexion exitosa!");
    Serial.print("Direccion ip local asignada IP : ");
    String ip = WiFi.localIP().toString();
    Serial.println(ip);
    
          display.drawStringMaxWidth(0, 30, 128,"Conexion exitosa. " );
          display.drawStringMaxWidth(0, 39, 128,"IP local Asignada: " );
          display.drawStringMaxWidth(0, 49, 128, ip);
          display.display();
          delay(4000);
}     

//RECEPCION DE DATA 
void callback(char* topic, byte* payload, unsigned int length) {
  display.drawStringMaxWidth(0,38 , 128,"Recepcion datos: _ ");
  display.display();
  Serial.print("Message arrived [");
  Serial.print(topic);
  String string_topic;
  string_topic = String(topic);
  Serial.print("] ");

  String readStrings="";
  char c;
  char json_data_in[50];
  for (int i = 0; i < length; i++) {
    
    Serial.print((char)payload[i]);
    c=payload[i];
    readStrings +=c; //concatena datos que llegan
 
  }
  Serial.println();


  Serial.println();
 //Serial.println(readStrings);

  readStrings.toCharArray(json_data_in, (readStrings.length() + 1));
  //Serial.println(json_data_in[1]);
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json_data_in);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  
   a_sleep=root["sleep"];
   t_sleep_s=root["t_sleep_s"];
   ener_reg=root["ener_reg"];
 Serial.println(a_sleep);Serial.println(t_sleep_s);Serial.println(ener_reg);

  /*
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
*/

}

void reconnect() 
{
  while (!client.connected()) 
  {
    Serial.print("Reconectando a broker MQTT :  ");
          display.clear();
          display.drawStringMaxWidth(0,0 , 128,"Reconectando a broker MQTT: " );
          display.display();
    //if (client.connect("ESP8266Client")) {
    if(client.connect(clientID,username,passwords,willTopic,willQoS,willRetain,willMessage)){//if (client.connect("ESP-002")) 
      Serial.println("Conexion exitosa!");
           display.drawStringMaxWidth(0,22, 128,"Conexion exitosa. " );
           display.display();
      client.publish("status", clientID);
      client.subscribe("inTopic");
      delay(2000);
    } else {
      Serial.print("Fallo , rc=");
      Serial.print(client.state());
      String c =String(client.state());
      Serial.println(" volviendo a conectar en 3 segundos");

          display.clear();
          display.drawStringMaxWidth(0,0 , 128,"Reconectando a broker MQTT: " );
          display.drawStringMaxWidth(0,22 , 128,"Fallo conexion, rc = "+c );
          display.drawStringMaxWidth(0,38 , 128,"volviendo a conectar en 3 segundos");
          display.display();
      // Wait 5 seconds before retrying
      delay(3000);
      conec_err_mqtt++;
      if(conec_err_mqtt>=10) //si en un minuto no se conecta, se reinicia todo reinicia 
      {
          display.drawStringMaxWidth(0,48 , 128,"Reiniciando...");
          display.display();
          Serial.println("Reiniciando...");
          delay(1000);
          ESP.restart();
      }
  }
}
}

//FUNCION PARA CONVERTIR A FORMATO JSON
String json(int lect) { 
    float temperature = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);
  //  return;
  }
  
  Serial.print("Sample OK: ");
  Serial.print((float)temperature); Serial.print(" *C, ");
  Serial.print((float)humidity); Serial.println(" RH%");
  
  String value = "\"bat\":" + String(mux.read(0));
  String value2 = ",\"C1\":" + String(mux.read(1)) ;
  String value3 = ",\"C2\":" + String(mux.read(2)) ;
  String value4 = ",\"C3\":" + String(mux.read(3)) ;
  String value5 = ",\"C4\":" + String(mux.read(4)) ;
  String value6 = ",\"C5\":" + String(mux.read(5)) ;
  String value7 = ",\"C6\":" + String(mux.read(6)) ;
  String value8 = ",\"C7\":" + String(mux.read(7)) ;
  String value9 = ",\"C8\":" + String(mux.read(8)) ;
  String value10 = ",\"C9\":" + String(mux.read(9)) ;
  String value11 = ",\"C10\":" + String(mux.read(10)) ;
  String value12 = ",\"C11\":" + String(mux.read(11)) ;
  String value13 = ",\"C12\":" + String(mux.read(12)) ;
  String value14 = ",\"C13\":" + String(mux.read(13)) ;
  String value15 = ",\"C14\":" + String(mux.read(14)) ;
  String value16 = ",\"C15\":" + String(mux.read(15)) ;
  String value17 = ",\"temp\":" + String(temperature) ;
  String value18 = ",\"hum\":" + String(humidity) ;

   value = value + value2 + value3+value4+value5+value6+value7+ value8
   +value9+value10+value11+value12+value13+value14+value15+value16+value17+value18;
  
   String payload = "{" + value + "}";
  
  return payload;
}


///////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP("saphi","12345678");  //contraseña wifi de nodo
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype){
 
  if ( webtype == 1 ) {
    server.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html> SAPHI - NODO EMISOR    -  Red:    ";
        content += ipStr;
        content += "<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32> <p> <label>PASS: </label> <input name='pass' length=64></p> ";
        content += "<p>&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp<input type='submit'></p>  </form>";
        content += "</html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"Success\":\"Se guardo la informacion en la eeprom... Reiniciando... \"}";
          statusCode = 200;
          aux_ret_desde_web=1;
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
        if(aux_ret_desde_web==1){ESP.restart(); }  ///si se guarda la informacion se resetea automaticamente nodemcu
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
    });
  }
}


void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}

void ap_wifi(){
   
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  /*
      display.clear()
      display.drawStringMaxWidth(0,28 , 128,"MODE AP  ");
      display.display();
    */  
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      //if (testWifi()) {
      if(false){
        launchWeb(0);
        return;
      } 
  }
  setupAP();
    while(1){
        server.handleClient();
      }
  }





void loop() {  
  if (!client.connected()) {
    reconnect();
  }  
  client.loop();

  long now = millis();
  long now2 = millis();
  digitalWrite(led,HIGH);
  
  if (now2 - lastMsg2 > 100) {
         lastMsg2 = now2;
         display.clear();
         display.drawStringMaxWidth(0,28 , 128,"Enviando datos:  ");
         display.drawStringMaxWidth(0,38 , 128,"Recepcion datos:  ");
         display.display();
  }
  if (now - lastMsg > 5000) {
          
          display.drawStringMaxWidth(0,28 , 128,"Enviando datos: |_|");
          display.display();
    //digitalWrite(led,LOW);
    lastMsg = now;
    //    ++value;
    //el string lo convierte a array, al final siempre se envian char arrays 
    String my_payload = json(0);
    my_payload.toCharArray(data, (my_payload.length() + 1));

    //snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(data);
    client.publish("outTopic", data);
    Serial.print("Modo Deep Sleep, segundos: ");
    Serial.print(t_sleep_s);
    if(a_sleep==1)
    {
      ESP.deepSleep(t_sleep_s * 1000000);
    }
  }
}
