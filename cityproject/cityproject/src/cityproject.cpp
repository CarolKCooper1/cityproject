/* 
 * Project Capstone
 * Author: CKCooper (Visual BubbleSort for Photon 2 and NeoPixel tower and Interrupts by Brian Rashap)
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

//constants
const int SPIN=D5;
const int SPIN1=D6;
const int SERVPIN = D13;
const int COUNT=5;
const int steps=512;

//variables
int counter;
int scentCounter;
int seqCounter;
int subValue,subValue1;

// Bubble sort Constants
const int PIXEL_COUNT = 25;
const int SPEED = 25;
const int colors[] = {0xFF0000, 0xE31C00, 0xC63900, 0xAA5500, 0x718E00, 0x55AA00, 0x39C600, 0x1CE300, 0x00FF00,
                      0x00E31C, 0x00C639, 0x00AA55, 0x00718E, 0x0055AA, 0x0039C6, 0x001CE3, 0x0000FF};

// Bubble sort Variables
int colorArray[PIXEL_COUNT];
bool sortShow;

Stepper myStepper(steps, D16, D17, D15, D18);
TCPClient TheClient; 
IoTTimer pixelTimer;
Servo myServo;
IoTTimer seqTimer;
IoTTimer scentTimer;
Button soundButton(D1);
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

//Bubble Sort objects
Adafruit_NeoPixel strip(PIXEL_COUNT, SPI1, WS2812B);
IoTTimer showTimer;
//IoTTimer showTimer1;

//functions
int randomPixel();
void MQTT_connect();
bool MQTT_ping();
void playsounds();

// Prototypes for local build, ok to leave in for Build IDE
void fillArray(int *clrArray, int n);
void bubbleSort(int *sortArray, int n);
void showArray(int *pixelArray, int n, int speed = SPEED);
void printArray(int *pixelArray, int n); 
void stopTheShow();
void MQTTinterrupt();


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
//pinMode(D10,INPUT_PULLDOWN);
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
dfPlay.setVolume(20);			// Sets volume level to 10 (valid range = 0 to 30)
Selection SDcard = {2,0,0,0,0}; // Selects all tracks on the SD card
Serial.printf("tracks selected\n");
dfPlay.play(SDcard);			// Plays the selection
Serial.printf("Tracks playing\n");

myStepper.setSpeed(10);

//Bubble sort setup
delay(1000);
strip.begin();
strip.setBrightness(150);
strip.show(); // Initialize all pixels to 'off'
pinMode(A0,INPUT);
pinMode(A1,INPUT);
randomSeed(analogRead(A0)*analogRead(A1));
pinMode(D3,INPUT_PULLDOWN);
attachInterrupt(D3,stopTheShow,RISING);
sortShow = false;
showTimer.startTimer(10000);

}

void loop() {
MQTT_connect();
MQTT_ping();

if(sortShow){
  Serial.printf("sortShow = %i\n",sortShow);
  fillArray(colorArray,PIXEL_COUNT);
  bubbleSort(colorArray,PIXEL_COUNT);
} 
else {
//buttons to manually start the lights
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
          seqTimer.startTimer(80000);
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
          seqTimer.startTimer(1000);
          break;
      case 3://scent 1 off
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN, LOW);
          Serial.printf("scent off\n");
          seqCounter++;
          seqTimer.startTimer(500);
          break;
      case 4://scent 2 on
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN1, HIGH);
          Serial.printf("scent 2 on\n");
          seqCounter++;
          seqTimer.startTimer(1000);
          break;
      case 5://scent 2 off
          Serial.printf("SC %i\n",seqCounter);
          digitalWrite(SPIN1, LOW);
          Serial.printf("scent 2 off\n");
          seqCounter++;
          seqTimer.startTimer(500);
          break;
      case 6://stepper motor
          Serial.printf("SC %i\n",seqCounter);
          myStepper.step(-steps);
          myStepper.step(-steps);
          delay(500);
          myStepper.step(steps);
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

//switch case for scent and stepper motor
// switch case for internet
if(scentTimer.isTimerReady()) {
     switch (scentCounter){
      case 0://scent 1 on
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN, HIGH);
          Serial.printf("scent on\n");
          scentCounter++;
          scentTimer.startTimer(1000);
          break;
      case 1://scent 1 off
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN, LOW);
          Serial.printf("scent off\n");
          scentCounter++;
          scentTimer.startTimer(500);
          break;
      case 2://scent 2 on
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN1, HIGH);
          scentCounter++;
          scentTimer.startTimer(1000);
          break;
      case 3://scent 2 off
          Serial.printf("Scent Counter %i\n",scentCounter);
          digitalWrite(SPIN1, LOW);
          scentCounter++;
          scentTimer.startTimer(500);
          break;
      case 4://stepper motor
          Serial.printf("Scent Counter %i\n",scentCounter);
          myStepper.step(-steps);
          myStepper.step(-steps);
          delay(500);
          myStepper.step(steps);
        
         
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

if(showTimer.isTimerReady()) {
  showTimer.startTimer(10000);
  sortShow = true;
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

// Functions by Brian Rashap
// Fill the array with random colors (Blue to Red)
void fillArray(int *clrArray, int n) {
  int i;

  for(i=0;i<n;i++) {
    clrArray[i] = colors[random(sizeof(colors)/4)];
  }
  showArray(clrArray, n);
  delay(3300);
  return;
}

// Display the array on the neoPixels
void showArray(int *pixelArray, int n, int speed) {
  int i;

  for(i=0;i<n;i++) {
    strip.setPixelColor(i,pixelArray[i]);
  }
  strip.show();
  MQTTinterrupt();
  if(!sortShow) {
    return;
  }
  delay(speed);
  return;
}

// Print the array to the screen (useful for troubleshooting the sort)
void printArray(int *pixelArray, int n) {
  int i;
  for(i=0;i<n;i++) {
    Serial.printf("Array[%i] = %i\n",i,pixelArray[i]);
  }
  return;
}

// Bubble Sort Function
void bubbleSort(int *sortArray, int n) {
  int i,j,swap;

  for(i=0;i<n;i++) {
    for(j=0;j<(n-1);j++) {
      if(!sortShow) {
        return;
      }
      if(sortArray[j] > sortArray[j+1]) {
        swap = sortArray[j];
        sortArray[j] = sortArray[j+1];
        sortArray[j+1] = swap;
      }
    showArray(sortArray,n);
    }
  }
}

void stopTheShow() {
  sortShow = false;
  showTimer.startTimer(250000);
}
void MQTTinterrupt(){

Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &buttonFeed) {
      subValue = atoi((char *)buttonFeed.lastread);
      Serial.printf("Button Subscription %i \n", subValue);
    
    if(subValue==1){
      stopTheShow();
    }
    }
} 
}