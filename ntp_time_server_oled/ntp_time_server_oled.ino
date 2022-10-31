/*
  Get the time from a Network Time Protocol (NTP) time server and dispaly on OLED Screen.

 Required Libraries : 
 1.Adafruit SSD1306 by Adafruit
 2.WizFi360_arduino_library  - https://github.com/Wiznet/WizFi360_arduino_library

*/

#include "WizFi360.h"
#include "WizFi360Udp.h"


#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Change this offsetime based your time zone
#define OFFSET_TIME_ZONE 19800   // UTC+5:30 = 5.5*60*60 = 19800

#define SCREEN_WIDTH 128 // OLED display width,  in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// declare an SSD1306 display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* Baudrate */
#define SERIAL_BAUDRATE   115200
#define SERIAL1_BAUDRATE  115200


/* Wi-Fi info */
char ssid[] = "AM";       // your network SSID (name)
char pass[] = "12345678";   // your network password

int status = WL_IDLE_STATUS;  // the Wifi radio's status

char timeServer[] = "time.nist.gov";  // NTP server
unsigned int localPort = 2390;        // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48;  // NTP timestamp is in the first 48 bytes of the message
const int UDP_TIMEOUT = 1000;    // timeout in milliseconds to wait for an UDP packet to arrive

byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

void setup() {
  // initialize serial for debugging
  Serial.begin(SERIAL_BAUDRATE);
  // initialize serial for WizFi360 module
  Serial1.begin(SERIAL1_BAUDRATE);

  // initialize WizFi360 module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // initialize OLED display with address 0x3C for 128x64
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  delay(2000);         // wait for initializing
  oled.clearDisplay(); // clear display
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  Udp.begin(localPort);
}

void loop() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  // wait for a reply for UDP_TIMEOUT milliseconds
  unsigned long startMs = millis();
  while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT) {}
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it into the buffer
    Udp.read(packetBuffer, NTP_PACKET_SIZE);

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = (secsSince1900 - seventyYears)+OFFSET_TIME_ZONE;
    oled.clearDisplay(); // clear display
    oled.setTextSize(3);          // text size
    oled.setTextColor(WHITE);     // text color
    oled.setCursor(0, 28);        // position to display
    // print the hour, minute and second:
    // UTC is the time at Greenwich Meridian (GMT)
    oled.print((epoch  % 86400L) / 3600);// print the hour (86400 equals secs per day)
    oled.print(':');

    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      oled.print('0');
    } 
    oled.print((epoch  % 3600) / 60);// print the minute (3600 equals secs per minute)
    oled.print(':');
    if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      oled.print('0');
    }
    oled.print(epoch % 60);// print the second
  }
  oled.display();               // show on OLED
  // wait few millisecond before asking for the time again
  delay(800);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char *ntpSrv) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123

  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  Udp.endPacket();
}
