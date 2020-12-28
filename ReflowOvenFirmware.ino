#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"

struct DataPoint{
  double temp;
  int elapsedTime;
};

struct physicalCoordinates{
  int x;
  int y;
};


// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO   PB14
#define MAXCS   PB12
#define MAXCLK  PB13
#define SSR     PA8
#define GRAPH_HEIGHT 50
#define GRAPH_WIDTH  64

// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R3, /* cs=*/ PA4, /* dc=*/ PB6, /* reset=*/ PB7); 

bool   SSR_State;
int    reflowState;
int    runState;
double tempC;
double oldTempC;
double tempMargin;
double targetTemp;
double preheatTemp;
double reflowTemp;
double maxTemp;

int preheatTime;
int reflowTime;
int coolTime;
int stageStartTime;
int stageTime;

double ambientTemp;
long startingTime;
long preheatStartTime;
double preheatSlope;

double preheatEndTemp;
long preheatEndTime;
long reflowStartTime;
double reflowSlope;


struct physicalCoordinates tempNTime2Coordinates(int temp, int inputTime){
  struct physicalCoordinates coordinates;
  int maxTime = 60+preheatTime+60+reflowTime+coolTime;
  
  float graph_width = 64;
  float graph_height = 50;
  coordinates.y = (int) graph_height - ((temp - 25.00)/ceil((maxTemp-25.00)/graph_height))-1;
  coordinates.x = (int) inputTime/(ceil(maxTime/graph_width));
//  Serial.println(inputTime);
//  Serial.println(maxTime/graph_width);
//  Serial.println(ceil(maxTime/graph_width));
//  Serial.println(inputTime/ceil(maxTime/graph_width));
//  Serial.println(floor(inputTime/ceil(maxTime/graph_width)));

  return coordinates;
}

void plotSettings(){
  struct physicalCoordinates startCoords;
  struct physicalCoordinates stopCoords;
  stopCoords = tempNTime2Coordinates(preheatTemp, 60);
  u8g2.drawLine(0,GRAPH_HEIGHT-1, stopCoords.x, stopCoords.y);

  startCoords = stopCoords;
  stopCoords = tempNTime2Coordinates(preheatTemp, 60+preheatTime);
  u8g2.drawLine(startCoords.x,startCoords.y, stopCoords.x, stopCoords.y);

  startCoords = stopCoords;
  stopCoords = tempNTime2Coordinates(reflowTemp, 60+preheatTime+60);
  u8g2.drawLine(startCoords.x,startCoords.y, stopCoords.x, stopCoords.y);

  startCoords = stopCoords;
  stopCoords = tempNTime2Coordinates(reflowTemp, 60+preheatTime+60+reflowTime);
  u8g2.drawLine(startCoords.x,startCoords.y, stopCoords.x, stopCoords.y);

  startCoords = stopCoords;
  stopCoords = tempNTime2Coordinates(25, 60+preheatTime+60+reflowTime+coolTime);
  u8g2.drawLine(startCoords.x,startCoords.y, stopCoords.x, stopCoords.y);
}

void drawDisplay(){
  u8g2.clearBuffer();
  u8g2.home();

  u8g2.drawFrame(0,0,GRAPH_WIDTH,GRAPH_HEIGHT);
  plotSettings();

  
  u8g2.setCursor(0,64);
  u8g2.print("T:");
  u8g2.print(tempC);

  u8g2.setCursor(0,76);
  u8g2.print("TT:");
  u8g2.print(targetTemp);

  u8g2.setCursor(0,88);
  u8g2.print("SSR: ");
  if(SSR_State) u8g2.print("On");
  else u8g2.print("Off");

  u8g2.setCursor(0,100);
  u8g2.print("Time:");
  u8g2.print(stageTime);

  u8g2.setCursor(0,112);
  u8g2.print("Status:");
  u8g2.setCursor(0,124);
  if(reflowState == 0){
    u8g2.print("Off");
  }else if(reflowState == 1){
    u8g2.print("Heating...");
  }else if(reflowState == 2){
    u8g2.print("Preheat");
  }else if(reflowState == 3){
    u8g2.print("Heating...");
  }else if(reflowState == 4){
    u8g2.print("Reflow");
  }else if(reflowState == 5){
    u8g2.print("Cooling");
  }
  
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting!");
  // put your setup code here, to run once:
  tempC = 0.00;
  oldTempC = 0.00;
  reflowState = 0;
  runState = 1;
  tempMargin = 5.00;
  targetTemp = 0.00;

  preheatTemp = 150.00;
  preheatTime = 120;
  reflowTemp = 220.00;
  reflowTime = 60;
  coolTime = 60;
  maxTemp = 250.00;

  preheatSlope = 0;
  reflowSlope = 0;
  
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, LOW);
  SSR_State = false;
  
  delay(500);
