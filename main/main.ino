SemaphoreHandle_t AccessSeaMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t SerialOutMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t AccessMyShipHealthMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t AccessOtherShipHealthMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t PrintSeaMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t inputNotReadSemaphore = xSemaphoreCreateBinary();
SemaphoreHandle_t CannonArrayMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t myShipMovedSem = xSemaphoreCreateBinary();
SemaphoreHandle_t myShipMovedBallsSem = xSemaphoreCreateBinary();
SemaphoreHandle_t NumBallsMutex = xSemaphoreCreateMutex();

const int xSize = 100;
const int ySize = 100;
int screenSize = 25;//screen size is actually twice as big

const int led_pin = LED_BUILTIN;
int critHealth = 3;
char* input;
int inputLength = 11;

char sea [xSize*ySize];
int ids [xSize*ySize];

struct Ship {
  int xCoor;
  int yCoor;
  int angleH;
  int angleV;
  int shipSizeL;
  int shipSizeW;
  int speed;
  int maxSpeed;
  int health;
  char selfSymbol;
  int id;
  int currPortCan;
  int currStarCan;
};

struct crashedAndType{
  bool crashed;
  bool crashedMyShip;
  Ship& selfShip;
  Ship* crashedOtherShip;
};

const int numCoves = 50;
int covesX[numCoves];
int covesY[numCoves];
struct Ship otherShips[numCoves];
const int maxNumBalls = 20;
int numOfBalls = 0;
int numOfBallsCreated = 0;
struct Ship cannonBallShips[maxNumBalls];

char char_SteadySea = '~';
char char_Ship = '=';
char char_OtherShip = '#';
char char_OutBounds = '*';
char char_Land = '^';
char char_DeadShip = 'x';
char char_DeadMyShip = '.';
char char_PrintBoarder = ':';
char char_CannonBall = '0';

Ship nullShip{-1, -1, -3, -3, -1, -1, -1, -1, -1, '\n', -1, -1, -1};
Ship MyShip{(xSize / 2), (ySize / 2), 0, -1, 4, 2, 0, 3, 8, char_Ship, 1, 0, 0};
int steadySeaID = 0;
int myShipID = 1;
//Then increment 1 for all cove ids
//Then increment 1 for all other ships ids
//Then increment 1 for all cannon ball ids (the ids are not reused after ball death)

void incMyShipHealth(){
  xSemaphoreTake(AccessMyShipHealthMutex, portMAX_DELAY);
  MyShip.health +=1;
  xSemaphoreGive(AccessMyShipHealthMutex);

  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.print("Your Health: ");
  Serial.println(MyShip.health);
  xSemaphoreGive(SerialOutMutex);
}
void decMyShipHealth(){
  xSemaphoreTake(AccessMyShipHealthMutex, portMAX_DELAY);
  MyShip.health -=1;
  xSemaphoreGive(AccessMyShipHealthMutex);

  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.print("Your Health: ");
  Serial.println(MyShip.health);
  xSemaphoreGive(SerialOutMutex);
}
int getMyShipHealth(){
  int temp;
  xSemaphoreTake(AccessMyShipHealthMutex, portMAX_DELAY);
  temp = MyShip.health;
  xSemaphoreGive(AccessMyShipHealthMutex);
  return temp;
}

void incOtherShipHealth(Ship& otherShip){
  xSemaphoreTake(AccessOtherShipHealthMutex, portMAX_DELAY);
  otherShip.health +=1;
  xSemaphoreGive(AccessOtherShipHealthMutex);
}
void decOtherShipHealth(Ship& otherShip){
  xSemaphoreTake(AccessOtherShipHealthMutex, portMAX_DELAY);
  otherShip.health -=1;
  xSemaphoreGive(AccessOtherShipHealthMutex);
}
int getOtherShipHealth(Ship& otherShip){
  int temp;
  xSemaphoreTake(AccessOtherShipHealthMutex, portMAX_DELAY);
  temp = otherShip.health;
  xSemaphoreGive(AccessOtherShipHealthMutex);
  return temp;
}

