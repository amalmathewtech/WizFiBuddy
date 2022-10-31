/*
 Web Server Controlled Semaphore Device
 Green LED is indicates you are available and Red LED indicates you are busy.

 Libraries Required : WizFi360_arduino_library(https://github.com/Wiznet/WizFi360_arduino_library)



*/

#include "WizFi360.h"


#define AVAILABLE 1
#define BUSY 0

#define GREEN_LED 13
#define RED_LED 12

/* Baudrate */
#define SERIAL_BAUDRATE   115200
#define SERIAL1_BAUDRATE  115200

/* Wi-Fi info */
char ssid[] = "AM";       // your network SSID (name)
char pass[] = "12345678";   // your network password

int status = WL_IDLE_STATUS;  // the Wifi radio's status

int ledStatus = AVAILABLE;

WiFiServer server(80);

// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  // initialize serial for debugging
  Serial.begin(SERIAL_BAUDRATE);
  // initialize serial for WizFi360 module
  Serial1.begin(SERIAL1_BAUDRATE);
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("You're connected to the network");
  printWifiStatus();
  digitalWrite(GREEN_LED, HIGH);  // Turn on GREEN LED  when Powered On  
  // start the web server on port 80
  server.begin();
}

void loop() {
  WiFiClient client = server.available();  // listen for incoming clients
  if (client) {                               // if you get a client,
    Serial.println("New client");             // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    while (client.connected()) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,
        char c = client.read();               // read a byte, then
        buf.push(c);                          // push it to the ring buffer
        // printing the stream to the serial monitor will slow down
        // the receiving of data from the WizFi360 filling the serial buffer
        //Serial.write(c);
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        if (buf.endsWith("\r\n\r\n")) {
          sendHttpResponse(client);
          break;
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (buf.endsWith("GET /H")) {
          Serial.println("Turn led ON");
          ledStatus = AVAILABLE;
          digitalWrite(GREEN_LED, HIGH);   
          digitalWrite(RED_LED, LOW);
        }
        else if (buf.endsWith("GET /L")) {
          Serial.println("Turn led OFF");
          ledStatus = BUSY;
          digitalWrite(GREEN_LED, LOW);    // turn the LED off by making the voltage LOW
          digitalWrite(RED_LED, HIGH);
        }
      }
    }
    // close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}

void sendHttpResponse(WiFiClient client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html>\r\n");
          client.print("<h1>WizFiBuddy Semaphore Device</h1>\r\n");
          client.println("<style type=\"text/css\">body {font-size:1.7rem; font-family: Georgia; text-align:center; color: #333; background-color: #cdcdcd;} div{width:75%; background-color:#fff; padding:15px; text-align:left; border-top:5px solid #bb0000; margin:25px auto;}</style>");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  // the content of the HTTP response follows the header:
  client.print("Status : I am  ");
  if(ledStatus == AVAILABLE){
     client.print("Available");
  }
  else{
    client.print("Busy");
  }
  
  client.println("<br>");
  client.println("<br>");
  //adds styles
  client.println("<style type=\"text/css\">body {font-size:1.7rem; font-family: Georgia; text-align:center; color: #333; background-color: #cdcdcd;} div{width:75%; background-color:#fff; padding:15px; text-align:left; border-top:5px solid #bb0000; margin:25px auto;}</style>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("</head>");
  client.println("<body>");
  client.println("<div>");
  client.println("* <a href=\"/H\">Available</a> <br>");
  client.println("* <a href=\"/L\">Busy</a> <br>");                 
  client.println("</form>");
  client.println("</form>");
  client.println("</div>");
  client.println("</body>");
  // The HTTP response ends with another blank line:
  client.println();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}
