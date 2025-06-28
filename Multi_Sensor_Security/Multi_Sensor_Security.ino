/*
                                 Smart Multi-Sensor Security and Monitoring System            
                                ***************************************************

  Description:
  This project implements a multi-functional security and monitoring system using Arduino.
  It integrates various sensors and modules to detect obstacles, flames, and ambient light levels,
  and allows user interaction through an IR remote and a physical button.

  Features:
  1. **Ultrasonic Distance Monitoring**:
     - Measures the distance of nearby objects.
     - If an obstacle is detected closer than a predefined threshold, the system locks and triggers a warning.
     - Distance is displayed in cm/inches and can be switched via IR remote.

  2. **Flame Detection**:
     - Detects the presence of flame using an analog flame sensor.
     - If fire is detected, the system locks, sounds an alert, and blinks warning LEDs.

  3. **Luminosity Monitoring**:
     - Measures ambient light using two photoresistors.
     - Controls the brightness of an LED based on average light level.
     - Displays the light intensity value on the serial monitor.

  4. **IR Remote Control**:
     - Unlock system (PLAY button)
     - Reset settings (OFF button)
     - Change display mode (UP/DOWN)
     - Toggle distance unit (EQ button)

  5. **EEPROM Support**:
     - Saves the user’s preferred distance unit across restarts.

  6. **User Feedback & Display**:
     - Uses LEDs and a buzzer to indicate warnings and system states.
     - Serial Monitor shows real-time sensor data and status messages.

  Components Used:
  - Ultrasonic Sensor (HC-SR04)
  - Flame Sensor
  - Two Photoresistors (LDRs)
  - IR Receiver Module
  - IR Remote
  - EEPROM
  - LEDs and Buzzer
  - Push Button

  Developed by: HARTIK RAI
*/


#include <IRremote.h>
#include <EEPROM.h>
#define PHOTORESISTOR_PIN1 A0
#define PHOTORESISTOR_PIN2 A1
#define FLAME_SENSOR A2
#define EEPROM_ADDRESS_DISTANCE_UNIT 50 
#define IR_RECEIVER 5
#define IR_BUTTON_PLAY 67
#define IR_BUTTON_OFF 22
#define IR_BUTTON_UP 21
#define IR_BUTTON_DOWN 7 
#define IR_BUTTON_EQ 9
#define ECHO_PIN 3
#define TRIGGER_PIN 4
#define LED_1 11
#define LED_3 6
#define ERROR_LED 10
#define BUTTON 2
#define BUZZER 12
int distanceInCm = 0;
int distanceInInches = 1;
double cmToInches = 0.393701; //constant that we have to multiply to convert cm into inches
int distanceUnit = 0;
unsigned long lastTimeTrigger = millis();
unsigned long triggerTimeperiod = 100;
volatile unsigned long pulseInStartTime = 0;
volatile unsigned long pulseInEndTime = 0;
volatile bool checkDistanceAvailable = false;
double lockDistance = 10.0; 
bool isLocked = false;
double previousDistance;
//FLAME_SENSOR;
unsigned long lastTimeFlame = millis();
unsigned long flameDetectTime = 200;
bool isFlameDetected = false;

//PHOTORESISTOR
unsigned long luminosity = 0;
unsigned long lastTimeLuminosity = millis();
unsigned long luminsoityMeasureAtTime = 200;

//WARNING LED
unsigned long lastTimeLED_1Blink = millis();
unsigned long LED_1BlinkTime = 4000;
int LED_1state = LOW;

//ERROR LED
unsigned long lastTimeErrorLEDBlink = millis();
unsigned long errorLEDBlinkTime = 300;
int errorLEDstate = LOW;
byte buzzerState = LOW;

//BUTTON
unsigned long lastTimeButtonPressed = millis();
unsigned long debounceDelay = 50;
byte buttonState;

//different Modes of screen
int screenMode = 0;
const int screenModeDistance = 0;
const int screenModeSettings = 1;
const int screenModeLuminosity = 2;
const int screenModeFlame = 3;


void lock(){
  if(!isLocked){
    isLocked = true;
    errorLEDstate = LOW; //to ensure both LEDs will blink at a same rate
    LED_1state = LOW;
  }
}

