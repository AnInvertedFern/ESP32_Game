const int xSize = 100;
const int ySize = 100;
int screenSize = 25;//screen size is actually twice as big

SemaphoreHandle_t AccessSeaMutex = xSemaphoreCreateMutex();
char sea [xSize*ySize];
//SemaphoreHandle_t AccessIdsMutex = xSemaphoreCreateMutex();
//Use AccessSeaMutex for both the sea matrix and ids matrix
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

int inputLength = 11;
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
//char char_ShipHead = '@';
char char_DeadShip = 'x';
char char_DeadMyShip = '.';
char char_PrintBoarder = ':';
char char_CannonBall = '0';

Ship nullShip{-1, -1, -3, -3, -1, -1, -1, -1, -1, '\n', -1, -1, -1};

Ship MyShip{(xSize / 2), (ySize / 2), 0, -1, 4, 2, 0, 3, 20, char_Ship, 1, 0, 0};
int steadySeaID = 0;
int myShipID = 1;
//Then increment 1 for all cove ids
//Then increment 1 for all other ships ids
//Then increment 1 for all cannon ball ids (the ids are not reused after ball death)

SemaphoreHandle_t SerialOutMutex = xSemaphoreCreateMutex();

SemaphoreHandle_t AccessMyShipHealthMutex = xSemaphoreCreateMutex();
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

SemaphoreHandle_t AccessOtherShipHealthMutex = xSemaphoreCreateMutex();
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
  for (int i = 0; i < 2*screenSize+2; ++i) {Serial.print(char_PrintBoarder);}
  Serial.println();
  for (int col = MyShip.yCoor - screenSize; col < MyShip.yCoor + screenSize; ++col) {
    Serial.print(char_PrintBoarder);
    for (int row = MyShip.xCoor - screenSize; row < MyShip.xCoor + screenSize; ++row) {
      if (row < xSize && row > 0 && col < ySize && col > 0) {
        Serial.print(getSea(row,col));
        //Serial.print('0'+getId(row,col));
        //Serial.print('|');
      } else {
        Serial.print(char_OutBounds);
      }
    }
    Serial.print(char_PrintBoarder);
    Serial.println();
  }
  for (int i = 0; i < 2*screenSize+2; ++i) {Serial.print(char_PrintBoarder);}
  Serial.println();
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
    //Serial.println(theShip.selfSymbol);
    offsetX = 0;
    offsetY = 0;
    offsetX += i*theShip.angleV*-1;
    offsetY += i*theShip.angleH;
    if (i!=0&&theShip.angleV<0 && theShip.angleH>0) {offsetY+=i*theShip.angleV;}
    if (i!=0&&theShip.angleV<0 && theShip.angleH<0) {offsetY+=i*theShip.angleV*-1;}
    if (i!=0&&theShip.angleV>0 && theShip.angleH>0) {offsetY+=i*theShip.angleV*-1;}
    if (i!=0&&theShip.angleV>0 && theShip.angleH<0) {offsetY+=i*theShip.angleV;}
    for (int j = 0; j < theShip.shipSizeL; ++j) {
      //Serial.println(theShip.selfSymbol);
      //Serial.println(theShip.xCoor+offsetX);
      toRunFunc(theShip.xCoor+offsetX,theShip.yCoor+offsetY, stateHolder);
      offsetX +=theShip.angleH;
      offsetY +=theShip.angleV;
    }
  }
  //return stateHolder;
}

