
// ##################################################
// ##                      dsRun                   ##
// ##################################################
void ds18B20Run()
{
  if (dsDelayElapsed > (dsStart + dsRead))
  {
      if (standalone==false)
        setColor(0,10,0);
      else  
        setColor(20,10,0);
      char temperaturenow [15];
      dtostrf(sensors.getTempCByIndex(0),3, 2, temperaturenow);  //// convert float to char      
      Serial.print("Temperature : "); 
      Serial.println(temperaturenow);  
      sprintf(commandtopic, "%s%s", basetopic, dsTopic);
      mqttClient.publish(commandtopic,temperaturenow);
      dsRunning = false;
      dsDelayElapsed = 0;
  } 
  else if (dsDelayElapsed > dsStart)
  {
      if (dsRunning == false)
      {
        setColor(50,50,50);
        sensors.setWaitForConversion(false);  // makes it async        
        sensors.requestTemperatures();
        //sensors.setWaitForConversion(true);
        dsRunning = true;
      }
  }
}
// ##################################################
// ##                    setColor                  ##
// ##################################################
void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
  analogWrite(bluePin, blueValue);
}

