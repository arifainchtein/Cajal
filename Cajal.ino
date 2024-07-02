#include "Arduino.h"
#include <LoRa.h>
#include <Timer.h>
#include <PCF8563TimeManager.h>
#include <SPI.h>
#include <Esp32SecretManager.h>
#include <FastLED.h>
#include <CajalWifiManager.h>
#include <DataManager.h>

//#include <TM1637TinyDisplay.h>
//#include "SevenSegmentTM1637.h"
//#include "SevenSegmentExtended.h"

#include <TM1637Display.h>
#include <Arduino_JSON.h>
#include <GloriaTankFlowPumpData.h>
#include <PanchoTankFlowData.h>
#include <LangleyData.h>
#include <CajalData.h>

#include <RosieData.h>
#include <sha1.h>
#include <totp.h>
#include "OneWire.h"
#include "DallasTemperature.h"
#include <GloriaTankFlowPumpSerializer.h>
#include <PanchoTankFlowSerializer.h>
#include <Wire.h>

DataManager dataManager;


String LIFE_CYCLE_EVENT_START_SYNCHRONOUS_CYCLE = "Start Synchronous Cycle";
String LIFE_CYCLE_EVENT_END_SYNCHRONOUS_CYCLE = "End Synchronous Cycle";
String LIFE_CYCLE_EVENT_START_ASYNCHRONOUS_CYCLE = "Start Asynchronous Cycle";
String LIFE_CYCLE_EVENT_END_ASYNCHRONOUS_CYCLE = "End Asynchronous Cycle";
String LIFE_CYCLE_EVENT_MOTHER_ALERT_WPS = "Mother Alert WPS";
String LIFE_CYCLE_EVENT_END_AWAKE = "End Awake";
String LIFE_CYCLE_EVENT_START_AWAKE = "Start Awake";
String COMMAND_REBOOT = "$Reboot";
String COMMAND_SHUTDOWN = "$Shutdown";
String USER_COMMAND = "UserCommand";
#define LED_TIMER_SECONDS 10
Timer ledTimer(LED_TIMER_SECONDS);
uint8_t currentLedSecond = 0;
bool gloriaTankFlowPumpNewData = false;
bool panchoTankFlowDataNewData = false;
// Variables will change:
int piControlLastState = LOW;  // the previous state from the input pin
int piControlCurrentState;     // the current reading from the input pin
unsigned long piControlPressedTime = 0;
unsigned long piControlReleasedTime = 0;
#define PI_CONTROL 39

#define PI_CONTROL_SHORT_PRESS_TIME 1000  //  milliseconds
#define PI_CONTROL_LONG_PRESS_TIME 3000   //  milliseconds

String userCommandState = "Ok";
//
// synchronous =0
// asynchroous =1
#define PI_STATE_SYNC 0
#define PI_STATE_ASYNC 1
#define PI_STATE_UNDEFINED 2
uint8_t piCurrentState;

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the lcd address to 0x27 for a 16 chars and 2 line display

OneWire oneWire(27);
DallasTemperature tempSensor(&oneWire);
uint8_t serialnumber[8];
#define SCK 14
#define MOSI 13
#define MISO 12
#define LoRa_SS 15
#define LORA_RESET 16
#define LORA_DI0 17

#define UI_CLK 33
#define UI2_DAT 25
#define UI3_DAT 26
#define UI4_DAT 5
#define UI5_DAT 18
#define UI6_DAT 23
#define UI7_DAT 32
#define RTC_BATT_VOLT 36
#define LED_PIN 19
#define NUM_LEDS 16
#define OP_MODE 34

Timer dsUploadTimer(60);
bool timeSet = false;
GloriaTankFlowPumpSerializer gloriaTankFlowPumpSerializer;
PanchoTankFlowSerializer panchoTankFlowSerializer;


PanchoConfigData panchoConfigData;
PanchoTankFlowData panchoTankFlowData;
CajalData cajalData;

String serialNumber;
uint8_t delayTime = 10;

TM1637Display display1(UI_CLK, UI2_DAT);
TM1637Display display2(UI_CLK, UI5_DAT);
TM1637Display display3(UI_CLK, UI3_DAT);
TM1637Display display4(UI_CLK, UI6_DAT);
TM1637Display display5(UI_CLK, UI4_DAT);
TM1637Display display6(UI_CLK, UI7_DAT);

 
#define FUN_1_FLOW 1
#define FUN_2_FLOW 2
#define FUN_1_FLOW_1_TANK 3
#define FUN_1_TANK 4
#define FUN_2_TANK 5

CRGB leds[NUM_LEDS];
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
bool isHost = true;
#define UNIQUE_ID_SIZE 8
#define RTC_CLK_OUT 4


String stationName;
uint8_t secondsSinceLastDataSampling = 0;
PCF8563TimeManager timeManager(Serial);
GeneralFunctions generalFunctions;
Esp32SecretManager secretManager(timeManager);

CajalWifiManager wifiManager(Serial,dataManager, timeManager, secretManager, cajalData, panchoConfigData);
bool wifiActive = false;
bool apActive = false;

int badPacketCount = 0;
byte msgCount = 0;         // count of outgoing messages
byte localAddress = 0xFF;  // address of this device
byte destination = 0xAA;   // destination to send to


long lastPulseTime = 0;
uint8_t uniqueId[UNIQUE_ID_SIZE];
long lastMillis;
uint8_t SECONDOFFSET = 10;
uint8_t timeZoneHours = 10;
static byte monthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
String currentIpAddress = "No IP";
bool inPulse = true;
String ipAddress = "";

bool internetAvailable;
bool loraActive = false;
bool inSerial = false;

//
// for the different devices
//
bool uploadRosieToDS=false;

long lastTimeUpdateMillis = 0;
RTCInfoRecord currentTimerRecord;
#define TIME_RECORD_REFRESH_SECONDS 3
volatile bool clockTicked = false;
volatile bool loraReceived = false;
volatile int loraPacketSize = 0;
struct DisplayData {
  int value;
  int dp;
} displayData;


RTCInfoRecord lastReceptionRTCInfoRecord;

RosieConfigData rosieConfigData;

GloriaTankFlowPumpData gloriaTankFlowPumpData;
RosieData rosieData;
LangleyData langleyData;


JSONVar jsonData;


CRGBPalette16 currentPalette;
TBlendType currentBlending;

const uint8_t langleyname[] = {
  SEG_D | SEG_E | SEG_F,                          // L
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,  // A
  SEG_C | SEG_E | SEG_G,                          // n
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F           // iG
};

const uint8_t glorianame[] = {
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // G
  SEG_D | SEG_E | SEG_F,                          // L
  SEG_F | SEG_D | SEG_A | SEG_B | SEG_C | SEG_E,  // O
  SEG_E | SEG_G                                   // r

};


const uint8_t panchoname[] = {
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,          // P
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,  // A
  SEG_C | SEG_E | SEG_G,                          // n
  SEG_D | SEG_E | SEG_G                           // c

};

