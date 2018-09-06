#include <ESP8266WiFi.h>
#include <time.h>
#include "stdlib.h"
#include "FirebaseArduino.h"
#include "user_interface.h"
#include "setupKey.h"

//PIN
#define PIR_R D5

String portNum = "1";

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
  
}

String pirStr1 = "";

int lock_1 = 2;
int re = 0;


void loop()
{

  unsigned long currentMillis = millis();
  int sensor_D5 = digitalRead(PIR_R);

  time_t now = time(nullptr);
  struct tm *newtime = localtime(&now);
  String tmpNowF = "";
  int tmpMonth =0;
  int tmpDay = 0;
  int hr = 0;
  hr = newtime->tm_hour;
  tmpMonth = newtime->tm_mon + 1;
  tmpDay = int(newtime->tm_mday);

  if(newtime->tm_hour < 10){
    tmpNowF += "0" + String(newtime->tm_hour);
  }else if(newtime->tm_hour >= 10){
     tmpNowF += String(newtime->tm_hour);
  }
  
  tmpNowF += ":";
  
  if(newtime->tm_min < 10){
    tmpNowF += "0" + String(newtime->tm_min);
  }else if(newtime->tm_min >= 10){
     tmpNowF += String(newtime->tm_min);
  }
  
  tmpNowF += ":";

  if(newtime->tm_sec < 10){
    tmpNowF += "0" + String(newtime->tm_sec);
  }else if(newtime->tm_sec >= 10){
     tmpNowF += String(newtime->tm_sec);
  }
  
  if (re == 1)
  {
    Firebase.setInt("setting/reset/" + portNum, 0);
    ESP.restart();
    re = 0;
  }

  if (currentMillis - previousMillisOne >= interval1sec) //check sensor every 1 sec
  {
    previousMillisOne = currentMillis;
    pubWhenChange(sensor_D5, tmpNowF); //set when sensor change
    re = Firebase.getInt("setting/reset/" + portNum); // restart sensor
  }

  if (currentMillis - previousMillisTwo >= interval1min) // set sensor every 1 min
  {
    previousMillisTwo = currentMillis;
    pubNow(sensor_D5, tmpNowF , tmpMonth , tmpDay, hr);
  }
}

void pubWhenChange(int senD5, String tmpNow)
{
  if(lock_1 == 2){
    setNow(senD5);
    Serial.println(tmpNow+"   Sensor System Started!");
    lock_1 = (senD5 == 0  ) ? 0 : 1;  
  }
  if (senD5 == 1 && lock_1 == 0)
  {
    setNow(senD5);
    Serial.println(tmpNow + "    Sensor Detected!");
    lock_1 = 1;
  }

  if (senD5 == 0 && lock_1 == 1)
  {
    setNow(senD5);
    Serial.println(tmpNow + "    Sensor Undetected!");
    lock_1 = 0;
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
  Firebase.push("log/sensor/" + portNum + "/" + dayN + monN + "/" + hrNow + ":00" , pir3);
  Serial.println(timeNow + "    Push data log/sensor/" + portNum + "/" + dayN + monN + "/" + hrNow + ":00");
  if (Firebase.failed()) {
      Serial.print("pushing /logs failed:");
      Serial.println(Firebase.error());  
      return;
  }
  
}

void setNow(int sensorD5set)
{
  Firebase.setInt("status/sensor/" + portNum, sensorD5set);
}
 
