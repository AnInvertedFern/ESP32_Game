const int xSize = 100;
const int ySize = 100;
int screenSize = 16;//screen size is actually twice as big

SemaphoreHandle_t AccessSeaMutex = xSemaphoreCreateMutex();
char* sea = (char*) pvPortMalloc(xSize*ySize*sizeof(char));

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
};

struct crashedAndType{
  bool crashed;
  bool crashedMyShip;
  Ship selfShip;
};

int inputLength = 11;
const int numCoves = 70;
int* covesX = (int*) pvPortMalloc(numCoves*sizeof(int));
int* covesY = (int*) pvPortMalloc(numCoves*sizeof(int));
struct Ship otherShips[numCoves];

char char_SteadySea = '~';
char char_Ship = '=';
char char_OtherShip = '#';
char char_OutBounds = '*';
char char_Land = '^';
char char_ShipHead = '@';
char char_DeadShip = 'x';
char char_DeadMyShip = '.';
char char_PrintBoarder = ':';

Ship MyShip{(xSize / 2), (ySize / 2), 0, -1, 4, 2, 0, 3, 4*2, char_Ship};
//int xCoor = (xSize / 2);
//int yCoor = (ySize / 2);
//int angleH = 1;
//int angleV = 0;
//int shipSizeL = 4;
//int shipSizeW = 2;
//int speed = 0;
//int maxSpeed = 3;
//int health = shipSizeL*shipSizeW;


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

char getSea(int x, int y){
  char temp;
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  temp = sea[x+y*xSize];
  xSemaphoreGive(AccessSeaMutex);
  return temp;
}
void setSea(int x, int y, char symbol){
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  sea[x+y*xSize] = symbol;
  xSemaphoreGive(AccessSeaMutex);
}

void makeSea() {
  for (int row{0}; row < xSize; ++row) {
    for (int col{0}; col < ySize; ++col) {
      setSea(row,col, char_SteadySea);
    }
  }
}

