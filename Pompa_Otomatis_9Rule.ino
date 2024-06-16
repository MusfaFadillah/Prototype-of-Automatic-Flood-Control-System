#include "arduino_secrets.h"

//Blynk
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6oCSY4ILO"
#define BLYNK_TEMPLATE_NAME "Rumah Pompa Otomatis"
#define BLYNK_AUTH_TOKEN "Xbc7-MGkc1uaZlMDf8g6xT_nJGvjmN6b"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "XxX";
char pass[] = "12345678";

// Sensor DHT11
#include <DHT.h>
#define dhtPin 14 // (digital) untuk membaca nilai suhu
#define dhtType DHT11
DHT dht(dhtPin, dhtType);

// Sensor HC-SR04
#define trigPin 12 // (digital) sebagai output untuk mengirimkan sinyal atau gelombang suara
#define echoPin 13 // (digital) sebagai input untuk membaca nilai durasi waktu sinyal untuk menghitung jarak 

// // Driver L298N
// #define enA 11 // (PWM) sebagai output untuk mengatur kecepatan motor (0 - 255)
// #define in1 10 // (digital) sebagai output untuk mengatur arah putaran motor (LOW/HIGH)
// #define in2 9 // (digital) sebagai output untuk mengatur arah putaran motor (LOW/HIGH)
// // Jika in1 = LOW & in2 = HIGH motor berputar searah jarum jam, maka ketika in1 = HIGH & in2 = LOW motor akan berputar berlawanan arah jarum jam
// // Begitu juga sebaliknya

// -------------------------------------------------------------------------------------------------------------------------------------------------

// Logika Fuzzy
#include <Fuzzy.h>
Fuzzy *fuzzy = new Fuzzy();

// FuzzyInput suhu
FuzzySet *hujan = new FuzzySet(0, 0, 20, 25);
FuzzySet *gerimis = new FuzzySet(20, 25, 25, 30);
FuzzySet *tidakHujan = new FuzzySet(25, 30, 40, 40);

// FuzzyInput ketinggian
FuzzySet *rendah = new FuzzySet(-10, -10, 0, 10);
FuzzySet *sedang = new FuzzySet(0, 10, 10, 20);
FuzzySet *tinggi = new FuzzySet(10, 20, 30, 30);

// FuzzyOutput kecepatan motor
FuzzySet *mati = new FuzzySet(-10, -10, 0, 128);
FuzzySet *pelan = new FuzzySet(0, 128, 128, 255);
FuzzySet *cepat = new FuzzySet(128, 255, 300, 300);