char getSea(int x, int y){
  char temp;
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  temp = sea[x+y*xSize];
  xSemaphoreGive(AccessSeaMutex);
  return temp;
}
char getId(int x, int y){
  int temp;
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  temp = ids[x+y*xSize];
  xSemaphoreGive(AccessSeaMutex);
  return temp;
}
void setSea(int x, int y, char symbol, int id){
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  sea[x+y*xSize] = symbol;
  ids[x+y*xSize] = id;
  xSemaphoreGive(AccessSeaMutex);
}

void makeSea() {
  for (int row{0}; row < xSize; ++row) {
    for (int col{0}; col < ySize; ++col) {
      setSea(row,col, char_SteadySea, steadySeaID);
    }
  }
}

void makeCoves() {
  int tempCoor = 0;
  for (int i{0}; i< numCoves; ++i) {
    tempCoor = random(0, xSize);
    if (tempCoor > MyShip.xCoor +5 || tempCoor < MyShip.xCoor-5)
    covesX[i] = tempCoor;
    else covesX[i] = 0;//Not a great solution, but it works
  }
  for (int i{0}; i< numCoves; ++i) {
    tempCoor = random(0, xSize);
    if (tempCoor > MyShip.yCoor +5 || tempCoor < MyShip.yCoor-5)
    covesY[i] = tempCoor;
    else covesY[i] = 0;//Not a great solution, but it works
  }
}
void makeOtherShips() {
  int tempCoorX = 0;
  int tempCoorY = 0;
  for (int i{0}; i< numCoves; ++i) {
    tempCoorX = random(0, xSize);
    if (tempCoorX < MyShip.xCoor +5 && tempCoorX > MyShip.xCoor-5)
    {tempCoorX = 0;}//Not a great solution, but it works
    
    tempCoorY = random(0, xSize);
    if (tempCoorY < MyShip.yCoor +5 && tempCoorY > MyShip.yCoor-5)
    {tempCoorY = 0;}//Not a great solution, but it works
    otherShips[i] = Ship{tempCoorX, tempCoorY, 1, 0, 3, 1, 0, 1, 1, char_OtherShip, 1+numCoves+i+1, 0, 0};
  }  
}

void printSea() {
  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  xSemaphoreTake(PrintSeaMutex, portMAX_DELAY);
  for (int i = 0; i < 2*screenSize+2; ++i) {Serial.print(char_PrintBoarder);}
  Serial.println();
  for (int col = MyShip.yCoor - screenSize; col < MyShip.yCoor + screenSize; ++col) {
    Serial.print(char_PrintBoarder);
    for (int row = MyShip.xCoor - screenSize; row < MyShip.xCoor + screenSize; ++row) {
      if (row < xSize && row > 0 && col < ySize && col > 0) {
        Serial.print(getSea(row,col));
      } else {
        Serial.print(char_OutBounds);
      }
    }
    Serial.print(char_PrintBoarder);
    Serial.println();
  }
  for (int i = 0; i < 2*screenSize+2; ++i) {Serial.print(char_PrintBoarder);}
  Serial.println();
  xSemaphoreGive(PrintSeaMutex);
  xSemaphoreGive(SerialOutMutex);
}
void drawCoves(){
  
  for (int i = 0; i < numCoves; ++i) {
      setSea(covesX[i],covesY[i], char_Land, 1+i+1);
      if (covesX[i]+1 < xSize){setSea(covesX[i]+1,covesY[i], char_Land, 1+i+1);}
      if (covesX[i]-1 > 0){setSea(covesX[i]-1,covesY[i], char_Land, 1+i+1);}
      if (covesY[i]+1 < ySize){setSea(covesX[i],covesY[i]+1, char_Land, 1+i+1);}
  }
}

