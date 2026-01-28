#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "NTC_adc.h"
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Debug Ayarları
#define DEBUG_MODE true
#define SHT31_ADDRESS 0x44

// Telegram Bot Ayarları
const char* botToken = "";  // BotFather'dan aldığın token
const long telegramChatId = ;        // @userinfobot ile kendi chat ID'n
WiFiClientSecure telegramClient;
UniversalTelegramBot bot(botToken, telegramClient);
void sendTelegramMessage(const String& message) {
  if (WiFi.status() == WL_CONNECTED) {
    telegramClient.setInsecure();  // Sertifika doğrulamasını atla
    bot.sendMessage(String(telegramChatId), message, "");
    if (DEBUG_MODE) Serial.println("[Telegram] Mesaj gönderildi: " + message);
  } else {
    if (DEBUG_MODE) Serial.println("[Telegram] WiFi bağlantısı yok!");
  }
}


// WiFi Ayarları
const char* ssid = "Red";
const char* password = "123456789";

// HiveMQ Cloud Ayarları
const char* mqtt_server = "c002eefd9ed14c69849867b03b3a627d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "esp32iot";
const char* mqtt_pass = "Esp32iot";

// Pin Tanımlamaları
#define LED_PIN 5
#define LDR_PIN 32
#define NTC_PIN 34

Adafruit_SHT31 sht31;
bool sht31_initialized = false;
int ldr_deger = 0;
double ntc_deger = 0;

// MQTT Topic'leri
#define TOPIC_HUM "esp32iot/sensors/humidity"
#define TOPIC_LED "esp32iot/actuators/led"
#define TOPIC_LDR "esp32iot/sensors/light"
#define TOPIC_NTC "esp32iot/sensors/ntc_temp"
#define TOPIC_STATUS "esp32iot/status"
#define TOPIC_HEARTBEAT "esp32iot/heartbeat"
#define TOPIC_ERROR "esp32iot/error"

// Zamanlayıcı Ayarları
#define SENSOR_INTERVAL 10000
#define HEARTBEAT_INTERVAL 30000
#define RECONNECT_DELAY 5000

// Fonksiyon Prototipleri
void read_LDR_Sensor();
void read_NTC_Sensor();
void publishLedStatus();
void publishSystemStatus();
void sendSensorData();
void sendHeartbeat();
void reconnectMQTT();
void logError(const String& error);
String generateDashboard();
void handleRoot();
void handleLedOn();
void handleLedOff();
void handleSensorData();
bool initSHT31();

WebServer server(80);
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Değişkenler
float lastHum = 0;
bool lastLedState = false;
unsigned long lastSensorTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastReconnectAttempt = 0;


/******************** Sensör Okuma Fonksiyonları ********************/
void read_LDR_Sensor() {
  ldr_deger = analogRead(LDR_PIN);
}

void read_NTC_Sensor() {
  double Vout, Rt = 0, adc = 0;
  adc = analogRead(NTC_PIN);
  adc = ADC_LUT[(int)adc];
  Vout = adc * Vs/adcMax;
  Rt = R1 * Vout / (Vs - Vout);
  double T = 1/(1/To + log(Rt/Ro)/Beta); // Kelvin
  double Tc = T - 273.15; // Celsius
  if (Tc > 0) ntc_deger = Tc;
}

/******************** SHT31 Başlatma Fonksiyonu ********************/
bool initSHT31() {
  if(DEBUG_MODE) Serial.println("\nSHT31 sensörü başlatılıyor...");
  
  Wire.begin();
  if(!sht31.begin(SHT31_ADDRESS)) {
    if(DEBUG_MODE) {
      Serial.println("0x44 adresinde SHT31 bulunamadı! 0x45 deneniyor...");
      Serial.println("Olası nedenler:");
      Serial.println("1. Yanlış bağlantı (SDA:21, SCL:22)");
      Serial.println("2. Güç sorunu (3.3V ve GND)");
      Serial.println("3. Sensör arızalı");
    }
    
    if(!sht31.begin(0x45)) {
      if(DEBUG_MODE) Serial.println("0x45 adresinde de SHT31 bulunamadı!");
      return false;
    }
  }
  
  sht31_initialized = true;
  if(DEBUG_MODE) Serial.println("SHT31 başarıyla başlatıldı");
  return true;
}

/******************** WiFi Bağlantı ********************/
void setup_wifi() {
  Serial.println();
  Serial.print("WiFi ağına bağlanıyor: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi bağlantısı başarılı");
    Serial.print("IP adresi: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi bağlantısı başarısız!");
    ESP.restart();
  }
}

