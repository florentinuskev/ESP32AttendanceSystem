#include <Arduino.h>

// LCD_I2C Dependencies
#include <LiquidCrystal_I2C.h>

// WiFi Dependencies
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// RC522 Dependencies
#include <SPI.h>
#include <MFRC522.h>

// LCD Definitions
int colLen = 16;
int rowLen = 2;
LiquidCrystal_I2C lcd(0x27, colLen, rowLen);

// Definitions for RC522
#define SS_PIN 5
#define RST_PIN 0
#define NR_OF_READERS 2

MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

byte bufferLen = 18;
byte readBlockData[18];
byte sector = 1;

String urlString;

// Definition of Data that will be written in the Tag / Card
int blockNum = 2;

// WiFi Definitions
const char *ssid = "IoT";
const char *password = "iottesting123";

// HTTP Server Definition
const String sheetURL = "https://script.google.com/macros/s/AKfycbye45JrHpUc6cEhl9t68a_INpHbtEMFmyNVtRF0zS7o1zlv9uEV6UWs3eEbnz7Ssl-V/exec?name="; // Please change this following your Spreadsheet URL.

/**************** Read Functions() ****************/

void readDataFromBlock(int blockNum, byte readBlockData[])
{
  // Authenticate using key A
  Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode)rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("Authentication Failed to Write: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Authentication Success");
  }

  // Read data from the block (again, should now be what we have written)
  Serial.print(F("Reading data from block "));
  Serial.print(blockNum);
  Serial.println(F(" ..."));

  status = (MFRC522::StatusCode)rfid.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
  }
  else
  {
    Serial.println("Successfully read data from block!");
  }
}

void setup()
{
  Serial.begin(9600);

  // Starting / Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing System...");

  // Starting WiFi Connectivity
  Serial.println("Connecting to Access Point...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }

  Serial.println();
  Serial.println("WiFi Connected...");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Starting / Initializing RC522
  SPI.begin();
  rfid.PCD_Init();

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  // Set LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your card...");
}

void loop()
{
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.println("**Card Detected");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card Detected!");

  /*---------------- Start to read data to block -----------------*/
  Serial.println("Reading data from block...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading data from block...");
  readDataFromBlock(blockNum, readBlockData);
  /*---------------- End to read data to block -----------------*/

  /*---------------- Print the result of the block -----------------*/
  Serial.print("\n");
  Serial.print("Data in RFID:");
  Serial.print(blockNum);
  Serial.print(" --> ");
  for (int j = 0; j < 16; j++)
  {
    Serial.write(readBlockData[j]);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome " + String((char *)readBlockData) + "!");
  /*---------------- Print the result of the block -----------------*/

  /*---------------- Send data to Google SpreadSheet -----------------*/

  if (WiFi.status() == WL_CONNECTED)
  {

    urlString = sheetURL + String((char *)readBlockData);

    WiFiClientSecure client;
    client.setInsecure();

    // HTTP
    HTTPClient https;
    Serial.println("[HTTPS] Begin....");
    if (https.begin(client, (String)urlString))
    {
      // Send HTTP GET request
      int httpResponseCode = https.GET();

      if (httpResponseCode > 0)
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = https.getString();
        Serial.println(payload);
        lcd.setCursor(0, 1);
        lcd.print("Data Recorded!");
        delay(2000);
      }
      else
      {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      https.end();
      delay(1000);
    }
  }
  else
  {
    Serial.println("[HTTPS] Unable to Connect...");
  }
  /*---------------- Send data to Google SpreadSheet -----------------*/

  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  // Set LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your card...");
}