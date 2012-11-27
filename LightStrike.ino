/*
 * Modified from an IRrecv demo from Ken Shirrif
 * Reads light strike IR codes
 * Sends codes formatted as [TEAM] [COUNT] [WEAPON] on serial input
 *
 * IR Emitter based on http://www.ladyada.net/make/tvbgone/design.html
 * http://rich.whiffen.org/2011/10/my-first-light-strike-mod-and-tear-down.html
 * http://www.meatandnetworking.com/projects/hacking-at-the-light-strike-laser-tag-game/
 * https://github.com/LightStrikePlusPlus/LightStrikeDecode
 * https://docs.google.com/spreadsheet/ccc?key=0Arz5A-6gtL3kdDRFNlVpSHdkUEhUTi1mMHhlaDU0WUE#gid=0
 * Hold down shield, teamselect, and fire to deactivate health meter on gun.
 * Teensyduino pinout http://www.pjrc.com/teensy/teensyduino.html
 * Adafruit i2c/spi character LCD Backpack  http://www.adafruit.com/products/292
 * Adafruit Standard LCD 16x2 + extras - white on blue  http://www.adafruit.com/products/181
 * Adafruit Teensy http://www.adafruit.com/products/199
*/

#include <IRremote.h>
#include "Wire.h"
#include "LiquidTWI2.h"

// Pin Assignments
// NOTE: LCD uses i2c so the hardware i2c pins are also used.
// Teensy: SCL=5  SDA=6
const int SCL_PIN = 5;      //PIN_D0
const int SDA_PIN = 6;      //PIN_D1
const int XMIT_PIN = 10;    //PIN_C7
const int RECV_PIN = 0;     //PIN_B0
const int redLedPin = 21;   //PIN_F0
const int greenLedPin = 20; //PIN_F1
const int blueLedPin = 19;  //PIN_F4
const int orangeLedPin = 11;//PIN_D6
const int speakerPin = 15;  //PIN_B6  PWM
const int fireButton = 9;   //PIN_C6
const int teamButton = 4;   //PIN_B7
const int weaponButton = 3; //PIN_B3

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI2 lcd(0);
IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results results;
unsigned long toLazer = 0;
String fireString;
int bombCount = 0;
const String teamName[] = {"Blue  ","Red   ","Yellow","Green "};
const String teamCode[] = {"07"    ,"04"    ,"05"    ,"06"};
const int teamCount = 4;
int teamSelected = 0;
const String weaponName[] = {"  P.Laser","P.Stealth","  P.Pulse","  P.Sonic",
                             "     Bomb","    Medic","  R.Laser","R.Stealth",
                             "  R.Pulse","   R.Rail","  R.Sonic","   Sentry",
                             "    Optic","Refractor","AutoStrik"," Launcher",
                             " TimeBomb","Trip-Mine"};
const String weaponCode[] = {     "0102",     "0202",     "0303",     "0406",
                                  "0E18",     "08XX",     "0502",     "0602",
                                  "0703",     "0806",     "0908",     "0F08",
                                  "0A0C",     "0B12",     "0C03",     "0D06",
                                  "0E18",     "0E18"};
const int weaponCount = 18;
int weaponSelected = 0;
const String headerString = "  Light Strike  ";
/*Weapon Info  
  Full Life is 12 HP.
  Relative damage of weapon--
      Hits to kill ------   |
                        |   |
  Pistol                |   |
    Laser               12  1
    Stealth             12  1
    Pulse               8   1.5
    Sonic               3   4
  Rifle
    Laser               12  1
    Stealth             12  1
    Pulse               8   1.5
    Rail                4   3
    Sonic               3   4
  ITS
    Sentry              3   4
    Bomb                1   12
    Medic               +3  +4
  Accessories
    Optic               2   6
    Refractor           2   6
    AutoStrike          8   1.5
    Launcher            4   3
*/

void setup(){
  // set up the LCD's number of rows and columns: 
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcdHeader();
  pinMode(blueLedPin,OUTPUT);
  pinMode(redLedPin,OUTPUT);
  pinMode(greenLedPin,OUTPUT);
  pinMode(teamButton,INPUT);
  pinMode(fireButton,INPUT);
  teamLED(teamSelected);
  irrecv.enableIRIn(); // Start the receiver
  Serial.begin(9600);

}