/******************** Hata Yönetimi ********************/
const char* mqttStateToString(int state) {
  switch(state) {
    case MQTT_CONNECTION_TIMEOUT: return "Bağlantı zaman aşımı";
    case MQTT_CONNECTION_LOST: return "Bağlantı kesildi";
    case MQTT_CONNECT_FAILED: return "Bağlantı başarısız";
    case MQTT_DISCONNECTED: return "Bağlantı yok";
    case MQTT_CONNECTED: return "Bağlantı başarılı";
    case MQTT_CONNECT_BAD_PROTOCOL: return "Geçersiz protokol";
    case MQTT_CONNECT_BAD_CLIENT_ID: return "Geçersiz client ID";
    case MQTT_CONNECT_UNAVAILABLE: return "Sunucu müsait değil";
    case MQTT_CONNECT_BAD_CREDENTIALS: return "Geçersiz kimlik bilgileri";
    case MQTT_CONNECT_UNAUTHORIZED: return "Yetkisiz erişim";
    default: return "Bilinmeyen hata";
  }
}

void logError(const String& error) {
  Serial.print("[HATA] ");
  Serial.println(error);
  
  if (mqttClient.connected()) {
    DynamicJsonDocument doc(256);
    doc["error"] = error;
    doc["timestamp"] = millis();
    String output;
    serializeJson(doc, output);
    mqttClient.publish(TOPIC_ERROR, output.c_str());
  }
}

/******************** MQTT Fonksiyonları ********************/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == TOPIC_LED) {
    if(message == "1" || message == "on" || message == "true") {
      digitalWrite(LED_PIN, HIGH);
      lastLedState = true;
    } else if(message == "0" || message == "off" || message == "false") {
      digitalWrite(LED_PIN, LOW);
      lastLedState = false;
    }
    publishLedStatus();
  }
}

void reconnectMQTT() {
  if (millis() - lastReconnectAttempt < RECONNECT_DELAY) return;
  
  lastReconnectAttempt = millis();
  Serial.println("MQTT sunucusuna bağlanılıyor...");

  espClient.setInsecure();
  
  String clientId = "ESP32Client-" + String(random(0xffff), HEX);
  if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println("MQTT bağlantısı başarılı!");
    mqttClient.subscribe(TOPIC_LED);
    publishSystemStatus();
    publishLedStatus();
  } else {
    String errorMsg = "MQTT bağlantı hatası: ";
    errorMsg += mqttStateToString(mqttClient.state());
    logError(errorMsg);
  }
}

void publishSystemStatus() {
  DynamicJsonDocument doc(256);
  doc["status"] = "online";
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["sht31_available"] = sht31_initialized;
  
  String output;
  serializeJson(doc, output);
  mqttClient.publish(TOPIC_STATUS, output.c_str());
}

void publishLedStatus() {
  DynamicJsonDocument doc(128);
  doc["led"] = lastLedState ? "on" : "off";
  doc["timestamp"] = millis();
  
  String output;
  serializeJson(doc, output);
  mqttClient.publish(TOPIC_LED "/status", output.c_str());
}

/******************** Sensör Fonksiyonları ********************/
void sendSensorData() {
  DynamicJsonDocument doc(256);
  
  // SHT31 nem verisi
  if (sht31_initialized) {
    float h = sht31.readHumidity();

    if (!isnan(h)) {
      lastHum = h;
      doc["humidity"] = round(h * 10) / 10.0;
    } else {
      doc["sht31_error"] = "Okuma hatası";
    }
  } else {
    doc["sht31_error"] = "Sensör bağlı değil";
  }
  if (ntc_deger > 27) {
    sendTelegramMessage("🔥 Uyarı! NTC sıcaklığı çok yüksek: " + String(ntc_deger, 1) + "°C");
    }
  // LDR ve NTC verileri
  read_LDR_Sensor();
  read_NTC_Sensor();
  doc["light"] = ldr_deger;
  doc["ntc_temperature"] = round(ntc_deger * 10) / 10.0;
  doc["timestamp"] = millis();
  
  String output;
  serializeJson(doc, output);
  mqttClient.publish(TOPIC_HUM, output.c_str());
  mqttClient.publish(TOPIC_LDR, output.c_str());
  mqttClient.publish(TOPIC_NTC, output.c_str());
  
  if(DEBUG_MODE) {
    Serial.print("Sensör verileri: ");
    Serial.print("Nem: "); Serial.print(lastHum); Serial.print("%, ");
    Serial.print("Işık: "); Serial.print(ldr_deger); Serial.print(", ");
    Serial.print("NTC Sıcaklık: "); Serial.print(ntc_deger); Serial.println("°C");
  }
}