const uint8_t rosiename[] = {
  SEG_E | SEG_G,                                  // r
  SEG_F | SEG_D | SEG_A | SEG_B | SEG_C | SEG_E,  // O
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,          // s
  SEG_E | SEG_F                                   // i

};

const uint8_t flow[] = {

    SEG_F | SEG_G | SEG_A | SEG_E,                  // F
    SEG_F | SEG_E | SEG_D,                          // L
    SEG_F | SEG_D | SEG_C | SEG_B | SEG_A | SEG_E,  // o
    SEG_F | SEG_D | SEG_C | SEG_B | SEG_E           // u
  };

const uint8_t flow2[] = {

    SEG_F | SEG_G | SEG_A | SEG_E,                  // F
    SEG_F | SEG_E | SEG_D,                          // L
    SEG_F | SEG_G | SEG_A | SEG_E,                  // F
    SEG_F | SEG_E | SEG_D                          // L
  };

const uint8_t flowtank[] = {

    SEG_F | SEG_G | SEG_A | SEG_E,                  // F
    SEG_F | SEG_E | SEG_D,                          // L
    SEG_F | SEG_G | SEG_D | SEG_E,                  // t
    SEG_C | SEG_D | SEG_E | SEG_B | SEG_A | SEG_G  // a
  };

  const uint8_t tank[] = {

    SEG_F | SEG_G | SEG_D | SEG_E,                  // t
    SEG_C | SEG_D | SEG_E | SEG_B | SEG_A | SEG_G,  // a
    SEG_C | SEG_E | SEG_G,                          // n
    SEG_G | SEG_D | SEG_E                           // c
  };

  const uint8_t tank2[] = {

    SEG_F | SEG_G | SEG_D | SEG_E,                  // t
    SEG_C | SEG_D | SEG_E | SEG_B | SEG_A | SEG_G,  // a
    SEG_F | SEG_G | SEG_D | SEG_E,                  // t
    SEG_C | SEG_D | SEG_E | SEG_B | SEG_A | SEG_G,  // a
  };


  
//
// Lora Functions
//
void sendMessage(String outgoing) {
  LoRa.beginPacket();             // start packet
  LoRa.write(destination);        // add destination address
  LoRa.write(localAddress);       // add sender address
  LoRa.write(msgCount);           // add message ID
  LoRa.write(outgoing.length());  // add payload length
  LoRa.print(outgoing);           // add payload
  LoRa.endPacket();               // finish packet and send it
  msgCount++;                     // increment message ID
}

void LoRa_rxMode() {
  LoRa.disableInvertIQ();  // normal mode
  LoRa.receive();          // set receive mode
}

void LoRa_txMode() {
  LoRa.idle();             // set standby mode
  LoRa.disableInvertIQ();  // normal mode
}



void onReceive(int packetSize) {
  loraReceived = true;
  loraPacketSize = packetSize;
}


