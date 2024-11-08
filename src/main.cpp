#include <Arduino.h>

const int ledRed = 26;
const int ledGreen = 12;
const int ledBlue = 14;
const int ledYellow = 27;

const int xPin = 34;
const int yPin = 35;

const int maxSequenceLength = 100;
int sequence[maxSequenceLength];
int sequenceLength = 0;
int playerIndex = 0;
bool gameActive = true;

unsigned long delayTime = 500;
unsigned long joystickDelay = 500;

void lightLed(int index) {
  switch (index) {
    case 0:
      digitalWrite(ledRed, HIGH);
      break;
    case 1:
      digitalWrite(ledGreen, HIGH);
      break;
    case 2:
      digitalWrite(ledBlue, HIGH);
      break;
    case 3:
      digitalWrite(ledYellow, HIGH);
      break;
  }
}

void turnOffLeds() {
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);
  digitalWrite(ledYellow, LOW);
}

int getPlayerInput() {
  while (true) {
    int xValue = analogRead(xPin);
    int yValue = analogRead(yPin);

    if (xValue < 1500) {
      Serial.println("Joueur: LED Jaune sélectionnée");
      lightLed(3);
      delay(joystickDelay);
      turnOffLeds();
      return 3;
    }
    else if (xValue > 2500) {
      Serial.println("Joueur: LED Verte sélectionnée");
      lightLed(1);
      delay(joystickDelay);
      turnOffLeds();
      return 1;
    }
    else if (yValue > 2500) {
      Serial.println("Joueur: LED Bleue sélectionnée");
      lightLed(2);
      delay(joystickDelay);
      turnOffLeds();
      return 2;
    }
    else if (yValue < 1500) {
      Serial.println("Joueur: LED Rouge sélectionnée");
      lightLed(0);
      delay(joystickDelay);
      turnOffLeds();
      return 0;
    }
  }
}

void showSequence() {
  Serial.println("Affichage de la séquence :");
  for (int i = 0; i < sequenceLength; i++) {
    lightLed(sequence[i]);
    switch (sequence[i]) {
      case 0:
        Serial.println("LED Rouge");
        break;
      case 1:
        Serial.println("LED Verte");
        break;
      case 2:
        Serial.println("LED Bleue");
        break;
      case 3:
        Serial.println("LED Jaune");
        break;
    }
    delay(delayTime);
    turnOffLeds();
    delay(200);
  }
}

void nextLevel() {
  sequence[sequenceLength] = random(0, 4);
  sequenceLength++;
  Serial.print("Niveau ");
  Serial.println(sequenceLength);
}

void startNewGame() {
  Serial.println("Nouvelle Partie !");
  sequenceLength = 0;
  nextLevel();
}

void gameOver() {
  Serial.println("Mauvaise séquence ! Game Over");
  for (int i = 0; i < 3; i++) {
    lightLed(0); lightLed(1); lightLed(2); lightLed(3);
    delay(200);
    turnOffLeds();
    delay(200);
  }
  delay(2000);
  gameActive = true;
  startNewGame();
}

void setup() {
  Serial.begin(115200);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  randomSeed(analogRead(0));
  startNewGame();
}

void loop() {
  if (gameActive) {
    showSequence();
    Serial.println("Reproduisez la séquence :");
    for (playerIndex = 0; playerIndex < sequenceLength; playerIndex++) {
      int playerChoice = getPlayerInput();
      if (playerChoice != sequence[playerIndex]) {
        gameActive = false;
        gameOver();
        return;
      }
    }
    Serial.println("Séquence correcte !");
    delay(1000);
    nextLevel();
  }
}
