// M1
#include <EEPROM.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include <Adafruit_ADS1X15.h>
Adafruit_ADS1115 ads;
Adafruit_ADS1115 ads2; /* Use this for the 16-bit version */

// SD Card
#include "FS.h"
#include "SD.h"
#include "SPI.h"

const int txPin = 17;
const int rxPin = 16;

unsigned long previousMillis = 0;
const unsigned long interval = 5000; // 1 second old was 1100
unsigned long currentMillis;

unsigned long SDCardpreviousMillis = 0;
const unsigned long SDCardinterval = 60000; // 1 second

unsigned long RestartpreviousMillis = 0;        // will store last time the code was executed
const unsigned long Restartinterval = 21600000; // 3600000interval at which to execute the code (1 hour in milliseconds) 6hrs
unsigned long RestartcurrentMillis = 0;

const float threshold = 0.01; // Precision threshold for floating-point comparison

const char filename[] = "/AX306_SD.csv";
const String title = "Boot Event\n";

const int totalChannels = 6;
float volts[totalChannels + 1];

String dateTime;

struct Address
{
  int MIN;
  int MAX;
  int FAC;
  int CAL;
};

struct Parameters
{
  float MIN;
  float MAX;
  float FAC;
  float CAL;
};

struct Data
{
  Address addr;
  Parameters param;
};

Data dataArray[totalChannels];

DynamicJsonDocument doc2(2048); // Increase size if needed to accommodate additional data

void performOperationAndSend();
float mapFloat(float I, float Ilow, float Ihigh, float Pvlow, float Pvhigh);
void updateParams(String input);
void read_adc1();
void read_adc2();
void printAnalogData();
void printParams();
void updateParams(String input);
void processAndAppendData(fs::FS &fs, DynamicJsonDocument &doc2);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void readEEPROM();

void setup()
{
  // Start the hardware serial port
  Serial2.begin(115200, SERIAL_8N1, rxPin, txPin); // RX on IO16, TX on IO17
  Serial.begin(115200);                            // For debugging purposes
  EEPROM.begin(512);                               // Initialize EEPROM with 512 bytes

  readEEPROM();

  delay(18000);

  // Example of receiving new data over UART
  if (Serial.available() > 0)
  {
    String input = Serial.readString();
    updateParams(input);
  }

  ads.setGain(GAIN_ONE);  // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads2.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV

  for (int j = 0; j < 5; j++)
  {
    if (!ads.begin())
    {
      Serial.println("Failed to initialize ADS-1.");
      delay(1000);
    }
    else
      Serial.println("Succesfully initialized ADS-1.");

    if (!ads2.begin(0x49))
    {
      Serial.println("Failed to initialize ADS-2.");
      delay(1000);
    }
    else
      Serial.println("Succesfully initialized ADS-2.");
  }

  Serial.print("Mounting SD Card");
  if (!SD.begin())
    Serial.println("Card Mount Failed");

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
    Serial.println("No SD card attached");

  String heading = "Timestamp";

  for (int i = 1; i <= totalChannels; i++)
    heading += ",CH" + String(i);
  heading += "\n";

  appendFile(SD, filename, (title + heading).c_str());
}

void loop()
{
  readEEPROM();
  if (Serial2.available() > 0)
  {
    String input = Serial2.readString();
    Serial.println(input);
    updateParams(input);
  }
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    if (ads.begin())
    {
      read_adc1();
      delay(50);
      read_adc2();
      printAnalogData();
      performOperationAndSend();
    }
    else
    {
      Serial.println("Not reading ADC as not initiliased");
    }
  }
  if (currentMillis - SDCardpreviousMillis >= SDCardinterval)
  {
    SDCardpreviousMillis = currentMillis;
    Serial.println("Appending in SD Card");
    processAndAppendData(SD, doc2);
  }
  RestartcurrentMillis = millis();
  if (RestartcurrentMillis - RestartpreviousMillis >= Restartinterval)
  {
    RestartpreviousMillis = RestartcurrentMillis;
    Serial.println("Performing hourly restart from void loop "); // Isko touch nh karna h
    delay(100);
    ESP.restart();
    return;
  }
}