void processLora(int packetSize) {



  // display1.clear();
  // display2.clear();
  // display3.clear();
  // display4.clear();
  // display5.clear();
  // display6.clear();

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("rec lora:");
  lcd.print(packetSize);

  if (packetSize == 0) return;  // if there's no packet, return
  if (inSerial) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("In Serial");
    return;
  }
  if (!timeSet) {
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print("currentTimerRecord is null");
    return;
  }
  int hour = currentTimerRecord.hour;
  int minute = currentTimerRecord.minute;

  int displaytime = (hour * 100) + minute;
  if (packetSize == sizeof(LangleyData)) {
    LoRa.readBytes((uint8_t*)&langleyData, sizeof(LangleyData));
    long messageReceivedTime = timeManager.getCurrentTimeInSeconds(currentTimerRecord);
    lastReceptionRTCInfoRecord.year = currentTimerRecord.year;
    lastReceptionRTCInfoRecord.month = currentTimerRecord.month;
    lastReceptionRTCInfoRecord.date = currentTimerRecord.date;
    lastReceptionRTCInfoRecord.hour = currentTimerRecord.hour;
    lastReceptionRTCInfoRecord.minute = currentTimerRecord.minute;
    lastReceptionRTCInfoRecord.second = currentTimerRecord.second;

    langleyData.rssi = LoRa.packetRssi();
    langleyData.snr = LoRa.packetSnr();

    int startValue = 4;
    int endValue = 8;
    for (int i = startValue; i < endValue; i++) {
      if (langleyData.capacitorVoltage > 4.5) {
        leds[i] = CRGB(0, 0, 255);
      } else if (langleyData.capacitorVoltage > 3.75 && langleyData.capacitorVoltage < 4.49) {
        leds[i] = CRGB(0, 255, 0);
      } else if (langleyData.capacitorVoltage > 3.25 && langleyData.capacitorVoltage < 3.74) {
        leds[i] = CRGB(255, 255, 0);
      } else if (langleyData.capacitorVoltage < 3.25) {
        leds[i] = CRGB(255, 0, 0);
      }
    }
    FastLED.show();
    delay(1000);
    for (int i = 0; i < 8; i++) {
      leds[i] = CRGB(0, 0, 0);
    }
    FastLED.show();
    int hour = currentTimerRecord.hour;
    int minute = currentTimerRecord.minute;
    int displaytime = (hour * 100) + minute;

  } else if (packetSize == sizeof(RosieData)) {
    LoRa.readBytes((uint8_t*)&rosieData, sizeof(RosieData));

    long messageReceivedTime = timeManager.getCurrentTimeInSeconds(currentTimerRecord);
    lastReceptionRTCInfoRecord.year = currentTimerRecord.year;
    lastReceptionRTCInfoRecord.month = currentTimerRecord.month;
    lastReceptionRTCInfoRecord.date = currentTimerRecord.date;
    lastReceptionRTCInfoRecord.hour = currentTimerRecord.hour;
    lastReceptionRTCInfoRecord.minute = currentTimerRecord.minute;
    lastReceptionRTCInfoRecord.second = currentTimerRecord.second;

    rosieData.rssi = LoRa.packetRssi();
    rosieData.snr = LoRa.packetSnr();

    lcd.setCursor(0, 2);
    lcd.print(rosieData.devicename);
    rosieData.rssi = LoRa.packetRssi();
    rosieData.snr = LoRa.packetSnr();
    lcd.setCursor(0, 3);
    lcd.print("sn:");
    lcd.print(rosieData.snr);
    lcd.print("  rs:");
    lcd.print(rosieData.rssi);


    // display1.setSegments(rosiename, 4, 0);
    // display2.clear();
    // display3.showNumberDec(rosieData.flowRate, false);

    // int liters = rosieData.totalMilliLitres / 1000.0;
    // int displayD;
    // if (rosieData.totalMilliLitres < 1000) {
    //   //   display4.showNumberDec(rosieData.totalMilliLitres, false);
    // } else {
    //   if (liters > 1000000) {
    //     displayD = 100 * liters / 1000000;
    //     //   display4.showNumberDecEx(displayD, (0x80 >> 1), false);
    //   } else if (liters > 100000) {
    //     displayD = 100 * liters / 100000;
    //     //    display4.showNumberDecEx(displayD, (0x80 >> 1), false);
    //   } else if (liters > 10000) {
    //     displayD = 100 * liters / 10000;
    //     //     display4.showNumberDecEx(displayD, (0x80 >> 1), false);
    //   } else if (liters < 10000) {
    //     displayD = (int)liters;
    //     //   display4.showNumberDec(displayD, false);
    //   }
    // }


    // display5.showNumberDec(rosieData.flowRate, false);
    // liters = rosieData.totalMilliLitres2 / 1000.0;
    // if (rosieData.totalMilliLitres2 < 1000) {
    //   display6.showNumberDec(rosieData.totalMilliLitres2, false);
    // } else {
    //   if (liters > 1000000) {
    //     displayD = 100 * liters / 1000000;
    //     //           display6.showNumberDecEx(displayD, (0x80 >> 1), false);
    //   } else if (liters > 100000) {
    //     displayD = 100 * liters / 100000;
    //     //   display6.showNumberDecEx(displayD, (0x80 >> 1), false);
    //   } else if (liters > 10000) {
    //     displayD = 100 * liters / 10000;
    //     //    display6.showNumberDecEx(displayD, (0x80 >> 1), false);
    //   } else if (liters < 10000) {
    //     displayD = (int)liters;
    //     //   display6.showNumberDec(displayD, false);
    //   }
    // }

    // int startValue = 0;
    // int endValue = 4;


    // for (int i = startValue; i < endValue; i++) {
    //   if (rosieData.capacitorVoltage > 4.5) {

    //     leds[i] = CRGB(0, 0, 255);
    //   } else if (rosieData.capacitorVoltage > 3.75 && rosieData.capacitorVoltage < 4.49) {

    //     leds[i] = CRGB(0, 255, 0);
    //   } else if (rosieData.capacitorVoltage > 3.25 && rosieData.capacitorVoltage < 3.74) {

    //     leds[i] = CRGB(255, 255, 0);
    //   } else if (rosieData.capacitorVoltage < 3.25) {
    //     leds[i] = CRGB(255, 0, 0);
    //   }
    // }
    // FastLED.show();
    // delay(1000);
    // for (int i = 0; i < 8; i++) {
    //   leds[i] = CRGB(0, 0, 0);
    // }

    // FastLED.show();


    int hour = currentTimerRecord.hour;
    int minute = currentTimerRecord.minute;
  } else if (packetSize == sizeof(GloriaTankFlowPumpData)) {
    memset(&gloriaTankFlowPumpData, 0, sizeof(GloriaTankFlowPumpData));
    LoRa.readBytes((uint8_t*)&gloriaTankFlowPumpData, sizeof(GloriaTankFlowPumpData));
    lcd.setCursor(0, 2);
    lcd.print(gloriaTankFlowPumpData.devicename);
    gloriaTankFlowPumpData.rssi = LoRa.packetRssi();
    gloriaTankFlowPumpData.snr = LoRa.packetSnr();
    lcd.setCursor(0, 3);
    lcd.print("sn:");
    lcd.print(gloriaTankFlowPumpData.snr);
    lcd.print("  rs:");
    lcd.print(gloriaTankFlowPumpData.rssi);

    long messageReceivedTime = timeManager.getCurrentTimeInSeconds(currentTimerRecord);
    lastReceptionRTCInfoRecord.year = currentTimerRecord.year;
    lastReceptionRTCInfoRecord.month = currentTimerRecord.month;
    lastReceptionRTCInfoRecord.date = currentTimerRecord.date;
    lastReceptionRTCInfoRecord.hour = currentTimerRecord.hour;
    lastReceptionRTCInfoRecord.minute = currentTimerRecord.minute;
    lastReceptionRTCInfoRecord.second = currentTimerRecord.second;


    gloriaTankFlowPumpData.secondsTime = messageReceivedTime;
    // display5.setSegments(glorianame, 4, 0);
    // display6.setSegments(glorianame, 4, 0);

    // display1.setSegments(glorianame, 4, 0);
    // display2.setSegments(glorianame, 4, 0);
    // display3.setSegments(glorianame, 4, 0);
    // display4.setSegments(glorianame, 4, 0);

    gloriaTankFlowPumpNewData = true;


  } else if (packetSize == sizeof(PanchoTankFlowData)) {
    memset(&panchoTankFlowData, 0, sizeof(PanchoTankFlowData));
    LoRa.readBytes((uint8_t*)&panchoTankFlowData, sizeof(PanchoTankFlowData));
    lcd.setCursor(0, 2);
    lcd.print(panchoTankFlowData.devicename);
    long messageReceivedTime = timeManager.getCurrentTimeInSeconds(currentTimerRecord);
    lastReceptionRTCInfoRecord.year = currentTimerRecord.year;
    lastReceptionRTCInfoRecord.month = currentTimerRecord.month;
    lastReceptionRTCInfoRecord.date = currentTimerRecord.date;
    lastReceptionRTCInfoRecord.hour = currentTimerRecord.hour;
    lastReceptionRTCInfoRecord.minute = currentTimerRecord.minute;
    lastReceptionRTCInfoRecord.second = currentTimerRecord.second;


    //  p.secondsTime=messageReceivedTime;
    panchoTankFlowData.rssi = LoRa.packetRssi();
    panchoTankFlowData.snr = LoRa.packetSnr();
    lcd.setCursor(0, 3);
    lcd.print("sn:");
    lcd.print(panchoTankFlowData.snr);
    lcd.print("  rs:");
    lcd.print(panchoTankFlowData.rssi);
    // display5.setSegments(panchoname, 4, 0);
    // display6.setSegments(panchoname, 4, 0);

    // display1.setSegments(panchoname, 4, 0);
    // display2.setSegments(panchoname, 4, 0);
    // display3.setSegments(panchoname, 4, 0);
    // display4.setSegments(panchoname, 4, 0);


    int startValue = 0;
    int endValue = 4;



    for (int i = startValue; i < endValue; i++) {
      if (panchoTankFlowData.flowRate > 0) {
        leds[i] = CRGB(0, 0, 255);
      } else {
        leds[i] = CRGB(0, 0, 0);
      }
    }
    FastLED.show();
    for (int i = 0; i < 8; i++) {
      leds[i] = CRGB(0, 0, 0);
    }

    FastLED.show();


    panchoTankFlowDataNewData = true;

  } else {
    badPacketCount++;
    lcd.print("  receive bad data size= ");
    lcd.print(packetSize);
  }
}



//
// end of lora functions
//
//
// interrupt functions
//


void IRAM_ATTR clockTick() {
  portENTER_CRITICAL_ISR(&mux);
  clockTicked = true;
  portEXIT_CRITICAL_ISR(&mux);
}