void sendHeartbeat() {
  DynamicJsonDocument doc(256);
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  doc["freeMemory"] = ESP.getFreeHeap();
  doc["mqttStatus"] = mqttClient.connected() ? "connected" : "disconnected";
  doc["sht31_available"] = sht31_initialized;
  
  String output;
  serializeJson(doc, output);
  mqttClient.publish(TOPIC_HEARTBEAT, output.c_str());
  
  if(DEBUG_MODE) Serial.println("Heartbeat gönderildi");
}

/******************** Web Arayüzü Fonksiyonları ********************/
String generateDashboard() {
  float h = NAN;
  
  if (sht31_initialized) {
    h = sht31.readHumidity();
    if (isnan(h)) {
      h = lastHum;
    }
  }

  read_LDR_Sensor();
  read_NTC_Sensor();

  String humValue = isnan(h) ? "N/A" : String(h, 1) + "%";
  String ledStatus = lastLedState ? "AÇIK" : "KAPALI";
  String mqttStatus = mqttClient.connected() ? "Bağlı" : "Bağlantı Yok";
  int rssi = WiFi.RSSI();
  String wifiQuality;
  
  if (rssi >= -50) wifiQuality = "Mükemmel";
  else if (rssi >= -60) wifiQuality = "Çok İyi";
  else if (rssi >= -70) wifiQuality = "İyi";
  else if (rssi >= -80) wifiQuality = "Orta";
  else wifiQuality = "Zayıf";

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="tr">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <title>ESP32 IoT Kontrol Paneli</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      :root {
        --primary: #3498db;
        --primary-dark: #2980b9;
        --danger: #e74c3c;
        --danger-dark: #c0392b;
        --success: #2ecc71;
        --success-dark: #27ae60;
        --warning: #f39c12;
        --dark: #34495e;
        --light: #ecf0f1;
        --gray: #95a5a6;
      }
      body {
        background: #f8f9fa;
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      }
      .dashboard-header {
        background: var(--dark);
        color: white;
        padding: 1.5rem;
        margin-bottom: 1.5rem;
        border-radius: 0.5rem;
      }
      .card {
        border: none;
        border-radius: 0.5rem;
        box-shadow: 0 0.125rem 0.25rem rgba(0,0,0,0.075);
        margin-bottom: 1.5rem;
        transition: transform 0.3s;
      }
      .card:hover {
        transform: translateY(-5px);
        box-shadow: 0 0.5rem 1rem rgba(0,0,0,0.1);
      }
      .card-title {
        font-weight: 600;
        color: var(--dark);
        margin-bottom: 1rem;
      }
      .sensor-value {
        font-size: 2rem;
        font-weight: 700;
        margin: 0.5rem 0;
      }
      .hum-value { color: var(--primary); }
      .light-value { color: var(--warning); }
      .ntc-value { color: var(--success); }
      .sensor-label {
        color: var(--gray);
        font-size: 0.8rem;
        text-transform: uppercase;
        letter-spacing: 1px;
      }
      .btn-control {
        width: 100%;
        font-weight: 600;
        min-height: 44px;
        padding: 10px 15px;
      }
      .btn-on { background: var(--success); border: none; }
      .btn-off { background: var(--danger); border: none; }
      .status-badge {
        font-size: 0.8rem;
        padding: 0.35rem 0.5rem;
      }
      .chart-container {
        position: relative;
        height: 250px;
        width: 100%;
      }
      .wifi-excellent { color: var(--success); }
      .wifi-good { color: #2ecc71; }
      .wifi-fair { color: #f39c12; }
      .wifi-poor { color: var(--danger); }
      .btn-control.active {
        transform: scale(0.95);
        box-shadow: inset 0 3px 5px rgba(0, 0, 0, 0.2);
      }
      .btn-control:focus {
        outline: none;
        box-shadow: none;
      }

      /* Mobil cihazlar için özel stiller */
      @media (max-width: 768px) {
        .dashboard-header {
          padding: 1rem;
          margin-bottom: 1rem;
        }
        .card {
          margin-bottom: 1rem;
        }
        .sensor-value {
          font-size: 1.5rem;
        }
        .btn-control {
          padding: 0.5rem;
          font-size: 0.9rem;
        }
        .chart-container {
          height: 200px;
        }
        .card-title {
          font-size: 1.1rem;
        }
        .card-body {
          padding: 1rem;
        }
      }

      /* Çok küçük ekranlar için */
      @media (max-width: 480px) {
        .col-6 {
          width: 100%;
        }
        .sensor-value {
          font-size: 1.3rem;
        }
        body {
          overflow-x: hidden;
        }
      }
    </style>
  </head>
  <body>
    <div class="container py-4">
      <div class="dashboard-header text-center">
        <h1><i class="fas fa-tachometer-alt me-2"></i>ESP32 IoT Kontrol Paneli</h1>
        <p class="mb-0">Gerçek zamanlı sensör verileri ve kontrol</p>
      </div>

      <div class="row">
        <!-- Nem Kartı -->
        <div class="col-12 col-sm-6 col-md-6 col-lg-4">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title"><i class="fas fa-tint me-2"></i>Nem Sensörü</h5>
              <div class="hum-value" id="hum-value">)rawliteral" + humValue + R"rawliteral(</div>
              <div class="sensor-label">Bağıl Nem Oranı</div>
              <div class="chart-container mt-3">
                <canvas id="humidityChart"></canvas>
              </div>
            </div>
          </div>
        </div>

        <!-- Işık Sensörü Kartı -->
        <div class="col-12 col-sm-6 col-md-6 col-lg-4">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title"><i class="fas fa-lightbulb me-2"></i>Işık Sensörü</h5>
              <div class="light-value" id="light-value">)rawliteral" + String(ldr_deger) + R"rawliteral(</div>
              <div class="sensor-label">Işık Seviyesi (0-4095)</div>
              <div class="chart-container mt-3">
                <canvas id="lightChart"></canvas>
              </div>
            </div>
          </div>
        </div>

        <!-- NTC Sensörü Kartı -->
        <div class="col-12 col-sm-6 col-md-6 col-lg-4">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title"><i class="fas fa-thermometer-half me-2"></i>NTC Sensörü</h5>
              <div class="ntc-value" id="ntc-value">)rawliteral" + String(ntc_deger, 1) + "°C" + R"rawliteral(</div>
              <div class="sensor-label">Sıcaklık Değeri</div>
              <div class="chart-container mt-3">
                <canvas id="tempChart"></canvas>
              </div>
            </div>
          </div>
        </div>

        <!-- LED Kontrol Kartı -->
        <div class="col-12 col-sm-6 col-md-6 col-lg-4">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title"><i class="fas fa-lightbulb me-2"></i>Sistem Kontrolü</h5>
              <div class="d-flex justify-content-between align-items-center mb-3">
                <span>Durum:</span>
                <span class="badge bg-primary status-badge" id="led-status">)rawliteral" + ledStatus + R"rawliteral(</span>
              </div>
              <div class="row g-2">
                <div class="col-6">
                  <button class="btn btn-success btn-control btn-on" onclick="controlLed('on')">AÇ</button>
                </div>
                <div class="col-6">
                  <button class="btn btn-danger btn-control btn-off" onclick="controlLed('off')">KAPAT</button>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- Sistem Durumu Kartı -->
        <div class="col-12 col-sm-6 col-md-6 col-lg-4">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title"><i class="fas fa-info-circle me-2"></i>Sistem Durumu</h5>
              <ul class="list-group list-group-flush">
                <li class="list-group-item d-flex justify-content-between align-items-center">
                  <span>MQTT Bağlantısı:</span>
                  <span class="badge )rawliteral" + (mqttClient.connected() ? "bg-success" : "bg-danger") + R"rawliteral( status-badge">)rawliteral" + mqttStatus + R"rawliteral(</span>
                </li>
                <li class="list-group-item d-flex justify-content-between align-items-center">
                  <span>WiFi Gücü:</span>
                  <span class="wifi-)rawliteral" + wifiQuality + R"rawliteral(">)rawliteral" + String(rssi) + " dBm (" + wifiQuality + R"rawliteral()</span>
                </li>
                <li class="list-group-item d-flex justify-content-between align-items-center">
                  <span>IP Adresi:</span>
                  <span>)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</span>
                </li>
                <li class="list-group-item d-flex justify-content-between align-items-center">
                  <span>SHT31 Durumu:</span>
                  <span>)rawliteral" + (sht31_initialized ? "<span class='badge bg-success'>Bağlı</span>" : "<span class='badge bg-danger'>Bağlı değil</span>") + R"rawliteral(</span>
                </li>
              </ul>
            </div>
          </div>
        </div>

        <!-- Veri Log Kartı -->
        <div class="col-12 col-sm-6 col-md-6 col-lg-4">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title"><i class="fas fa-history me-2"></i>Veri Geçmişi</h5>
              <div class="chart-container">
                <canvas id="combinedChart"></canvas>
              </div>
            </div>
          </div>
        </div>
      </div>

      <footer class="text-center text-muted mt-4">
        <small>Son Güncelleme: <span id="last-update"></span></small>
      </footer>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js"></script>
    <script src="https://kit.fontawesome.com/a076d05399.js" crossorigin="anonymous"></script>
    <script>
      // Grafikler için veri yapıları
      const timeLabels = Array(10).fill('').map((_, i) => `${i*5}s önce`);
      let humData = Array(10).fill(0);
      let lightData = Array(10).fill(0);
      let tempData = Array(10).fill(0);

      // Grafikleri oluştur
      const humCtx = document.getElementById('humidityChart').getContext('2d');
      const humidityChart = new Chart(humCtx, {
        type: 'line',
        data: {
          labels: timeLabels,
          datasets: [{
            label: 'Nem (%)',
            data: humData,
            borderColor: 'rgba(52, 152, 219, 1)',
            backgroundColor: 'rgba(52, 152, 219, 0.1)',
            tension: 0.3,
            fill: true
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: { legend: { display: false } },
          scales: { y: { beginAtZero: false } }
        }
      });

      const lightCtx = document.getElementById('lightChart').getContext('2d');
      const lightChart = new Chart(lightCtx, {
        type: 'bar',
        data: {
          labels: timeLabels,
          datasets: [{
            label: 'Işık Seviyesi',
            data: lightData,
            backgroundColor: 'rgba(243, 156, 18, 0.7)',
            borderColor: 'rgba(243, 156, 18, 1)',
            borderWidth: 1
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: { legend: { display: false } },
          scales: { y: { beginAtZero: true } }
        }
      });

      const tempCtx = document.getElementById('tempChart').getContext('2d');
      const tempChart = new Chart(tempCtx, {
        type: 'line',
        data: {
          labels: timeLabels,
          datasets: [{
            label: 'Sıcaklık (°C)',
            data: tempData,
            borderColor: 'rgba(46, 204, 113, 1)',
            backgroundColor: 'rgba(46, 204, 113, 0.1)',
            tension: 0.3,
            fill: true
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: { legend: { display: false } },
          scales: { y: { beginAtZero: false } }
        }
      });

      const combinedCtx = document.getElementById('combinedChart').getContext('2d');
      const combinedChart = new Chart(combinedCtx, {
        type: 'line',
        data: {
          labels: timeLabels,
          datasets: [
            {
              label: 'Nem (%)',
              data: humData,
              borderColor: 'rgba(52, 152, 219, 1)',
              backgroundColor: 'rgba(52, 152, 219, 0.1)',
              yAxisID: 'y',
              tension: 0.3
            },
            {
              label: 'Sıcaklık (°C)',
              data: tempData,
              borderColor: 'rgba(46, 204, 113, 1)',
              backgroundColor: 'rgba(46, 204, 113, 0.1)',
              yAxisID: 'y1',
              tension: 0.3
            }
          ]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: { 
            legend: { 
              position: 'bottom',
              labels: {
                boxWidth: 12,
                font: {
                  size: window.innerWidth < 768 ? 10 : 12
                }
              }
            } 
          },
          scales: {
            y: {
              type: 'linear',
              display: true,
              position: 'left',
              title: { display: true, text: 'Nem (%)' },
              ticks: {
                font: {
                  size: window.innerWidth < 768 ? 8 : 10
                }
              }
            },
            y1: {
              type: 'linear',
              display: true,
              position: 'right',
              title: { display: true, text: 'Sıcaklık (°C)' },
              grid: { drawOnChartArea: false },
              ticks: {
                font: {
                  size: window.innerWidth < 768 ? 8 : 10
                }
              }
            },
            x: {
              ticks: {
                maxRotation: 45,
                minRotation: 45,
                font: {
                  size: window.innerWidth < 768 ? 8 : 10
                }
              }
            }
          }
        }
      });

      // LED kontrol fonksiyonu
      function controlLed(state) {
        fetch('/led_' + state, {
          method: 'GET'
        })
        .then(response => {
          if(response.ok) {
            // LED durumunu güncelle
            const ledStatus = document.getElementById('led-status');
            ledStatus.textContent = state === 'on' ? 'AÇIK' : 'KAPALI';
            ledStatus.className = state === 'on' ? 'badge bg-success status-badge' : 'badge bg-danger status-badge';
            
            // Butonları güncelle
            if(state === 'on') {
              document.querySelector('.btn-on').classList.add('active');
              document.querySelector('.btn-off').classList.remove('active');
            } else {
              document.querySelector('.btn-on').classList.remove('active');
              document.querySelector('.btn-off').classList.add('active');
            }
          }
        })
        .catch(error => console.error('Hata:', error));
      }

      // Veri güncelleme fonksiyonu
      function updateSensorData() {
        fetch('/sensor_data')
          .then(response => response.json())
          .then(data => {
            // Değerleri güncelle
            if(data.humidity !== undefined) {
              document.getElementById('hum-value').textContent = data.humidity.toFixed(1) + '%';
              humData.shift();
              humData.push(data.humidity);
              humidityChart.update();
            }
            
            if(data.light !== undefined) {
              document.getElementById('light-value').textContent = data.light;
              lightData.shift();
              lightData.push(data.light);
              lightChart.update();
            }
            
            if(data.ntc_temp !== undefined) {
              document.getElementById('ntc-value').textContent = data.ntc_temp.toFixed(1) + '°C';
              tempData.shift();
              tempData.push(data.ntc_temp);
              tempChart.update();
              combinedChart.update();
            }
            
            if(data.led_state !== undefined) {
              const ledStatus = document.getElementById('led-status');
              ledStatus.textContent = data.led_state ? 'AÇIK' : 'KAPALI';
              ledStatus.className = data.led_state ? 'badge bg-success status-badge' : 'badge bg-danger status-badge';
            }
            
            document.getElementById('last-update').textContent = new Date().toLocaleString();
          })
          .catch(error => console.error('Error:', error));
      }

      // Her 5 saniyede bir verileri güncelle
      setInterval(updateSensorData, 5000);
      
      // Sayfa yüklendiğinde ilk verileri al
      document.addEventListener('DOMContentLoaded', function() {
        updateSensorData();
        // Başlangıçta aktif butonu ayarla
        if()rawliteral" + (lastLedState ? "true" : "false") + R"rawliteral() {
          document.querySelector('.btn-on').classList.add('active');
        } else {
          document.querySelector('.btn-off').classList.add('active');
        }
      });
    </script>
  </body>
  </html>
  )rawliteral";

  return html;
}

void handleRoot() {
  server.send(200, "text/html", generateDashboard());
}

void handleSensorData() {
  DynamicJsonDocument doc(256);
  
  if (sht31_initialized) {
    float h = sht31.readHumidity();
    if (!isnan(h)) {
      lastHum = h;
      doc["humidity"] = h;
    }
  }
  
  doc["light"] = ldr_deger;
  doc["ntc_temp"] = ntc_deger;
  doc["led_state"] = lastLedState;
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleLedOn() {
  digitalWrite(LED_PIN, HIGH);
  lastLedState = true;
  mqttClient.publish(TOPIC_LED, "on");
  publishLedStatus();
  server.send(200, "text/plain", "LED açıldı");
}

void handleLedOff() {
  digitalWrite(LED_PIN, LOW);
  lastLedState = false;
  mqttClient.publish(TOPIC_LED, "off");
  publishLedStatus();
  server.send(200, "text/plain", "LED kapatıldı");
}

/******************** Ana Fonksiyonlar ********************/
void setup() {
  Serial.begin(115200);
  while(!Serial);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(LDR_PIN, INPUT);
  pinMode(NTC_PIN, INPUT);

  initSHT31();
  setup_wifi();
  sendTelegramMessage("🔧 Test mesajı: ESP32 Telegram bağlantısı çalışıyor mu?");
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
  server.on("/", handleRoot);
  server.on("/sensor_data", handleSensorData);
  server.on("/led_on", handleLedOn);
  server.on("/led_off", handleLedOff);
  server.begin();
  
  Serial.println("\nSistem başlatma tamamlandı");
  Serial.print("Web arayüzü: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  
  if (!mqttClient.connected()) {
    reconnectMQTT();
  } else {
    mqttClient.loop();
    
    unsigned long now = millis();
    if (now - lastSensorTime > SENSOR_INTERVAL) {
      lastSensorTime = now;
      sendSensorData();
    }
    
    if (now - lastHeartbeatTime > HEARTBEAT_INTERVAL) {
      lastHeartbeatTime = now;
      sendHeartbeat();
    }
  }
  delay(10);
}