void unlock(){
  if(isLocked){
    isLocked = false;
    isFlameDetected = false; //if system is locked due to detection of flame.
    errorLEDstate = LOW;
    digitalWrite(ERROR_LED,errorLEDstate);
    buzzerState = LOW;
    digitalWrite(BUZZER,buzzerState); //turn off the buzzer when system is unlock.
  }
}

void detectFlame(){
  int val = analogRead(FLAME_SENSOR);
  if(val > 30){
    isFlameDetected = true;
    lock();
  }
  else{
    isFlameDetected = false;
  }
}

void luminosityMeasure(){
  //Take average of two different luminosity measured by two different photoresistor placed at two different location for better accuracy.
  long luminosity1 = analogRead(PHOTORESISTOR_PIN1);
  long luminosity2 = analogRead(PHOTORESISTOR_PIN2);
  luminosity = 0.5 * (luminosity1 + luminosity2);
  analogWrite(LED_3,255-(luminosity/4));  //luminosity gives val ranges 0 -> 1023 but analogWrite takes value ranges 0-> 255
}

void handleIRCommand(int command){
  switch(command){
    case IR_BUTTON_PLAY:{
      unlock();
      break;
    }
    case IR_BUTTON_OFF :{
      resetSettingsToDefault();
      break;
    }
    case IR_BUTTON_UP :{
      toggleScreenMode(false);
      break;
    }
    case IR_BUTTON_DOWN :{
      toggleScreenMode(true);
      break;
    }
    case IR_BUTTON_EQ :{
      toggleDistanceUnit();
      break;
    }
    default:{ //do nothing
    }
  }
}
void triggerPulseIn(){
  //send trigger pulse of 10 microseconds width
  digitalWrite(TRIGGER_PIN,LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN,HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN,LOW);
}

void pulseInInterrupt(){
  if(digitalRead(ECHO_PIN) == HIGH){
    pulseInStartTime = micros();
  }
  else{
    pulseInEndTime = micros();
    checkDistanceAvailable = true;
  }
}

double getUltraSonicDistance(){
  double duration = pulseInEndTime - pulseInStartTime ;
  double distance = duration/58.0;
  if(distance > 400.0){
    // to neglect the false readings.
    return previousDistance;
  }
  // add a complimentary filter for smooth transition
  distance = 0.6 * previousDistance + 0.4 * distance; 
  previousDistance  = distance;
  return distance; 
}

void blinkErrorLED(){
  errorLEDstate = ( errorLEDstate == HIGH ) ? LOW : HIGH;
  digitalWrite(ERROR_LED , errorLEDstate);
}

void blinkLED1(){
  LED_1state = ( LED_1state == HIGH ) ? LOW : HIGH;
  digitalWrite(LED_1 , LED_1state);
}

void resetSettingsToDefault(){
  if(screenMode == screenModeSettings){
    if(distanceUnit != distanceInCm){
      distanceUnit = distanceInCm;
      EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT,distanceUnit);
    }
    screenMode = screenModeDistance;    
    Serial.print("Settings have been reset.");
    digitalWrite(BUZZER,HIGH);
    delay(500);
    digitalWrite(BUZZER,LOW);

  }
}
void toggleDistanceUnit(){
  if(distanceUnit == distanceInCm){
    distanceUnit = distanceInInches;
  }
  else{
    distanceUnit = distanceInCm;
  }
  EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT,distanceUnit);
}

void toggleScreenMode(bool next){
  switch(screenMode){
    case screenModeDistance :{
      screenMode =(next) ? screenModeSettings :screenModeLuminosity ;
      break;
    }
    case screenModeSettings :{
      screenMode = (next) ? screenModeLuminosity : screenModeDistance;
      break;
    }
    case screenModeLuminosity :{
      screenMode = (next) ? screenModeDistance : screenModeSettings;
      break;
    }
    default: {
      screenMode = screenModeDistance;
    }
  }
}

