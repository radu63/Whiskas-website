#include <Arduino.h>  // pinMode, digitalWrite, analogWrite, delay, millis, pulseIn, Serial
#include <Servo.h>    // Servo control for angle values
#include <Adafruit_NeoPixel.h> //Num, Pin, Color, begin, setPixelColor, show


#define POSFin 0b10001111 //adress of the bot to signal finish


//bool StartAllowed = false; // becomes true when start byte received
bool raceStarted = false; // becomes true when flag up
bool checkFlagStart(); //checks flagstate to start
int flagBaselineCm =-1; // baseline distance for flag detection
bool finishSent = false; // ensure finish packet sent only once

void sendFinishPacketOnce(){ //sends finish packet
  if(finishSent) return;
  finishSent = true;
  Serial.write((uint8_t)POSFin); //send it twice to ensure reception it needs to be read two times
  Serial.write((uint8_t)POSFin);
}


// NeoPixels
const int NEO_PIN = 4;
const int NEO_COUNT = 4;
Adafruit_NeoPixel pixels(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800); //Neo_KHZ800 standard communication speed

uint32_t COL_BLUE; //32-bit color values
uint32_t COL_GREEN;
uint32_t COL_YELLOW;
uint32_t COL_RED;
uint32_t COL_PURPLE;
uint32_t COL_WHITE;


// gripper
const int gripperPin = 12;
Servo gripper;


// motor pins
const int motorL_fwd = 3;
const int motorL_rev = 11;

const int motorR_fwd = 6;
const int motorR_rev = 5;

// flip direction if wiring is reversed
bool invertLeft  = false;
bool invertRight = false;

// sonar
const int trigger = 7;
const int echo    = 8;

// sonar rate limit (to avoid spamming / confusing other bots)
const unsigned long SONAR_INTERVAL_MS = 300;
unsigned long lastSonarPingMs = 0;
int lastDistanceCm = 999;


// line sensors
const int sensorCount = 8;
const int sensorPins[sensorCount] = {A0, A1, A2, A3, A4, A5, A6, A7};
int sensorValues[sensorCount];

const int BLACK = 850; // decrease for more sensitivity

enum LineState { LINE_NONE, LINE_LEFT, LINE_RIGHT, LINE_CENTER, LINE_T, LINE_END }; //define line states with names



// timings (ms)
unsigned long startBlindMs        = 850;
unsigned long grabHoldMs          = 350;
unsigned long turnLeftMs          = 630;
unsigned long afterTurnForwardMs  = 600;

// drop timing
unsigned long backupMs                = 3000;  // total reverse time
unsigned long releaseAfterReverseMs   = 250;   // reverse first, then open
unsigned long dropHoldMs              = 450;   // keep open while still reversing

// speeds while blind start
float startL = 0.85;
float startR = 1.00;

float turnSpd = 0.70;

float afterTurnL = 0.70;
float afterTurnR = 0.65;

float backL = 0.80;
float backR = 0.85;


// line follow speeds
float followCenterL = 0.85;
float followCenterR = 1.00;

float followLeftTurnL  = 0.55;
float followLeftTurnR  = 0.85;

float followRightTurnL = 0.85;
float followRightTurnR = 0.50;

// gripper angles
int gripOpen  = 100;
int gripClose = 40;

// gripper keep-alive (20x/sec)
const unsigned long GRIPPER_HZ_MS = 80;   // 50ms about 20Hz. increase if stutter
bool gripperHoldOpen = false;             // true = keep open, false = keep closed
unsigned long lastGripCmd = 0;            // last time gripper command sent

// drop marker detect
int markerBlackCount = 7;   // 7 of 8 sensors over black to consider marker seen
int markerStableMs   = 80;  // 80ms delay to consider marker stable before drop

unsigned long dropMinMs     = 2000; //first drop cannot be while blind
unsigned long nonBlackArmMs = 200; //200ms so crossing small gaps doesn't arm drop

// flag detection
const int FLAG_MIN_CM = 5;
const int FLAG_MAX_CM = 20;


// sonar avoid
int obstacleCm = 20; //distance to trigger obstacle avoidance

unsigned long avoidTurnLeftMs    = 500; //actually right now
unsigned long avoidForward1Ms    = 650;
unsigned long avoidCurveRightMs  = 800; //left now to turn back to line
unsigned long avoidForward2Ms    = 200;
unsigned long avoidTurnBackMs    = 0;
unsigned long avoidSeekMs        = 10;

