#include <WiFi.h>
#include <BluetoothSerial.h>
#include <WebServer.h>
#include <Preferences.h>

BluetoothSerial SerialBT;
WebServer server1(8080); // Use port 8080 for HTTP
Preferences preferences;

String ssid;
String password;
String BTname="ESP32-CAM";

const char* RESET_KEYWORD = "RESET";

void handleRoot() {
  server1.send(200, "text/html", "<html><body><h1>Hello from ESP32-CAM!</h1></body></html>");
}

void setup() {
  Serial.begin(115200);
  // Bluetooth device name
  SerialBT.begin(BTname); 
  Serial.println("Bluetooth initialized"); // Debugging line

  // Open preferences with namespace "wifi"
  preferences.begin("wifi", false); 
  Serial.println("Preferences opened"); // Debugging line

  preferences.putString("BT", BTname); 
  // Retrieve stored credentials
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");

  if (ssid == "" || password == "") {
    SerialBT.println("No stored WiFi credentials. Waiting for Bluetooth client...");
    delay(500); // Allow time for message to be sent

    // Wait for Bluetooth client connection
    while (!SerialBT.available()) {
      delay(1000);
    }

    SerialBT.println("Bluetooth client connected!");
    delay(500); // Allow time for message to be sent
    getWiFiCredentials();

    // Store credentials in preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
  }

  // Attempt to connect to WiFi
  connectToWiFi();

  // Setup web server routes
  server1.on("/", HTTP_GET, handleRoot);

  server1.begin();

  // Close preferences
  preferences.end(); 
}

void loop() {
  // Handle client requests
  server1.handleClient(); 

  // Check for reset command
  checkForResetCommand();

  delay(1);
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
  SerialBT.println("Connecting to WiFi...");
  delay(500); // Allow time for message to be sent

  WiFi.begin(ssid.c_str(), password.c_str());

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 10) {
    SerialBT.print(".");
    delay(500);
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    SerialBT.println("Connected");
    delay(500); // Allow time for message to be sent
    SerialBT.println(WiFi.localIP());
    delay(500); // Allow time for message to be sent

    // Send IP address and port to the Bluetooth client
    String ipInfo = "BTname:" + BTname + " IP:http://" + WiFi.localIP().toString() + ":8080\n";
    SerialBT.println(ipInfo);
    delay(500); // Allow time for message to be sent
  } else {
    SerialBT.println("Connection failed. Check credentials or network.");
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

