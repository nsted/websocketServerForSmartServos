/*
 *
 *  Relay data between a smart servo such as a Dynamixel and a websocket client.
 *  Tested with an XL430-w250.
 *
 *  You'll need a circuit that corresponds to the half-duplex UART configuration
 *  used by these servos. Here's an example:
 *  https://emanual.robotis.com/docs/en/dxl/x/xl430-w250/#communication-circuit
 *
 *  Written Nov. 2024
 *  by Nicholas Stedman, nick@devicist.com
 *
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "yourSSID";
const char* password = "yourPassword";

#define DEBUG  // comment out to turn off logging

#define USE_SERIAL Serial
#define TX_EN D8
#define WAIT_FOR_FLUSH_TO_COMPLETE 40

HardwareSerial SERVO_BUS(0);
WebSocketsServer webSocket = WebSocketsServer(80);

// Process sensors command
void handleSensorsCommand(uint8_t num, uint8_t* payload, size_t length) {
#ifdef DEBUG
  USE_SERIAL.printf("[%u] Handling sensors command: %s\n", num, payload);
#endif
  // Insert sensor-handling code here
}

// Process configuration commands (e.g., setting parameters, control modes, etc.)
void handleConfigurationCommand(uint8_t num, uint8_t* payload, size_t length) {
#ifdef DEBUG
  USE_SERIAL.printf("[%u] Handling configuration command: %s\n", num, payload);
#endif
  // Insert configuration-related code here
}

void relayToServo(const void* mem, uint32_t len) {
  const uint8_t* src = (const uint8_t*)mem;
  digitalWrite(TX_EN, HIGH);
  SERVO_BUS.write(src, len);
  SERVO_BUS.flush();
  delayMicroseconds(WAIT_FOR_FLUSH_TO_COMPLETE);
  digitalWrite(TX_EN, LOW);
}

void hexdump(const void* mem, uint32_t len, uint8_t cols = 16) {
#ifdef DEBUG
  const uint8_t* src = (const uint8_t*)mem;
  USE_SERIAL.printf("[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    USE_SERIAL.printf("%02X ", *src);
    src++;
  }
  USE_SERIAL.printf("\n");
#endif
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
#ifdef DEBUG
      USE_SERIAL.printf("[%u] Disconnected!\n", num);
#endif
      if (num == 0) {
        digitalWrite(LED_BUILTIN, LOW);
      }
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
#ifdef DEBUG
        USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
#endif
        if (num == 0) {
          digitalWrite(LED_BUILTIN, HIGH);
        }
      }
      break;
    case WStype_TEXT:
#ifdef DEBUG
      USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
#endif
      // filtering based on message content
      if (strstr((char*)payload, "sensors") != nullptr) {
        // If the message contains the word "sensors", call the sensors command handler
        handleSensorsCommand(num, payload, length);
      } else if (strstr((char*)payload, "config") != nullptr) {
        // If the message contains the word "config", call the configuration handler
        handleConfigurationCommand(num, payload, length);
      } else {
#ifdef DEBUG
        USE_SERIAL.printf("[%u] Unknown command type: %s\n", num, payload);
#endif
      }
      break;
    case WStype_BIN:
      relayToServo(payload, length);
#ifdef DEBUG
      USE_SERIAL.println();
      USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
#endif
      break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void setup() {
  int8_t i;
  pinMode(TX_EN, OUTPUT);
#ifdef DEBUG
  USE_SERIAL.begin(1000000);
#endif
  SERVO_BUS.begin(1000000);
  delay(1000);

#ifdef DEBUG
  USE_SERIAL.print("\nAttaching to WiFi '" + String(ssid) + String("' ..."));
  USE_SERIAL.setDebugOutput(true);
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  WiFi.begin(ssid, password);
#ifdef DEBUG
  USE_SERIAL.print(" attached to WiFi.\nConnecting to network ... ");
#endif
  for (i = 60; i != 0; i--) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(333);
  }
  if (i == 0) {
#ifdef DEBUG
    USE_SERIAL.println("Network connection failed!\nRestarting ESP32!");
#endif
    delay(1000);
    ESP.restart();
  }
#ifdef DEBUG
  USE_SERIAL.print(" network connected.\nLocal IP address: ");
  USE_SERIAL.println(WiFi.localIP());
  USE_SERIAL.print("Ready! Use port 80 to connect.");
#endif

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  relayFromServo();
}

void relayFromServo() {
  while (SERVO_BUS.available()) {
    long len = SERVO_BUS.available();
    uint8_t buffer[len];
    SERVO_BUS.readBytes(buffer, len);
#ifdef DEBUG
    USE_SERIAL.println();
    USE_SERIAL.println("reply: ");
    hexdump(buffer, len);
#endif
    webSocket.sendBIN(0, buffer, len);
  }
}
