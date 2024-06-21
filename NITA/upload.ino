#include <WiFi.h>
#include <BluetoothSerial.h>
#include <WebServer.h>
#include <Preferences.h>

BluetoothSerial SerialBT;
WebServer server(8080); // Use port 8080 for HTTP
Preferences preferences;

String ssid;
String password;

const char* RESET_KEYWORD = "RESET";

void handleRoot() {
  server.send(200, "text/html", "<html><body><h1>Hello from ESP32-CAM!</h1></body></html>");
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32-CAM"); // Bluetooth device name
  
  preferences.begin("wifi-credentials", false); // Open preferences with namespace "wifi-credentials"

  // Retrieve stored credentials
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");

  if (ssid == "" || password == "") {
    Serial.println("No stored WiFi credentials. Waiting for Bluetooth client...");
    
    // Wait for Bluetooth client connection
    while (!SerialBT.available()) {
      delay(1000);
    }

    Serial.println("Bluetooth client connected!");
    getWiFiCredentials();

    // Store credentials in preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    // Attempt to connect to WiFi
    connectToWiFi();
  } else {
    Serial.println("Using stored WiFi credentials:");
    Serial.print("SSID: "); Serial.println(ssid);
    Serial.print("Password: "); Serial.println(password);

    // Attempt to connect to WiFi
    connectToWiFi();
  }

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);

  server.begin();
  Serial.println("HTTP server started");

  // Display stored WiFi credentials
  displayStoredCredentials();

  preferences.end(); // Close preferences
}

void loop() {
  server.handleClient(); // Handle client requests

  // Reconnect to WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected! Retrying...");
    connectToWiFi(); // Attempt to reconnect if not connected
  }

  // Check for reset command
  checkForResetCommand();

  delay(1000);
}

void getWiFiCredentials() {
  String ssid_str = "";
  String password_str = "";

  Serial.println("Waiting for SSID...");
  while (SerialBT.available() == 0) {
    delay(500);
  }
  ssid_str = SerialBT.readStringUntil('\n');
  ssid_str.trim();
  ssid = ssid_str;
  Serial.println("SSID received: " + ssid_str);

  Serial.println("Waiting for password...");
  while (SerialBT.available() == 0) {
    delay(500);
  }
  password_str = SerialBT.readStringUntil('\n');
  password_str.trim();
  password = password_str;
  Serial.println("Password received: " + password_str);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("Password: "); Serial.println(password);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 10) {
    delay(1000);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Connection failed. Check credentials or network.");
  }
}

void checkForResetCommand() {
  // Check for reset command
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    if (command.equals(RESET_KEYWORD)) {
      resetCredentials();
    }
  }
}

void resetCredentials() {
  Serial.println("Resetting WiFi credentials...");
  preferences.begin("wifi-credentials", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  Serial.println("Credentials reset. Restarting...");
  ESP.restart();
}

void displayStoredCredentials() {
  Serial.println("Stored WiFi credentials:");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("Password: "); Serial.println(password);
}