template<typename Fn, typename T>
void accessShip(Ship theShip, Fn toRunFunc, T& stateHolder) {
  int offsetX = 0;
  int offsetY = 0;
  for (int i = 0; i < theShip.shipSizeW; ++i) {
    offsetX = 0;
    offsetY = 0;
    offsetX += i*theShip.angleV*-1;
    offsetY += i*theShip.angleH;
    if (i!=0&&theShip.angleV<0 && theShip.angleH>0) {offsetY+=i*theShip.angleV;}
    if (i!=0&&theShip.angleV<0 && theShip.angleH<0) {offsetY+=i*theShip.angleV*-1;}
    if (i!=0&&theShip.angleV>0 && theShip.angleH>0) {offsetY+=i*theShip.angleV*-1;}
    if (i!=0&&theShip.angleV>0 && theShip.angleH<0) {offsetY+=i*theShip.angleV;}
    for (int j = 0; j < theShip.shipSizeL; ++j) {
      toRunFunc(theShip.xCoor+offsetX,theShip.yCoor+offsetY, stateHolder);
      offsetX +=theShip.angleH;
      offsetY +=theShip.angleV;
    }
  }
  //return stateHolder;
}

void drawFrame(Ship theShip, char symbol = char_Ship) {
  int throwAway = 0;
  accessShip(theShip, 
    [symbol, theShip](int X, int Y, int&){
      setSea(X,Y, symbol,theShip.id);
    }
    , throwAway);
}

void initDrawOtherShips(){
  for (int i{0}; i< numCoves; ++i) {
    drawFrame(otherShips[i], char_OtherShip);
  } 
}
void cleanFrame(Ship theShip) {
  int throwAway = 0;
  accessShip(theShip, 
    [](int X, int Y, int&){
      setSea(X,Y, char_SteadySea,steadySeaID);
    }
    , throwAway);
}
void getCommand(void *pvParameters) {
  static bool makeNewInput = true;
  char c;
  uint8_t index = 0;

  if (makeNewInput && uxSemaphoreGetCount(inputNotReadSemaphore)==1) {
    input = (char*) pvPortMalloc(inputLength*sizeof(char));
    memset(input, 0, inputLength);
    makeNewInput=false;
  }
  if (uxSemaphoreGetCount(inputNotReadSemaphore)==1){
    while(Serial.available() > 0) {
      
      c = Serial.read();
  
      if (c == '\n'|| index >= inputLength) {
        input[index] = '\0';
        xSemaphoreTake(inputNotReadSemaphore, (TickType_t) 10);
        makeNewInput = true;
      }
      else if (index < inputLength - 1) {
        input[index] = c;
        index++;
      }
    }
  }
  vTaskDelete(NULL);
}

