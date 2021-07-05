SemaphoreHandle_t AccessSeaMutex = xSemaphoreCreateMutex(); //Controls access to the sea matrix (the view)
SemaphoreHandle_t SerialOutMutex = xSemaphoreCreateMutex(); //Controls access to the serial output
SemaphoreHandle_t AccessMyShipHealthMutex = xSemaphoreCreateMutex(); //Controls access to the player's ship's health
SemaphoreHandle_t AccessOtherShipHealthMutex = xSemaphoreCreateMutex(); //Controls access to functions that control all other ships health
SemaphoreHandle_t PrintSeaMutex = xSemaphoreCreateMutex(); //Used to prevent race conditions when updating ship placement and printing the view
SemaphoreHandle_t inputNotReadSemaphore = xSemaphoreCreateBinary(); //Used to signal if the player has given a new command
SemaphoreHandle_t CannonArrayMutex = xSemaphoreCreateMutex(); //Controls acces to the array of living cannon balls
SemaphoreHandle_t myShipMovedSem = xSemaphoreCreateBinary(); //Used to tell the task that controls the other ships when the player has moved
SemaphoreHandle_t myShipMovedBallsSem = xSemaphoreCreateBinary(); //Used to tell the task that updates the cannon balls that the player has moved
SemaphoreHandle_t NumBallsMutex = xSemaphoreCreateMutex(); //Controls access the the number of living cannon ball counter

//Size of the whole sea
const int xSize = 100;
const int ySize = 100;
//Size of the slice of the sea displayed at once
int screenSize = 25;//screen size is actually twice as big

const int led_pin = LED_BUILTIN;
//The health at which the led starts to flash rapidly
int critHealth = 3;
//Holds input commands from the player
char* input;
int inputLength = 11; //The input strings max size

char sea [xSize*ySize]; //The view of the game, holds all tiles of the game, including off screen ones
int ids [xSize*ySize]; //Holds the ids of objects in relation to the tiles on the screen
//i.e. it mirrors the sea matix but holds object ids (as ints) instead of object types (via chars)
// (0,0) is upper left corner, (xSize,ySize) is lower right corner
//Both are flattened martices

struct Ship { //Holds all info that functions need to know about a particular ship
  int xCoor; //Current x position in terms of sea matrix
  int yCoor; //Current y position
  int angleH; //Current horizontal angle; 1 means it points to the right, -1 means it points to the left
  int angleV; //Current vertical angle; 1 means it points down; -1 means it points up
  int shipSizeL; //Size of the ship long-wise (out from the center of the screen)
  int shipSizeW; //Size of the ship width-wise (perpendicular from the center of the screen)
  int speed;  //Current speed of the boat in tiles per frame
  int maxSpeed; //Max speed of the boat
  int health; //Current hit points of the boat
  char selfSymbol; //The symbol (char) used for printing and displaying the boat
  int id; //The id for reference through the ids matrix
  int currPortCan; //The gunport port-side (left) the next cannon ball should come out of
  int currStarCan; //The gunport starboard-side (right) the next cannon ball should come out of
};

struct crashedAndType{ //Used to pass and extract information into the ship look-up function to monitor collisions
  //It is the stateHolder (state) variable in the accessShip for collision handling (with the checkCrashedHelper function passed in)
  bool crashed; //Is set to true if the passed in ship hit something (is set in the collision function)
  bool crashedMyShip; //Is set to true if the thing that was hit was the player's ship (is set in the collision function)
  Ship& selfShip; //The Ship struct that is doing the moving (is before the collision function)
  Ship* crashedOtherShip; //The Ship struct that got hit (is set in the collision function)
};

