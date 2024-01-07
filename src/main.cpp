/*
  Rui Santos
  Complete project details at our blog: https://RandomNerdTutorials.com/esp32-esp8266-firebase-authentication/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based in the Authenticatiions Examples by Firebase-ESP-Client library by mobizt: https://github.com/mobizt/Firebase-ESP-Client/tree/main/examples/Authentications
*/

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <FirebaseHelper.h>
#include <FastLED.h>
#include <Controls.h>
#include <EEPROM.h>
#include <BluetoothHelper.h>

// Insert your network credentials
// #define WIFI_SSID "TTNET_ZyXEL_3JN3"
// #define WIFI_SSID "Emir's iPhone 14"
// #define WIFI_SSID "Asaf's Galaxy S21 5G"
// #define WIFI_PASSWORD "ataturk_1881"
// #define WIFI_PASSWORD "test12345"
// #define WIFI_PASSWORD "fnsc2964"
#define BAUD_RATE 115200
bool ledEnabled = false;
BulbData bulbData;
String ssid;
String password;
// Initialize WiFi
void initWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

void logIncomingStreamData(FirebaseStream data)
{
  Serial.print("Stream Data -> resource path: ");
  Serial.print(data.dataPath());
  Serial.print(", resource type: ");
  Serial.println(data.dataType());

  // Print out the value
  // Stream data can be many types which can be determined from function dataType
  if (data.dataTypeEnum() == firebase_rtdb_data_type_integer)
    Serial.println(data.to<int>());
  else if (data.dataTypeEnum() == firebase_rtdb_data_type_float)
    Serial.println(data.to<float>(), 5);
  else if (data.dataTypeEnum() == firebase_rtdb_data_type_double)
    printf("%.9lf\n", data.to<double>());
  else if (data.dataTypeEnum() == firebase_rtdb_data_type_boolean)
    Serial.println(data.to<bool>() ? "true" : "false");
  else if (data.dataTypeEnum() == firebase_rtdb_data_type_string)
    Serial.println(data.to<String>());
  else if (data.dataTypeEnum() == firebase_rtdb_data_type_json)
  {
    FirebaseJson *json = data.to<FirebaseJson *>();
    Serial.println(json->raw());
  }
  else if (data.dataTypeEnum() == firebase_rtdb_data_type_array)
  {
    FirebaseJsonArray *arr = data.to<FirebaseJsonArray *>();
    Serial.println(arr->raw());
  }
}

void streamCallback(FirebaseStream data)
{
  logIncomingStreamData(data);
  String path = data.dataPath();
  if (path == "/")
  {
    // Received entire object, parse and validate
    FirebaseJson *json = data.to<FirebaseJson *>();
    bulbData = BulbData(json);
  }
  else
  {
    // Received partial object with only the change, parse and validate
    bulbData.processJson(data);
  }

  FirebaseJson *dataJson = bulbData.toFirebaseJson();
  dataJson->toString(Serial, true);

  // Adjust lights
  adjustLights(bulbData);

  if (bulbData.containsCorrectedValue())
  {
    // Clear corrected values
    bulbData.clearCorrectedValue();

    // Update DB
    Serial.println("Data contains corrected value, will update db");
    String pathToDevice = resourcePath("", homegroupId);
    bool success = Firebase.RTDB.setJSON(&fbdo, pathToDevice, dataJson);
    Serial.print("Save ");
    Serial.println(success ? "successful." : "failed.");

    if (!success)
    {
      String message = fbdo.errorReason();
      Serial.print("Error message:");
      Serial.println(message);
    }
  }
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    // Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }
}
void readDataFromEEPROM()
{
  ssid.reserve(SSID_MAX_LENGTH);
  password.reserve(PASSWORD_MAX_LENGTH);
  homegroupId.reserve(SSID_MAX_LENGTH);
  Serial.println("String for data reserved");
  ssid = EEPROM.readString(0);
  password = EEPROM.readString(SSID_MAX_LENGTH);
  homegroupId = EEPROM.readString(SSID_MAX_LENGTH * 2);
  Serial.print("ssid:");
  Serial.println(ssid);
  Serial.print("password:");
  Serial.println(password);
  Serial.print("homegroupId:");
  Serial.println(homegroupId);
}

bool isEEPROMDataValid()
{
  return (!ssid.isEmpty() && !password.isEmpty() && !homegroupId.isEmpty());
}

void completeSetup()
{
  setupFirebase();
  initializeLeds();

  // Getting the user UID might take a few seconds
  waitForUserUid();

  // Start HTTP stream
  if (!Firebase.RTDB.beginStream(&fbdo, resourcePath("", homegroupId)))
  {
    Serial.println("Can't begin stream connection...");
    Serial.print("Reason: ");
    Serial.print(fbdo.errorReason().c_str());
  }

  // streamCallback runs each time when stream receives data
  // streamTimeoutCallback runs when connection was timeout during HTTP stream
  Firebase.RTDB.setStreamCallback(&fbdo, streamCallback, streamTimeoutCallback);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  EEPROM.begin(512);
  readDataFromEEPROM();
  if (!isEEPROMDataValid())
  {
    Serial.println("EEPROM data not valid!");
    // Bluetooth helper will call setupCallback when connection is success
    setupCallback = &completeSetup;
    startBLE();
  }
  else
  {
    initWiFi();
    completeSetup();
    setupCallback = &completeSetup;
    startBLE();
  }
}

void loop()
{
  bool firebaseReady = Firebase.ready() && auth.token.uid != NULL;
  if (!firebaseReady)
    return;
}