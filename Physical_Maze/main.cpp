#include <Arduino.h>
#define gripper 11

#define LINE_NONE 0
#define LINE_LEFT 1 
#define LINE_RIGHT 2
#define LINE_CENTER 3
#define LINE_T 4
#define LINE_END 5

#define dazeyleftWheel 0b00100001
#define dazeyrightWheel 0b00100010
#define dazeyGripper 0b00100011
#define dazeyLine 0b00100100
#define dazeySonar 0b00100101

//
void DanyloDone(void);
void gripper_Close(void);
void Override(void);
//Motor Number, Speed is a percentage between 0 and 1, forward yes no
void motorSpeedAdjuster (int, float, bool);
//stop
void stop (void);
//left, right, go
void left(void);
void right(void);
void forward(void);
//mazercise
void mazercise(void);
void distanceRead(void);
//timer
void howLong(void);
void howLong2(void);
void howLong3(void);
//line
int readLine(void);
void handleLine(int);
//motor 1 is left, motor 2 is right

const int motorSpeed1 = 10;
const int motorSpeed2 = 6;
const int motorSpeedReverse1 = 9;
const int motorSpeedReverse2 = 5;
const int motor1 = 9;
const int motor2 = 5;
unsigned long timer;
unsigned long timer2;
unsigned long timer3;

int distance = 0;

const int trigger = 4;
const int echo = 7;

const int sensorCount = 8;
const int sensorPins[sensorCount] = {A0, A1, A2, A3, A4, A5, 3, 11};
int sensorValues[sensorCount];

const int BLACK = 900;
// timings (ms)
unsigned long startBlindMs        = 1000;
unsigned long grabHoldMs          = 350;

//MAZE STUFF
bool Object = false;
//North East South West start?
void protomaze(void);
// Maze[leftCell][forwardCell][direction]
// int Maze[7][7][4]{
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
//   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
// };
// //Is it trapped cause it missed a T?
// bool motherHelp = false;
//direction 0 - N 1 - E 2 - S 3 - W
int direction = 1;
int TempDir = 0;
// int forwardCell = 6;
// int leftCell = 5;
//COMMS
void iHaveAMouthandIMustScream(void);
bool iGo = false;
bool iGoon = false;
/*
2 bytes, 1 header - what the data is about
1 trailer - raw data - number

0000 0001 - left wheel speed
0000 0010 - right wheel speed
0000 0011 - Gripper
0000 0100 - line sensors

1000 0000 - POS - In progress
0100 0000 - Wall-E - In progress
0010 0000 - Dazey - In progress

1000 1111 - POS - finished

example packet
0010 0010 - Daisy Right Wheel speed
0110 0100 - Right wheel at full speed

start packet 
1111 1111
1111 1111

ack
1110 1110
1111 1111
*/

void setup() {
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(3), Override, RISING);

  pinMode(motorSpeed1, OUTPUT);
  pinMode(motorSpeedReverse1, OUTPUT);
  pinMode(motorSpeed2, OUTPUT);
  pinMode(motorSpeedReverse2, OUTPUT);

  stop();

  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(gripper, OUTPUT);

  for(int i=0; i<sensorCount; i++) 
    pinMode(sensorPins[i], INPUT);
  distanceRead();

  howLong();
  howLong2();
  howLong3();

}

void DanyloDone(void){
  while(Serial.available()){
    byte YIPPEE = Serial.read();
      if(YIPPEE == 0b01001111){
        iGo = true;
      }
  }
}


//37 ms + 20ms stop for a perfect 90
void loop() {
  if(!iGoon){
    //HOLD STILL but only once
    while(!iGo){
      DanyloDone();
    }
    forward();
    howLong();
    while(millis() <= timer + startBlindMs){}
    stop();
    howLong();
    while(millis() <= timer + grabHoldMs){
      gripper_Close();
    }
    left();
    delay(425);
    stop();
    delay(20);
    forward();
    howLong();
    while(millis() <= timer + 300){}
    while(readLine() != LINE_END && readLine() != LINE_NONE){
      handleLine(readLine());
    }
    iGoon = true;
  }
  
  else{
    if(readLine() != LINE_END && readLine() != LINE_NONE){
      handleLine(readLine());
    }
    else{
      protomaze();
    }
    //COMMS
    if (millis() >= timer2 + 500){
      distanceRead();
      iHaveAMouthandIMustScream();
      gripper_Close();
        howLong2();
    }
    ///////
  }
  
}
//863 is 30 cm
void protomaze(void){
  if(readLine() != LINE_END && readLine() != LINE_NONE){
    handleLine(readLine());
  }
  

  if (distance == 0 || distance > 15){
    forward();
    howLong();
    while(millis() < timer + 863){}
    TempDir = direction;
  }
  else if(distance < 15 && direction == TempDir){
    right();
    delay(425 * 2 + 20);
  }
  else if(distance < 15 && direction != ((TempDir - 2 < 0) ? TempDir - 2 : TempDir + 1)){
    left();
    delay(425);
  }
  else{
    left();
    delay(425 * 2 + 20);
  }
  stop();
  howLong();
  while(millis() < timer + 350){
    if(readLine() != LINE_END && readLine() != LINE_NONE){
      handleLine(readLine());
    }
  }
}


// void mazercise(void){
//   if (millis() >= timer + 500){
//     distanceRead();
//     howLong();
//   }
//   if(distance == 0 || distance > 15){
//     howLong();
//     forward();
//     while(millis() <= timer + 300){}
//   }
  
