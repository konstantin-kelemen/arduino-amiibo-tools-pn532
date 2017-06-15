#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (10)
#define PN532_MISO (12)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void) {
  Serial.begin(115200); // Initialize serial communications with the PC
  while (!Serial);      // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  nfc.begin();          // Init NFC

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print(F("Didn't find PN53x board"));
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print(F("Found chip PN5")); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print(F("Firmware ver. ")); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print(F(".")); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println();
  Serial.println(F("Arduino amiibo reader. Put your NFC-tag to the reader"));
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID
    
  // Wait for an NTAG215 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    Serial.println();
    Serial.print(F("Tag UID: "));
    PrintHexShort(uid, uidLength);
    Serial.println();
    
    if (uidLength == 7)
    {     
      uint8_t data[32];

      Serial.println(F("Starting dump..."));
      Serial.println();

      for (uint8_t i = 0; i < 135; i++) 
      {
        success = nfc.ntag2xx_ReadPage(i, data);

        // Display the results, depending on 'success'
        if (success) 
        {
          // Dump the page data
          PrintHexShort(data, 4);
        }
        else
        {
          Serial.println(F("Unable to read the requested page!"));
        }
      }      
    }
    else
    {
      Serial.println(F("This doesn't seem to be Amiibo!"));
    }

    Serial.println();
    Serial.println(F("Dump finished! Now please remove your Amiibo from the reader."));
    countdown();
    return;
  }
}

void countdown() {
  delay(1000);
  Serial.println();
  Serial.print(F("New attempt in 3..."));
  delay(1000);
  Serial.print(F("2..."));
  delay(1000);
  Serial.print(F("1..."));
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.println(F("Ready to read."));
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void PrintHexShort(const byte * data, const uint32_t numBytes)
{
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++) 
  {
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      Serial.print(F("0"));
    Serial.print(data[szPos]&0xff, HEX);
  }
  Serial.println();
}
