/*

This is a simple game that utilizes a TM1638 "Led&Key" module.

Setup:
Connect VCC pin to 3.3v pin
Connect GND pin to GND pin
Connect STB pin to D7 pin
Connect CLK pin to D9 pin
Connect DIO pin to D8 pin

Connect arduino's RST pin to D4 pin. (you need to do this after uploading the sketch, otherwise there will be an error during uploading).

When turned on, press the S1 button to start the game.

The controls are:
press the S1 button to go up
press the S8 button to go down.

The goal is to dodge the squares and score as many points as you can. The squares' speed will increase after a certain amount of points.
Your score is displayed in binary during the game and shown after the "GAME OVER" message.

The game will restart itself if arduino's RST pin is connected to D4 pin. If it's not, you will need to either connect them or reset the arduino.

*/


#define UPPER_MINUS 11
#define LOWER_MINUS 10
#define CLEAR 12
#define UPPER_SQUARE 13
#define LOWER_SQUARE 14
#define PLAYER_AND_LOWER_SQUARE 15
#define PLAYER_AND_UPPER_SQUARE 16


const int strobe = 7;
const int clock = 9;
const int data = 8;
const int reset = 4;

char playerPosition;
char gameStatus;
char generatorStatus = 0;
char tick = 0;
char difficulty = 100;
char score = 0;
int binaryScore = 0;

char obstacles[2][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}
};

char gameOverMessage[] = {'G','A','M','E','O','V','E','R'};
char startMessage[] = {'S','T','A','R','T',' ','S',1};
char scoreMessage[] = {'S','C','O','R','E'};
char restartMessage[] = {'R','E','S','T','A','R','T', ' '};
char switchMessage[] = {'S','W','I','T','C','H',' ',' '};

void SendCommand(uint8_t value)
{
  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, value);
  digitalWrite(strobe, HIGH);
}

void Reset(){
  SendCommand(0x40);
  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, 0xc0);
  for (uint8_t i = 0; i < 16; i++){
    shiftOut(data, clock, LSBFIRST, 0x00);
  }
  digitalWrite(strobe, HIGH);
}

uint8_t ReadButtons(void)
{
  uint8_t buttons = 0;
  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, 0x42);
 
  pinMode(data, INPUT);
 
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t v = shiftIn(data, clock, LSBFIRST) << i;
    buttons |= v;
  }
 
  pinMode(data, OUTPUT);
  digitalWrite(strobe, HIGH);
  return buttons;
}

void SetLed(uint8_t value, uint8_t position)
{
  pinMode(data, OUTPUT);
 
  SendCommand(0x44);
  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, 0xC1 + (position << 1));
  shiftOut(data, clock, LSBFIRST, value);
  digitalWrite(strobe, HIGH);
}

void DisplayMessage(int num){
  //0 - empty message
  //1 - game over
  //2 - start s1
  //3 - score
  //4 - restart
  //5 - switch
  for (int i = 0; i < 8; i++){
    if (num == 0){
      SendChar(CLEAR, i);
    }
    if (num == 1){
      SendChar(gameOverMessage[i],i);
    }
    if (num == 2){
      SendChar(startMessage[i],i);
    }
    if (num == 3){
      if(i < 5) SendChar(scoreMessage[i],i);
      else{
        if (i == 5) SendChar(CLEAR, i);
        if (i == 6) SendChar(binaryScore /10, i);
        if (i == 7) SendChar(binaryScore % 10, i);
      }
    }
    if (num == 4){
      SendChar(restartMessage[i],i);
    }
    if (num == 5){
      SendChar(switchMessage[i],i);
    }
  }
}

