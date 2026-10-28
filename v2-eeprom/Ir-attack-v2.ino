#include <IRremote.hpp>
#include <EEPROM.h>

const int irReceiver = 2;
const int irSender = 3;
const int btn = 7;

uint16_t lastAddress = 0;
uint16_t lastCommand = 0;
bool hasSignal = false;
decode_type_t protocol;

// EEPROM memory map
// Address 0-1 : lastAddress (uint16_t = 2 bytes)
// Address 2-3 : lastCommand (uint16_t = 2 bytes)
// Address 4   : protocol (1 byte)
// Address 5   : flag — 0xAA means valid data exists

const int ADDR_ADDRESS = 0;
const int ADDR_COMMAND = 2;
const int ADDR_PROTOCOL = 4;
const int ADDR_FLAG = 5;

void saveToEEPROM() {
  EEPROM.put(ADDR_ADDRESS, lastAddress);
  EEPROM.put(ADDR_COMMAND, lastCommand);
  EEPROM.write(ADDR_PROTOCOL, (uint8_t)protocol);
  EEPROM.write(ADDR_FLAG, 0xAA);
  Serial.println("[EEPROM] Signal saved!");
}

void loadFromEEPROM() {
  if (EEPROM.read(ADDR_FLAG) == 0xAA) {
    EEPROM.get(ADDR_ADDRESS, lastAddress);
    EEPROM.get(ADDR_COMMAND, lastCommand);
    protocol = (decode_type_t)EEPROM.read(ADDR_PROTOCOL);
    hasSignal = true;
    Serial.println("[EEPROM] Signal loaded from memory:");
    Serial.print("  Protocol: ");
    Serial.println(getProtocolString(protocol));
    Serial.print("  Address: 0x");
    Serial.println(lastAddress, HEX);
    Serial.print("  Command: 0x");
    Serial.println(lastCommand, HEX);
  } else {
    Serial.println("[EEPROM] No saved signal found.");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(btn, INPUT_PULLUP);
  IrReceiver.begin(irReceiver);
  IrSender.begin(irSender);
  Serial.println("IR Replay attack tool started...");
  loadFromEEPROM();
}

void loop() {
  // Capture
  if (IrReceiver.decode()) {
    uint16_t address = IrReceiver.decodedIRData.address;
    uint16_t command = IrReceiver.decodedIRData.command;

    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      protocol = IrReceiver.decodedIRData.protocol;
      lastAddress = address;
      lastCommand = command;
      hasSignal = true;
      Serial.print("Protocol: ");
      Serial.println(getProtocolString(protocol));
      Serial.print("Address: 0x");
      Serial.println(address, HEX);
      Serial.print("Command: 0x");
      Serial.println(command, HEX);
      saveToEEPROM();
      Serial.println("---");
    }

    IrReceiver.resume();
  }

  // Replay
  if (digitalRead(btn) == LOW && hasSignal) {
    IrReceiver.stop();
    Serial.print("REPLAY -> Addr: 0x");
    Serial.print(lastAddress, HEX);
    Serial.print(" Cmd: 0x");
    Serial.println(lastCommand, HEX);

    switch (protocol) {
      case NEC:
        IrSender.sendNEC(lastAddress, lastCommand, 3);
        Serial.println("Sent!");
        break;
      case SONY:
        IrSender.sendSony(lastAddress, lastCommand, 3);
        Serial.println("Sent!");
        break;
      case SAMSUNG:
        IrSender.sendSamsung(lastAddress, lastCommand, 3);
        Serial.println("Sent!");
        break;
      case RC5:
        IrSender.sendRC5(lastAddress, lastCommand, 3);
        Serial.println("Sent!");
        break;
      case RC6:
        IrSender.sendRC6(lastAddress, lastCommand, 3);
        Serial.println("Sent!");
        break;
      default:
        Serial.println("Protocol not supported for replay");
        break;
    }

    delay(500);
    while (digitalRead(btn) == LOW);
    delay(100);
    IrReceiver.start();
  }
}
