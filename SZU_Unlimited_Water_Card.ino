#include <SPI.h>
#include <RFID.h>

RFID rfid(10, 9);

unsigned char serNum[5];
unsigned char status;
unsigned char str[MAX_LEN];
unsigned char blockAddr;

unsigned char writeDate[16] = {0X5F, 0XCB, 0XC9, 0X12, 'o', 'm', 'e', ' ', 'Y', 'F', 'R', 'o', 'b', 'o', 't', 0};

unsigned char sectorKeyA[16][16] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.init();
  Serial.println("Ready. Place CAMPUS CARD...");
}

void loop() {
  // === Step 1: Read campus card ===
  if (!waitForCard("CAMPUS")) return;
  memcpy(writeDate, str, 4);
  writeDate[4] = str[0] ^ str[1] ^ str[2] ^ str[3];  // BCC fix
  writeDate[5] = 0x08;  // SAK: MIFARE Classic 1K (was 'm'=0x6D)
  writeDate[6] = 0x04;  // ATQA byte 0 (was 'e'=0x65)
  writeDate[7] = 0x00;  // ATQA byte 1 (was ' '=0x20)
  rfid.selectTag(rfid.serNum);
  rfid.halt();

  // === Step 2: Swap to white card ===
  Serial.println(">> REMOVE campus card, place WHITE CARD now! (3 sec)");
  delay(3000);

  // === Step 3: Read white card ===
  if (!waitForCard("WHITE")) {
    Serial.println("No white card detected! Reset to retry.");
    while (1);
  }
  rfid.selectTag(rfid.serNum);

  // === Step 4: Write ===
  writeCard(0);
  rfid.halt();
  delay(100);

  // === Step 5: Verify (use WUPA since card was halted) ===
  rfid.MFRC522Request(PICC_REQALL, str);
  if (rfid.anticoll(str) == MI_OK) {
    Serial.print("Verify - new UID: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(0x0F & (str[i] >> 4), HEX);
      Serial.print(0x0F & str[i], HEX);
    }
    Serial.println("");
    Serial.println("SUCCESS! White card now has campus UID.");
  } else {
    Serial.println("VERIFY FAILED! Card may be bricked.");
  }

  rfid.halt();
  SPI.end();
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  while (1);
}

bool waitForCard(const char* label) {
  unsigned long start = millis();
  while (millis() - start < 5000) {
    rfid.MFRC522Request(PICC_REQIDL, str);
    if (rfid.anticoll(str) == MI_OK) {
      Serial.print("[");
      Serial.print(label);
      Serial.print("] UID: ");
      for (int i = 0; i < 4; i++) {
        Serial.print(0x0F & (str[i] >> 4), HEX);
        Serial.print(0x0F & str[i], HEX);
      }
      Serial.println("");
      memcpy(rfid.serNum, str, 5);
      return true;
    }
    delay(200);
  }
  Serial.print("[");
  Serial.print(label);
  Serial.println("] Timeout - no card found!");
  return false;
}

void writeCard(int blockAddr) {
  if (rfid.auth(PICC_AUTHENT1A, blockAddr, sectorKeyA[blockAddr / 4], rfid.serNum) == MI_OK) {
    if (rfid.write(blockAddr, writeDate) == MI_OK) {
      Serial.println("Write card OK!");
    } else {
      Serial.println("Write FAILED!");
    }
  } else {
    Serial.println("Auth FAILED! Is this a CUID card?");
  }
}