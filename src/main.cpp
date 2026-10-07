#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include <EEPROM.h>

// ==================== CONFIGURACIÓN ====================
const char* NTP_SERVER = "pool.ntp.org";

#define DEFAULT_GMT_OFFSET_SEC (-5 * 3600)
#define DEFAULT_TACTNTP_MIN  30.0f
#define DEFAULT_TXCLK_MIN    1.0f

#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0x4E545031

// ==================== ESTRUCTURA EEPROM ====================
struct EepromData {
    uint32_t magic;
    long     gmtOffsetSec;
    float    tactntpMin;
    float    txclkMin;
    time_t   lastSync;
};

// ==================== WiFiManager ====================
WiFiManager wm;
WiFiManagerParameter param_gmt("gmt", "GMT Offset (horas, ej: -5)", "-5", 8);
WiFiManagerParameter param_tactntp("tactntp", "Intervalo NTP (min, ej: 30)", "30", 8);
WiFiManagerParameter param_txclk("txclk", "Intervalo TX serial (min, ej: 1)", "1", 8);

// ==================== ESTADO ====================
long    gmtOffsetSec     = DEFAULT_GMT_OFFSET_SEC;
float   tactntpMin       = DEFAULT_TACTNTP_MIN;
float   txclkMin         = DEFAULT_TXCLK_MIN;
time_t  lastNtpSync      = 0;
bool    ntpSynced        = false;
uint32_t lastNtpAttemptMs = 0;
uint32_t lastTxMs        = 0;

// ==================== EEPROM ====================
void loadSettings() {
    EEPROM.begin(EEPROM_SIZE);
    EepromData data;
    EEPROM.get(0, data);

    if (data.magic == EEPROM_MAGIC) {
        gmtOffsetSec = data.gmtOffsetSec;
        tactntpMin   = data.tactntpMin;
        txclkMin     = data.txclkMin;
        lastNtpSync  = data.lastSync;
    }

    if (tactntpMin < 0.1f) tactntpMin = DEFAULT_TACTNTP_MIN;
    if (txclkMin < 0.1f)   txclkMin   = DEFAULT_TXCLK_MIN;
}

void saveSettings() {
    EepromData data;
    data.magic       = EEPROM_MAGIC;
    data.gmtOffsetSec = gmtOffsetSec;
    data.tactntpMin  = tactntpMin;
    data.txclkMin    = txclkMin;
    data.lastSync    = lastNtpSync;

    EEPROM.put(0, data);
    EEPROM.commit();
}

// ==================== NTP ====================
bool syncNTP() {
    configTime(gmtOffsetSec, 0, NTP_SERVER);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
        lastNtpSync = time(nullptr);
        ntpSynced = true;
        Serial1.printf("NTP OK: %02d:%02d:%02d\n",
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        saveSettings();
        return true;
    }
    return false;
}

// ==================== TX SERIAL ====================
void sendClock() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
        time_t now = time(nullptr);
        if (now > 1609459200) {
            Serial.printf("$SETCLK,%02d%02d%02d\r\n",
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(9600);
    Serial1.begin(115200);
    delay(100);
    Serial1.println("\n=== ESP01NTP ===");

    loadSettings();

    if (lastNtpSync > 1609459200) {
        struct timeval tv = { .tv_sec = lastNtpSync };
        settimeofday(&tv, NULL);
        Serial1.println("Hora restaurada desde memoria");
    }

    wm.setDebugOutput(false);
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(180);

    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", gmtOffsetSec / 3600);
    param_gmt.setValue(buf, 8);
    snprintf(buf, sizeof(buf), "%.1f", tactntpMin);
    param_tactntp.setValue(buf, 8);
    snprintf(buf, sizeof(buf), "%.1f", txclkMin);
    param_txclk.setValue(buf, 8);

    wm.addParameter(&param_gmt);
    wm.addParameter(&param_tactntp);
    wm.addParameter(&param_txclk);

    wm.setAPCallback([](WiFiManager *myWM) {
        Serial1.println("AP: conectate y abre 192.168.4.1");
    });

    bool res = wm.autoConnect("ESP01NTP");
    if (res) {
        Serial1.print("WiFi conectado. IP: ");
        Serial1.println(WiFi.localIP());

        long newGmt = atol(param_gmt.getValue()) * 3600;
        float newTactntp = atof(param_tactntp.getValue());
        float newTxclk   = atof(param_txclk.getValue());

        if (newTactntp < 0.1f) newTactntp = DEFAULT_TACTNTP_MIN;
        if (newTxclk < 0.1f)   newTxclk   = DEFAULT_TXCLK_MIN;

        gmtOffsetSec = newGmt;
        tactntpMin   = newTactntp;
        txclkMin     = newTxclk;

        saveSettings();
        syncNTP();
    } else {
        Serial1.println("WiFi no configurado. Portal AP activo.");
    }

    Serial1.println("Setup completo.");
}

// ==================== LOOP ====================
void loop() {
    static bool wifiPrev = false;

    bool wifiNow = (WiFi.status() == WL_CONNECTED);

    if (!wifiNow) {
        wm.process();
        wifiPrev = false;
        return;
    }

    wm.process();

    if (!wifiPrev) {
        Serial1.println("WiFi reconectado.");
        syncNTP();
    }
    wifiPrev = wifiNow;

    uint32_t now = millis();

    uint32_t ntpIntervalMs = (uint32_t)(tactntpMin * 60000.0f);
    if (wifiNow && (!ntpSynced || (now - lastNtpAttemptMs > ntpIntervalMs))) {
        lastNtpAttemptMs = now;
        syncNTP();
    }

    uint32_t txIntervalMs = (uint32_t)(txclkMin * 60000.0f);
    if (wifiNow && (now - lastTxMs > txIntervalMs)) {
        lastTxMs = now;
        sendClock();
    }
}
