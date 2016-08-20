/*
   note on i2c for display: address is 0x3c, SDA is gpio 4 (D2), SCL is gpio 5 (D1)

  For the 12 position switch:
  solder a 10k resistor between each leg, except between position 0 and 12. connect position 0 to GND and position 12 to 3V3 and the output (center) to analog input (A0) of the NodeMCU
  for input value stability, add a 1uF cap between A0 and GND

  The buttons act as pulldowns to GND. Set on of the connections of the buttons to GND, the other one to: green button: D6, red button D7

 * */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>  // Use WiFiClientSecure class to create TLS connection
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>  //linienabstand: 16
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
//#include <Fonts/FreeSans24pt7b.h>
#include "global.h"
#include "eeprom.h"
#include "webpage.h"
#include "timekeeping.h"
#include "base64.h"
#include "toggle_API.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);  // delete any old wifi configuration
  delay(200);
  switch_changed = false;

  WiFi.mode(WIFI_STA);

  pinMode(12, INPUT_PULLUP);  // start (NodeMCU D6)
  pinMode(13, INPUT_PULLUP);  // stop (NodeMCU D7)

  attachInterrupt(12, startButtonISR, FALLING);
  attachInterrupt(13, stopButtonISR, FALLING);
  startpressed = false;  // clear button states
  stoppressed = false;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Wire.setClock(400000);  // use fast speed
  display.clearDisplay();
  display.display();  //

  EEPROM.begin(128);
  if (!ReadConfig()) {
    writeDefaultConfig();
  }

  server.on("/", sendPage);
  server.onNotFound(handleNotFound);  // handle page not found (send 404 response)
  server.begin();                     // start webserver

  // display configuration
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  display.setCursor(0, 0);
  display.println(F("Connecting to network: "));
  display.println(config.ssid);
  display.display();

  // if string read from eeprom is used directly, it won't connect after AP mode. A bug?

  // WiFi.begin(config.ssid.c_str(), config.password.c_str());
  String ssidstr = config.ssid;
  String pwstr = config.password;
  WiFi.begin(ssidstr.c_str(), pwstr.c_str());

  Serial.print(F("connecting to "));
  Serial.println(ssidstr);

  uint8_t trialcounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    trialcounter++;

    if (trialcounter > 70 || (startpressed && stoppressed))  // timeout or both buttons pressed during connect
    {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP_togglr", "12345678");  // start accesspoint with config page
      display.println(F("Starting Accesspoint:"));
      display.println(F("SSID: ESP_togglr"));
      display.println(F("PW: 12345678"));
      display.display();
      while (true) {
        server.handleClient();
        yield();
      }
    }
  }
  Serial.println("");
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  display.println(F("WiFi connected"));
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.print(F("Toggl Server"));
  display.display();

  // create authentication string for toggl account from API key
  authentication = config.APIkey + ":api_token";
  size_t encodedlength;
  authentication = String(base64_encode((const unsigned char*)authentication.c_str(), authentication.length(), &encodedlength));
  authentication.remove(encodedlength);  // trim the returned array to actual length

  WiFiClientSecure client;
  if (toggl_connect(&client)) {
    Serial.println(F("Connected"));
    display.print(F("."));
    display.display();
  }

  if (readWorkspaces(&client)) {
    display.print(F("."));
    display.display();
    Serial.println(F("Server Response OK"));
  }

  if (readProjects(&client)) {
    display.print(F("."));
    display.display();
    Serial.println(F("Server Response OK"));
  }

  // check if we got at least one workspace and one project:
  if (projectID[0].length() > 1 && workspaceID[0].length() > 1) {
    display.println(F("OK"));
    display.display();
  } else {
    display.println(F(" FAIL!"));
    display.display();
    display.println(F("No Projects"));
    display.display();
    delay(5000);
    ESP.restart();  // reboot
  }

  Serial.println("\n\n");

  for (int i = 0; i < 12; i++) {
    Serial.print(i);
    Serial.print(":\t");
    Serial.print(projectID[i]);
    Serial.print("\t");
    Serial.println(projectName[i]);
  }
  if (readCurrentTimerID(&client))  // if timer is currently running
  {
    Serial.println(F("Timer is running!"));
    isrunning = true;
  }
}

// testproject: at index 7