uint8_t GetCharCode(int symbol){
  switch (symbol){

    case 1:
      return 0x06;
     break;

    case 2:
      return 0x5B;
      break;
    
    case 3:
      return 0x4F;
      break;

    case 4:
      return 0x66;
      break;

    case 5:
      return 0x6D;
      break;

    case 6:
      return 0x7D;
      break;

    case 7:
      return 0x07;
      break;

    case 8:
      return 0x7F;
      break;

    case 9:
      return 0x6F;
      break;

    case 0:
      return 0x3F;
      break;

    case '-':
      return 0x40;
      break;

    case 10:    //lower minus
      return 0x08;
      break;

    case 11:    //upper minus
      return 0x01;
      break;

    case 12:    //clear
      return 0x00;
      break;

    case 13:
      return 0x63;
      break;

    case 14:
      return 0x5C;
      break;

    case 15:
      return 0x5D;
      break;

    case 16:
      return 0x6B;
      break;

    case 'A':
      return 0x77;
      break;

    case 'B':
      return 0x7F;
      break;

    case 'C':
      return 0x39;
      break;

    case 'D':
      return 0x5E;
      break;

    case 'E':
      return 0x79;
      break;

    case 'F':
      return 0x71;
      break;

    case 'G':
      return 0x7D;
      break;

    case 'H':
      return 0x76;
      break;

    case 'I':
      return 0x06;
      break;

    case 'J':
      return 0x1E;
      break;

    case 'K':
      return 0x75;
      break;

    case 'L':
      return 0x38;
      break;

    case 'M':
      return 0x55;
      break;

    case 'N':
      return 0x37;
      break;
    
    case 'O':
      return 0x3F;
      break;

    case 'P':
      return 0x73;
      break;

    case 'Q':
      return 0x67;
      break;

    case 'R':
      return 0x50;
      break;

    case 'S':
      return 0x6D;
      break;

    case 'T':
      return 0x78;
      break;

    case 'U':
      return 0x1C;
      break;

    case 'V':
      return 0x2A;
      break;

    case 'W':
      return 0x6A;
      break;

    case 'X':
      return 0x14;
      break;

    case 'Y':
      return 0x6E;
      break;

    case 'Z':
      return 0x5B;
      break;

    case ' ':
      return 0x00;
      break;

    default:
      return 0x71;
      break;
  }
}

void SendChar(int symbol, int position){

  position %= 8;

  uint8_t adress = 0xc0;
  uint8_t symbolHex = GetCharCode(symbol);
  adress += position * 2;

  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, adress);
  shiftOut(data, clock, LSBFIRST, symbolHex);
  digitalWrite(strobe, HIGH);
}

void LedON(int number){
  uint8_t adress = 0xc1;
  adress += number * 2;

  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, adress);
  shiftOut(data, clock, LSBFIRST, 0x01);
  digitalWrite(strobe, HIGH);
}

void LedOFF(int number){
  uint8_t adress = 0xc1;
  adress += number * 2;

  digitalWrite(strobe, LOW);
  shiftOut(data, clock, LSBFIRST, adress);
  shiftOut(data, clock, LSBFIRST, 0x00);
  digitalWrite(strobe, HIGH);
}

void BinaryCounter(){
  score += 1;
  if (score == 20){
    binaryScore += 1;
    score = 0;
    
    if (binaryScore & 1)    LedON(7);
    else LedOFF(7);
    if (binaryScore & 2)    LedON(6);
    else LedOFF(6);
    if (binaryScore & 4)    LedON(5);
    else LedOFF(5);
    if (binaryScore & 8)    LedON(4);
    else LedOFF(4);
    if (binaryScore & 16)   LedON(3);
    else LedOFF(3);
    if (binaryScore & 32)   LedON(2);
    else LedOFF(2);
    if (binaryScore & 64)   LedON(1);
    else LedOFF(1);
    if (binaryScore & 128)  LedON(0);
    else LedOFF(0);
    
    if (binaryScore == 2) difficulty = 80;
    if (binaryScore == 4) difficulty = 60;
    if (binaryScore == 6) difficulty = 50;
    if (binaryScore == 8) difficulty = 40;
    if (binaryScore == 16) difficulty = 35;
    if (binaryScore == 18) difficulty = 30;
    if (binaryScore == 24) difficulty = 25;
    if (binaryScore == 32) difficulty = 20;
    if (binaryScore == 64) difficulty = 15;
  }
}

void ObstacleGenerate(){
  char randNum = random(2);
  if (randNum){
    obstacles[0][7] = 1;
    return;
  }
  else obstacles[1][7] = 1;
}

