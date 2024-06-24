#include <WiFi.h>
#include <BluetoothSerial.h>
#include <WebServer.h>
#include <Preferences.h>

BluetoothSerial SerialBT;
WebServer server(8080); // Use port 8080 for HTTP
Preferences preferences;

String ssid;
String password;
String BTname="ESP32-CAM";

const char* RESET_KEYWORD = "RESET";

void handleRoot() {
  server.send(200, "text/html", "<html><body><h1>Hello from ESP32-CAM!</h1></body></html>");
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32-CAM"); // Bluetooth device name

  preferences.begin("wifi", false); // Open preferences with namespace "wifi"

  preferences.putString("BT", BTname); 
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
  }

  // // Debug print to verify stored credentials
  // Serial.println("Stored credentials after reading preferences:");
  // Serial.print("SSID: "); Serial.println(ssid);
  // Serial.print("Password: "); Serial.println(password);

  displayStoredCredentials();

  // Attempt to connect to WiFi
  connectToWiFi();

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);

  server.begin();
  Serial.println("HTTP server started");

  preferences.end(); // Close preferences
}

void loop() {
  server.handleClient(); // Handle client requests

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

  
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 10) {
    Serial.print(".");
    delay(500);
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Send IP address and port to the Bluetooth client
    String ipInfo = "IP: " + WiFi.localIP().toString() + ", Port: 8080\n";
    SerialBT.println(ipInfo);

  } else {
    Serial.println("Connection failed. Check credentials or network.");
    SerialBT.println("Connection failed. Check credentials or network.");
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
    String ipInfo = "http://" + WiFi.localIP().toString() + ": 8080\n";
    SerialBT.println(BTname); 
    SerialBT.println(ipInfo);
  } else {
    SerialBT.println("WiFi not connected.");
  }
}


void resetCredentials() {
  Serial.println("Resetting WiFi credentials...");
  preferences.begin("wifi", false);
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
