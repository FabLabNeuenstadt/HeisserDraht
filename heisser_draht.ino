// software for http://www.kreativekiste.de/elektro/dem-heissen-draht-auf-der-spur
// developed by Timo Denk (Nov 2014)
// modified by Sebastian Sproesser (2015)
//
// We use a Arduino Mega, a seven segment display and a LCD
//
// Wire-wiring: 
// 22: start
// 24: stop
// 26: mistake
// 28: buzzer
// A0: penalty time
//
// In this implementation start and stop are interchangeable, we can
// begin from either side. We trigger the start/stop with our game loop

// Connections seven segment display:
// 45, 46, 47, 48, 49, 50, 51, 52

// Connections LCD:
// rs (LCD pin 4) to Arduino pin 12
// rw (LCD pin 5) to Arduino pin 11
// enable (LCD pin 6) to Arduino pin 10
// LCD pin 15 to Arduino pin 13
// LCD pins d4, d5, d6, d7 to Arduino pins 5, 4, 3, 2

// libraries
#include <Wire.h>
#include <LiquidCrystal.h>
#include <SevSeg.h>

#include "heisser_draht.h"

SevSeg sevseg;
 
LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);
int backLight = 13;    // pin 13 will control the backlight


const byte startPin = 22, stopPin = 24, mistakePin = 26, buzzerPin = 28, penaltyTimePin = A0;
const byte minPenaltyTime = 1, maxPenaltyTime = 15; // [t] = 1 s

byte actualStartPin, actualStopPin;

const String penaltyTimeString = "Fehler", finalTimeString = "Endzeit", timeString = "Zeit", commercialString = "FABLAB Neuenstadt", totalTime = "Gesamtzeit"; 

unsigned long millisStart, millisEnd; 

int mistakes, penaltyTime, lastPenaltyTime; 

unsigned long lastStatusUpdate = 0;
unsigned long lastMistakes = 0;

void setup() {
  // Initialise our game pins
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.
  pinMode(startPin, INPUT); 
  pinMode(stopPin, INPUT);
  pinMode(mistakePin, INPUT);
  pinMode(buzzerPin, OUTPUT); 
  digitalWrite(buzzerPin, LOW); 
  digitalWrite(startPin, HIGH);
  digitalWrite(stopPin, HIGH);
  digitalWrite(mistakePin, HIGH);

  // Initialise seven segment display
  byte numDigits = 4;
  byte digitPins[] = {41, 42, 43, 44};
  byte segmentPins[] = {45, 46, 47, 48, 49, 50, 51, 52};
  sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins);
  sevseg.setBrightness(100);
   
  // Initialise LCD
  lcd.begin(20,4);
   
  Serial.begin(9600);
  clearScreen();
}

