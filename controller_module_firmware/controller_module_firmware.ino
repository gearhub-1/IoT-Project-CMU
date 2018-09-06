#include <ESP8266WiFi.h>
#include <time.h>
#include "stdlib.h"
#include "FirebaseArduino.h"
#include "setupKey.h"
#include "user_interface.h"
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

unsigned long pre = 0;

unsigned long previous = 0;
int delaytimer = 60000;

void setup()
{
  Serial.begin(9600);
  pinMode(relay, OUTPUT);

  wifi_set_phy_mode(PHY_MODE_11G);
  system_phy_set_max_tpw(40);
  WiFi.mode(WIFI_STA);
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

  //Time Server
  configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while (!time(nullptr))
  {
    Serial.print(".");
    delay(500);
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println("connected Firebase server!");
}

int sumSensor = 0;
int sensorSensitive = 5;

int lockAuto = 0;
int timeDelay = 1;
int startOff1st = 0;
String power = "N/A";
int modeAP = 0;
int manualPower = 0;
int sum = 0;
int sleepM = 0;
unsigned long sleepTime = 0;
unsigned long sleepCurrent = 0;
int re = 0;
int s1st = 0;
int a1st = 0;
int m1st = 0;
int m2nd = 0;

void loop()
{

  unsigned long currentMillis = millis();

  StaticJsonBuffer<200> jsonBuffer3;
  JsonObject &controller = jsonBuffer3.createObject();

  //time function
  time_t now = time(nullptr);
  struct tm *newtime = localtime(&now);
  String tmpNowF = "";
  String dayN = "";
  String monN = "";
  int tmpMonth =0;
  int tmpDay = 0;
  int hr = 0;
  hr = newtime->tm_hour;
  tmpMonth = newtime->tm_mon + 1;
  tmpDay = int(newtime->tm_mday);

  if (tmpMonth < 10) {
    monN = "0" + String(tmpMonth);
  } else if (tmpMonth >= 10) {
    monN = String(tmpMonth);
  }
  if (tmpDay < 10) {
    dayN = "0" + String(tmpDay);
  } else if (tmpDay >= 10) {
    dayN = String(tmpDay);
  }



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
    Firebase.setInt("setting/reset/controller", 0);
    Serial.println(tmpNowF+"   System restart ...");
    ESP.restart();
  }

  if (currentMillis - pre >= interval)
  {
    //Serial.println(tmpNowF+"   Get setting from firbase");
    pre = currentMillis;
    sensorSensitive = Firebase.getInt("setting/sensitive");
    delaytimer = Firebase.getInt("setting/timmer_delay");
    modeAP = Firebase.getInt("status/controller/mode");
    manualPower = Firebase.getInt("status/controller/power");
    sleepTime = Firebase.getInt("setting/sleep_delay");
    re = Firebase.getInt("setting/reset/controller");
  }

  if (modeAP == 0) //manual
  {
    digitalWrite(relay, (manualPower == 1 ? HIGH : LOW));
    if(manualPower == 1 && m1st == 0){
      Serial.println(tmpNowF + "   Manual Turn ON");
      controller["Log"] = tmpNowF + " Manual Turn ON";
      Firebase.push("log/controller/"+ dayN + monN ,controller);
      m1st = 1;
      m2nd = 0;
    }
    if(manualPower == 0 && m2nd == 0){
      Serial.println(tmpNowF + "   Manual Turn OFF");
      controller["Log"] = tmpNowF + " Manual Turn OFF";
      Firebase.push("log/controller/"+ dayN + monN ,controller);
      m1st = 0;
      m2nd = 1;
    }

    a1st = 0;
    lockAuto = 0;
    sleepM = 0;
    startOff1st = 0;
  }

  else if (modeAP == 3) //sleep
  {
    if (sleepM == 0)
    {
      sleepM = 1;
      digitalWrite(relay, HIGH); //on
      sleepCurrent = currentMillis;
      Serial.println(tmpNowF+"   Start sleep mode");
      controller["Log"] = tmpNowF + " Start sleep mode";
      Firebase.push("log/controller/"+ dayN + monN ,controller);
    }
    else if (sleepM == 1)
    {
      if ((currentMillis - sleepCurrent) >= sleepTime)
      {
        digitalWrite(relay, LOW); //off
        sleepM = 0;
        Firebase.setInt("status/controller/mode", 0);
        Firebase.setInt("status/controller/power", 0);
        Serial.println(tmpNowF+"   Time up Turn OFF");
        controller["Log"] = tmpNowF + " Time up Turn OFF";
        Firebase.push("log/controller/"+ dayN + monN ,controller);
        s1st = 0;
      }
    }
    lockAuto = 0;
    a1st = 0;
    startOff1st = 0;
  }

    if (lockAuto == 0 && modeAP == 1) //auto mode
    {
      s1st = 0;
      sleepM = 0;

      if(a1st == 0){
        Serial.println(tmpNowF+"   Automatic Mode");
        controller["Log"] = tmpNowF + " Automatic Mode";
        Firebase.push("log/controller/"+ dayN + monN ,controller);
        a1st = 1;
      }
      if (calSum() >= sensorSensitive ) // if sum of(sensor == "true") > sensitive turn on light
      {
        lockAuto = 1;
        digitalWrite(relay, HIGH);
        Serial.println(tmpNowF + "   Automatic Turn ON");
        controller["Log"] = tmpNowF + " Automatic Turn ON";
        Firebase.push("log/controller/"+ dayN + monN ,controller);
        Firebase.setInt("status/controller/power", 1);
      }
      else if (startOff1st == 0 && calSum() < 1) //first time if sum of(sensor=="true") < sensitive turn off light
      {
        digitalWrite(relay, LOW);
        Serial.println(tmpNowF + "   Automatic turn OFF");
        controller["Log"] = tmpNowF + " Automatic turn OFF";
        Firebase.push("log/controller/"+ dayN + monN ,controller);
        Firebase.setInt("status/controller/power", 0);
        startOff1st = 1;
      }
    }

  if (currentMillis - previous >= delaytimer) //circle time check sum of sensor for turn off light
  {
    previous = currentMillis;
    if (lockAuto == 1 && modeAP == 1)
    {
      if (calSum() < sensorSensitive) //check sensor sum
      {
        lockAuto = 0; //set for turn on if()
        digitalWrite(relay, LOW);
        Serial.println(tmpNowF + "   Automatic turn OFF");
        controller["Log"] = tmpNowF + " Automatic turn OFF";
        Firebase.push("log/controller/"+ dayN + monN ,controller);
        Firebase.setInt("status/controller/power", 0);
      }
    }
  }
}

int calSum()
{
  int s1d5 = 0;
  int s2d5 = 0;
  int s3d5 = 0;
  int s4d5 = 0;

  s1d5 = Firebase.getInt("status/sensor/1");
  s2d5 = Firebase.getInt("status/sensor/2");
  s3d5 = Firebase.getInt("status/sensor/3");
  s4d5 = Firebase.getInt("status/sensor/4");

  sumSensor = s1d5  + s2d5  + s3d5  + s4d5 ;
  //Serial.print("SUM: ");Serial.println(sumSensor);
  return sumSensor;
}
