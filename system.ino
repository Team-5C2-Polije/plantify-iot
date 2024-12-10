#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// Pin dan Konfigurasi
#define DHTPIN 26            // Pin DHT22
#define LDRPIN 32            // Pin LDR
#define MOISTUREPIN 34       // Pin sensor kelembapan tanah
#define TRIGPIN 14           // Pin trigger Ultrasonic
#define ECHOPIN 13           // Pin echo Ultrasonic
#define RelayPIN 27          // Pin relay untuk pompa air

#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Konfigurasi Wi-Fi
const char* ssid = "Darkweb";     // Ganti dengan SSID Wi-Fi Anda
const char* password = "qwertyuiopp";          // Ganti dengan password Wi-Fi Anda
const char* serverUrl = "http://192.168.110.153:5000/device/update_sensors";
const char* deviceToken = "YtfPhcCWf3gYiHU61yfS";

// Ambang batas kelembapan tanah
const int soilMoistureThreshold = 3000; // Ambang batas kelembapan (lebih tinggi = tanah lebih kering)

// Variabel untuk kontrol pompa
bool isPumpOn = false;
unsigned long pumpStartTime = 0; // Menyimpan waktu saat pompa dinyalakan
const unsigned long pumpOnDuration = 60000; // Durasi pompa menyala (1 menit)

// Fungsi untuk mengontrol pompa air
void turnPumpOn() {
  if (!isPumpOn) {
    digitalWrite(RelayPIN, HIGH); // Hidupkan relay
    isPumpOn = true;
    pumpStartTime = millis(); // Catat waktu saat pompa dihidupkan
    Serial.println("Pompa air dihidupkan.");
  }
}

void turnPumpOff() {
  if (isPumpOn) {
    digitalWrite(RelayPIN, LOW); // Matikan relay
    isPumpOn = false;
    Serial.println("Pompa air dimatikan.");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Konfigurasi pin
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(LDRPIN, INPUT);
  pinMode(MOISTUREPIN, INPUT);
  pinMode(RelayPIN, OUTPUT);
  digitalWrite(RelayPIN, LOW);  // Matikan relay saat awal

  // Koneksi Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nTerhubung ke WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Membaca sensor kelembapan tanah
  int moistureValue = analogRead(MOISTUREPIN);
  Serial.print("Nilai Kelembapan Tanah: ");
  Serial.println(moistureValue);

  // Logika penyiraman berdasarkan kelembapan tanah
  if (moistureValue >= soilMoistureThreshold) { // Jika kelembapan tanah tinggi (tanah kering)
    Serial.println("Tanah kering, menghidupkan pompa air...");
    turnPumpOn(); // Hidupkan pompa air
  } else { // Jika kelembapan tanah rendah (tanah basah)
    Serial.println("Tanah basah, mematikan pompa air...");
    turnPumpOff(); // Matikan pompa air
  }

  // Matikan pompa setelah 1 menit menyala
  if (isPumpOn && (millis() - pumpStartTime >= pumpOnDuration)) {
    Serial.println("Pompa air telah menyala selama 1 menit, mematikannya...");
    turnPumpOff(); // Matikan pompa air
  }

  // Membaca sensor DHT22
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembaban)) {
    Serial.println("Gagal membaca sensor DHT22!");
  } else {
    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.print(" °C, Kelembaban: ");
    Serial.print(kelembaban);
    Serial.println(" %");
  }

  // Membaca sensor LDR
  int ldrValue = analogRead(LDRPIN);
  Serial.print("Nilai LDR: ");
  Serial.println(ldrValue);

  // Membaca jarak menggunakan sensor Ultrasonic
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  long duration = pulseIn(ECHOPIN, HIGH);
  float tinggi = 19.0;
  float jarak = (duration / 2) * 0.0344;
  float cVolAir = (jarak / tinggi) * 100;
  float volAir = 100 - cVolAir;
  if (volAir < 0) {
    volAir = 0;
  }

  Serial.print("Tinggi Galon : ");
  Serial.print(tinggi);
  Serial.println(" cm");
  Serial.println("----");
  Serial.print("Jarak Air : ");
  Serial.print(jarak);
  Serial.println(" cm");
  Serial.println("----");
  Serial.print("Volume Air (Estimasi): ");
  Serial.print(volAir);
  Serial.println(" %");
  Serial.println("----");

  // Kirim data ke server menggunakan HTTP POST
  String payload = "{\"token\": \"" + String(deviceToken) + "\", \"lightIntensity\": " + String(ldrValue) + ", \"soilMoisture\": " + String(moistureValue) + ", \"temperature\": " + String(suhu) + ", \"humidity\": " + String(kelembaban) + ", \"waterVol\": " + String(volAir) + "}";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("POST request berhasil. Response Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Gagal mengirim POST request. Response Code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi tidak terhubung");
  }

  delay(15000);  // Tunggu sebelum loop berikutnya
}