#include <stdio.h>
#include <Wire.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <elapsedMillis.h>
#include <PubSubClient.h>
#include <AccelStepper.h>
#include <IRLib.h>

//******************************************************

/*
 * Pins used by the ethernet shield
 * 
D2 - Ethernet interrupt (optional with solder bridge "INT")
D4 - SD SPI CS
D10 - Ethernet SPI CS
D11 - Not connected (but should be SPI MOSI)
D12 - Not connected (but should be SPI MISO)
D13 - SPI SCK
A0 - SD Write Protect
A1 - SD Detect

 * RJ45 connector (ULN2003)
 * 1 - orange (coil 1)
 * 2 - 
 * 3 - 
 * 4 - blue (coil 4)
 * 5 - red (common)
 * 6 - pink (coil 3)
 * 7 - 
 * 8 - yellow (coil 2)
*/

#define FULL4WIRE    4

// pins for WS2801 shield
#define CKIPIN      8
#define SDIPIN      9
#define redPin      11    // RGB LED red pin   (pwm)
#define greenPin    12    // RGB LED green pin (pwm)
#define bluePin     7     // RGB LED blue pin  (pwm)

#define resetPin    5
// motor pins
#define motorPin1   30    // IN1 on the ULN2003 driver 1
#define motorPin2   31    // IN2 on the ULN2003 driver 1
#define motorPin3   32    // IN3 on the ULN2003 driver 1
#define motorPin4   33    // IN4 on the ULN2003 driver 1

#define motorPin5   34    // IN1 on the ULN2003 driver 2
#define motorPin6   35    // IN2 on the ULN2003 driver 2
#define motorPin7   36    // IN3 on the ULN2003 driver 2
#define motorPin8   37    // IN4 on the ULN2003 driver 2

#define motorPin9   38    // IN1 on the ULN2003 driver 3
#define motorPin10  39    // IN2 on the ULN2003 driver 3
#define motorPin11  40    // IN3 on the ULN2003 driver 3
#define motorPin12  41    // IN4 on the ULN2003 driver 3

#define IRPin       42    // data pin for the IR Receiver
#define OneWirePin  43    // data pin for the DS18B20

#define Reset1Pin   44    // data pin for rollercurtain 1 zero reset
#define Reset2Pin   45    // data pin for rollercurtain 1 zero reset
#define Reset3Pin   46    // data pin for rollercurtain 1 zero reset

#define MQTTName              "ArduinoC1"       // the name this Arduino uses to connect ot the broker, must be unique

#define basevector            "raw/mac/"        // do not exceed 10 chars, see below sram note
#define statusvector          "/Status"         // do not exceed 10 chars, see below sram note
#define lightTopic            "/Light/"
#define lightGroup            "/Lgroup/"
#define rollerblindTopic      "/Curtain/"
#define rollerblindGroup      "/Cgroup/"
#define dsTopic               "/Temp/"
#define controlTopic          "/Command/"
#define openhabTopic          "openhab/time"


#define heartbeat_timeout         61000        // we expect an MQTT message at least every 60 seconds from openhab
#define initialmqttconnecttimeout 35000        // should be 35000
#define MQTTDisconnect 5000                    // number of miliseconds between each mqtt reconnect try

#define Nblinds        3                       // Number of stepper / roller blinds
#define NBgroups       2                       // Number of roller blind groups

#define blindsEndPoint -20000                  // initial EndPoint of the blinds
#define statusUpdate   1000                    // time in miliseconds between each statusupdate
#define fullUpdate     60                      // Full status update send  = fullUpdate * statusUpdate miliseconds

#define STRIP_LENGTH   1                       // This is to use more than one RGB LED board. In this case only one board connected
#define NLgroups       2                       // Number of light groups used
#define LightDelay     10                      // Light Dimming speed, larger = slower, steptime in miliseconds
#define dim_step       10                      // Steps per increase / decrease

#define Nlamps         STRIP_LENGTH * 3                       // Number of lamps, = 3 * STRIP_LENGTH

#define dsStart        59250                   // Read temp once per minute
#define dsRead         750                     // time between request and display
// offsets for storage of values in EEPROM
#define MacOffset      0                                  // 0    Offset of Mac Address in Eeprom (6 positions)
#define LightOffset    MacOffset + 6                      // 6    Offset of Light settings in Eeprom 
#define BlindsOffset   LightOffset + Nlamps               // 9    Offset of Curtain endpoints in Eeprom (LightOffset + Nlamps)
#define LGroupOffset   BlindsOffset + (Nblinds * 4)       // 21
#define BGroupOffset   LGroupOffset + (NLgroups * Nlamps) // 27

#define UP             0
#define DOWN           1


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield

byte writemac[] = { 0xB0, 0x0B, 0xB0, 0x0B, 0x00, 0x06 };
byte mac[]      = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char macString[13]; // 12 digit MAC address displayed at startup plus null terminator...