//
// end of interrupt functions
//




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(RTC_CLK_OUT, INPUT_PULLUP);
  tempSensor.begin();
  uint8_t address[8];
  lcd.setCursor(0, 0);
  if (tempSensor.getAddress(address, 0)) {

    tempSensor.getAddress(address, 0);
    for (uint8_t i = 0; i < 8; i++) {
      //if (address[i] < 16) Serial.print("0");
      serialNumber += String(address[i], HEX);
    }

    lcd.print("sn:");
    lcd.print(serialNumber);
    memcpy(cajalData.serialnumberarray, address, sizeof(address));

    // for (int i = 0; i < sizeof(cajalData.serialnumberarray); i++) {
    //   if (cajalData.serialnumberarray[i] != NULL) lcd.print(cajalData.serialnumberarray[i], HEX);
    // }


  } else {
    lcd.print(F("Error fetching the temp sensor address"));
    return;
  }

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  //  for(int i=0;i<NUM_LEDS;i++){
  //      leds[i] = CRGB(255, 255, 0);
  //   }
  //    FastLED.show();
  timeManager.start();
  timeManager.PCF8563osc1Hz();
  pinMode(RTC_CLK_OUT, INPUT_PULLUP);  // set up interrupt pin
  digitalWrite(RTC_CLK_OUT, HIGH);     // turn on pullup resistors
  // attach interrupt to set_tick_tock callback on rising edge of INT0
  attachInterrupt(digitalPinToInterrupt(RTC_CLK_OUT), clockTick, RISING);

  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);
  display3.setBrightness(0x0f);
  display4.setBrightness(0x0f);
  display5.setBrightness(0x0f);
  display6.setBrightness(0x0f);
  display1.clear();
  display2.clear();
  display3.clear();
  display4.clear();
  display5.clear();
  display6.clear();

  pinMode(RTC_BATT_VOLT, INPUT);
  pinMode(OP_MODE, INPUT_PULLUP);

  //wifiManager.restartWifi();

  //
  // lora code
  //
  SPI.begin(SCK, MISO, MOSI);
  pinMode(LoRa_SS, OUTPUT);
  pinMode(LORA_RESET, OUTPUT);
  pinMode(LORA_DI0, INPUT);
  digitalWrite(LoRa_SS, HIGH);
  delay(100);
  LoRa.setPins(LoRa_SS, LORA_RESET, LORA_DI0);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  leds[0] = CRGB(255, 255, 0);
  leds[1] = CRGB(255, 255, 0);
  leds[2] = CRGB(255, 255, 0);
  FastLED.show();
  const uint8_t lora[] = {
    SEG_F | SEG_E | SEG_D,                         // L
    SEG_E | SEG_G | SEG_C | SEG_D,                 // o
    SEG_E | SEG_G,                                 // r
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G  // A
  };

  const uint8_t on[] = {
    SEG_E | SEG_G | SEG_C | SEG_D,  // o
    SEG_C | SEG_E | SEG_G           // n
  };

  const uint8_t off[] = {
    SEG_E | SEG_G | SEG_C | SEG_D,  // o
    SEG_A | SEG_G | SEG_E | SEG_F,  // F
    SEG_A | SEG_G | SEG_E | SEG_F   // F
  };

  display1.setSegments(lora, 4, 0);
  lcd.setCursor(0, 1);
  if (!LoRa.begin(433E6)) {
    lcd.print("Starting LoRa failed!");
    while (1)
      ;
    leds[1] = CRGB(255, 0, 0);
    display2.setSegments(off, 3, 1);
  } else {
    lcd.print("Starting LoRa worked!");
    //leds[1] = CRGB(0, 0, 255);
    display2.setSegments(on, 2, 2);
    loraActive = true;
  }
  String grp = secretManager.getGroupIdentifier();
  char gprid[grp.length()];
  grp.toCharArray(gprid, grp.length());
  strcpy(cajalData.groupidentifier, gprid);
  lcd.setCursor(0, 2);
  lcd.print("Starting wifi gi=");
  lcd.setCursor(0, 3);
  lcd.print(cajalData.groupidentifier);

  String identifier = "PanchoV";
  char ty[identifier.length() + 1];
  identifier.toCharArray(ty, identifier.length() + 1);
  strcpy(cajalData.deviceTypeId, ty);

  panchoConfigData.fieldId = secretManager.getFieldId();

  // FastLED.show();
  delay(1000);

  wifiManager.start();
  bool stationmode = wifiManager.getStationMode();
  lcd.setCursor(0, 3);

  lcd.print("stationmode=");
  lcd.print(stationmode);

  //  serialNumber = wifiManager.getMacAddress();
  wifiManager.setSerialNumber(serialNumber);
  wifiManager.setLora(loraActive);
  String ssid = wifiManager.getSSID();

  if (stationmode) {
    ipAddress = wifiManager.getIpAddress();
    lcd.setCursor(0, 2);
    lcd.print("ipaddress=");
    lcd.setCursor(0, 3);
    lcd.print(ipAddress);

    if (ipAddress == "" || ipAddress == "0.0.0.0") {
      for (int i = 0; i < 4; i++) {
        leds[i] = CRGB(255, 0, 0);
      }
      for (int i = 4; i < 8; i++) {
        leds[i] = CRGB(255, 255, 0);
      }
      FastLED.show();
      setApMode();

    } else {
      setStationMode(ipAddress);
    }
  } else {
    setApMode();
  }
  internetAvailable = wifiManager.getInternetAvailable();
  lcd.setCursor(0, 3);
  lcd.print("internet avail=");
  lcd.print(internetAvailable);
 Serial.print(F("internet avail="));
Serial.println(internetAvailable);
  if (loraActive) {
    leds[1] = CRGB(0, 0, 255);
    // LoRa_rxMode();
    // LoRa.setSyncWord(0xF3);
    LoRa.onReceive(onReceive);
    // put the radio into receive mode
    LoRa.receive();

  } else {
    leds[1] = CRGB(0, 0, 0);
  }
  FastLED.show();

  dsUploadTimer.start();




  //
  // end of lora code
  //
  pinMode(PI_CONTROL, INPUT);
  piCurrentState = PI_STATE_UNDEFINED;

  ledTimer.start();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ok-Ready");
}

bool doit = true;


