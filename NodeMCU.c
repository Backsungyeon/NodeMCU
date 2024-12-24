#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <TinyGPS++.h> // GPS 라이브러리
#include <SoftwareSerial.h> // SoftwareSerial 사용

// Wi-Fi 정보
const char* ssid = "DIT_FREE_WiFi";
const char* password = "";

// 텔레그램 봇 정보
const char* botToken = "7797912634:AAE585OVN4L7H81pg5xfd_kN7W0QzTiZaz8";
const char* chatID = "7467347090";  // 텔레그램 채팅 ID

// 핀 정의
const int firePin = D1;
const int piezo = D2;
const int led = D4;
const int gpsRxPin = D3; // GPS RX 핀
const int gpsTxPin = D5; // GPS TX 핀 (필요 시 설정)

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);
TinyGPSPlus gps;

// SoftwareSerial을 GPS 통신에 사용
SoftwareSerial gpsSerial(gpsRxPin, gpsTxPin);

int ledState = 0;
int piezoTone = 1000;
int blinkCount = -1;
bool alertSent = false;
unsigned long lastAlertTime = 0;
const unsigned long alertInterval = 10000; // 최소 메시지 간격 (10초)

void setup() {
    pinMode(firePin, INPUT);
    pinMode(piezo, OUTPUT);
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
    noTone(piezo);

    Serial.begin(115200);
    gpsSerial.begin(9600); // GPS 모듈과 통신 시작
    Serial.println("GPS 초기화 완료");

    // Wi-Fi 연결
    connectToWiFi();

    // Telegram TLS 설정
    client.setInsecure(); // 인증서 검증 비활성화

    // GPS 연결 완료 확인
    waitForGPS(); // GPS 데이터가 유효해질 때까지 대기
}

void loop() {
    int fireState = digitalRead(firePin);

    // GPS 데이터 처리
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    if (fireState == LOW) { // 화재 감지
        if (!alertSent && millis() - lastAlertTime > alertInterval) {
            sendTelegramAlert(); // 텔레그램 메시지 전송
            alertSent = true;
        }

        // LED 및 피에조 작동
        if (blinkCount >= 2) {
            piezoTone = (piezoTone == 1000) ? 1500 : 1000;
            blinkCount = 0;
        } else {
            blinkCount++;
        }

        tone(piezo, piezoTone, 400);

        if (ledState == 0) {
            digitalWrite(led, HIGH);
            ledState = 1;
        } else {
            digitalWrite(led, LOW);
            ledState = 0;
        }

        Serial.println("Fire detected!");
    } else { // 화재 감지 없음
        noTone(piezo);
        digitalWrite(led, LOW);
        ledState = 0;
        blinkCount = -1;
        piezoTone = 1000;
        alertSent = false;

        Serial.println("No fire detected.");
    }

    delay(200);
}

// Wi-Fi 연결 함수
void connectToWiFi() {
    Serial.print("Wi-Fi 연결 중...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi 연결 성공!");
    Serial.print("IP 주소: ");
    Serial.println(WiFi.localIP());
}

// GPS 연결 대기 함수
void waitForGPS() {
    Serial.print("GPS 연결 대기 중...");
    while (true) {
        while (gpsSerial.available() > 0) {
            gps.encode(gpsSerial.read());
        }
        if (gps.location.isValid()) { // GPS 데이터가 유효한지 확인
            Serial.println("\nGPS 연결 완료!");
            Serial.print("위도: ");
            Serial.println(gps.location.lat());
            Serial.print("경도: ");
            Serial.println(gps.location.lng());
            break; // 유효한 GPS 데이터를 얻으면 루프 탈출
        }
        delay(500);
        Serial.print(".");
    }
}

// 텔레그램 메시지 전송 함수
void sendTelegramAlert() {
    String message = "🔥 화재 발생 알림! 🔥\n";

    // GPS 데이터를 읽어 위치 추가
    if (gps.location.isValid()) {
        message += "위도: ";
        message += gps.location.lat();
        message += "\n경도: ";
        message += gps.location.lng();
    } else {
        message += "GPS 데이터를 가져올 수 없습니다.\n";
    }

    Serial.println("텔레그램 메시지 전송 시도...");
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi 연결이 끊어졌습니다. 다시 연결을 시도합니다...");
        connectToWiFi();
    }

    bool success = bot.sendMessage(chatID, message, "Markdown");
    if (success) {
        Serial.println("텔레그램 메시지가 성공적으로 전송되었습니다!");
    } else {
        Serial.println("텔레그램 메시지 전송 실패. 네트워크 상태를 확인하세요.");
    }

    lastAlertTime = millis(); // 마지막 메시지 전송 시간 기록
}
