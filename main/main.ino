const int xSize = 100;
const int ySize = 100;
int screenSize = 20;//screen size is actually twice as big
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
const int numCoves = 20;
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


char& getSea(int x, int y){
  return sea[x+y*xSize];
}

void makeSea() {
  for (int row{0}; row < xSize; ++row) {
    for (int col{0}; col < ySize; ++col) {
      getSea(row,col) = char_SteadySea;
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
    otherShips[i] = Ship{tempCoorX, tempCoorY, 1, 0, 3, 1, 0, 3, 2, char_OtherShip};
  }  
}

void printSea() {
  for (int col = MyShip.yCoor - screenSize; col < MyShip.yCoor + screenSize; ++col) {
    for (int row = MyShip.xCoor - screenSize; row < MyShip.xCoor + screenSize; ++row) {
      if (row < xSize && row > 0 && col < ySize && col > 0) {
        Serial.print(getSea(row,col));
      } else {
        Serial.print(char_OutBounds);
      }

    }
    Serial.println();
  }
}
void drawCoves(){
  
  for (int i = 0; i < numCoves; ++i) {
      getSea(covesX[i],covesY[i]) = char_Land;
      if (covesX[i]+1 < xSize){getSea(covesX[i]+1,covesY[i]) = char_Land;}
      if (covesX[i]-1 > 0){getSea(covesX[i]-1,covesY[i]) = char_Land;}
      if (covesY[i]+1 < ySize){getSea(covesX[i],covesY[i]+1) = char_Land;}
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
      getSea(X,Y) = symbol;
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
      getSea(X,Y) = char_SteadySea;
    }
    , throwAway);
}

char* getCommand() {
  char c;
  //char* input = new char[inputLength];
  char* input = (char*) pvPortMalloc(inputLength*sizeof(char));
  uint8_t index = 0;

  memset(input, 0, inputLength);
  
  while(true) {
    if (Serial.available() > 0) {
      c = Serial.read();

      if (c == '\n') {
        input[index] = '\0';
        return input;
      }
      if (index < inputLength - 1) {
        input[index] = c;
        index++;
      }
    }
  }
}
void updateAngle(Ship& theShip, int newH, int newV){
  Ship tempShip = theShip;
  crashedAndType crashed{false, false, tempShip};

  
  tempShip.angleH = newH;
  tempShip.angleV = newV;
  accessShip(tempShip, checkCrashedHelper, crashed);
  
  if (crashed.crashed){
    theShip.health -=1;
    if (crashed.crashedMyShip) {MyShip.health-=1;}
  }else {
    theShip.angleH = newH;
    theShip.angleV = newV;
  }
}
void processCommand(char* input, Ship& theShip){
  int tempH = theShip.angleH;
  int tempV = theShip.angleV;
  if (strcmp(input, "down") == 0){
    if (theShip.angleV < 1 && theShip.angleH!=0) { tempV += 1; Serial.println("down");}
  } else if (strcmp(input, "up") == 0) {
    if (theShip.angleV > -1&& theShip.angleH!=0) { tempV -= 1; Serial.println("up");}
  } else if (strcmp(input, "right") == 0) {
    if (theShip.angleH < 1&& theShip.angleV!=0) { tempH += 1; Serial.println("right");}
  } else if (strcmp(input, "left") == 0) {
    if (theShip.angleH > -1&& theShip.angleV!=0) { tempH -= 1; Serial.println("left");}
  } else if (strcmp(input, "faster") == 0) {
    if (theShip.speed < theShip.maxSpeed+1) { theShip.speed+=1; Serial.println("faster");}
  } else if (strcmp(input, "slower") == 0) {
    if (theShip.speed>0) { theShip.speed -=1; Serial.println("slower");}
  } else Serial.println("none");

  updateAngle(theShip, tempH, tempV);
}


void checkCrashedHelper(int newX, int newY, crashedAndType& crashed){
  if (newX >= xSize || newX <= 0 || newY >= ySize || newY <= 0) { crashed.crashed = true;}//Serial.println("Hit world boarder");}
  if (getSea(newX,newY) != char_SteadySea&&getSea(newX,newY) != crashed.selfShip.selfSymbol) 
  {
    crashed.crashed = true;//Serial.println("You crashed");
    if (getSea(newX,newY) == char_Ship) {crashed.crashedMyShip = true;Serial.println("You got hit");}//{MyShip.health-=1;Serial.println("You got hit");}//really bad fix
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
      theShip.health -=1;
      if (crashed.crashedMyShip) {MyShip.health-=1;}
    }else {
      theShip.xCoor = tempX;
      theShip.yCoor = tempY;
    }
  }
}

//I dont want the ships drawing themselves until the printSea() command, but that would likely be bug prone
void otherShipAI(Ship& theShip){
  if (theShip.health>0){
    cleanFrame(theShip);
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
    updateShips(theShip);
    drawFrame(theShip, char_OtherShip);
  } else drawFrame(theShip, char_DeadShip);
}

void setup() {
  Serial.begin(250000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  makeSea();
  makeCoves();
  drawCoves();
  makeOtherShips();
  initDrawOtherShips();
  drawFrame(MyShip);
  printSea();
  char* input = getCommand(); //REM TO DELETE
  cleanFrame(MyShip);
  processCommand(input, MyShip);
  vPortFree((void*)input);
  updateShips(MyShip);
  drawFrame(MyShip);
  printSea();
  vTaskDelay(1000 / portTICK_PERIOD_MS);

}



void loop() {
  if (MyShip.health>0){
    char* input = getCommand(); //REM TO DELETE
    cleanFrame(MyShip);
    processCommand(input, MyShip);
    vPortFree((void*)input);
    updateShips(MyShip);
    drawFrame(MyShip);
    printSea();
    for (int i = 0; i < numCoves; ++i){
      otherShipAI(otherShips[i]);
      
    }
    if (MyShip.health<=0){Serial.println("The Ship died!");}
  }
  else {}
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  

  
  // It wont let me free my variables, it just corrupts my heap
  //probably cause theyre global
  //FIX LATER
  //or because this is a loop, im too tired
  //vPortFree((void*)covesX);
  //vPortFree((void*)covesY);
  //vPortFree((void*)sea);
}
