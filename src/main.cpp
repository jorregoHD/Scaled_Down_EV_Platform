#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "Spedometer.h"

namespace {

constexpr uint8_t RELAY_PRECHARGE_PIN = 10;
constexpr uint8_t RELAY_MAIN_NEGATIVE_PIN = 11;
constexpr uint8_t RELAY_MAIN_POSITIVE_PIN = 12;

constexpr uint8_t BUTTON_NEXT_PIN = 0;
constexpr uint8_t BUTTON_SELECT_PIN = 14;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;

constexpr uint16_t COLOR_BACKGROUND = 0x0000;
constexpr uint16_t COLOR_PANEL = 0x10A2;
constexpr uint16_t COLOR_PANEL_ALT = 0x2104;
constexpr uint16_t COLOR_TEXT = 0xFFFF;
constexpr uint16_t COLOR_MUTED = 0x7BEF;
constexpr uint16_t COLOR_ACCENT = 0x07FF;
constexpr uint16_t COLOR_GREEN = 0x07E0;
constexpr uint16_t COLOR_AMBER = 0xFCA0;
constexpr uint16_t COLOR_RED = 0xF800;
constexpr uint16_t COLOR_WHITE = 0xFFFF;
constexpr uint32_t SPEED_DISPLAY_INTERVAL_MS = 125;

Arduino_DataBus *displayBus = new Arduino_ESP32PAR8Q(
    7, 6, 8, 9,
    39, 40, 41, 42, 45, 46, 47, 48);
Arduino_GFX *display = new Arduino_ST7789(
    displayBus, 5, 0, true, 170, 320, 35, 0, 35, 0);

struct Button {
    uint8_t pin;
    bool stableState;
    bool lastReading;
    uint32_t lastChangeAt;
};

Button nextButton{BUTTON_NEXT_PIN, HIGH, HIGH, 0};
Button selectButton{BUTTON_SELECT_PIN, HIGH, HIGH, 0};

enum class ScreenId : uint8_t {
    Overview,
    Battery,
    Climate,
    Charging,
    Settings,
};

ScreenId currentScreen = ScreenId::Overview;
uint8_t selectedOption = 0;
uint32_t lastSpeedDisplayAt = 0;

bool wasPressed(Button &button, uint32_t now) {
    const bool reading = digitalRead(button.pin);
    if (reading != button.lastReading) {
        button.lastReading = reading;
        button.lastChangeAt = now;
    }

    if (reading != button.stableState &&
        now - button.lastChangeAt >= BUTTON_DEBOUNCE_MS) {
        button.stableState = reading;
        return button.stableState == LOW;
    }
    return false;
}

void drawGaugeBand(int x, int y, int width, int height, uint16_t color) {
    display->drawRoundRect(x, y, width, height, 10, color);
    display->fillRoundRect(x + 4, y + 4, width - 8, height - 8, 8, COLOR_PANEL);
}

void drawSpeedReadings() {
    display->fillRoundRect(15, 75, 290, 52, 12, COLOR_PANEL_ALT);

    display->setTextColor(COLOR_WHITE);
    display->setTextSize(2);
    display->setCursor(28, 86);
    display->print("SPEED");
    display->setTextSize(3);
    display->setCursor(145, 82);
    display->print(Spedometer::speedMph(), 1);
    display->setCursor(260, 90);
    display->print("MPH");

    display->setTextSize(1);
    display->setCursor(28, 112);
    display->print("WHEEL RPM");
    display->setCursor(180, 112);
    display->print(static_cast<uint32_t>(Spedometer::wheelRpm() + 0.5F));
}

void drawHeader(const char *title, const char *subtitle) {
    display->fillRoundRect(10, 10, 300, 34, 10, COLOR_PANEL);
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(2);
    display->setCursor(20, 18);
    display->print(title);
    display->setTextColor(COLOR_MUTED);
    display->setTextSize(1);
    display->setCursor(220, 22);
    display->print(subtitle);
}

void drawMetricTile(int x, int y, int w, int h, const char *label,
                    const char *value, uint16_t accent) {
    display->fillRoundRect(x, y, w, h, 10, COLOR_PANEL);
    display->drawRoundRect(x, y, w, h, 10, accent);
    display->setTextColor(COLOR_MUTED);
    display->setTextSize(1);
    display->setCursor(x + 10, y + 10);
    display->print(label);
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(2);
    display->setCursor(x + 10, y + 22);
    display->print(value);
}

void drawMenuList(const char *items[], uint8_t count, uint8_t selected, int startY) {
    for (uint8_t i = 0; i < count; ++i) {
        const int y = startY + i * 28;
        const bool active = i == selected;

        display->fillRoundRect(18, y, 284, 22, 8, active ? COLOR_ACCENT : COLOR_PANEL);
        display->setTextColor(active ? COLOR_BACKGROUND : COLOR_TEXT);
        display->setTextSize(1);
        display->setCursor(28, y + 7);
        display->print(items[i]);
    }
}

void drawOverviewScreen() {
    drawHeader("EV OVERVIEW", "READY");
    drawGaugeBand(22, 52, 276, 90, COLOR_ACCENT);

    display->setTextColor(COLOR_WHITE);
    display->setTextSize(3);
    display->setCursor(48, 78);
    display->print("231");
    display->setTextSize(2);
    display->setCursor(125, 92);
    display->print("mi range");

    display->setTextColor(COLOR_MUTED);
    display->setTextSize(1);
    display->setCursor(50, 120);
    display->print("Battery 89%  |  406 V  |  125 kW");

    drawMetricTile(20, 150, 88, 58, "SOC", "89%", COLOR_GREEN);
    drawMetricTile(116, 150, 88, 58, "TEMP", "20C", COLOR_AMBER);
    drawMetricTile(212, 150, 88, 58, "MPH", "68", COLOR_ACCENT);

    const char *options[] = {"Drive", "Climate", "Charging", "Lights", "Audio"};
    drawMenuList(options, 5, selectedOption, 220);
}

void drawBatteryScreen() {
    drawHeader("BATTERY", "SYSTEM");
    drawMetricTile(20, 60, 130, 70, "PACK", "406V", COLOR_ACCENT);
    drawMetricTile(170, 60, 130, 70, "STATE", "89%", COLOR_GREEN);
    drawMetricTile(20, 145, 130, 70, "TEMP", "20C", COLOR_AMBER);
    drawMetricTile(170, 145, 130, 70, "CHG", "12kW", COLOR_RED);

    const char *options[] = {"Cell Balancing", "Preconditioning", "Battery Health", "Range Estimate"};
    drawMenuList(options, 4, selectedOption, 230);
}

void drawClimateScreen() {
    drawHeader("CLIMATE", "CABIN");
    drawMetricTile(20, 60, 140, 74, "IN CABIN", "72F", COLOR_GREEN);
    drawMetricTile(170, 60, 130, 74, "OUTSIDE", "58F", COLOR_ACCENT);
    drawMetricTile(20, 150, 140, 74, "A/C", "ON", COLOR_AMBER);
    drawMetricTile(170, 150, 130, 74, "HEAT", "OFF", COLOR_RED);

    const char *options[] = {"Auto Climate", "Defrost", "Seat Heat", "Fan Boost"};
    drawMenuList(options, 4, selectedOption, 235);
}

void drawChargingScreen() {
    drawHeader("CHARGING", "PORT");
    drawGaugeBand(40, 58, 240, 92, COLOR_GREEN);
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(3);
    display->setCursor(88, 85);
    display->print("12kW");
    display->setTextColor(COLOR_MUTED);
    display->setTextSize(1);
    display->setCursor(90, 118);
    display->print("Estimated 43 min remaining");

    drawMetricTile(20, 165, 90, 54, "SOC", "89%", COLOR_GREEN);
    drawMetricTile(120, 165, 90, 54, "RATE", "12kW", COLOR_ACCENT);
    drawMetricTile(220, 165, 80, 54, "PORT", "LOCKED", COLOR_AMBER);

    const char *options[] = {"Start Charge", "Scheduled Charge", "Range Mode", "Cost Saver"};
    drawMenuList(options, 4, selectedOption, 225);
}

void drawSettingsScreen() {
    drawHeader("SETTINGS", "DISPLAY");
    drawMetricTile(20, 60, 130, 64, "THEME", "DARK", COLOR_ACCENT);
    drawMetricTile(170, 60, 130, 64, "BRIGHT", "75%", COLOR_AMBER);
    drawMetricTile(20, 140, 130, 64, "SOUND", "ON", COLOR_GREEN);
    drawMetricTile(170, 140, 130, 64, "VOICE", "READY", COLOR_RED);

    const char *options[] = {"Driver Profile", "Vehicle Controls", "Software", "Reset Display"};
    drawMenuList(options, 4, selectedOption, 215);
}

void drawCurrentScreen() {
    display->fillScreen(COLOR_BACKGROUND);
    display->setTextWrap(false);

    switch (currentScreen) {
        case ScreenId::Overview:
            drawOverviewScreen();
            break;
        case ScreenId::Battery:
            drawBatteryScreen();
            break;
        case ScreenId::Climate:
            drawClimateScreen();
            break;
        case ScreenId::Charging:
            drawChargingScreen();
            break;
        case ScreenId::Settings:
            drawSettingsScreen();
            break;
        default:
            drawOverviewScreen();
            break;
    }

    display->fillRoundRect(8, 280, 304, 30, 10, COLOR_PANEL);
    display->setTextColor(COLOR_TEXT);
    display->setTextSize(1);
    display->setCursor(18, 289);
    display->print("BOOT:SCREEN  GPIO14:OPTION");
}

void setRelayOutputs() {
    // Keep the hardware relay outputs available for the bench trainer,
    // while the display itself is driven by the Tesla-inspired menu system.
    digitalWrite(RELAY_PRECHARGE_PIN, LOW);
    digitalWrite(RELAY_MAIN_NEGATIVE_PIN, LOW);
    digitalWrite(RELAY_MAIN_POSITIVE_PIN, LOW);
}

void initializeRelayOutput(uint8_t pin) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
}

}  // namespace

void setup() {
    initializeRelayOutput(RELAY_PRECHARGE_PIN);
    initializeRelayOutput(RELAY_MAIN_NEGATIVE_PIN);
    initializeRelayOutput(RELAY_MAIN_POSITIVE_PIN);
    setRelayOutputs();

    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);

    Spedometer::begin();

    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    pinMode(38, OUTPUT);
    digitalWrite(38, HIGH);

    Serial.begin(115200);
    display->begin();
    display->setRotation(1);
    drawCurrentScreen();
    lastSpeedDisplayAt = millis();
}

void loop() {
    const uint32_t now = millis();

    Spedometer::update();

    if (wasPressed(nextButton, now)) {
        currentScreen = static_cast<ScreenId>((static_cast<uint8_t>(currentScreen) + 1u) % 5u);
        selectedOption = 0;
        drawCurrentScreen();
        Serial.printf("Screen: %u\n", static_cast<uint8_t>(currentScreen));
    } else if (wasPressed(selectButton, now)) {
        const uint8_t optionCount = currentScreen == ScreenId::Overview ? 5u
            : currentScreen == ScreenId::Battery ? 4u
            : currentScreen == ScreenId::Climate ? 4u
            : currentScreen == ScreenId::Charging ? 4u
            : 4u;

        selectedOption = (selectedOption + 1u) % optionCount;
        drawCurrentScreen();
        Serial.printf("Selected option: %u on screen %u\n",
                      selectedOption, static_cast<uint8_t>(currentScreen));
    }

    if (now - lastSpeedDisplayAt >= SPEED_DISPLAY_INTERVAL_MS) {
        drawSpeedReadings();
        lastSpeedDisplayAt = now;
    }

    delay(5);
}