void dump(decode_results *results){
  int count = results->rawlen;
/*
 Dumps out the decode_results structure.
 Call this after IRrecv::decode()
 void * to work around compiler issue
 void dump(void *v) {
 decode_results *results = (decode_results *)v
*/
  if (results->decode_type == UNKNOWN) {
    Serial.println("Could not decode message");
  } 
  else {
    if (results->decode_type == NEC) {
      Serial.print("Decoded NEC: ");
    } 
    else if (results->decode_type == SONY) {
      Serial.print("Decoded SONY: ");
    } 
    else if (results->decode_type == RC5) {
      Serial.print("Decoded RC5: ");
    } 
    else if (results->decode_type == RC6) {
      Serial.print("Decoded RC6: ");
    }
    
    Serial.print(results->value, HEX);
    Serial.print(" (");
    Serial.print(results->bits, DEC);
    Serial.println(" bits)");
  }
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 0; i < count; i++) {
    if ((i % 2) == 1) {
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    } 
    else {
      Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println("");
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(results->value, HEX);
  tone(speakerPin,440,500);
  delay(1000);
  lcdHeader();    
}

void loop(){
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    dump(&results);
    irrecv.resume(); // Receive the next value
  }

  if (digitalRead(weaponButton) == HIGH)
  {
    weaponChange();
  }
  if (digitalRead(teamButton) == HIGH)
  {
    teamChange();
  }  
  if (digitalRead(fireButton) == HIGH)
  {
//     irsend.sendNEC(0x61A030CF, 32);
//     delay(1000);
//     irrecv.enableIRIn();
    sendPulse();
  }

  if (Serial.available() > 0) {

    // read the incoming byte:
    String input;
    char array[16];
    while(Serial.available() > 0) {
      input += (char)Serial.read();
    }
    if(input == " " && toLazer != 0)
    {
      Serial.println("Re-firin' mah lazer");
      irsend.sendLS(toLazer, 32);
    } else {
      Serial.println("Firin' mah lazer");
      toLazer = stringToLazer(input);
      irsend.sendLS(toLazer, 32);
    }  
    irrecv.enableIRIn();
  }
}

int htoi (char c){
  //one hex digit in ascii to an int
  //does not check that input is valid
  if (c<='9')
    return c-'0';
  if (c<='F')
    return c-'A'+10;
  if (c<='f')
    return c-'a'+10;
  return 0;
}

unsigned long stringToLazer(String input){
//Take string in format of "[hex team code] [hex count] [hex weapon]" and returns the int to send out
  String buf[3];
  int first = 0;
  for(int i = 0; i < 2; i++)
  {
    int last;
    last = input.indexOf(' ', first)+1;
    if(last != -1)
    {
      buf[i] = input.substring(first, last-1);
      first = last;
    } else {
      return 0;
    }
  }
  buf[2] = input.substring(first);
  unsigned int section[3] = {0, 0, 0};
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < buf[i].length(); j++)
    {
      char charbuf[6];
      buf[i].toCharArray(charbuf, 6);
      section[i] += htoi(charbuf[j]) << (4*(buf[i].length()-j-1));
    }
  }
  unsigned long output = ((unsigned long)section[0] << 24) | ((unsigned long)section[1] << 16) | section[2];
  Serial.println(output, HEX);
  return output;
}

void weaponChange(){
  weaponSelected++;
  if (weaponSelected > weaponCount - 1)
  { 
    weaponSelected = 0;
  }
  tone(speakerPin,440,500);
  lcd.setCursor(7,1);
  lcd.print(weaponName[weaponSelected]);
  delay(300);
}

void teamChange(){
  teamSelected++;
  if (teamSelected > teamCount - 1)
  { 
     teamSelected = 0;
  }
  teamLED(teamSelected);
  lcd.setCursor(0,1);
  lcd.print(teamName[teamSelected]);
  tone(speakerPin,440,500);
  delay(300);
}

