const int xSize = 100;
const int ySize = 100;
int screenSize = 20;//screen size is actually twice as big
char* sea = (char*) pvPortMalloc(xSize*ySize*sizeof(char));
int xCoor = (xSize / 2);
int yCoor = (ySize / 2);
int angleH = 1;
int angleV = 0;
int shipSizeL = 4;
int shipSizeW = 2;
int inputLength = 11;
int numCoves = 70;
int* covesX = (int*) pvPortMalloc(numCoves*sizeof(int));
int* covesY = (int*) pvPortMalloc(numCoves*sizeof(int));
int speed = 0;
int maxSpeed = 3;
int health = shipSizeL*shipSizeW;

char char_SteadySea = '~';
char char_Ship = '=';
char char_OutBounds = '*';
char char_Land = '^';


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
    if (tempCoor > xCoor +5 || tempCoor < xCoor-5)
    covesX[i] = tempCoor;
    else covesX[i] = NULL;
  }
  for (int i{0}; i< numCoves; ++i) {
    tempCoor = random(0, xSize);
    if (tempCoor > yCoor +5 || tempCoor < yCoor-5)
    covesY[i] = tempCoor;
    else covesY[i] = NULL;
  }
}

void printSea() {
  for (int col = yCoor - screenSize; col < yCoor + screenSize; ++col) {
    for (int row = xCoor - screenSize; row < xCoor + screenSize; ++row) {
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

void drawFrame() {
  
  //sea[xCoor][yCoor] = '*';
  int offsetX = 0;
  int offsetY = 0;
  for (int i = 0; i < shipSizeW; ++i) {
    offsetX = 0;
    offsetY = 0;
    offsetX += i*angleV*-1;
    offsetY += i*angleH;
    //Serial.print(xCoor+offsetX);
    //Serial.print(yCoor+offsetY);
    //sea[xCoor+offsetX+1][yCoor+offsetY] = '*';
    //offsetX +=angleH;
    //offsetY +=angleV;
    for (int j = 0; j < shipSizeL; ++j) {
      getSea(xCoor+offsetX,yCoor+offsetY) = char_Ship;
      offsetX +=angleH;
      offsetY +=angleV;
    }
  }
}

void cleanFrame() {
  //sea[xCoor][yCoor] = '*';
  int offsetX = 0;
  int offsetY = 0;
  for (int i = 0; i < shipSizeW; ++i) {
    offsetX = 0;
    offsetY = 0;
    offsetX += i*angleV*-1;
    offsetY += i*angleH;
    //Serial.print(xCoor+offsetX);
    //Serial.print(yCoor+offsetY);
    //sea[xCoor+offsetX+1][yCoor+offsetY] = '*';
    //offsetX +=angleH;
    //offsetY +=angleV;
    for (int j = 0; j < shipSizeL; ++j) {
      getSea(xCoor+offsetX,yCoor+offsetY) = char_SteadySea;
      offsetX +=angleH;
      offsetY +=angleV;
    }
  }
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

void processCommand(char* input){
  if (strcmp(input, "down") == 0){
    if (angleV < 1) { angleV += 1; Serial.println("down");}
    if (angleV == 0) { angleV += 1; }
  } else if (strcmp(input, "up") == 0) {
    if (angleV > -1) { angleV -= 1; Serial.println("up");}
  } else if (strcmp(input, "right") == 0) {
    if (angleH < 1) { angleH += 1; Serial.println("right");}
  } else if (strcmp(input, "left") == 0) {
    if (angleH > -1) { angleH -= 1; Serial.println("left");}
  } else if (strcmp(input, "faster") == 0) {
    if (speed < maxSpeed+1) { speed+=1; Serial.println("faster");}
  } else if (strcmp(input, "slower") == 0) {
    if (speed>0) { speed -=1; Serial.println("slower");}
  } else Serial.println("none");

  if (angleH == 0 && angleV == 0) {angleV = -1;}
}


void updateShips(){
  int tempX = xCoor;
  int tempY = yCoor;
  Serial.println(angleH);
  Serial.println(angleV);
  Serial.println(xCoor);
  Serial.println(yCoor);
  
  for (int s = 0; s < speed; ++s){
    tempX += angleH;
    tempY += angleV;
    //This really should be handled by a function that takes a function to run on each location
    //but doing that in strongly typed systems makes a mess
  
    bool crashed = false;
    int offsetX = 0;
    int offsetY = 0;
    for (int i = 0; i < shipSizeW; ++i) {
      offsetX = 0;
      offsetY = 0;
      offsetX += i*angleV*-1;
      offsetY += i*angleH;
      for (int j = 0; j < shipSizeL; ++j) {
        if (getSea(tempX+offsetX,tempY+offsetY) != char_SteadySea) {crashed = true;Serial.println("crashed");}
        if (tempX+offsetX >= xSize || tempX+offsetX <= 0 || tempY+offsetY >= ySize || tempY+offsetY <= 0) { crashed = true;}
        offsetX +=angleH;
        offsetY +=angleV;
      }
    }
    if (crashed){
      health -=1;
    }else {
      xCoor = tempX;
      yCoor = tempY;
    }
  }
}

void setup() {
  Serial.begin(250000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  randomSeed(5); //Want same layout each time for debug
  makeSea();
  makeCoves();
  drawCoves();
  drawFrame();
  printSea();
  char* input = getCommand(); //REM TO DELETE
  cleanFrame();
  processCommand(input);
  vPortFree((void*)input);
  updateShips();
  drawFrame();
  printSea();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  //Serial.println("Finished");

}


void loop() {
  
  char* input = getCommand(); //REM TO DELETE
  cleanFrame();
  processCommand(input);
  vPortFree((void*)input);
  updateShips();
  drawFrame();
  printSea();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  

  
  // It wont let me free my variables, it just corrupts my heap
  //probably cause theyre global
  //FIX LATER
  //or because this is a loop, im too tired
  //vPortFree((void*)covesX);
  //vPortFree((void*)covesY);
  //vPortFree((void*)sea);
}