// -------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  // Put your setup code here, to run once:
  Serial.begin(9600); // Untuk membuat serial monitor

  //Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Sensor DHT11
  dht.begin(); // Untuk memulai library DHT.h

  // Sensor HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // // Driver L298N
  // pinMode(enA, OUTPUT);
  // pinMode(in1, OUTPUT);
  // pinMode(in2, OUTPUT);

  // -----------------------------------------------------------------------------------------------------------------------------------------------
  
  // FuzzyInput suhu
  FuzzyInput *suhu = new FuzzyInput(1);
  suhu->addFuzzySet(hujan);
  suhu->addFuzzySet(gerimis);
  suhu->addFuzzySet(tidakHujan);
  fuzzy->addFuzzyInput(suhu);

  // FuzzyInput ketinggian
  FuzzyInput *ketinggian = new FuzzyInput(2);
  ketinggian->addFuzzySet(rendah);
  ketinggian->addFuzzySet(sedang);
  ketinggian->addFuzzySet(tinggi);
  fuzzy->addFuzzyInput(ketinggian);

  // FuzzyOutput kecepatan motor
  FuzzyOutput *kecepatan = new FuzzyOutput(1);
  kecepatan->addFuzzySet(mati);
  kecepatan->addFuzzySet(pelan);
  kecepatan->addFuzzySet(cepat);
  fuzzy->addFuzzyOutput(kecepatan);

  // FuzzyRule 1
  FuzzyRuleAntecedent *tidakHujan_rendah = new FuzzyRuleAntecedent();
  tidakHujan_rendah->joinWithAND(tidakHujan, rendah);
  FuzzyRuleConsequent *thenMati1 = new FuzzyRuleConsequent();
  thenMati1->addOutput(mati);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, tidakHujan_rendah, thenMati1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // FuzzyRule 2
  FuzzyRuleAntecedent *tidakHujan_sedang = new FuzzyRuleAntecedent();
  tidakHujan_sedang->joinWithAND(tidakHujan, sedang);
  FuzzyRuleConsequent *thenMati2 = new FuzzyRuleConsequent();
  thenMati2->addOutput(mati);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, tidakHujan_sedang, thenMati2);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // FuzzyRule 3
  FuzzyRuleAntecedent *tidakHujan_tinggi = new FuzzyRuleAntecedent();
  tidakHujan_tinggi->joinWithAND(tidakHujan, tinggi);
  FuzzyRuleConsequent *thenPelan1 = new FuzzyRuleConsequent();
  thenPelan1->addOutput(pelan);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, tidakHujan_tinggi, thenPelan1);
  fuzzy->addFuzzyRule(fuzzyRule3);

  // FuzzyRule 4
  FuzzyRuleAntecedent *gerimis_rendah = new FuzzyRuleAntecedent();
  gerimis_rendah->joinWithAND(gerimis, rendah);
  FuzzyRuleConsequent *thenMati3 = new FuzzyRuleConsequent();
  thenMati3->addOutput(mati);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, gerimis_rendah, thenMati3);
  fuzzy->addFuzzyRule(fuzzyRule4);

  // FuzzyRule 5
  FuzzyRuleAntecedent *gerimis_sedang = new FuzzyRuleAntecedent();
  gerimis_sedang->joinWithAND(gerimis, sedang);
  FuzzyRuleConsequent *thenPelan2 = new FuzzyRuleConsequent();
  thenPelan2->addOutput(pelan);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, gerimis_sedang, thenPelan2);
  fuzzy->addFuzzyRule(fuzzyRule5);

  // FuzzyRule 6
  FuzzyRuleAntecedent *gerimis_tinggi = new FuzzyRuleAntecedent();
  gerimis_tinggi->joinWithAND(gerimis, tinggi);
  FuzzyRuleConsequent *thenCepat1 = new FuzzyRuleConsequent();
  thenCepat1->addOutput(cepat);
  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, gerimis_tinggi, thenCepat1);
  fuzzy->addFuzzyRule(fuzzyRule6);

  // FuzzyRule 7
  FuzzyRuleAntecedent *hujan_rendah = new FuzzyRuleAntecedent();
  hujan_rendah->joinWithAND(hujan, rendah);
  FuzzyRuleConsequent *thenPelan3 = new FuzzyRuleConsequent();
  thenPelan3->addOutput(pelan);
  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, hujan_rendah, thenPelan3);
  fuzzy->addFuzzyRule(fuzzyRule7);

  // FuzzyRule 8
  FuzzyRuleAntecedent *hujan_sedang = new FuzzyRuleAntecedent();
  hujan_sedang->joinWithAND(hujan, sedang);
  FuzzyRuleConsequent *thenCepat2 = new FuzzyRuleConsequent();
  thenCepat2->addOutput(cepat);
  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, hujan_sedang, thenCepat2);
  fuzzy->addFuzzyRule(fuzzyRule8);

  // FuzzyRule 9
  FuzzyRuleAntecedent *hujan_tinggi = new FuzzyRuleAntecedent();
  hujan_tinggi->joinWithAND(hujan, tinggi);
  FuzzyRuleConsequent *thenCepat3 = new FuzzyRuleConsequent();
  thenCepat3->addOutput(cepat);
  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, hujan_tinggi, thenCepat3);
  fuzzy->addFuzzyRule(fuzzyRule9);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
  // Put your main code here, to run repeatedly:

  // Sensor DHT11
  float in_suhu; // Suhu dalam variabel float karena mengandung desimal
  in_suhu = dht.readTemperature(); // Untuk membaca suhu dan menyimpannya ke variabel suhu
  // Untuk input suhu secara manual tanpa sensor, jika tidak maka jadikan comment
  // in_suhu = 22;
  // Print nilai suhu
  Serial.print("Suhu = ");
  Serial.print(in_suhu);
  Serial.println(" Â°C");


  // Sensor HC-SR04
  long durasi; // Variabel untuk menyimpan durasi pulsa ultrasonik (dengan long karena durasi dalam satuan mikrodetik sehingga butuh angka yang panjang)
  float jarak, in_ketinggian; // Variabel untuk menyimpan jarak dan ketinggian air (dengan float agar bisa menghasilkan angka desimal)
  // Kirim pulsa ultrasonik
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Baca pulsa kembali dari sensor
  durasi = pulseIn(echoPin, HIGH); // Untuk membaca pulsa input dalam mikrodetik dan disimpan di variabel durasi
  // Menghitung jarak berdasarkan durasi
  // Dengan menggunakan kecepatan suara yaitu 0.034 cm / mikro second
  jarak = (durasi * 0.034) / 2;  // Rumus: jarak = (durasi * kecepatan suara) / 2
  // Dengan menggunakan jarak untuk menghitung ketinggian air
  in_ketinggian = 20 - jarak;
  // Untuk input ketinggian secara manual tanpa sensor, jika tidak maka jadikan comment
  // in_ketinggian = 10;
  // Print nilai ketinggian air
  Serial.print("Ketinggian = ");
  Serial.print(in_ketinggian);
  Serial.println(" cm");


  // Logika Fuzzy
  fuzzy->setInput(1, in_suhu);
  fuzzy->setInput(2, in_ketinggian);
  // Running the Fuzzification
  fuzzy->fuzzify();
  // Running the Defuzzification
  float kecepatan = fuzzy->defuzzify(1);

  // Print nilai kecepatan motor
  Serial.print("Kecepatan = ");
  Serial.println(kecepatan);


  // // Driver L298N
  // analogWrite(enA, kecepatan);
  // digitalWrite(in1, HIGH);
  // digitalWrite(in2, LOW);

  //Blynk
  Blynk.run();
  //Ketinggian Air
  Blynk.virtualWrite(V0, in_ketinggian);
  //Suhu Udara
  Blynk.virtualWrite(V1, in_suhu);
  //Kecepatan Pompa
  Blynk.virtualWrite(V2, kecepatan);

  delay(1500);
}

  /*
  !!!!! Catatan penting ketika upload program lepaskan dulu sensor ultrasonik (cukup pin trigger dan pin echonya saja
  tidak apa-apa yang penting sensor ultrasoniknya sudah tidak terhubung ke esp32 nya) dan ketika mengupload atau
  menjalankan program sebaiknya jika ada alat atau sensor yang tidak dipasang atau tidak digunakan lebih baik bagian
  program dari alat tersebut dijadikan komen terlebih dahulu karena dapat menyebabkan error !!!!!
  */

  /*
  Jika ingin membuat atau mengubah secara drastis suatu program atau sistem lebih baik periksa setiap bagian alat dan
  program terlebih dahulu untuk mengecek apakah ada masalah pada salah satu bagian alat atau program karena jika
  semuanya langsung dijalankan sekaligus akan menyebabkan kebingungan dalam melakukan troubleshooting dan menemukan
  bagian mana yang bermasalah jika terjadi suatu error atau permasalahan
  */