float avoidTurnSpd = 0.70;
float avoidFwdL    = 0.85;
float avoidFwdR    = 0.85;

float avoidTurnRightL = 0.50;   
float avoidTurnRightR = 0.50;

float avoidTurnLeftL = 0.00;   
float avoidTurnLeftR = 0.90;

float avoidTurnBackL = 0.60;   
float avoidTurnBackR = 0.00;  

enum State { // define states with names
  WAIT_FLAG,
  START_FORWARD,
  START_GRAB,
  START_TURN_LEFT,
  START_FORWARD_AFTER_TURN,
  RUN_FOLLOW,

  AVOID_TURN_LEFT,
  AVOID_FORWARD_1,
  AVOID_CURVE_RIGHT,
  AVOID_FORWARD_2,
  AVOID_TURN_BACK,
  AVOID_SEEK_LINE,

  DROP_BACKUP_BEFORE_OPEN,
  DROP_OPEN_WHILE_BACKING,
  DROP_BACKUP_AFTER_OPEN,

  DONE
};

State state = WAIT_FLAG; // initial state
unsigned long stateStart = 0;

// drop logic
bool dropArmed = false;
unsigned long runFollowStart = 0;
unsigned long nonBlackSince = 0;

// marker stable
bool markerStable = false;
unsigned long markerSince = 0;

// drop reverse timing
unsigned long dropReverseStart = 0;


void ledsAll(uint32_t c){ 
  for(int i=0; i<NEO_COUNT; i++){ //for every led set color and put it in memory
    pixels.setPixelColor(i, c);
  }
  pixels.show();
}

void ledsLeftRight(uint32_t leftC, uint32_t rightC){ //Sets left and right to the array adress
  pixels.setPixelColor(3, leftC);
  pixels.setPixelColor(0, leftC);
  pixels.setPixelColor(1, rightC);
  pixels.setPixelColor(2, rightC);
  pixels.show();
}

void setLedsForState(State s){
  if(s == START_FORWARD || s == START_GRAB || s == START_TURN_LEFT || s == START_FORWARD_AFTER_TURN){ //colors for specific states
    ledsAll(COL_BLUE);
  }
  else if(s == RUN_FOLLOW){
    ledsAll(COL_GREEN);
  }
  else if(s == AVOID_TURN_LEFT || s == AVOID_FORWARD_1 || s == AVOID_CURVE_RIGHT || s == AVOID_FORWARD_2 || s == AVOID_TURN_BACK || s == AVOID_SEEK_LINE){
    ledsAll(COL_RED);
  }
  else if(s == DROP_BACKUP_BEFORE_OPEN || s == DROP_OPEN_WHILE_BACKING || s == DROP_BACKUP_AFTER_OPEN || s == DONE){
    ledsAll(COL_PURPLE);
  }
  else{
    ledsAll(COL_GREEN);
  }
}

void setLedsForLine(LineState line){ //colors for line states curves
  if(line == LINE_LEFT){
    ledsLeftRight(COL_YELLOW, COL_GREEN);
  }
  else if(line == LINE_RIGHT){
    ledsLeftRight(COL_GREEN, COL_YELLOW);
  }
  else{
    ledsAll(COL_GREEN);
  }
}

//  motors

void motorSpeedAdjuster(int pinFwd, int pinRev, float speed, boolean forward, boolean invertDir){ //converts pwm so the user can use 0.0-1.0 floats
  int pwm = (int)(255.0f * speed); // convvert 0.0-1.0 to 0-255
  pwm = constrain(pwm, 0, 255); //range limit

  bool dir = forward;
  if(invertDir) dir = !dir; //if invert flip

  if(dir){ // always opposite pins for fwd/rev
    analogWrite(pinRev, 0);
    analogWrite(pinFwd, pwm);
  } else {
    analogWrite(pinFwd, 0);
    analogWrite(pinRev, pwm);
  }
}

void stopMotors(void){ // set variable for stop
  analogWrite(motorL_fwd, 0);
  analogWrite(motorL_rev, 0);
  analogWrite(motorR_fwd, 0);
  analogWrite(motorR_rev, 0);
}

void driveForward(float l, float r){ //variable for forward drive
  motorSpeedAdjuster(motorL_fwd, motorL_rev, l, true,  invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, r, true,  invertRight);
}

void driveBackward(float l, float r){ //variable for backward drive
  motorSpeedAdjuster(motorL_fwd, motorL_rev, l, false, invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, r, false, invertRight);
}

void turnLeftBlind(float spd){
  motorSpeedAdjuster(motorL_fwd, motorL_rev, spd, false, invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, spd, true,  invertRight);
}