void loop() {
  // put your main code here, to run repeatedly:
  bool opmode = false;
  int hour, minute, second;
  // put your main code here, to run repeatedly:
  if (clockTicked) {
    portENTER_CRITICAL(&mux);
    clockTicked = false;
    portEXIT_CRITICAL(&mux);
    uint16_t ledTimerCurrent = ledTimer.tick();

    secondsSinceLastDataSampling++;
    currentTimerRecord = timeManager.now();
    timeSet = true;
    wifiManager.setCurrentTimerRecord(currentTimerRecord);
    uint8_t dsUploadTimerCounter= dsUploadTimer.tick();
 //    Serial.print("dsUploadTimerCounter=");
 // Serial.print(dsUploadTimerCounter);
 //  Serial.print("   stastus=");
 // Serial.println(dsUploadTimer.status());
    opmode = digitalRead(OP_MODE);
    bool weatherStationMode = false;
    tempSensor.requestTemperatures();
    float tempC = tempSensor.getTempCByIndex(0);
    hour = currentTimerRecord.hour;
    minute = currentTimerRecord.minute;
    second = currentTimerRecord.second;
    lcd.setCursor(0, 0);
    lcd.print(hour);
    lcd.print(":");
    lcd.print(minute);
    lcd.print(":");
    lcd.print(second);
    lcd.print(" ");
    lcd.print(currentLedSecond);

    if (ledTimer.status()) {
      currentLedSecond++;
      if (currentLedSecond < 8) {
        for (int i = 4; i < (currentLedSecond + 4); i++) {
          if (piCurrentState == PI_STATE_ASYNC) {
            leds[i] = CRGB(0, 255, 0);
          } else if (piCurrentState == PI_STATE_SYNC) {
            leds[i] = CRGB(0, 0, 255);
          } else if (piCurrentState == PI_STATE_UNDEFINED) {
            leds[i] = CRGB(255, 255, 0);
          }
        }
        FastLED.show();
        delay(100);
      } else {
        currentLedSecond = 0;
        for (int i = 4; i < 12; i++) {
          leds[i] = CRGB(0, 0, 0);
        }
        FastLED.show();
      }
      ledTimer.reset();
    }

  }


  if (loraReceived) {
    processLora(loraPacketSize);
    loraReceived = false;
    bool show = false;
    currentPalette = RainbowStripeColors_p;
    currentBlending = NOBLEND;
    if (!inSerial) performLedShow(500);
    // display4.clear();
    // display5.clear();
    // display6.clear();
    // delay(1000);
    if (loraPacketSize == sizeof(LangleyData)) {
       lcd.setCursor(0, 2);
       lcd.print(F("Received "));
       lcd.print(langleyname);
    
      display1.setSegments(langleyname, 4, 0);
      display2.showNumberDec(2222, false);
      display3.showNumberDec(rosieData.capacitorVoltage, false);
      display4.showNumberDec(rosieData.solarVoltage, false);

      display5.showNumberDec(gloriaTankFlowPumpData.snr, false);
      display6.showNumberDec(gloriaTankFlowPumpData.rssi, false);

    } else if (loraPacketSize == sizeof(RosieData)) {
      uploadRosieToDS=true;
      display1.setSegments(rosiename, 4, 0);
      if (rosieData.currentFunctionValue == FUN_1_FLOW){
        display2.setSegments(flow, 4, 0);
        
      } else if ( rosieData.currentFunctionValue == FUN_2_FLOW){
        display2.setSegments(flow2, 4, 0);
      } else if ( rosieData.currentFunctionValue == FUN_1_FLOW_1_TANK){
        display2.setSegments(flowtank, 4, 0);
      } else if ( rosieData.currentFunctionValue == FUN_1_TANK){
        display2.setSegments(tank, 4, 0);
      } else if (rosieData.currentFunctionValue == FUN_2_TANK) {
        display2.setSegments(tank2, 4, 0);
      }
      display3.showNumberDec(rosieData.capacitorVoltage, false);
      display4.showNumberDec(rosieData.solarVoltage, false);
      display5.showNumberDec(rosieData.flowRate, false);

      int liters = rosieData.totalMilliLitres / 1000.0;
      int displayD;
      if (rosieData.totalMilliLitres < 1000) {
          display6.showNumberDec(rosieData.totalMilliLitres, false);
      } else {
        if (liters > 1000000) {
          displayD = 100 * liters / 1000000;
            display6.showNumberDecEx(displayD, (0x80 >> 1), false);
        } else if (liters > 100000) {
          displayD = 100 * liters / 100000;
            display6.showNumberDecEx(displayD, (0x80 >> 1), false);
        } else if (liters > 10000) {
          displayD = 100 * liters / 10000;
          display6.showNumberDecEx(displayD, (0x80 >> 1), false);
        } else if (liters < 10000) {
          displayD = (int)liters;
          display6.showNumberDec(displayD, false);
        }
      }
      //
      // push to digitalstables
      //
     
    } else if (loraPacketSize == sizeof(GloriaTankFlowPumpData)) {
      display1.setSegments(glorianame, 4, 0);
      display2.showNumberDec(hour, false);
      display3.showNumberDec(minute, false);

      int rtcBatVolti = processDisplayValue(gloriaTankFlowPumpData.rtcBatVolt, &displayData);
      int dp4 = displayData.dp;
      if (dp4 > 0) {
        display4.showNumberDecEx(rtcBatVolti, (0x80 >> dp4), false);
      } else {
        display4.showNumberDec(rtcBatVolti, false);
      }

      int snri = processDisplayValue(gloriaTankFlowPumpData.snr, &displayData);
      int dp5 = displayData.dp;
      if (dp5 > 0) {
        display5.showNumberDecEx(snri, (0x80 >> dp5), false);
      } else {
        display5.showNumberDec(snri, false);
      }

      int rssii = processDisplayValue(gloriaTankFlowPumpData.rssi, &displayData);
      int dp6 = displayData.dp;
      if (dp6 > 0) {
        display6.showNumberDecEx(rssii, (0x80 >> dp6), false);
      } else {
        display6.showNumberDec(rssii, false);
      }
    } else if (loraPacketSize == sizeof(PanchoTankFlowData)) {
      show = true;
      display1.setSegments(panchoname, 4, 0);
      display2.showNumberDec(hour, false);
      display3.showNumberDec(second, false);

      int rtcBatVolti = processDisplayValue(panchoTankFlowData.rtcBatVolt, &displayData);
      int dp4 = displayData.dp;
      if (dp4 > 0) {
        display4.showNumberDecEx(rtcBatVolti, (0x80 >> dp4), false);
      } else {
        display4.showNumberDec(rtcBatVolti, false);
      }

      int snri = processDisplayValue(panchoTankFlowData.snr, &displayData);
      int dp5 = displayData.dp;
      if (dp5 > 0) {
        display5.showNumberDecEx(snri, (0x80 >> dp5), false);
      } else {
        display5.showNumberDec(snri, false);
      }

      int rssii = processDisplayValue(panchoTankFlowData.rssi, &displayData);
      int dp6 = displayData.dp;
      if (dp6 > 0) {
        display6.showNumberDecEx(rssii, (0x80 >> dp6), false);
      } else {
        display6.showNumberDec(rssii, false);
      }

      //  delay(500);
    }
    // currentPalette = RainbowStripeColors_p;
    //  currentBlending = NOBLEND;
    //   if(!inSerial && show)performLedShow(100);
  }
  loraPacketSize = 0;


  if (dsUploadTimer.status() && internetAvailable) {
    //char secret[27];
    //   lcd.print("Uploading to digitalstables");
    Serial.println(F("Uploading to digitalstables"));
    String secret = "J5KFCNCPIRCTGT2UJUZFSMQK";

    leds[2] = CRGB(0, 255, 0);
    FastLED.show();

    TOTP totp = TOTP(secret.c_str());
    char totpCode[7];  //get 6 char code

    long timeVal = timeManager.getCurrentTimeInSeconds(currentTimerRecord);
    cajalData.secondsTime = timeVal;
    long code = totp.gen_code(timeVal);

    //gloriaTankFlowPumpData.dsLastUpload=timeVal;

    wifiManager.setCurrentToTpCode(code);
     uint16_t response=0;
    if(uploadRosieToDS){
      response = wifiManager.uploadRosieDataToDigitalStables(rosieData);
      uploadRosieToDS=false;
    }
     
    //int response = wifiManager.uploadDataToDigitalStables();

    if (response == 200) {
      leds[2] = CRGB(0, 0, 255);
    } else if (response == 500) {
      leds[2] = CRGB(255, 0, 255);
    } else if (response == 404) {
      leds[2] = CRGB(255, 0, 255);
    } else {
      leds[2] = CRGB(0, 0, 0);
    }
    FastLED.show();

    dsUploadTimer.reset();
  }

  if (Serial.available() != 0) {
    inSerial = true;

    LoRa_txMode();
    String command = Serial.readString();
    lcd.setCursor(0, 2);
    lcd.print(F("cmd="));
    lcd.print(command);

    if (command == USER_COMMAND) {
      Serial.println(F("Ok-"));
    } else if (command == LIFE_CYCLE_EVENT_START_AWAKE) {
      Serial.print(F("Ok-"));
      Serial.println(LIFE_CYCLE_EVENT_START_AWAKE);
      currentPalette = RainbowStripeColors_p;
      currentBlending = NOBLEND;
      performLedShow(500);
    } else if (command == LIFE_CYCLE_EVENT_END_AWAKE) {
      // currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND;
      // performLedShow(500);
      Serial.print(F("Ok-"));
      Serial.println(LIFE_CYCLE_EVENT_END_AWAKE);
    } else if (command == LIFE_CYCLE_EVENT_START_SYNCHRONOUS_CYCLE) {
      Serial.print(F("Ok-"));
      Serial.println(LIFE_CYCLE_EVENT_START_SYNCHRONOUS_CYCLE);
      piCurrentState = PI_STATE_SYNC;
      ledTimer.reset();
      currentLedSecond = 0;
      userCommandState = "Ok";
      SetupPurpleAndGreenPalette();
      currentBlending = LINEARBLEND;
      performLedShow(500);
      for (int i = 12; i < 16; i++) {
        leds[i] = CRGB(0, 0, 255);
      }
      FastLED.show();
    } else if (command == LIFE_CYCLE_EVENT_END_SYNCHRONOUS_CYCLE) {

      // for (int i = 12; i < 16; i++) {
      //   leds[i] = CRGB(0, 0, 0);
      // }
      // FastLED.show();
      SetupPurpleAndGreenPalette();
      currentBlending = LINEARBLEND;
      performLedShow(500);
      Serial.print(F("Ok-"));
      Serial.println(LIFE_CYCLE_EVENT_END_SYNCHRONOUS_CYCLE);
    } else if (command == LIFE_CYCLE_EVENT_START_ASYNCHRONOUS_CYCLE) {
      Serial.print(F("Ok-"));
      Serial.println(LIFE_CYCLE_EVENT_START_ASYNCHRONOUS_CYCLE);
      // ledTimer.reset();
      currentLedSecond = 0;
      piCurrentState = PI_STATE_ASYNC;
      SetupBlackAndWhiteStripedPalette();
      currentBlending = NOBLEND;
      performLedShow(500);
      for (int i = 12; i < 16; i++) {
        leds[i] = CRGB(0, 255, 0);
      }
      FastLED.show();
    } else if (command == LIFE_CYCLE_EVENT_END_ASYNCHRONOUS_CYCLE) {
      // for (int i = 12; i < 16; i++) {
      //   leds[i] = CRGB(0, 0, 0);
      // }
      // FastLED.show();
      Serial.print(F("Ok-"));
      Serial.println(LIFE_CYCLE_EVENT_END_ASYNCHRONOUS_CYCLE);
    } else if (command.startsWith("Ping")) {
      Serial.println(F("Ok-Ping"));
      performLedShow(500);
    } else if (command.startsWith("ScanNetworks")) {
      wifiManager.scanNetworks();
    } else if (command.startsWith("SetGroupId")) {
      String grpId = generalFunctions.getValue(command, '#', 1);
      secretManager.setGroupIdentifier(grpId);
      lcd.print(F("set group id to "));
      lcd.print(grpId);

      lcd.print(F("Ok-SetGroupId"));
    } else if (command.startsWith("GetGroupId")) {
      String grpId = generalFunctions.getValue(command, '#', 1);
      lcd.print(F("groupid= "));
      lcd.print(secretManager.getGroupIdentifier());

      lcd.print(F("Ok-SetGroupId"));
    } else if (command.startsWith("GetWifiStatus")) {


      uint8_t status = wifiManager.getWifiStatus();
      lcd.print("WifiStatus=");
      lcd.print(status);


      Serial.println("Ok-GetWifiStatus");

    } else if (command.startsWith("GetRememberedValueData")) {
      Serial.println("Ok-GetRememberedValueData");
    } else if (command.startsWith("ConfigWifiSTA")) {
      //ConfigWifiSTA#ssid#password
      //ConfigWifiSTA#MainRouter24##Build4SolarPowerVisualizer#
      String ssid = generalFunctions.getValue(command, '#', 1);
      String password = generalFunctions.getValue(command, '#', 2);
      String hostname = generalFunctions.getValue(command, '#', 3);
      bool staok = wifiManager.configWifiSTA(ssid, password, hostname);
      if (staok) {
        leds[0] = CRGB(0, 0, 255);
      } else {
        leds[0] = CRGB(255, 0, 0);
      }
      FastLED.show();
      Serial.println("Ok-ConfigWifiSTA");

    } else if (command.startsWith("ConfigWifiAP")) {
      //ConfigWifiAP#soft_ap_ssid#soft_ap_password#hostaname
      //ConfigWifiAP#pancho5##pancho5

      String soft_ap_ssid = generalFunctions.getValue(command, '#', 1);
      String soft_ap_password = generalFunctions.getValue(command, '#', 2);
      String hostname = generalFunctions.getValue(command, '#', 3);

      bool stat = wifiManager.configWifiAP(soft_ap_ssid, soft_ap_password, hostname);
      if (stat) {
        leds[0] = CRGB(0, 255, 0);
      } else {
        leds[0] = CRGB(255, 0, 0);
      }
      FastLED.show();
      Serial.println("Ok-ConfigWifiAP");

    } else if (command.startsWith("GetOperationMode")) {
      uint8_t switchState = digitalRead(OP_MODE);
      if (switchState == LOW) {
        Serial.println(F("PGM"));
      } else {
        Serial.println(F("RUN"));
      }
    } else if (command.startsWith("SetTime")) {
      //SetTime#8#5#24#4#18#22#25
      uint8_t switchState = digitalRead(OP_MODE);
      if (switchState == LOW) {
        timeManager.setTime(command);
        lcd.print("Ok-SetTime");
      } else {
        Serial.println("Failure-SetTime");
      }

    } else if (command.startsWith("SetFieldId")) {
      // fieldId= GeneralFunctions::getValue(command, '#', 1).toInt();
    } else if (command.startsWith("GetTime")) {
      timeManager.printTimeToSerial(currentTimerRecord);
      Serial.flush();
      lcd.print("Ok-GetTime");
      Serial.flush();
    } else if (command.startsWith("GetCommandCode")) {
      long code = secretManager.generateCode();
      //
      // patch a bug in the totp library
      // if the first digit is a zero, it
      // returns a 5 digit number
      if (code < 100000) {
        Serial.print("0");
        Serial.println(code);
      } else {
        lcd.print(code);
      }

      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("GetSerialNumber")) {
        Serial.println(serialNumber);
        Serial.println("Ok-GetSerialNumber");
        Serial.flush();
    } else if (command.startsWith("VerifyUserCode")) {
      String codeInString = generalFunctions.getValue(command, '#', 1);
      long userCode = codeInString.toInt();
      boolean validCode = true;  //secretManager.checkCode( userCode);
      String result = "Failure-Invalid Code";
      if (validCode) result = "Ok-Valid Code";
      Serial.println(result);
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("GetSecret")) {
      uint8_t switchState = digitalRead(OP_MODE);
      if (switchState == LOW) {
        //  char secretCode[SHARED_SECRET_LENGTH];
        String secretCode = secretManager.readSecret();
        Serial.println(secretCode);
        Serial.println("Ok-GetSecret");
      } else {
        Serial.println("Failure-GetSecret");
      }
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("SetSecret")) {
      uint8_t switchState = digitalRead(OP_MODE);
      if (switchState == LOW) {
        //SetSecret#IZQWS3TDNB2GK2LO#6#30
        String secret = generalFunctions.getValue(command, '#', 1);
        int numberDigits = generalFunctions.getValue(command, '#', 2).toInt();
        int periodSeconds = generalFunctions.getValue(command, '#', 3).toInt();
        secretManager.saveSecret(secret, numberDigits, periodSeconds);
        Serial.println("Ok-SetSecret");
        Serial.flush();
        delay(delayTime);
      } else {
        Serial.println("Failure-SetSecret");
      }


    } else if (command == "Flush") {
      while (Serial.read() >= 0)
        ;
      Serial.println("Ok-Flush");
      Serial.flush();
    } else if (command.startsWith("PulseStart")) {
      inPulse = true;
      Serial.println("Ok-PulseStart");
      Serial.flush();
      delay(delayTime);

    } else if (command.startsWith("PulseFinished")) {
      display1.showNumberDec(8888, false);

      inPulse = false;
      Serial.println("Ok-PulseFinished");
      Serial.flush();
      delay(delayTime);

    } else if (command.startsWith("IPAddr")) {
      currentIpAddress = generalFunctions.getValue(command, '#', 1);
      Serial.println("Ok-IPAddr");
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("SSID")) {
      String currentSSID = generalFunctions.getValue(command, '#', 1);
      wifiManager.setCurrentSSID(currentSSID.c_str());
      Serial.println("Ok-currentSSID");
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("GetIpAddress")) {
      Serial.println(wifiManager.getIpAddress());
      Serial.println("Ok-GetIpAddress");
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("RestartWifi")) {
      wifiManager.restartWifi();
      Serial.println("Ok-restartWifi");
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("HostMode")) {
      Serial.println("Ok-HostMode");
      Serial.flush();
      delay(delayTime);
      isHost = true;
    } else if (command.startsWith("NetworkMode")) {
      Serial.println("Ok-NetworkMode");
      Serial.flush();
      delay(delayTime);
      isHost = false;
    } else if (command.startsWith("GetSensorData")) {


      Serial.println();
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("AsyncData")) {

      lcd.setCursor(0, 1);
      lcd.print("pnd=");
      lcd.print(panchoTankFlowDataNewData);

      lcd.print(" gnd=");
      lcd.print(gloriaTankFlowPumpNewData);
      lcd.print("    ");
      if (panchoTankFlowDataNewData) {
        panchoTankFlowSerializer.pushToSerial(Serial, panchoTankFlowData);
        panchoTankFlowDataNewData = false;
      }

      if (gloriaTankFlowPumpNewData) {
        gloriaTankFlowPumpSerializer.pushToSerial(Serial, gloriaTankFlowPumpData);
        gloriaTankFlowPumpNewData = false;
      }

      Serial.println("Ok-AsyncCycleUpdate");
      Serial.flush();
      delay(delayTime);
    } else if (command.startsWith("GetLifeCycleData")) {
      display1.showNumberDec(7777, false);

      CRGB orange = CRGB::Amethyst;
      CRGB orangered = CRGB::Azure;
      SetupTwoColorPalette(orange, orangered);
      performLedShow(500);
      Serial.println("Ok-GetLifeCycleData");
      Serial.flush();
    } else if (command.startsWith("GetWPSSensorData")) {
      Serial.println("Ok-GetWPSSensorData");
      Serial.flush();
    } else {
      //
      // call read to flush the incoming
      //
      Serial.println("Failure-Command Not Found-" + command);
      Serial.flush();
      delay(delayTime);
    }
    LoRa_rxMode();
    inSerial = false;
  }
}


