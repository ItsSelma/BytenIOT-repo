#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error "Ovaj kod je podržan samo za ESP32 ili ESP8266!"
#endif

#include <Firebase_ESP_Client.h>  // Firebase biblioteka
#include <addons/TokenHelper.h>   // Token helper za prikaz tokena
#include <addons/RTDBHelper.h>

#define RXPin 4  // RX (D2 na NodeMCU)
#define TXPin 5  // TX (D1 na NodeMCU)
#define GPSBaud 9600

TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);

#define WIFI_SSID "mojatv_full_7802"
#define WIFI_PASSWORD "123456123456"
#define API_KEY "AIzaSyDWNrTwpHeRH_EMPZoVyq370vfd8H0GkSk"
#define DATABASE_URL "byten-9bbb9-default-rtdb.europe-west1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

double homeSirina = 43.297331;
double homeDuzina = 17.846777;
double homeRadijus = 500.0; // Radijus u metrima
double homeNadmorskaVisina = 55.0; // Nadmorska visina kuće (u metrima)

// Promenljive za praćenje promene nadmorske visine
double prijasnjaNadmorska = 0.0;
bool JeLiSeNadmorskaPromjenila = false;

// Funkcija za izračunavanje udaljenosti između dve tačke u metrima
double distance(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000; // Radijus Zemlje u metrima
  double lat1Rad = lat1 * (PI / 180.0);
  double lon1Rad = lon1 * (PI / 180.0);
  double lat2Rad = lat2 * (PI / 180.0);
  double lon2Rad = lon2 * (PI / 180.0);

  double dlat = lat2Rad - lat1Rad;
  double dlon = lon2Rad - lon1Rad;

  double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1Rad) * cos(lat2Rad) * sin(dlon / 2) * sin(dlon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double distanca = R * c;
  return distanca;
}

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(GPSBaud);
  Serial.println("GPS sensor počinje prikupljati podatke...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Konektujemo se na Wi-Fi.. ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("...");
    delay(500);
//pinMode(LED_BUILTIN, OUTPUT);
  }

  Serial.println();
  Serial.print("Konektovano sa sledećim IP-jem: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("Nažalost, konekcija sa bazom nije uspešna!");
  }
}