void loop() {
  bool switchproject = false;  // switch project
  if (WiFi.status() == WL_CONNECTED) {

    if (projectID[switchposition] == "") //empty project position
    {
      startpressed = false; //ignore start, cannot start an empty project
    }

    if (startpressed) {
      if (!isrunning) {
        WiFiClientSecure client;
        // display.setFont(&FreeSans9pt7b);
        display.setFont(&FreeSans18pt7b);

        display.clearDisplay();
        display.setCursor(0, 45);
        display.println(F("START!"));
        display.display();
        toggl_connect(&client);

        if (client.connected())  // todo: need to make this more safe, maybe try to connect a few times... or record time locally and update remote later?
        {
          projectstarttime = millis(); //start the local timer
          // send the POST to start the time
          String url = "/api/v8/time_entries/start";
          String messagebody = "{\"time_entry\":{\"description\":\"ButtonBox OK\",\"pid\":" + projectID[switchposition] + ",\"created_with\": \"ButtonBox\"}}\r\n";

          String header = "POST " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Authorization: Basic " + authentication + "\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " +
                          String(messagebody.length()) + "\r\n\r\n";

          client.print(header);
          client.print(messagebody);

          // Serial.println(header);
          // Serial.println(messagebody);

          // print the response
          uint8_t timeout = 0;
          bool responseok = false;
          while (client.available() == 0) {
            delay(10);
            if (timeout++ > 200)
              break;  // wait 2s max
          }

          // Read all the lines of the reply and find the "id" entry
          while (client.available()) {
            String line = client.readStringUntil('\r');
            //  Serial.print(line);
            if (line.indexOf("200 OK", 0) != -1) {
              responseok = true;
            }

            int idx_start = line.indexOf("\"id\"") + 5;
            int idx_end = line.indexOf(",", idx_start);

            if (idx_start > 5 && idx_end > 5) {
              runningTimerID = line.substring(idx_start, idx_end);
              Serial.print(F("Timer ID = "));
              Serial.println(runningTimerID);
              startpressed = false;
              isrunning = true;
              
            }
          }

          if (responseok)
            Serial.println(F("Server Response OK"));
          // client.flush();
        }
        //  client.stop(); //disconnect
      }
      // if already running and switch changed before start button is pressed
      else if (switch_changed) {
        switchproject = true;
        stoppressed = true;  // stop current project timer before starting new timer in next loop
      }
      else
      {
        startpressed = false; //if start pressed, running and project did not change, discard button action
      }
    }

    if (stoppressed) {
      if (isrunning)
      {
        if (!switchproject) {    // not switching projects
          startpressed = false;  // clear any pending start button detections
        }
        WiFiClientSecure client;
        display.setFont(&FreeSans18pt7b);
        display.clearDisplay();
        display.setCursor(11, 45);
        display.println(F("STOP!"));
        display.display();

        toggl_connect(&client);

        if (client.connected())  // todo: need to make this more safe, maybe try to connect a few times... or record time locally and update remote later?
        {
          // send PUT to stop the time

          String url = "/api/v8/time_entries/" + runningTimerID + "/stop";

          String header = "PUT " + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Authorization: Basic " + authentication + "\r\n" + "Content-Type: application/json\r\n\r\n";

          client.print(header);
          // Serial.println(header);

          // print the response

          uint8_t timeout = 0;
          while (client.available() == 0) {
            delay(10);
            if (timeout++ > 200)
              break;  // wait 2s max
          }

          while (client.available()) {
            String line = client.readStringUntil('\r');
            //  Serial.print(line);
            if (line.indexOf("200 OK", 0) != -1) {
              isrunning = false;
              stoppressed = false;
              Serial.println(F("Timer Stopped"));
            }
          }
        }
        // client.stop(); //disconnect
      }
      else {
        stoppressed = false; //if not running, discard stop pressed
      }
    }
    updateState();
  } else {
    display.setFont(0);
    display.clearDisplay();
    display.setCursor(20, 45);
    display.println(F("WIFI DISCONNECTED"));
    display.display();
  }

  // todo: check like every 5 seconds if timer is running or if it was stopped through web interface

  delay(50);
}

/*

  288347 Christoph Romer's workspace
  1277454 mine
  1448829 testworkspace

  User ID = 2046360


  0:  13245672  0-Gemeinkosten
  1:  14947268  1250-AF-OIS
  2:  14862753  1165-OptoTester
  3:  13246358  1206-Fiz Roy
  4:  13914058  0108-Knowles
  5:  13245666  1208-Electronics Engineering
  6:  13328722  1235-Deneb
  7:  15690909  testproject
  8:
  9:
  10:
  11:



*/
