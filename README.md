# 🏭 Alçı Dökme Makinesi Otomasyonu

Arduino Mega 2560 tabanlı, alçı karıştırma ve kalıba dökme işlemini tam otomatik olarak gerçekleştiren makine kontrol sistemi.

---

## 📋 İçindekiler

- [Genel Bakış](#genel-bakış)
- [Donanım](#donanım)
- [Devre Bağlantıları](#devre-bağlantıları)
- [Çalışma Sırası](#çalışma-sırası)
- [Kurulum](#kurulum)
- [Kullanım](#kullanım)
- [Özellikler](#özellikler)
- [Medya](#medya)

---

## Genel Bakış

Bu proje; dalgıç pompa, rötari, karıştırıcı, konveyör bant, servo kapak ve IR sensörden oluşan bir alçı dökme makinesini Arduino Mega 2560 ile kontrol eder. Sistem, su dolumu → karıştırma → kalıplara dökme adımlarını sırayla ve otomatik olarak yürütür.

---

## Donanım

| Bileşen | Adet | Açıklama |
|---|---|---|
| Arduino Mega 2560 | 1 | Ana mikrodenetleyici |
| L298N Motor Sürücü | 2 | DC motor kontrolü |
| 12V DC Motor | 3 | Rötari, Karıştırıcı, Konveyör Bant |
| Dalgıç Pompa | 1 | Su/sıvı aktarımı |
| Servo Motor (MG996R) | 1 | Dağıtım kapağı |
| IR Sensör | 1 | Kap algılama |
| 16x2 I2C LCD | 1 | Durum ekranı |
| 12V Güç Adaptörü | 2 | Motor kartları için |
| PC / USB | 1 | Arduino güç kaynağı + seri haberleşme |

---

## Devre Bağlantıları

### L298N #1 — Rötari + Konveyör Bant

| L298N #1 | Arduino Mega |
|---|---|
| IN1 | Pin 22 |
| IN2 | Pin 23 |
| ENA | Pin 2 (PWM) |
| IN3 | Pin 24 |
| IN4 | Pin 25 |
| ENB | Pin 3 (PWM) |
| 12V | GK #1 (+) |
| GND | GK #1 (−) + Arduino GND |

- **OUT1 / OUT2** → Rötari DC Motor  
- **OUT3 / OUT4** → Konveyör Bant DC Motor  
> ⚠️ ENA ve ENB üzerindeki jumper'ları söküp kablo takın.

---

### L298N #2 — Karıştırıcı + Dalgıç Pompa

| L298N #2 | Arduino Mega |
|---|---|
| IN1 | Pin 26 |
| IN2 | Pin 27 |
| ENA | Pin 4 (PWM) |
| IN3 | Pin 28 |
| IN4 | Pin 29 |
| ENB | Pin 5 (PWM) |
| 12V | GK #2 (+) |
| GND | GK #2 (−) + Arduino GND |

- **OUT1 / OUT2** → Karıştırıcı DC Motor  
- **OUT3 / OUT4** → Dalgıç Pompa  
> ⚠️ ENA ve ENB üzerindeki jumper'ları söküp kablo takın.

---

### Servo Motor (MG996R)

| Kablo | Bağlantı |
|---|---|
| Kahverengi (GND) | Arduino GND + Harici 5V (−) |
| Kırmızı (VCC) | Harici 5V (+) |
| Turuncu (Sinyal) | Arduino Pin 8 |

---

### IR Sensör

| IR Sensör | Bağlantı |
|---|---|
| VCC | Arduino 5V |
| GND | Arduino GND |
| OUT | Arduino Pin 30 |

---

### 16x2 I2C LCD (Adres: 0x27)

| LCD | Bağlantı |
|---|---|
| VCC | Arduino 5V |
| GND | Arduino GND |
| SDA | Arduino Pin 20 |
| SCL | Arduino Pin 21 |

---

### Acil Durdurma Butonu

| Buton | Bağlantı |
|---|---|
| Bir ucu | Arduino Pin 18 (INT1) |
| Diğer ucu | GND |

---

## Çalışma Sırası

```
[BAŞLA]
   │
   ▼
Dalgıç Pompa — 35 sn çalışır (su/alçı besleme)
   │
   ▼
Rötari Motor — 8 sn çalışır (karıştırma hazırlığı)
   │
   ▼
Karıştırıcı Motor — 10 sn çalışır
   │
   ▼
Konveyör Bant çalışır → IR Sensör Kap #1'i algılar
   │
   ▼
Bant durur (3 sn) → Servo 110° açılır → 1 sn sonra kapanır
   │
   ▼
Konveyör Bant çalışır → IR Sensör Kap #2'yi algılar
   │
   ▼
Bant durur (3 sn) → Servo 110° açılır → 1 sn sonra kapanır
   │
   ▼
Son bant hareketi — 3 sn
   │
   ▼
[TAMAMLANDI]
```

---

## Hız Ayarları

| Motor | PWM Değeri | Oran |
|---|---|---|
| Dalgıç Pompa | 191 | %75 |
| Rötari | 255 | %100 (tam hız) |
| Karıştırıcı | 191 | %75 |
| Konveyör Bant | 191 | %75 |

> Rötari tam hızda dijital HIGH ile sürülür; Servo kütüphanesi Timer çakışmasını önlemek için PWM yerine `digitalWrite(HIGH)` tercih edilir.

---

## Kurulum

### Gerekli Kütüphaneler

Arduino IDE'ye aşağıdaki kütüphaneleri yükleyin:

- `LiquidCrystal_I2C` — Frank de Brabander
- `Servo` — Arduino (yerleşik)
- `Wire` — Arduino (yerleşik)

### Adımlar

1. Bu repoyu klonlayın:
   ```bash
   git clone https://github.com/KULLANICI_ADI/alci-dokum-makinesi.git
   ```
2. `alci_otomasyon.ino` dosyasını Arduino IDE ile açın.
3. Kütüphaneleri yükleyin.
4. **Araçlar → Kart:** `Arduino Mega or Mega 2560` seçin.
5. Doğru COM portunu seçin ve yükleyin.

---

## Kullanım

Kodu yükledikten sonra Arduino'yu PC'ye bağlı bırakın ve **Seri Monitör**ü açın (9600 baud):

| Komut | Açıklama |
|---|---|
| `s` | Sistemi başlat |
| `t` | IR sensör testi |
| `x` | Acil durdurma |
| `r` | Acil durdurma sonrası sıfırla |

LCD ekranda aktif faz adı, kalan süre ve ilerleme çubuğu görüntülenir.

---

## Özellikler

- **Tam otomasyon:** Pompa → Karıştırma → Dağıtım sırası tek komutla çalışır
- **Acil durdurma:** Hem donanım butonu (Pin 18, interrupt) hem Serial `x` komutu ile anlık durdurma
- **LCD ilerleme çubuğu:** Her faz için gerçek zamanlı geri sayım ve doluluk çubuğu
- **Sensor timeout:** IR sensör 20 saniye içinde kap algılamazsa hata mesajıyla devam eder
- **Debounce:** IR sensör için yazılımsal titreşim giderme (15 ms)
- **Timer çakışması önlemi:** Rötari motoru Servo kütüphanesiyle aynı anda sorunsuz çalışır

---

## Medya

<!-- Fotoğraf ve videolarını buraya ekle -->

### Fotoğraflar

| | |
|---|---|
| ![Devre genel görünüm](media/foto1.jpg) | ![Motor sürücüler](media/foto2.jpg) |
| ![LCD ekran](media/foto3.jpg) | ![Makine genel](media/foto4.jpg) |

### Video

[![Çalışma videosu](https://img.shields.io/badge/▶_Video-YouTube-red?style=for-the-badge&logo=youtube)](https://youtube.com/LINK_BURAYA)

---

## Lisans

MIT License — dilediğiniz gibi kullanabilirsiniz.