void checkPiControlButton() {

  const uint8_t pi[] = {
    SEG_A | SEG_B | SEG_G | SEG_E | SEG_F,  // P
    SEG_B | SEG_C                           //I
  };

  const uint8_t shut[] = {
    SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,  // S
    SEG_B | SEG_C | SEG_G | SEG_E | SEG_F,  //H
    SEG_C | SEG_D | SEG_E,                  //U
    SEG_E | SEG_F | SEG_D | SEG_G           //t
  };

  const uint8_t down[] = {
    SEG_E | SEG_B | SEG_G | SEG_C | SEG_D,  // D
    SEG_D | SEG_C | SEG_G | SEG_E,          //O
    SEG_C | SEG_D | SEG_E,                  //u
    SEG_E | SEG_G | SEG_C                   //n
  };

  const uint8_t re[] = {
    SEG_E | SEG_G,                         // r
    SEG_A | SEG_F | SEG_E | SEG_G | SEG_D  //E
  };

  const uint8_t boot[] = {
    SEG_F | SEG_E | SEG_G | SEG_C | SEG_D,  // b
    SEG_D | SEG_C | SEG_G | SEG_E,          //o
    SEG_D | SEG_C | SEG_G | SEG_E,          //o
    SEG_E | SEG_F | SEG_D | SEG_G           //t
  };

  const uint8_t ples[] = {
    SEG_A | SEG_B | SEG_G | SEG_E | SEG_F,  // p
    SEG_F | SEG_E | SEG_D,                  //l
    SEG_A | SEG_F | SEG_E | SEG_G | SEG_D,  //E
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G   //s
  };

  const uint8_t wait[] = {
    SEG_C | SEG_D | SEG_E,                          //u
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,  // A
    SEG_E | SEG_F,                                  //I
    SEG_E | SEG_F | SEG_D | SEG_G                   //t

  };
  //  Serial.print("piControlLastState=");
  //  Serial.println(piControlLastState);

  // read the state of the switch/button:
  piControlCurrentState = digitalRead(PI_CONTROL);
  if (piControlLastState == HIGH && piControlCurrentState == LOW) {  // button is pressed
    piControlPressedTime = millis();
  } else if (piControlLastState == LOW && piControlCurrentState == HIGH) {  // button is released
    piControlReleasedTime = millis();
    long pressDuration = piControlReleasedTime - piControlPressedTime;
    if (pressDuration < PI_CONTROL_SHORT_PRESS_TIME && piCurrentState == PI_STATE_ASYNC) {
      // Serial.println("A short press is detected");
      userCommandState = COMMAND_REBOOT;
      display1.clear();
      display2.clear();
      display3.clear();
      display4.clear();
      display5.clear();
      display6.clear();
      display1.setSegments(pi, 2, 0);
      display3.setSegments(re, 2, 2);
      display4.setSegments(boot, 4, 0);
      display5.setSegments(ples, 4, 0);
      display6.setSegments(wait, 4, 0);
    }

    if (pressDuration >= PI_CONTROL_LONG_PRESS_TIME && piCurrentState == PI_STATE_ASYNC) {
      //   Serial.println("A long press is detected");
      userCommandState = COMMAND_SHUTDOWN;

      display1.clear();
      display2.clear();
      display3.clear();
      display4.clear();
      display5.clear();
      display6.clear();


      display1.setSegments(pi, 2, 0);
      display3.setSegments(shut, 4, 0);
      display4.setSegments(down, 4, 0);
      display5.setSegments(ples, 4, 0);
      display6.setSegments(wait, 4, 0);
    }
    if (piCurrentState == PI_STATE_SYNC) {
      CRGB orange = CRGB::Orange;
      CRGB orangered = CRGB::OrangeRed;
      SetupTwoColorPalette(orange, orangered);
      performLedShow(500);
      for (int i = 12; i < 16; i++) {
        leds[i] = CRGB(0, 0, 255);
      }
      FastLED.show();
    }
  }
  piControlLastState = piControlCurrentState;
}
void setStationMode(String ipAddress) {
  Serial.println("settting Station mode, address ");
  Serial.println(ipAddress);
  leds[0] = CRGB(0, 0, 255);
  FastLED.show();
  const uint8_t ip[] = {
    SEG_F | SEG_E,                         // I
    SEG_F | SEG_G | SEG_A | SEG_B | SEG_E  // P
  };
  display1.clear();
  display2.clear();
  display3.clear();
  display4.clear();
  display5.clear();
  display6.clear();
  uint8_t ip1, ip2, ip3, ip4;
  ip1 = GeneralFunctions::getValue(ipAddress, '.', 0).toInt();
  display1.showNumberDec(ip1, false);
  ip2 = GeneralFunctions::getValue(ipAddress, '.', 1).toInt();
  display2.showNumberDec(ip2, false);
  ip3 = GeneralFunctions::getValue(ipAddress, '.', 2).toInt();
  display3.showNumberDec(ip3, false);
  ip4 = GeneralFunctions::getValue(ipAddress, '.', 3).toInt();
  display4.showNumberDec(ip4, false);

  // display5.clear();
  // display6.clear();
  // display1.clear();
  // display2.clear();
  // display3.clear();
  // display4.clear();


  const uint8_t pls[] = {
    SEG_F | SEG_G | SEG_A | SEG_B | SEG_E,  // P
    SEG_F | SEG_E | SEG_D,                  // l
    SEG_A | SEG_G | SEG_C | SEG_D | SEG_F   // S
  };
  display5.setSegments(pls, 4, 0);
  const uint8_t vait[] = {
    SEG_F | SEG_E | SEG_D | SEG_B | SEG_C,          // U
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,  // a
    SEG_F | SEG_E | SEG_G | SEG_D                   // t
  };
  display6.setSegments(vait, 4, 0);
  delay(3000);

}

