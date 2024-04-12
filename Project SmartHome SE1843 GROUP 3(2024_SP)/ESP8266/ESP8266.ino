#define BLYNK_TEMPLATE_ID "TMPL6h6QVp8KK"
#define BLYNK_TEMPLATE_NAME "SmartHome"
#define BLYNK_AUTH_TOKEN "EwNgMQOgtNywafp5IHwVxn2pzZRnK8AW"
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SoftwareSerial.h>
#include <BlynkSimpleEsp8266.h>
String URL = "http://api.thingspeak.com/update?api_key=(YOURAPIKEY)&field1=";

const char *ssid = "Huy";           // Enter your WIFI SSID
const char *password = "14121412";  // Enter your WIFI Password

#define BOTtoken "BOTTOKEN"  // Enter the bottoken you got from botfather
#define CHAT_ID "USERCHATID"                                       // Enter your chatID you got from chatid bot

SoftwareSerial ESP82666(D2, D3);

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
unsigned long lastTime = 0;     // initialize it in setup()
unsigned long interval = 3000;  // the time we need to wait
void myTimerEvent() {
  Blynk.virtualWrite(V1, millis() / 1000);
}
// for servo
BLYNK_WRITE(V2) {
  int position = param.asInt();
  // send the position message to arduino uno
  Serial.println(String("SERVO:") + position);
}

// for buzzer
BLYNK_WRITE(V3) {
  int state = param.asInt();
  // send the state message to arduino uno
  Serial.println(String("BUZZER:") + state);
}

// for led
BLYNK_WRITE(V4) {
  int state = param.asInt();
  // send the state message to arduino uno
  Serial.println(String("LED:") + state);
}
void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  configTime(0, 0, "pool.ntp.org");
  client.setTrustAnchors(&cert);
  WiFi.disconnect();
  delay(1000);
  Serial.print("Start connection");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while ((!(WiFi.status() == WL_CONNECTED))) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Wifi Connected!", "");
  bot.sendMessage(CHAT_ID, "System has Started!!", "");
  lastTime = millis();
}

void loop() {
  Blynk.run();
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
  while (Serial.available() == 0) {
    ;  // wait until data is fully available
  }
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    int comma1 = data.indexOf(',');
    int comma2 = data.lastIndexOf(',');
    if (comma1 != -1 && comma2 != -1 && comma1 != comma2) {
      float temp = data.substring(0, comma1).toFloat();
      int light = data.substring(comma1 + 1, comma2).toInt();
      int distance = data.substring(comma2 + 1).toInt();
      Serial.print("nhietdo: ");
      Serial.println(temp);
      Serial.print("anh sang: ");
      Serial.println(light);
      Serial.print("khoangcach: ");
      Serial.println(distance);
      sendData(temp, light, distance);
      if (distance <= 10 && distance >= 0) {
        bot.sendMessage(CHAT_ID, "Co nguoi den", "");
      }
      if (temp > 80) {
        bot.sendMessage(CHAT_ID, "Nhiet do phong dang cao", "");
      }
    }
  }
}

void sendData(float temp, int light, int distance) {
  WiFiClient client;
  HTTPClient http;
  String newUrl = URL + String(temp) + "&field2=" + String(light) + "&field3=" + String(distance);
  http.begin(client, newUrl);
  int responsecode = http.GET();
  String data = http.getString();
  Serial.println(data);
  http.end();
}
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "cua") {
      // Gửi lệnh qua Serial
      Serial.println(String("SERVO:") + 180);
    }else if (text == "den"){
      Serial.println(String("LED:") + 1);
    }else if((text == "loa")){
      Serial.println(String("BUZZER:") + 1);
    }
  }
}
