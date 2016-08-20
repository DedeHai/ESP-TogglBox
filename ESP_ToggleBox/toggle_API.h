
bool toggl_connect(WiFiClientSecure* client)
{
  client->stop(); //disconnect and clean any previous connection (will crash because of out of ram if more than one connection is attempted without cleaning)
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client->connect(host, httpsPort)) {
    Serial.println("connection failed");
    return false;
  }

  if (client->verify(fingerprint, host)) {
    Serial.println("certificate matches");
    return true;
  } else {
    Serial.println("certificate doesn't match");
  }
  return false;
}

//(need to be connected to call this function)
bool readCurrentTimerID(WiFiClientSecure* client) //get currently running timer ID (returns false if not running, true if it is)
{
  if (!client->connected()) return false;
  String url = "/api/v8/time_entries/current";
  //  Serial.print("requesting URL: ");
  //  Serial.println(url);

  bool responseok = false;
  bool timerrunning = false;
  int timeout = 0;

  String header = "GET " + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Authorization: Basic " + authentication + "\r\n\r\n";

  client->print(header);
  //  Serial.print(header);

  while (client->available() == 0)
  {
    delay(10);
    if (timeout++ > 200) break; //wait 2s max
  }


  // Read all the lines of the reply and find the "id" entry
  while (client->available()) {
    String line = client->readStringUntil('\r');
      Serial.print(line);
    if (line.indexOf("200 OK" , 0) != -1)
    {
      responseok = true;
    }

    int idx_start = line.indexOf("\"id\"") + 5;
    int idx_end =  line.indexOf(",", idx_start);

    if (idx_start > 5 && idx_end > 5)
    {
      runningTimerID = line.substring(idx_start, idx_end);
      Serial.print(F("Timer ID = "));
      Serial.println(runningTimerID);

      timerrunning = true;
    }
  }

  if (responseok)   Serial.println(F("Server Response OK"));

  if (timerrunning) return true;

  return false;

}


//(need to be connected to call this function)
bool readWorkspaces(WiFiClientSecure* client) //get toggl workspaces (returns false if not successful)
{
  //get account information (user ID & Workspaces)

  String url = "/api/v8/me";
  //  Serial.print(F("requesting URL: "));
  //  Serial.println(url);

  bool responseok = false;
  int timeout = 0;
  byte writeindex = 0;

  String header = "GET " + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Authorization: Basic " + authentication + "\r\n\r\n";


  client->print(header);
  // Serial.print(header);

  while (client->available() == 0)
  {
    delay(10);
    if (timeout++ > 200) break; //wait 2s max
  }


  // Read all the lines of the reply and interpret them
  while (client->available()) {
    String line = client->readStringUntil('\r');
    //  Serial.print(line);
    if (line.indexOf("200 OK" , 0) != -1)
    {
      responseok = true;
    }

    int idx_start = line.indexOf("\"id\"") + 5;
    int idx_end =  line.indexOf(",", idx_start);

    //todo: read the user's name? format is: "fullname":"Damian Schneider"
    if (idx_start > 5 && idx_end > 5)
    {
      //first "id" is the user ID

      userID = line.substring(idx_start, idx_end);
      Serial.print(F("User ID = "));
      Serial.println(userID);

      //read all workspace IDs & names

      while (idx_start >= 0 && idx_end >= 0)
      {
        idx_start = line.indexOf("\"id\"", idx_end);
        if (idx_start > 0)
        {
          idx_start += 5;
          idx_end = line.indexOf(",", idx_start);
          if (idx_end > 0)
          {
            workspaceID[ writeindex] = line.substring(idx_start, idx_end); //read project ID
          } else break;
        }
        else break;
        //read the workspace's name
        idx_start = line.indexOf("name", idx_end);
        if (idx_start > 0)
        {
          idx_start += 7;
          idx_end = line.indexOf(",", idx_start);
          if (idx_end > 0)
          {
            workspaceName[ writeindex] = line.substring(idx_start, idx_end - 1); //read project ID
          } else break;
        } else break;

        //     Serial.print(workspaceID[ writeindex]);
        //     Serial.print("\t");
        //     Serial.println(workspaceName[ writeindex]); //debug
        writeindex++;
        if ( writeindex >= 12) break; //maximum reached
      }

    }
  }

  if (responseok) {

    return true;
  }

  return false;

}

//(need to be connected to call this function)
bool readProjects(WiFiClientSecure* client) //get toggl workspaces (returns false if not successful)
{

  String url;  
  bool responseok;
  int timeout = 0;
  byte writeindex = 0;
  
  //get projects from all workspaces:
  
  for (int i = 0; i < 12; i++) //go through all 12 possible workspaces
  {
    if (workspaceID[i].length() < 1) break; //empty workspace ID, we're done

    url = "/api/v8/workspaces/" + workspaceID[i] + "/projects";

    String header = "GET " + url + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Authorization: Basic " + authentication + "\r\n\r\n";

    client->print(header);
    //  Serial.print(header);

    responseok = false;
    timeout = 0;

    while (client->available() == 0)
    {
      delay(10);
      if (timeout++ > 200) break; //wait 2s max
    }
    // Read all the lines of the reply from server and interpret them
    while (client->available()) {
      String line = client->readStringUntil('\r');
      //  Serial.print(line);
      if (line.indexOf("200 OK" , 0) != -1)
      {
        responseok = true;
      }

      int idx_start = 0;
      int idx_end = 0;
      while (idx_start >= 0 && idx_end >= 0)
      {
        idx_start = line.indexOf("\"id\"", idx_end);
        if (idx_start > 0)
        {
          idx_start += 5;
          idx_end = line.indexOf(",", idx_start);
          if (idx_end > 0)
          {
            projectID[writeindex] = line.substring(idx_start, idx_end); //read project ID
          } else break;
        }
        else break;
        //read the project's name
        idx_start = line.indexOf("name", idx_end);
        if (idx_start > 0)
        {
          idx_start += 7;
          idx_end = line.indexOf(",", idx_start);
          if (idx_end > 0)
          {
            projectName[ writeindex] = line.substring(idx_start, idx_end - 1); //read project ID
          } else break;
        } else break;

        //    Serial.print(projectID[ writeindex]);
        //    Serial.print("\t");
        //    Serial.println(projectName[ writeindex]); //debug
        writeindex++;
        if ( writeindex >= 12) break; //maximum reached
      }


    }
    if (responseok) {
      Serial.println(F("Server Response OK"));
      display.print(F("."));
      display.display();
    }

  }
}
