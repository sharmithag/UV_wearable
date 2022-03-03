
#include <TFT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_TCS34725.h>
#include "RTClib.h"
#include <EEPROM.h>

RTC_DS1307 rtc;

#define cs 10
#define dc 9
#define rst 8

int addrem = 0;
int addrmo = 1;
int addrno = 2;
int addrev = 3;
int addrni = 4;

TFT TFTscreen = TFT(cs, dc, rst);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

//Hardware pin definitions
int UVOUT = A0; //Output from the sensor
int REF_3V3 = A1; //3.3V power on the Arduino board
uint16_t r, g, b, c, colorTemp, lux;
int btnpin = 2;
char timePrintout[6];
bool btnpre = false;
int state = 0;
int laststate = 1;
float maxUV = 0; //Max UV index read
byte valee;
float fvalee;


void setup()
{
  Serial.begin(9600);
  mlx.begin();
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
  }
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  Serial.println("UV Watch");
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.setTextSize(2);
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.println();
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  pinMode(btnpin, INPUT);
  //attachInterrupt(digitalPinToInterrupt(btnpin), blink, RISING);
}

void loop()
{
  if (digitalRead(btnpin) == HIGH) {
    btnpre = true;
  }
  if (btnpre) {
    btnpre = false;
    if (state == 0) {
      state = 1;
      Serial.println("111111");
    } else if (state == 1) {
      state = 2;
      Serial.println("222222");
    } else if (state == 2) {
      state = 0;
      Serial.println("000000");
    }
  }
  if (laststate != state) {
    if (state == 0) {
      laststate = state;

      tcs.getRawData(&r, &g, &b, &c);
      colorTemp = tcs.calculateColorTemperature(r, g, b);
      lux = tcs.calculateLux(r, g, b);

      Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
      Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");

      Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
      Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
      Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
      Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
      Serial.println(" ");

      int redRandom = random(0, 255);
      int greenRandom = random (0, 255);
      int blueRandom = random (0, 255);
      TFTscreen.background(0, 0, 0);
      TFTscreen.stroke(redRandom, greenRandom, blueRandom);

      int uvLevel = averageAnalogRead(UVOUT);
      int refLevel = averageAnalogRead(REF_3V3);

      //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
      float outputVoltage = 3.3 / refLevel * uvLevel;
      float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level

      if (maxUV < uvIntensity) {
        maxUV = uvIntensity;
      }

      byte categ = (byte)(maxUV + 0.5); //round up
      if ((categ >= 0) && (categ < 3)) {
        Serial.println("Index: ");
        Serial.print(categ);
        Serial.println("LOW");
      } else if ((categ >= 3) && (categ < 6)) {
        Serial.println("Index: ");
        Serial.print(categ);
        Serial.println("MODERATE");
      } else if ((categ >= 6) && (categ < 8)) {
        Serial.println("Index: ");
        Serial.print(categ);
        Serial.println("HIGH");
      } else if ((categ >= 8) && (categ < 10)) {
        Serial.println("Index: ");
        Serial.print(categ);
        Serial.println("VERY HIGH");
      } else if (categ >= 11) {
        Serial.println("Index: ");
        Serial.print(categ);
        Serial.println("EXTREME");
      }
      Serial.print("output: ");
      Serial.print(refLevel);

      Serial.print("ML8511 output: ");
      Serial.print(uvLevel);

      Serial.print(" / ML8511 voltage: ");
      Serial.print(outputVoltage);

      Serial.print(" / UV Intensity (mW/cm^2): ");
      Serial.print(uvIntensity);

      Serial.println();

      DateTime now = rtc.now();
      String str = String(now.hour(), DEC) + ':' + String(now.minute(), DEC);
      str.toCharArray(timePrintout, 6);
      TFTscreen.text(timePrintout, 6, 17);
      if (now.hour() >= 5 && now.hour() < 9) {
        Serial.println("Early Morning");
        TFTscreen.text("EM", 86, 17);
        EEPROM.write(addrem, categ);
      } else if (now.hour() >= 9 && now.hour() < 12) {
        Serial.println("Morning");
        TFTscreen.text("M", 86, 17);
        EEPROM.write(addrmo, categ);
      } else if (now.hour() >= 12 && now.hour() < 15) {
        Serial.println("Noon");
        TFTscreen.text("N", 86, 17);
        EEPROM.write(addrno, categ);
      } else if (now.hour() >= 15 && now.hour() < 18) {
        Serial.println("Evening");
        TFTscreen.text("E", 86, 17);
        EEPROM.write(addrev, categ);
      } else if (now.hour() >= 18) {
        Serial.println("Night");
        TFTscreen.text("NI ", 86, 17);
        EEPROM.write(addrni, categ);
      }

      char valueText[8]; //enough space for "-XXX.XX" + 1 for the null terminator
      char valueText1[8];
      char valueText2[8];

      float fcateg = (float)categ;
      Serial.println("FCateg:");
      Serial.println(fcateg);
      dtostrf(fcateg, 1, 2, valueText);
      TFTscreen.text("UV Ind: ", 6, 37);
      TFTscreen.text(valueText, 86, 37);

      Serial.print("Ambient: ");
      float ambtemp = mlx.readAmbientTempC();
      Serial.print(ambtemp);
      dtostrf(ambtemp, 1, 2, valueText1);
      TFTscreen.text("AmbTem: ", 6, 57);
      TFTscreen.text(valueText1, 86, 57);

      Serial.print(" C");
      Serial.print("Target:  ");
      float realtemp = mlx.readObjectTempC();
      Serial.print(realtemp);
      dtostrf(realtemp, 1, 2, valueText2);
      TFTscreen.text("RealTe: ", 6, 77);
      TFTscreen.text(valueText2, 86, 77);

      TFTscreen.text("Tan: ", 6, 97);
      if (r > 0 && r < 100) {
        Serial.println("Tan 4");
        TFTscreen.text("Tan 4", 86, 97);
      } else if (r > 100 && r < 300) {
        Serial.println("Tan 3");
        TFTscreen.text("Tan 3", 86, 97);
      } else if (r > 300 && r < 450) {
        Serial.println("Tan 2");
        TFTscreen.text("Tan 2", 86, 97);
      } else if (r > 450 && r < 1400) {
        Serial.println("Tan 1");
        TFTscreen.text("Tan 1", 86, 97);
      } else if (r > 1400 && r < 2056) {
        Serial.println("Tan 0");
        TFTscreen.text("Tan 0", 86, 97);
      }




      Serial.print(" C");
      Serial.println();
      delay(2000);
    }
    else if (state == 1) {
      laststate = state;
      Serial.println("Display 2");
      int redRandom = random(0, 255);
      int greenRandom = random (0, 255);
      int blueRandom = random (0, 255);
      TFTscreen.background(0, 0, 0);
      TFTscreen.stroke(redRandom, greenRandom, blueRandom);
      TFTscreen.text("EMOR: ", 6, 17);
      valee = EEPROM.read(addrem);
      fvalee = (float)valee;
      char cvalee[8];
      dtostrf(fvalee, 1, 2, cvalee);
      TFTscreen.text(cvalee, 86, 17);

      TFTscreen.text("MORN: ", 6, 37);
      valee = EEPROM.read(addrmo);
      fvalee = (float)valee;
      char cvalee1[8];
      dtostrf(fvalee, 1, 2, cvalee1);
      TFTscreen.text(cvalee1, 86, 37);

      TFTscreen.text("NOON: ", 6, 57);
      valee = EEPROM.read(addrno);
      fvalee = (float)valee;
      char cvalee2[8];
      dtostrf(fvalee, 1, 2, cvalee2);
      TFTscreen.text(cvalee2, 86, 57);

      TFTscreen.text("EVEN: ", 6, 77);
      valee = EEPROM.read(addrev);
      fvalee = (float)valee;
      char cvalee3[8];
      dtostrf(fvalee, 1, 2, cvalee3);
      TFTscreen.text(cvalee3, 86, 77);

      TFTscreen.text("NIGH: ", 6, 97);
      valee = EEPROM.read(addrni);
      fvalee = (float)valee;
      char cvalee4[8];
      dtostrf(fvalee, 1, 2, cvalee4);
      TFTscreen.text(cvalee4, 86, 97);
      delay(2000);
    } else if (state == 2) {
      laststate = state;
      int redRandom = random(0, 255);
      int greenRandom = random (0, 255);
      int blueRandom = random (0, 255);
      TFTscreen.background(0, 0, 0);
      TFTscreen.stroke(redRandom, greenRandom, blueRandom);
      TFTscreen.text("0-2: ", 6, 17);
      TFTscreen.text("SunGlass", 56, 17);
      TFTscreen.text("3-5: ", 6, 37);
      TFTscreen.text("SPF30", 56, 37);
      TFTscreen.text("6-7: ", 6, 57);
      TFTscreen.text("Hat&SPF50", 56, 57);
      TFTscreen.text("7-10: ", 6, 77);
      TFTscreen.text("Shade", 56, 77);
      TFTscreen.text("11+: ", 6, 97);
      TFTscreen.text("Indoor", 56, 97);
      delay(2000);
    }
  }


}

//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for (int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return (runningValue);
}

//The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
