#ifndef ReflowOvenFirmware
#define ReflowOvenFirmware

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "AutoPID.h"

// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO   PB14
#define MAXCS   PB12
#define MAXCLK  PB13
#define SSR     PA8
#define GRAPH_HEIGHT 50
#define GRAPH_WIDTH  64

struct DataPoint{
  double temp;
  int elapsedTime;
};

struct physicalCoordinates{
  int x;
  int y;
};

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
double outputPWM;

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

volatile bool btnPressed;


#endif