#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configuration des LEDs
const int ledRed = 26;
const int ledGreen = 12;
const int ledBlue = 14;
const int ledYellow = 27;

// Configuration du joystick
const int xPin = 34;
const int yPin = 35;

// Configuration du buzzer
const int buzzerPin = 33;

// Fréquences pour chaque couleur
const int frequencyRed = 500;
const int frequencyGreen = 600;
const int frequencyBlue = 700;
const int frequencyYellow = 800;

// Variables pour le jeu Simon
const int maxSequenceLength = 100;
int sequence[maxSequenceLength];
int sequenceLength = 0;
int playerIndex = 0;
bool gameActive = false;  // Le jeu est désactivé au départ
String currentUsername = ""; // Stocke le nom de l'utilisateur

unsigned long delayTime = 500;
unsigned long joystickDelay = 500;

// Configuration Wi-Fi et MQTT
const char* ssid = "Xiaomi 11 Lite 5G NE GC";
const char* password = "Gui38Gui";
const char* mqttServer = "192.168.89.166"; // Adresse IP du broker MQTT
const int mqttPort = 1883;
const char* mqttTopicScore = "simon/score";
const char* mqttTopicControl = "simon/start"; // Nouveau topic pour les commandes de démarrage

WiFiClient espClient;
PubSubClient client(espClient);

// Fonction pour jouer un son correspondant à la couleur
void playSound(int colorIndex) {
  int frequency = 0;
  switch (colorIndex) {
    case 0: frequency = frequencyRed; break;
    case 1: frequency = frequencyGreen; break;
    case 2: frequency = frequencyBlue; break;
    case 3: frequency = frequencyYellow; break;
  }
  tone(buzzerPin, frequency, 200); // Joue le son pendant 200 ms
}

// Fonction pour allumer la LED correspondante et jouer un son
void lightLed(int index) {
  playSound(index); // Joue le son correspondant à la couleur
  switch (index) {
    case 0: digitalWrite(ledRed, HIGH); break;
    case 1: digitalWrite(ledGreen, HIGH); break;
    case 2: digitalWrite(ledBlue, HIGH); break;
    case 3: digitalWrite(ledYellow, HIGH); break;
  }
}

// Fonction pour éteindre toutes les LEDs
void turnOffLeds() {
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);
  digitalWrite(ledYellow, LOW);
}

// Fonction pour récupérer l'entrée du joueur à partir du joystick
int getPlayerInput() {
  while (true) {
    int xValue = analogRead(xPin);
    int yValue = analogRead(yPin);

    if (xValue < 1100) {
      Serial.println("Joueur: LED Jaune sélectionnée");
      lightLed(3);
      delay(joystickDelay);
      turnOffLeds();
      return 3;
    } else if (xValue > 2900) {
      Serial.println("Joueur: LED Verte sélectionnée");
      lightLed(1);
      delay(joystickDelay);
      turnOffLeds();
      return 1;
    } else if (yValue > 2900) {
      Serial.println("Joueur: LED Bleue sélectionnée");
      lightLed(2);
      delay(joystickDelay);
      turnOffLeds();
      return 2;
    } else if (yValue < 1100) {
      Serial.println("Joueur: LED Rouge sélectionnée");
      lightLed(0);
      delay(joystickDelay);
      turnOffLeds();
      return 0;
    }
  }
}

// Fonction pour afficher la séquence de LEDs
void showSequence() {
  Serial.println("Affichage de la séquence :");
  for (int i = 0; i < sequenceLength; i++) {
    lightLed(sequence[i]);
    switch (sequence[i]) {
      case 0: Serial.println("LED Rouge"); break;
      case 1: Serial.println("LED Verte"); break;
      case 2: Serial.println("LED Bleue"); break;
      case 3: Serial.println("LED Jaune"); break;
    }
    delay(delayTime);
    turnOffLeds();
    delay(200);
  }
}

// Fonction pour ajouter un niveau
void nextLevel() {
  sequence[sequenceLength] = random(0, 4);
  sequenceLength++;
  Serial.print("Niveau ");
  Serial.println(sequenceLength);
}

// Fonction pour démarrer une nouvelle partie
void startNewGame() {
  Serial.println("Nouvelle Partie !");
  sequenceLength = 0;
  gameActive = true;  // Active le jeu
  nextLevel();
}

// Fonction pour envoyer le score et le nom d'utilisateur via MQTT
void envoyerScoreMQTT() {
  if (client.connected()) {
    // Créer le message JSON avec le score et le nom d'utilisateur
    String scoreMsg = "{\"username\": \"" + currentUsername + "\", \"score\": " + String(sequenceLength - 1) + "}";
    client.publish(mqttTopicScore, scoreMsg.c_str());
    Serial.print("Score envoyé via MQTT: ");
    Serial.println(scoreMsg);
  }
}

// Fonction pour gérer le Game Over
void gameOver() {
  Serial.println("Mauvaise séquence ! Game Over");
  for (int i = 0; i < 3; i++) {
    lightLed(0); lightLed(1); lightLed(2); lightLed(3);
    delay(200);
    turnOffLeds();
    delay(200);
  }
  delay(2000);
  envoyerScoreMQTT(); // Envoi du score final avec le nom d'utilisateur
  gameActive = false;  // Désactive le jeu
}

// Callback appelé lors de la réception d'un message MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convertir le message en chaîne de caractères JSON
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Utiliser ArduinoJson pour analyser le message JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error) {
    // Extraire les données du JSON
    const char* action = doc["action"];
    const char* username = doc["username"];

    // Si l'action est "start", démarrer une nouvelle partie et stocker le nom d'utilisateur
    if (String(action) == "start" && username != nullptr) {
      currentUsername = String(username);
      startNewGame();
    }
  } else {
    Serial.println("Erreur de parsing JSON");
  }
}

// Connexion au Wi-Fi
void setupWiFi() {
  Serial.print("Connexion au WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnecté au WiFi");
}

// Connexion au broker MQTT
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connexion au broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connecté au broker MQTT");
      client.subscribe(mqttTopicControl); // S'abonne au topic de contrôle
    } else {
      Serial.print("Erreur de connexion MQTT : ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// Configuration initiale
void setup() {
  Serial.begin(115200);

  // Configuration des broches
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  randomSeed(analogRead(0)); // Initialiser le générateur de nombres aléatoires

  // Connexion au WiFi et au broker MQTT
  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback); // Définit le callback MQTT
  reconnectMQTT();
}

// Boucle principale
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

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
