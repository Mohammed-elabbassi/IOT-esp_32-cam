#include <WiFi.h> 
#include <HTTPClient.h>
#include "esp_camera.h"
#include "base64.h"

// Broches ESP32-CAM AI-Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid = "MLAIM";
const char* password = "11111111";
const char* apiUrl = "https://6844cd83fc51878754d9df1c.mockapi.io/capture";

// Configuration caméra
camera_config_t configCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  return config;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Démarrage ESP32-CAM...");

  WiFi.begin(ssid, password);
  Serial.print("Connexion au WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecté. Adresse IP : " + WiFi.localIP().toString());

  camera_config_t config = configCamera();
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erreur caméra: 0x%x\n", err);
    return;
  }
  Serial.println("Caméra initialisée avec succès.");
}

void loop() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck >= 5000) {
    lastCheck = millis();
    checkCaptureRequest();
  }
}

// Vérifie s’il y a une commande "capture"
void checkCaptureRequest() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi déconnecté");
    return;
  }

  HTTPClient http;
  String fullUrl = String(apiUrl) + "?order=createdAt&page=1&limit=1";
  http.begin(fullUrl);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    Serial.println("Réponse GET : " + response);

    int cmdIndex = response.indexOf("\"command\":\"capture\"");
    if (cmdIndex != -1) {
      Serial.println("Commande 'capture' détectée. Capture en cours...");
      captureAndSend();
    } else {
      Serial.println("Aucune commande 'capture' trouvée.");
    }
  } else {
    Serial.printf("Erreur GET : %d\n", httpCode);
  }

  http.end();
}

// Capture une photo et l’envoie en base64
void captureAndSend() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Erreur de capture");
    return;
  }

  String imageBase64 = base64::encode(fb->buf, fb->len);

  HTTPClient http;
  http.begin(apiUrl); 
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"status\":\"done\",\"image\":\"" + imageBase64 + "\"}";
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.printf("Image envoyée, réponse: %d\n", httpResponseCode);
  } else {
    Serial.printf("Erreur POST : %d\n", httpResponseCode);
  }

  http.end();
  esp_camera_fb_return(fb);
  Serial.println("Capture envoyée.");
}
