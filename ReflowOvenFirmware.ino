#include <ReflowOvenFirmware.h>


// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R3, /* cs=*/ PA4, /* dc=*/ PB6, /* reset=*/ PB7); 

#define KP 90.0 //90
#define KI 6.0 //6
#define KD 20.0 //20

AutoPID ovenPID(&tempC, &targetTemp, &outputPWM, 0, 4095, KP, KI, KD);

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
  u8g2.print("SSR:");
  u8g2.print((outputPWM/4095)*100);
  // if(SSR_State) u8g2.print("On");
  // else u8g2.print("Off");

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

void btnpress(){
  btnPressed = true;
}

void setup() {
  Serial.begin(9600);
  // Serial.println("Starting!");
  // put your setup code here, to run once:
  tempC = 0.00;
  oldTempC = 0.00;
  reflowState = 0;
  runState = 1;
  tempMargin = 10.00;
  targetTemp = 0.00;
  outputPWM = 0.00;

  preheatTemp = 150.00;
  preheatTime = 120;
  reflowTemp = 220.00;
  reflowTime = 60;
  coolTime = 60;
  maxTemp = 250.00;

  preheatSlope = 0;
  reflowSlope = 0;

  btnPressed = false;
  
  analogWriteResolution(12);
  analogWriteFrequency(500);
  ovenPID.setTimeStep(200);
  ovenPID.setBangBang(tempMargin);
  pinMode(SSR, OUTPUT);

  pinMode(PB11, OUTPUT);
  pinMode(PB10, OUTPUT);
  pinMode(PB1, OUTPUT);

  analogWrite(SSR, (int) round(outputPWM));
  SSR_State = false;
  
  pinMode(PB5, INPUT);
  attachInterrupt(digitalPinToInterrupt(PB5), btnpress, RISING);

  pinMode(PB4, INPUT);
  attachInterrupt(digitalPinToInterrupt(PB4), btnpress, RISING);

  pinMode(PB3, INPUT);
  attachInterrupt(digitalPinToInterrupt(PB3), btnpress, RISING);

  pinMode(PA15, INPUT);
  attachInterrupt(digitalPinToInterrupt(PA15), btnpress, RISING);

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
  // Serial.println(tempC);
//  Serial.println("C");
  
  if(btnPressed){
    Serial.println("Button Pressed!");
    digitalWrite(PB11, HIGH);
    digitalWrite(PB10, HIGH);
    digitalWrite(PB1, HIGH);
    btnPressed = false;
  }else{
    digitalWrite(PB11, LOW);
    digitalWrite(PB10, LOW);
    digitalWrite(PB1, LOW);
  }

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
        // Serial.println("");
        // Serial.println(ambientTemp);
        // Serial.println(preheatStartTime);
        // Serial.println(startingTime);
        double timeDiff = (double) (preheatStartTime-startingTime);
        preheatSlope = (tempC-ambientTemp)/timeDiff;
        // Serial.print("Preheat Slope(delta T / delta ms):");
        // Serial.println(preheatSlope);
        // Serial.println("");
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
        // Serial.print("Reflow Slope(delta T / delta ms):");
        // Serial.println(reflowSlope);
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

  ovenPID.run();
  analogWrite(SSR, (int) round(outputPWM));
  Serial.print(runState);
  Serial.print(",");
  Serial.print(tempC);
  Serial.print(",");
  Serial.print(targetTemp);
  Serial.print(",");
  Serial.print(outputPWM);
  Serial.println("");

  // if(round(outputPWM) < 5){
  //   SSR_State = false;
  // }else{
  //   SSR_State = true;
  // }

  // if(tempC < targetTemp - tempMargin){
  //   analogWrite(SSR, HIGH);
  //   SSR_State = true;
  // }else if(tempC > targetTemp + tempMargin || tempC > maxTemp){
  //   digitalWrite(SSR, LOW);
  //   SSR_State = false;
  // }

//  u8g2.setPowerSave(0);
  drawDisplay();
  
  stageTime = (int) millis()/1000 - stageStartTime;
  delay(200);
//  u8g2.setPowerSave(1);
//  delay(1000);
}
