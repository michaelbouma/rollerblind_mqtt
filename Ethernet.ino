void etherInit()
{
  byte i;
  uint8_t dsAddress[8];
  
  Serial.println( "Searching for DS18B20..." );
  ds.reset_search();  // Start the search with the first device
  if( !ds.search(dsAddress) )
  {
    Serial.println( "none found. Reading MAC address from EEPROM." );
    ReadMac();
  } else {
    Serial.println( "success. Setting MAC address:" );
    Serial.print( " DS18B20 ROM  =" );
    for( i = 0; i < 8; i++)
    {
      Serial.write(':');
      Serial.print( dsAddress[i], HEX );
    }
    Serial.println();
    
    // Offset array to skip DS18B20 family code, and skip mac[0]
    
    mac[1] = dsAddress[3];
    mac[2] = dsAddress[4];
    mac[3] = dsAddress[5];
    mac[4] = dsAddress[6];
    mac[5] = dsAddress[7];

  }
  PrintMac();
  
  Serial.println();
    Serial.println("Starting Ethernet..");
    //WriteMac();
  // Retrieve IP address via DHCP
  if (Ethernet.begin(mac) == 0) {
    // no point in carrying on, so do nothing forevermore:
    Serial.println("No IP Address, Halted");
    for(;;)
      ;
  }

  Serial.print(" My IP address : ");
  Serial.println(Ethernet.localIP());  
  // Retrieve MQTT broker IP Address (DHCP option 151)  
  IPAddress mqttBroker(Ethernet.mqttBrokerIP());
  
  Serial.print(" My MQTT broker: ");
  Serial.println(Ethernet.mqttBrokerIP());  
  Serial.println();
}

// *********************************************************
// PrintMac: Print the mac address
// *********************************************************
void PrintMac()
{
  Serial.print( " Ethernet MAC =" );
  for(int i = 0; i < 6; i++ )
  {
    Serial.write( ' ' );
    Serial.print( mac[i], HEX );
  }
  Serial.println();
}

// *********************************************************
// WriteMac: Write mac address to EEPROM
// *********************************************************
void WriteMac()  
{
  for (int i=0;i<6;i++)
    EEPROM.write(i, writemac[i]);
}

// *********************************************************
// ReadMac: Read mac address from EEPROM
// *********************************************************
void ReadMac()
{
  // Retrieve mac address from eeprom 
  for (int i=0;i<6;i++)
  {
    mac[i]=EEPROM.read(i);
  }  
}
