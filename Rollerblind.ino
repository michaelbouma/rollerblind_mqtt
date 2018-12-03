// *********************************************************
// blindsInit; Initialise the steppers with their initial settings
// *********************************************************
void blindsInit()
{
  for(int i=0; i < Nblinds; i++)
  {   
    end_point[i] = EEPROMReadlong((i * 4) + BlindsOffset);   // Read end_point of the curtains from EEPROM
    Serial.print("end_point from EEPROM for Curtain:");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(end_point[i]);

    steppers[i]->setMaxSpeed(maxBlindSpeed);                // Initialize maximum speed
    steppers[i]->setAcceleration(blindAcceleration);        // Initialize acceleration
    steppers[i]->setSpeed(blindSpeed);                      // Initialize running speed
    steppers[i]->setCurrentPosition(0);                     // Position on startup is assumed 0 (Top)
    
  }
}


void blindIrControl(int blindID)
{
  long endPoint = end_point[blindID];
  if (steppers[blindID]->isRunning() == true)
    steppers[blindID]->stop();
  else if (steppers[blindID]->currentPosition() == 0)
    steppers[blindID]->moveTo(endPoint);
  else  
    steppers[blindID]->moveTo(0);   
  blindStat[blindID] = true;
    
}

// *********************************************************
// blindsRun; Run the processblinds only when needed
// *********************************************************
void blindsRun() 
{
  if (blinds == true)
    Processblinds();
    
}

// *********************************************************
// statusRun; Run the processblinds only when needed
// *********************************************************
void statusRun() 
{
  if (statusSet == true)
  {
    String action = StatusTopic;
    Serial.print("Status action = ");
    Serial.println(action);
    if      (action == "Reset") digitalWrite(resetPin,LOW);
    else if (action == "Lights") LightStatus();
    
    statusSet = false;
  }
    
}


// *********************************************************
// blindsStatus; Process Status depening on position
// *********************************************************
void blindsStatus() 
{
  long curPos = 0;
  if (StatusElapsed > statusUpdate) // Check status only once per second
  {
    for(int StepperID=0; StepperID < Nblinds; StepperID++)
    {
      if (blindStat[StepperID] == true)
      {
        curPos = steppers[StepperID]->currentPosition();  
        if (curPos == 0)
        {
          steppers[StepperID]->disableOutputs(); // Switch of power on zero position
          blindStat[StepperID] = false;
        } 
        else if (curPos == steppers[StepperID]->targetPosition())
        {
          steppers[StepperID]->disableOutputs(); // Switch of power on zero position
          blindStat[StepperID] = false;
        }
        
        dtostrf(round(curPos / (end_point[StepperID] / 100)),0,0,buff);
        sprintf(commandtopic, "%s%s%s%i%s", basetopic, statusvector, rollerblindTopic, StepperID + 1,"/CurPos");
        mqttClient.publish(commandtopic,buff);
      }
    }
    if (NFullUpdate >= fullUpdate)
    {
      for(int StepperID = 0; StepperID < Nblinds; StepperID++)
      {
        dtostrf(round(curPos / (end_point[StepperID] / 100)),0,0,buff);
        sprintf(commandtopic, "%s%s%s%i%s", basetopic, statusvector, rollerblindTopic, StepperID + 1,"/CurPos");
        mqttClient.publish(commandtopic,buff);
        blindStat[StepperID] = true;
      }
      LightStatus();
      NFullUpdate = 0;
    }
    else
      NFullUpdate++;
    
    StatusElapsed = 0;
  }
}

// *********************************************************
// Processblinds: Processes the via MQTT received values
// *********************************************************
void Processblinds() 
{
  if (CgroupTopic != "")  
  {
    String action = CgroupTopic;
    Serial.print("Blinds Group action = ");
    Serial.println(action);

    for(int StepperID=0; StepperID<Nblinds; StepperID++)
    {    
      long endPoint = end_point[StepperID];
      if      (action == "UP")    steppers[StepperID]->moveTo(0);                                // Move to Top Position
      else if (action == "DOWN")  steppers[StepperID]->moveTo(endPoint);              // Move to Bottom Position
      else if (action == "STOP")  steppers[StepperID]->stop();
      else                        steppers[StepperID]->moveTo((endPoint / 100) * action.toInt()); // convert percentage to byte
      blindStat[StepperID] = true;
    }
    CgroupTopic = "";
  }
  
  // blinds Functions
  if (blindsTopic != "")
  {
    int    StepperID = blindsTopic.substring(0,1).toInt() - 1;
    String action = blindsTopic.substring(2,6);
    Serial.print("Blind:");
    Serial.print(StepperID);
    Serial.print(" action = ");
    Serial.println(action);
        
    if      (action == "UP")    steppers[StepperID]->moveTo(0);                                // Move to Top Position
    else if (action == "DOWN")  steppers[StepperID]->moveTo(end_point[StepperID]);             // Move to Bottom Position
    else if (action == "STAR")  steppers[StepperID]->setCurrentPosition(0);                    // Set current position as Top Position
    else if (action == "END")                                                                  // Set current position as Bottom Position
    {
      Serial.print("New end_point = ");
      Serial.print(steppers[StepperID]->currentPosition());
      Serial.print(" For curtain:");
      Serial.println(StepperID + 1);
      end_point[StepperID] = steppers[StepperID]->currentPosition();                            
      EEPROMWritelong((StepperID * 4) + BlindsOffset,end_point[StepperID]);                     // Save the new Bottom position
    }  
    else if (action == "MOVE")                                                                  // Move to set position
    {
      long newEndPoint = blindsTopic.substring(7).toInt();
      long CurrentPosition = steppers[StepperID]->currentPosition();
      steppers[StepperID]->moveTo(CurrentPosition + newEndPoint);
    }  
    else if (action == "STOP")  steppers[StepperID]->stop();
    else                        steppers[StepperID]->moveTo((end_point[StepperID] / 100) * action.toInt()); // convert percentage to byte
    blindStat[StepperID] = true;
  }
  blindsTopic = "";
  blinds = false;
}

// *********************************************************
// EEPROMWritelong
// This function will write a 4 byte (32bit) long to the eeprom at
// the specified address to address + 3.
// *********************************************************
void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}
      
// *********************************************************
// EEPROMReadlong
// This function will read a 4 byte (32bit) long from the eeprom at
// the specified address to address + 3.
// *********************************************************
long EEPROMReadlong(int address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