void teamLED(int team){
  switch (team)
  {
    case 0:
      //BLUE
      digitalWrite(blueLedPin,LOW);
      digitalWrite(redLedPin,HIGH);
      digitalWrite(greenLedPin,HIGH);
      break;
    case 1:
      //RED
      digitalWrite(blueLedPin,HIGH);
      digitalWrite(redLedPin,LOW);
      digitalWrite(greenLedPin,HIGH);
      break;
    case 2:
      //YELLOW
      digitalWrite(blueLedPin,HIGH);
      digitalWrite(redLedPin,LOW);
      digitalWrite(greenLedPin,LOW); 
      break;
    case 3:
      //GREEN
      digitalWrite(blueLedPin,HIGH);
      digitalWrite(redLedPin,HIGH);
      digitalWrite(greenLedPin,LOW);
      break;
    }
}

void sendPulse(){
  String count = "00";
  if (weaponSelected == 4)  //Bomb Mode
  {
     sendBomb();
  }

  else if (weaponSelected == 5)  //medic mode
  {
    fireString = String("08 01 08"+teamCode[teamSelected]);
    irBlast(fireString);
  }
  else if (weaponSelected == 16)  //TimeBomb
  {
    timeBomb();
  }
  else if (weaponSelected == 17)  //Trip-Mine
  {
     tripMine();
  }
  else
  {
    fireString = String(teamCode[teamSelected]+" "+count+" " + weaponCode[weaponSelected]);
    irBlast(fireString);
  }
}

void sendBomb(){
  String count;
  if (bombCount < 10)
  {
    count=String("0"+String(bombCount));
  }
  else
  {
    count=String(bombCount);
  }   
  bombCount++;
  fireString = String(teamCode[teamSelected]+" "+count+" " + weaponCode[weaponSelected]);
  irBlast(fireString);
}

void timeBomb(){
  int timeDelay = 10;
  String count;
  long previousMillis = 0;
  long interval = 1000;
  while ((digitalRead(weaponButton) == LOW) && ( timeDelay > 0 ))
  {
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > interval)
    {
      previousMillis = currentMillis;   
      timeDelay--;
      Serial.println(String(timeDelay));
      lcd.setCursor(0,0);
      lcd.print("    Time Bomb    ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(8,1);
      lcd.print(String(timeDelay));
    }
  }
  if (timeDelay != 0)
  {
    lcd.clear();
    lcd.print("*****Abort******");
    Serial.println("*****Abort*****");
    delay(1000);
    lcdHeader();
  }
  else
  {
    Serial.println("Firing Bomb");
    sendBomb();
    delay(1000);
    lcdHeader();
  }
}

void lcdHeader(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(headerString);
  lcd.setCursor(0,1);
  lcd.print(teamName[teamSelected]);
  lcd.setCursor(7,1);
  lcd.print(weaponName[weaponSelected]);
}

void irBlast(String fireStr){
  if (fireStr != "")
  {
    toLazer = stringToLazer(fireStr);
    irsend.sendLS(toLazer, 32);
    irrecv.enableIRIn();
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,0);
    lcd.print("--==>FIRING<==--");
    tone(speakerPin,494,500);
    delay(1000);
    lcdHeader();
  }
}

void tripMine(){
  long comp;
  boolean tripped = false;
  switch (teamSelected)
  {
    case 0:
      //BLUE
      comp = 0x7000202;
      break;
    case 1:
      //RED
      comp = 0x4000202;
      break;
    case 2:
      //YELLOW
      comp = 0x5000202;
      break;
    case 3:
      //GREEN
      comp = 0x6000202;
      break;
  }
  lcd.clear();
  lcd.print("Trip Mine");
  lcd.setCursor(0,1);
  lcd.print("Waiting...");
  delay(500);
  while ((digitalRead(weaponButton) == LOW) && (!tripped)) {   
    if (irrecv.decode(&results)) {
      if (results.value == comp) {
        tripped = true;
      }
     irrecv.resume(); // Receive the next value
    }   
  }
  if (tripped) {
    lcd.clear();
    sendBomb();
    delay(1000);
  }
  lcdHeader();
  delay(500);
}
