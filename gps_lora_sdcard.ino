#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <lorawan.h>
#include <SPI.h>
#include <SD.h>
File myFile;

#define Interval 15000
//#define Interval 5000
#define GPS_LED PB4
#define LED PB5
#define GPSEN PA2
#define BUTTON PB15
#define GPSRST PB1

#define GPSTX PB6 //pin number for GPS TX output - data from Arduino into GPS
#define GPSRX PB7 //pin number for GPS RX input - to Arduino from GPS
#define GPSBaud 9600 //GPS Baud rate
#define Serial_Monitor_Baud 115200   //this is baud rate used for the Arduino IDE Serial Monitor
SoftwareSerial GPSserial(GPSRX, GPSTX);
HardwareSerial SerialMon(PA10, PA9);


TinyGPSPlus gps;
String latitude, longitude, alt, m, d, y, hr, mn, sc;
String GPSData;
float vbatt;
bool gpsvalid, datevalid, timevalid, locating = true, buttonstate = true;
unsigned long T;
char gpschar;

const char *devAddr = "4b83986a";
const char *nwkSKey = "93df5e92cbaf1efc0000000000000000";
const char *appSKey = "000000000000000002e21425cc8ccfa2";

char myStr[50];

const sRFM_pins RFM_pins = {
  .CS = PA4,
  .RST = PB0,
  .DIO0 = PA3,
  .DIO1 = PA0,
};


void setup() {
  // put your setup code here, to run once:
  SerialMon.begin(Serial_Monitor_Baud);   //start Serial console ouput
  GPSserial.begin(GPSBaud);//start softserial for GPS at defined baud rate
  pinMode(LED, OUTPUT);
  pinMode(GPS_LED, OUTPUT);
  pinMode(GPSEN, OUTPUT);
  pinMode(GPSRST, OUTPUT);
  pinMode(BUTTON, INPUT);
  digitalWrite(LED, HIGH);
  digitalWrite(GPSEN, HIGH);

  if (!lora.init()) {
    SerialMon.println("  [LORA] RFM95 not detected");
    delay(5000);
    return;
  }

  // Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDeviceClass(CLASS_C);

  // Set Data Rate
  lora.setDataRate(0);

  // set channel to random
  lora.setChannel(MULTI);

  // Put ABP Key and DevAddress here
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);
  SerialMon.println("    [SD] Initializing SD card...");
  if (!SD.begin(PA15)) {
    SerialMon.println("    [SD] Initialize Failed");
  }
  else
    SerialMon.println("    [SD] Initialize Done");

}

void loop() {
  digitalWrite(GPS_LED, HIGH);
  while (GPSserial.available() > 0)
  {
    gpschar = GPSserial.read();
    SerialMon.write(gpschar);
    digitalWrite(GPS_LED, LOW);
    if (gps.encode(gpschar))
    {
      GetGPSData();
    }

  }
  if ((digitalRead(BUTTON) == HIGH) && (buttonstate == true))
  {
    buttonstate = false;
    digitalWrite(GPSRST, LOW);
    SerialMon.println("   [BTN] REELASED");
  }
  else if ((digitalRead(BUTTON) == LOW) && (buttonstate == false))
  {
    buttonstate = true;
    digitalWrite(GPSRST, HIGH);
    SerialMon.println("   [BTN] PRESSED");
  }
  if ((millis() - T) > Interval)
  {
    if (gpsvalid && datevalid && timevalid)
    {
      SerialMon.println();
      SerialMon.print("   [LAT] "); SerialMon.println(latitude);
      SerialMon.print("  [LONG] "); SerialMon.println(longitude);
      SerialMon.print("   [ALT] "); SerialMon.println(alt);
      SerialMon.print("  [DATE] "); SerialMon.print(d); SerialMon.print("/"); SerialMon.print(m); SerialMon.print("/"); SerialMon.println(y);
      SerialMon.print("  [TIME] "); SerialMon.print(hr); SerialMon.print(":"); SerialMon.print(mn); SerialMon.print(":"); SerialMon.println(sc);
      SerialMon.print("  [DATA] "); SerialMon.println(GPSData);
      SerialMon.print("  [SEND] ");

      String dataSend = "$," + GPSData + ",*";
      String dataDateTime = d + "/" + m + "/" + y + " " + hr + ":" + mn + ":" + sc;
      SerialMon.println(dataSend);
      dataSend.toCharArray(myStr, dataSend.length() + 1);
      lora.sendUplink(myStr, strlen(myStr), 0);
      myFile = SD.open("LOCATION.txt", FILE_WRITE);
      if (myFile) {
        SerialMon.println("    [SD] Writing GPS Location to LOCATION.txt");
        myFile.print(dataDateTime);myFile.print(" ");
        myFile.println(dataSend);
        myFile.close();
        SerialMon.println("    [SD] Writing GPS Location done!");
      } else {
        // if the file didn't open, print an error:
        SerialMon.println("    [SD] Error Opening LOCATION.txt");
      }
      digitalWrite(LED, LOW);
      delay(500);
      digitalWrite(LED, HIGH);
      delay(500);
      digitalWrite(LED, LOW);
      delay(500);
      digitalWrite(LED, HIGH);
      gpsvalid = false;
      datevalid = false;
      timevalid = false;
    }
    else
    {
      SerialMon.println("   [GPS] CANNOT GET LOCATION...");
      SerialMon.print("  [SEND] ");
      String dataSend = "$,NA,NA,NA," + String(vbatt) + ",*";
      SerialMon.println(dataSend);
      dataSend.toCharArray(myStr, dataSend.length() + 1);
      lora.sendUplink(myStr, strlen(myStr), 0);
      digitalWrite(LED, LOW);
      delay(2000);
      digitalWrite(LED, HIGH);
    }
    T = millis();
  }
  lora.update();
}

void GetGPSData()
{
  vbatt = (((analogRead(PA1)) * 3.9741) / (1024));
  if (gps.location.isValid())
  {
    gpsvalid = true;
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
    alt = String(gps.altitude.meters());
  }
  else
  {
    gpsvalid = false;
    if (gpsvalid == false && locating == true)
    {
      SerialMon.println(F("   [GPS] Locating..."));
      locating = false;
    }

  }
  if (gps.date.isValid())
  {
    datevalid = true;
    if (gps.date.month() < 10)
      m = "0" + String(gps.date.month());
    else
      m = String(gps.date.month());

    if (gps.date.day() < 10)
      d = "0" + String(gps.date.day());
    else
      d = String(gps.date.day());

    y = String(gps.date.year());
  }
  else
  {
    datevalid = false;
    SerialMon.println(F("   [GPS] Initializing Date..."));
  }
  if (gps.time.isValid())
  {
    timevalid = true;
    if ((gps.time.hour() + 7) < 10)
      hr = "0" + String(gps.time.hour() + 7);
    else
    {
      if ((gps.time.hour() + 7) > 24)
      {
        hr = "0" + String((gps.time.hour() + 7) - 24);
      }
      else
        hr = String(gps.time.hour() + 7);
    }


    if (gps.time.minute() < 10)
      mn = "0" + String(gps.time.minute());
    else
      mn = String(gps.time.minute());

    if (gps.time.second() < 10)
      sc = "0" + String(gps.time.second());
    else
      sc = String(gps.time.second());
  }
  else
  {
    timevalid = false;
    SerialMon.println(F("   [GPS] Initializing Time..."));
  }

  if (gpsvalid && datevalid && timevalid)
  {
    GPSData =  latitude + "," + longitude + "," + alt + "," + String(vbatt);
  }
  else
    GPSData = "No Data";

}