void loop() {
  printStringCenter(1, "Heisser Draht"); 

  // Waiting for start or stop pin
  while (digitalRead(startPin) == HIGH &&
	 digitalRead(stopPin) == HIGH) {
    printStringCenter(2, "Bitte auf Start!");

    if (lastStatusUpdate) {
        printStringCenter(3, "F: " + String(mistakes) + " Zeit: " + sec2MinSecString(lastStatusUpdate + (mistakes * penaltyTime)));
    }
  }
  
  // Recognize which Pin serves as the starting point for this round.
  // The other one will be the stop pin.
  
  if (digitalRead(startPin) == LOW) {
	actualStartPin = startPin;
	actualStopPin = stopPin;
  } else {
	actualStartPin = stopPin;
	actualStopPin = startPin;
  }

  strafzeit:

  printStringCenter(2, "Strafzeit einstellen"); 
  lastPenaltyTime = 0, penaltyTime = 0; 
  
  // waiting for game to start
  while (digitalRead(actualStartPin) == LOW) {
    penaltyTime = (byte)round(map(analogRead(penaltyTimePin), 0, 1023, minPenaltyTime, maxPenaltyTime));
    if (penaltyTime != lastPenaltyTime) {
      clearRow(3); 
      printStringCenter(3, String(penaltyTime) + " s");
      lastPenaltyTime = penaltyTime; 
    }
  }


//  Serial.println("00:00.000;000;Start");
  char buf[21];
  sprintf(buf,"%02d:%02d.000;000;Start",(int)(penaltyTime/60), (int)(penaltyTime%60));

  // game running
  millisStart = millis(); 
  clearScreen();
  mistakes = 0; 
  unsigned long lastDisplayUpdate = 0;
  unsigned long lastError = 0;

  bool buzzerOn = 0;
  
  while (digitalRead(actualStopPin) == HIGH) {
	
	// Check for reset (by again touching actualStartPin)
	// Yes, goto is ugly. I know.
	
	if (digitalRead(actualStartPin) == LOW) {
		if (buzzerOn) {
			digitalWrite(buzzerPin, LOW);
			buzzerOn = 0;
		}
		Serial.println("00:00.000;000;Reset");
		goto strafzeit;
	}
	
    unsigned long now = millis();

    sevseg.refreshDisplay();
    if (lastDisplayUpdate + 10 < now) {
      printStatusOnScreen();
      printSevSeg();
      lastDisplayUpdate = now;
    }

    if (!buzzerOn) {
      // Check for mistakes
      if (digitalRead(mistakePin) == LOW) {
        mistakes++;
        unsigned long errorMillis = millis();

        unsigned long CurrentTime = round((errorMillis - millisStart)/1000) + (mistakes * penaltyTime);
        char buf[21];
        sprintf(buf,"%02d:%02d.%03d;%03d;Mistake",(int)(CurrentTime/60), (int)(CurrentTime%60), (int)((errorMillis-millisStart)%1000), (int)(mistakes));
        Serial.println(buf);
        digitalWrite(buzzerPin, HIGH); 
        buzzerOn = 1;
        lastError = now;
      }
    }

    if (lastError + 1000 < now) {
      // Reset buzzer
      if (buzzerOn) {
        digitalWrite(buzzerPin, LOW);
        buzzerOn = 0;
      }
      
      lastError = now;
    }
}
  
  // Game finished
  millisEnd = millis(); 
  unsigned long CurrentTime = round((millisEnd - millisStart)/1000) + (mistakes * penaltyTime);

  sprintf(buf,"%02i:%02i.%03i;%03i;Stop",(int)(CurrentTime/60), (int)(CurrentTime%60), (int)((millisEnd-millisStart)%1000), (int)(mistakes));
  Serial.println(buf);

  clearRow(0);
  printStringCenter(0, "Spiel beendet!");
  printStatusOnScreen();

  // Show the result for 5 seconds

  while (millisEnd + 5000 > millis()) {
    sevseg.refreshDisplay();
  }

  clearScreen();

}

unsigned long lastPlayingTime = 0;

void printSevSeg() {
    unsigned long playingTime = round((millis() - millisStart)/1000) + (mistakes * penaltyTime);
    if (playingTime != lastPlayingTime) {
//      Serial.println(playingTime);
      int seconds = playingTime % 60;
      int minutes = playingTime / 60;
      int displayTime = minutes * 100 + seconds;

      sevseg.setNumber(displayTime,2); 
      lastPlayingTime = playingTime;
    }
}

void printStatusOnScreen() {
    unsigned long playingTime = round((millis() - millisStart)/1000), penaltyTimeSum = mistakes * penaltyTime;
    if ((lastStatusUpdate != playingTime) ||
        (lastMistakes != mistakes) ) {
      printStringCenter(1, "Spielzeit: " + sec2MinSecString(playingTime)); 
      printStringCenter(2, "Fehler: " + String(mistakes) + " x " + String(penaltyTime) + "s"); 
      printStringCenter(3, "Gesamt: " + sec2MinSecString(playingTime + penaltyTimeSum));
      lastStatusUpdate = playingTime;
      lastMistakes = mistakes;
    }
}

void showCommercial() {
  clearRow(0); 
  printStringCenter(0, commercialString); 
}

void clearScreen() {
  for (byte i = 0; i < 4; i++)
    clearRow(i); 
    
  showCommercial();
}

void printStringCenter(byte row, String string) {
  lcd.setCursor((int)((20 - string.length()) / 2), row); 
  lcd.print(string); 
}

void clearRow(byte row) {
  lcd.setCursor(0, row); 
  lcd.print("                    "); 
}

String sec2MinSecString(int input) { // converts seconds to a string like "minutes:seconds"
  char buf[6];
  int minutes = (int)(input / 60);
  int seconds = (int)(input % 60);
  if (minutes > 100) {
    minutes = 99;
  }
  sprintf(buf, "%d:%02d ", minutes, seconds);
  return String(buf);
}
