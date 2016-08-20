

void WriteStringToEEPROM(int beginaddress, String str)
{
  if (str.length() > 40)  str.remove(40); //truncate string if its too long, eeprom only has space allocated for 31 characters per string
  char  charBuf[str.length() + 1];
  str.toCharArray(charBuf, str.length() + 1);
  for (int t =  0; t < sizeof(charBuf); t++)
  {
    EEPROM.write(beginaddress + t, charBuf[t]);
  }
}

String  ReadStringFromEEPROM(int beginaddress)
{
  byte counter = 0;
  char rChar;
  String retString = "";
  while (1)
  {
    rChar = EEPROM.read(beginaddress + counter);
    if (rChar == 0) break;
    if (counter > 31) break;
    counter++;
    retString.concat(rChar);

  }
  return retString;
}


void WriteConfig()
{
  Serial.println("Writing Config");
  EEPROM.write(0, 'C');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'G');


  WriteStringToEEPROM(8, config.ssid);
  WriteStringToEEPROM(42, config.password);
  WriteStringToEEPROM(76, config.APIkey);

  EEPROM.commit();
}





boolean ReadConfig()
{

  Serial.println("Reading Configuration");
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F'  && EEPROM.read(2) == 'G' )
  {
    Serial.println("Configurarion Found!");

    config.ssid = ReadStringFromEEPROM(8);
    config.password = ReadStringFromEEPROM(42);
    config.APIkey = ReadStringFromEEPROM(76);

    return true;
  }
  else
  {
    Serial.println(F("Configurarion NOT FOUND"));
    return false;
  }
}

void writeDefaultConfig(void)
{
  // DEFAULT CONFIG
  config.ssid = "MYSSID";
  config.password = "MYPASSWORD";
  config.APIkey = "get it at toggl.com";
  
  WriteConfig();
  Serial.println(F("Standard config applied"));
}
