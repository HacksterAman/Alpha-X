// GSM
#include <Arduino.h>
#include <ArduinoJson.h>
#define TINY_GSM_MODEM_SIM7600 // SIM7600 AT instruction is compatible with A7670
#define SerialAT Serial1
#define SerialMon Serial
#define TINY_GSM_USE_GPRS true
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

#define RXD2 27    // GSM RXD INTERNALLY CONNECTED
#define TXD2 26    // GSM TXD INTERNALLY CONNECTED
#define powerPin 4 ////ESP32 PIN D4 CONNECTED TO POWER PIN OF A7670C CHIPSET, INTERNALLY CONNECTED

const char apn[] = ""; // APN automatically detects for 4G SIM, NO NEED TO ENTER, KEEP IT BLANK

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char *mqttServer = "mqtt.reflowtech.cloud";
const int mqttPort = 1883;
const char *mqttUser = "mqttreflowtech";
const char *mqttPassword = "mastertransfer#321@66#mqtt";

#define MQTT_TOPIC_INPUT "AX3/01/INPUT"
#define MQTT_TOPIC_OUTPUT "AX3/01/OUTPUT"
#define MQTT_TOPIC_LOG "AX3/01/LOG"

const unsigned long intervalDataDump = 60000;   // 1 minute
const unsigned long intervalLogPublish = 60000; // 1 minute

unsigned long previousMillisDataDump = 0;
unsigned long previousMillisLogPublish = 0;

unsigned long RestartpreviousMillis = 0;        // will store last time the code was executed
const unsigned long Restartinterval = 21600000; // 3600000interval at which to execute the code (1 hour in milliseconds)6hrs
unsigned long RestartcurrentMillis = 0;

const int txPin = 17;
const int rxPin = 16;

char dateTimeStr[20];
String previousDateTime = "";
String currentDateTime;
String storedMessage;

StaticJsonDocument<300> jsonDoc;

void sendDataDump();
void handleUart2Data(const String &rawData);
void publishLogData(const String &rawData);
void callback(char *topic, byte *payload, unsigned int length);
void connectToMqttServer();
char *getDateTime(bool includeSeconds);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, rxPin, txPin);
  pinMode(powerPin, OUTPUT);

  // Power on GSM Module
  digitalWrite(powerPin, LOW);
  delay(700);
  digitalWrite(powerPin, HIGH);
  delay(1000);
  digitalWrite(powerPin, LOW);
  delay(200);

  Serial.println("\nConfiguring REFLOW_ALPHA_X SERIES. Kindly wait");
  delay(10000);
  SerialAT.begin(115200, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Waiting for network...");
  if (!modem.waitForNetwork())
  {
    Serial.println("Network fail");
    return;
  }
  Serial.println("Network success");

  if (!modem.gprsConnect(apn))
  {
    Serial.println("GPRS connect fail");
    return;
  }
  Serial.println("GPRS connected");

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);
  connectToMqttServer();
}

void loop()
{
  if (!mqtt.connected())
  {
    connectToMqttServer();
  }
  mqtt.loop();

  unsigned long currentMillis = millis();

  // Send data dump every minute
  if (currentMillis - previousMillisDataDump >= intervalDataDump)
  {
    previousMillisDataDump = currentMillis;
    sendDataDump();
  }

  String rawData;
  // Publish data from UART2 every minute
  if (Serial2.available())
  {
    rawData = Serial2.readStringUntil('\n');
    handleUart2Data(rawData);
  }

  // Publish log data every minute
  if (currentMillis - previousMillisLogPublish >= intervalLogPublish && rawData.length() > 0)
  {
    previousMillisLogPublish = currentMillis;
    publishLogData(rawData);
  }
  RestartcurrentMillis = millis();
  if (RestartcurrentMillis - RestartpreviousMillis >= Restartinterval)
  {
    RestartpreviousMillis = RestartcurrentMillis;
    Serial.println("Performing hourly restart from void loop "); // Isko touch nh karna h
    delay(100);
    modem.restart();
    delay(100);
    ESP.restart();
    return;
  }
}

