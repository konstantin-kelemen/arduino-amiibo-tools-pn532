/*
 * This sketch is writing to page 3 which is impossible to achieve with defauld
 * Adafruit library. Please use my fork instead:
 * https://github.com/konstantin-kelemen/Adafruit-PN532
 */

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
  Serial.println(F("Arduino amiibo writer. Put your NFC-tag to the reader"));
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID

  byte pagebytes[] = {0, 0, 0, 0};          // Buffer to store 4 page bytes
  byte pages = 135;
  byte dataBlock[] = {

/*  Paste your dump here
 *
 *  This is an example of how the pasted dump should look like:
 *  
 *  0x04, 0x11, 0x22, 0xbf,
 *  0x33, 0x44, 0x55, 0x80,
 *  0xa2, 0x48, 0x00, 0x00,
 *  0xf1, 0x10, 0xff, 0xee,
 *  ...
 *  0x00, 0x00, 0x00, 0xbd,
 *  0x00, 0x00, 0x00, 0x04,
 *  0x5f, 0x00, 0x00, 0x00,
 *  0x88, 0x33, 0xcc, 0x91,
 *  0x80, 0x80, 0x00, 0x00
 */

  };

  // Wait for an NTAG215 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println();
    Serial.println(F("Tag found, writing..."));
    Serial.println();

    if (uidLength == 7)
    {
      // Write main data
      for (byte page = 3; page < pages; page++) {
        // Write data to the page
        Serial.print(F("Writing data into page ")); Serial.print(page);
        Serial.println(F(" ..."));
        for (byte byte = 0; byte < 4; byte++) {
          pagebytes[byte] = dataBlock[(page * 4) + byte];
        }
        success = nfc.ntag2xx_WritePage(page, pagebytes);
        
        if (success)
        {
          // Dump the page data
          PrintHexShort(pagebytes, 4);
        }
        else
        {
          Serial.println(F("Write process failed, please try once more."));
          Serial.println(F("Your tag is still fine, just remove it and put back again in 3 seconds."));
          countdown();
          return;
        }
        Serial.println();
      }
      
      // Write lock bytes - the last thing you should do.
      // If you write them too early - your tag is wasted.
      // Write the Dynamic Lock Bytes
      byte DynamicLockBytes[] = {0x01, 0x00, 0x0F, 0xBD};

      Serial.println(F("Writing dynamic lock bytes"));
      success = nfc.ntag2xx_WritePage(130, DynamicLockBytes);

      if (success)
      {
        // Dump the page data
        PrintHexShort(DynamicLockBytes, 4);
      }
      else
      {
        Serial.println(F("Write process failed, please try once more."));
        Serial.println(F("Your tag is probably still fine, just remove it and put back again in 3 seconds."));
        Serial.println(F("Try a new tag if that didn't help."));
        countdown();
        return;
      }
      Serial.println();
      
      // Now we can write Static Lock Bytes - first 4 bytes
      byte StaticLockBytes[] =  {0x00, 0x00, 0x0F, 0xE0};
      
      success = nfc.ntag2xx_WritePage(2, StaticLockBytes);

      if (success)
      {
        // Dump the page data
        PrintHexShort(StaticLockBytes, 4);
      }
      else
      {
        Serial.println(F("Write process failed."));
        Serial.println(F("The current tag is probably useless now."));
        Serial.println(F("Please try to use another tag. Don't forget to update the dump."));
        countdown();
        return;
      }
      Serial.println();
      Serial.println(F("Write process finished! Now please take your Amiibo card away!"));
      countdown();
      return;
    }
    else
    {
      Serial.println(F("This doesn't seem to be NTAG215 tag!"));
      countdown();
      return;
    }
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
  Serial.println(F("Ready to write."));
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
