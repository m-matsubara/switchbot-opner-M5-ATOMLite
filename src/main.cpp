/*
  Switch Bot SmartLock Opener (M5 ATOM Lite)

  2026/05/20 m.matsubara
  Code authored with assistance from Claude (Anthropic).
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <M5Atom.h>
#include <time.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>

#include "const.hpp"

// Use true for a quick first test. Replace with setCACert for better security.
static const bool USE_INSECURE_TLS = true;

// M5 ATOM Lite button (GPIO39)
static const int PIN_BTN = 39;
static const bool BUTTON_ACTIVE_LOW = true;

// Behavior
static const uint32_t LONG_PRESS_MS = 2000;
static const uint32_t COOLDOWN_MS = 5000;

// LED colors (GRB order for M5Atom)
static const uint32_t COLOR_OFF      = 0x000000;
static const uint32_t COLOR_GREEN    = 0x00FF00;  // Lock success blink
static const uint32_t COLOR_RED      = 0xFF0000;  // Unlock success blink
static const uint32_t COLOR_BLUE     = 0x0000FF;  // WiFi connecting
static const uint32_t COLOR_YELLOW   = 0xFFFF00;  // API error
static const uint32_t COLOR_PURPLE   = 0xFF00FF;  // Sending command

static const int BLINK_COUNT = 3;
static const uint32_t BLINK_ON_MS = 200;
static const uint32_t BLINK_OFF_MS = 200;

// ======= State =======
static uint32_t last_action_ms = 0;
static uint32_t btn_down_ms = 0;
static bool btn_was_pressed = false;

static uint32_t error_until_ms = 0;
static bool has_valid_time = false;

static void setLed(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  M5.dis.drawpix(0, CRGB(r, g, b));
}

static void blinkLed(uint32_t color, int count) {
  for (int i = 0; i < count; ++i) {
    setLed(color);
    delay(BLINK_ON_MS);
    setLed(COLOR_OFF);
    if (i < count - 1) delay(BLINK_OFF_MS);
  }
}

static bool waitForTimeSync() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = 0;
  for (int i = 0; i < 30; ++i) {
    time(&now);
    if (now > 1600000000) {
      has_valid_time = true;
      return true;
    }
    delay(500);
  }
  return false;
}

static String makeUuidV4() {
  uint8_t b[16];
  esp_fill_random(b, sizeof(b));
  b[6] = (b[6] & 0x0F) | 0x40;
  b[8] = (b[8] & 0x3F) | 0x80;
  char out[37];
  snprintf(out, sizeof(out),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
           b[12], b[13], b[14], b[15]);
  return String(out);
}

static String hmacSha256Base64Upper(const String& payload, const char* secret) {
  uint8_t hmac[32];
  size_t olen = 0;

  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, reinterpret_cast<const unsigned char*>(secret), strlen(secret));
  mbedtls_md_hmac_update(&ctx, reinterpret_cast<const unsigned char*>(payload.c_str()), payload.length());
  mbedtls_md_hmac_finish(&ctx, hmac);
  mbedtls_md_free(&ctx);

  unsigned char b64[64] = {0};
  mbedtls_base64_encode(b64, sizeof(b64), &olen, hmac, sizeof(hmac));

  String out = String(reinterpret_cast<char*>(b64));
  out.toUpperCase();
  return out;
}

static bool sendCommand(const char* command) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return false;
  }

  time_t now = time(nullptr);
  if (now < 1600000000) {
    Serial.println("Time not synced.");
    return false;
  }

  String nonce = makeUuidV4();
  uint64_t t_ms = static_cast<uint64_t>(now) * 1000ULL;
  String t = String(t_ms);
  String signPayload = String(SWITCHBOT_TOKEN) + t + nonce;
  String sign = hmacSha256Base64Upper(signPayload, SWITCHBOT_SECRET);

  String url = String("https://api.switch-bot.com/v1.1/devices/") +
               SWITCHBOT_DEVICE_ID + "/commands";

  WiFiClientSecure client;
  if (USE_INSECURE_TLS) {
    client.setInsecure();
  } else {
    // TODO: Replace with current CA cert for api.switch-bot.com
    // client.setCACert(your_root_ca_pem);
  }

  HTTPClient https;
  if (!https.begin(client, url)) {
    Serial.println("HTTPS begin failed.");
    return false;
  }

  https.addHeader("Content-Type", "application/json; charset=utf8");
  https.addHeader("Authorization", SWITCHBOT_TOKEN);
  https.addHeader("sign", sign);
  https.addHeader("t", t);
  https.addHeader("nonce", nonce);

  String body = String("{\"command\":\"") + command +
                "\",\"parameter\":\"default\",\"commandType\":\"command\"}";

  int httpCode = https.POST(body);
  String resp = https.getString();
  https.end();

  Serial.printf("HTTP %d, resp: %s\n", httpCode, resp.c_str());
  return httpCode == 200;
}

static bool isPressed() {
  int v = digitalRead(PIN_BTN);
  return BUTTON_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  M5.begin(true, false, true); // Serial, I2C, LED
  setLed(COLOR_BLUE);

  pinMode(PIN_BTN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  if (waitForTimeSync()) {
    Serial.println("Time sync done.");
  } else {
    Serial.println("Time sync pending.");
  }

  setLed(COLOR_OFF);
}

void loop() {
  uint32_t now = millis();

  if (error_until_ms && now >= error_until_ms) {
    error_until_ms = 0;
    setLed(COLOR_OFF);
  }

  bool pressed = isPressed();

  if (pressed && !btn_was_pressed) {
    btn_down_ms = now;
  }

  if (!pressed && btn_was_pressed) {
    uint32_t held = now - btn_down_ms;

    if (held >= LONG_PRESS_MS) {
      // Long press: Lock
      if (now - last_action_ms >= COOLDOWN_MS) {
        Serial.println("Lock command");
        setLed(COLOR_PURPLE);
        if (sendCommand("lock")) {
          blinkLed(COLOR_GREEN, BLINK_COUNT);
          last_action_ms = millis();
        } else {
          error_until_ms = millis() + 3000;
          setLed(COLOR_YELLOW);
        }
      }
    } else {
      // Short press: Unlock
      if (now - last_action_ms >= COOLDOWN_MS) {
        Serial.println("Unlock command");
        setLed(COLOR_PURPLE);
        if (sendCommand("unlock")) {
          blinkLed(COLOR_RED, BLINK_COUNT);
          last_action_ms = millis();
        } else {
          error_until_ms = millis() + 3000;
          setLed(COLOR_YELLOW);
        }
      }
    }
    btn_down_ms = 0;
  }

  btn_was_pressed = pressed;
  delay(10);
}