// pivot RIGHT on the spot (away from obstacle)
void obstaclePivotRight(float leftSpd, float rightSpd){
  motorSpeedAdjuster(motorL_fwd, motorL_rev, leftSpd,  true,  invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, rightSpd, false, invertRight);
}

// pivot LEFT on the spot (back towards line)
void obstaclePivotLeft(float leftSpd, float rightSpd){
  motorSpeedAdjuster(motorL_fwd, motorL_rev, leftSpd,  true, invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, rightSpd, true,  invertRight);
}

void curveRight(float leftSpd, float rightSpd){
  motorSpeedAdjuster(motorL_fwd, motorL_rev, leftSpd,  true, invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, rightSpd, true, invertRight);
}

//  sonar

int readDistanceCm(void){ // returns 999 if no reading or invalid state
if(state != RUN_FOLLOW && state != WAIT_FLAG){
  return 999;
}

  unsigned long now = millis();

  if(now - lastSonarPingMs < SONAR_INTERVAL_MS){ //return distance if sonar interval not reached
    return lastDistanceCm;
  }
  lastSonarPingMs = now;

  digitalWrite(trigger, LOW); //ultrasonic pulse ping standard
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);

  long us = pulseIn(echo, HIGH, 6000); // measure pulse width with timeout
  if(us == 0){
    lastDistanceCm = 999;
  } else {
    lastDistanceCm = (int)((us * 0.034f) / 2.0f); // convert data to cm
  }

  return lastDistanceCm;
}

bool checkFlagStart(){
  int d = readDistanceCm();
  if(d == 999) return false;

  return (d >= FLAG_MIN_CM && d <= FLAG_MAX_CM);
}


//  line

LineState readLine(void){
  for(int i=0; i<sensorCount; i++){
    sensorValues[i] = analogRead(sensorPins[i]); //refresh sensor values
  }

  bool right  = sensorValues[0] > BLACK || sensorValues[1] > BLACK || sensorValues[2] > BLACK; //Sets black threshold for line sensors for each side
  bool center = sensorValues[3] > BLACK || sensorValues[4] > BLACK;
  bool left   = sensorValues[5] > BLACK || sensorValues[6] > BLACK || sensorValues[7] > BLACK;

  if(left && center && right) return LINE_T;        //black square detected
  if(!left && !center && !right) return LINE_END;   //line lost
  if(center) return LINE_CENTER;                    //center
  if(left) return LINE_LEFT;                        //left
  if(right) return LINE_RIGHT;                      //right
  return LINE_NONE;                                 //no line detected 
}

bool lineSeenNow(void){ //looks for any sensor seeing black. Makes the obstacle avoidance seek line easier
  for(int i=0; i<sensorCount; i++){
    if(sensorValues[i] > BLACK) return true;
  }
  return false;
}

void handleLine(LineState line){ //picks motor speeds based on line state
  if(line == LINE_CENTER){
    driveForward(followCenterL, followCenterR);
  }
  else if(line == LINE_LEFT){
    motorSpeedAdjuster(motorL_fwd, motorL_rev, followLeftTurnL, false, invertLeft);
    motorSpeedAdjuster(motorR_fwd, motorR_rev, followLeftTurnR, true,  invertRight);
  }
  else if(line == LINE_RIGHT){
    motorSpeedAdjuster(motorL_fwd, motorL_rev, followRightTurnL, true,  invertLeft);
    motorSpeedAdjuster(motorR_fwd, motorR_rev, followRightTurnR, false, invertRight);
  }
  else{
    driveForward(followCenterL, followCenterR);
  }
}

// obstacle avoidance turns ONLY

// hard pivot RIGHT (on the spot)
void obstacleTurnRight(float spd){
  // left motor forward, right motor backward
  motorSpeedAdjuster(motorL_fwd, motorL_rev, spd, true,  invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, spd, false, invertRight);
}

// hard pivot LEFT (on the spot)
void obstacleTurnLeft(float spd){
  // left motor backward, right motor forward
  motorSpeedAdjuster(motorL_fwd, motorL_rev, spd, true, invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, spd, true,  invertRight);
}



//  marker stable

void updateMarkerStable(void){
  unsigned long now = millis();

  int blackCount = 0; //sensor count 0
  for(int i=0; i<sensorCount; i++){
    if(sensorValues[i] > BLACK) blackCount++;
  }

  if(blackCount >= markerBlackCount){ // make sure it is the black square with ms
    if(markerSince == 0) markerSince = now;
    if(now - markerSince >= (unsigned long)markerStableMs){
      markerStable = true;
    }
  } else {
    markerSince = 0;
    markerStable = false;
  }
}

