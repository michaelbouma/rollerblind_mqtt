void ethernetInit()
{

  byte i;
  byte dsAddress[8];
  Serial.begin(38400);
  Serial.println("Starting Arduino");

  Serial.println( "Searching for DS18B20..." );
  ds.reset_search();  // Start the search with the first device
  if( !ds.search(dsAddress) )
  {
    Serial.println( "none found. Using default MAC address." );
    ReadMac();
  } else {
    Serial.println( "success. Setting MAC address:" );
    Serial.print( " DS18B20 ROM  =" );
    for( i = 0; i < 8; i++)
    {
      Serial.write(' ');
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
//  WriteMac();
  //ReadMac();
  // Retrieve IP address via DHCP
  if (Ethernet.begin(mac) == 0) {
    // no point in carrying on, so do nothing forevermore:
    Serial.println("No IP Address, Halted");
    for(;;)
      ;
  }
    // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
    
  // Retrieve MQTT broker IP Address (DHCP option 151)  
  IPAddress mqttBroker(Ethernet.mqttBrokerIP());
  

  Serial.print("My MQTT broker: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
     //print the value of each byte of the IP address:
    Serial.print(Ethernet.mqttBrokerIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
}

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

void WriteMac()  
{
  for (int i=0;i<6;i++)
    EEPROM.write(i, writemac[i]);
}

void ReadMac()
{
  // Retrieve mac address from eeprom 
  for (int i=0;i<6;i++)
  {
    mac[i]=EEPROM.read(i);
  }  
}