//   else if(distance <= 15){
//     stop();
//     if(Maze[leftCell][forwardCell][direction] == 1){
//       stop();
//       while(1==1){
//         digitalWrite(gripper, HIGH);
//         delay(1);
//         digitalWrite(gripper, LOW);
//       }
//     } 
//     else{
//       Maze[leftCell][forwardCell][direction] = 1;
//       howLong();
//       right();
//       while(millis() <= timer + 300){}
//       mazercise();
//     }
//   }
// }

void distanceRead(void){
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  distance = (.034 * pulseIn(echo, HIGH, 5900))/2;
}



//Stop the BOT
void stop (void){
  analogWrite(motorSpeed1, 0);
  analogWrite(motorSpeedReverse1, 0);
  analogWrite(motorSpeed2, 0);
  analogWrite(motorSpeedReverse2, 0);
  
}

//Left
void left(void){
  motorSpeedAdjuster(motor1, .95, false);
  motorSpeedAdjuster(motor2, 1, true);
  direction--;
  if(direction < 0)
    direction = 3;
  
}

//right
void right(void){
  motorSpeedAdjuster(motor1, .95, true);
  motorSpeedAdjuster(motor2, 1, false);
  direction++;
  if(direction > 3)
    direction = 0;
  
}

//forward
void forward(void){
  motorSpeedAdjuster(motor1, .95, true);
  motorSpeedAdjuster(motor2, 1, true);
  // if(direction == 0){
  //   forwardCell--;
  // }
  // else if(direction == 1){
  //   leftCell--;
  // }
  // else if(direction == 2){
  //   forwardCell++;
  // }
  // else if(direction == 3){
  //   leftCell++;
  // }
  
}



//COMMS
void iHaveAMouthandIMustScream(void){
  Serial.write(dazeyLine);
  Serial.write(readLine());
  Serial.write(dazeyGripper);
  Serial.write(0b11111111);
  Serial.write(dazeySonar);
  Serial.write(distance);
}



int readLine(void) {
  for(int i = 0; i < sensorCount; i++)
    sensorValues[i] = analogRead(sensorPins[i]);

  bool right = sensorValues[0] > BLACK || sensorValues[1] > BLACK || sensorValues[2] > BLACK;
  bool center = sensorValues[3] > BLACK || sensorValues[4] > BLACK;
  bool left = sensorValues[5] > BLACK || sensorValues[6] > BLACK || sensorValues[7] > BLACK;

  if(left && center && right) 
    return LINE_T;
  else if(!left && !center && !right)
    return LINE_END;
  else if(center) 
    return LINE_CENTER;
  else if(left) 
    return LINE_LEFT;
  else if(right) 
    return LINE_RIGHT;
  else
    return LINE_NONE;
}

void handleLine(int line) {
  for(int i=0; i<sensorCount; i++)
    sensorValues[i] = analogRead(sensorPins[i]);

  if(sensorValues[0] > BLACK && sensorValues[1] > BLACK && sensorValues[2] > BLACK) {
    motorSpeedAdjuster(motor1, .95, true);
    motorSpeedAdjuster(motor2, 1, false);
    
  }
  else if(sensorValues[5] > BLACK && sensorValues[6] > BLACK && sensorValues[7] > BLACK) {
    motorSpeedAdjuster(motor1, .95, false);
    motorSpeedAdjuster(motor2, 1, true);
    
  }

  if(line == LINE_CENTER){
    motorSpeedAdjuster(motor1, .95, true);
    motorSpeedAdjuster(motor2, 1, true);
    
  }
  else if (line == LINE_LEFT)
  {
    motorSpeedAdjuster(motor1, .6, false);
    motorSpeedAdjuster(motor2, 1, true);
    
  }
  else if (line == LINE_RIGHT){
    motorSpeedAdjuster(motor1, .95, true);
    motorSpeedAdjuster(motor2, .6, false);
    
  }
  else if (line == LINE_T){
    motorSpeedAdjuster(motor1, .95, true);
    motorSpeedAdjuster(motor2, 1, true);
    
  }
  else if (line == LINE_END){
    protomaze();
  }
  else if (line == LINE_NONE){
    protomaze();
  }
}



void howLong(void){
  timer = millis();
}

void howLong2(void){
  timer2 = millis();
}

void howLong3(void){
  timer3 = millis();
}


//individual motor control
void motorSpeedAdjuster (int Number, float Speed, bool Forward) {
  int AnalogSpeed = 255 * Speed;
  int ServerSpeed = Speed * 100;
  
  if(millis() >= timer3 + 500 && Number == 9){
    Serial.write(dazeyleftWheel);
    if(Forward){
      Serial.write(ServerSpeed);
    }
    if(!Forward){
      Serial.write(ServerSpeed*-1);
    }
  }
  else if(millis() >= timer3 + 500 && Number == 5){
    Serial.write(dazeyrightWheel);
    if(Forward){
      Serial.write(ServerSpeed);
    }
    if(!Forward){
      Serial.write(ServerSpeed*-1);
    }
      howLong3();
  }

  if(Speed < .6){
    for (float sauce = 1; sauce != Speed; sauce -= .1)
    {
      AnalogSpeed = 255 * sauce;
      if(Forward){
        analogWrite(Number, 0);
        analogWrite(Number + 1, AnalogSpeed);
      }

      else{
        analogWrite(Number + 1, 0);
        analogWrite(Number, AnalogSpeed);
      }
      howLong();
      while (millis() <= timer + 5){}
    }
  }

  if(Forward){
    analogWrite(Number, 0);
    analogWrite(Number + 1, AnalogSpeed);
  }

  else{
    analogWrite(Number + 1, 0);
    analogWrite(Number, AnalogSpeed);
  }
}

void gripper_Close(void){
  digitalWrite(gripper, HIGH);
  delayMicroseconds(1020);
  digitalWrite(gripper, LOW);
}

void Override(void){
  iGo = true;
}