void DrawFrame(){
  for(int i = 7; i >= 0; i--){
   if (obstacles[0][i]){
        SendChar(UPPER_SQUARE, i);
       continue;
     }
   if (obstacles[1][i]){
       SendChar(LOWER_SQUARE, i);
       continue;
      }
  }
}

void DrawPlayer(){
  if (obstacles[0][0]){
    if (playerPosition == 1) SendChar(PLAYER_AND_UPPER_SQUARE, 0);
    else SendChar(UPPER_SQUARE, 0);
  }
  if (obstacles[1][0]){
    if (playerPosition == -1) SendChar(PLAYER_AND_LOWER_SQUARE, 0);
    else SendChar(LOWER_SQUARE, 0);
  }
  if (!obstacles[0][0] && !obstacles[1][0]){
    if (playerPosition == -1) SendChar(UPPER_MINUS, 0);
    if (playerPosition == 0) SendChar('-', 0);
    if (playerPosition == 1) SendChar(LOWER_MINUS, 0);
  }
}

void MoveObstacles(){
  for (int i = 0; i < 8; i++){
    SendChar(CLEAR, i);
      if (i < 7){
        obstacles[0][i] = obstacles[0][i+1];
        obstacles[1][i] = obstacles[1][i+1];
      }
      else{
        obstacles[0][i-1] = obstacles[0][i];
        obstacles[1][i-1] = obstacles[1][i];
        obstacles[0][i] = 0;
        obstacles[1][i] = 0;
      }
  }
}

void Generate(){
  if (generatorStatus % 3 == 0){
      ObstacleGenerate();
      generatorStatus %= 12;
    }
    generatorStatus++;
}

void StartGame(int num){
  if (num == 1) gameStatus = 1;
}

void GameOver(){
    delay(1000);
    DisplayMessage(1); // game over
    delay(2000);
    DisplayMessage(3); //score
    delay(3000);
    while(true){
      digitalWrite(reset, LOW);
      DisplayMessage(4);
      delay(500);
      DisplayMessage(5);
      delay(500);
      DisplayMessage(0);
      delay(500);      
    }
}

void MovePlayer(unsigned char button){
  if (button == 1){
    if (playerPosition == 1) playerPosition = 0;
    if (playerPosition == 0) playerPosition = -1;
  }
  if (button == 128){
    if (playerPosition == 0) playerPosition = 1;
    if (playerPosition == -1) playerPosition = 0;
  }
}

void CollisionCheck(){
  if (playerPosition == -1 && obstacles[0][0]) GameOver();
  if (playerPosition == 0 && (obstacles[0][0] || obstacles[1][0])) GameOver();
  if (playerPosition == 1 && obstacles[1][0]) GameOver();
}

void ObstacleReset(){
  for(int i = 0; i < 2; i++){
    for (int j = 0; j < 8; j++){
      obstacles[i][j] = 0;
    }
  }
}


void setup() {
  digitalWrite(reset, HIGH);

  pinMode(strobe, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(data, OUTPUT);
  pinMode(reset, OUTPUT);

  SendCommand(0x8f);
  Reset();

  Serial.begin(9600);
  Serial.println("bruh");
  randomSeed(analogRead(0));

  gameStatus = -1;

  playerPosition = 0;
}


void loop() {

  SendCommand(0x44);
  DisplayMessage(2); //start s1

  uint8_t buttons = ReadButtons();

  for(uint8_t position = 0; position < 8; position++)
    {
      uint8_t mask = 0x1 << position;

      StartGame(buttons);
    }

  while (gameStatus == 1){

    buttons = ReadButtons();
    
    for(uint8_t position = 0; position < 8; position++)
    {
      uint8_t mask = 0x1 << position;

      MovePlayer(buttons);
      DrawPlayer();
      CollisionCheck();
    }


    tick++;
    Serial.println((int)tick);
    if (tick / difficulty == 1){
      BinaryCounter();

      MoveObstacles();

      Generate();
  
      DrawFrame();

      tick = 0;
    }
  }
}