void makeCoves() {
  int tempCoor = 0;
  for (int i{0}; i< numCoves; ++i) {
    tempCoor = random(0, xSize);
    if (tempCoor > MyShip.xCoor +5 || tempCoor < MyShip.xCoor-5)
    covesX[i] = tempCoor;
    else covesX[i] = 0;//FIGURE OUT BETTER SOLUTION LATER
  }
  for (int i{0}; i< numCoves; ++i) {
    tempCoor = random(0, xSize);
    if (tempCoor > MyShip.yCoor +5 || tempCoor < MyShip.yCoor-5)
    covesY[i] = tempCoor;
    else covesY[i] = 0;//FIGURE OUT BETTER SOLUTION LATER
  }
}
void makeOtherShips() {
  int tempCoorX = 0;
  int tempCoorY = 0;
  for (int i{0}; i< numCoves; ++i) {
    tempCoorX = random(0, xSize);
    if (tempCoorX < MyShip.xCoor +5 && tempCoorX > MyShip.xCoor-5)
    {tempCoorX = 0;}//FIGURE OUT BETTER SOLUTION LATER
    
    tempCoorY = random(0, xSize);
    if (tempCoorY < MyShip.yCoor +5 && tempCoorY > MyShip.yCoor-5)
    {tempCoorY = 0;}//FIGURE OUT BETTER SOLUTION LATER
    otherShips[i] = Ship{tempCoorX, tempCoorY, 1, 0, 3, 1, 0, 1, 2, char_OtherShip};
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
      setSea(covesX[i],covesY[i], char_Land);
      if (covesX[i]+1 < xSize){setSea(covesX[i]+1,covesY[i], char_Land);}
      if (covesX[i]-1 > 0){setSea(covesX[i]-1,covesY[i], char_Land);}
      if (covesY[i]+1 < ySize){setSea(covesX[i],covesY[i]+1, char_Land);}
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
    [symbol](int X, int Y, int&){
      setSea(X,Y, symbol);
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
      setSea(X,Y, char_SteadySea);
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
    //Serial.println("here");
    //char* input = new char[inputLength];
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
void updateAngle(Ship& theShip, int newH, int newV){//changing angles does not clean the ship properly?   fixed
  Ship tempShip = theShip;
  crashedAndType crashed{false, false, tempShip};

  
  tempShip.angleH = newH;
  tempShip.angleV = newV;
  accessShip(tempShip, checkCrashedHelper, crashed);
  
  if (crashed.crashed){
    if (&theShip==&MyShip){decMyShipHealth();}else{theShip.health -=1;}
    if (crashed.crashedMyShip) {
      decMyShipHealth();
      xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
      Serial.println("You got hit");
      xSemaphoreGive(SerialOutMutex);
      }
  }else {
    theShip.angleH = newH;
    theShip.angleV = newV;
  }
}
void processCommand(char* input, Ship& theShip){
//  Serial.println(input[0]);
//  Serial.println(input[1]);
//  Serial.println(input[2]);
//  Serial.println(input[3]);
//  Serial.println(input[4]=='\n');
//  Serial.println(input[5]=='\n');
//  Serial.println(strcmp(input, "left"));
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
    if (theShip.speed < theShip.maxSpeed+1) { theShip.speed+=1;}// Serial.println("faster");}
  } else if (strcmp(input, "slower") == 0) {
    if (theShip.speed>0) { theShip.speed -=1;}// Serial.println("slower");}
  } //else Serial.println("none");

  updateAngle(theShip, tempH, tempV);
}


void checkCrashedHelper(int newX, int newY, crashedAndType& crashed){
  if (newX >= xSize || newX <= 0 || newY >= ySize || newY <= 0) { crashed.crashed = true;}//Serial.println("Hit world boarder");}
  if (getSea(newX,newY) != char_SteadySea&&getSea(newX,newY) != crashed.selfShip.selfSymbol) 
  {
    crashed.crashed = true;//Serial.println("You crashed");
    if (getSea(newX,newY) == char_Ship) {crashed.crashedMyShip = true;}//{MyShip.health-=1;Serial.println("You got hit");}//really bad fix
  }
}

void updateShips(Ship& theShip){
  int tempX = theShip.xCoor;
  int tempY = theShip.yCoor;
  
  for (int s = 0; s < theShip.speed; ++s){
    tempX += theShip.angleH;
    tempY += theShip.angleV;

    Ship tempShip = theShip;
    crashedAndType crashed{false, false, tempShip};

    
    tempShip.xCoor = tempX;
    tempShip.yCoor = tempY;
    accessShip(tempShip, checkCrashedHelper, crashed);
    
    if (crashed.crashed){
      if (&theShip==&MyShip){decMyShipHealth();}else{theShip.health -=1;}
      if (crashed.crashedMyShip) {
        decMyShipHealth();
        xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
        Serial.println("You got hit");
        xSemaphoreGive(SerialOutMutex);
      }
    }else {
      theShip.xCoor = tempX;
      theShip.yCoor = tempY;
    }
  }
}

//I dont want the ships drawing themselves until the printSea() command, but that would likely be bug prone
void otherShipAI(Ship& theShip){
  if (theShip.health>0){
    Ship copyShip = theShip;
    if (abs(MyShip.xCoor-theShip.xCoor)<20&&abs(MyShip.yCoor-theShip.yCoor)<20){
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
void runOtherShipAIs(void *pvParameters){//triggers stack canary   fixed
  while(1){
    if (uxSemaphoreGetCount(myShipMovedSem)==0){
      for (int i = 0; i < numCoves; ++i){
        otherShipAI(otherShips[i]);
      }
    }
    xSemaphoreGive(myShipMovedSem);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    //vTaskDelete(NULL);
  }
}

void restartOnDelay(void* pvParameters){
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.println("Restarting Game");
  xSemaphoreGive(SerialOutMutex);
      
  //Serial.println("Restarting Game");
  ESP.restart();
}
void makeGetCommandTask(TimerHandle_t xTimer){
  xTaskCreatePinnedToCore(getCommand, "get_command", 1024, NULL, 4, NULL, tskNO_AFFINITY);
}

void setup() {
  Serial.begin(250000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  xSemaphoreGive(inputNotReadSemaphore);
  xSemaphoreGive(AccessSeaMutex);
  xSemaphoreGive(AccessMyShipHealthMutex);
  xSemaphoreGive(myShipMovedSem);
  xSemaphoreGive(SerialOutMutex);
  
  makeSea();
  makeCoves();
  drawCoves();
  makeOtherShips();
  initDrawOtherShips();
  drawFrame(MyShip);
  //Serial.println("here");
  printSea();
  
//  xTaskCreatePinnedToCore(getCommand, "get_command", 1024, NULL, 1, NULL, tskNO_AFFINITY);
//  vTaskDelay(5000 / portTICK_PERIOD_MS);
//  if (uxSemaphoreGetCount(inputNotReadSemaphore)==0){
//    processCommand(input, MyShip); 
//    vPortFree((void*)input);
//    xSemaphoreGive(inputNotReadSemaphore);
////  }
//  cleanFrame(MyShip);
//  updateShips(MyShip);
//  xSemaphoreTake(myShipMovedSem, (TickType_t) 10);
//  drawFrame(MyShip);
//  printSea();
  TimerHandle_t inputTimer = xTimerCreate("make_get_command_task", 500 / portTICK_PERIOD_MS, pdTRUE, (void *)0, makeGetCommandTask);
  if (inputTimer == NULL) {
    ESP.restart();
  } else {
    //Serial.println("here");
    xTimerStart(inputTimer, portMAX_DELAY);
  }
  xTaskCreatePinnedToCore(runOtherShipAIs, "run_other_ship_AIs", 5024, NULL, 1, NULL, tskNO_AFFINITY);

}



void loop() {
  if (getMyShipHealth()>0){
    //xTaskCreatePinnedToCore(getCommand, "get_command", 1024, NULL, 4, NULL, tskNO_AFFINITY);
    Ship tempShip = MyShip;
    if (uxSemaphoreGetCount(inputNotReadSemaphore)==0){
      //Serial.println("got command");
      //Serial.println(input);
      processCommand(input, MyShip); 
      vPortFree((void*)input);
      xSemaphoreGive(inputNotReadSemaphore);
    }
    cleanFrame(tempShip);
    updateShips(MyShip);
    drawFrame(MyShip);
  }
  xSemaphoreTake(myShipMovedSem, (TickType_t) 10);
  printSea();
  if (getMyShipHealth()<=0){
    
    xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
    Serial.println("The Ship died!");
    xSemaphoreGive(SerialOutMutex);
    drawFrame(MyShip, char_DeadMyShip);
    xTaskCreatePinnedToCore(restartOnDelay, "restart_on_delay", 1024, NULL, 1, NULL, tskNO_AFFINITY);
  }
  //}
  //else {}
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  

  
  // It wont let me free my variables, it just corrupts my heap
  //probably cause theyre global
  //FIX LATER
  //or because this is a loop, im too tired
  //vPortFree((void*)covesX);
  //vPortFree((void*)covesY);
  //vPortFree((void*)sea);
}
