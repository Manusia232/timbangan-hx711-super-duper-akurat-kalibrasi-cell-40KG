#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HX711_ADC.h>
#include <Preferences.h>
#include <algorithm>
#include <ArduinoOTA.h>
#include "esp_task_wdt.h"
#define HX711_DOUT 4
#define HX711_SCK 5
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PW";
const char* OTA_HOSTNAME = "tibangandigital";
const char* OTA_PASSWORD = "11223344";
AsyncWebServer server(80);
HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);
Preferences prefs;
float CALIBRATION_FACTOR = 158.6634f;
float zeroRaw = 0.0f;
float knownRaw = 0.0f;
#define CAL_SAMPLES 150
#define CAL_TIMEOUT_MS 20000UL
const int MED_SIZE = 5;
float medBuf[MED_SIZE];
int medIdx = 0;
bool medFull = false;
float emaWeight = 0.0f;
const float EMA_FAST = 0.45f;
const float EMA_SLOW = 0.15f;
const float EMA_DELTA_THRESHOLD = 8.0f;
const float DEADBAND = 1.0f;
const float AUTO_ZERO = 0.7f;
float stableWeight = 0.0f;
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <title>Timbangan Digital</title>
    <style>
         :root {
            --bg: #f8f9fa;
            --surface: #ffffff;
            --text: #212529;
            --muted: #6c757d;
            --border: #dee2e6;
            --primary: #0d6efd;
            --success: #198754;
            --danger: #dc3545;
            --warning: #ffc107;
            --radius: 8px;
        }
        
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            -webkit-tap-highlight-color: transparent;
        }
        
        body {
            background: var(--bg);
            color: var(--text);
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            padding-bottom: 40px;
            line-height: 1.5;
        }
        
        header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 16px;
            background: var(--surface);
            border-bottom: 1px solid var(--border);
            position: sticky;
            top: 0;
            z-index: 100;
        }
        
        .logo-text {
            font-size: 16px;
            font-weight: 600;
        }
        
        .logo-sub {
            font-size: 11px;
            color: var(--muted);
            display: block;
        }
        
        .hdr-right {
            display: flex;
            align-items: center;
            gap: 12px;
        }
        
        .cf-badge {
            font-size: 12px;
            color: var(--muted);
        }
        
        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: var(--success);
        }
        
        .main-card {
            background: var(--surface);
            margin: 16px;
            border-radius: var(--radius);
            padding: 24px;
            border: 1px solid var(--border);
            text-align: center;
        }
        
        .weight-label {
            font-size: 13px;
            color: var(--muted);
            margin-bottom: 8px;
        }
        
        .weight-display {
            display: flex;
            align-items: baseline;
            justify-content: center;
            gap: 4px;
            margin: 8px 0;
        }
        
        .weight-value {
            font-size: 64px;
            font-weight: 700;
            color: var(--text);
            letter-spacing: -1px;
        }
        
        .weight-unit {
            font-size: 24px;
            color: var(--muted);
        }
        
        .stab-lbl {
            font-size: 12px;
            color: var(--muted);
            margin-top: 8px;
        }
        
        .unit-toggle {
            display: flex;
            gap: 8px;
            margin: 0 16px 16px;
        }
        
        .unit-btn {
            flex: 1;
            padding: 8px;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            color: var(--text);
            font-size: 12px;
            font-weight: 500;
            cursor: pointer;
        }
        
        .unit-btn.active {
            background: var(--text);
            color: var(--surface);
            border-color: var(--text);
        }
        
        .stats-row {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 8px;
            margin: 0 16px 16px;
        }
        
        .stat-card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            padding: 12px 8px;
            text-align: center;
        }
        
        .stat-val {
            font-size: 14px;
            font-weight: 600;
        }
        
        .stat-lbl {
            font-size: 11px;
            color: var(--muted);
            margin-top: 2px;
        }
        
        .btn-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
            margin: 0 16px 16px;
        }
        
        .btn {
            padding: 12px;
            border: 1px solid var(--border);
            border-radius: var(--radius);
            font-size: 13px;
            font-weight: 500;
            cursor: pointer;
            background: var(--surface);
            color: var(--text);
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 6px;
        }
        
        .btn:active {
            background: #f1f3f5;
        }
        
        .btn-primary {
            background: var(--primary);
            color: white;
            border: none;
        }
        
        .btn-primary:active {
            background: #0b5ed7;
        }
        
        .btn-full {
            grid-column: 1/-1;
        }
        
        .section-title {
            font-size: 13px;
            font-weight: 600;
            color: var(--text);
            margin: 24px 16px 8px;
        }
        
        .history-list {
            margin: 0 16px 16px;
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        
        .h-item {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            padding: 12px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        
        .h-num {
            font-size: 11px;
            color: var(--muted);
        }
        
        .h-weight {
            font-size: 15px;
            font-weight: 600;
        }
        
        .h-time {
            font-size: 11px;
            color: var(--muted);
        }
        
        .h-empty {
            text-align: center;
            color: var(--muted);
            font-size: 13px;
            padding: 24px;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
        }
        
        .panel {
            margin: 0 16px 16px;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            overflow: hidden;
            display: none;
        }
        
        .panel.open {
            display: block;
        }
        
        .panel-hdr {
            padding: 12px 16px;
            border-bottom: 1px solid var(--border);
            font-weight: 600;
            font-size: 13px;
            background: #f1f3f5;
        }
        
        .panel-body {
            padding: 16px;
        }
        
        .cal-tabs {
            display: flex;
            gap: 6px;
            margin-bottom: 16px;
            background: #f1f3f5;
            padding: 4px;
            border-radius: var(--radius);
        }
        
        .cal-tab {
            flex: 1;
            padding: 6px;
            border-radius: 6px;
            color: var(--muted);
            font-size: 12px;
            font-weight: 500;
            cursor: pointer;
            text-align: center;
        }
        
        .cal-tab.active {
            background: var(--surface);
            color: var(--text);
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
        }
        
        .cal-mode {
            display: none;
        }
        
        .cal-mode.active {
            display: block;
        }
        
        .step {
            display: none;
        }
        
        .step.active {
            display: block;
        }
        
        .step-title {
            font-size: 14px;
            font-weight: 600;
            margin-bottom: 4px;
        }
        
        .step-desc {
            font-size: 12px;
            color: var(--muted);
            line-height: 1.5;
            margin-bottom: 12px;
        }
        
        .info-box {
            background: #f8f9fa;
            border-radius: var(--radius);
            padding: 12px;
            margin-bottom: 12px;
            border: 1px solid var(--border);
        }
        
        .ib-label {
            font-size: 11px;
            color: var(--muted);
        }
        
        .ib-val {
            font-size: 20px;
            font-weight: 600;
            color: var(--text);
        }
        
        .ib-sub {
            font-size: 11px;
            color: var(--muted);
        }
        
        .input-group {
            margin-bottom: 12px;
        }
        
        .input-group label {
            display: block;
            font-size: 12px;
            color: var(--text);
            margin-bottom: 4px;
            font-weight: 500;
        }
        
        .inp {
            width: 100%;
            padding: 10px;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            color: var(--text);
            font-size: 14px;
            outline: none;
        }
        
        .inp:focus {
            border-color: var(--primary);
        }
        
        .result-box {
            background: #e8f5e9;
            border: 1px solid #c8e6c9;
            border-radius: var(--radius);
            padding: 12px;
            margin-bottom: 12px;
            color: var(--success);
        }
        
        .rb-label {
            font-size: 11px;
            font-weight: 600;
        }
        
        .rb-val {
            font-size: 16px;
            font-weight: 700;
        }
        
        .warn-box {
            background: #ffebee;
            border: 1px solid #ffcdd2;
            border-radius: var(--radius);
            padding: 10px;
            margin-bottom: 12px;
            font-size: 12px;
            color: var(--danger);
            display: none;
        }
        
        .info-alert {
            background: #fffde7;
            border: 1px solid #fff9c4;
            border-radius: var(--radius);
            padding: 10px;
            margin-bottom: 12px;
            font-size: 12px;
            color: #663300;
        }
        
        .progress-steps {
            display: flex;
            gap: 4px;
            margin-bottom: 16px;
        }
        
        .ps {
            flex: 1;
            height: 4px;
            border-radius: 2px;
            background: var(--border);
        }
        
        .ps.done {
            background: var(--success);
        }
        
        .ps.active {
            background: var(--primary);
        }
        
        .toast {
            position: fixed;
            bottom: 20px;
            left: 50%;
            transform: translateX(-50%) translateY(100px);
            background: #323232;
            color: white;
            padding: 10px 20px;
            border-radius: 4px;
            font-size: 13px;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
            z-index: 9999;
            transition: transform 0.2s ease;
            white-space: nowrap;
            max-width: 90vw;
            text-align: center;
        }
        
        .toast.show {
            transform: translateX(-50%) translateY(0);
        }
        
        .toast.green {
            background: var(--success);
        }
        
        .toast.red {
            background: var(--danger);
        }
        
        .toast.orange {
            background: var(--warning);
            color: #212529;
        }
    </style>
</head>

<body>

    <header>
        <div>
            <span class="logo-text">Timbangan Digital</span>
            <span class="logo-sub">Sistem Pengukuran Berat</span>
        </div>
        <div class="hdr-right">
            <span class="cf-badge">Akurasi: <span id="cfBadge">–</span></span>
            <div class="status-dot" id="statusDot"></div>
        </div>
    </header>

    <div class="main-card">
        <div class="weight-label">Berat Saat Ini</div>
        <div class="weight-display">
            <span class="weight-value" id="weightVal">–</span>
            <span class="weight-unit" id="unitLabel">g</span>
        </div>
        <div class="stab-lbl" id="stabLbl">Menghubungkan ke alat...</div>
    </div>

    <div class="unit-toggle">
        <button class="unit-btn active" onclick="setUnit('g',this)">GRAM</button>
        <button class="unit-btn" onclick="setUnit('kg',this)">KG</button>
        <button class="unit-btn" onclick="setUnit('oz',this)">OZ</button>
        <button class="unit-btn" onclick="setUnit('lb',this)">LB</button>
    </div>

    <div class="stats-row">
        <div class="stat-card">
            <div class="stat-val" id="statPeak">–</div>
            <div class="stat-lbl">Paling Berat</div>
        </div>
        <div class="stat-card">
            <div class="stat-val" id="statCount">0</div>
            <div class="stat-lbl">Data Tersimpan</div>
        </div>
        <div class="stat-card">
            <div class="stat-val" id="statStab">–</div>
            <div class="stat-lbl">Kestabilan</div>
        </div>
    </div>

    <div class="btn-grid">
        <button class="btn" onclick="doTare()">Nol-kan (Tare)</button>
        <button class="btn" onclick="doSave()">Simpan Angka</button>
        <button class="btn" onclick="toggleCal()">Pengaturan Akurasi</button>
        <button class="btn" onclick="clearHistory()">Hapus Riwayat</button>
    </div>

    <div class="section-title">Riwayat Timbangan</div>
    <div class="history-list" id="historyList">
        <div class="h-empty">Belum ada data. Silakan tekan tombol "Simpan Angka" setelah menimbang.</div>
    </div>

    <div class="section-title">Pengaturan Kalibrasi</div>
    <div class="panel" id="calPanel">
        <div class="panel-hdr">Menu Kalibrasi</div>
        <div class="panel-body">

            <div class="cal-tabs">
                <div class="cal-tab active" onclick="switchMode('ratio',this)">Cara Cepat</div>
                <div class="cal-tab" onclick="switchMode('guided',this)">Cara Akurat</div>
                <div class="cal-tab" onclick="switchMode('manual',this)">Ketik Manual</div>
            </div>

            <div class="cal-mode active" id="mode-ratio">
                <div class="step-title">Koreksi Cepat</div>
                <div class="step-desc">
                    Letakkan benda yang sudah Anda ketahui berat pastinya. Periksa angka yang terbaca, lalu masukkan berat yang sebenarnya di bawah ini.
                </div>

                <div class="info-alert">
                    Pastikan timbangan sudah berada di posisi 0 sebelum meletakkan benda, dan tunggu sampai angka tidak berubah-ubah.
                </div>

                <div class="info-box">
                    <div class="ib-label">Terbaca Timbangan Saat Ini</div>
                    <div class="ib-val" id="ratioRead">– g</div>
                    <div class="ib-sub" id="ratioStabStatus">–</div>
                </div>

                <div class="input-group">
                    <label>Berat Sebenarnya (gram)</label>
                    <input class="inp" type="number" id="ratioActual" placeholder="Contoh: 100" min="0.1" step="0.1">
                </div>

                <div class="warn-box" id="ratioWarn"></div>

                <button class="btn btn-primary btn-full" onclick="applyRatio()">Terapkan Penyesuaian</button>

                <div class="result-box" id="ratioResult" style="display:none; margin-top:12px;">
                    <div class="rb-label">Berhasil Diperbarui</div>
                    <div class="rb-val" id="ratioResultVal">–</div>
                    <div style="font-size:11px; margin-top:4px" id="ratioVerif">Silakan angkat dan timbang ulang benda untuk memastikan.</div>
                </div>
            </div>

            <div class="cal-mode" id="mode-guided">
                <div class="progress-steps">
                    <div class="ps active" id="ps1"></div>
                    <div class="ps" id="ps2"></div>
                    <div class="ps" id="ps3"></div>
                </div>

                <div class="step active" id="gs1">
                    <div class="step-title">Langkah 1: Kosongkan Timbangan</div>
                    <div class="step-desc">
                        Pastikan tidak ada benda sama sekali di atas timbangan. Sistem akan mencatat titik nol yang bersih.
                    </div>
                    <div class="info-box">
                        <div class="ib-label">Berat Saat Ini</div>
                        <div class="ib-val" id="gs1Read">– g</div>
                        <div class="ib-sub">Angka harus mendekati 0 sebelum lanjut</div>
                    </div>
                    <div class="warn-box" id="gs1Warn"></div>
                    <button class="btn btn-primary btn-full" onclick="guidedStep1()">Atur Titik Nol</button>
                </div>

                <div class="step" id="gs2">
                    <div class="step-title">Langkah 2: Taruh Beban Contoh</div>
                    <div class="step-desc">
                        Taruh benda yang sudah tahu berat pastinya (disarankan minimal 100 gram). Tunggu angka tenang, lalu ketik berat aslinya.
                    </div>
                    <div class="info-box">
                        <div class="ib-label">Terbaca Saat Ini</div>
                        <div class="ib-val" id="gs2Read">– g</div>
                        <div class="ib-sub" id="gs2Stab">–</div>
                    </div>
                    <div class="input-group">
                        <label>Berat Beban Contoh (gram)</label>
                        <input class="inp" type="number" id="guidedMass" placeholder="Contoh: 500" min="10" step="0.1">
                    </div>
                    <div class="warn-box" id="gs2Warn"></div>
                    <button class="btn btn-primary btn-full" onclick="guidedStep2()">Simpan & Hitung Akurasi</button>
                    <button class="btn btn-full" style="margin-top:8px; border:none;" onclick="guidedBack()">Kembali</button>
                </div>

                <div class="step" id="gs3">
                    <div class="step-title" style="color:var(--success)">Kalibrasi Selesai</div>
                    <div class="step-desc">Timbangan sudah disesuaikan dan disimpan ke memori alat.</div>
                    <div class="result-box">
                        <div class="rb-label">Angka Akurasi Baru</div>
                        <div class="rb-val" id="guidedResultVal">–</div>
                    </div>
                    <div class="result-box" style="background:#f8f9fa; border-color:var(--border); color:var(--text)">
                        <div class="rb-label">Uji Coba Berat</div>
                        <div class="rb-val" id="guidedVerifRead">–</div>
                    </div>
                    <button class="btn btn-primary btn-full" onclick="guidedReset()">Ulangi Kalibrasi</button>
                    <button class="btn btn-full" style="margin-top:8px;" onclick="toggleCal()">Tutup Pengaturan</button>
                </div>
            </div>

            <div class="cal-mode" id="mode-manual">
                <div class="step-title">Masukkan Angka Manual</div>
                <div class="step-desc">
                    Gunakan pilihan ini jika Anda sudah mengetahui angka kalibrasi dari pengaturan sebelumnya.
                </div>
                <div class="info-box">
                    <div class="ib-label">Angka Akurasi Aktif</div>
                    <div class="ib-val" id="manualCurrentFactor">–</div>
                </div>
                <div class="input-group">
                    <label>Masukkan Angka Baru</label>
                    <input class="inp" type="number" id="manualFactor" placeholder="Contoh: 157.00" step="0.0001" min="0.001">
                </div>
                <button class="btn btn-primary btn-full" onclick="applyManual()">Simpan ke Alat</button>
            </div>

        </div>
    </div>

    <div class="toast" id="toast"></div>

    <script>
        let currentUnit = 'g';
        let currentGram = 0;
        let prevGram = 0;
        let history = JSON.parse(localStorage.getItem('ss_history') || '[]');
        let savedCount = history.length;
        let peak = 0;
        let stabBuf = [];
        let online = false;

        const UF = {
            g: 1,
            kg: 0.001,
            oz: 0.03527396,
            lb: 0.00220462
        };
        const UD = {
            g: 1,
            kg: 3,
            oz: 2,
            lb: 3
        };

        async function poll() {
            try {
                const [rW, rF] = await Promise.all([
                    fetch('/weight').then(r => r.text()),
                    fetch('/getfactor').then(r => r.text())
                ]);
                const g = parseFloat(rW);
                if (!isNaN(g)) {
                    updateDisplay(g);
                    setDot(true);
                }
                const cf = parseFloat(rF);
                if (!isNaN(cf)) document.getElementById('cfBadge').textContent = cf.toFixed(2);
            } catch {
                setDot(false);
            }
        }

        function setDot(on) {
            online = on;
            const d = document.getElementById('statusDot');
            d.style.background = on ? 'var(--success)' : 'var(--danger)';
        }

        function conv(g) {
            return (g * UF[currentUnit]).toFixed(UD[currentUnit]);
        }

        function updateDisplay(g) {
            currentGram = g;

            stabBuf.push(g);
            if (stabBuf.length > 15) stabBuf.shift();
            const rng = Math.max(...stabBuf) - Math.min(...stabBuf);
            const pct = Math.max(0, Math.min(100, 100 - rng * 12));

            const stabTxt = pct > 88 ? 'Angka Stabil' : pct > 55 ? 'Menyetabilkan...' : 'Beban Bergerak';
            document.getElementById('stabLbl').textContent = stabTxt;
            document.getElementById('statStab').textContent = Math.round(pct) + '%';

            const disp = conv(g);
            const el = document.getElementById('weightVal');
            el.textContent = disp;
            prevGram = g;

            if (g > peak) {
                peak = g;
                document.getElementById('statPeak').textContent = conv(peak) + ' ' + currentUnit;
            }

            const rr = document.getElementById('ratioRead');
            if (rr) {
                rr.textContent = g.toFixed(2) + ' g';
            }
            const rs = document.getElementById('ratioStabStatus');
            if (rs) {
                rs.textContent = stabTxt;
            }
            const g1 = document.getElementById('gs1Read');
            if (g1) g1.textContent = g.toFixed(2) + ' g';
            const g2 = document.getElementById('gs2Read');
            if (g2) g2.textContent = g.toFixed(2) + ' g';
            const g2s = document.getElementById('gs2Stab');
            if (g2s) g2s.textContent = stabTxt;
            const gv = document.getElementById('guidedVerifRead');
            if (gv && document.getElementById('gs3').classList.contains('active')) {
                gv.textContent = g.toFixed(2) + ' g';
            }
        }

        function setUnit(u, btn) {
            currentUnit = u;
            document.getElementById('unitLabel').textContent = u;
            document.querySelectorAll('.unit-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            updateDisplay(currentGram);
        }

        async function doTare() {
            showToast('Mengatur ulang ke posisi 0...', '');
            peak = 0;
            stabBuf = [];
            document.getElementById('statPeak').textContent = '–';
            try {
                await fetch('/tare');
                showToast('Timbangan kembali ke angka 0', 'green');
            } catch {
                showToast('Gagal mereset, periksa koneksi alat', 'red');
            }
        }

        function doSave() {
            if (Math.abs(currentGram) < 0.3) {
                showToast('Timbangan masih kosong', 'red');
                return;
            }
            const e = {
                gram: currentGram,
                disp: conv(currentGram) + ' ' + currentUnit,
                time: new Date().toLocaleTimeString('id-ID', {
                    hour: '2-digit',
                    minute: '2-digit',
                    second: '2-digit'
                })
            };
            history.unshift(e);
            if (history.length > 50) history.pop();
            localStorage.setItem('ss_history', JSON.stringify(history));
            savedCount = history.length;
            document.getElementById('statCount').textContent = savedCount;
            renderHistory();
            showToast('Berhasil disimpan: ' + e.disp, 'green');
        }

        function clearHistory() {
            if (!history.length) {
                showToast('Riwayat sudah kosong', '');
                return;
            }
            history = [];
            savedCount = 0;
            peak = 0;
            localStorage.setItem('ss_history', '[]');
            document.getElementById('statCount').textContent = '0';
            document.getElementById('statPeak').textContent = '–';
            renderHistory();
            showToast('Semua riwayat telah dihapus', '');
        }

        function renderHistory() {
            const l = document.getElementById('historyList');
            if (!history.length) {
                l.innerHTML = '<div class="h-empty">Belum ada data. Silakan tekan tombol "Simpan Angka" setelah menimbang.</div>';
                return;
            }
            l.innerHTML = history.map((h, i) => `
    <div class="h-item">
      <span class="h-num">Data ke-${history.length-i}</span>
      <span class="h-weight">${h.disp}</span>
      <span class="h-time">${h.time}</span>
    </div>`).join('');
        }

        function toggleCal() {
            const p = document.getElementById('calPanel');
            const opening = !p.classList.contains('open');
            p.classList.toggle('open');
            if (opening) loadFactor();
        }

        function switchMode(m, btn) {
            document.querySelectorAll('.cal-tab').forEach(t => t.classList.remove('active'));
            btn.classList.add('active');
            document.querySelectorAll('.cal-mode').forEach(x => x.classList.remove('active'));
            document.getElementById('mode-' + m).classList.add('active');
            if (m === 'manual') loadFactor();
            if (m === 'ratio') {
                document.getElementById('ratioResult').style.display = 'none';
                document.getElementById('ratioWarn').style.display = 'none';
            }
        }

        async function loadFactor() {
            try {
                const v = await fetch('/getfactor').then(r => r.text());
                const cf = parseFloat(v).toFixed(4);
                const el = document.getElementById('manualCurrentFactor');
                if (el) el.textContent = cf;
                document.getElementById('cfBadge').textContent = parseFloat(cf).toFixed(2);
            } catch {}
        }

        async function applyRatio() {
            const actual = parseFloat(document.getElementById('ratioActual').value);
            const read = currentGram;
            const warnEl = document.getElementById('ratioWarn');

            warnEl.style.display = 'none';

            if (isNaN(actual) || actual <= 0) {
                warnEl.textContent = 'Silakan masukkan berat asli yang benar!';
                warnEl.style.display = 'block';
                return;
            }
            if (Math.abs(read) < 1.0) {
                warnEl.textContent = 'Timbangan mendeteksi beban kosong. Letakkan benda terlebih dahulu.';
                warnEl.style.display = 'block';
                return;
            }
            if (Math.abs(actual - read) / actual < 0.01) {
                showToast('Timbangan sudah cukup akurat', 'orange');
                return;
            }

            showToast('Memproses penyesuaian...', '');
            try {
                const res = await fetch('/calratio?read=' + read.toFixed(6) + '&actual=' + actual.toFixed(6));
                const txt = await res.text();
                if (!res.ok) {
                    showToast('Gagal: ' + txt, 'red');
                    return;
                }

                const newCF = parseFloat(txt);
                document.getElementById('ratioResultVal').textContent = newCF.toFixed(4);
                document.getElementById('ratioResult').style.display = 'block';
                document.getElementById('ratioActual').value = '';
                document.getElementById('cfBadge').textContent = newCF.toFixed(2);
                await loadFactor();
                showToast('Akurasi berhasil diperbarui', 'green');
            } catch (e) {
                showToast('Gagal memproses data', 'red');
            }
        }

        function updateProgress(step) {
            for (let i = 1; i <= 3; i++) {
                const el = document.getElementById('ps' + i);
                if (i < step) el.className = 'ps done';
                else if (i === step) el.className = 'ps active';
                else el.className = 'ps';
            }
        }

        async function guidedStep1() {
            const warnEl = document.getElementById('gs1Warn');
            warnEl.style.display = 'none';
            showToast('Mencatat titik nol, mohon tunggu sebentar...', 'orange');
            try {
                const res = await fetch('/calzero');
                const txt = await res.text();
                if (!res.ok) {
                    warnEl.textContent = 'Gagal: ' + txt;
                    warnEl.style.display = 'block';
                    return;
                }
                document.getElementById('gs1').classList.remove('active');
                document.getElementById('gs2').classList.add('active');
                updateProgress(2);
                showToast('Titik nol berhasil dicatat', 'green');
            } catch (e) {
                warnEl.textContent = 'Koneksi terputus';
                warnEl.style.display = 'block';
            }
        }

        async function guidedStep2() {
            const mass = parseFloat(document.getElementById('guidedMass').value);
            const warnEl = document.getElementById('gs2Warn');
            warnEl.style.display = 'none';

            if (isNaN(mass) || mass < 1) {
                warnEl.textContent = 'Masukkan berat beban contoh dengan benar!';
                warnEl.style.display = 'block';
                return;
            }

            showToast('Menghitung akurasi, mohon tunggu...', 'orange');
            try {
                const res = await fetch('/calfactor?mass=' + mass.toFixed(4));
                const txt = await res.text();
                if (!res.ok) {
                    warnEl.textContent = 'Gagal: ' + txt;
                    warnEl.style.display = 'block';
                    return;
                }

                const newCF = parseFloat(txt);
                document.getElementById('guidedResultVal').textContent = newCF.toFixed(4);
                document.getElementById('gs2').classList.remove('active');
                document.getElementById('gs3').classList.add('active');
                updateProgress(3);
                document.getElementById('cfBadge').textContent = newCF.toFixed(2);
                await loadFactor();
                showToast('Kalibrasi selesai', 'green');
            } catch (e) {
                warnEl.textContent = 'Koneksi terputus';
                warnEl.style.display = 'block';
            }
        }

        function guidedBack() {
            document.getElementById('gs2').classList.remove('active');
            document.getElementById('gs1').classList.add('active');
            updateProgress(1);
        }

        function guidedReset() {
            ['gs1', 'gs2', 'gs3'].forEach(s => document.getElementById(s).classList.remove('active'));
            document.getElementById('gs1').classList.add('active');
            document.getElementById('guidedMass').value = '';
            document.getElementById('gs1Warn').style.display = 'none';
            document.getElementById('gs2Warn').style.display = 'none';
            updateProgress(1);
        }

        async function applyManual() {
            const v = parseFloat(document.getElementById('manualFactor').value);
            if (isNaN(v) || v <= 0) {
                showToast('Masukkan angka yang benar!', 'red');
                return;
            }
            showToast('Menyimpan data...', '');
            try {
                const res = await fetch('/setfactor?v=' + v.toFixed(6));
                const txt = await res.text();
                if (!res.ok) {
                    showToast('Gagal menyimpan', 'red');
                    return;
                }
                document.getElementById('manualFactor').value = '';
                document.getElementById('cfBadge').textContent = v.toFixed(2);
                await loadFactor();
                showToast('Angka berhasil disimpan', 'green');
            } catch (e) {
                showToast('Koneksi terputus', 'red');
            }
        }

        let tt;

        function showToast(msg, type = '') {
            const t = document.getElementById('toast');
            t.textContent = msg;
            t.className = 'toast show' + (type ? ' ' + type : '');
            clearTimeout(tt);
            tt = setTimeout(() => t.classList.remove('show'), 3000);
        }

        renderHistory();
        document.getElementById('statCount').textContent = savedCount;
        setInterval(poll, 150);
        poll();
    </script>
</body>

</html>
)rawliteral";
float getRawADCAverage(int samples){
  float sum = 0;
  int got = 0;
  unsigned long start = millis();
  while(got < samples && millis() - start < CAL_TIMEOUT_MS){
    if(LoadCell.update()){
      sum += LoadCell.getData() * CALIBRATION_FACTOR;
      got++;
    }
    yield();
    esp_task_wdt_reset();
  }
  if(got == 0) return 0;
  return sum / (float)got;
}
float medianSort(float* arr, int n){
  float tmp[MED_SIZE];
  int cnt = (n < MED_SIZE) ? n : MED_SIZE;
  memcpy(tmp, arr, cnt * sizeof(float));
  std::sort(tmp, tmp + cnt);
  return tmp[cnt / 2];
}
float getFilteredWeight(){
  extern float medBuf[];
  extern int medIdx;
  extern bool medFull;

  LoadCell.update();
  float raw = LoadCell.getData();

  medBuf[medIdx] = raw;
  medIdx = (medIdx + 1) % MED_SIZE;
  if(!medFull && medIdx == 0) medFull = true;

  int cnt = medFull ? MED_SIZE : (medIdx == 0 ? MED_SIZE : medIdx);
  if(cnt < 1) return 0;
  float med = medianSort(medBuf, cnt);

  float delta = fabsf(med - emaWeight);
  float alpha = (delta > EMA_DELTA_THRESHOLD) ? EMA_FAST : EMA_SLOW;
  emaWeight = alpha * med + (1.0f - alpha) * emaWeight;

  if(fabsf(emaWeight) < AUTO_ZERO) emaWeight = 0.0f;
  if(fabsf(emaWeight - stableWeight) >= DEADBAND) stableWeight = emaWeight;

  return stableWeight;
}
void resetFilter(){
  memset(medBuf, 0, sizeof(medBuf));
  medIdx = 0;
  medFull = false;
  emaWeight = 0.0f;
  stableWeight = 0.0f;
}
void saveCalFactor(){
  prefs.begin("scale", false);
  prefs.putFloat("cal_factor", CALIBRATION_FACTOR);
  prefs.end();
  Serial.printf("[CAL] Factor disimpan ke EEPROM: %.6f\n", CALIBRATION_FACTOR);
}
void setup(){
  Serial.begin(115200);
  delay(200);
  Serial.println("\n╔══════════════════════╗");
  Serial.println("║  tibangandigital v4.0 OTA ║");
  Serial.println("╚══════════════════════╝");

  resetFilter();

  prefs.begin("scale", true);
  CALIBRATION_FACTOR = prefs.getFloat("cal_factor", 1000.0f);
  prefs.end();
  Serial.printf("[CAL] Factor dari EEPROM: %.6f\n", CALIBRATION_FACTOR);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Connecting");
  int wTry = 0;
  while(WiFi.status() != WL_CONNECTED && wTry < 40){
    delay(500); Serial.print("."); wTry++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.printf("\n[WiFi] ✓ IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] ✗ Gagal! Cek SSID/password.");
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if(strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA
    .onStart([](){
      String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
      Serial.println("[OTA] Mulai update " + type);
    })
    .onEnd([](){
      Serial.println("\n[OTA] Selesai, reboot...");
    })
    .onProgress([](unsigned int progress, unsigned int total){
      Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error){
      Serial.printf("[OTA] Error[%u]: ", error);
      if(error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if(error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if(error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if(error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if(error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("[OTA] Siap. Di Arduino IDE: Tools > Port > pilih '" + String(OTA_HOSTNAME) + " at " + WiFi.localIP().toString() + "'");

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 10000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = false
    };

esp_task_wdt_init(&wdt_config);

  LoadCell.begin();
  LoadCell.start(2000, true);
  if(LoadCell.getTareTimeoutFlag() || !LoadCell.getTareStatus()){
    Serial.println("[ERR] HX711 tidak terdeteksi! Cek kabel:");
    Serial.printf("      DOUT → GPIO%d\n      SCK  → GPIO%d\n", HX711_DOUT, HX711_SCK);
    LoadCell.begin();
    LoadCell.start(2000, true);
  }
  LoadCell.setCalFactor(CALIBRATION_FACTOR);
  LoadCell.setSamplesInUse(1);

  Serial.println("[HX711] OK");
  Serial.printf("[HX711] Nilai awal: %.2f g\n", LoadCell.getData());
  Serial.println("[HX711] CATATAN KECEPATAN: untuk mencapai ~80SPS (mendekati");
  Serial.println("        timbangan pasar), pastikan pin RATE modul HX711");
  Serial.println("        ditarik ke HIGH (VCC), bukan GND/floating.");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send_P(200, "text/html", index_html);
  });

  server.on("/weight", HTTP_GET, [](AsyncWebServerRequest* req){
    float w = getFilteredWeight();
    float r = roundf(w * 2.0f) / 2.0f;
    req->send(200, "text/plain", String(r, 1));
  });

  server.on("/tare", HTTP_GET, [](AsyncWebServerRequest* req){
    LoadCell.tareNoDelay();
    resetFilter();
    req->send(200, "text/plain", "OK");
  });

  server.on("/calratio", HTTP_GET, [](AsyncWebServerRequest* req){
    if(!req->hasParam("read") || !req->hasParam("actual")){
      req->send(400, "text/plain", "ERR: missing params"); return;
    }
    float read = req->getParam("read")->value().toFloat();
    float actual = req->getParam("actual")->value().toFloat();

    if(fabsf(read) < 0.5f){
      req->send(400, "text/plain", "ERR: read terlalu kecil (<0.5g)"); return;
    }
    if(actual <= 0){
      req->send(400, "text/plain", "ERR: actual harus >0"); return;
    }

    float rawADC = read * CALIBRATION_FACTOR;
    float newFactor = rawADC / actual;

    if(newFactor <= 0 || isnan(newFactor) || isinf(newFactor)){
      req->send(500, "text/plain", "ERR: hasil faktor tidak valid"); return;
    }

    Serial.printf("[CAL-A] read=%.4fg, actual=%.4fg\n", read, actual);
    Serial.printf("[CAL-A] rawADC=%.2f, factor: %.6f → %.6f\n",
                  rawADC, CALIBRATION_FACTOR, newFactor);

    CALIBRATION_FACTOR = newFactor;
    LoadCell.setCalFactor(CALIBRATION_FACTOR);
    resetFilter();
    saveCalFactor();

    req->send(200, "text/plain", String(CALIBRATION_FACTOR, 6));
  });

  server.on("/calzero", HTTP_GET, [](AsyncWebServerRequest* req){
    Serial.printf("[CAL-B] Merekam titik nol (%d sampel)...\n", CAL_SAMPLES);

    LoadCell.tare();
    delay(200);

    zeroRaw = getRawADCAverage(CAL_SAMPLES);

    Serial.printf("[CAL-B] zeroRaw = %.2f\n", zeroRaw);
    req->send(200, "text/plain", String(zeroRaw, 2));
  });

  server.on("/calfactor", HTTP_GET, [](AsyncWebServerRequest* req){
    if(!req->hasParam("mass")){
      req->send(400, "text/plain", "ERR: missing mass"); return;
    }
    float mass = req->getParam("mass")->value().toFloat();
    if(mass <= 0){
      req->send(400, "text/plain", "ERR: mass harus >0"); return;
    }

    Serial.printf("[CAL-B] Merekam titik beban %.2fg (%d sampel)...\n", mass, CAL_SAMPLES);
    knownRaw = getRawADCAverage(CAL_SAMPLES);

    float diff = knownRaw - zeroRaw;
    Serial.printf("[CAL-B] zeroRaw=%.2f, knownRaw=%.2f, diff=%.2f\n",
                  zeroRaw, knownRaw, diff);

    if(fabsf(diff) < 100){
      req->send(500, "text/plain", "ERR: perbedaan ADC terlalu kecil, cek beban & kabel"); return;
    }

    float newFactor = diff / mass;
    if(newFactor <= 0 || isnan(newFactor)){
      req->send(500, "text/plain", "ERR: hasil faktor tidak valid"); return;
    }

    CALIBRATION_FACTOR = newFactor;
    LoadCell.setCalFactor(CALIBRATION_FACTOR);
    resetFilter();
    saveCalFactor();

    Serial.printf("[CAL-B] factor baru = %.6f\n", CALIBRATION_FACTOR);
    req->send(200, "text/plain", String(CALIBRATION_FACTOR, 6));
  });

  server.on("/setfactor", HTTP_GET, [](AsyncWebServerRequest* req){
    if(!req->hasParam("v")){ req->send(400,"text/plain","ERR: missing v"); return; }
    float v = req->getParam("v")->value().toFloat();
    if(v <= 0){ req->send(400,"text/plain","ERR: must >0"); return; }
    CALIBRATION_FACTOR = v;
    LoadCell.setCalFactor(CALIBRATION_FACTOR);
    resetFilter();
    saveCalFactor();
    req->send(200,"text/plain", String(CALIBRATION_FACTOR,6));
  });

  server.on("/getfactor", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send(200,"text/plain", String(CALIBRATION_FACTOR,6));
  });

  server.begin();
  Serial.println("[HTTP] Server started ✓");
  Serial.printf("[HTTP] Buka: http://%s\n", WiFi.localIP().toString().c_str());
}
void loop(){
  ArduinoOTA.handle();

  LoadCell.update();

  if(LoadCell.getTareStatus()){
    resetFilter();
    Serial.println("[TARE] ✓ Selesai");
  }

  static unsigned long lastLog = 0;
  if(millis() - lastLog > 2000){
    lastLog = millis();
    Serial.printf("[W] stable=%.2fg | ema=%.2f | CF=%.4f | wifi=%s\n",
                  stableWeight, emaWeight, CALIBRATION_FACTOR,
                  WiFi.status()==WL_CONNECTED?"OK":"DISC");
  }
}
