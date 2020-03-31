#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "painlessMesh.h"
/* Put your SSID & Password */
const char* ssid = "NodeMCU";  // Enter SSID here
const char* password = "12345678";  //Enter Password here


/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80);


#define PIN        4 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 7 // Popular NeoPixel ring size
#define   MESH_PREFIX     "NodeMCU"
#define   MESH_PASSWORD   "12345678"
#define   MESH_PORT       5555

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Scheduler     userScheduler; // to control your personal task

painlessMesh  mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;
String msg;    //to broadcast message
String ptr ;  // to store html page
// Decode HTTP GET value
String redString = "0";
String greenString = "0";
String blueString = "0"; 
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;

// Prototype
void receivedCallback( uint32_t from, String &msg );
void sendMessage();
void reStartServer(void);
void handle_onchange_color(void);

// Send my ID every 10 seconds to inform others
Task taskSendMessage( TASK_SECOND*10, TASK_FOREVER, &sendMessage ); // start with a one second interval

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  server.on("/", handleRoot); 
  server.begin();
  Serial.println("HTTP server started");
  delay(100);
  mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | DEBUG ); // all types on
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA,6 );
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  
}


void reStartServer(void)
{
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  server.on("/", handleRoot);  
  server.begin();
  Serial.println("HTTP server started");  

}

void loop() {
  mesh.update();
  server.handleClient();
  for(int i=0; i<NUMPIXELS; i++)  // For each pixel...
    {
      pixels.clear(); // Set all pixel colors to 'off
      pixels.setPixelColor(i, pixels.Color(redString.toInt(), greenString.toInt(), blueString.toInt()));
      // pixels.setPixelColor(i, pixels.Color(0,255,255));
      pixels.show();   // Send the updated pixel colors to the hardware.
    }
}




void handleRoot() {
  Serial.println("HandleRoot*****************************");
  server.send(200, "text/html", SendHTML());
  String http = server.uri();
//  http += (server.method() == HTTP_GET)?"GET":"POST";
//  http += server.args();
  for (uint8_t i=0; i<server.args(); i++){
    http += server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println(http);
             pos1 = http.indexOf('r');
             pos2 = http.indexOf('g');
             pos3 = http.indexOf('b');
             pos4 = http.indexOf('&');
             redString = http.substring(pos1+1, pos2);
             greenString = http.substring(pos2+1, pos3);
             blueString = http.substring(pos3+1, pos4);
//             Serial.println(redString.toInt());
//             Serial.println("redString");
//             Serial.println(greenString.toInt());
//             Serial.println(blueString.toInt());
             http = ""; 
             
      
}



String SendHTML(){
  ptr = "<!DOCTYPE html><html>";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  ptr +="<link rel=\"icon\" href=\"data:,\">";
  ptr +="<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">";
  ptr +="<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>";
  ptr +="</head><body><div class=\"container\"><div class=\"row\"><h1>ESP Color Picker</h1></div>";
  ptr +="<a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a> ";
  ptr +="<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\"></div>";
  ptr +="<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);";
  ptr +="document.getElementById(\"change_color\").href=\"?r\" + Math.round(picker.rgb[0]) + \"g\" +  Math.round(picker.rgb[1]) + \"b\" + Math.round(picker.rgb[2]) + \"&\";}</script></body></html>";
  return ptr;
}

void sendMessage() {
  String msg;
  msg +=" "+redString;
  msg +=" "+greenString;
  msg +=" "+blueString;
 
  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  mesh.sendBroadcast(msg);
  
  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("Sending message: %s\n", msg.c_str());
  taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
  reStartServer();
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
