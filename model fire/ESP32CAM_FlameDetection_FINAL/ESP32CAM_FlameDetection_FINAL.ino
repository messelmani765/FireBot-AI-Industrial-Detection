#include <robot_fire_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <WebServer.h>
#include <WiFi.h>

// ── WiFi ──────────────────────────────────────────────
const char* ssid     = "Redmi Note 13";
const char* password = "Mohamed123";

// ── Modèle caméra : AI Thinker (module noir classique) ──
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// ── Constantes ─────────────────────────────────────────
#define CAM_W 320
#define CAM_H 240
#define FRAME_BYTES 3
#define FIRE_THRESHOLD 0.7f   // seuil de confiance

WebServer server(80);

static bool     is_initialised = false;
uint8_t*        snapshot_buf   = nullptr;
bool            fire_detected  = false;
float           fire_confidence = 0.0f;
ei_impulse_result_bounding_box_t fire_boxes[10];
uint32_t        fire_boxes_count = 0;

// ── Config caméra ──────────────────────────────────────
static camera_config_t camera_config = {
  .pin_pwdn  = PWDN_GPIO_NUM, .pin_reset = RESET_GPIO_NUM,
  .pin_xclk  = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM, .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7=Y9_GPIO_NUM,.pin_d6=Y8_GPIO_NUM,.pin_d5=Y7_GPIO_NUM,
  .pin_d4=Y6_GPIO_NUM,.pin_d3=Y5_GPIO_NUM,.pin_d2=Y4_GPIO_NUM,
  .pin_d1=Y3_GPIO_NUM,.pin_d0=Y2_GPIO_NUM,
  .pin_vsync=VSYNC_GPIO_NUM,.pin_href=HREF_GPIO_NUM,.pin_pclk=PCLK_GPIO_NUM,
  .xclk_freq_hz = 20000000,
  .ledc_timer   = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,
  .frame_size   = FRAMESIZE_QVGA,
  .jpeg_quality = 12,
  .fb_count     = 1,
  .fb_location  = CAMERA_FB_IN_PSRAM,
  .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

// ── Initialisation caméra ──────────────────────────────
bool ei_camera_init() {
  if (is_initialised) return true;
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) { Serial.printf("Camera init failed: 0x%x\n", err); return false; }
  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1); s->set_hmirror(s, 1); s->set_awb_gain(s, 1);
  is_initialised = true;
  return true;
}

// ── Callback données pour Edge Impulse ─────────────────
static int ei_camera_get_data(size_t offset, size_t length, float* out_ptr) {
  size_t px = offset * 3;
  for (size_t i = 0; i < length; i++, px += 3)
    out_ptr[i] = (snapshot_buf[px+2] << 16) | (snapshot_buf[px+1] << 8) | snapshot_buf[px];
  return 0;
}

// ── Inférence (appelée depuis loop) ────────────────────
void run_inference() {
  snapshot_buf = (uint8_t*)malloc(CAM_W * CAM_H * FRAME_BYTES);
  if (!snapshot_buf) return;

  // Capture
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { free(snapshot_buf); return; }
  bool ok = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
  esp_camera_fb_return(fb);
  if (!ok) { free(snapshot_buf); return; }

  // Resize si nécessaire
  if (EI_CLASSIFIER_INPUT_WIDTH != CAM_W || EI_CLASSIFIER_INPUT_HEIGHT != CAM_H) {
    ei::image::processing::crop_and_interpolate_rgb888(
      snapshot_buf, CAM_W, CAM_H,
      snapshot_buf, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
  }

  // Lancer le classifieur
  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

  if (err == EI_IMPULSE_OK) {
    fire_detected = false;
    fire_boxes_count = 0;

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    // Mode détection d'objets → bounding boxes
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
      auto& bb = result.bounding_boxes[i];
      if (bb.value >= FIRE_THRESHOLD && strcmp(bb.label, "fire") == 0) {
        fire_detected = true;
        fire_confidence = bb.value;
        if (fire_boxes_count < 10)
          fire_boxes[fire_boxes_count++] = bb;
      }
    }
#else
    // Mode classification simple
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      if (strcmp(ei_classifier_inferencing_categories[i], "fire") == 0 &&
          result.classification[i].value >= FIRE_THRESHOLD) {
        fire_detected = true;
        fire_confidence = result.classification[i].value;
      }
    }
#endif
  }
  free(snapshot_buf);
}

