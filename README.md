# Timbangan HX711 Super Duper Akurat Kalibrasi Cell 40KG

Timbangan digital presisi tinggi berbasis ESP32, HX711, dan Load Cell 40KG dengan tampilan web yang elegan dan fitur kalibrasi lengkap. Cocok untuk timbangan dapur, paket, atau proyek IoT apapun yang butuh akurasi tinggi.

---

## Daftar Isi

- [Fitur Utama](#fitur-utama)
- [Komponen yang Dibutuhkan](#komponen-yang-dibutuhkan)
- [Skema Koneksi Hardware](#skema-koneksi-hardware)
- [Cara Kerja Sistem](#cara-kerja-sistem)
- [Instalasi Software](#instalasi-software)
- [Konfigurasi WiFi dan OTA](#konfigurasi-wifi-dan-ota)
- [Cara Upload Kode](#cara-upload-kode)
- [Penggunaan Setelah Upload](#penggunaan-setelah-upload)
- [Panduan Kalibrasi](#panduan-kalibrasi)
- [Penjelasan Filter Pembacaan Berat](#penjelasan-filter-pembacaan-berat)
- [Problem Solving](#problem-solving)
- [Tips Meningkatkan Akurasi](#tips-meningkatkan-akurasi)
- [Spesifikasi Teknis](#spesifikasi-teknis)
- [Lisensi](#lisensi)

---

## Fitur Utama

Berikut fitur-fitur yang membuat timbangan ini super duper akurat:

- Pembacaan berat real-time dengan kecepatan hingga 80 SPS
- Tampilan web yang modern dan responsif
- Empat pilihan satuan berat (Gram, Kilogram, Ons, Pound)
- Fitur TARE untuk mengatur ulang titik nol
- Tiga metode kalibrasi (Cepat, Akurat 2-titik, Manual)
- Riwayat pengukuran yang tersimpan di browser
- Peak weight tracker (menyimpan berat maksimum)
- Indikator kestabilan berat
- Update firmware via OTA (Over-The-Air)
- Tidak ada restart ESP saat kalibrasi (sampling 150x aman)

---

## Komponen yang Dibutuhkan

| Komponen | Jumlah | Keterangan |
|----------|--------|------------|
| ESP32 Dev Board | 1 | Otak sistem, menjalankan web server |
| Modul HX711 | 1 | Penguat sinyal dan ADC 24-bit |
| Load Cell 40KG | 1 | Sensor berat utama |
| Kabel USB | 1 | Untuk power dan upload program |
| Kabel Jumper Female-Female | 4 | Menghubungkan ESP32 ke HX711 |
| Power Supply 5V (opsional) | 1 | Untuk penggunaan tanpa USB |

---

## Skema Koneksi Hardware

Koneksi antara ESP32 dan modul HX711 sangat sederhana:

| ESP32 | HX711 | Keterangan |
|-------|-------|------------|
| GPIO 4 | DOUT | Data output dari HX711 |
| GPIO 5 | SCK | Clock untuk komunikasi |
| 3.3V | VCC | Sumber tegangan |
| GND | GND | Ground bersama |

Koneksi antara HX711 dan Load Cell:

| HX711 | Load Cell | Warna Kabel (umum) |
|-------|-----------|-------------------|
| E+ | Kabel Merah/Eksitasi+ | Merah |
| E- | Kabel Hitam/Eksitasi- | Hitam |
| A+ | Kabel Hijau/Signal+ | Hijau |
| A- | Kabel Putih/Signal- | Putih |

Catatan penting tentang pin RATE pada HX711:

```
HX711 Pin RATE = HIGH (3.3V)  -> 80 SPS (Sample Per Second)
HX711 Pin RATE = LOW/GND      -> 10 SPS
HX711 Pin RATE = Floating     -> 10 SPS
```

Untuk mendapatkan kecepatan 80 SPS, sambungkan pin RATE ke VCC (3.3V). Ini membuat timbangan terasa responsif seperti timbangan pasar.

---

## Cara Kerja Sistem

Sistem bekerja dengan alur sebagai berikut:

1. Load Cell menghasilkan sinyal listrik proporsional dengan berat yang diberikan
2. Sinyal diperkuat dan dikonversi ke digital 24-bit oleh HX711
3. ESP32 membaca data digital melalui protokol komunikasi serial
4. Data mentah diproses dengan filter untuk menghilangkan noise:
   - Median filter ukuran 5 untuk membuang spike
   - EMA filter adaptif untuk menghaluskan data
5. Data bersih dikonversi ke gram menggunakan calibration factor
6. ESP32 menjalankan web server yang menampilkan data berat
7. Browser di perangkat pengguna menampilkan antarmuka web

Diagram alir sederhana:

```
Load Cell (sinyal analog mV) 
    ↓
HX711 (penguat + ADC 24-bit) 
    ↓
ESP32 (baca data + filter + komputasi)
    ↓
WiFi (HTTP server)
    ↓
Browser (tampilan web real-time)
```

---

## Instalasi Software

### Pilihan 1: Menggunakan Arduino IDE

Langkah-langkah instalasi:

1. Install Arduino IDE dari situs resmi Arduino

2. Tambahkan board ESP32:
   - Buka Arduino IDE
   - Masuk ke File > Preferences
   - Pada Additional Boards Manager URLs, tambahkan:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   - Klik OK
   - Buka Tools > Board > Boards Manager
   - Cari "esp32" dan install paket "esp32 by Espressif Systems"

3. Install library yang diperlukan:
   - Buka Tools > Manage Libraries
   - Install library berikut satu per satu:
     - `HX711_ADC` oleh Olav Kallhovd
     - `ESPAsyncWebServer` oleh dvarrel
     - `AsyncTCP` (dependency dari ESPAsyncWebServer)

### Pilihan 2: Menggunakan PlatformIO

Buat file `platformio.ini` dengan konten:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    HX711_ADC
    ESPAsyncWebServer
    AsyncTCP
```

Kemudian buka folder project di VS Code dengan PlatformIO extension.

---

## Konfigurasi WiFi dan OTA

Sebelum upload, kamu perlu mengatur nama WiFi dan password di kode. Cari bagian ini:

```cpp
// ====================
// WIFI
// ====================
const char* ssid     = "LUTHFi 5G";
const char* password = "Risy0208";

// OTA
const char* OTA_HOSTNAME = "smartscale";
const char* OTA_PASSWORD = "smartscale123";
```

Ganti nilai sesuai jaringan WiFi rumahmu:

```cpp
const char* ssid     = "nama_wifi_kamu";
const char* password = "password_wifi_kamu";
```

Untuk keamanan OTA, ganti password:

```cpp
const char* OTA_PASSWORD = "password_ota_kamu";
```

---

## Cara Upload Kode

### Menggunakan Arduino IDE

1. Buka file .ino di Arduino IDE
2. Pilih board:
   - Tools > Board > ESP32 Arduino > ESP32 Dev Module
3. Pilih port:
   - Tools > Port > pilih port USB ESP32-mu (contoh: COM3 di Windows, /dev/ttyUSB0 di Linux)
4. Klik tombol Upload (panah kanan di toolbar)

### Menggunakan PlatformIO

1. Buka folder project di VS Code
2. Klik tombol Upload di toolbar PlatformIO
3. Atau jalankan perintah:
   ```
   pio run --target upload
   ```

### Monitor Serial

Buka Serial Monitor untuk melihat status ESP32:

- Arduino IDE: Tools > Serial Monitor (baud rate 115200)
- PlatformIO: Serial Monitor di toolbar

Setelah berhasil, akan muncul IP address seperti:

```
[WiFi] ✓ IP: 192.168.1.100
```

---

## Penggunaan Setelah Upload

Setelah ESP32 berhasil terhubung ke WiFi:

1. Buka browser di HP, tablet, atau laptop yang terhubung ke jaringan WiFi yang sama

2. Ketik IP address yang muncul di Serial Monitor:
   ```
   http://192.168.1.100
   ```

3. Tampilan web akan muncul dengan:

| Bagian | Fungsi |
|--------|--------|
| Berat Sekarang | Tampilan utama berat dalam angka besar |
| Indikator Kestabilan | Bar progress menunjukkan seberapa stabil berat |
| Tombol TARE | Mengatur ulang titik nol |
| Tombol Simpan | Menyimpan berat ke riwayat |
| Tombol Kalibrasi | Membuka panel kalibrasi |
| Tombol Hapus | Menghapus semua riwayat |
| Pilihan Satuan | Gram, Kilogram, Ons, Pound |
| Riwayat | Daftar berat yang tersimpan |

---

## Panduan Kalibrasi

Timbangan ini menyediakan tiga metode kalibrasi. Pilih yang paling sesuai dengan kebutuhanmu.

### Metode 1: Kalibrasi Cepat (Metode Rasio)

Metode paling mudah dan cepat:

1. Letakkan benda yang sudah diketahui beratnya di atas timbangan
2. Buka panel kalibrasi (tekan tombol Kalibrasi)
3. Pilih tab "Cepat"
4. Lihat nilai "Terbaca Sensor Sekarang"
5. Masukkan berat sebenarnya di kolom "Berat Asli"
6. Klik "Terapkan Koreksi"

Contoh:
```
Terbaca Sensor = 125.5 gram
Berat Asli     = 130.0 gram
Faktor baru akan dihitung otomatis
```

### Metode 2: Kalibrasi Akurat 2-Titik

Metode paling akurat untuk hasil presisi:

1. Pilih tab "Akurat"
2. Langkah 1: Kosongkan timbangan, klik "Rekam Titik Nol"
   - ESP akan mengambil 150 sampel untuk rata-rata
   - Proses aman, tidak akan restart
3. Langkah 2: Letakkan beban referensi (minimal 100 gram)
   - Tunggu hingga angka stabil
   - Masukkan berat benda referensi
   - Klik "Rekam & Hitung Faktor"

### Metode 3: Manual

Jika sudah tahu nilai calibration factor:

1. Pilih tab "Manual"
2. Lihat faktor aktif saat ini
3. Masukkan nilai faktor baru
4. Klik "Terapkan & Simpan ke EEPROM"

### Menyimpan Kalibrasi

Nilai calibration factor otomatis disimpan ke EEPROM ESP32. Setelah restart, ESP akan menggunakan faktor yang sama.

---

## Penjelasan Filter Pembacaan Berat

Untuk mendapatkan hasil yang stabil dan akurat, kode ini menggunakan dua jenis filter:

### 1. Median Filter

```
Ukuran buffer: 5 sampel
Fungsi: Membuang nilai ekstrem (spike)
```

Cara kerja: dari 5 sampel terakhir, ambil nilai tengah setelah diurutkan. Ini menghilangkan gangguan sesaat seperti getaran atau noise listrik.

### 2. EMA Filter (Exponential Moving Average)

```
EMA Cepat = 0.45 (saat perubahan besar)
EMA Lambat = 0.15 (saat stabil)
Ambang perubahan = 8.0 gram
```

Cara kerja:
- Jika perubahan berat > 8 gram, filter bereaksi cepat (0.45)
- Jika perubahan berat <= 8 gram, filter bereaksi lambat (0.15)
- Hasilnya: responsif saat ada perubahan, tetap halus saat stabil

### 3. Deadband

```
Nilai deadband = 1.0 gram
```

Berat di bawah 1 gram dianggap nol. Ini mencegah angka melayang saat timbangan kosong.

---

## Problem Solving

### ESP32 Restart Saat Kalibrasi

Sudah diatasi dengan `esp_task_wdt_reset()` di setiap iterasi sampling. 150 sampel aman.

### Angka Berat Tidak Stabil

Periksa:
- Koneksi kabel load cell ke HX711
- Pin RATE HX711 ke HIGH (3.3V) untuk 80 SPS
- Permukaan tempat timbangan datar
- Tidak ada getaran di sekitar

### WiFi Tidak Terhubung

Cek:
- SSID dan password benar
- Router WiFi menggunakan frekuensi 2.4GHz (ESP32 tidak support 5GHz)
- Jarak ESP32 ke router tidak terlalu jauh

### Tampilan Web Tidak Muncul

- Pastikan perangkat satu jaringan dengan ESP32
- Coba ping IP ESP32 dari terminal
- Cek Serial Monitor untuk IP yang benar

### Berat Negatif atau Tidak Masuk Akal

- Tekan tombol TARE
- Kalibrasi ulang dengan metode 2-titik
- Cek kabel load cell tidak terbalik

### Load Cell Tidak Terdeteksi

Serial Monitor akan menampilkan:
```
[ERR] HX711 tidak terdeteksi! Cek kabel:
      DOUT → GPIO4
      SCK  → GPIO5
```

Periksa kembali koneksi hardware.

---

## Tips Meningkatkan Akurasi

1. **Kalibrasi dengan beban berat**: Semakin berat beban referensi, semakin akurat hasilnya. Minimal 100 gram, idealnya 500 gram atau lebih.

2. **Gunakan permukaan datar**: Letakkan timbangan di meja yang rata dan stabil.

3. **Hindari getaran**: Jauhkan dari sumber getaran seperti kipas angin atau mesin.

4. **Pastikan kabel bebas tegangan**: Kabel load cell tidak boleh tertarik atau tertekuk.

5. **Lakukan warm-up**: Biarkan timbangan menyala 2-3 menit sebelum digunakan.

6. **TARE sebelum menimbang**: Selalu tekan TARE sebelum meletakkan benda baru.

7. **Pin RATE ke HIGH**: Ini penting untuk mendapatkan 80 SPS.

8. **Gunakan power supply stabil**: Jika memakai adaptor, pastikan 5V stabil.

---

## Spesifikasi Teknis

| Parameter | Nilai |
|-----------|-------|
| Mikrokontroler | ESP32 |
| ADC | HX711 24-bit |
| Load Cell | 40 KG |
| Resolusi ADC | 24-bit |
| Sample Rate | 80 SPS (max) |
| Akurasi | 1-2 gram (tergantung kalibrasi) |
| Koneksi | WiFi 2.4GHz |
| Protocol | HTTP |
| Update | OTA (Over-The-Air) |
| Filter | Median (5) + EMA Adaptif |
| Tampilan | Web Browser |
| Satuan | Gram, Kilogram, Ons, Pound |
| Fitur Kalibrasi | 3 metode |
| Daya | USB 5V / Micro USB |

---

## Struktur Kode

```
├── Setup
│   ├── Inisialisasi Serial
│   ├── Baca Calibration Factor dari EEPROM
│   ├── Koneksi WiFi
│   ├── Setup OTA
│   ├── Inisialisasi HX711
│   └── Setup Web Server Routes
│
├── Loop Utama
│   ├── ArduinoOTA.handle()
│   ├── LoadCell.update()
│   └── Logging periodik
│
├── Fungsi Filter
│   ├── getRawADCAverage() - sampling akurat
│   ├── medianSort() - filter median
│   └── getFilteredWeight() - filter lengkap
│
├── Fungsi Kalibrasi
│   ├── saveCalFactor() - simpan ke EEPROM
│   └── resetFilter() - reset filter state
│
└── Web Server Routes
    ├── / - halaman utama HTML
    ├── /weight - data berat real-time
    ├── /tare - reset titik nol
    ├── /calratio - kalibrasi cepat
    ├── /calzero - rekam titik nol
    ├── /calfactor - rekam beban referensi
    ├── /setfactor - set faktor manual
    └── /getfactor - ambil faktor saat ini
```

---

## Lisensi

Proyek ini bersifat open source dan bebas digunakan, dimodifikasi, dan didistribusikan. 

---

## Kontribusi

Mau berkontribusi atau ada saran? Silakan buat issue atau pull request. Semua masukan sangat dihargai.

---

## Disclaimer

Akurasi timbangan sangat bergantung pada kualitas komponen, kalibrasi, dan lingkungan penggunaan. Selalu verifikasi dengan timbangan referensi untuk penggunaan yang membutuhkan presisi tinggi.

---

**Selamat mencoba dan semoga timbangannya akurat!**
