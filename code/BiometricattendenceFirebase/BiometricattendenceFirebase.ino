#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <Adafruit_SSD1306.h>
#include <FirebaseESP32.h>

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Adafruit_SSD1306 display(128, 32, &Wire, -1);
FirebaseData firebaseData;

const char* ssid = "Madhav";          // Replace with your own SSID
const char* password = "0987654321";  // Replace with your own password
const char* host = "api.pushingbox.com";

String member = "";
int flag = 0;

void setup()
{
  Serial.begin(115200);
  delay(100);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  Serial.println();
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  delay(1500);

  Serial.println("Waiting for Fingerprint Sensor...");
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Fingerprint sensor found");
    delay(1000);
  }
  else
  {
    Serial.println("Fingerprint sensor not found!");
    while (1);
  }

  // Initialize Firebase
  Firebase.begin("biometric-attendence-59b5d-default-rtdb.firebaseio.com/", "km2502DkQOeGu5WT2UHXynf3RAsmwGF6wlEeAhaW");
  Firebase.reconnectWiFi(true);
}

void loop()
{
  int fingerprintID = getFingerprintID();
  delay(10000);

  if (fingerprintID >= 0)
  {
    String name = getMemberName(fingerprintID);
    if (name != "")
    {
     Serial.println("Welcome " + name);
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("Welcome");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.println(name);
      display.display();

      // Update Firebase
      updateFirebase(name);
      connectHost(name);

      flag = 0;
    }
    else
    {
      Serial.println("Unknown fingerprint!");
    }
  }
  else if (fingerprintID == -1)
  {
    Serial.println("Did not match any fingerprints!");
    Serial.println("Place a valid fingerprint");
  }
  else
  {
    Serial.println("Waiting for a fingerprint...");
  }
}

int getFingerprintID()
{
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)
    return -2;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)
    return -2;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)
    return -1;

  return finger.fingerID;
}

String getMemberName(int fingerprintID)
{
  switch (fingerprintID)
  {
    case 1:
      return "Madhav";
    case 2:
      return "Karthik";
    case 3:
      return "Sashikanth";
    case 4:
      return "Chaitanya";
    default:
      return "";
  }
}

void updateFirebase(String name)
{
  if (flag == 0)
  {
    member = name;
    flag = 1;

    if (Firebase.set(firebaseData, "/attendance/Time_in", member))
    {
      Serial.println("Time in updated on Firebase");
    }
    else
    {
      Serial.println("Failed to update Time in on Firebase");
    }
  }
}

void connectHost(String data)
{
  if (flag == 0)
  {
    member = data;
    flag = 1;
    Serial.print("Connecting to host: ");
    Serial.println(host);

    WiFiClient client;
    const int httpPort = 80;

    if (!client.connect(host, httpPort))
    {
      Serial.println("Connection failed");
      return;
    }

    String url = "/pushingbox?";
    url += "devid=";
    url += "vB7C7DFA39A86A00";
    url += "&Name=" + member;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.connected() && !client.available())
    {
      if (millis() - timeout > 5000)
      {
        Serial.println("Client Timeout!");
        client.stop();
        return;
      }
    }

    while (client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    Serial.println();
    Serial.println("Data Sent!");
    Serial.println("Closing connection");
    client.stop();
  }
}
