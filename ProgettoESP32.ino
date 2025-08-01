// Controllo Computer tramite Alexa con ESP32
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// Configura le credenziali WiFi
const char* ssid = "WIFI-Name";
const char* password = "WIFI-Password";

// Credenziali Sinric Pro (da ottenere su sinric.pro)
#define APP_KEY    "APP_KEY"      // Da sinric.pro
#define APP_SECRET "APP_SECRET"   // Da sinric.pro
#define DEVICE_ID_1 "DEVICE_ID_1" // ID per Computer 1
#define DEVICE_ID_2 "DEVICE_ID_2" // ID per Reset Forzato

// Pin GPIO per controllo computer
const int powerPin = 4;   // Pin per accensione computer
const int resetPin = 4;   // Pin per reset forzato
const int buttonPin = 15;  // Pin per pulsante fisico

// Variabili per gestione pulsante
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Callback per accensione/spegnimento computer
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Computer %s\n", state ? "ACCESO" : "SPENTO");
  
  // Esegue sempre l'impulso del pulsante power (sia per accendere che spegnere)
  digitalWrite(powerPin, LOW);   // Impulso basso
  delay(1000);                   // 1 secondo
  digitalWrite(powerPin, HIGH);  // Rilascia
  
  if (state) {
    Serial.println("Comando accensione computer inviato!");
  } else {
    Serial.println("Comando spegnimento computer inviato!");
  }
  
  return true; // Conferma comando ricevuto
}

// Callback per reset forzato
bool onResetState(const String &deviceId, bool &state) {
  Serial.printf("Reset forzato %s\n", state ? "AVVIATO" : "FERMATO");
  
  if (state) {
    Serial.println("Reset forzato in corso (15 secondi)...");
    digitalWrite(resetPin, LOW);   // Impulso basso
    delay(15000);                  // 15 secondi
    digitalWrite(resetPin, HIGH);  // Rilascia
    Serial.println("Reset completato!");
    
    // Auto-spegni il dispositivo virtuale dopo reset
    SinricProSwitch& mySwitch = SinricPro[DEVICE_ID_2];
    mySwitch.sendPowerStateEvent(false);
  }
  
  return true;
}

// Funzione per eseguire il comando power
void executePowerCommand() {
  Serial.println("Comando power eseguito tramite pulsante fisico!");
  digitalWrite(powerPin, LOW);   // Impulso basso
  delay(1000);                   // 1 secondo
  digitalWrite(powerPin, HIGH);  // Rilascia
  Serial.println("Impulso power completato!");
}

// Funzione per gestire il pulsante fisico
void handleButton() {
  int reading = digitalRead(buttonPin);
  
  // Se lo stato è cambiato, resetta il timer di debounce
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // Se è passato abbastanza tempo, considera il cambio di stato valido
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Se lo stato del pulsante è effettivamente cambiato
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      // Se il pulsante è stato premuto (transizione da HIGH a LOW)
      if (currentButtonState == LOW) {
        executePowerCommand();
      }
    }
  }
  
  lastButtonState = reading;
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connessione WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }
  
  Serial.println("\nWiFi connesso!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setupSinricPro() {
  // Configura dispositivo 1 (Computer)
  SinricProSwitch& computer = SinricPro[DEVICE_ID_1];
  computer.onPowerState(onPowerState);

  // Configura dispositivo 2 (Reset)
  SinricProSwitch& resetSwitch = SinricPro[DEVICE_ID_2];
  resetSwitch.onPowerState(onResetState);

  // Avvia Sinric Pro
  SinricPro.onConnected([]() {
    Serial.println("Connesso a Sinric Pro!");
  });
  
  SinricPro.onDisconnected([]() {
    Serial.println("Disconnesso da Sinric Pro!");
  });

  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  Serial.begin(115200);
  
  // Configura pin
  pinMode(powerPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Pulsante con pull-up interno
  digitalWrite(powerPin, HIGH);  // Stato iniziale alto
  digitalWrite(resetPin, HIGH);  // Stato iniziale alto
  
  setupWiFi();
  setupSinricPro();
  
  Serial.println("Sistema pronto! Puoi:");
  Serial.println("- Dire 'Alexa, accendi computer' - per accendere");
  Serial.println("- Dire 'Alexa, spegni computer' - per spegnere");
  Serial.println("- Dire 'Alexa, accendi reset computer' - per reset forzato");
  Serial.println("- Premere il pulsante fisico sul pin 2 - per accendere/spegnere");
}

void loop() {
  SinricPro.handle();
  handleButton();  // Gestisce il pulsante fisico
}