char basetopic[30] = {'<','u','n','k','n','o','w','n','>','\0'};        // base topic (30 chars max to be received from OpenHAB)
char commandtopic[51];                   // array size: topicroot/zonevector/commandvector (max 11 + 15 + 11 respectively, plus 2x "/" strings)
  
int mqttresponsetime = 0;                // Boot sequence: show OpenHAB response time, i.e. difference in time between
unsigned long defaultmodetimer;          // used for sleep delay, capsense delay, set mode to default
unsigned long heartbeatsense = 0;        // We expect an MQTT message every 1 minute (60000 millis) at least
                                         // There's an OH rule sending datetime at least every minute.
                                         // We use this as our heartbeat
                                         
float maxBlindSpeed     = 500;           // maximum speed of the motors
float blindAcceleration = 200;           // Desired acceleration in steps per second
float blindSpeed        = 300;           // maximum constant speed

//byte Nlamps = STRIP_LENGTH * 3;
long strip_colors[STRIP_LENGTH];
int  lamp_fade[Nlamps];
int  group_fade[NLgroups];
int  group_member[NLgroups][Nlamps];

bool lampToggle    = true;
bool mqttConnected = false;
bool standalone    = true;

long end_point[Nblinds];
bool blindStat[Nblinds];
int  lamp_value[Nlamps];

int  NFullUpdate   = fullUpdate;

String LightTopic  = "";
String LgroupTopic = "";
String blindsTopic = "";
String CgroupTopic = "";
String StatusTopic = "";
bool Light         = false;
bool Light2        = false;
bool blinds        = false;
bool statusSet     = false;
bool dsRunning     = false;

char buff[43]; // general buffer. char array. Assuming topicroot is max 10 chars with null terminator
               // this should not ever exceed 43. #sram


IRrecv My_Receiver(IRPin); 
OneWire ds(OneWirePin); 
DallasTemperature sensors(&ds);

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);
AccelStepper stepper2(FULL4WIRE, motorPin5, motorPin7, motorPin6, motorPin8);
AccelStepper stepper3(FULL4WIRE, motorPin9, motorPin11, motorPin10, motorPin12);

AccelStepper* steppers[Nblinds] = {   
    &stepper1,
    &stepper2,
    &stepper3
};

// Initialize the Ethernet client library
EthernetClient ethClient;
// Initialzie the MQTT client
PubSubClient mqttClient(ethClient);

IRdecode My_Decoder;

elapsedMillis LightDelayElapsed;
elapsedMillis dsDelayElapsed;
elapsedMillis StatusElapsed;
elapsedMillis MQTTDisconnectElapsed;
elapsedMillis openhabTimeElapsed;


/******************************************/
/*        Pre-Setup Functions             */
/******************************************/

void(* resetFunc) (void) = 0; //declare reset function @ address 0

// ##################################################
// ##         System Setup, Only run once          ##
// ##################################################
void setup() {
  Serial.begin(38400);
  Serial.println("Starting Arduino");

  // pins for the WS2801 shield
  pinMode(SDIPIN,   OUTPUT);
  pinMode(CKIPIN,   OUTPUT);
  // pins for rgb led
  pinMode(redPin,   OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin,  OUTPUT);
  // pin for remote reset
  digitalWrite(resetPin, HIGH);
  pinMode(resetPin, OUTPUT);

  //Clear out the array
  strip_colors[0] = Color(30,10,10);
  post_frame();

  setColor(0, 0, 50); // Color blue
  
  My_Receiver.enableIRIn(); // Start the receiver

  etherInit();
  blindsInit();
  lightsInit();
  
  // Initialize MQTT
  mqttClient.setServer(Ethernet.mqttBrokerIP(),1883);
  mqttClient.setCallback(callback);
  setColor(0, 0, 50); // Color blue

  sensors.begin(); 
  
  // First subscribe to a topic consisting of this arduino's MAC address
  // Connect to Broker, give it arduino as the name
  // Connect to the broker  
  if (mqttClient.connect(MQTTName)) 
    MQTTinit();
  else
    Serial.println("No Connection");

  strip_colors[0] = 0;
  post_frame();
  
}


// ##################################################
// ##                  Main Loop                   ##
// ##################################################
void loop() {
  mqttRun();           // handle MQTT traffic
  
  blindsRun();         // Read MQTT and control the blinds

  blindsStatus();      // Update the position of the blinds via MQTT
  
  lightsRun();         // Read MQTT and control the lights

  ds18B20Run();        // Read ds18B20 data and write to MQTT topic

  statusRun();         // 

  infraredRun(); 
  
  for(int StepperID = 0; StepperID < Nblinds; StepperID++){    
    steppers[StepperID]->run();
  }
  standaloneRun();     // What to do if we run standalone
  
}