//SemaphoreHandle_t NumBallCount = xSemaphoreCreateCounting((UBaseType_t) maxNumBalls, (UBaseType_t 0);
void spawnCannonBall(Ship& theShip, bool isLeft){
  int portNum = isLeft ? theShip.currPortCan++ : theShip.currStarCan++;
  if (theShip.currPortCan>=theShip.shipSizeL){theShip.currPortCan = 0;}
  if (theShip.currStarCan>=theShip.shipSizeL){theShip.currStarCan = 0;}
  if (numOfBalls < maxNumBalls){
    int newXCoor = theShip.xCoor;
    int newYCoor = theShip.yCoor;
    newXCoor = newXCoor +portNum*theShip.angleH;
    newYCoor = newYCoor +portNum*theShip.angleV;
    int newHAngle = 0;
    int newVAngle = 0;
    if (isLeft) {
      if (theShip.angleH== 0) newXCoor += theShip.angleV;
      else if (theShip.angleV== 0) newYCoor += theShip.angleH*-1;
      else if (theShip.angleV < 0) newXCoor += -1;
      else if (theShip.angleV > 0) newXCoor += 1;
      newHAngle = theShip.angleV;
      newVAngle = theShip.angleH*-1;
    } else {
      if (theShip.angleH== 0) newXCoor += -1*theShip.angleV*(theShip.shipSizeW);
      else if (theShip.angleV== 0) newYCoor += theShip.angleH*(theShip.shipSizeW);
      else if (theShip.angleV < 0) newXCoor += 1*(theShip.shipSizeW);
      else if (theShip.angleV > 0) newXCoor += -1*(theShip.shipSizeW);
      newHAngle = theShip.angleV*-1;
      newVAngle = theShip.angleH;
    }
    
    xSemaphoreTake(CannonArrayMutex, portMAX_DELAY);
    int openSlot = -1;//If the function got past the inital if statment, there should be a open slot
    for (int i = 0; i < maxNumBalls; ++i){ if (cannonBallShips[i].id == nullShip.id) {openSlot=i;} }
        
    cannonBallShips[openSlot]=Ship{newXCoor, newYCoor, newHAngle, newVAngle, 1, 1, 2, 4, 1, char_CannonBall,  1+numCoves+numCoves+numOfBallsCreated+1, 0, 0};
    xSemaphoreTake(NumBallsMutex, portMAX_DELAY);
    numOfBalls +=1;
    xSemaphoreGive(NumBallsMutex);
    //xSemaphoreGive(NumBallCount);
    numOfBallsCreated+=1;
    xSemaphoreGive(CannonArrayMutex);
    
  } else {
    xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
    Serial.println("Misfire");
    xSemaphoreGive(SerialOutMutex);
    
  }
  
}
void updateAngle(Ship& theShip, int newH, int newV){
  Ship tempShip = theShip;
  crashedAndType crashed{false, false, tempShip, nullptr};

  
  tempShip.angleH = newH;
  tempShip.angleV = newV;
  accessShip(tempShip, checkCrashedHelper, crashed);
  
  if (crashed.crashed){
    if (&theShip==&MyShip){decMyShipHealth();}else{decOtherShipHealth(theShip);}
    if (crashed.crashedMyShip) {
      decMyShipHealth();
      xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
      Serial.println("You got hit");
      xSemaphoreGive(SerialOutMutex);
    }
    if (crashed.crashedOtherShip != nullptr){
      decOtherShipHealth(*crashed.crashedOtherShip);
    }
  }else {
    theShip.angleH = newH;
    theShip.angleV = newV;
  }
}

void processCommand(char* input, Ship& theShip){
  int tempH = theShip.angleH;
  int tempV = theShip.angleV;
  if (strcmp(input, "down") == 0){
    if (theShip.angleV < 1 && theShip.angleH!=0) { tempV += 1;}// Serial.println("down");}
  } else if (strcmp(input, "up") == 0) {
    if (theShip.angleV > -1&& theShip.angleH!=0) { tempV -= 1;}// Serial.println("up");}
  } else if (strcmp(input, "right") == 0) {
    if (theShip.angleH < 1&& theShip.angleV!=0) { tempH += 1;}// Serial.println("right");}
  } else if (strcmp(input, "left") == 0) {
    if (theShip.angleH > -1&& theShip.angleV!=0) { tempH -= 1;}// Serial.println("left");}
  } else if (strcmp(input, "faster") == 0) {
    if (theShip.speed < theShip.maxSpeed) { theShip.speed+=1;}// Serial.println("faster");}
  } else if (strcmp(input, "slower") == 0) {
    if (theShip.speed>0) { theShip.speed -=1;}// Serial.println("slower");}
  } else if (strcmp(input, "port") == 0) {
    spawnCannonBall(theShip, true);
  }  else if (strcmp(input, "star") == 0) {
    spawnCannonBall(theShip, false);
  }  //else Serial.println("none");

  updateAngle(theShip, tempH, tempV);
}


void checkCrashedHelper(int newX, int newY, crashedAndType& crashed){
  if (newX >= xSize || newX <= 0 || newY >= ySize || newY <= 0) { crashed.crashed = true;}
  //Use if you don't want other ships to collide with each other
  //if (getSea(newX,newY) != char_SteadySea&&getSea(newX,newY) != crashed.selfShip.selfSymbol) 
  //Prevents other ships from going through each other
  if (getSea(newX,newY) != char_SteadySea&&getId(newX,newY) != crashed.selfShip.id) 
  {
    crashed.crashed = true;
    if (getId(newX,newY) > 1+numCoves) {
      crashed.crashedOtherShip = &otherShips[getId(newX,newY)-numCoves-1-1];
    }
    if (getSea(newX,newY) == char_Ship) {crashed.crashedMyShip = true;}
  }
}