void drawFrame(Ship theShip, char symbol = char_Ship) {
  //Serial.println(theShip.selfSymbol);
  //Serial.println(theShip.id);
  int throwAway = 0;
  accessShip(theShip, 
    [symbol, theShip](int X, int Y, int&){
      //Serial.println(symbol);
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
static char* input;
SemaphoreHandle_t inputNotReadSemaphore = xSemaphoreCreateBinary();
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
        //Serial.print("newinput:");
        //Serial.println(index);
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

SemaphoreHandle_t CannonArrayMutex = xSemaphoreCreateMutex();
void spawnCannonBall(Ship& theShip, bool isLeft){//, int portNum = 2, int shotAngle = NULL){
  int portNum = isLeft ? theShip.currPortCan++ : theShip.currStarCan++;  //this might not be thread safe?,  but nothing else should update or access this?
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
    int openSlot = -1;//If the function got past the if statment, there should be a open slot
    for (int i = 0; i < maxNumBalls; ++i){ if (cannonBallShips[i].id == nullShip.id) {openSlot=i;} }
    
    //Serial.println(openSlot);
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    cannonBallShips[openSlot]=Ship{newXCoor, newYCoor, newHAngle, newVAngle, 1, 1, 1, 1, 1, char_CannonBall,  1+numCoves+numCoves+numOfBallsCreated+1, 0, 0};
    numOfBalls +=1;
    numOfBallsCreated+=1;
    xSemaphoreGive(CannonArrayMutex);

    
    //drawFrame(cannonBallShips[openSlot], char_CannonBall);//REMOVE LATER IS FOR TESTING
    
  } else {
    xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
    Serial.println("Misfire");
    xSemaphoreGive(SerialOutMutex);
    
  }
  
}
void updateAngle(Ship& theShip, int newH, int newV){
  Ship tempShip = theShip;
  //Ship copyNullShip = nullShip;
  crashedAndType crashed{false, false, tempShip, nullptr};//copyNullShip};

  
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
    //Serial.println(crashed.crashedOtherShip.id != nullShip.id);
    //Serial.println(crashed.crashedOtherShip.id);
    //Serial.println(nullShip.id);
    if (crashed.crashedOtherShip != nullptr){//nullShip.id) {
      decOtherShipHealth(*crashed.crashedOtherShip);
      //xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
      //Serial.print("Hit another ship. Hit Ship: ");
      //Serial.println((*crashed.crashedOtherShip).id);
      //Serial.println(crashed.crashedOtherShip.xCoor);
      //Serial.println(crashed.crashedOtherShip.yCoor);
      //Serial.println((*crashed.crashedOtherShip).health);
      //xSemaphoreGive(SerialOutMutex);
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
  //if (getSea(newX,newY) != char_SteadySea&&getSea(newX,newY) != crashed.selfShip.selfSymbol) 
  if (getSea(newX,newY) != char_SteadySea&&getId(newX,newY) != crashed.selfShip.id) 
  {
    crashed.crashed = true;
    if (getId(newX,newY) > 1+numCoves) {
      crashed.crashedOtherShip = &otherShips[getId(newX,newY)-numCoves-1-1];
      //Serial.println("hits");
      //Serial.println(getId(newX,newY));
      //Serial.println(crashed.crashedOtherShip.id);
      //crashed.crashedOtherShip.health = -1;
      //Serial.println(crashed.crashedOtherShip.health);
      //Serial.println(otherShips[getId(newX,newY)-numCoves-1-1].health);
      //Serial.println("hats");
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
    //Ship copyNullShip = nullShip;
    crashedAndType crashed{false, false, tempShip, nullptr};//copyNullShip};

    
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
      //Serial.println(crashed.crashedOtherShip.id != nullShip.id);
      //Serial.println(crashed.crashedOtherShip.id);
      //Serial.println(nullShip.id);
      if (crashed.crashedOtherShip != nullptr){//nullShip.id) {
        decOtherShipHealth(*crashed.crashedOtherShip);
        //xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
        //Serial.print("Hit another ship. Hit Ship: ");
        //Serial.println((*crashed.crashedOtherShip).id);
        //Serial.println(crashed.crashedOtherShip.xCoor);
        //Serial.println(crashed.crashedOtherShip.yCoor);
        //Serial.println((*crashed.crashedOtherShip).health);
        //xSemaphoreGive(SerialOutMutex);
      }
    }else {
      theShip.xCoor = tempX;
      theShip.yCoor = tempY;
    }
  }
}
int AIVisionDis = 20;
//I dont want the ships drawing themselves until the printSea() command, but that would likely be bug prone
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
    cleanFrame(copyShip);//this might cause a minor race condition that causes the other ships to sometimes disappear for a frame
    updateShips(theShip);//but it is not worth making this many function calls a critical zone?
    drawFrame(theShip, char_OtherShip);//or maybe it is?
  } else drawFrame(theShip, char_DeadShip);
}

SemaphoreHandle_t myShipMovedSem = xSemaphoreCreateBinary();
void runOtherShipAIs(void *pvParameters){
  while(1){
    if (uxSemaphoreGetCount(myShipMovedSem)==0){
      for (int i = 0; i < numCoves; ++i){
        otherShipAI(otherShips[i]);
      }
    }
    xSemaphoreGive(myShipMovedSem);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    //vTaskDelete(NULL);
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
static const int led_pin = LED_BUILTIN;
int critHealth = 3;
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

SemaphoreHandle_t myShipMovedBallsSem = xSemaphoreCreateBinary();
void runCannonBalls(void *pvParameters){
  while(1){
    if (numOfBalls>0 && uxSemaphoreGetCount(myShipMovedBallsSem)==0){
      
      for (int i = 0; i < maxNumBalls; ++i){
        if (cannonBallShips[i].id != nullShip.id){
          xSemaphoreTake(CannonArrayMutex, portMAX_DELAY);
          if(cannonBallShips[i].health>0){
            cleanFrame(cannonBallShips[i]);//this might cause a minor race condition that causes the other ships to sometimes disappear for a frame
            updateShips(cannonBallShips[i]);//but it is not worth making this many function calls a critical zone?
            drawFrame(cannonBallShips[i], char_CannonBall);//or maybe it is?
          } else {
          drawFrame(cannonBallShips[i], char_SteadySea);
          numOfBalls-=1;
          cannonBallShips[i] = nullShip;
          }
          xSemaphoreGive(CannonArrayMutex);
        }
      }
    }
    xSemaphoreGive(myShipMovedBallsSem);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    //vTaskDelete(NULL);
  }
}

void setup() {
  Serial.begin(250000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  pinMode(led_pin, OUTPUT);
  xSemaphoreGive(inputNotReadSemaphore);
  xSemaphoreGive(AccessSeaMutex);
  xSemaphoreGive(AccessMyShipHealthMutex);
  xSemaphoreGive(myShipMovedSem);
  xSemaphoreGive(SerialOutMutex);
  xSemaphoreGive(myShipMovedBallsSem);
  for (int i = 0; i < maxNumBalls; ++i){ cannonBallShips[i] = nullShip;}
  
  makeSea();
  makeCoves();
  drawCoves();
  makeOtherShips();
  initDrawOtherShips();
  drawFrame(MyShip);
  printSea();

//  spawnCannonBall(MyShip, true, 0, 2);
//  //Serial.println(cannonBallShips[0].xCoor);
//  //Serial.println(cannonBallShips[0].yCoor);
//  //Serial.println(cannonBallShips[0].selfSymbol);
//  drawFrame(cannonBallShips[0], char_CannonBall);
//  printSea();
  
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
  
//  cleanFrame(cannonBallShips[0]);
//  updateShips(cannonBallShips[0]);
//  drawFrame(cannonBallShips[0], char_CannonBall);
  
  xSemaphoreTake(myShipMovedSem, (TickType_t) 10);
  xSemaphoreTake(myShipMovedBallsSem, (TickType_t) 10);
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
