void infraredRun()
{
  
  if (My_Receiver.GetResults(&My_Decoder)) 
  {
    My_Decoder.decode();		//Decode the data
    //    My_Decoder.DumpResults();
    if (My_Decoder.decode_type == NEC) 
    {
      switch(My_Decoder.value) {
        case 0xff807f:  //Delay Off - Stop all rollerblind motors
          CgroupTopic = "STOP";
          blinds = true;
        break;
        case 0xff00ff:  //Power - Set lamps to startup value or off
          
          for(int i=0; i < Nlamps; i++)
          {
            if (lampToggle == true)
              lamp_fade[i] = EEPROM.read(LightOffset + i);
            else  
              lamp_fade[i] = 0;
          }
          
          Light2 = true;
          //LightStatus();
          lampToggle = !lampToggle;
        break;
        case 0xff30cf:  //Down - All rollerblinds Down
          CgroupTopic = "DOWN";
          blinds = true;
        break;
        case 0xff906f:  //Up - All rollerblinds Up
          CgroupTopic = "UP";
          blinds = true;
        break;
        case 0xff50af:  //25% - All rollerblinds 25%
          CgroupTopic = "25";
          blinds = true;
        break;
        case 0xffb04f:  //50% - All rollerblinds 50%
          CgroupTopic = "50";
          blinds = true;
        break;
        case 0xffa857:  //75% - All rollerblinds 75%
          CgroupTopic = "75";
          blinds = true;
        break;
        case 0xff6897:  //100% - All rollerblinds 100%
          CgroupTopic = "100";
          blinds = true;
        break;
        case 0xff708f:  //1 - 
          blindIrControl(0);
        break;
        case 0xff609f:  //2
          blindIrControl(1);
        break;
        case 0xff28d7:  //3
          blindIrControl(2);
        break;
          case 0xff10ef:  //4
            Serial.println("Not yet appointed");
          break;
      }
    }
    My_Receiver.resume(); 		//Restart the receiver
  }
}