//  gripper keep-alive

void gripperHoldUpdate(void){ //update gripper position
  unsigned long now = millis();
  if(now - lastGripCmd < GRIPPER_HZ_MS) return;

  lastGripCmd = now;

  if(gripperHoldOpen){
  gripper.write(gripOpen);  
} else {
  gripper.write(gripClose); 
}
}


//  state

void enterState(State s){
  state = s; //give the state a timer
  stateStart = millis();

  if(s == DONE && !finishSent){
    finishSent = true;
    Serial.write((uint8_t)POSFin); //send finish packet
    Serial.write((uint8_t)POSFin);
  }

  if(s == START_GRAB){ //force close gripper
    gripperHoldOpen = false;
    gripper.write(gripClose);
  }

  if(s == RUN_FOLLOW){ //close gripper and reset drop logic
    dropArmed = false;
    runFollowStart = millis();
    nonBlackSince = 0;
    gripperHoldOpen = false;
  }

  if(s == DROP_BACKUP_BEFORE_OPEN){ //start backup then drop
    dropReverseStart = millis();
    gripperHoldOpen = false;
  }

  if(s == DROP_OPEN_WHILE_BACKING){ //open gripper while still backing
    gripperHoldOpen = true;
    gripper.write(gripOpen);
  }

  setLedsForState(s);
  if(s == DROP_BACKUP_BEFORE_OPEN){ //color indicator
  //sendFinishPacketOnce();
}
  if(s == DONE){ // you did it!!
  //sendFinishPacketOnce();
}

}

//  setup / loop

void setup(){
  Serial.begin(9600);

  pixels.begin(); //initialize neo pixels with pins e.g
  pixels.setBrightness(40); //set brightness 0-255, but 255 is very bright
  COL_BLUE   = pixels.Color(0, 0, 240);
  COL_GREEN  = pixels.Color(0, 240, 0);
  COL_YELLOW = pixels.Color(245, 170, 0);
  COL_RED    = pixels.Color(245, 0, 0);
  COL_PURPLE = pixels.Color(160, 0, 245);
  COL_WHITE  = pixels.Color(245, 245, 245);
  ledsAll(COL_WHITE); //boot color

  pinMode(motorL_fwd, OUTPUT); //pin setup motors as output
  pinMode(motorL_rev, OUTPUT);
  pinMode(motorR_fwd, OUTPUT);
  pinMode(motorR_rev, OUTPUT);

  pinMode(trigger, OUTPUT); //signal out
  pinMode(echo, INPUT); //signal in

  for(int i=0; i<sensorCount; i++){ //line sensor pins as input
    pinMode(sensorPins[i], INPUT);
  }

  gripper.attach(gripperPin); //attach gripper to pin
  gripperHoldOpen = false; // start open
  gripper.write(gripClose);


  stopMotors(); //ensure motors are stopped at the beginning

  gripperHoldOpen = true; // open gripper at start
  gripper.write(gripOpen);

  state = WAIT_FLAG;
  setLedsForState(state); //set leds for starting state

  stateStart = millis();
}