void readEEPROM()
{
  for (int i = 0; i < totalChannels; i++)
  {
    int addr_MIN = i * sizeof(Parameters);
    int addr_MAX = addr_MIN + sizeof(float);
    int addr_FAC = addr_MAX + sizeof(float);
    int addr_CAL = addr_FAC + sizeof(float);

    EEPROM.get(addr_MIN, dataArray[i].param.MIN);
    EEPROM.get(addr_MAX, dataArray[i].param.MAX);
    EEPROM.get(addr_FAC, dataArray[i].param.FAC);
    EEPROM.get(addr_CAL, dataArray[i].param.CAL);
  }
}

void printParams()
{
  for (int i = 0; i < totalChannels; i++)
  {
    Serial.print("Channel: ");
    Serial.println(i + 1);
    Serial.print("MIN: ");
    Serial.println(dataArray[i].param.MIN);
    Serial.print("MAX: ");
    Serial.println(dataArray[i].param.MAX);
    Serial.print("FAC: ");
    Serial.println(dataArray[i].param.FAC);
    Serial.print("CAL: ");
    Serial.println(dataArray[i].param.CAL);
    Serial.println();
  }
}

void performOperationAndSend()
{
  float mappedValue[totalChannels];
  for (int i = 0; i < totalChannels; i++)
  {
    // Map the raw value using linear interpolation
    mappedValue[i] = mapFloat(volts[i], 0.6, 3.0, dataArray[i].param.MIN, dataArray[i].param.MAX); // Mapping function

    // Apply factor operation based on factor value
    float outputValue = mappedValue[i];
    switch (int(dataArray[i].param.FAC))
    {
    case 0: // Addition
      outputValue += dataArray[i].param.CAL;
      break;
    case 1: // Subtraction
      outputValue -= dataArray[i].param.CAL;
      break;
    case 2: // Multiplication
      outputValue *= dataArray[i].param.CAL;
      break;
    case 3: // Division
      if (dataArray[i].param.CAL != 0)
        outputValue /= dataArray[i].param.CAL;
      break;
    }

    // Calculate error: if mappedValue is less than 0.3, set error to 1, else 0
    int error = (mappedValue[i] < 0.3) ? 1 : 0;

    // Add the values to the JSON document with 2-point precision
    doc2[String("RawCH") + String(i + 1)] = String(mappedValue[i], 2);
    doc2[String("CH") + String(i + 1)] = String(outputValue, 2);
    doc2[String("ERR") + String(i + 1)] = error;
  }

  // Serialize JSON to string and send through UART
  String jsonString;
  serializeJson(doc2, jsonString);
  Serial2.println(jsonString); // Send JSON string to M2 through UART
}

float mapFloat(float I, float Ilow, float Ihigh, float Pvlow, float Pvhigh)
{
  // Check if Ihigh and Ilow are the same to avoid division by zero
  if (Ihigh == Ilow)
  {
    Serial.println("Current range is zero. Returning Pvlow.");
    return Pvlow;
  }
  if (I == 0)
  {
    return I;
  }

  // Apply the conversion formula
  float mappedValue = ((Pvhigh - Pvlow) / (Ihigh - Ilow)) * (I - Ilow) + Pvlow;

  return mappedValue;
}

void read_adc1()
{
  volts[0] = ads.computeVolts(ads.readADC_SingleEnded(2));
  volts[1] = ads.computeVolts(ads.readADC_SingleEnded(1));
  volts[2] = ads.computeVolts(ads.readADC_SingleEnded(0));
  volts[3] = ads.computeVolts(ads.readADC_SingleEnded(3));

  // Set readings close to zero to zero
  if (abs(volts[0]) < 0.2)
    volts[0] = 0.0;
  if (abs(volts[1]) < 0.2)
    volts[1] = 0.0;
  if (abs(volts[2]) < 0.2)
    volts[2] = 0.0;
  if (abs(volts[3]) < 0.2)
    volts[3] = 0.0;
}

void read_adc2()
{
  volts[4] = ads2.computeVolts(ads2.readADC_SingleEnded(0));
  volts[5] = ads2.computeVolts(ads2.readADC_SingleEnded(1));
  volts[6] = ads2.computeVolts(ads2.readADC_SingleEnded(2));

  // Set readings close to zero to zero
  if (abs(volts[4]) < 0.2)
    volts[4] = 0.0;
  if (abs(volts[5]) < 0.2)
    volts[5] = 0.0;
  if (abs(volts[6]) < 0.2)
    volts[6] = 0.0;
}

