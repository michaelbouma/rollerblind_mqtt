void lightsInit()
{
  Serial.println();
  strip.begin();
  strip.show();
  
  for(int i=0; i < Nlamps; i++)
  {
    // Set current lamp value to 0;
    lamp_value[i] = 0;
    // Set lamp value in groups to 0
    for(int j=0; j < NLgroups; j++)
      group_member[j][i] = 0;
    
    // Read startup value from eeprom
    lamp_fade[i]   = EEPROM.read(LightOffset + i);
    Serial.print("LightSetting from EEPROM for Light:");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(lamp_fade[i]);
  }
  for(int i=0; i < NLgroups; i++)
  {
      Serial.print("Loading settings for group");
      Serial.println(i);
      for(int j=0; j < Nlamps; j++)
      {
          group_member[i][j] = EEPROM.read((i * Nlamps) + LGroupOffset + j);
          Serial.print("Lamp");
          Serial.print(j);
          Serial.print(" = ");
          if (group_member[i][j] == 1)
            Serial.println("member");
          else  
            Serial.println("no member");
      } 
  }
  Light2 = true;
  LightStatus();
}

// *********************************************************
void LightStatus()
{
  for(int i=0; i < Nlamps; i++)
  {
    dtostrf(round(lamp_fade[i] / 2.55),0,0,buff);
    sprintf(commandtopic, "%s%s%s%i%s", basetopic, statusvector, lightTopic, i + 1,"/Level");
    mqttClient.publish(commandtopic,buff);
  }
}

// *********************************************************
// ProcessLight: Processes the via MQTT received values
// Lamps are in groups of three (r,g,b)
// *********************************************************
void ProcessLight() 
{
  if (LgroupTopic != "")
  // Group Functions
  {
    byte   GroupID = LgroupTopic.substring(0,1).toInt() - 1;
    String action  = LgroupTopic.substring(2);
    
    Serial.print("Group ");
    Serial.print(GroupID + 1);
    Serial.print(" action:");
    Serial.println(action);
    
    if      (action == "INCREASE")  group_fade[GroupID] += dim_step;  // Increase with stepsize
    else if (action == "DECREASE")  group_fade[GroupID] -= dim_step;  // decrease with stepsize
    else if (action == "ON")        group_fade[GroupID] = 255;        // Light to maximum
    else if (action == "OFF")       group_fade[GroupID] = 0;          // Light off
    else if (action == "MIN")       group_fade[GroupID] = 1;          // Light minimum
    else if (action == "SAVE")      
    {
      for(int i=0; i < Nlamps; i++)
      {
          EEPROM.write((GroupID * Nlamps) + LGroupOffset + i,group_member[GroupID][i]);
          Serial.print("Saving Group-");
          Serial.println(GroupID);
          Serial.print("EEProm offset = ");
          Serial.print((GroupID * Nlamps) + LGroupOffset + i);
          Serial.print(" value: ");
          Serial.println(group_member[GroupID][i]);

          
      }
    }  
    else                            group_fade[GroupID] = round(action.toInt() * 2.55); // convert percentage to byte

    // Limit minimum and maximum
    if (group_fade[GroupID] > 255) group_fade[GroupID] = 255;
    if (group_fade[GroupID] < 0)   group_fade[GroupID] = 0;

    // Set value of all lights in the group
    for(int i=0; i < Nlamps; i++)
    {
      if (group_member[GroupID][i] == 1)
        lamp_fade[i] = group_fade[GroupID];
    }
    // Clear Group Topic
    LgroupTopic = "";
    Light = false;
    Light2 = true;
  }
  
  if (LightTopic != "")
  // Light Functions
  {
    byte   LampID = LightTopic.substring(0,1).toInt() - 1;
    String action = LightTopic.substring(2);
        
    if      (action == "INCREASE")  lamp_fade[LampID] += dim_step;
    else if (action == "DECREASE")  lamp_fade[LampID] -= dim_step;
    else if (action == "ON")        lamp_fade[LampID] = 255;
    else if (action == "OFF")       lamp_fade[LampID] = 0;
    else if (action == "MIN")       lamp_fade[LampID] = 1;
    else if (action == "SAVE")      
    {
      EEPROM.write(LightOffset + LampID,lamp_fade[LampID]);
      Serial.print("Saving Value ");
      Serial.print(lamp_fade[LampID]);
      Serial.print(" to lamp ");
      Serial.println(LampID + 1);
      
    }
    else if (action.substring(0,4) == "GAdd")
    {  // Add Light to Group
      byte GroupID = action.substring(4).toInt() - 1;
      group_member[GroupID][LampID] = 1;
      Serial.print("Adding Light ");
      Serial.print(LampID + 1);
      Serial.print(" to group ");
      Serial.println(GroupID + 1);
    }
    else if (action.substring(0,4) == "GDel")
    {  // Delete Light from Group
      byte GroupID = action.substring(4).toInt() - 1;
      group_member[GroupID][LampID] = 0;
      Serial.print("Removing Light ");
      Serial.print(LampID + 1);
      Serial.print(" from group ");
      Serial.println(GroupID);
    }
    else
      lamp_fade[LampID] = round(action.toInt() * 2.55); // convert percentage to byte

    if (lamp_fade[LampID] > 255) lamp_fade[LampID] = 255;
    if (lamp_fade[LampID] < 0)   lamp_fade[LampID] = 0;

    //Serial.print("lamp:");Serial.print(LampID);Serial.print(" action:");Serial.println(LightTopic);
    //Serial.print("Lamp:");Serial.print(LampID);Serial.print(" Set to ");Serial.println(lamp_value[LampID]);

    LightStatus();
    LightTopic = "";
    Light = false;
    Light2 = true;
  }
}

void dsRun()
{
  if (dsDelayElapsed > (dsStart + dsRead))
  {
      char temperaturenow [15];
      dtostrf(sensors.getTempCByIndex(0),3, 2, temperaturenow);  //// convert float to char      
      Serial.print("Temperature is: "); 
      Serial.println(temperaturenow);  
      sprintf(commandtopic, "%s%s%s", basetopic, statusvector, dsTopic);
      mqttClient.publish(commandtopic,temperaturenow);
      dsRunning = false;
      dsDelayElapsed = 0;
  } 
  else if (dsDelayElapsed > dsStart)
  {
      if (dsRunning == false)
      {
        sensors.requestTemperatures();
        dsRunning = true;
      }
  }
}

void lightsRun()
{
  if (Light == true)
    ProcessLight();
    
  if (Light2 == true)
    if (LightDelayElapsed > LightDelay)
    {
      // increase or decrease lamp values for smooth transition between dim values.
      byte t = 0;  // indicator if all dim values are met
      for(int i=0; i < Nlamps; i++)
      {
        if (lamp_value[i] == lamp_fade[i])
          t++;
        if (lamp_value[i] > lamp_fade[i])
          lamp_value[i]--;
        if (lamp_value[i] < lamp_fade[i])
          lamp_value[i]++;
      }

      // if all lamps have reached there final level do not run this procedure anymore
      if (t > 2)
      {
        Light2 = false;
        LightStatus();
      }

      // Convert the three led values to a 32bit value
      strip.setPixelColor(0,Color(lamp_value[0],lamp_value[1],lamp_value[2]));
      strip.show();
  
      LightDelayElapsed = 0;
  }
}

// *********************************************************
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