void loop(){

if(state == WAIT_FLAG){
  stopMotors();
  ledsAll(COL_BLUE);

  lastSonarPingMs = 0;   

  int ok = 0;
  for(int i = 0; i < 5; i++){
    if(checkFlagStart()) ok++;
  }

  if(ok >= 3){
    raceStarted = true;
    enterState(START_FORWARD);
  }
  return;
}

  unsigned long now = millis(); //store current time

  LineState line = readLine(); //read line sensors and classifys them
  updateMarkerStable();

  gripperHoldUpdate(); // update gripper position

  if(state == START_FORWARD){ // blind forward
    driveForward(startL, startR);
    if(now - stateStart >= startBlindMs){
      stopMotors();
      enterState(START_GRAB);
    }
  }

  else if(state == START_GRAB){ //blind grab
    if(now - stateStart >= grabHoldMs){
      enterState(START_TURN_LEFT);
    }
  }

  else if(state == START_TURN_LEFT){ // blind left turn
    turnLeftBlind(turnSpd);
    if(now - stateStart >= turnLeftMs){
      stopMotors();
      enterState(START_FORWARD_AFTER_TURN);
    }
  }

  else if(state == START_FORWARD_AFTER_TURN){ // blind forward after turn
    driveForward(afterTurnL, afterTurnR);
    if(now - stateStart >= afterTurnForwardMs){
      stopMotors();
      enterState(RUN_FOLLOW);
    }
  }

  else if(state == RUN_FOLLOW){ //start line following

    int d = readDistanceCm();// read sonar distance
    if(d <= obstacleCm){ //obstacle detected, start avoid routine
      stopMotors();
      enterState(AVOID_TURN_LEFT); // avoidance start
      return;
    }

    handleLine(line); // adjust motors for line
    setLedsForLine(line); // adjust leds for line

    if(!dropArmed){ // getting into ready state
      if(now - runFollowStart >= dropMinMs){ // minimum follow time before drop allowed
        if(!markerStable){ // only arm drop when marker not seen
          if(nonBlackSince == 0) nonBlackSince = now;
          if(now - nonBlackSince >= nonBlackArmMs){
            dropArmed = true;
          }
        } else {
          nonBlackSince = 0;
        }
      }
    }

    if(dropArmed && markerStable){ //next stable marker detected, start drop
      stopMotors();
      enterState(DROP_BACKUP_BEFORE_OPEN);
    }
  }

  // avoid: left -> forward -> curve right -> forward -> hard right -> seek line
else if(state == AVOID_TURN_LEFT){ // obstacle: pivot RIGHT
// RIGHT pivot (first turn)
    obstaclePivotRight(avoidTurnRightL, avoidTurnRightR);


  if(now - stateStart >= avoidTurnLeftMs){
    stopMotors();
    enterState(AVOID_FORWARD_1);
  }
}


  else if(state == AVOID_FORWARD_1){ // forward blind
    driveForward(avoidFwdL, avoidFwdR);
    if(now - stateStart >= avoidForward1Ms){
      stopMotors();
      enterState(AVOID_CURVE_RIGHT);
    }
  }

else if(state == AVOID_CURVE_RIGHT){ // obstacle: pivot LEFT
// LEFT pivot (second turn)
    obstaclePivotLeft(avoidTurnLeftL, avoidTurnLeftR);

  readLine();
  if(lineSeenNow()){
    enterState(RUN_FOLLOW);
    return;
  }

  if(now - stateStart >= avoidCurveRightMs){
    stopMotors();
    enterState(AVOID_FORWARD_2);
  }
}



  else if(state == AVOID_FORWARD_2){ // forward blind
    driveForward(avoidFwdL, avoidFwdR);
    if(now - stateStart >= avoidForward2Ms){
      stopMotors();
      enterState(AVOID_TURN_BACK);
    }
  }

else if(state == AVOID_TURN_BACK){ // turn back towards line

  // left motor forward, right motor backward -> controlled right turn
  motorSpeedAdjuster(motorL_fwd, motorL_rev, avoidTurnBackL, true,  invertLeft);
  motorSpeedAdjuster(motorR_fwd, motorR_rev, avoidTurnBackR, false, invertRight);

  readLine(); // update sensors
  if(lineSeenNow()){
    enterState(RUN_FOLLOW);
    return;
  }

  if(now - stateStart >= avoidTurnBackMs){
    stopMotors();
    enterState(AVOID_SEEK_LINE);
  }
}


  else if(state == AVOID_SEEK_LINE){ // forward until line seen or timeout
    driveForward(afterTurnL, afterTurnR);

    readLine();                 // update sensorValues

    if(lineSeenNow()){
      enterState(RUN_FOLLOW);
    } else if(now - stateStart >= avoidSeekMs){
      enterState(RUN_FOLLOW);
    }
  }

  // drop: reverse first then open while still reversing
  else if(state == DROP_BACKUP_BEFORE_OPEN){ // reverse before opening gripper
    driveBackward(backL, backR);
    if(now - dropReverseStart >= releaseAfterReverseMs){
      enterState(DROP_OPEN_WHILE_BACKING);
    }
  }

  else if(state == DROP_OPEN_WHILE_BACKING){ // open gripper while still reversing
    driveBackward(backL, backR);
    if(now - dropReverseStart >= (releaseAfterReverseMs + dropHoldMs)){
      enterState(DROP_BACKUP_AFTER_OPEN);
    }
  }

  else if(state == DROP_BACKUP_AFTER_OPEN){ // continue reversing after opening gripper
    driveBackward(backL, backR);
    if(now - dropReverseStart >= backupMs){
      stopMotors();
      enterState(DONE);
    }
  }

  else if(state == DONE){
    stopMotors();
  }
}
