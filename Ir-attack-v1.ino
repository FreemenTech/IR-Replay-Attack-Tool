#include <IRremote.hpp>

const int irReceiver = 2;
const int irSender = 3;
const int btn = 7;

uint16_t lastAddress = 0; //address of target
uint16_t lastCommand = 0;  //action
//uint16_t is for unsigned interger in other words interger without sign so positive number 0 to 65535

bool hasSignal = false; //boolean to know if something is captured
decode_type_t protocol ;


void setup() {
  Serial.begin(9600);
  pinMode(btn, INPUT_PULLUP);
  IrReceiver.begin(irReceiver);
  IrSender.begin(irSender);
  Serial.println("IR Replay attack tool started...");
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
      Serial.println("---");
    }

    IrReceiver.resume();
  }

  // Replay
  if (digitalRead(btn) == LOW && hasSignal) {
    IrReceiver.stop(); //turn off receiver
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
    while (digitalRead(btn) == LOW);  // attend relachement
    delay(100);
    IrReceiver.start(); //restart receiver 
  }
}