void printAnalogData()
{
  Serial.println("-----------------------------------------------------------");
  Serial.printf("Input Voltage: %.2fv\n", volts[6] + (volts[6] * 10.58));
  Serial.printf("AIN0: %.2fv\n", volts[0]);
  Serial.printf("AIN1: %.2fv\n", volts[1]);
  Serial.printf("AIN2: %.2fv\n", volts[2]);
  Serial.printf("AIN3: %.2fv\n", volts[3]);
  Serial.printf("AIN4: %.2fv\n", volts[4]);
  Serial.printf("AIN5: %.2fv\n", volts[5]);
}

void updateParams(String input)
{
  // Parse input string and update only changed values
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, input);

  const float threshold = 0.01; // Precision threshold for floating-point comparison

  for (int i = 0; i < 6; i++)
  {
    // Update only if the keys exist in the input JSON
    if (doc.containsKey("MIN" + String(i + 1)))
    {
      float new_MIN = doc["MIN" + String(i + 1)].as<float>();
      if (abs(new_MIN - dataArray[i].param.MIN) > threshold)
      {
        Serial.println("Updating MIN" + String(i + 1) + " from " + String(dataArray[i].param.MIN) + " to " + String(new_MIN));
        dataArray[i].param.MIN = new_MIN;
        EEPROM.put(i * sizeof(Parameters) + offsetof(Parameters, MIN), dataArray[i].param.MIN);
      }
    }

    if (doc.containsKey("MAX" + String(i + 1)))
    {
      float new_MAX = doc["MAX" + String(i + 1)].as<float>();
      if (abs(new_MAX - dataArray[i].param.MAX) > threshold)
      {
        Serial.println("Updating MAX" + String(i + 1) + " from " + String(dataArray[i].param.MAX) + " to " + String(new_MAX));
        dataArray[i].param.MAX = new_MAX;
        EEPROM.put(i * sizeof(Parameters) + offsetof(Parameters, MAX), dataArray[i].param.MAX);
      }
    }

    if (doc.containsKey("FAC" + String(i + 1)))
    {
      float new_FAC = doc["FAC" + String(i + 1)].as<float>();
      if (abs(new_FAC - dataArray[i].param.FAC) > threshold)
      {
        Serial.println("Updating FAC" + String(i + 1) + " from " + String(dataArray[i].param.FAC) + " to " + String(new_FAC));
        dataArray[i].param.FAC = new_FAC;
        EEPROM.put(i * sizeof(Parameters) + offsetof(Parameters, FAC), dataArray[i].param.FAC);
      }
    }

    if (doc.containsKey("CAL" + String(i + 1)))
    {
      float new_CAL = doc["CAL" + String(i + 1)].as<float>();
      if (abs(new_CAL - dataArray[i].param.CAL) > threshold)
      {
        Serial.println("Updating CAL" + String(i + 1) + " from " + String(dataArray[i].param.CAL) + " to " + String(new_CAL));
        dataArray[i].param.CAL = new_CAL;
        EEPROM.put(i * sizeof(Parameters) + offsetof(Parameters, CAL), dataArray[i].param.CAL);
      }
    }
  }

  EEPROM.commit(); // Commit changes to EEPROM
  printParams();
}

void processAndAppendData(fs::FS &fs, DynamicJsonDocument &doc2)
{
  String csvLine = doc2["Timestamp"].as<String>();

  // Loop through CH1 to CH6 and append to csvLine
  for (int i = 1; i <= totalChannels; i++)
    csvLine += "," + doc2["CH" + String(i)].as<String>();
  csvLine += "\n";
  Serial.println(csvLine);

  // Append the CSV header only once
  /* static bool headerWritten = false;
   if (!headerWritten) {
       appendFile(fs, "/Alpha.csv", "Timestamp,CH1,CH2,CH3\n");
       headerWritten = true;
   }*/

  // Append the data line to the CSV file
  appendFile(fs, "/AX306_SD.csv", csvLine.c_str());
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
    Serial.println("Message appended");
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}