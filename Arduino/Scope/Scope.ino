#include <SPI.h>
#include <ILI_SdSpi.h>
#include <ILI_SdFatConfig.h>
#include <ILI9341_due_gText.h>
#include <ILI9341_due.h>
#include "fonts\Verdana18.h"

#define WIDTH 320
#define HEIGHT 240

#define CS 9
#define DC 8
#define RST 7

#define ANALOG 18
#define DIGITAL 5
#define DIGITALMODE 2

#define divSize 32

// Color definitions
// redundant!
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF

// ILI9341_due lib init
ILI9341_due tft = ILI9341_due(CS, DC, RST);
ILI9341_due_gText vMaxArea(&tft);
ILI9341_due_gText vMinArea(&tft);
ILI9341_due_gText timePerDivArea(&tft);
ILI9341_due_gText frequencyArea(&tft);

// buffer
byte valueBuffer[WIDTH][2];
byte vMax[2];
byte vMin[2];
unsigned long timeMessured[2];
float frequency[2];

// indexes
int valueIndex = 0;
byte bufferIndex = 0;

boolean fillScreen = false;
boolean digitalMode = true;
int digitalDelay;
float steps;

// temp
int poti = 0;
int triggerLevel = 512;

void setup()
{
  // sanity check
  delay(5000);
  
  Serial.begin(115200);
  
  pinMode(ANALOG, INPUT);
  pinMode(DIGITAL, INPUT);
  pinMode(DIGITALMODE, INPUT);
  
  // temp? signal output (pwm 50%)
  pinMode(3, OUTPUT);
  analogWrite(3, 128);
  
  setupTFT();
  
  vMax[1] = 255;
  vMin[1] = 0;
}


void loop()
{
  if (valueIndex == 0)
  {
    digitalMode = digitalRead(DIGITALMODE) == HIGH ? true : false;
 
    if (bufferIndex == 0)
    {
      bufferIndex = 1;
    }
    else
    {
      bufferIndex = 0;
    }
    
    vMax[0] = 255;
    vMin[0] = 0;
    
    if (digitalMode)
    {
      steps = WIDTH;
      digitalDelay = (1024 - poti) / 8;
    }
    else
    {
      steps = (float)(1024 - poti) / 1024 * WIDTH;
      if (steps < 2)
      {
        steps = 2;
      }
    }
    
    timeMessured[0] = micros();
  }
  if (valueIndex >= 0 && valueIndex < steps)
  {
    if (digitalMode)
    {
      valueBuffer[valueIndex][bufferIndex] = digitalRead(DIGITAL) == HIGH ? 0 : 6 * divSize;
      delayMicroseconds(digitalDelay);
    }
    else
    {
      valueBuffer[valueIndex][bufferIndex] = (6 * divSize) - ((float)analogRead(ANALOG) / 1024 * 6 * divSize);
    }
    valueIndex++;
  }
  else 
  {  
    timeMessured[0] = micros() - timeMessured[0];
    valueIndex = 0;
    updateTFT();
  }
}

void setupTFT()
{
  tft.begin();
  tft.setRotation(iliRotation270);
  tft.fillScreen(BLACK);
  
  vMaxArea.defineArea(8, (6 * divSize) + 6, WIDTH / 2, (6 * divSize) + ((HEIGHT - (6 * divSize)) / 2));
  vMaxArea.selectFont(Verdana18);
  vMaxArea.setFontLetterSpacing(2);
  vMaxArea.setFontColor(WHITE, BLACK);
  
  vMinArea.defineArea(8, (6 * divSize) + ((HEIGHT - (6 * divSize)) / 2), WIDTH / 2, HEIGHT);
  vMinArea.selectFont(Verdana18);
  vMinArea.setFontLetterSpacing(2);
  vMinArea.setFontColor(WHITE, BLACK);
  
  timePerDivArea.defineArea(WIDTH / 2, (6 * divSize) + 6, WIDTH, (6 * divSize) + ((HEIGHT - (6 * divSize)) / 2));
  timePerDivArea.selectFont(Verdana18);
  timePerDivArea.setFontLetterSpacing(2);
  timePerDivArea.setFontColor(WHITE, BLACK);
  
  frequencyArea.defineArea(WIDTH / 2, (6 * divSize) + ((HEIGHT - (6 * divSize)) / 2), WIDTH, HEIGHT);
  frequencyArea.selectFont(Verdana18);
  frequencyArea.setFontLetterSpacing(2);
  frequencyArea.setFontColor(WHITE, BLACK);

  drawGUI();
}

void drawGUI()
{
  for (byte i = 1; i < 10; i++)
  {
    tft.drawFastVLine(i * divSize, 0, 6 * divSize, WHITE);
  }
  for (byte i = 1; i <= 6; i++)
  {
    tft.drawFastHLine(0, i * divSize, WIDTH, WHITE);
  }
}