void printScreen(double distance){
  if(isLocked){
    Serial.println("--------------------------------------------------");
    Serial.println("Obstacle...!!");
    Serial.println("Your System Has Been Locked.");
    Serial.println("Kindly press to unlock the system.");
    Serial.println("--------------------------------------------------");
  }
  else if(screenMode == screenModeDistance){
    Serial.println("--------------------------------------------------");
    if(distanceUnit == distanceInCm){
      Serial.print("Distance of the object from the sensor is : ");
      Serial.print(distance);
      Serial.println("cm. ");
    }
    else{
      Serial.print("Distance of the object from the sensor is : ");
      Serial.print(distance * cmToInches);
      Serial.println("in. ");
    }

    if(distance > 100.0){
      Serial.println("No obstacle");
    }
    else{
      Serial.println("!!! Warning !!!");
    }
    Serial.println("--------------------------------------------------") ;
  }
  else if(screenMode == screenModeSettings){
    Serial.println("--------------------------------------------------") ;
    Serial.println("Press on OFF to ");
    Serial.println("Reset settings...!");
    Serial.println("--------------------------------------------------") ;    
  }
  else if(screenMode == screenModeLuminosity){
    Serial.println("--------------------------------------------------") ;
    Serial.print("The Luminosity of the environment is ");
    Serial.println(luminosity);
    Serial.println("--------------------------------------------------") ;    
  }
}

void setup() {
  Serial.begin(115200); //baudrate
  pinMode(ECHO_PIN,INPUT);
  pinMode(TRIGGER_PIN,OUTPUT);
  pinMode(LED_1,OUTPUT);
  pinMode(ERROR_LED,OUTPUT);
  pinMode(LED_3,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN),pulseInInterrupt,CHANGE);
  digitalWrite(LED_1,LOW);
  pinMode(BUTTON,INPUT);
  buttonState = digitalRead(BUTTON);
  pinMode(BUZZER,OUTPUT);
  IrReceiver.begin(IR_RECEIVER);//on the IR Receiver

  distanceUnit = EEPROM.read(EEPROM_ADDRESS_DISTANCE_UNIT);
  if(distanceUnit == 255){
    distanceUnit = distanceInCm;
  }
}

void loop() {
  unsigned long timeNow = millis();
  if(timeNow - lastTimeLuminosity > luminsoityMeasureAtTime){ 
    //measure the luminosity of the environment continuosly with some time period
    lastTimeLuminosity += luminsoityMeasureAtTime;
    luminosityMeasure();
  }

  if(timeNow - lastTimeFlame > flameDetectTime){
    //check whether there is a fire or not at a regular interval
    detectFlame();
    lastTimeFlame += flameDetectTime;
  }

  if(isLocked){
    if(timeNow - lastTimeErrorLEDBlink > errorLEDBlinkTime){
      //blink both LEDs and buzzer with some time period.
      lastTimeErrorLEDBlink += errorLEDBlinkTime;
      blinkErrorLED();
      blinkLED1();
      buzzerState = ( buzzerState == HIGH ) ? LOW : HIGH;
      digitalWrite(BUZZER , buzzerState);
    }
    if(timeNow - lastTimeButtonPressed > debounceDelay){
      //check whether button is pressed or not
      //add some delay to solve the debounce problem of the button
      byte newButtonState = digitalRead(BUTTON);
      if(newButtonState != buttonState){
        //it means button is pressed
        lastTimeButtonPressed += debounceDelay;
        buttonState = newButtonState;
        if(buttonState == LOW){
          unlock();
        }
      }
    }
  }
  else{
    //blink the led1 with a frequency propoertional to distance.
    if(timeNow - lastTimeLED_1Blink > LED_1BlinkTime){
      lastTimeLED_1Blink += LED_1BlinkTime;
      blinkLED1();
    }
  }

  //send trigger pulse continously
  if(timeNow - lastTimeTrigger > triggerTimeperiod){
    lastTimeTrigger += triggerTimeperiod;
    triggerPulseIn();
  }  
  if(IrReceiver.decode()){
    IrReceiver.resume();
    long command = IrReceiver.decodedIRData.command;
    handleIRCommand(command);
    // Serial.println(command);
  }
  if(checkDistanceAvailable){
    checkDistanceAvailable = false;
    double distance = getUltraSonicDistance();
    LED_1BlinkTime = 4 * distance; //change the blink rate proportional with distance
    printScreen(distance);
    if(distance < lockDistance){
      lock();
    }
  }
}

