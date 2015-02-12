#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

#define WIDTH 320
#define HEIGHT 240

#define CS 9
#define DC 8
#define RST 7

#define ANALOG 18
#define DIGITAL 5
#define DIGITALMODE 2

#define divSize 32

Adafruit_ILI9340 tft = Adafruit_ILI9340(CS, DC, RST);

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF


byte valueBuffer[WIDTH][2];
int valueIndex = 0;
byte bufferIndex = 0;
byte vMax[2];
byte vMin[2];
unsigned long timeMessured[2];
float frequency[2];
boolean fillScreen = false;

int digitalDelay;

int poti = 0;
int triggerLevel = 512;
boolean digitalMode = true;


float steps;


void setup()
{
  vMax[1] = 255;
  vMin[1] = 0;
  // sanity check
  delay(5000);
  Serial.begin(115200);
  pinMode(ANALOG, INPUT);
  pinMode(DIGITAL, INPUT);
  pinMode(DIGITALMODE, INPUT);
  pinMode(3, OUTPUT);
  analogWrite(3, 128);
  setupTFT();
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
  tft.setRotation(3);
  tft.fillScreen(BLACK);
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
    tft.fillRect(0, 0, WIDTH, (6 * divSize) + 2, BLACK);
  }
  else
  {
    removeGraph();
  }
  drawGUI();
  
  unsigned long stopwatch = millis();
  float stepSize = (float)WIDTH / (steps - 1);

  for (int i = 1; i < (int)steps; i++)
  {
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
  if (millis() - stopwatch >= 1000)
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
      
  removeText();
  
  if (vMax[0] != vMax[1] && !digitalMode)
  {
    vMax[1] = vMax[0];
    writeMessure("Vmax: ", (float)((6 * divSize) - vMax[0]) / (6 * divSize) * 5, "V", 8, (6 * divSize) + 8, WHITE);
  }
  if (vMin[0] != vMin[1] && !digitalMode)
  {
    vMin[1] = vMin[0];
    writeMessure("Vmin: ", (float)((6 * divSize) - vMin[0]) / (6 * divSize) * 5, "V", 8, (6 * divSize) + 28, WHITE);
  }
  if ((timeMessured[0] >= 10000 ? timeMessured[0] / 100 : timeMessured[0]) != (timeMessured[1] >= 10000 ? timeMessured[1] / 100 : timeMessured[1]))
  {
    timeMessured[1] = timeMessured[0];
    writeMessure("", timeMessured[0] >= 10000 ? (float)timeMessured[0] / 10000 : (float)timeMessured[0] / 10, timeMessured[0] >= 10000 ? "mS/Div" : "uS/Div", 168, (6 * divSize) + 8, WHITE);
  }
  if (frequency[0] != frequency[1])
  {
    frequency[1] = frequency[0];
    writeMessure("f: ", frequency[0], "Hz", 168, (6 * divSize) + 28, WHITE);
  }
  
  delay(1000);
}

void writeMessure(String prefix, float value, String suffix, byte x, byte y, int color)
{
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  prefix += value;
  prefix += suffix;
  tft.println(prefix);
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

void removeText()
{
  if (vMax[0] != vMax[1] || digitalMode)
  {
    writeMessure("Vmax: ", (float)((6 * divSize) - vMax[1]) / (6 * divSize) * 5, "V", 8, (6 * divSize) + 8, BLACK);
  }
  if (vMin[0] != vMin[1] || digitalMode)
  {
    writeMessure("Vmin: ", (float)((6 * divSize) - vMin[1]) / (6 * divSize) * 5, "V", 8, (6 * divSize) + 28, BLACK);
  }
  if ((timeMessured[0] >= 10000 ? timeMessured[0] / 100 : timeMessured[0]) != (timeMessured[1] >= 10000 ? timeMessured[1] / 100 : timeMessured[1]))
  {
    writeMessure("", timeMessured[1] >= 10000 ? (float)timeMessured[1] / 10000 : (float)timeMessured[1] / 10, timeMessured[1] >= 10000 ? "mS/DIV" : "uS/Div", 168, (6 * divSize) + 8, BLACK);
  }
  if (frequency[0] != frequency[1])
  {
    writeMessure("f: ", frequency[1], "Hz", 168, (6 * divSize) + 28, BLACK);
  }
}
