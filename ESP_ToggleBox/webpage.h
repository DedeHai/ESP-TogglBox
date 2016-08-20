///////////////////////////
// format the html response
///////////////////////////

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

//decode string from url-arguments
String urldecode(String input) // (based on https://code.google.com/p/avr-netino/)
{
  char c;
  String ret = "";

  for (byte t = 0; t < input.length(); t++)
  {
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {


      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }

    ret.concat(c);
  }
  return ret;

}

void sendPage(void)
{

  uint8_t i;
  Serial.println("HTML Page");

  if (server.args() > 0 )  // Save Settings
  {

    uint8_t i;


    for (i = 0; i < server.args(); i++ ) {
      //Wifi configuration:
      if (server.argName(i) == "SSID") config.ssid = urldecode(server.arg(i));
      if (server.argName(i) == "PASS") config.password = urldecode(server.arg(i));
      //API key:
      if (server.argName(i) == "PL_KEY")  config.APIkey = server.arg(i);

    }
    WriteConfig();
  }




String pageContent = "<HTML><HEAD> <title>Toggl Control</title></HEAD>\n";
pageContent += "<BODY bgcolor=\"#CC00FF\" text=\"#000000\">";
pageContent += "<FONT size=\"6\" FACE=\"Verdana\"><b>Toggl Control Box</b></FONT>";
pageContent += "<FONT size=\"4\" FACE=\"Verdana\">\n<BR><b>Setup</b><BR><BR></font>";
pageContent += "<form action=\"\" method=\"GET\">\n";
pageContent += "<b>WiFi Configuration</b><BR><BR>\n";
pageContent += "<input type=\"text\"  size=\"24\" maxlength=\"31\" name=\"SSID\" value=\"";
pageContent += config.ssid;
pageContent += "\"> SSID<BR>\n";

pageContent += "<input type=\"password\"  size=\"24\" maxlength=\"31\" name=\"PASS\" value=\"";
pageContent += config.password;
pageContent += "\"> Password<BR>\n";
pageContent += "<input type=\"submit\" value=\"Send\"><BR></form>";
pageContent += "<HR>"; //horizontal line-----------------------------------------------------------------------

pageContent += "<form action=\"\" method=\"GET\">\n";
pageContent += "<b>Server Setup</b><BR><BR>\n";
pageContent += "<input type=\"text\" size=\"35\" maxlength=\"40\" name=\"PL_KEY\" value=\"";
pageContent += config.APIkey;
pageContent += "\"> API Key<BR>\n";
pageContent += "<input type=\"submit\" value=\"Send\"><BR></form>";


server.send ( 200, "text/html", pageContent );

}



void handleNotFound() {

  // if(SD_initialized && loadFromSdCard(server.uri())) return;
  String message = "Not Found:\n\n";

  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}