void updateShips(Ship& theShip){
  int tempX = theShip.xCoor;
  int tempY = theShip.yCoor;
  
  for (int s = 0; s < theShip.speed; ++s){
    tempX += theShip.angleH;
    tempY += theShip.angleV;

    Ship tempShip = theShip;
    crashedAndType crashed{false, false, tempShip, nullptr};

    
    tempShip.xCoor = tempX;
    tempShip.yCoor = tempY;
    accessShip(tempShip, checkCrashedHelper, crashed);
    
    if (crashed.crashed){
      if (&theShip==&MyShip){decMyShipHealth();}else{decOtherShipHealth(theShip);}
      if (crashed.crashedMyShip) {
        decMyShipHealth();
        xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
        Serial.println("You got hit");
        xSemaphoreGive(SerialOutMutex);
      }
      if (crashed.crashedOtherShip != nullptr){
        decOtherShipHealth(*crashed.crashedOtherShip);
      }
    }else {
      theShip.xCoor = tempX;
      theShip.yCoor = tempY;
    }
  }
}
int AIVisionDis = 20;
void otherShipAI(Ship& theShip){
  if (theShip.health>0){
    Ship copyShip = theShip;
    if (abs(MyShip.xCoor-theShip.xCoor)<AIVisionDis&&abs(MyShip.yCoor-theShip.yCoor)<AIVisionDis){
      if (MyShip.xCoor-theShip.xCoor<0 && theShip.angleH!=-1&& theShip.angleV!=0){
        processCommand("left", theShip);
      } else if (MyShip.xCoor-theShip.xCoor>0 && theShip.angleH!=1&& theShip.angleV!=0){
        processCommand("right", theShip);
      } else if (MyShip.yCoor-theShip.yCoor<0 && theShip.angleV!=-1&& theShip.angleH!=0){
        processCommand("up", theShip);
      } else if (MyShip.yCoor-theShip.yCoor>0 && theShip.angleV!=1&& theShip.angleH!=0){
        processCommand("down", theShip);
      } else {processCommand("faster", theShip);}
    } else{processCommand("slower", theShip);}
    xSemaphoreTake(PrintSeaMutex, portMAX_DELAY);
    cleanFrame(copyShip);
    updateShips(theShip);
    drawFrame(theShip, char_OtherShip);
    xSemaphoreGive(PrintSeaMutex);
  } else drawFrame(theShip, char_DeadShip);
}

