/* 
 * Project Capstone
 * Author: CKCooper
 * Date: 11/28/2023
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include <neopixel.h>
#include "Colors.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "IoTTimer.h"
#include "Button.h"
#include "credentials.h"
#include <DFPlay.h>
#include "Stepper.h"

const int steps=2048;

Stepper myStepper(steps, D16, D17, D15, D18);
TCPClient TheClient; 
IoTTimer pixelTimer;
IoTTimer seqTimer;
IoTTimer scentTimer;
Button seqButton(D3);
Button scentButton(D10);
Button startButton(D4);
Adafruit_NeoPixel pixel(24, SPI1, WS2812B);
DFPlay dfPlay;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

/****************************** Feeds ***************************************/ 
// Setup Feeds to publish or subscribe 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname> 
Adafruit_MQTT_Subscribe buttonFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/buttononoff"); 
Adafruit_MQTT_Subscribe scentFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/scentbutton"); 
// Let Device OS manage the connection to the Particle Cloud

//variables
const int SPIN=D5;
const int SPIN1=D6;
const int SERVPIN = D13;
const int COUNT=5;
int counter;
int scentCounter;
int seqCounter;
int subValue,subValue1;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
//functions
int randomPixel();
void MQTT_connect();
bool MQTT_ping();
void playsounds();


SYSTEM_MODE(SEMI_AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

void setup() {
Serial.begin(9600);
waitFor(Serial.isConnected,10000);

pinMode (SPIN, OUTPUT);
pinMode (SPIN1, OUTPUT);
// pinMode(D4,INPUT_PULLDOWN);
pinMode(D10,INPUT_PULLDOWN);
// pinMode(D1,INPUT_PULLDOWN);
pixel.begin();
pixel.setBrightness(255);
pixel.show();

pixelTimer.startTimer(1000);
scentTimer.startTimer(1000);
counter=COUNT;
scentCounter=COUNT;
seqCounter=COUNT;

WiFi.on();
WiFi.connect();
while(WiFi.connecting()) {
  Serial.printf(".");
}
 Serial.printf("\n\n");

mqtt.subscribe(&buttonFeed);
mqtt.subscribe(&scentFeed);


//dhplayer setup
dfPlay.begin();					// Prepares DFPlay for execution
dfPlay.setVolume(15);			// Sets volume level to 10 (valid range = 0 to 30)
Selection SDcard = {2,0,0,0,0}; // Selects all tracks on the SD card
Serial.printf("tracks selected\n");
dfPlay.play(SDcard);			// Plays the selection
Serial.printf("Tracks playing\n");

myStepper.setSpeed(10);
}

void loop() {
MQTT_connect();
MQTT_ping();

//buttons to manually start the lights and open the scent box
if(startButton.isClicked()){
Serial.printf("pixel button is clicked\n");
  counter = 0;
}
//counter
if(counter<COUNT){
  counter=counter+randomPixel();
}
else{
  if(pixelTimer.isTimerReady()){
    pixel.clear();
    pixel.show();
}
}
//sound button
if (scentButton.isClicked()){
  Serial.printf("soundButton.isClicked\n");
  playsounds();
} 
else {
  dfPlay.stop();
  dfPlay.manageDevice();
} 


if(seqButton.isClicked()){
  Serial.printf("scent button is clicked\n");
  seqCounter=0;
  seqTimer.startTimer(1);
  Serial.printf("pixel button is clicked\n");
  counter = 0;
}

//switch case for sequence
if(seqTimer.isTimerReady()) {
     switch (seqCounter){
      case 0://lights
          Serial.printf("SC %i\n",seqCounter);
          Serial.printf("lights\n");
          //counter
        if(counter<COUNT){
          counter=counter+randomPixel();
          }
        else{
        if(pixelTimer.isTimerReady()){
          pixel.clear();
          pixel.show();
        }
      }
          seqCounter++;
          seqTimer.startTimer(50000);
          break;
      case 1://sound
          Serial.printf("SC %i\n",seqCounter);
          Serial.printf("sound\n");
          playsounds();
          dfPlay.stop();
          dfPlay.manageDevice();
          seqCounter++;
          seqTimer.startTimer(96000);
          break;
      case 2://scent 1 on
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN, HIGH);
          Serial.printf("scent on\n");
          seqCounter++;
          seqTimer.startTimer(10000);
          break;
      case 3://scent 1 off
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN, LOW);
          Serial.printf("scent off\n");
          seqCounter++;
          seqTimer.startTimer(5000);
          break;
      case 4://scent 2 on
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN1, HIGH);
          Serial.printf("scent 2 on\n");
          seqCounter++;
          seqTimer.startTimer(10000);
          break;
      case 5://scent 2 off
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN1, LOW);
          Serial.printf("scent 2 off\n");
          seqCounter++;
          seqTimer.startTimer(5000);
          break;
      case 6://stepper motor
          Serial.printf("SC %i\n",seqCounter);
          myStepper.step(-steps);
          Serial.printf("candy time\n");
          seqCounter++;
          seqTimer.startTimer(5000);
          break;

      }
}
if(scentButton.isClicked()){
  Serial.printf("scent button is clicked\n");
  scentCounter=0;
  scentTimer.startTimer(1);
}

// switch case for internet
if(scentTimer.isTimerReady()) {
     switch (scentCounter){
      case 0:
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN, HIGH);
          scentCounter++;
          scentTimer.startTimer(10000);
          break;
      case 1:
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN, LOW);
          scentCounter++;
          scentTimer.startTimer(5000);
          break;
      case 2:
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN1, HIGH);
          scentCounter++;
          scentTimer.startTimer(10000);
          break;
      case 3:
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN1, LOW);
          scentCounter++;
          scentTimer.startTimer(5000);
          break;
      case 4://stepper motor
          Serial.printf("Scentcounter %i\n",scentCounter);
          myStepper.step(-steps);
          Serial.printf("candy time\n");
          scentCounter++;
          scentTimer.startTimer(5000);
          break;
      }
}
Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) {
    if (subscription == &buttonFeed) {
      subValue = atoi((char *)buttonFeed.lastread);
      Serial.printf("Button Subscription %i \n", subValue);
    
    if(subValue==1){
      counter=0;
    }
    }
    if (subscription == &scentFeed) {
      subValue1 = atoi((char *)scentFeed.lastread);
      Serial.printf("scent Subscription %i \n", subValue1);
    
    if(subValue1==2){
      scentCounter=0;
      scentTimer.startTimer(1);
    }
    }
}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {//this lets adafruit know we're still here
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}

// function to return a random color on a random pixel on a timer
int randomPixel(){
  int whichPix;
  int randColor;
  int value;

  value=0;
  if(pixelTimer.isTimerReady()){
      pixel.clear();
      whichPix=random(10);
      randColor=random(7);
      Serial.printf("Pixel %i and Color %i \n", whichPix, randColor);
      pixel.setPixelColor(whichPix, rainbow[randColor]);
      pixel.show();
      pixelTimer.startTimer(10000);
      value=1;
  } 

  return value;
}
void playsounds(){
  Serial.printf("playsounds is running\n");
  dfPlay.manageDevice();     // Sends commands to DFPlayer & processes returned data.
  Selection SDcard = {2,0,0,0,0}; // Selects all tracks on the SD card
  dfPlay.play(SDcard);     // Plays the selection
  Serial.printf("Tracks playing\n");
  dfPlay.pause();
  Serial.printf("Player Paused\n");
 
}