#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <NewPing.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>

//BT Maintenance libraries

#include <BluetoothSerial.h>
#include <WebServer.h>
#include <Preferences.h>

String receivedMessage = "";

BluetoothSerial SerialBT;

WebServer server1(8080); // Use port 8080 for HTTP

Preferences preferences;

String ssid;
String password;
String BTname="ESP32-1";

const char* RESET_KEYWORD = "RESET";

WiFiServer server(80);
const char* host = "192.168.200.168"; // IP address of the receiver ESP32

IPAddress local_IP("192.168.200.40"); // Set your desired static IP address

IPAddress gateway("192.168.200.1"); 
IPAddress subnet("255.255.255.0"); // Subnet mask for iPhone hotspot
IPAddress primaryDNS("8.8.8.8"); // Optional: Google DNS
IPAddress secondaryDNS("8.8.4.4"); // Optional: Google DNS

const byte ROW_NUM = 4;
const byte COLUMN_NUM = 3;
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte pin_rows[ROW_NUM] = {26, 16, 17, 33}; // Updated pin numbers for ESP32
byte pin_column[COLUMN_NUM] = {25, 27, 32}; // Updated pin numbers for ESP32
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

unsigned long lastKeyPressTime = 0;
const unsigned long debounceDelay = 200; // Increased debounce delay
#define mySerial Serial2 // Use Serial2 for fingerprint sensor on ESP32
const unsigned long WiFiCheckInterval = 30000;
unsigned long lastWiFiCheckTime = 0;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const int RELAY_PIN = 4;
const int trigPin = 13; // HC-SR04 Trig pin
const int echoPin = 14; // HC-SR04 Echo pin
const int extled = 15;

NewPing sonar(trigPin, echoPin); // Define the HC-SR04 sensor

#define SS_PIN 5  // RFID SS pin
#define RST_PIN 0 // RFID RST pin
#define SCK_PIN 18  // RFID SCK pin
#define MOSI_PIN 23 // RFID MOSI pin
#define MISO_PIN 19 // RFID MISO pin

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

void setup() {
  Serial.begin(115200);

  SerialBT.begin(BTname); 
  Serial.println("Bluetooth initialized"); // Debugging line
   // Connect to WiFi
   if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  // Open preferences with namespace "wifi"
  preferences.begin("wifi", false); 
  Serial.println("Preferences opened"); // Debugging line

  preferences.putString("BT", BTname); 
  // Retrieve stored credentials
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");

  if (ssid == "" || password == "") {
    Serial.println("No stored WiFi credentials. Waiting for Bluetooth client...");
    delay(500); // Allow time for message to be sent

    // Wait for Bluetooth client connection
    while (!SerialBT.available()) {
      delay(1000);
    }

    Serial.println("Bluetooth client connected!");
    delay(500); // Allow time for message to be sent
    getWiFiCredentials();

    // Store credentials in preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
  }

  for (int i = 0; i < ROW_NUM; i++) {
    pinMode(pin_rows[i], INPUT_PULLUP);
  }
  for (int i = 0; i < COLUMN_NUM; i++) {
    pinMode(pin_column[i], INPUT_PULLUP);
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(extled, OUTPUT); // Initialize 27 pin as output for the LED
  
  // Initialize HC-SR04 sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize fingerprint sensor
  mySerial.begin(57600, SERIAL_8N1, 22, 21);  // RX on GPIO 16, TX on GPIO 17
  finger.begin(57600);
  unsigned long start = millis();
  while (!finger.verifyPassword()) {
    if (millis() - start > 5000) {  // Wait for 5 seconds to find the sensor
      Serial.println(F("Did not find fingerprint sensor, restarting..."));
      ESP.restart();  // Reset the ESP32 board
    }
    delay(100);
  }
  Serial.println(F("Found fingerprint sensor!"));

  // Initialize RFID reader
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  unsigned long lastDistanceCheckTime = 0;
  const unsigned long distanceCheckInterval = 1000; // Check distance every second

  bool fingerprintAuthenticating = false;
  bool rfidAuthenticating = false;
  bool passwordAuthenticating = false;

 // Connect to WiFi
  connectToWiFi();
 
  // server1.on("/", HTTP_GET, handleRoot);
  // New route for activating the relay
  server1.on("/activate", HTTP_GET, handleActivateRelay); 

  server1.begin();

  Serial.println(F("HTTP server started"));

  do {

    WiFiClient client = server.available();
    // Handle client requests
    server1.handleClient(); 

    // Check for reset command
    checkForResetCommand();

    if (client) {
      Serial.println("Client connected");

      // Read the request
      String request = client.readStringUntil('\r');
      Serial.println(request);
     
      receivedMessage = request;
      Serial.println(receivedMessage);

      // Send a response to the client
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println();
      client.println("<html><body><h1>Message received</h1></body></html>");
      client.stop(); // Close the connection

    }
    if (millis() - lastWiFiCheckTime >= WiFiCheckInterval) {
    lastWiFiCheckTime = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Restarting...");
      ESP.restart();
    }
  }
    if (receivedMessage.indexOf("unlocked") >= 0) {
      digitalWrite(extled, LOW); // Turn off exterior LED
      controlRelay();
      ESP.restart();
    }
    // Check distance every second
    if (millis() - lastDistanceCheckTime >= distanceCheckInterval) {
      lastDistanceCheckTime = millis(); // Update last check time

      // Read distance from HC-SR04 sensor
      unsigned int distance = sonar.ping_cm();
     
      // If distance is less than 30 cm, turn on LED
      if (distance < 30) {
        digitalWrite(extled, HIGH); // Turn on LED connected to 27
      } else {
        digitalWrite(extled, LOW); // Turn off LED connected to 27
      }
    }
    
    if (!fingerprintAuthenticating) {
      // If not already authenticating with fingerprint, attempt fingerprint authentication
      fingerprintAuthenticating = fingerprintAuthenticate();
    }

    if (!rfidAuthenticating) {
      // If not already authenticating with RFID, attempt RFID authentication
      rfidAuthenticating = rfidAuthenticate();
    }

    // If any authentication method is successful, break out of the loop
    if (fingerprintAuthenticating || rfidAuthenticating) {
      break;
    }

    if (!passwordAuthenticating) {
      // If not already authenticating with password, attempt password authentication
      passwordAuthenticating = passwordAuthenticate();
    }

  } while (true); // Loop indefinitely until authentication succeeds
 // Close preferences
  preferences.end(); 

  // Restart the ESP32 after successful authentication
  ESP.restart();
}

bool fingerprintAuthenticate() {
  Serial.println(F("\nAttempting fingerprint authentication..."));

  // Capture fingerprint image
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("\nFingerprint sensor error. Please try again."));
    return false;
  }

  // Convert fingerprint image to template
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("\nFingerprint image conversion failed. Please try again."));
    return false;
  }

  // Search for fingerprint match
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println(F("\nFingerprint authentication successful!"));
    
    sendMessage("   Door unlocked\n\n    (Fingerprint)\r\n");

    digitalWrite(extled, LOW); // Turn off extled after successful authentication
    controlRelay();
    return true;
  } else {
    Serial.println(F("\nFingerprint authentication failed."));
    
    sendMessage("      Invalid \n\n     fingerprint\r\n");

    return false;
  }
}

bool passwordAuthenticate() {
  Serial.println(F("\nPlease enter password:"));
  String password = "";
  unsigned long startTime = millis();
  bool timedOut = false; // Flag to track if time limit is reached
  do {
    char key = keypad.getKey();
    if ((key >= '0' && key <= '9') || key == '*' || key == '#') {
      if (millis() - lastKeyPressTime > debounceDelay) {
        password += key;
        Serial.print(key); // Print actual key for each entered digit
        lastKeyPressTime = millis();
      }
    }
    if (millis() - startTime >= 5000) {
      timedOut = true;
    }
    delay(100); // Delay to avoid multiple key inputs
  } while (password.length() < 4 && !timedOut);

  bool success = password == "8888";

  if (success && !timedOut) { // Check if password is correct and not timed out
    Serial.println(F("\nPassword authentication successful!"));
    
    sendMessage("   Door unlocked\n\n     (Password)\r\n");

    digitalWrite(extled, LOW); // Turn off extled after successful authentication
    controlRelay();
  } else {
    if (!timedOut) {
      Serial.println(F("\nPassword authentication failed."));
      
      sendMessage("      Invalid\n\n      Password\r\n");
    }
  }
  return success && !timedOut; // Return success only if password is correct and not timed out
}

bool rfidAuthenticate() {
  Serial.println(F("\nAttempting RFID authentication..."));
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      Serial.print("UID:");
      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        uid += String(rfid.uid.uidByte[i], HEX);
      }
      Serial.println(uid);
      // Expected UID: D8 8D CA 9E
      if (rfid.uid.uidByte[0] == 0xD8 &&
          rfid.uid.uidByte[1] == 0x8D &&
          rfid.uid.uidByte[2] == 0xCA &&
          rfid.uid.uidByte[3] == 0x9E) {
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        Serial.println("RFID authentication successful!");

        sendMessage("   Dor unlocked\n\n       (RFID)\r\n");

        digitalWrite(extled, LOW); // Turn off extled after successful authentication
        controlRelay();
        
        return true;
      } else {
        Serial.println("RFID authentication failed! (Incorrect UID)");
        
        sendMessage("     Invalid\n\n        RFID\r\n");

        return false;
      }
    }
    delay(100); // Small delay to avoid overloading the RFID reader
  }
  Serial.println("RFID authentication failed! (Timeout)");
  return false;
}

void loop() {
  // Empty loop as main authentication is handled in setup
}

void controlRelay() {
  Serial.println("Turning relay ON for 30 seconds.");
  digitalWrite(RELAY_PIN, LOW); // Turn on the relay
  delay(30000);
  digitalWrite(RELAY_PIN, HIGH); // Turn on the relay
}

void sendMessage(String message) {
  bool messageSent = false;
  int retryCount = 0;
  const int maxRetries = 5;

  while (!messageSent && retryCount < maxRetries) {
    WiFiClient client;
    if (client.connect(host, 80)) {
      // Send the message
      client.print(message);
      client.print("Host: ");
      client.print(host);
      client.print("\r\n");
      client.print("Connection: close\r\n\r\n");
      delay(10);

      Serial.println("Message sent");
      client.stop();
      messageSent = true;
    } else {
      Serial.println("Connection failed, retrying...");
      delay(1000); // Wait 1 second before retrying
      retryCount++;
    }
  }

  if (!messageSent) {
    Serial.println("Failed to send message after 5 attempts.");
  }
}

// Maintenance functions

void handleRoot() {
  server1.send(200, "text/html", "<html><body><h1>Hello from ESP32-CAM!</h1></body></html>");
}
void handleActivateRelay() {
  controlRelay(); // Call the controlRelay function
  server1.send(200, "text/html", "<html><body><h1>Relay Activated</h1></body></html>"); // Response to the client
}


void getWiFiCredentials() {
  String ssid_str = "";
  String password_str = "";

  SerialBT.println("SSID:");
  delay(500); // Allow time for message to be sent
  while (SerialBT.available() == 0) {
    delay(500);
  }
  ssid_str = SerialBT.readStringUntil('\n');
  ssid_str.trim();
  ssid = ssid_str;

  SerialBT.println("Password:");
  delay(500); // Allow time for message to be sent
  while (SerialBT.available() == 0) {
    delay(500);
  }
  password_str = SerialBT.readStringUntil('\n');
  password_str.trim();
  password = password_str;
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  delay(500); // Allow time for message to be sent

  WiFi.begin(ssid.c_str(), password.c_str());

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 10) {
    SerialBT.print(".");
    delay(500);
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected");
    delay(500); // Allow time for message to be sent
    Serial.println(WiFi.localIP());
    delay(500); // Allow time for message to be sent

    // Send IP address and port to the Bluetooth client
    String ipInfo = "BTname:" + BTname + " IP:http://" + WiFi.localIP().toString() + ":8080\n";
    SerialBT.println(ipInfo);
    delay(500); // Allow time for message to be sent
  } else {
    SerialBT.println("Connection failed. Check credentials or network.");
    Serial.println("Connection failed. Check credentials or network.");
    delay(500); // Allow time for message to be sent
  }
}


void checkForResetCommand() {
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    if (command.equals(RESET_KEYWORD)) {
      resetCredentials();
    } else if (command.equals("INFO")) {
      sendInfo();
    }
  }
}

void sendInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    String ipInfo = "BTname:" + BTname + " IP:http://" + WiFi.localIP().toString() + ":8080\n"; 
    SerialBT.println(ipInfo);
  } else {
    SerialBT.println("WiFi not connected.");
  }
}


void resetCredentials() {
  SerialBT.println("Resetting WiFi credentials...");
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  SerialBT.println("Credentials reset");
  ESP.restart();
}


