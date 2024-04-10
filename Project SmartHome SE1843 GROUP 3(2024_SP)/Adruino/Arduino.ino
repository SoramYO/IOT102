#include <Keypad_I2C.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Buzzer.h>
#include <Servo.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define I2CADDR 0x20
#define LCD_ADDR 0x27

Servo servo;
SoftwareSerial ArduinoUno(10, 9);
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 7, 6, 5, 4 };
byte colPins[COLS] = { 3, 2, 1, 0 };

Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR);

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

const int motorPin = 2;
const int buzzerPin = 3;
const int ledPin = 4;
const int tempPin = A0;
const int lightSensorPin = A1;
const int buttonPin = A2;
const int ECHO_PIN = 6;
const int TRIG_PIN = 5;
const long interval = 4000;
unsigned long previousMillis = 0;
int inputCount = 0;
int timeLock = 0;
bool doorOpen = false;
const int distanceThreshold = 20;  // 20cm
int val;
const int ANALOG_THRESHOLD = 20;

String D2003 = "1234";
String D2004 = "5678";
String D2005 = "1357";


bool autoLedControl = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // Initialize I2C communication
  keypad.begin(makeKeymap(keys));
  lcd.init();       // Initialize the LCD
  lcd.backlight();  // Turn on backlight
  lcd.clear();      // Clear the LCD screen
  lcd.setCursor(0, 0);
  servo.attach(motorPin);
  servo.write(0);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);         // LED pin as output
  pinMode(lightSensorPin, INPUT);  // Light sensor pin as input
  pinMode(buttonPin, INPUT);
  pinMode(tempPin, INPUT);  // Button pin as input

  Serial.println("Bat dau");
  if (readStringFromEEPROM(0) == "") {
    writeStringToEEPROM(0, D2003);
  } else {
    D2003 = readStringFromEEPROM(0);
  }
  if (readStringFromEEPROM(5) == "") {
    writeStringToEEPROM(5, D2004);
  } else {
    D2004 = readStringFromEEPROM(5);
  }
  if (readStringFromEEPROM(10) == "") {
    writeStringToEEPROM(10, D2005);
  } else {
    D2005 = readStringFromEEPROM(10);
  }
}
float tempRoom(int tempPin) {
  int val = analogRead(tempPin);
  float millivolts = (val / 1024.0) * 5000;
  float cel = millivolts / 10;
  return cel;
}
void loop() {
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');

    // control servo motor
    if (message.startsWith("SERVO:")) {
      int position = message.substring(6).toInt();
      servo.write(position);
      delay(3000);
      closeDoor();
    }

    // control buzzer
    if (message.startsWith("BUZZER:")) {
      int state = message.substring(7).toInt();
      digitalWrite(buzzerPin, state);
    }

    // control led
    if (message.startsWith("LED:")) {
      int state = message.substring(4).toInt();

      if (state == 1) {
        Serial.println("LED is ON");
        digitalWrite(ledPin, HIGH);
      } else if (state == 0) {
        Serial.println("LED is OFF");
        digitalWrite(ledPin, LOW);
      }
    }
     // control auto led
    if (message.startsWith("AUTOLED")) {
      if (message.startsWith("AUTOLEDON")) {
        autoLedControl = true;
      } else if (message.startsWith("AUTOLEDOFF")) {
        autoLedControl = false;
      }
    }
  }
  // Read light sensor value
  unsigned long currentMillis = millis();
  int lightValue = analogRead(lightSensorPin);
  int buttonState = digitalRead(buttonPin);
  int distance = measureDistance();
  float temp = tempRoom(tempPin);
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.print(temp);
    Serial.print(",");
    Serial.print(lightValue);
    Serial.print(",");
    Serial.print(distance);
    Serial.println();
    if (autoLedControl) {
      ledControl(lightValue);
    }
  }



  // If the button is pressed, open the door
  if (buttonState == LOW) {
    if (distance < distanceThreshold && !doorOpen) {
      String enteredPassword = getPasswordInput(false);
      String passEEPROM1 = readStringFromEEPROM(0);
      String passEEPROM2 = readStringFromEEPROM(5);
      String passEEPROM3 = readStringFromEEPROM(10);
      if (verifyPassword(enteredPassword, passEEPROM1) || verifyPassword(enteredPassword, passEEPROM2) || verifyPassword(enteredPassword, passEEPROM3)) {
        lcd.clear();
        beepBuzzer();
        lcd.print("Door Opened!");
        inputCount = 0;
        openDoor();
        delay(3000);
        closeDoor();
      } else if (enteredPassword == "change") {
        lcd.clear();
        lcd.print("Change success");
        delay(2000);
      } else if (enteredPassword == "not change") {
        lcd.clear();
        lcd.print("Change failed!");
        delay(2000);
      } else if (enteredPassword == "Timeout! No input.") {
        lcd.clear();
        lcd.print("Timeout!");
        delay(1000);
      } else {
        inputCount++;
        if (inputCount >= 3) {
          lockKeypad();
        } else {
          lcd.clear();
          lcd.print("Wrong password!");
          delay(1000);
          lcd.clear();
        }
      }
    } else if (distance > distanceThreshold) {
      lcd.clear();
    }
    delay(100);
  } else {
    beepBuzzer();
    openDoor();
    lcd.print("Door Opened!");
    delay(3000);  // Keep the door open for 5 seconds
    closeDoor();
    lcd.clear();
    lcd.print("Door Closed!");
    delay(2000);
  }
}