// ── Page HTML principale ────────────────────────────────
void handleRoot() {
  String html = R"rawhtml(
<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<title>FireBot Detection</title>
<style>
  body{background:#111;color:#fff;font-family:sans-serif;text-align:center;margin:0;padding:20px}
  #container{position:relative;display:inline-block}
  #stream{border:3px solid #333;border-radius:8px}
  #overlay{position:absolute;top:0;left:0;pointer-events:none}
  #status{margin-top:12px;padding:10px 20px;border-radius:8px;font-size:18px;font-weight:bold}
  .fire{background:#c0392b;animation:blink 0.5s infinite}
  .ok{background:#27ae60}
  @keyframes blink{0%,100%{opacity:1}50%{opacity:0.4}}
</style>
</head><body>
<h2>🔥 FireBot — Détection en temps réel</h2>
<div id="container">
  <img id="stream" src="/stream" width="320" height="240">
  <canvas id="overlay" width="320" height="240"></canvas>
</div>
<div id="status" class="ok">✅ Aucun feu détecté</div>
<script>
const canvas = document.getElementById('overlay');
const ctx = canvas.getContext('2d');
const statusDiv = document.getElementById('status');

function poll() {
  fetch('/status')
    .then(r => r.json())
    .then(data => {
      ctx.clearRect(0, 0, 320, 240);
      if (data.fire) {
        statusDiv.className = 'fire';
        statusDiv.textContent = '🔥 FEU DÉTECTÉ ! (' + Math.round(data.confidence*100) + '%)';
        // Dessine les bounding boxes
        if (data.boxes && data.boxes.length > 0) {
          data.boxes.forEach(bb => {
            ctx.strokeStyle = '#ff0000';
            ctx.lineWidth = 3;
            ctx.strokeRect(bb.x, bb.y, bb.w, bb.h);
            ctx.fillStyle = 'rgba(255,0,0,0.7)';
            ctx.fillRect(bb.x, bb.y - 22, 80, 22);
            ctx.fillStyle = '#fff';
            ctx.font = 'bold 13px sans-serif';
            ctx.fillText('fire ' + Math.round(bb.conf*100) + '%', bb.x + 4, bb.y - 6);
          });
        } else {
          // Cadre global si classification simple
          ctx.strokeStyle = '#ff0000';
          ctx.lineWidth = 4;
          ctx.strokeRect(5, 5, 310, 230);
          ctx.fillStyle = 'rgba(255,0,0,0.6)';
          ctx.fillRect(5, 5, 120, 28);
          ctx.fillStyle = '#fff';
          ctx.font = 'bold 15px sans-serif';
          ctx.fillText('FIRE ' + Math.round(data.confidence*100) + '%', 10, 24);
        }
      } else {
        statusDiv.className = 'ok';
        statusDiv.textContent = '✅ Aucun feu détecté';
      }
    })
    .catch(() => {});
  setTimeout(poll, 500);
}
poll();
</script>
</body></html>
)rawhtml";
  server.send(200, "text/html", html);
}

// ── Endpoint JSON /status ──────────────────────────────
void handleStatus() {
  String json = "{\"fire\":" + String(fire_detected ? "true" : "false");
  json += ",\"confidence\":" + String(fire_confidence, 3);
  json += ",\"boxes\":[";
  for (uint32_t i = 0; i < fire_boxes_count; i++) {
    if (i > 0) json += ",";
    // Recalcule les coordonnées en pixels 320x240
    float scaleX = 320.0f / EI_CLASSIFIER_INPUT_WIDTH;
    float scaleY = 240.0f / EI_CLASSIFIER_INPUT_HEIGHT;
    json += "{\"x\":" + String((int)(fire_boxes[i].x * scaleX));
    json += ",\"y\":" + String((int)(fire_boxes[i].y * scaleY));
    json += ",\"w\":" + String((int)(fire_boxes[i].width * scaleX));
    json += ",\"h\":" + String((int)(fire_boxes[i].height * scaleY));
    json += ",\"conf\":" + String(fire_boxes[i].value, 3) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// ── Stream MJPEG ───────────────────────────────────────
void handleStream() {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) continue;
    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    esp_camera_fb_return(fb);
    delay(50); // ~20 FPS
  }
}

// ── Setup ──────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  if (!ei_camera_init()) { Serial.println("Camera FAILED"); while(1); }

  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnecté ! IP: " + WiFi.localIP().toString());

  server.on("/",       handleRoot);
  server.on("/status", handleStatus);
  server.on("/stream", handleStream);
  server.begin();
  Serial.println("Serveur démarré → http://" + WiFi.localIP().toString());
}

// ── Loop ───────────────────────────────────────────────
void loop() {
  server.handleClient();

  static unsigned long lastInference = 0;
  // Inférence toutes les 2 secondes (pour ne pas bloquer le stream)
  if (millis() - lastInference > 2000) {
    run_inference();
    lastInference = millis();
    if (fire_detected)
      Serial.printf("🔥 FEU ! Confiance: %.1f%%\n", fire_confidence * 100);
  }
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif