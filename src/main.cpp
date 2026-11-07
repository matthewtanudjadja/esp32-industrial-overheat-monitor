#include "DHTesp.h"

// ==========================================
// CONFIGURATION & PIN MAPPING
// ==========================================
namespace Config {
    constexpr uint8_t DHT_PIN         = 15;
    constexpr uint8_t BUZZER_PIN      = 2;
    constexpr float   TEMP_THRESHOLD  = 40.0f; // Threshold alarm (°C)
    constexpr uint32_t POLL_INTERVAL  = 2000;  // Interval baca sensor (ms)
}

// ==========================================
// SYSTEM STATES & GLOBAL VARS
// ==========================================
enum class SystemState {
    INITIALIZING,
    NORMAL,
    ALARM_ACTIVE,
    SENSOR_ERROR
};

DHTesp dht;
SystemState currentState = SystemState::INITIALIZING;
uint32_t lastPollTime    = 0;

// ==========================================
// HELPER FUNCTIONS
// ==========================================
void setBuzzerState(bool active) {
    digitalWrite(Config::BUZZER_PIN, active ? HIGH : LOW);
}

void processSensorData() {
    TempAndHumidity data = dht.getTempAndHumidity();

    if (dht.getStatus() != 0) {
        if (currentState != SystemState::SENSOR_ERROR) {
            currentState = SystemState::SENSOR_ERROR;
            Serial.println(F("[ERROR] Failed to read from DHT22 sensor. Checking connection..."));
            setBuzzerState(false);
        }
        return;
    }

    Serial.printf("[TELEMETRY] Temp: %.1f°C | Humidity: %.1f%%\n", data.temperature, data.humidity);

    if (data.temperature > Config::TEMP_THRESHOLD) {
        if (currentState != SystemState::ALARM_ACTIVE) {
            currentState = SystemState::ALARM_ACTIVE;
            Serial.printf("[CRITICAL] Thermal anomaly! Temp (%.1f°C) exceeded limits (%.1f°C).\n", 
                          data.temperature, Config::TEMP_THRESHOLD);
            setBuzzerState(true);
        }
    } else {
        if (currentState != SystemState::NORMAL) {
            currentState = SystemState::NORMAL;
            Serial.println(F("[INFO] System operating within safe thermal parameters."));
            setBuzzerState(false);
        }
    }
}

// ==========================================
// CORE FIRMWARE LOOPS
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);

    pinMode(Config::BUZZER_PIN, OUTPUT);
    setBuzzerState(false);

    dht.setup(Config::DHT_PIN, DHTesp::DHT22);
    Serial.println(F("[SYSTEM] ESP32 Industrial Overheat Monitor Started."));
}

void loop() {
    uint32_t currentMillis = millis();

    if (currentMillis - lastPollTime >= Config::POLL_INTERVAL) {
        lastPollTime = currentMillis;
        processSensorData();
    }
}
