#include <EEPROM.h>
#include <timer.h>
#include "Adafruit_Keypad.h"
#define greenledpin 13
#define redledpin 12
#define lockpin 9
#define buzzer 11
#define unlockpin 10

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8}; 
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const byte passwordLength = 4;
char selectedKeys[passwordLength] = {'0', '0', '0', '0'};
char password[passwordLength] = {'1', '4', '5', '6'};
int currentCharPosision = 0;
boolean canChangePasswordInNextLoop = false;
boolean canChangePassword = false;
auto timerRestLed = timer_create_default();
auto changePasswordBlink = timer_create_default();
auto changePasswordBlinkLow = timer_create_default();
long userLastTyped; 
long unlockClock; 

void setup() {
  Serial.begin(9600);
   while (!Serial) {
    ;
  }
  pinMode(greenledpin, OUTPUT);
  pinMode(redledpin, OUTPUT);
  pinMode(lockpin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(unlockpin, INPUT);

  digitalWrite(lockpin, HIGH);

  customKeypad.begin();

  char firstPasswordKey = EEPROM.read(0);

  if (firstPasswordKey >= '0' || 
  firstPasswordKey <= '9' || 
  firstPasswordKey == '0' || 
  firstPasswordKey == '*' || 
  firstPasswordKey == '#') {
     password[0] = firstPasswordKey;
     password[1] = (char)EEPROM.read(1);
     password[2] = (char)EEPROM.read(2);
     password[3] = (char)EEPROM.read(3);
  }
  Serial.println(password);  
}

boolean isPasswordValid(char arr1[], char arr2[]) {
  for (byte i = 0; i < passwordLength; i++) {
    if (arr1[i] != arr2[i]) {
      return false;
    }
  }
  return true;
}

void notifyWrongPassword() {
  Serial.println("error");
  digitalWrite(redledpin, HIGH);

  timerRestLed.in(1000, restRedLed);
}

void notifyKeyTyped() {
  Serial.println("press");
  digitalWrite(redledpin, HIGH);
  digitalWrite(buzzer, HIGH);
  timerRestLed.in(50, restRedLed);
  userLastTyped = millis();
}

void unlock() {
  Serial.println("unlock");
  digitalWrite(greenledpin, HIGH);
  digitalWrite(lockpin, LOW);

  unlockClock = millis();
}

void unlockWithPassword() {
  Serial.println("unlockWithPassword");
  canChangePasswordInNextLoop = true;
  digitalWrite(greenledpin, HIGH);
  digitalWrite(lockpin, LOW);

  unlockClock = millis();
}

void lock() {
  digitalWrite(greenledpin, LOW);
  digitalWrite(lockpin, HIGH);
  canChangePasswordInNextLoop = false;
}

boolean didUserEnterChangePasswordMode(char selectedKeys[]) {
   for (byte i = 0; i < passwordLength; i++) {
    if (selectedKeys[i] != keys[3][0]) {
      return false;
    }
  }

  return canChangePasswordInNextLoop;
}

void changePassword(char selectedKeys[]) {
  for (byte i = 0; i < passwordLength; i++) {
   EEPROM.write(i, selectedKeys[i]);
   password[i] = selectedKeys[i];
}
 Serial.print("password changed");
}

void restRedLed() {
  digitalWrite(redledpin, LOW);
  digitalWrite(buzzer, LOW);
}

void notifyPasswordChangeMode() {
  Serial.print("password change mode");
  digitalWrite(greenledpin, HIGH);
  changePasswordBlinkLow.in(150, [](void*) -> void { digitalWrite(greenledpin, LOW); });
}

void resetPasswordCursore() {
  currentCharPosision = 0;
  digitalWrite(redledpin, HIGH);
  delay(50);
  digitalWrite(redledpin, LOW);
  delay(50);
  digitalWrite(redledpin, HIGH);
  delay(50);
  digitalWrite(redledpin, LOW);
  delay(50);
  digitalWrite(redledpin, HIGH);
  delay(50);
  digitalWrite(redledpin, LOW);
}

Timer<>::Task task;
void loop() {
  customKeypad.tick();
  timerRestLed.tick();
  changePasswordBlink.tick();
  changePasswordBlinkLow.tick();

  byte unlockStatus = digitalRead(unlockpin);
  if (unlockStatus == LOW) {
     unlock();
  }

  if (unlockClock + 3000 < millis()) {
    lock();
  }

  if (userLastTyped + 10000 < millis() && currentCharPosision) {
    resetPasswordCursore();
    Serial.print("reset password cursore");
  }

  while(customKeypad.available()){
    keypadEvent e = customKeypad.read();

    if (e.bit.EVENT == KEY_JUST_PRESSED) {
      selectedKeys[currentCharPosision] = (char)e.bit.KEY;
      notifyKeyTyped();
      if (currentCharPosision == passwordLength - 1) {
          if (canChangePassword) {
            changePassword(selectedKeys);
            canChangePassword = false;
            changePasswordBlink.cancel(task);
          }
          if (didUserEnterChangePasswordMode(selectedKeys)) {
            canChangePassword = true;
            task = changePasswordBlink.every(300, notifyPasswordChangeMode);
          }
          else if (isPasswordValid(selectedKeys, password)) {
            unlockWithPassword();
          } else {
            notifyWrongPassword();
        }
      }

      currentCharPosision = (currentCharPosision + 1) % passwordLength;
    }
  }

  delay(10);
}