int ledControl(int lightValue) {
  if (lightValue < ANALOG_THRESHOLD) {
    digitalWrite(ledPin, HIGH);  // turn on LED
  } else {
    digitalWrite(ledPin, LOW);
  }
}

int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration / 29 / 2;
  return distance;
}

void writeStringToEEPROM(int addrOffset, String strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset) {
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++) {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  return String(data);
}

boolean verifyPassword(String enteredPassword, String password) {
  return enteredPassword.indexOf(password) != -1;
}

String getPasswordInput(boolean changePass) {
  lcd.clear();
  unsigned long startTime = millis();
  unsigned long timeout = 20000;
  if (changePass == false) {
    lcd.print("Enter password :");
  } else {
    lcd.print("New password: ");
  }
  String lcdPass = "";
  String input = "";
  char key;
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == '#') {
        break;
      } else if (key == 'C') {
        boolean success = changePassword();
        if (success) {
          return "change";
        } else {
          return "not change";
        }
      } else if (key == 'D') {
        lcdPass = "";
        input = "";
        lcd.clear();
        lcd.print("Enter password :");
      } else if (key == 'B') {
        writeStringToEEPROM(0, "1234");
        writeStringToEEPROM(5, "5678");
        writeStringToEEPROM(10, "1357");
      } else {
        lcd.setCursor(0, 1);
        lcdPass += "*";
        lcd.print(lcdPass);
        input += key;
      }
    }
    if (millis() - startTime > timeout) {
      return "Timeout! No input.";
    }
  }
  return input;
}

boolean changePassword() {
  lcd.clear();
  lcd.print("Before password:");

  String lcdPass = "";
  String input = "";
  char key;

  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == '#') {
        break;
      } else {
        lcd.setCursor(0, 1);
        lcdPass += "*";
        lcd.print(lcdPass);
        input += key;
      }
    }
  }
  String passEEPROM = readStringFromEEPROM(0);
  String newPass = "";
  if (input == D2003 || input == passEEPROM) {
    newPass = getPasswordInput(true);
    D2003 = newPass;
    writeStringToEEPROM(0, D2003);
    return true;
  } else if (input == D2004) {
    newPass = getPasswordInput(true);
    D2004 = newPass;
    writeStringToEEPROM(5, D2004);
    return true;
  } else if (input == D2005) {
    newPass = getPasswordInput(true);
    D2005 = newPass;
    writeStringToEEPROM(10, D2005);
    return true;
  } else {
    lcd.clear();
    lcd.print("Invalid password");
    delay(2000);
  }
  return false;
}

void incorrectPass() {
  lcd.print("Lock surcurity!");
  tone(buzzerPin, 1000, 3000);
  delay(2000);
  noTone(buzzerPin);
  lcd.setCursor(0, 0);
}

void openDoor() {
  servo.write(180);
  doorOpen = true;
}

void closeDoor() {
  servo.write(0);
  doorOpen = false;
}

void lockKeypad() {
  lcd.clear();
  lcd.print("Keypad locked!");
  String countDown = "";
  tone(buzzerPin, 1000, 3000);

  lcd.setCursor(0, 1);
  timeLock += 3000;

  for (int i = timeLock / 1000; i > 0; i--) {
    lcd.setCursor(0, 1);
    if (i < 10) {
      lcd.print("0");
    }
    lcd.print(i);
    delay(1000);
  }
  lcd.clear();
  lcd.print("Unlocking...");
  delay(1000);
}
void beepBuzzer() {
  digitalWrite(buzzerPin, HIGH);
  delay(500);  // Đợi 0,5 giây
  digitalWrite(buzzerPin, LOW);
  delay(500);  // Đợi 0,5 giây
}
