void MQTTinit() {
    sprintf (macString, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);  
    sprintf (buff, "%s%s/#", basevector, macString);
    mqttClient.subscribe(buff); // subscribing to e.g. raw/mac/MAC/#
    Serial.print("Subscribing to Topic: ");
    Serial.println(buff);

    // First Send init to reset OpenHAB
    mqttClient.publish(basevector, "init");
    // Now publish this MAC address to a generic topic e.g. raw/mac
    Serial.print("Sending to Topic: ");
    Serial.print(basevector);
    Serial.print(" Message: ");
    Serial.println(macString);
    mqttClient.publish(basevector, macString);
    
    // Retrieve Zonename and Zonevect from openHab
    unsigned long mqttconnecttimeout = millis();
    while (basetopic[0] == '<') { // whilst we haven't finished downloading controller startup stuff
      if (millis() - mqttconnecttimeout > initialmqttconnecttimeout) {
        Serial.println("MQTT timeout");
        break;
      }
      mqttClient.loop(); // This is usually found in the main loop, but we are checking for incoming MQTT messages at this stage too
    }
    mqttresponsetime = millis() - mqttconnecttimeout;

    // Set up commandtopic
    sprintf(commandtopic, "%s%s", basetopic, statusvector);
    mqttClient.publish(commandtopic, "Setup Done!");
    
    sprintf(commandtopic, "%s%s", basetopic, "/Light/#");
    mqttClient.subscribe(commandtopic);  
    Serial.print("Subscribing to topic: ");
    Serial.println(commandtopic);
    sprintf(commandtopic, "%s%s", basetopic, "/Lgroup/#");
    mqttClient.subscribe(commandtopic);  
    Serial.print("Subscribing to topic: ");
    Serial.println(commandtopic);
    sprintf(commandtopic, "%s%s", basetopic, "/Curtain/#");
    mqttClient.subscribe(commandtopic);  
    Serial.print("Subscribing to topic: ");
    Serial.println(commandtopic);
    sprintf(commandtopic, "%s%s", basetopic, "/Cgroup/#");
    mqttClient.subscribe(commandtopic);  
    Serial.print("Subscribing to topic: ");
    Serial.println(commandtopic);
    sprintf(commandtopic, "%s%s", basetopic, "/Command/#");
    mqttClient.subscribe(commandtopic);  
    Serial.print("Subscribing to topic: ");
    Serial.println(commandtopic);
    
}

void mqttRun()
{
  if (!mqttClient.loop()) 
  {   // ProcessMQTT events
    mqttConnected = false;
    if (MQTTDisconnectElapsed > MQTTDisconnect)
    {
      
      Serial.println("MQTT Client disconnected...");
      if (mqttClient.connect(MQTTName))
      {
        Serial.println("MQTT reconnected");
        MQTTinit();
      } 
    
      else 
      {
        Serial.println("MQTT failed !!");
        Serial.print("retry in ");
        Serial.print(MQTTDisconnect);
        Serial.println(" milliseconds");
      }
      MQTTDisconnectElapsed = 0;
    }
  }
  else
    mqttConnected = true;
}


// ##################################################
// ##            MQTT Callback function            ##
// ##################################################

void callback(char* topic, byte* payload, unsigned int length) {
  char* json;
  json = (char*) malloc(length + 1);
  memcpy(json, payload, length);
  json[length] = '\0';  
  String MyTopic = topic;

  //Serial.println("Callback");

//  int i;
//  for (i=0;i<6;i++)
//  {
//    mac[i]=EEPROM.read(i);
//  }  

  String TmpStr;

  // Controller Setup / paging functionality
  sprintf(buff, "%s%02X%02X%02X%02X%02X%02X", basevector, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  TmpStr = buff;
  if(MyTopic.substring(0,TmpStr.length()) == TmpStr) {
    if (payload[4]==84) {
      for (int i=0; i<30; i++) basetopic[i] = payload[i+10]; // base topic (T)
    }
    if (payload[4]==82) {                                  // Remote reboot (R)
      resetFunc();
    }
  }
  
  sprintf(buff, "%s%s", basetopic, "/Light");
  TmpStr = buff;
  if (MyTopic.substring(0,TmpStr.length()) == TmpStr)
  {
    LightTopic = MyTopic.substring(TmpStr.length()+1) + ":" + json;
    Light = true;
  }  
  
  sprintf(buff, "%s%s", basetopic, "/Lgroup");
  TmpStr = buff;
  if(MyTopic.substring(0,TmpStr.length()) == TmpStr)
  {
    LgroupTopic = MyTopic.substring(TmpStr.length()+1) + ":" + json;
    Light = true;
  }  
  
  sprintf(buff, "%s%s", basetopic, "/Curtain");
  TmpStr = buff;
  if (MyTopic.substring(0,TmpStr.length()) == TmpStr)
  {
    blindsTopic = MyTopic.substring(TmpStr.length()+1) + ":" + json;
    blinds = true;
  }  
  
  sprintf(buff, "%s%s", basetopic, "/Cgroup");
  TmpStr = buff;
  if(MyTopic.substring(0,TmpStr.length()) == TmpStr)
  {
    CgroupTopic = MyTopic.substring(TmpStr.length()+1) + json;
    blinds = true;
  }  
  
  sprintf(buff, "%s%s", basetopic, "/Command");
  TmpStr = buff;
  if(MyTopic.substring(0,TmpStr.length()) == TmpStr)
  {
    StatusTopic = MyTopic.substring(TmpStr.length()+1) + json;
    statusSet = true;
  }  
  
 free(json);
}


