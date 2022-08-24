#include <PubSubClient.h>
#include <SensirionI2cScd30.h>
#include <Wire.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_GREY 0x5AEB // New colour

const char* WIFI_SSID     = "433_adsl2";
const char* WIFI_PASSWORD = "1234567890";

const char* MQTT_SERVER = "gogogo.kr";

char mac[13];
char msg[100];//mqtt publish 메시지
long lastMsgTime = -60000;//마지막 publish 시간

static char errorMessage[128];
static int16_t error;

WiFiClient wifiClient;
PubSubClient psClient(wifiClient);
SensirionI2cScd30 sensor;
TFT_eSPI tft = TFT_eSPI();

void setupSensor();
void setupWiFi();
void pubMessage(float co2, float tmpr, float hmd);
void displayTft(float co2, float tmpr, float hmd);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
      delay(100);
  }
  
  setupSensor();
  setupWiFi();

  psClient.setServer(MQTT_SERVER, 1883);

  tft.init();//lcd 화면 초기화
  tft.setRotation(1);
}

void loop() {
  float co2Concentration = 0.0;
  float temperature = 0.0;
  float humidity = 0.0;
  sensor.blockingReadMeasurementData(co2Concentration, temperature, humidity);
  pubMessage(co2Concentration, temperature, humidity);
}

void setupSensor(){
  Serial.println("----------------start setupSensor----------------");
  Wire.begin();
  sensor.begin(Wire, SCD30_I2C_ADDR_61);
  
  sensor.stopPeriodicMeasurement();
  sensor.softReset();
  delay(2000);
  uint8_t major = 0;
  uint8_t minor = 0;
  error = sensor.readFirmwareVersion(major, minor);
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute readFirmwareVersion(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }
  Serial.print("firmware version major: ");
  Serial.print(major);
  Serial.print("\t");
  Serial.print("minor: ");
  Serial.print(minor);
  Serial.println();
  error = sensor.startPeriodicMeasurement(0);
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }
  Serial.println("----------------end setupSensor----------------");
}

void setupWiFi(){
  Serial.println("----------------start setupWiFi----------------");

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  byte macByte[6];
  WiFi.macAddress(macByte);
  sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", macByte[5], macByte[4], macByte[3], macByte[2], macByte[1], macByte[0]);
  Serial.println("MAC address: ");
  Serial.println(mac);
  
  Serial.println("----------------end setupWiFi----------------");
}

void pubMessage(float co2, float tmpr, float hmd){
  long now = millis();
  if(now - lastMsgTime < 60000)//10초 마다 실행하도록
    return;

  Serial.println("----------------start pubMessage----------------");

  Serial.print("mqtt connecting...");
  while(!psClient.connected()) {
    Serial.print(".");
    psClient.connect("dcDeepSensorClient1");
    delay(1000);
  }
  Serial.println("done");
  
  sprintf(msg, "{\"mac\":\"%s\", \"co2\":%.2f, \"temperature\":%.2f, \"humidity\":%.2f}", mac, co2, tmpr, hmd);
  Serial.println(msg);
  psClient.publish("/dcDeep/sensor", msg);

  lastMsgTime = now;

  Serial.println("----------------end pubMessage----------------");

  displayTft(co2, tmpr, hmd);
}

void displayTft(float co2, float tmpr, float hmd){
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(1);

  tft.println("SCD-30 sensor");
  tft.print("mac: ");tft.println(mac);
  tft.print("ip: ");tft.println(WiFi.localIP());
  tft.print("ssid: ");tft.println(WIFI_SSID);
  tft.print("password: ");tft.println(WIFI_PASSWORD);
  tft.print("co2: ");tft.println(co2);
  tft.print("temperature: ");tft.println(tmpr);
  tft.print("humidity: ");tft.println(hmd);
}
