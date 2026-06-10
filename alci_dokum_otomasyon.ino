/* ============================================================
    ALCI DOKUM MAKINESI - OTOMASYON (GÜNCELLENDİ)
    Arduino Mega 2560 + 2x L298N + Servo + IR Sensor + 16x2 I2C LCD
     ============================================================ */

  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  #include <Servo.h>

  // ---------- LCD ----------
  LiquidCrystal_I2C lcd(0x27, 16, 2);

  // ---------- SERVO ----------
  Servo servo;
  #define SERVO_PIN     8
  #define SERVO_KAPALI  25    // baslangic / kapali aci
  #define SERVO_ACIK    110   // dagitim acisi
  #define SERVO_BEKLE   1000  // 110'da bekleme suresi (ms)

  // ---------- IR SENSOR ----------
  #define SENSOR_PIN    30
  #define SENSOR_AKTIF  LOW

  // ---------- ACIL DURDURMA ----------
  #define ESTOP_PIN     18
  volatile bool acilDurum = false;

  // ---------- L298N #1 (GK #1) : Bant + Rotari ----------
  // BANT   -> OUT1/OUT2 (kanal A)
  #define BANT_IN1   22
  #define BANT_IN2   23
  #define BANT_EN    2     // ENA (PWM)
  // ROTARI -> OUT3/OUT4 (kanal B)
  #define ROTARI_IN1 24    // fiziksel IN3
  #define ROTARI_IN2 25    // fiziksel IN4
  #define ROTARI_EN  3     // ENB (PWM)

  // ---------- L298N #2 (GK #2) : Karistirici + Pompa ----------
  #define KARIS_IN1  26
  #define KARIS_IN2  27
  #define KARIS_EN   4     // ENA (PWM)
  #define POMPA_IN1  28    // fiziksel IN3
  #define POMPA_IN2  29    // fiziksel IN4
  #define POMPA_EN   5     // ENB (PWM)

  // ---------- HIZLAR (0-255) ----------
  // Karıştırıcı kalkışta kilitlenmesin ve servo zamanlayıcısından etkilenmesin diye 255 (Tam Hız) yapıldı.
  #define HIZ_POMPA        191   // 3/4 hiz
  #define HIZ_ROTARI       255   // tam hiz
  #define HIZ_KARISTIRICI  191   // 3/4 hiz
  #define HIZ_BANT         191   // 3/4 hiz

  // ---------- SURELER (ms) ----------
  #define POMPA_SURE        35000UL
  #define ROTARI_SURE        8000UL
  #define KARISTIRICI_SURE  10000UL
  #define BANT_DURMA         3000UL   
  #define SON_BANT_SURE      3000UL   
  #define BANT_TIMEOUT      20000UL   

  #define KAP_SAYISI 2

  // ---------- LCD ozel karakterler ----------
  byte g1[8] = {B10000,B10000,B10000,B10000,B10000,B10000,B10000,B10000};
  byte g2[8] = {B11000,B11000,B11000,B11000,B11000,B11000,B11000,B11000};
  byte g3[8] = {B11100,B11100,B11100,B11100,B11100,B11100,B11100,B11100};
  byte g4[8] = {B11110,B11110,B11110,B11110,B11110,B11110,B11110,B11110};
  byte g5[8] = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};

  // ============================================================
  //   ACIL DURDURMA
  // ============================================================
  void acilISR() {
    acilDurum = true;
    digitalWrite(ROTARI_EN, LOW);  digitalWrite(BANT_EN, LOW);
    digitalWrite(KARIS_EN, LOW);   digitalWrite(POMPA_EN, LOW);
    digitalWrite(ROTARI_IN1, LOW); digitalWrite(ROTARI_IN2, LOW);
    digitalWrite(BANT_IN1, LOW);   digitalWrite(BANT_IN2, LOW);
    digitalWrite(KARIS_IN1, LOW);  digitalWrite(KARIS_IN2, LOW);
    digitalWrite(POMPA_IN1, LOW);  digitalWrite(POMPA_IN2, LOW);
  }

  bool acilKontrol() {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == 'x' || c == 'X') acilDurum = true;
    }
    return acilDurum;
  }

  void bekle(unsigned long ms) {
    unsigned long b = millis();
    while (millis() - b < ms) {
      if (acilKontrol()) return;
    }
  }

  // ============================================================
  //   YARDIMCI FONKSIYONLAR
  // ============================================================
  void motorBaslat(int in1, int in2, int en, int hiz) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    
    // Eger tam hiz istenmisse PWM sinyali yerine dogrudan HIGH vererek
    // Servo kütüphanesinin olasi Timer çakışmalarını bypass ediyoruz.
    if (hiz >= 255) {
      digitalWrite(en, HIGH);
    } else {
      analogWrite(en, hiz);
    }
  }

  void motorDurdur(int in1, int in2, int en) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(en, LOW);
  }

  void tumMotorlariDurdur() {
    motorDurdur(ROTARI_IN1, ROTARI_IN2, ROTARI_EN);
    motorDurdur(BANT_IN1,   BANT_IN2,   BANT_EN);
    motorDurdur(KARIS_IN1,  KARIS_IN2,  KARIS_EN);
    motorDurdur(POMPA_IN1,  POMPA_IN2,  POMPA_EN);
  }

  void cubukCiz(int dolu) {
    int tam   = dolu / 5;
    int kalan = dolu % 5;
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++) {
      if (i < tam)                       lcd.write((uint8_t)5);      
      else if (i == tam && kalan > 0)    lcd.write((uint8_t)kalan);   
      else                               lcd.write(' ');              
    }
  }

  void saniyeYaz(int sn) {
    char buf[6];
    sprintf(buf, "%3ds", sn);
    lcd.setCursor(12, 0);
    lcd.print(buf);
  }

  void zamanliFaz(const char* ad, unsigned long sureMs,
                  int in1, int in2, int en, int hiz) {
    Serial.print(F(">> ")); Serial.print(ad);
    Serial.print(F(" (")); Serial.print(sureMs / 1000); Serial.println(F(" sn)"));

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(ad);

    motorBaslat(in1, in2, en, hiz);

    unsigned long bas = millis();
    int sonSn = -1, sonDolu = -1;
    while (millis() - bas < sureMs) {
      if (acilKontrol()) { motorDurdur(in1, in2, en); return; }   
      unsigned long gecen = millis() - bas;
      int kalanSn = (int)((sureMs - gecen + 999) / 1000);
      if (kalanSn != sonSn) { sonSn = kalanSn; saniyeYaz(kalanSn); }
      int dolu = (int)((float)gecen / sureMs * 80.0 + 0.5);
      if (dolu != sonDolu) { sonDolu = dolu; cubukCiz(dolu); }
    }
    cubukCiz(80);
    saniyeYaz(0);
    motorDurdur(in1, in2, en);
  }

  bool bantBekleVeAlgila(int kap) {
    Serial.print(F(">> Bant calisiyor, kap "));
    Serial.print(kap); Serial.println(F(" bekleniyor..."));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kap "); lcd.print(kap); lcd.print(" bekleniyor");

    motorBaslat(BANT_IN1, BANT_IN2, BANT_EN, HIZ_BANT);

    unsigned long bas = millis(), sonAnim = 0;
    int frame = 0;
    bool temizBekle = true;   

    while (true) {
      if (acilKontrol()) { motorDurdur(BANT_IN1, BANT_IN2, BANT_EN); return false; }  

      bool aktif = (digitalRead(SENSOR_PIN) == SENSOR_AKTIF);

      if (temizBekle) {
        if (!aktif) temizBekle = false;          
      } else if (aktif) {
        delay(15);                               // debounce
        if (digitalRead(SENSOR_PIN) == SENSOR_AKTIF) {
          motorDurdur(BANT_IN1, BANT_IN2, BANT_EN);
          Serial.print(F(">> Kap ")); Serial.print(kap); Serial.println(F(" algilandi."));
          return true;
        }
      }

      if (millis() - bas > BANT_TIMEOUT) {
        motorDurdur(BANT_IN1, BANT_IN2, BANT_EN);
        Serial.println(F("!! Sensor zaman asimi - sensoru/ayari kontrol et."));
        return false;
      }

      if (millis() - sonAnim > 120) {            
        sonAnim = millis();
        lcd.setCursor(0, 1);
        for (int i = 0; i < 16; i++)
          lcd.write(i == (frame % 16) ? (uint8_t)5 : ' ');
        frame++;
      }
    }
  }

  void dagitimFazi(int kap) {
    Serial.print(F(">> Kap ")); Serial.print(kap); Serial.println(F(" dagitiliyor (servo)"));
    motorDurdur(BANT_IN1, BANT_IN2, BANT_EN);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kap "); lcd.print(kap); lcd.print(" Dagitim");

    servo.write(SERVO_ACIK);            
    unsigned long bas = millis();
    bool kapandi = false;
    int sonSn = -1, sonDolu = -1;

    while (millis() - bas < BANT_DURMA) {
      if (acilKontrol()) { servo.write(SERVO_KAPALI); return; }   
      unsigned long gecen = millis() - bas;
      if (!kapandi && gecen >= SERVO_BEKLE) { servo.write(SERVO_KAPALI); kapandi = true; }
      int kalanSn = (int)((BANT_DURMA - gecen + 999) / 1000);
      if (kalanSn != sonSn) { sonSn = kalanSn; saniyeYaz(kalanSn); }
      int dolu = (int)((float)gecen / BANT_DURMA * 80.0 + 0.5);
      if (dolu != sonDolu) { sonDolu = dolu; cubukCiz(dolu); }
    }
    if (!kapandi) servo.write(SERVO_KAPALI);
    cubukCiz(80);
  }

  void sensorUyari() {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("! Sensor gelmedi");
    lcd.setCursor(0, 1); lcd.print("Devam ediliyor");
    bekle(1500);
  }

  void beklemeEkrani() {
    tumMotorlariDurdur();
    servo.write(SERVO_KAPALI);
    lcd.clear();
    lcd.setCursor(3, 0); lcd.print("ALCI DOKUM");
    lcd.setCursor(0, 1); lcd.print("Basla:'s' Stop:x");
    Serial.println(F("\nHAZIR. 's'=baslat | 't'=sensor testi | 'x'=acil stop"));
  }

  void acilDurumIsle() {
    tumMotorlariDurdur();
    servo.write(SERVO_KAPALI);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(" !! ACIL STOP !!");
    lcd.setCursor(0, 1); lcd.print("Sifirla: 'r'");
    Serial.println(F("\n!!!!! ACIL DURDURMA !!!!! Tum motorlar kesildi."));
    Serial.println(F("Butonu birak ve 'r' gonder (sistem sifirlanir)."));

    while (true) {                 
      tumMotorlariDurdur();        
      if (digitalRead(ESTOP_PIN) == HIGH && Serial.available()) {
        char c = Serial.read();
        while (Serial.available()) Serial.read();
        if (c == 'r' || c == 'R') break;
      }
    }
    acilDurum = false;
    Serial.println(F(">> Sistem sifirlandi."));
    beklemeEkrani();
  }

  // ============================================================
  //   ANA SISTEM DONGUSU
  // ============================================================
  void calistirSistem() {
    Serial.println(F("\n##### SISTEM BASLADI #####"));

    // --- HAZIRLIK (bir kez) ---
    zamanliFaz("Pompa",    POMPA_SURE,       POMPA_IN1,  POMPA_IN2,  POMPA_EN,  HIZ_POMPA);        if (acilDurum) return;
    zamanliFaz("Rotari",   ROTARI_SURE,      ROTARI_IN1, ROTARI_IN2, ROTARI_EN, HIZ_ROTARI);       if (acilDurum) return;
    zamanliFaz("Karistir", KARISTIRICI_SURE, KARIS_IN1,  KARIS_IN2,  KARIS_EN,  HIZ_KARISTIRICI);  if (acilDurum) return;

    // --- DAGITIM (2 kap, art arda) ---
    for (int kap = 1; kap <= KAP_SAYISI; kap++) {
      bool ok = bantBekleVeAlgila(kap);   if (acilDurum) return;
      if (!ok) sensorUyari();             if (acilDurum) return;
      dagitimFazi(kap);                   if (acilDurum) return;
    }

    // --- SON ---
    zamanliFaz("Bitiriliyor", SON_BANT_SURE, BANT_IN1, BANT_IN2, BANT_EN, HIZ_BANT);  if (acilDurum) return;

    tumMotorlariDurdur();
    servo.write(SERVO_KAPALI);
    lcd.clear();
    lcd.setCursor(1, 0); lcd.print("TAMAMLANDI :)");
    Serial.println(F("\n##### SISTEM TAMAMLANDI #####"));
    bekle(2500);
    if (acilDurum) return;
    beklemeEkrani();
  }

  void sensorTest() {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Sensor Testi");
    Serial.println(F("\n-- SENSOR TESTI -- Elini sensorun onune getir."));
    Serial.println(F("Cikis icin herhangi bir karakter gonder."));
    while (!Serial.available()) {
      if (acilDurum) return;
      int v = digitalRead(SENSOR_PIN);
      lcd.setCursor(0, 1);
      lcd.print(v == HIGH ? "Okuma: HIGH (1) " : "Okuma: LOW  (0) ");
      Serial.print(F("Pin30 = ")); Serial.println(v);
      delay(200);
    }
    while (Serial.available()) Serial.read();
    beklemeEkrani();
  }

  // ============================================================
  //   SETUP / LOOP
  // ============================================================
  void setup() {
    Serial.begin(9600);

    int cikislar[] = {
      ROTARI_IN1, ROTARI_IN2, ROTARI_EN,
      BANT_IN1,   BANT_IN2,   BANT_EN,
      KARIS_IN1,  KARIS_IN2,  KARIS_EN,
      POMPA_IN1,  POMPA_IN2,  POMPA_EN
    };
    for (unsigned int i = 0; i < sizeof(cikislar) / sizeof(cikislar[0]); i++)
      pinMode(cikislar[i], OUTPUT);

    pinMode(SENSOR_PIN, INPUT_PULLUP);

    pinMode(ESTOP_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), acilISR, FALLING);

    servo.attach(SERVO_PIN);
    servo.write(SERVO_KAPALI);

    lcd.init();
    lcd.backlight();
    lcd.createChar(1, g1);
    lcd.createChar(2, g2);
    lcd.createChar(3, g3);
    lcd.createChar(4, g4);
    lcd.createChar(5, g5);

    tumMotorlariDurdur();
    beklemeEkrani();
  }

  void loop() {
    if (acilDurum) { acilDurumIsle(); return; }

    if (Serial.available()) {
      char c = Serial.read();
      while (Serial.available()) Serial.read();   
      if      (c == 's' || c == 'S') calistirSistem();
      else if (c == 't' || c == 'T') sensorTest();
      else if (c == 'x' || c == 'X') acilDurum = true;
    }
  }