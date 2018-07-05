#include <ESP8266WiFi.h>
#include <time.h>
#include "stdlib.h"
#include "FirebaseArduino.h"
#include "user_interface.h"

//PIN
#define PIR_R D5

//CONFIG FIREBASE
#define FIREBASE_HOST "cs-cmu-iot.firebaseio.com"
#define FIREBASE_AUTH "AWdEysMAVzMqdrYq3kEA9i1lW8EZRBHurj1Rc5Ts"

// CONFIG WIFI
#define WIFI_SSID "CSB308"
#define WIFI_PASSWORD "123456789"

String portNum = "4";

// Config time
int timezone = 7;
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";
int dst = 0;

int pirSum = 0;

// Config Loop Time
unsigned long previousMillisOne = 0;
unsigned long previousMillisTwo = 0;
const long interval1sec = 1000;
const long interval1min = 60000;

void pubWhenChange(int senD5);
void pubNow(int sensorD5, String timeNow);
void consoleDisplay(int one, String timeN);
void setNow(int sensorD5set);



void setup()
{

  Serial.begin(9600);

  //PIN MODE
  pinMode(PIR_R, INPUT);

  //WiFi Connect
  wifi_set_phy_mode(PHY_MODE_11G);
  system_phy_set_max_tpw(40);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  //Time Server
  configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while (!time(nullptr))
  {
    Serial.print(".");
    delay(500);
  }

  //Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  //Firebase.remove("TEST/1");
}

String pirStr1 = "";

int lock_1 = 0;
int re = 0;

void loop()
{

  unsigned long currentMillis = millis();
  time_t now = time(nullptr);
  struct tm *newtime = localtime(&now);

  int sensor_D5 = digitalRead(PIR_R);

  String tmpNowF = "";
  int tmpMonth =0;
  int tmpDay = 0;
  int hr = 0;
  hr = newtime->tm_hour;
  tmpMonth = newtime->tm_mon + 1;
  tmpDay = int(newtime->tm_mday);

  tmpNowF += String(newtime->tm_hour);
  tmpNowF += ":";
  tmpNowF += String(newtime->tm_min);
  tmpNowF += ":";
  tmpNowF += String(newtime->tm_sec);
  re = Firebase.getInt("reset/sensor/" + portNum);

  if (re == 1)
  {
    Firebase.setInt("reset/sensor/" + portNum, 0);
    ESP.restart();
  }

  if (currentMillis - previousMillisOne >= interval1sec)
  {
    previousMillisOne = currentMillis;
    pubWhenChange(sensor_D5, tmpNowF);
    consoleDisplay(sensor_D5, tmpNowF);
    setNow(sensor_D5);
    Serial.println("SENSOR: " + portNum);
  }

  if (currentMillis - previousMillisTwo >= interval1min)
  {
    previousMillisTwo = currentMillis;
    pubNow(sensor_D5, tmpNowF , tmpMonth , tmpDay, hr);
  }
}
void consoleDisplay(int one, String timeN)
{
  Serial.println("----------------------------");
  Serial.print("PIR D5: ");
  Serial.println(one);
  Serial.print("TIME: ");
  Serial.println(timeN);
  Serial.println("----------------------------");
}

void pubWhenChange(int senD5, String tmpNow)
{

  StaticJsonBuffer<200> jsonBuffer1;
  JsonObject &pir1 = jsonBuffer1.createObject();

  if (senD5 == 1 && lock_1 == 0)
  {
    Serial.print("Publish: push/sensor/event/" + portNum + "/D5 '1' ");
    Serial.print("Time :");
    Serial.println(tmpNow);
    pir1["PIR"] = 1;
    pir1["time"] = tmpNow;
    lock_1 = 1;
    pirStr1 = Firebase.push("push/sensor/event/" + portNum + "/D5", pir1);
  }

  if (senD5 == 0 && lock_1 == 1)
  {
    Serial.print("Publish: push/sensor/event/" + portNum + "/D5 '0' ");
    Serial.print("Time :");
    Serial.println(tmpNow);
    pir1["PIR"] = 0;
    pir1["time"] = tmpNow;
    lock_1 = 0;
    pirStr1 = Firebase.push("push/sensor/event/" + portNum + "/D5", pir1);
  }
}

void pubNow(int sensorD5, String timeNow , int monthNow , int dayNow, int hrNow)
{

  StaticJsonBuffer<200> jsonBuffer3;
  JsonObject &pir3 = jsonBuffer3.createObject();
  String monN = "";
  String dayN = "";

  if (monthNow < 10) {
    monN = "0" + String(monthNow);
  } else if (monthNow >= 10) {
    monN = String(monthNow);
  }
  if (dayNow < 10) {
    dayN = "0" + String(dayNow);
  } else if (dayNow >= 10) {
    dayN = String(dayNow);
  }

  pir3["D5"] = sensorD5;
  pir3["time"] = timeNow;
  Firebase.push("push/sensor/interval/" + portNum + "/" + dayN + monN + "/" + hrNow + ":00" , pir3);
}

void setNow(int sensorD5set)
{
  StaticJsonBuffer<200> jsonBuffer4;
  JsonObject &pir4 = jsonBuffer4.createObject();

  pir4["D5"] = sensorD5set;
  Firebase.set("set/sensor/interval/" + portNum, pir4);
}