void sendDataDump()
{
  char tempDate[20];
  strncpy(tempDate, getDateTime(1), 19);
  StaticJsonDocument<500> copyDoc;
  copyDoc["Date&Time"] = String(tempDate);
  for (JsonPair kv : jsonDoc.as<JsonObject>())
  {
    copyDoc[kv.key()] = kv.value();
  }
  serializeJson(copyDoc, Serial2);
}

void handleUart2Data(const String &rawData)
{

  String outputData = rawData;
  currentDateTime = getDateTime(1);
  if (outputData.startsWith("{") && outputData.endsWith("}\r"))
  {
    outputData.remove(outputData.length() - 2);
    outputData += ", \"UpdateTimeStamp\": \"" + currentDateTime + "\"}";
    mqtt.publish(MQTT_TOPIC_OUTPUT, outputData.c_str(), true);
  }
}

void publishLogData(const String &rawData)
{
  // StaticJsonDocument<256> tempDoc;
  StaticJsonDocument<256> logDoc;
  // DeserializationError error = deserializeJson(tempDoc, Serial2.readStringUntil('\n'));
  // String outputData = Serial2.readStringUntil('\n');
  // Remove any leading/trailing whitespace or newline characters (just in case)
  // outputData.trim();

  // Print the received string (for debugging)
  Serial.print("Received Data: ");
  Serial.println(rawData);

  // Create a StaticJsonDocument for parsing
  StaticJsonDocument<300> jsonDoc;

  // Parse the cleaned outputData string into the jsonDoc
  DeserializationError error = deserializeJson(jsonDoc, rawData);
  Serial.print("Error Code:");
  Serial.println(error.c_str());
  // if (!error) {

  logDoc["DeviceId"] = "AX301";
  logDoc["Timestamp"] = getDateTime(0);
  logDoc["CH1"] = jsonDoc["CH1"];
  logDoc["CH2"] = jsonDoc["CH2"];
  logDoc["CH3"] = jsonDoc["CH3"];
  String logData;
  serializeJson(logDoc, logData);
  mqtt.publish(MQTT_TOPIC_LOG, logData.c_str());
  //}
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println("Received MQTT message: " + message);

  if (String(topic) == MQTT_TOPIC_INPUT)
  {
    StaticJsonDocument<400> tempDoc;
    DeserializationError error = deserializeJson(tempDoc, message);
    for (JsonPair kv : tempDoc.as<JsonObject>())
    {
      jsonDoc[kv.key()] = kv.value();
    }
    sendDataDump();
  }
}

void connectToMqttServer()
{
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect("AX301", mqttUser, mqttPassword))
    {
      Serial.println("Connected");
      if (mqtt.subscribe(MQTT_TOPIC_INPUT))
      {
        Serial.println("Subscribed");
      }
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }
  }
}

char *getDateTime(bool includeSeconds)
{
  static char dateTimeStr[20];
  SerialAT.println("AT+CCLK?");
  delay(50);

  if (SerialAT.available())
  {
    String response = SerialAT.readString();
    int startIndex = response.indexOf("+CCLK: \"");
    int endIndex = response.indexOf("\"", startIndex + 8);
    if (startIndex != -1 && endIndex != -1)
    {
      String dateAndTimeInfo = response.substring(startIndex + 8, endIndex);
      String year = dateAndTimeInfo.substring(0, 2);
      String month = dateAndTimeInfo.substring(3, 5);
      String day = dateAndTimeInfo.substring(6, 8);
      String hour = dateAndTimeInfo.substring(9, 11);
      String minute = dateAndTimeInfo.substring(12, 14);
      String second = dateAndTimeInfo.substring(15, 17);
      // String dateTime = day + "/" + month + "/" + year + " " + hour + ":" + minute + ":" + second;
      String dateTime;
      if (includeSeconds)
      {
        dateTime = day + "/" + month + "/" + year + " " + hour + ":" + minute + ":" + second;
      }
      else
      {
        dateTime = "20" + year + "-" + month + "-" + day + " " + hour + ":" + minute + ":00";
      }
      strncpy(dateTimeStr, dateTime.c_str(), sizeof(dateTimeStr) - 1);
      dateTimeStr[sizeof(dateTimeStr) - 1] = '\0';
    }
  }
  return dateTimeStr;
}
