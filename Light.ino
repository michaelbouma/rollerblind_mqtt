void lightsInit()
{
  Serial.println();
  
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
      strip_colors[0] = Color(lamp_value[0],lamp_value[1],lamp_value[2]);
      post_frame();
  
      LightDelayElapsed = 0;
  }
}

// ##################################################
// ##      Send data to the WS2801 Led Strip       ##
// ##################################################

void post_frame (void) {
  //Each LED requires 24 bits of data
  //MSB: R7, R6, R5..., G7, G6..., B7, B6... B0 
  //Once the 24 bits have been delivered, the IC immediately relays these bits to its neighbor
  //Pulling the clock low for 500us or more causes the IC to post the data.

  for(int LED_number = 0 ; LED_number < 1 ; LED_number++) {
    long this_led_color = strip_colors[LED_number]; //24 bits of color data

    for(byte color_bit = 23 ; color_bit != 255 ; color_bit--) {
      //Feed color bit 23 first (red data MSB)
      
      digitalWrite(CKIPIN, LOW); //Only change data when clock is low
      
      long mask = 1L << color_bit;
      //The 1'L' forces the 1 to start as a 32 bit number, otherwise it defaults to 16-bit.
      
      if(this_led_color & mask) 
        digitalWrite(SDIPIN, HIGH);
      else
        digitalWrite(SDIPIN, LOW);
  
      digitalWrite(CKIPIN, HIGH); //Data is latched when clock goes high
    }
  }

  //Pull clock low to put strip into reset/post mode
  digitalWrite(CKIPIN, LOW);
  delayMicroseconds(500); //Wait for 500us to go into reset
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