void updateTFT()
{
  if (fillScreen)
  {
    //tft.fillRect(0, 0, WIDTH, (6 * divSize) + 2, BLACK);
  }
  else
  {
    //removeGraph();
  }
  drawGUI();
  
  unsigned long stopwatch = millis();
  float stepSize = (float)WIDTH / (steps - 1);

  for (int i = 1; i < (int)steps; i++)
  {
    // temp playing around with continious waves
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex == 0 ? 1 : 0] - 1, i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex == 0 ? 1 : 0] - 1, BLACK);
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex == 0 ? 1 : 0], i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex == 0 ? 1 : 0], BLACK);
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex == 0 ? 1 : 0] + 1, i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex == 0 ? 1 : 0] + 1, BLACK);
    
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex] - 1, i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex] - 1, digitalMode ? GREEN : RED);
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex], i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex], digitalMode ? GREEN : RED);
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex] + 1, i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex] + 1, digitalMode ? GREEN : RED);
    
    // Messurements
    if (valueBuffer[i][bufferIndex] < vMax[0])
    {
      vMax[0] = valueBuffer[i][bufferIndex];
    }
    if (valueBuffer[i][bufferIndex] > vMin[0])
    {
      vMin[0] = valueBuffer[i][bufferIndex];
    }
  }
  // time it takes fillRect to clear graph
  if (millis() - stopwatch > 208)
  {
    fillScreen = true;
  }
  else
  {
    fillScreen = false;
  }
  
  byte midValue = (vMax[0] + vMin[0]) / 2;
  int periodPoints[2] = { 0, 0 };
  
  for (int i = 1; i < (int)steps; i++)
  {
    if (periodPoints[0] == 0 && valueBuffer[i - 1][bufferIndex] < midValue && valueBuffer[i][bufferIndex] >= midValue)
    {
      periodPoints[0] = i;
    }
    else if (periodPoints[0] != 0 && valueBuffer[i - 1][bufferIndex] < midValue && valueBuffer[i][bufferIndex] >= midValue)
    {
      periodPoints[1] = i;
      break;
    }
  }
  if (periodPoints[0] != 0 && periodPoints[1] != 0)
  {  
    frequency[0] = 1 / ((float)timeMessured[0] / floor(steps) * (periodPoints[1] - periodPoints[0]) * 0.000001);
  }
  else
  {
    frequency[0] = 0;
  }
  
  char tempString[20];
  char tempValue[10];

  if (vMax[0] != vMax[1])
  {
    vMax[1] = vMax[0];
    ftoa(tempValue, (float)((6 * divSize) - vMax[0]) / (6 * divSize) * 5, 2);
    sprintf(tempString, "Vmax: %sV", tempValue);
    vMaxArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  if (vMin[0] != vMin[1])
  {
    vMin[1] = vMin[0];
    ftoa(tempValue, (float)((6 * divSize) - vMin[0]) / (6 * divSize) * 5, 2);
    sprintf(tempString, "Vmin: %sV", tempValue);
    vMinArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  if ((timeMessured[0] >= 10000 ? timeMessured[0] / 100 : timeMessured[0]) != (timeMessured[1] >= 10000 ? timeMessured[1] / 100 : timeMessured[1]))
  {
    timeMessured[1] = timeMessured[0];
    ftoa(tempValue, timeMessured[0] >= 10000 ? (float)timeMessured[0] / 10000 : (float)timeMessured[0] / 10, 2);
    sprintf(tempString, timeMessured[0] >= 10000 ? "dt: %sms/Div" : "%sus/Div", tempValue);
    timePerDivArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  if (frequency[0] != frequency[1])
  {
    frequency[1] = frequency[0];
    sprintf(tempString, "f: %dHz", (int)frequency[0]);
    frequencyArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  
  delay(1);
}

void removeGraph()
{
  float stepSize = (float)WIDTH / (steps - 1);
  for (int i = 1; i < (int)steps; i++)
  {
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex == 0 ? 1 : 0] - 1, i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex == 0 ? 1 : 0] - 1, BLACK);
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex == 0 ? 1 : 0], i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex == 0 ? 1 : 0], BLACK);
    tft.drawLine((i * stepSize) - stepSize, valueBuffer[i - 1][bufferIndex == 0 ? 1 : 0] + 1, i == floor(steps - 1) ? WIDTH : i * stepSize, valueBuffer[i][bufferIndex == 0 ? 1 : 0] + 1, BLACK);
  }
}

char *ftoa(char *a, double f, int precision)
{
 long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
 char *ret = a;
 long heiltal = (long)f;
 itoa(heiltal, a, 10);
 while (*a != '\0') a++;
 *a++ = '.';
 long desimal = abs((long)((f - heiltal) * p[precision]));
 itoa(desimal, a, 10);
 return ret;
}