void setApMode() {

  leds[0] = CRGB(0, 0, 255);
  FastLED.show();
  Serial.println("settting AP mode");
  //
  // set ap mode
  //
  //  wifiManager.configWifiAP("PanchoTankFlowV1", "", "PanchoTankFlowV1");
  String apAddress = wifiManager.getApAddress();
  Serial.println("settting AP mode, address ");
  Serial.println(apAddress);
  const uint8_t ap[] = {
    SEG_F | SEG_G | SEG_A | SEG_B | SEG_C | SEG_E,  // A
    SEG_F | SEG_G | SEG_A | SEG_B | SEG_E           // P
  };
  display1.setSegments(ap, 2, 2);


  uint8_t ipi3 = GeneralFunctions::getValue(apAddress, '.', 0).toInt();
  display3.showNumberDec(ipi3, false);

  uint8_t ipi4 = GeneralFunctions::getValue(apAddress, '.', 1).toInt();
  display4.showNumberDec(ipi4, false);

  uint8_t ipi5 = GeneralFunctions::getValue(apAddress, '.', 2).toInt();
  display5.showNumberDec(ipi5, false);

  uint8_t ipi6 = GeneralFunctions::getValue(apAddress, '.', 3).toInt();
  display6.showNumberDec(ipi6, false);
  //    display6.setBrightness(7, false);  // Turn off

  // for (int i = 0; i < 4; i++) {
  //   ipi = GeneralFunctions::getValue(apAddress, '.', i).toInt();
  //   display1.showNumberDec(ipi, false);
  //   delay(1000);
  // }
  delay(1000);
  display1.clear();
  display2.clear();
  display3.clear();
  display4.clear();
  display5.clear();
  display6.clear();

  for (int i = 3; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  leds[0] = CRGB(0, 255, 0);
  if (loraActive) {
    leds[1] = CRGB(0, 0, 255);
  } else {
    leds[1] = CRGB(255, 0, 0);
  }

  FastLED.show();
}


void performLedShow(int millisseconds) {
  long startmillis = millis();
  long currentMillis = startmillis;
  while (currentMillis < startmillis + (millisseconds)) {
    // currentPalette = RainbowColors_p;
    // currentBlending = LINEARBLEND;
    //  ChangePalettePeriodically();
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    FillLEDsFromPaletteColors(startIndex);
    FastLED.show();
    currentMillis = millis();
  }
  for (int i = 0; i < 16; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
  //  delay(1000);
}

int processDisplayValue(double valueF, struct DisplayData* displayData) {
  int value = 0;
  if (valueF == (int)valueF) {
    if (valueF < 0) {
      if (valueF < -1000) {
        value = (int)(valueF / 10);
        displayData->dp = 1;
      } else {
        value = (int)valueF;
        displayData->dp = -1;
      }
    } else {
      if (valueF > 9999) {
        value = (int)(valueF / 1000);
        displayData->dp = 0;
      } else {
        value = (int)valueF;
        displayData->dp = -1;
      }
    }

  } else {
    value = (int)(100 * valueF);
    displayData->dp = 1;
  }
  displayData->value = value;
  return value;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}


// This function sets up a palette of purple and green stripes.
void SetupTwoColorPalette(CRGB color1, CRGB color2) {
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
    color1, color1, black, black,
    color2, color2, black, black,
    color1, color1, black, black,
    color2, color2, black, black);
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette() {
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_BLUE, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
    green, green, black, black,
    purple, purple, black, black,
    green, green, black, black,
    purple, purple, black, black);
}
// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette() {
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Green);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}
