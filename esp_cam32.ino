#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "Arduino.h"

// Konfigurasi Wi-Fi
const char* ssid = "mahen kecil";
const char* password = "1231231234";

// URL API untuk mengirim gambar
const String serverUrl = "http://172.16.111.24:5000/device/add_photo_by_token";

// Kamera config
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM      5
#define Y2_GPIO_NUM      4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

camera_config_t config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA,  // Mengurangi resolusi untuk menghemat memori
    .jpeg_quality = 12,
    .fb_count = 2
};

void setup() {
    Serial.begin(115200);

    // Hubungkan ke Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");

    // Inisialisasi kamera
    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init failed");
        return;
    }
}


void loop() {
    Serial.println("Starting capture...");

    // Tangkap gambar
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb || fb->buf == nullptr) {
        Serial.println("Failed to capture image");
        delay(1000);
        return;
    }

    Serial.printf("Captured image: %d bytes\n", fb->len);

    // Persiapkan HTTP request
    HTTPClient http;
    http.begin(serverUrl);

    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    String contentType = "multipart/form-data; boundary=" + boundary;
    http.addHeader("Content-Type", contentType);

    // String formDataStart = "--" + boundary + "\r\n";
    // formDataStart += "Content-Disposition: form-data; name=\"token\"\r\n\r\n";
    // formDataStart += "yRv26pObT6gen4F2GFht\r\n";
    // formDataStart += "--" + boundary + "\r\n";
    // formDataStart += "Content-Disposition: form-data; name=\"photo\"; filename=\"image.jpg\"\r\n";
    // formDataStart += "Content-Type: image/jpeg\r\n\r\n";

    String formDataStart = "--" + boundary + "\r\n";
    formDataStart += "Content-Disposition: form-data; name=\"token\"\r\n\r\n";
    formDataStart += "YtfPhcCWf3gYiHU61yfS\r\n";
    formDataStart += "--" + boundary + "\r\n";
    formDataStart += "Content-Disposition: form-data; name=\"photo\"; filename=\"image.jpg\"\r\n";
    formDataStart += "Content-Type: image/jpeg\r\n\r\n";

    // Menambahkan data gambar dari buffer (fb->buf) ke form data
    String formDataImage = "";
    for (size_t i = 0; i < fb->len; i++) {
        formDataImage += (char)fb->buf[i];
    }

    formDataStart += formDataImage;  // Menambahkan data gambar ke form data

    String formDataEnd = "\r\n--" + boundary + "--\r\n";

    // Hitung total content length
    int contentLength = formDataStart.length() + fb->len + formDataEnd.length();
    http.addHeader("Content-Length", String(contentLength));

    // Kirim data menggunakan sendRequest()
    String body = formDataStart + String((char*)fb->buf) + formDataEnd;

    // Kirim POST request dengan body
    int httpResponseCode = http.sendRequest("POST", body);
    Serial.printf("Response: %d\n", httpResponseCode);

    if (httpResponseCode == 200) {
        Serial.println("Image uploaded successfully");
        Serial.println(http.getString());  // Menampilkan response dari server
    } else {
        // Jika server mengembalikan kode error, tampilkan pesan kesalahan lebih lengkap
        String response = http.getString();  // Ambil body dari response server
        Serial.printf("Error:  %d %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
        Serial.println("Server Response Body: ");
        Serial.println(response);  // Menampilkan pesan yang diberikan oleh server
    }

    // Bersihkan buffer dan akhiri HTTP request
    esp_camera_fb_return(fb);
    http.end();

    if (httpResponseCode == 200) {
      delay(1800000);
    }else{
      delay(10000);
    }
}