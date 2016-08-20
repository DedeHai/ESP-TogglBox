Adafruit_SSD1306 display(0); //do not use reset pin
ESP8266WebServer server(80);    // The Webserver
#define SWITCHPOSITIONS 12 //number of positions on position-switch
#define ANALOGMAX 970 //maximum value that is read from analog (should be 1023, can be less, not clear why)

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

struct ESPConfig {
  String ssid; //32 bytes maximum
  String password; //32 bytes maximum
  String APIkey;  //17 bytes (16 byte key plus termination zero)
} config;


const char* host = "www.toggl.com";
const int httpsPort = 443;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "D1 27 31 46 99 DF FE C1 30 B2 00 7D 6C DE 62 02 86 AC 2D 54"; //got it using https://www.grc.com/fingerprints.htm

String userID;
String workspaceID[12]; //12 workspaces max
String workspaceName[12]; //12 workspaces max
String projectID[12]; //12 projects max
String projectName[12]; //12 projects max
String runningTimerID; //ID of currently running timer
bool isrunning = false; //true if timer is running (checked during setup)
bool startpressed; //button interrupt checking
bool stoppressed;
uint8_t switchposition;
bool switch_changed;
long projectstarttime; //millis time current project time tracking started (currently for display reference only)
String authentication; //authentication string, created during setup from API key using Base64 encoding (see API documentation)

void displaytime(long val); //function prototype

bool readSwitch(void) //returns true if switch position changed since last read
{
  static uint8_t lastswitchposition = 0;
  int state = analogRead(0)+(ANALOGMAX/(2*(SWITCHPOSITIONS-1))); //read analog value, add half a switch position value to detect in between the static values
  switchposition = map(state, 0, ANALOGMAX, 0, SWITCHPOSITIONS-1); //map analog read to switch positions

  if (switchposition != lastswitchposition)
  {
    
    Serial.print(switchposition);
    Serial.print("\t ");
    Serial.println(analogRead(0));
    lastswitchposition = switchposition;
    return true;
  }
  else return false;
}


void updateState(void)
{
  static long statetimestamp = 0;
  static int textscroll = 0;
  static uint8_t waitstate = 0;
  if (millis() - statetimestamp > 100) //do not read analog too fast or values will be wrong (or even crash)
  {
    statetimestamp = millis();
    if (readSwitch()) //switch position changed
    {
      switch_changed = true;
      textscroll = 0;
      //if (isrunning) stoppressed = true; //if timer is running, stop it
      waitstate = 0;
    }
    //now display the current selected project, timer and running state
    int16_t x1, y1;
    uint16_t w, h;
    display.clearDisplay();
    display.setFont(&FreeSans9pt7b);
    display.setCursor(textscroll, 14);
    if ( waitstate > 10)    textscroll -= 2;
    else waitstate++;
    display.getTextBounds((char*)projectName[switchposition].c_str(), 0, 0, &x1, &y1, &w, &h);
    if (textscroll < -w) textscroll = 140;
    display.println(projectName[switchposition]);
    display.setFont(&FreeSans18pt7b);
    display.setCursor(0, 63);
    if (isrunning) displaytime((millis() - projectstarttime) / 1000);
    else displaytime(0);
    
    display.display();
  }
}




void ICACHE_RAM_ATTR startButtonISR(void)
{
  startpressed = true;
}

void ICACHE_RAM_ATTR stopButtonISR(void)
{
  stoppressed = true;
}