const int numCoves = 50; //The max number of islands in the game (ships can collide with them); it is also used for the number of enemy ships
int covesX[numCoves]; //Array of the x coordinates of all coves
int covesY[numCoves]; //Array of the y coordinates of all coves
struct Ship otherShips[numCoves]; //Array of all other (non-player) ships structs in the game
int AIVisionDis = 20; //Holds how far the enemy ships can see the player from
const int maxNumBalls = 20; //The max number of cannon balls that can be on the screen/game at once
int numOfBalls = 0; //Current number of live balls
int numOfBallsCreated = 0; //Total number of shots fired (used to create ids for new cannon balls)
struct Ship cannonBallShips[maxNumBalls]; //Array of all living cannon balls in the game
//Cannon balls are ships

//All characters (ascii) displayed in the game
char char_SteadySea = '~'; //Char for normal sea
char char_Ship = '='; //Char for the player's boat
char char_OtherShip = '#'; //Char for all other boats (not including cannon balls)
char char_OutBounds = '*'; //Char for world border
char char_Land = '^'; //Char for islands
char char_DeadShip = 'x'; //Char for non-living non-player non-cannon ball ships
char char_DeadMyShip = '.'; //Char for dead player ship
char char_PrintBoarder = ':'; //Char for screen (not sea) boarder; used for display
char char_CannonBall = '0'; //Char for cannon balls

Ship nullShip{-1, -1, -3, -3, -1, -1, -1, -1, -1, '\n', -1, -1, -1}; //A null Ship to pass into functions to be replaced
Ship MyShip{(xSize / 2), (ySize / 2), 0, -1, 4, 2, 0, 3, 8, char_Ship, 1, 0, 0}; //The player's Ship
int steadySeaID = 0; //The id for all sea tiles; their ids are all the same
int myShipID = 1; //The id for the player's ship
//Then increment 1 for all cove ids (the number of coves is set at compile time)
//Then increment 1 for all other ships ids (the number of coves is set at compile time)
//Then increment 1 for all cannon ball ids (the ids are not reused after ball death and new ids are drawn from the end)

void incMyShipHealth(){ //Increments the health of the player ship; not currently used
  xSemaphoreTake(AccessMyShipHealthMutex, portMAX_DELAY);
  MyShip.health +=1;
  xSemaphoreGive(AccessMyShipHealthMutex);

  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.print("Your Health: ");
  Serial.println(MyShip.health);
  xSemaphoreGive(SerialOutMutex);
}

void decMyShipHealth(){ //Decrements the health of the player ship
  xSemaphoreTake(AccessMyShipHealthMutex, portMAX_DELAY);
  MyShip.health -=1;
  xSemaphoreGive(AccessMyShipHealthMutex);

  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.print("Your Health: ");
  Serial.println(MyShip.health);
  xSemaphoreGive(SerialOutMutex);
}

int getMyShipHealth(){ //Returns the player ship's health
  int temp;
  xSemaphoreTake(AccessMyShipHealthMutex, portMAX_DELAY);
  temp = MyShip.health;
  xSemaphoreGive(AccessMyShipHealthMutex);
  return temp;
}

void incOtherShipHealth(Ship& otherShip){ //Increments the health of a non-player ship; not currently used
  xSemaphoreTake(AccessOtherShipHealthMutex, portMAX_DELAY);
  otherShip.health +=1;
  xSemaphoreGive(AccessOtherShipHealthMutex);
}

void decOtherShipHealth(Ship& otherShip){ //Decrements the health of a non-player ship
  xSemaphoreTake(AccessOtherShipHealthMutex, portMAX_DELAY);
  otherShip.health -=1;
  xSemaphoreGive(AccessOtherShipHealthMutex);
}

int getOtherShipHealth(Ship& otherShip){ //Returns the health of a non-player ship
  int temp;
  xSemaphoreTake(AccessOtherShipHealthMutex, portMAX_DELAY);
  temp = otherShip.health;
  xSemaphoreGive(AccessOtherShipHealthMutex);
  return temp;
}

char getSea(int x, int y){ //Returns the char for the tile
  char temp;
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  temp = sea[x+y*xSize]; //Is a flattened array
  xSemaphoreGive(AccessSeaMutex);
  return temp;
}

