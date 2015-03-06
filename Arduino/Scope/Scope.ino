#include <SPI.h>
#include <ILI_SdSpi.h>
#include <ILI_SdFatConfig.h>
#include <ILI9341_due_gText.h>
#include <ILI9341_due.h>
#include "fonts\Verdana18.h"
#include "floatToString.h"

// Pins
#define CS 8
#define DC 10
#define RST 9
#define ANALOG 14
#define DIGITAL 6
#define DIGITAL_MODE 7
#define PWM_SIGNAL 3

// Display
#define WIDTH 320
#define HEIGHT 240

// Settings
#define DIV_SIZE 32

// Colors
#define COLOR_BG 0x0000
#define COLOR_TEXT 0xFFFF
#define COLOR_DIV 0x7BEF
#define COLOR_ANALOG 0xF800
#define COLOR_DIGITAL 0x07E0

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
float steps[2];

// indexes
int valueIndex = 0;
byte bufferIndex = 0;

boolean digitalMode = true;
int digitalDelay;

// temp
int timeAxis = 0;
byte triggerLevel = 172;
bool triggerEnabled = true;
bool triggerMode = true; // true = raising edge, false = falling edge
bool triggered = false;

void setup()
{
  // sanity check
  delay(5000);
    
  Serial.begin(115200);
  
  pinMode(ANALOG, INPUT);
  pinMode(DIGITAL, INPUT);
  pinMode(DIGITAL_MODE, INPUT);
  pinMode(PWM_SIGNAL, OUTPUT);
    
  // Boost ADC conversion time from 9.6kS/s vs. 76.9kS/s (without overhead)
  bitClear(ADCSRA,ADPS0); 
  bitSet(ADCSRA,ADPS1); 
  bitClear(ADCSRA,ADPS2);
  
  setupTFT();
  
  analogWrite(PWM_SIGNAL, 128);
}