void loop() {
/* if (Firebase.RTDB.getInt(&fbdo, "/gps/blink")) { // Čitanje blink vrijednosti iz Firebase
  if (fbdo.dataType() == "int") { // Provjera da li je podatak tipa int
    int BrojacLED = fbdo.intData(); // Dohvatanje vrijednosti iz Firebase

    if (BrojacLED % 2 == 1) { // Ako je broj neparan, trepni lampicom
      digitalWrite(LED_BUILTIN, HIGH); // Uključi LED lampicu
      delay(500);                      // Zadrži 500ms

      digitalWrite(LED_BUILTIN, LOW);  // Isključi LED lampicu
      delay(500);                      // Zadrži 500ms
    } else { // Ako je broj paran, ugasi lampicu
      digitalWrite(LED_BUILTIN, LOW); // Ugasi LED
    }
-----------------------------ovaj
/*
if (Firebase.RTDB.getInt(&fbdo, "/gps/blink")) {
  if (fbdo.dataType() == "int") {
    int BrojacLED = fbdo.intData();  // Dohvatanje vrijednosti iz Firebase
if (BrojacLED % 2 == 1) {
  digitalWrite(LED_BUILTIN, HIGH);  // Uključi LED
  delay(500);                      // Pauza od 500ms
  digitalWrite(LED_BUILTIN, LOW);   // Isključi LED
  delay(500);                      // Pauza od 500ms
} else {
  digitalWrite(LED_BUILTIN, LOW);  // Ugasi LED
}
if (Firebase.RTDB.setInt(&fbdo, "/gps/blinkStatus", BrojacLED % 2)) {
  Serial.println("Status poslan u Firebase.");
} else {
  Serial.println("Greška pri slanju statusa.");
  Serial.println(fbdo.errorReason());
}
*/

    // Ažuriraj stanje LED-a u Firebase
    if (Firebase.RTDB.setInt(&fbdo, "/gps/blinkStatus", BrojacLED % 2)) {
      Serial.println("Status poslan u Firebase.");
    } else {
      Serial.println("Greška pri slanju statusa.");
      Serial.println(fbdo.errorReason());
    }
  }
} */
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    if (gps.encode(c)) {
      if (gps.location.isUpdated()) {

        double TrenutnaSirina = gps.location.lat();
        double TrenutnaDuzina = gps.location.lng();

        // Izračunaj udaljenost od početne tačke
        double TrenutnaUdaljenost = distance(homeSirina, homeDuzina, TrenutnaSirina, TrenutnaDuzina);

        // Ispis trenutnih podataka
        Serial.print("Geografska širina: ");
        Serial.println(TrenutnaSirina, 6);
        Serial.print("Geografska dužina: ");
        Serial.println(TrenutnaDuzina, 6);

        // Dodana provera i ispis nadmorske visine
        if (gps.altitude.isValid()) {  // Koristi .isValid() umesto isUpdated()
          double nadmorskaVisina = gps.altitude.meters();
          Serial.print("Nadmorska visina u metrima: ");
          Serial.println(nadmorskaVisina);

          // Provera promene nadmorske visine
          
          if (prijasnjaNadmorska > 0 && (prijasnjaNadmorska - nadmorskaVisina) > 20) {
            Serial.println("Obavještenje: Nadmorska visina se smanjila za više od 20 metara!");
            
            // Slanje obaveštenja u Firebase
            if (Firebase.RTDB.setBool(&fbdo, "/gps/obavjestenje", true)) {
              Serial.println("Obaveštenje poslato u Firebase");
            } else {
              Serial.println("Greška prilikom slanja obaveštenja u Firebase");
              Serial.println(fbdo.errorReason());
            }
          }
  
          prijasnjaNadmorska = nadmorskaVisina; // Ažuriraj prethodnu nadmorsku visinu
        } else {
          Serial.println("Nadmorska visina nije dostupna.");
        }

        Serial.print("Sateliti: ");
        Serial.println(gps.satellites.value());
        Serial.print("HDOP: ");
        Serial.println(gps.hdop.value());
        Serial.print("Trenutna udaljenost od kuće: ");
        Serial.print(TrenutnaUdaljenost, 2);
        Serial.println(" m");

        // Provera geofencinga
        if (TrenutnaUdaljenost > homeRadijus) {
          Serial.println("Izlazak iz geofencinga! Slanje uputstva za povratak:");
          Serial.print("Povratak ka: Lat: ");
          Serial.print(homeSirina, 6);
          Serial.print(", Lon: ");
          Serial.println(homeDuzina, 6);
        } else {
          Serial.println("Unutar geofencinga.");
        }

        // Slanje podataka u Firebase kada je Firebase spreman
        if (Firebase.ready() && signupOK) {
          // Slanje trenutnih koordinata u Firebase
          if (Firebase.RTDB.setDouble(&fbdo, "/gps/sirina", TrenutnaSirina)) {
            Serial.println("Geografska širina poslana u Firebase");
          } else {
            Serial.println("Greška prilikom slanja geografske širine u Firebase");
            Serial.println(fbdo.errorReason());
          }

          if (Firebase.RTDB.setDouble(&fbdo, "/gps/duzina", TrenutnaDuzina)) {
            Serial.println("Geografska dužina poslana u Firebase");
          } else {
            Serial.println("Greška prilikom slanja geografske u Firebase");
            Serial.println(fbdo.errorReason());
          }

          // Slanje udaljenosti od kuće u Firebase
          if (Firebase.RTDB.setDouble(&fbdo, "/gps/udaljenost", TrenutnaUdaljenost)) {
            Serial.println("Udaljenost od kuće poslana u Firebase");
          } else {
            Serial.println("Greška prilikom slanja udaljenosti u Firebase");
            Serial.println(fbdo.errorReason());
          }

          // Slanje nadmorske visine u Firebase ako je dostupna
          if (gps.altitude.isValid()) {
            double nadmorskaVisina = gps.altitude.meters();
            if (Firebase.RTDB.setDouble(&fbdo, "/gps/nadmorskaVisina", nadmorskaVisina)) {
              Serial.println("Nadmorska visina poslana u Firebase");
            } else {
              Serial.println("Greška prilikom slanja nadmorske visine u Firebase");
              Serial.println(fbdo.errorReason());
            }
          }

          // Provera geofencinga i nadmorske visine za /provjera
          bool geofencingStatus = TrenutnaUdaljenost <= homeRadijus;  // True ako je unutar geofencinga
          bool nadmorskaStatus = (gps.altitude.isValid() && (gps.altitude.meters() >= homeNadmorskaVisina - 20));  // True ako nije 20m ispod

          // Slanje podataka u /provjera
          if (Firebase.RTDB.setBool(&fbdo, "/provjera/geofencing", geofencingStatus)) {
            Serial.println("Status geofencinga poslan u Firebase");
          } else {
            Serial.println("Greška prilikom slanja statusa geofencinga u Firebase");
            Serial.println(fbdo.errorReason());
          }

          if (Firebase.RTDB.setBool(&fbdo, "/provjera/nadmorskaVisina", nadmorskaStatus)) {
            Serial.println("Status nadmorske visine poslan u Firebase");
          } else {
            Serial.println("Greška prilikom slanja statusa nadmorske visine u Firebase");
            Serial.println(fbdo.errorReason());
          }
        }
      }
    }
  }
}
