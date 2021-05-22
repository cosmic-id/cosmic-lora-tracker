#include <SPI.h>
#include <SD.h>
#include <lorawan.h>

//ABP Credentials
const char *devAddr = "4b83986a";
const char *nwkSKey = "93df5e92cbaf1efc0000000000000000";
const char *appSKey = "000000000000000002e21425cc8ccfa2";


const unsigned long interval = 10000;    // 10 s interval to send message
unsigned long previousMillis = 0;  // will store last time message sent
unsigned int counter = 0;     // message counter

char myStr[50];
byte outStr[255];
byte recvStatus = 0;
int port, channel, freq;
bool newmessage = false;

const sRFM_pins RFM_pins = {
  .CS = PA4,
  .RST = PB0,
  .DIO0 = PA3,
  .DIO1 = PA0,
};

File myFile;
void setup() {
  // Open serial communications and wait for port to open:
  pinMode(PA2, OUTPUT);
  digitalWrite(PA2, HIGH);
  Serial.begin(115200);
  if (!lora.init()) {
    Serial.println("RFM95 not detected");
    delay(5000);
    return;
  }
  Serial.println("STARTING");

  // Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDeviceClass(CLASS_C);

  // Set Data Rate
  lora.setDataRate(SF12BW125);

  // Set FramePort Tx
  lora.setFramePortTx(5);

  // set channel to random
  lora.setChannel(MULTI);

  // Put ABP Key and DevAddress here
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);
  Serial.print("Initializing SD card...");
  if (!SD.begin(PA15)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.

}
void loop() {

  // Check interval overflow
  if (millis() - previousMillis > interval) {
    previousMillis = millis();


    byte myByte[] = {0xEE, 0xA2, 0x33, 0xD4, 0x55};
    Serial.print("Sending: ");
    for (int i = 0; i < sizeof(myByte); i++)
    {
      Serial.print(myByte[i]); Serial.print(" ");
    }
    Serial.println();


    lora.sendUplinkHex(myByte, sizeof(myByte), 0);
    port = lora.getFramePortTx();
    channel = lora.getChannel();
    freq = lora.getChannelFreq(channel);
    Serial.print(F("fport: "));    Serial.print(port); Serial.print(" ");
    Serial.print(F("Ch: "));    Serial.print(channel); Serial.print(" ");
    Serial.print(F("Freq: "));    Serial.print(freq); Serial.println(" ");

    // nothing happens after setup
    myFile = SD.open("log.txt", FILE_WRITE);
    // if the file opened okay, write to it:
    if (myFile) {
      Serial.print("Writing to log.txt...");
      myFile.println("This is a test file :)");
      myFile.println("testing 1, 2, 3.");
      for (int i = 0; i < sizeof(myByte); i++)
      {
       myFile.print(myByte[i]); myFile.print(" ");
      }
      myFile.println();
      // close the file:
      myFile.close();
      Serial.println("done.");
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening log.txt");
    }
    digitalWrite(PA5, LOW);
    delay(1000);
    digitalWrite(PA5, HIGH);

  }

  // Check Lora RX
  lora.update();

  recvStatus = lora.readDataByte(outStr);
  if (recvStatus) {
    newmessage = true;
    int counter = 0;
    port = lora.getFramePortRx();
    channel = lora.getChannelRx();
    freq = lora.getChannelRxFreq(channel);

    for (int i = 0; i < recvStatus; i++)
    {
      if (((outStr[i] >= 32) && (outStr[i] <= 126)) || (outStr[i] == 10) || (outStr[i] == 13))
        counter++;
    }
    if (port != 0)
    {
      if (counter == recvStatus)
      {
        Serial.print(F("Received String : "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(char(outStr[i]));
        }
      }
      else
      {
        Serial.print(F("Received Hex : "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(outStr[i], HEX); Serial.print(" ");
        }
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port); Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel); Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq); Serial.println(" ");
    }
    else
    {
      Serial.print(F("Received Mac Cmd : "));
      for (int i = 0; i < recvStatus; i++)
      {
        Serial.print(outStr[i], HEX); Serial.print(" ");
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port); Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel); Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq); Serial.println(" ");
    }
  }
}