void runOtherShipAIs(void *pvParameters){
  while(1){
    xSemaphoreTake(myShipMovedSem, portMAX_DELAY);
      for (int i = 0; i < numCoves; ++i){
        otherShipAI(otherShips[i]);
      }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void restartOnDelay(void* pvParameters){
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.println("Restarting Game");
  xSemaphoreGive(SerialOutMutex);
  
  ESP.restart();
}
void makeGetCommandTask(TimerHandle_t xTimer){
  xTaskCreatePinnedToCore(getCommand, "get_command", 1024, NULL, 4, NULL, tskNO_AFFINITY);
}
void blinkHealthLED(void* pvParameters){
  while(1){
    if(getMyShipHealth()>critHealth) {
      digitalWrite(led_pin, HIGH);
      vTaskDelay((getMyShipHealth()*200) / portTICK_PERIOD_MS);
      digitalWrite(led_pin, LOW);
      vTaskDelay((getMyShipHealth()*500) / portTICK_PERIOD_MS);
    } else if (getMyShipHealth()<=critHealth&&getMyShipHealth()>0) {
      xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
      Serial.println("Health Low!");
      xSemaphoreGive(SerialOutMutex);
      digitalWrite(led_pin, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(led_pin, LOW);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }
}

void runCannonBalls(void *pvParameters){
  while(1){
    xSemaphoreTake(myShipMovedBallsSem, portMAX_DELAY);
    if (numOfBalls>0) {
      for (int i = 0; i < maxNumBalls; ++i){
        xSemaphoreTake(CannonArrayMutex, portMAX_DELAY);
        if (cannonBallShips[i].id != nullShip.id){
          if(cannonBallShips[i].health>0){
            xSemaphoreTake(PrintSeaMutex, portMAX_DELAY);
            cleanFrame(cannonBallShips[i]);
            updateShips(cannonBallShips[i]);
            drawFrame(cannonBallShips[i], char_CannonBall);
            xSemaphoreGive(PrintSeaMutex);
          } else {
            drawFrame(cannonBallShips[i], char_SteadySea);
            xSemaphoreTake(NumBallsMutex, portMAX_DELAY);
            numOfBalls-=1;
            xSemaphoreGive(NumBallsMutex);
            cannonBallShips[i] = nullShip;
          }
        }
        xSemaphoreGive(CannonArrayMutex);
      }
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(250000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  pinMode(led_pin, OUTPUT);
  xSemaphoreGive(inputNotReadSemaphore);
  xSemaphoreGive(AccessSeaMutex);
  xSemaphoreGive(AccessMyShipHealthMutex);
  xSemaphoreGive(SerialOutMutex);
  for (int i = 0; i < maxNumBalls; ++i){ cannonBallShips[i] = nullShip;}
  
  makeSea();
  makeCoves();
  drawCoves();
  makeOtherShips();
  initDrawOtherShips();
  drawFrame(MyShip);

  Serial.print('<');
  for (int i = 0; i < (int)(screenSize*2+2)*1.3; ++i){ Serial.print('=');}
  Serial.println('>');
  
  printSea();
  
  Serial.println("This is a game where you control a ship and dodge islands and other ships.");
  Serial.println("Use \'up\' to turn up, \'down\' to turn down, \'left\' to turn left, \'right\' to turn right.");
  Serial.println("Use \'faster\' to speed up, and \'slower\' to slow down.");
  Serial.println("Use \'port\' to fire left, and \'star\' to fire right.");
  Serial.println("Ready...");
  
  TimerHandle_t inputTimer = xTimerCreate("make_get_command_task", 500 / portTICK_PERIOD_MS, pdTRUE, (void *)0, makeGetCommandTask);
  if (inputTimer == NULL) {
    ESP.restart();
  } else {
    xTimerStart(inputTimer, portMAX_DELAY);
  }
  xTaskCreatePinnedToCore(runOtherShipAIs, "run_other_ship_AIs", 5024, NULL, 1, NULL, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(blinkHealthLED, "blink_health_lED", 1024, NULL, 2, NULL, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(runCannonBalls, "run_cannon_balls", 5024, NULL, 2, NULL, tskNO_AFFINITY);
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  Serial.println("Start!");
  vTaskDelay(500 / portTICK_PERIOD_MS);

}



void loop() {
  Serial.print('<');
  for (int i = 0; i < (int)(screenSize*2+2)*1.3; ++i){ Serial.print('=');}
  Serial.println('>');
  if (getMyShipHealth()>0){
    Ship tempShip = MyShip;
    if (uxSemaphoreGetCount(inputNotReadSemaphore)==0){
      processCommand(input, MyShip); 
      vPortFree((void*)input);
      xSemaphoreGive(inputNotReadSemaphore);
    }
    cleanFrame(tempShip);
    updateShips(MyShip);
    drawFrame(MyShip);
  }
  
  
  xSemaphoreGive(myShipMovedSem);
  xSemaphoreGive(myShipMovedBallsSem);
  printSea();
  if (getMyShipHealth()<=0){
    
    xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
    Serial.println("The Ship died!");
    xSemaphoreGive(SerialOutMutex);
    drawFrame(MyShip, char_DeadMyShip);
    xTaskCreatePinnedToCore(restartOnDelay, "restart_on_delay", 1024, NULL, 1, NULL, tskNO_AFFINITY);
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
}
