#include <ESP8266WiFi.h>
#include <time.h>
#include "stdlib.h"
#include "FirebaseArduino.h"
#include "setupKey.h"

#define relay D5



// Config time
int timezone = 7;
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";
int dst = 0;

int calSum();
void displayScreen(String powerRev, String timeN, int modeN, int delayN, int sst);

unsigned long previousMillis = 0;
const long interval = 1000;

unsigned long previous = 0;
int delaytimer = 60000;

void setup() {
  Serial.begin(9600);
  pinMode(relay, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println("connected Firebase server!");
}


int sumSensor = 0;
int sensorSensitive = 5;

int lockDelay = 0;
int timer = 0;
int timeDelay = 1;
int run1stTime = 0;
String power = "N/A";
int modeAP = 0;
int manualPower = 0;
int sum = 0;
int sleepM = 0;
unsigned long sleepTime = 0;
unsigned long sleepCurrent = 0 ;
int re = 0;

void loop() {

  unsigned long currentMillis = millis();

  sensorSensitive = Firebase.getInt("setting/sensitive");
  delaytimer = Firebase.getInt("setting/delay");
  modeAP = Firebase.getInt("status/mode");
  manualPower = Firebase.getInt("status/power");
  sleepTime = Firebase.getInt("setting/sleep");
  re = Firebase.getInt("reset/controller/1");

  if(re == 1){
    Firebase.setInt("reset/controller/1",0);
    ESP.restart();
  }

  if (modeAP == 0) {
    digitalWrite(relay, (manualPower == 1 ? HIGH : LOW));
    power = (manualPower == 1) ? "ON" : "OFF";
    lockDelay = 0;
    sleepM = 0;
  } else if (modeAP == 3) {
    if (sleepM == 0) {
      sleepM = 1;
      digitalWrite(relay, HIGH);
      sleepCurrent = currentMillis;
    } else if (sleepM == 1) {
      if ((currentMillis - sleepCurrent) >= sleepTime) {
        digitalWrite(relay, LOW);
        sleepM = 0;
        Firebase.setInt("status/mode", 0);
        Firebase.setInt("status/power", 0);
      }
    }
    lockDelay = 0;
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (modeAP == 1) {
      sum = calSum();
      sleepM = 0;
      if ( sum >= sensorSensitive && lockDelay == 0) {
        lockDelay = 1;
        digitalWrite(relay, HIGH);
        Serial.println("TURN ON");
        Firebase.setInt("status/power", 1);
        power = "ON";
        timer = 0;
      } else if ( sum < 1 && lockDelay == 0 && run1stTime == 0) {
        digitalWrite(relay, LOW);
        Serial.println("TURN OFF 1st");
        Firebase.setInt("status/power", 0);
        run1stTime = 1;
        power = "OFF";
      }
    }
    //displayScreen(power, modeAP, delaytimer, sensorSensitive,sum);
  }

  if (currentMillis - previous >= delaytimer) {
    // save the last time you blinked the LED
    previous = currentMillis;
    if (lockDelay == 1 && modeAP == 1) {
      timer++;
      Serial.print("INTERVAL :"); Serial.println(timer);
      if (calSum() < sensorSensitive) {
        lockDelay = 0;
        digitalWrite(relay, LOW);
        Serial.println("TURN OFF");
        Firebase.setInt("status/power", 0);
        power = "OFF";
        timer = 0;
      }
    }
  }
}

void displayScreen(String powerRev, int modeN, int delayN, int sst, int sumN) {

  Serial.print("STATUS : "); Serial.println(powerRev);
  Serial.print("SENSOR : "); Serial.println(sumN);
  Serial.print("MODE   : "); (modeN == 1) ? Serial.println("AUTO") : Serial.println("MANUAL");
  Serial.print("DELAY  : "); Serial.print((delayN / 1000) / 60); Serial.println(" min");
  Serial.print("SST    : "); Serial.println(sst);
  Serial.println("");
}

int calSum() {

  int s1d5 = 0; int s1d6 = 0;
  int s2d5 = 0; int s2d6 = 0;
  int s3d5 = 0; int s3d6 = 0;
  int s4d5 = 0; int s4d6 = 0;

  s1d5 = Firebase.getInt("set/sensor/interval/1/D5");
  s1d6 = Firebase.getInt("set/sensor/interval/1/D6");

  s2d5 = Firebase.getInt("set/sensor/interval/2/D5");
  s2d6 = Firebase.getInt("set/sensor/interval/2/D6");

  s3d5 = Firebase.getInt("set/sensor/interval/3/D5");
  s3d6 = Firebase.getInt("set/sensor/interval/3/D6");

  s4d5 = Firebase.getInt("set/sensor/interval/4/D5");
  s4d6 = Firebase.getInt("set/sensor/interval/4/D6");

  sumSensor = s1d5 + s1d6 + s2d5 + s2d6 + s3d5 + s3d6 + s4d5 + s4d6;
  //Serial.print("SUM: ");Serial.println(sumSensor);
  return sumSensor;
}
