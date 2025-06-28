/*
*********************************************************************************************************************************
  Project Title: Reaction-Based LED Button Game with Buzzer Feedback

  Description:
  This project is a simple reflex-based game using an Arduino, four LEDs, four push buttons, and a buzzer. 
  The system randomly lights up one LED at a time. The player must press the corresponding button as quickly 
  as possible to score a point. If the correct button is pressed, the score increases. If the wrong button is 
  pressed or the player fails to respond in time, a buzzer sounds as negative feedback.

  Key Features:
  - Random LED selection with logic to prevent same LED repetition.
  - Button press detection with debounce handling.
  - Timeout feature: if no button is pressed in the given time, it's counted as a miss.
  - Buzzer feedback for incorrect or late responses.
  - Score tracking displayed in the Serial Monitor.

  Hardware Connections:
  - 4 LEDs connected to pins 12, 11, 10, and 9.
  - 4 Push Buttons connected to pins 2, 3, 4, and 5.
  - Buzzer connected to pin 6.
  - A0 is used as analog noise input for random seed generation.

*********************************************************************************************************************************
*/


#define LED1 12
#define LED2 11
#define LED3 10
#define LED4 9
#define B1 2
#define B2 3
#define B3 4
#define B4 5
#define BUZZER 6

int LED_Array[] = {LED1,LED2,LED3,LED4};
int Button_Array[] = {B1,B2,B3,B4}; 
bool buttonPressed = false;
int whichButtonPressed ; //use to identify the number of button pressed
unsigned long LED_beginTime = millis();
unsigned long lastButtonPressed = millis();
unsigned long changeTime = 2000; // after what time LED has to change
unsigned long debounceDelay = 200;
unsigned int count =0; //count how many time you pressed button correctly
unsigned long LED_timeNow =0; 
unsigned long buttonTimeNow = 0;
int LED_number = 0;  //which LED will glow 
bool LED_change_state = 1;
bool LED_active = false; // Flag to know if LED is still waiting for input
bool lastButtonState = false; //to avoid if button is pressed for too long time , it may create ambiguity in logic.

void randomlyOn(){
  int prev_number = LED_number; 
  lastButtonState = false;
  analogWrite(BUZZER,0); //if buzzzer is on then we have to off the buzzer
  LED_number = int(random(0,4)); //generate random number between 0 to 3
  if(prev_number == LED_number ){ //SAME LED ON FOR CONSECUTIVE TIME 
    LED_number = (LED_number+2)%4;
  }
  digitalWrite(LED_Array[LED_number],HIGH);
  LED_beginTime = LED_timeNow;
  Serial.print("LED_number = ");
  Serial.print(LED_number);
  Serial.print((" and count = "));
  Serial.println(count);
  LED_active = true;
  lastButtonPressed = millis(); //for Time out logic
}

void setup() {
  Serial.begin(115200);
  for(int i =0;i<4;i++){
    pinMode(LED_Array[i],OUTPUT);
    pinMode(Button_Array[i],INPUT);
  }
  pinMode(BUZZER,OUTPUT);
  randomSeed(analogRead(A0)); //it read noise value
}

void loop() {

  LED_timeNow = millis();

  if(LED_timeNow - LED_beginTime >= changeTime  ){ //after certain amount of time LED number has to be change 
    LED_change_state = true;
  }

  if(LED_change_state){
    digitalWrite(LED_Array[LED_number],LOW); //endure that on LED will off before glowing of another LED
    LED_change_state = false;
    randomlyOn();
  }

  buttonTimeNow = millis();
  if(buttonTimeNow - lastButtonPressed >= debounceDelay){ //to get rid of debounce problem of switch
    for(int i =0;i<4;i++){
      if(digitalRead(Button_Array[i]) == HIGH){ //check which button is pressed
        buttonPressed = true;
        whichButtonPressed = i;
        lastButtonPressed = buttonTimeNow;
        break;
      }
    }    
  }

  if(buttonPressed  && (buttonPressed != lastButtonState)){ //if button is pressed 
    buttonPressed = false;
    lastButtonState = true;
    LED_active = false;
    Serial.print("Button pressed ");
    Serial.println(whichButtonPressed);
    if(whichButtonPressed == LED_number){ //correct button pressed then increase the count value
      count++;
    }
    else{
      analogWrite(BUZZER,25); //for incorrect button , play the buzzer
      delay(100);
      analogWrite(BUZZER,0);
    }

    digitalWrite(LED_Array[LED_number],LOW);
    LED_change_state = true;  
  }
  
    if(!buttonPressed && (buttonTimeNow - LED_beginTime > changeTime-100)){ //Time out Logic 
      analogWrite(BUZZER,25);
      delay(100);
      analogWrite(BUZZER,0);
      digitalWrite(LED_Array[LED_number],LOW); 
      LED_change_state = true;         
    } 
}




// *******************************************************************************************************************
//-------------------------------------------> End of the program<----------------------------------------------------
//-----------------------------------------------> HARTIK RAI<--------------------------------------------------------
//----------------------------------------------> 24.JUNE.2025 <------------------------------------------------------
//********************************************************************************************************************

