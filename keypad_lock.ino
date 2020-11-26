#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define max_input 8
#define PASSWORD_COUNT 8

const int magnetPin = 13;
const int latchPin = 10;
//Pin connected to clock pin (SH_CP) of 74HC595
const int clockPin = 12;
////Pin connected to Data in (DS) of 74HC595
const int dataPin = 11;

char data[max_input+1]; 
char *masters[] = {
  "1111",
  "2222",
  "3333",
  "4444",
  "5555",
  "6666",
  "7777",
  "8888"
}; 
int data_count = 0;
char customKey;
int locksCleared = 0;
int allLocksCleared = B11111111;
bool inPasswordEdit = false, unlocked = false, blinking = false, gameStarted=false;
int currentMasterPass = 0;

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2);  

void setup(){
  lcd.init(); 
  lcd.backlight();
  customKeypad.setHoldTime(3000);
  pinMode(magnetPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Hold * to start");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Hold * to start");
}

void loop(){
  if (unlocked) {
    allLocksClearedIndicator();
  }
  customKey = customKeypad.getKey();
  if (customKey == '*') {
    if (!gameStarted) {
      resetPasswords();
    } else {
      clearData();
    }
  } else if (customKey == '#') {
    if (unlocked) {
      stopGame();
    } else {
      if (inPasswordEdit) {
        savePassword();
      } else {
        checkPassword();
      }
    }
  } else if (customKey){
    data[data_count] = customKey;
    setLCDEntry(); 
    data_count++;
  }
}

void resetPasswords() {
  Serial.println("Resetting passwords");
  inPasswordEdit = true;
  unlocked = false;
  currentMasterPass = 0;
  clearData();
  return;
}

void savePassword() {
  masters[currentMasterPass] = strdup(data);
  currentMasterPass++;
  if (currentMasterPass >= PASSWORD_COUNT) {
    currentMasterPass = 0;
    inPasswordEdit = false;
    for (int i=0; i<PASSWORD_COUNT; i++) {
      Serial.println("Password " + String(i+1) + ": " + masters[i]);
    }
    digitalWrite(magnetPin, HIGH);
    locksCleared = 0;
  }
  clearData();
  return;
}

void stopGame() {
  digitalWrite(magnetPin, LOW);
  unlocked = false;
  locksCleared = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Game stopped");
  lcd.setCursor(0,1);
  lcd.print("Hold * to start");
}

void setLCDEntry() { 
  lcd.print(data[data_count]);
  if (data_count % 2 == 1) {
    lcd.print(" ");
  }
  Serial.println("Current entry:");
  Serial.println(data);
  
  return;
}

void resetLCDForEntry() {
  lcd.setCursor(0,0);
  if (inPasswordEdit) {
    lcd.print("Enter Master " + String(currentMasterPass + 1) + ":");
  } else {
    lcd.print("Enter Password:");
  }
  lcd.setCursor(0,1);
  
  return;
}

void setLockIndicators() {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, locksCleared);
  digitalWrite(latchPin, HIGH);
}

void allLocksClearedIndicator() {
  if (blinking) {
    locksCleared = 0;
    setLockIndicators();
    delay(1000);
    blinking = false;
  } else {
    locksCleared = allLocksCleared;
    setLockIndicators();
    delay(1000);
    blinking = true;
  }
  return;
}

void clearAndPrintLCD(String entry){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(entry);
  delay(1000);
  
  return;
}

void printBinary(byte inByte)
{
  for (int b = 7; b >= 0; b--)
  {
    Serial.print(bitRead(inByte, b));
  }
  
  Serial.println("");
  
  return;
}

void checkPassword() {
    clearAndPrintLCD("Checking entry...");
    Serial.println("Checking entry...");
    
    for (int i = 0; i < PASSWORD_COUNT; i++) {
      if(!strcmp(data, masters[i])){
        if (locksCleared >> i & 1 == 1) {
          clearAndPrintLCD("Lock: " + String(i + 1) + " already cleared");
          Serial.println("Lock: " + String(i + 1) + " already cleared");
        } else {
          clearAndPrintLCD("Lock: " + String(i + 1) + " Correct");
          Serial.println("Lock: " + String(i + 1) + " Correct");
          locksCleared = locksCleared | 1 << i;
          setLockIndicators();
          Serial.println("Locks currently cleared:");
          printBinary(locksCleared);
          
          checkAllLocks();
        }
        if (!unlocked) {
          clearData();
        }
        return;
      }
    }
    
    clearAndPrintLCD("Incorrect");
    Serial.println("Incorrect");
    delay(1000);

    clearData();
    
    return;
}

void checkAllLocks() {
  if (allLocksCleared == locksCleared) {
    digitalWrite(magnetPin, LOW);
    unlocked = true;
    clearAndPrintLCD("Chest opened...");
    
    Serial.println("All locks cleared!");
  }
  
  return;
}

void clearData(){
  if (inPasswordEdit) {
    clearAndPrintLCD("Saving...");
    Serial.println("Saving...");
  } else {
    clearAndPrintLCD("Clearing entry...");
    Serial.println("Clearing entry...");
  }

  while(data_count !=0){
    data[data_count--] = 0; 
  }
  
  resetLCDForEntry();
  return;
}
