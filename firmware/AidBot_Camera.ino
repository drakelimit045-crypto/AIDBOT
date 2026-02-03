#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <WebServer.h>

// ---- Wi-Fi credentials ----
#define WIFI_SSID "PepperMutaLemon"
#define WIFI_PASSWORD "M@y151986"

// ---- Camera pins (OV2640 for ESP32-CAM) ----
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

WebServer server(80);

void initCamera() {
  camera_config_t camera_config;
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d7 = Y9_GPIO_NUM;
  camera_config.pin_d6 = Y8_GPIO_NUM;
  camera_config.pin_d5 = Y7_GPIO_NUM;
  camera_config.pin_d4 = Y6_GPIO_NUM;
  camera_config.pin_d3 = Y5_GPIO_NUM;
  camera_config.pin_d2 = Y4_GPIO_NUM;
  camera_config.pin_d1 = Y3_GPIO_NUM;
  camera_config.pin_d0 = Y2_GPIO_NUM;
  camera_config.pin_vsync = VSYNC_GPIO_NUM;
  camera_config.pin_href = HREF_GPIO_NUM;
  camera_config.pin_pclk = PCLK_GPIO_NUM;
  camera_config.pin_pwdn = PWDN_GPIO_NUM;
  camera_config.pin_reset = RESET_GPIO_NUM;
  camera_config.pin_xclk = XCLK_GPIO_NUM;
  camera_config.pin_sscb_sda = SIOD_GPIO_NUM;
  camera_config.pin_sscb_scl = SIOC_GPIO_NUM;

  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_JPEG;
  camera_config.frame_size = FRAMESIZE_VGA;
  camera_config.jpeg_quality = 10;
  camera_config.fb_count = 1;

  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");
}

void handleStream() {
  server.sendHeader("Content-Type", "multipart/x-mixed-replace; boundary=frame");
  server.send(200, "multipart/x-mixed-replace; boundary=frame", "");
  
  while (server.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    
    server.sendContent("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\n\r\n");
    server.client().write(fb->buf, fb->len);
    server.sendContent("\r\n");
    esp_camera_fb_return(fb);
    
    delay(30); // ~30 FPS
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32-CAM Starting...");

  // Wi-Fi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Wi-Fi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to Wi-Fi");
  }

  // Initialize camera
  initCamera();

  // Web server routes
  server.on("/stream", handleStream);
  server.on("/", []() {
    server.send(200, "text/html", "<h1>ESP32-CAM Streaming</h1><p>Stream URL: /stream</p>");
  });
  
  server.begin();
  Serial.println("Web server started on port 80");
  Serial.print("Camera stream URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

void loop() {
  server.handleClient();
}