void loop()
{
  if (valueIndex == 0)
  {
    // Digital Inputs (Switches)
    digitalMode = digitalRead(DIGITAL_MODE) == HIGH ? true : false;
    //triggerEnabled = digitalRead(TRIGGER_ENABLED) == HIGH ? true : false;
    //triggerMode = digitalRead(TRIGGER_MODE) == HIGH ? true : false;
    
    // Analog Inputs (Potentiometers)
    //timeAxis = analogRead(TIME_AXIS);
    //triggerLevel = (6 * DIV_SIZE) - ((float)analogRead(TRIGGER_LEVEL) / 1024 * 6 * DIV_SIZE);
    
    // Switch buffer
    bufferIndex = bufferIndex == 0 ? 1 : 0;
    
    // Calculate the time axis
    if (digitalMode)
    {
      steps[bufferIndex] = WIDTH;
      digitalDelay = (1024 - timeAxis) / 8;
    }
    else
    {
      steps[bufferIndex] = (float)(1024 - timeAxis) / 1024 * WIDTH;
      if (steps[bufferIndex] < 2)
      {
        steps[bufferIndex] = 2;
      }
    }
    
    // Reset trigger
    triggered = false;
    
    // Reset messurement values
    vMax[0] = 255;
    vMin[0] = 0;
    timeMessured[0] = micros();
  }
  if (valueIndex >= 0 && valueIndex < steps[bufferIndex])
  {
    if (digitalMode)
    {
      valueBuffer[valueIndex][bufferIndex] = digitalRead(DIGITAL) == HIGH ? 0 : 6 * DIV_SIZE;
      delayMicroseconds(digitalDelay);
    }
    else
    {     
      valueBuffer[valueIndex][bufferIndex] = (6 * DIV_SIZE) - ((unsigned long)analogRead(ANALOG) * 6 * DIV_SIZE / 1024);

      if ((triggerEnabled && !triggered && valueIndex > 0) && ((triggerMode && valueBuffer[valueIndex - 1][bufferIndex] > triggerLevel && valueBuffer[valueIndex][bufferIndex] <= triggerLevel) || !triggerMode && valueBuffer[valueIndex - 1][bufferIndex] < triggerLevel && valueBuffer[valueIndex][bufferIndex] >= triggerLevel))
      {
        // We triggered!
        // Use current value as first value
        valueBuffer[0][bufferIndex] = valueBuffer[valueIndex][bufferIndex];
        // Reset some vars
        valueIndex = 0;
        timeMessured[0] = micros();
        // Making sure this runthrough wont get triggered again
        triggered = true;
      }
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

  vMaxArea.defineArea(8, (6 * DIV_SIZE) + 6, WIDTH / 2, (6 * DIV_SIZE) + ((HEIGHT - (6 * DIV_SIZE)) / 2));
  vMaxArea.selectFont(Verdana18);
  vMaxArea.setFontLetterSpacing(2);
  vMaxArea.setFontColor(COLOR_TEXT, COLOR_BG);
  
  vMinArea.defineArea(8, (6 * DIV_SIZE) + ((HEIGHT - (6 * DIV_SIZE)) / 2), WIDTH / 2, HEIGHT);
  vMinArea.selectFont(Verdana18);
  vMinArea.setFontLetterSpacing(2);
  vMinArea.setFontColor(COLOR_TEXT, COLOR_BG);
  
  timePerDivArea.defineArea(WIDTH / 2, (6 * DIV_SIZE) + 6, WIDTH, (6 * DIV_SIZE) + ((HEIGHT - (6 * DIV_SIZE)) / 2));
  timePerDivArea.selectFont(Verdana18);
  timePerDivArea.setFontLetterSpacing(2);
  timePerDivArea.setFontColor(COLOR_TEXT, COLOR_BG);
  
  frequencyArea.defineArea(WIDTH / 2, (6 * DIV_SIZE) + ((HEIGHT - (6 * DIV_SIZE)) / 2), WIDTH, HEIGHT);
  frequencyArea.selectFont(Verdana18);
  frequencyArea.setFontLetterSpacing(2);
  frequencyArea.setFontColor(COLOR_TEXT, COLOR_BG);

  tft.fillScreen(COLOR_BG);
  for (byte i = 1; i < 10; i++)
  {
    tft.drawFastVLine(i * DIV_SIZE, 0, 6 * DIV_SIZE, COLOR_DIV);
  }
  for (byte i = 1; i <= 6; i++)
  {
    tft.drawFastHLine(0, i * DIV_SIZE, WIDTH, COLOR_DIV);
  }
}

void updateTFT()
{
  byte oldBufferIndex = bufferIndex == 0 ? 1 : 0;
  float stepSize[2];
  stepSize[bufferIndex] = (float)WIDTH / (steps[bufferIndex] - 1);
  stepSize[oldBufferIndex] = (float)WIDTH / (steps[oldBufferIndex] - 1);
  int x2 = 1;
  
  for (int x1 = 1; x1 <= (int)max(steps[0], steps[1]); x1++)
  {
    if (x1 < steps[bufferIndex == 0 ? 1 : 0])
    {
      // Remove old graph
      tft.drawLine((x1 * stepSize[oldBufferIndex]) - stepSize[oldBufferIndex], valueBuffer[x1 - 1][oldBufferIndex] - 1, x1 == floor(steps[oldBufferIndex] - 1) ? WIDTH : x1 * stepSize[oldBufferIndex], valueBuffer[x1][oldBufferIndex] - 1, COLOR_BG);
      tft.drawLine((x1 * stepSize[oldBufferIndex]) - stepSize[oldBufferIndex], valueBuffer[x1 - 1][oldBufferIndex], x1 == floor(steps[oldBufferIndex] - 1) ? WIDTH : x1 * stepSize[oldBufferIndex], valueBuffer[x1][oldBufferIndex], COLOR_BG);
      tft.drawLine((x1 * stepSize[oldBufferIndex]) - stepSize[oldBufferIndex], valueBuffer[x1 - 1][oldBufferIndex] + 1, x1 == floor(steps[oldBufferIndex] - 1) ? WIDTH : x1 * stepSize[oldBufferIndex], valueBuffer[x1][oldBufferIndex] + 1, COLOR_BG);

      // Draw vertical division line dynamically
      for (int i = (x1 * stepSize[oldBufferIndex]) - stepSize[oldBufferIndex]; i < x1 * stepSize[oldBufferIndex]; i++)
      {
        if (i % DIV_SIZE == 0 && i != 0)
        {
          byte s = max(0, min(valueBuffer[x1 - 1][oldBufferIndex], valueBuffer[x1][oldBufferIndex]) - 1);
          byte h = abs(valueBuffer[x1 - 1][oldBufferIndex] - valueBuffer[x1][oldBufferIndex]) + 3;
          if (s + h > 6 * DIV_SIZE)
          {
            h -= s + h - (6 * DIV_SIZE);
          }
          tft.drawFastVLine(i, s, h, COLOR_DIV);
        }
      }

      // Draw horizontal division lines dynamically
      byte s = min(valueBuffer[x1 - 1][oldBufferIndex], valueBuffer[x1][oldBufferIndex]);
      byte d = abs(valueBuffer[x1 - 1][oldBufferIndex] - valueBuffer[x1][oldBufferIndex]);

      for (int i = s - 1; i < s + d + 3; i++)
      {
        if (i % DIV_SIZE == 0 && i != 0)
        {
          tft.drawFastHLine((x1 * stepSize[oldBufferIndex]) - stepSize[oldBufferIndex] - 1, i, stepSize[oldBufferIndex] + 3, COLOR_DIV);
        }
      }
    }

    if ((stepSize[bufferIndex] <= stepSize[oldBufferIndex] || (int)(x2 * stepSize[bufferIndex]) < (int)(x1 * stepSize[oldBufferIndex])) && x2 < (int)steps[bufferIndex])
    {
      // Draw new graph
      tft.drawLine((x2 * stepSize[bufferIndex]) - stepSize[bufferIndex], max(valueBuffer[x2 - 1][bufferIndex] - 1, 0), x2 == floor(steps[bufferIndex] - 1) ? WIDTH : x2 * stepSize[bufferIndex], max(valueBuffer[x2][bufferIndex] - 1, 0), digitalMode ? COLOR_DIGITAL : COLOR_ANALOG);
      tft.drawLine((x2 * stepSize[bufferIndex]) - stepSize[bufferIndex], valueBuffer[x2 - 1][bufferIndex], x2 == floor(steps[bufferIndex] - 1) ? WIDTH : x2 * stepSize[bufferIndex], valueBuffer[x2][bufferIndex], digitalMode ? COLOR_DIGITAL : COLOR_ANALOG);
      tft.drawLine((x2 * stepSize[bufferIndex]) - stepSize[bufferIndex], min(valueBuffer[x2 - 1][bufferIndex] + 1, 6 * DIV_SIZE), x2 == floor(steps[bufferIndex] - 1) ? WIDTH : x2 * stepSize[bufferIndex], min(valueBuffer[x2][bufferIndex] + 1, 6 * DIV_SIZE), digitalMode ? COLOR_DIGITAL : COLOR_ANALOG);

      // Messurements
      if (valueBuffer[x2][bufferIndex] < vMax[0])
      {
        vMax[0] = valueBuffer[x2][bufferIndex];
      }
      if (valueBuffer[x2][bufferIndex] > vMin[0])
      {
        vMin[0] = valueBuffer[x2][bufferIndex];
      }
      x2++;
    }
  }

  byte midValue = (vMax[0] + vMin[0]) / 2;
  int periodPoints[2] = { 0, 0 };
  
  for (int i = 1; i < (int)steps[bufferIndex]; i++)
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
    frequency[0] = 1 / ((float)timeMessured[0] / floor(steps[bufferIndex]) * (periodPoints[1] - periodPoints[0]) * 0.000001);
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
    floatToString(tempValue, (float)((6 * DIV_SIZE) - vMax[0]) / (6 * DIV_SIZE) * 5, 2);
    sprintf(tempString, "Vmax: %sV", tempValue);
    vMaxArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  if (vMin[0] != vMin[1])
  {
    vMin[1] = vMin[0];
    floatToString(tempValue, (float)((6 * DIV_SIZE) - vMin[0]) / (6 * DIV_SIZE) * 5, 2);
    sprintf(tempString, "Vmin: %sV", tempValue);
    vMinArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  if ((timeMessured[0] >= 10000 ? timeMessured[0] / 100 : timeMessured[0]) != (timeMessured[1] >= 10000 ? timeMessured[1] / 100 : timeMessured[1]))
  {
    timeMessured[1] = timeMessured[0];
    floatToString(tempValue, timeMessured[0] >= 10000 ? (float)timeMessured[0] / 10000 : (float)timeMessured[0] / 10, 2);
    sprintf(tempString, timeMessured[0] >= 10000 ? "t/Div: %sms" : "t/Div: %sus", tempValue);
    timePerDivArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
  if (frequency[0] != frequency[1])
  {
    frequency[1] = frequency[0];
    sprintf(tempString, "f: %dHz", (int)frequency[0]);
    frequencyArea.drawString(tempString, gTextAlignMiddleLeft, gTextEraseFullLine);
  }
}