char getId(int x, int y){ //Returns the id for the object drawn on the tile
  int temp;
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  temp = ids[x+y*xSize];
  xSemaphoreGive(AccessSeaMutex);
  return temp;
}

void setSea(int x, int y, char symbol, int id){ //Sets both the char for display and the id for a tile; both must be set or replaced as one
  xSemaphoreTake(AccessSeaMutex, portMAX_DELAY);
  sea[x+y*xSize] = symbol;
  ids[x+y*xSize] = id;
  xSemaphoreGive(AccessSeaMutex);
}

void makeSea() { //Fills the sea matrix with normal sea chars and its id
  for (int row{0}; row < xSize; ++row) {
    for (int col{0}; col < ySize; ++col) {
      setSea(row,col, char_SteadySea, steadySeaID);
    }
  }
}

void makeCoves() { //Fills the cove arrays (covesX and covesY) with coordinates to be drawn-in in the drawCoves function
  //If the random coordinate is too close to the player's starting location (in the center of the map)
  //They are pushed to the corners of the map sit there
  //All the alternatives require either too much tracking or unbounded while loops
  //Coves can be placed ontop of each other
  //Coves do not currently do much other than serve as obstacle, so it does not currently matter
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

void makeOtherShips() { //Files the otherShips array with the enemy ships to be drawn-in in the initDrawOtherShips
  //If the random coordinate is too close to the player's starting location (in the center of the map)
  //They are pushed to the corners of the map to die, it works
  //All the alternatives require either too much tracking or unbounded while loops
  //Ships can be placed ontop of each other, which kills them
  //Ships can also be placed on top of each other, which also kills them
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

void printSea() { //Draws the view to the screen
  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  xSemaphoreTake(PrintSeaMutex, portMAX_DELAY);
  for (int i = 0; i < 2*screenSize+2; ++i) {Serial.print(char_PrintBoarder);} //Print border
  Serial.println();
  for (int col = MyShip.yCoor - screenSize; col < MyShip.yCoor + screenSize; ++col) {
    Serial.print(char_PrintBoarder); //Print border
    for (int row = MyShip.xCoor - screenSize; row < MyShip.xCoor + screenSize; ++row) {
      if (row < xSize && row > 0 && col < ySize && col > 0) {
        Serial.print(getSea(row,col)); //Get and print char for that tile
      } else {
        Serial.print(char_OutBounds); //If the screen is off the map, print the out of bounds char
      }
    }
    Serial.print(char_PrintBoarder); //Print border
    Serial.println();
  }
  for (int i = 0; i < 2*screenSize+2; ++i) {Serial.print(char_PrintBoarder);} //Print border
  Serial.println();
  xSemaphoreGive(PrintSeaMutex);
  xSemaphoreGive(SerialOutMutex);
}

void drawCoves(){ //Draw the islands to the sea matrix, but does not print them
  for (int i = 0; i < numCoves; ++i) {
      setSea(covesX[i],covesY[i], char_Land, 1+i+1); //All islands are T shaped
      if (covesX[i]+1 < xSize){setSea(covesX[i]+1,covesY[i], char_Land, 1+i+1);}
      if (covesX[i]-1 > 0){setSea(covesX[i]-1,covesY[i], char_Land, 1+i+1);}
      if (covesY[i]+1 < ySize){setSea(covesX[i],covesY[i]+1, char_Land, 1+i+1);}
  }
}

//Finds the coordinates of the ship passed into the function
//Then applies those coordinates (one at a time) to the function toRunFunc with a state variable stateHolder
template<typename Fn, typename T>
void accessShip(Ship theShip, Fn toRunFunc, T& stateHolder) {
  int offsetX = 0;
  int offsetY = 0;
  //Trace the ship out one length-wise section at a time
  for (int i = 0; i < theShip.shipSizeW; ++i) { //For each width section of the Ship
    offsetX = 0;
    offsetY = 0;
    offsetX += i*theShip.angleV*-1;
    offsetY += i*theShip.angleH;
    if (i!=0&&theShip.angleV<0 && theShip.angleH>0) {offsetY+=i*theShip.angleV;} //The Ship grows width-wise in another direction than what it is facing
    if (i!=0&&theShip.angleV<0 && theShip.angleH<0) {offsetY+=i*theShip.angleV*-1;}
    if (i!=0&&theShip.angleV>0 && theShip.angleH>0) {offsetY+=i*theShip.angleV*-1;}
    if (i!=0&&theShip.angleV>0 && theShip.angleH<0) {offsetY+=i*theShip.angleV;}
    for (int j = 0; j < theShip.shipSizeL; ++j) { //Trace out the direction it is pointing
      toRunFunc(theShip.xCoor+offsetX,theShip.yCoor+offsetY, stateHolder); //Apply the found coordinates to the passed function
      offsetX +=theShip.angleH;
      offsetY +=theShip.angleV;
    }
  }
  //stateHolder is a reference so right now it is not passed back since no tuples
  //return stateHolder;
}

void drawFrame(Ship theShip, char symbol = char_Ship) { //Draws the Ship at its coordiates in the sea matrix with char symbol
  //This function does not print the ships
  int throwAway = 0;
  accessShip(theShip, 
    [symbol, theShip](int X, int Y, int&){
      setSea(X,Y, symbol,theShip.id);
    }
    , throwAway);
}

void initDrawOtherShips(){ //Does an inital drawing of the other Ships to the sea function; it does not print the Ships
  for (int i{0}; i< numCoves; ++i) {
    drawFrame(otherShips[i], char_OtherShip);
  } 
}

void cleanFrame(Ship theShip) { //Resets the coordiantes of the Ship back to being sea tiles
  //This function is used to move ships in the sea matrix
  int throwAway = 0;
  accessShip(theShip, 
    [](int X, int Y, int&){
      setSea(X,Y, char_SteadySea,steadySeaID);
    }
    , throwAway);
}

void getCommand(void *pvParameters) { //Task that checks serial-in for a command and then sets a semaphore
  static bool makeNewInput = true; //Tells if the task was in mid-input
  char c;
  uint8_t index = 0;

  //inputNotReadSemaphore is set to one if the main thread has finished processing all previous inputs
  if (makeNewInput && uxSemaphoreGetCount(inputNotReadSemaphore)==1) { 
    input = (char*) pvPortMalloc(inputLength*sizeof(char));
    memset(input, 0, inputLength);
    makeNewInput=false;
  }
  if (uxSemaphoreGetCount(inputNotReadSemaphore)==1){
    while(Serial.available() > 0) { //While there are chars in serial's buffer
      
      c = Serial.read();
  
      if (c == '\n'|| index >= inputLength) { //true if the command is finished
        input[index] = '\0';
        xSemaphoreTake(inputNotReadSemaphore, (TickType_t) 10); //Tell the main task to process input
        makeNewInput = true; //Tells the next getCommand task to make a new string
      }
      else if (index < inputLength - 1) { //adds the char to the input string
        input[index] = c;
        index++;
      }
    }
  }
  vTaskDelete(NULL); //This task is run regularly on a timer, so the current task deletes itself
}

void spawnCannonBall(Ship& theShip, bool isLeft){ //Spawns a new cannon ball into the game
  
  int portNum = isLeft ? theShip.currPortCan++ : theShip.currStarCan++; //Get and increment the gunport for right side
  if (theShip.currPortCan>=theShip.shipSizeL){theShip.currPortCan = 0;} //If you have passed the length of the Ship, use the first gunport again
  if (theShip.currStarCan>=theShip.shipSizeL){theShip.currStarCan = 0;}
  
  if (numOfBalls < maxNumBalls){ //Check that there are enough slots left for cannon balls in the cannonBallShips matrix
    
    int newXCoor = theShip.xCoor; //Start using the Ship's original coordinates
    int newYCoor = theShip.yCoor;
    newXCoor = newXCoor +portNum*theShip.angleH; //Then move out to the right gunport on the port-side of the Ship
    newYCoor = newYCoor +portNum*theShip.angleV;
    
    int newHAngle = 0;
    int newVAngle = 0;
    
    if (isLeft) {
      if (theShip.angleH== 0) newXCoor += theShip.angleV; //If the ship is in a cardinal direction move it out one dependin if it up or down
      else if (theShip.angleV== 0) newYCoor += theShip.angleH*-1; //Or if it is left or right
      else if (theShip.angleV < 0) newXCoor += -1; //If it is diagonal just move it over one to it's left side
      else if (theShip.angleV > 0) newXCoor += 1;
      
      newHAngle = theShip.angleV; //The cannon ball moves perpendicular to the Ship one way if it is port-side
      newVAngle = theShip.angleH*-1;
    } else {
      if (theShip.angleH== 0) newXCoor += -1*theShip.angleV*(theShip.shipSizeW); //These are the same (but opposite) except the cannon ball must also move past the ship's girth as well
      else if (theShip.angleV== 0) newYCoor += theShip.angleH*(theShip.shipSizeW);
      else if (theShip.angleV < 0) newXCoor += 1*(theShip.shipSizeW);
      else if (theShip.angleV > 0) newXCoor += -1*(theShip.shipSizeW);
      
      newHAngle = theShip.angleV*-1; //The cannon ball moves perpendicular to the Ship the other way if it is starboard-side
      newVAngle = theShip.angleH;
    }
    
    xSemaphoreTake(CannonArrayMutex, portMAX_DELAY);
    int openSlot = -1;//If the function got past the inital if statment, there should be a open slot
    for (int i = 0; i < maxNumBalls; ++i){ if (cannonBallShips[i].id == nullShip.id) {openSlot=i;} } //This grabs the last available slot, but that is fine
        
    cannonBallShips[openSlot]=Ship{newXCoor, newYCoor, newHAngle, newVAngle, 1, 1, 2, 4, 1, char_CannonBall,  1+numCoves+numCoves+numOfBallsCreated+1, 0, 0};
    xSemaphoreTake(NumBallsMutex, portMAX_DELAY);
    numOfBalls +=1;
    xSemaphoreGive(NumBallsMutex);
    numOfBallsCreated+=1;
    xSemaphoreGive(CannonArrayMutex);
    
  } else {
    xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
    Serial.println("Misfire");
    xSemaphoreGive(SerialOutMutex); 
  }
}

void updateAngle(Ship& theShip, int newH, int newV){ //This function attempts to update a Ship direction (angle) and checks for collisions
  //This function does not draw or print the Ship
  Ship tempShip = theShip; //Used to check if the new angle will overlap with another Ship
  crashedAndType crashed{false, false, tempShip, nullptr}; //A state holding variable for the checkCrashHelper function

  
  tempShip.angleH = newH;
  tempShip.angleV = newV;
  accessShip(tempShip, checkCrashedHelper, crashed); //Checks if the Ship's new coordinates (from the direction change) are free
  
  if (crashed.crashed){ //If there has been a crash
    
    if (&theShip==&MyShip){decMyShipHealth();} //Decrement the health of the Ship that attempted to turn
    else{decOtherShipHealth(theShip);}
    
    if (crashed.crashedMyShip) { //If the Ship that attempted to turn hit the player's Ship
      decMyShipHealth(); //Decrement the player's health
      xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
      Serial.println("You got hit");
      xSemaphoreGive(SerialOutMutex);
    }
    else if (crashed.crashedOtherShip != nullptr){ //If the Ship that attempted to turn hit some other ship
      decOtherShipHealth(*crashed.crashedOtherShip); //Decrement the health of the Ship that got hit
    }
  }else { //If the ship did not crash update the Ship's angles
    theShip.angleH = newH;
    theShip.angleV = newV;
  }
}

void processCommand(char* input, Ship& theShip){ //Takes commands (for any Ship) and attempts to modify the Ship
  int tempH = theShip.angleH;
  int tempV = theShip.angleV;
  if (strcmp(input, "down") == 0){ //Changes in direction are only allowed in a circular pattern to prevent both angles from becomeing zero
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


void checkCrashedHelper(int newX, int newY, crashedAndType& crashed){ //A helper function that checks coordinates for if another object is already on said coordinate
  if (newX >= xSize || newX <= 0 || newY >= ySize || newY <= 0) { crashed.crashed = true;} //If the coordinate is out of bounds, then that is a collision
  
  //Use if you don't want other ships to collide with each other
  //if (getSea(newX,newY) != char_SteadySea&&getSea(newX,newY) != crashed.selfShip.selfSymbol) 
  //This prevents the enemy Ships from ramming each other
  
  //Use to prevent other ships from going through each other
  if (getSea(newX,newY) != char_SteadySea&&getId(newX,newY) != crashed.selfShip.id) 
  
  {
    crashed.crashed = true; //Set that the Ship crashed
    if (getId(newX,newY) > 1+numCoves) {
      crashed.crashedOtherShip = &otherShips[getId(newX,newY)-numCoves-1-1]; //If it was a Ship, record who it was
    }
    if (getSea(newX,newY) == char_Ship) {crashed.crashedMyShip = true;} //If it was the player, that gets special attention
  }
}

void updateShips(Ship& theShip){ //Moves a Ship's coordinate's forward; it does not draw or print the Ship
  int tempX = theShip.xCoor;
  int tempY = theShip.yCoor;
  
  for (int s = 0; s < theShip.speed; ++s){ //Move more than once depending on the speed
    tempX += theShip.angleH; //Move along the Ship's angle
    tempY += theShip.angleV;

    Ship tempShip = theShip; //theShip has not actually moved yet
    tempShip.xCoor = tempX;
    tempShip.yCoor = tempY;
    crashedAndType crashed{false, false, tempShip, nullptr}; //State variable for helper function
    accessShip(tempShip, checkCrashedHelper, crashed); //Check to see if the Ship crashed
    
    if (crashed.crashed){ //If there has been a crash

      if (&theShip==&MyShip){decMyShipHealth();} //Decrement the health of the Ship that attempted to move
      else{decOtherShipHealth(theShip);}
      
      if (crashed.crashedMyShip) { //If the Ship that attempted to move hit the player's Ship
        decMyShipHealth(); //Decrement the player's health
        xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
        Serial.println("You got hit");
        xSemaphoreGive(SerialOutMutex);
      }
      if (crashed.crashedOtherShip != nullptr){ //If the Ship that attempted to move hit some other ship
        decOtherShipHealth(*crashed.crashedOtherShip); //Decrement the health of the Ship that got hit
      }
    }else { //Otherwise move the ship forward
      theShip.xCoor = tempX;
      theShip.yCoor = tempY;
    }
  }
}

void otherShipAI(Ship& theShip){ //Runs the AI for a single ship
  if (theShip.health>0){ //If the ship is alive
    Ship copyShip = theShip; //Used to clean the ship's previous location
    if (abs(MyShip.xCoor-theShip.xCoor)<AIVisionDis&&abs(MyShip.yCoor-theShip.yCoor)<AIVisionDis){ //If the Ship sees the player
      if (MyShip.xCoor-theShip.xCoor<0 && theShip.angleH!=-1&& theShip.angleV!=0){ //Then the Ship charges the player
        processCommand("left", theShip);
      } else if (MyShip.xCoor-theShip.xCoor>0 && theShip.angleH!=1&& theShip.angleV!=0){
        processCommand("right", theShip);
      } else if (MyShip.yCoor-theShip.yCoor<0 && theShip.angleV!=-1&& theShip.angleH!=0){
        processCommand("up", theShip);
      } else if (MyShip.yCoor-theShip.yCoor>0 && theShip.angleV!=1&& theShip.angleH!=0){
        processCommand("down", theShip);
      } else {processCommand("faster", theShip);} //If the Ship is on target then the Ship tries to speed up
    } else{processCommand("slower", theShip);} //If the player is out of sight the Ship slows down
    xSemaphoreTake(PrintSeaMutex, portMAX_DELAY);
    cleanFrame(copyShip); //Clean the ship's previous location
    updateShips(theShip); //Move the ship forward
    drawFrame(theShip, char_OtherShip); //Draw the ship back down onto the sea matrix
    xSemaphoreGive(PrintSeaMutex);
  } else drawFrame(theShip, char_DeadShip); //Other-wise draw a dead ship
}

void runOtherShipAIs(void *pvParameters){ //Task that runs the AI for each non-player, non-cannon ball Ship
  while(1){
    xSemaphoreTake(myShipMovedSem, portMAX_DELAY); //Wait until the player has moved agian
      for (int i = 0; i < numCoves; ++i){
        otherShipAI(otherShips[i]); //Run each Ship's AI one by one 
      }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void restartOnDelay(void* pvParameters){ //Used after the player has died; it restarts the game
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
  Serial.println("Restarting Game");
  xSemaphoreGive(SerialOutMutex);
  
  ESP.restart();
}

void makeGetCommandTask(TimerHandle_t xTimer){ //Timer handler function: makes a task that tries to fetch a command
  xTaskCreatePinnedToCore(getCommand, "get_command", 1024, NULL, 4, NULL, tskNO_AFFINITY);
}

void blinkHealthLED(void* pvParameters){ //Task that gives player health feedback via ESP32's LED's flashing's speed
  while(1){
    if(getMyShipHealth()>critHealth) { //If the player's Ship has more than a certian threashold of health
      digitalWrite(led_pin, HIGH); //Blink long and slow
      vTaskDelay((getMyShipHealth()*200) / portTICK_PERIOD_MS);
      digitalWrite(led_pin, LOW);
      vTaskDelay((getMyShipHealth()*500) / portTICK_PERIOD_MS);
    } else if (getMyShipHealth()<=critHealth&&getMyShipHealth()>0) { //If the player's Ship's health is critical
      xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
      Serial.println("Health Low!");
      xSemaphoreGive(SerialOutMutex);
      digitalWrite(led_pin, HIGH); //Blink fast and tense
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(led_pin, LOW);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }
}

void runCannonBalls(void *pvParameters){ //Task that updates any living cannon balls
  while(1){
    xSemaphoreTake(myShipMovedBallsSem, portMAX_DELAY); //Wait until the player's Ship has moved
    if (numOfBalls>0) { //If there are living balls
      for (int i = 0; i < maxNumBalls; ++i){
        xSemaphoreTake(CannonArrayMutex, portMAX_DELAY);
        if (cannonBallShips[i].id != nullShip.id){ //If the cannon balls are not nullShip
          if(cannonBallShips[i].health>0){ //If the ball is still alive (hasn't hit anything yet)
            xSemaphoreTake(PrintSeaMutex, portMAX_DELAY);
            cleanFrame(cannonBallShips[i]); //Clear the ball from the sea matrix
            updateShips(cannonBallShips[i]); //Move the ball forward
            drawFrame(cannonBallShips[i], char_CannonBall); //And then redraw it to the sea matrix
            xSemaphoreGive(PrintSeaMutex);
          } else { //If the ball is now dead (i.e. it hit something)
            drawFrame(cannonBallShips[i], char_SteadySea); //Clear the ball from the sea matrix; cannon ball do not leave wreaks
            xSemaphoreTake(NumBallsMutex, portMAX_DELAY);
            numOfBalls-=1; //One less ball alive
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

void setup() { //Arduino runs this function, then it runs loop()
  Serial.begin(250000); //Set serial
  vTaskDelay(1000 / portTICK_PERIOD_MS); //Wait for serial to load
  pinMode(led_pin, OUTPUT); //Set up the LED
  xSemaphoreGive(inputNotReadSemaphore); //Semaphore used to signal that input has been read, but not processed
  xSemaphoreGive(AccessSeaMutex); //Mutex used to control access to the sea matrix
  xSemaphoreGive(AccessMyShipHealthMutex); //Mutex used to control access to the player's ship's health
  xSemaphoreGive(SerialOutMutex); //Mutex used to prevent serial outputs from overlapping
  for (int i = 0; i < maxNumBalls; ++i){ cannonBallShips[i] = nullShip;} //Set all cannonballs to nullShip
  
  makeSea(); //These functions init the game board
  makeCoves();
  drawCoves();
  makeOtherShips();
  initDrawOtherShips();
  drawFrame(MyShip);

  Serial.print('<'); //Draws a border to signal new frames
  for (int i = 0; i < (int)(screenSize*2+2)*1.3; ++i){ Serial.print('=');}
  Serial.println('>');
  
  printSea(); //Prints sea (the view)
  
  Serial.println("This is a game where you control a ship and dodge islands and other ships.");
  Serial.println("Use \'up\' to turn up, \'down\' to turn down, \'left\' to turn left, \'right\' to turn right.");
  Serial.println("Use \'faster\' to speed up, and \'slower\' to slow down.");
  Serial.println("Use \'port\' to fire left, and \'star\' to fire right.");
  Serial.println("Ready...");

  //Make repeating timer for the task that gets commands
  TimerHandle_t inputTimer = xTimerCreate("make_get_command_task", 500 / portTICK_PERIOD_MS, pdTRUE, (void *)0, makeGetCommandTask);
  if (inputTimer == NULL) {
    ESP.restart();
  } else {
    xTimerStart(inputTimer, portMAX_DELAY);
  }
  //Make the task that runs the other Ships' AIs
  xTaskCreatePinnedToCore(runOtherShipAIs, "run_other_ship_AIs", 5024, NULL, 1, NULL, tskNO_AFFINITY);
  //Make the task that blinks the LED based on player's Ship's health
  xTaskCreatePinnedToCore(blinkHealthLED, "blink_health_lED", 1024, NULL, 2, NULL, tskNO_AFFINITY);
  //Make the task that runs the cannon balls
  xTaskCreatePinnedToCore(runCannonBalls, "run_cannon_balls", 5024, NULL, 2, NULL, tskNO_AFFINITY);
  vTaskDelay(10000 / portTICK_PERIOD_MS); //Halt the main task so the player can read the instructions
  Serial.println("Start!");
  vTaskDelay(500 / portTICK_PERIOD_MS);
}

void loop() {
  Serial.print('<'); //Draws a border to signal new frames
  for (int i = 0; i < (int)(screenSize*2+2)*1.3; ++i){ Serial.print('=');}
  Serial.println('>');
  if (getMyShipHealth()>0){ //If the player is not yet dead
    Ship tempShip = MyShip;
    if (uxSemaphoreGetCount(inputNotReadSemaphore)==0){ //If there is a command, then process the command
      processCommand(input, MyShip); 
      vPortFree((void*)input);
      xSemaphoreGive(inputNotReadSemaphore);
    }
    cleanFrame(tempShip); //Clear the player's Ship's previous location
    updateShips(MyShip); //Move the Ship forward
    drawFrame(MyShip); //Drawn the Ship again
  }
  
  xSemaphoreGive(myShipMovedSem); //Tell the runOtherShipAIs task that the player moved
  xSemaphoreGive(myShipMovedBallsSem); //Tell the runCannonBalls task that the player moved
  
  printSea();  //Prints sea (the view)
  //The other objects in the game are moved seperately
  
  if (getMyShipHealth()<=0){ //If the player has died
    
    xSemaphoreTake(SerialOutMutex, portMAX_DELAY);
    Serial.println("The Ship died!"); //Inform the player
    xSemaphoreGive(SerialOutMutex);
    drawFrame(MyShip, char_DeadMyShip); //Draw the player's wreak
    //Restart the game on a delay to let the other Ships move on screen for a while
    xTaskCreatePinnedToCore(restartOnDelay, "restart_on_delay", 1024, NULL, 1, NULL, tskNO_AFFINITY); 
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