//  Serial.println("Initializing MAX31855 TC sensor...");
  thermocouple.begin();
  
//  Serial.println("Initialising OLED...");

  u8g2.begin();
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_11_tf);

  delay(1000);
}

void loop() {
  tempC = thermocouple.readCelsius();
  if(isnan(tempC)) tempC = oldTempC;
  else oldTempC = tempC;

//  Serial.print("Temp: ");
  Serial.println(tempC);
//  Serial.println("C");
  
  if(runState == 1){
    if(reflowState == 0){
      targetTemp = preheatTemp;
      reflowState = 1;

      ambientTemp = tempC;
      startingTime = millis();
      
   
    // Heating to preheat
    }else if(reflowState == 1){
      if(tempC > targetTemp-tempMargin && tempC < targetTemp+tempMargin){
        reflowState = 2;
        stageStartTime = (int) millis()/1000;
        stageTime = 0;
        preheatStartTime = millis();
        Serial.println("");
        Serial.println(ambientTemp);
        Serial.println(preheatStartTime);
        Serial.println(startingTime);
        double timeDiff = (double) (preheatStartTime-startingTime);
        preheatSlope = (tempC-ambientTemp)/timeDiff;
        Serial.print("Preheat Slope(delta T / delta ms):");
        Serial.println(preheatSlope);
        Serial.println("");
      }
    
    // Preheat
    }else if(reflowState == 2){
      if(stageTime >= preheatTime){
        targetTemp = reflowTemp;
        reflowState = 3;
        
        preheatEndTemp = tempC;
        preheatEndTime = millis();
      }
      
    // Heating to Reflow
    }else if(reflowState == 3){
      if(tempC > targetTemp-tempMargin && tempC < targetTemp+tempMargin){
        reflowState = 4;
        stageStartTime = (int) millis()/1000;
        stageTime = 0;

        reflowStartTime = millis();
        double timeDiff = (double) (preheatEndTime-reflowStartTime);
        reflowSlope = (tempC - preheatEndTemp)/(timeDiff);
        Serial.print("Reflow Slope(delta T / delta ms):");
        Serial.println(reflowSlope);
      }

    // Relfowing
    }else if(reflowState == 4){
      if(stageTime >= reflowTime){
        targetTemp = 0;
        reflowState = 5;
      }
      
    // Cooling
    }else if(reflowState == 5){
      if(tempC < targetTemp + 25){
        runState = 0;
      }
    }
  }else{
    targetTemp = 0;
    reflowState = 0;
    stageStartTime = 0;
    stageTime = 0;
  }

  
  if(tempC < targetTemp - tempMargin){
    digitalWrite(SSR, HIGH);
    SSR_State = true;
  }else if(tempC > targetTemp + tempMargin || tempC > maxTemp){
    digitalWrite(SSR, LOW);
    SSR_State = false;
  }

//  u8g2.setPowerSave(0);
  drawDisplay();
  
  stageTime = (int) millis()/1000 - stageStartTime;
  delay(200);
//  u8g2.setPowerSave(1);
//  delay(1000);
}
