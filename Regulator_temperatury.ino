//inicjalizacja bibliotek
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DS18B20.h>
#include <EEPROM.h>

// Ustawienie parametrów wyświetlacza dla komunikacji I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

//zmienne globalne
int temp = 0;
int temp2 = 0;
int level = 0;
//zmienna odczytująca wartość wskazanych adresów EEPROM
//po ponownym uruchomieniu program - po zaniku zasilania
//zmienna przechowuje nastawy temperatury załaczania
long int reference[6] = {
  EEPROM.read(0),
  EEPROM.read(1),
  EEPROM.read(2),
  EEPROM.read(3),
  EEPROM.read(4),
  EEPROM.read(5),
};
int actual_temp[6] = {0, 0, 0, 0, 0, 0};
int sensor_value = 0;
int powered[6] = {0, 0, 0, 0, 0, 0};
//zmienna odczytująca wartość wskazanych adresów EEPROM
//po ponownym uruchomieniu program - po zaniku zasilania
//zmienna przechowuje nastawy histerezy
int histeresis[6] = {
  EEPROM.read(6),
  EEPROM.read(7),
  EEPROM.read(8),
  EEPROM.read(9),
  EEPROM.read(10),
  EEPROM.read(11)
};
int temp_hist = 0;
//zmienne globalne do pamiętania stanu przycisków
bool button_up = 0;
bool last_up = 0;
bool button_down = 0;
bool last_down = 0;
int button_ok = 0;
bool last_ok = 0;
//deklaracja numerów pinów przycisków
const int UP = 9;
const int DOWN = 10;
const int OK = 11;
//deklaracja adresów czujników
const byte address[SENSORS_NUM][8]PROGMEM = {
  0x28, 0xA6, 0x9F, 0x7, 0xD6, 0x1, 0x3C, 0xBF,
  0x28, 0x4F, 0x55, 0x7, 0xD6, 0x1, 0x3C, 0x9F,
  0x28, 0x8F, 0xC8, 0x7, 0xD6, 0x1, 0x3C, 0x5B,
  0x28, 0xD3, 0xC9, 0x7, 0xD6, 0x1, 0x3C, 0xC5,
  0x28, 0x25, 0x82, 0x7, 0xD6, 0x1, 0x3C, 0x9C,
  0x28, 0xC, 0x2F, 0x7, 0xD6, 0x1, 0x3C, 0x63
};

//deklaracja numeru pinu dla czujników OneWire
#define ONEWIRE_PIN 2
//deklaracja ilości czujników
#define SENSORS_NUM 6

//przyporządkowanie interfejsu OneWire do komunikacji z urządzeniem zewnętrznym
OneWire onewire(ONEWIRE_PIN);
DS18B20 sensors(&onewire);

void setup() {
  //inicjalizacje
  Serial.begin(9600);
  lcd.init();
  lcd.begin(16, 2);
  lcd.clear();
  sensors.begin();
  sensors.request();
  //konfiguracja wyprowadzeń jako wejścia lub wyjścia
  //piny dla przycisków
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);
  pinMode(OK, INPUT);
  //piny dla przekaźników
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  // funkcje zabezpieczeniowe
  for (int i = 0; i < 6; i++) {
    if (reference[i] == 255) {
      reference[i] = 60;
    }
  }
  for (int i = 0; i < 6; i++) {
    if (histeresis[i] == 255) {
      histeresis[i] = 5;
    }
  }
}
void loop() {
  //wywołanie funkcji menu
  menu();
}

//funkcja menu użytkownika
void menu() {
  //wywołanie fukcji odczytującej aktualną wartość temperatury, załączająca przekaźniki
  relay();
  //zerowanie zmiennych odpoweidzianych za obsługę stanów przycisków
  button_up = 0;
  button_down = 0;
  button_ok = 0;

  // sprawdzanie stanu przycisku OK i czy był on podtrzymywany dłużej
  if (digitalRead(OK) == HIGH) {
    if (last_ok == 0) {
      button_ok = 1;
    }
    delay(500);
    if ((digitalRead(OK) == HIGH) && (last_ok == 0)) {
      button_ok = 2;
    }
    last_ok = 1;
  }
  else last_ok = 0;
  //przypisanie akcji dla krótkiego i długiego naciśnięcia przycisku
  if (button_ok == 1) {
    level = level + 1;
    if ((temp == 2 || temp == 3) && level > 3) {
      level = 3;
    }
    lcd.clear();
    if ((temp != 2 && temp != 3) && level > 2) {
      level = 2;
    }
  }
  if (button_ok == 2) {
    level = level - 1;
    lcd.clear();
    if (level < 0) {
      level = 0;
    }
  }

  // sprawdzanie zbocza narastajacego przycisku UP
  if (digitalRead(UP) == HIGH) {
    if (last_up == 0) {
      button_up = 1;
    }
    last_up = 1;
  }
  else last_up = 0;

  // sprawdzanie zbocza narastajacego przycisku DOWN
  if (digitalRead(DOWN) == HIGH) {
    if (last_down == 0) {
      button_down = 1;
    }
    last_down = 1;
  }
  else last_down = 0;

  // okeślenie jaki poziom menu głównego ma być wyświetlony
  //poziom 0 - wygaszenie wyświetlacza
  if (level == 0) {
    lcd.noBacklight();
  }
  //wywołanie funkcji odpowiedzialnej za obsługę poziomu 1
  if (level == 1) {
    lcd.backlight();
    temp2 = 0;
    level_1(button_up, button_down);
  }
  //wywołanie funkcji odpowiedzialnej za obsługę poziomu 2
  if (level == 2) {
    level_2(button_up, button_down);
  }
  //wywołanie funkcji odpowiedzialnej za obsługę poziomu 3
  if (level == 3) {
    level_3(button_up, button_down, button_ok);
  }
}

//funkcje główne menu
void level_1(bool up, bool down) {
  //czyszczenie ekranu w momencie zmiany stanu przycisków
  if (up == 1 || down == 1) {
    lcd.clear();
  }
  //zmiana zmiennej w zależności od przycisków UP i DOWN i jej zabezpieczenia
  temp = temp - up + down;
  if (temp > 3) {
    temp = 0;
  }
  if (temp < 0) {
    temp = 2;
  }

  //sprawdzenie zmiennej i wyświetlenie odpowieniej informacji na wyświetlaczu lcd
  if (temp == 0) {
    arrows();
    lcd.setCursor(4, 0);
    lcd.print("Aktualna");
    lcd.setCursor(2, 1);
    lcd.print("temperatura");
    delay (100);
  }
  else if (temp == 1) {
    arrows();
    lcd.setCursor(3, 0);
    lcd.print("Zalaczone");
    lcd.setCursor(2, 1);
    lcd.print("przekazniki");
    delay (100);
  }
  else if (temp == 2) {
    arrows();
    lcd.setCursor(2, 0);
    lcd.print("Temperatura");
    lcd.setCursor(2, 1);
    lcd.print("zalaczajaca");
    delay (100);
  }
  else if (temp == 3) {
    arrows();
    lcd.setCursor(2, 0);
    lcd.print("Temperatura");
    lcd.setCursor(2, 1);
    lcd.print("wylaczajaca");
    delay (100);
  }
}
void level_2(bool up, bool down) {
  //czyszczenie ekranu w momencie zmiany stanu przycisków
  if (up == 1 || down == 1) {
    lcd.clear();
  }
  temp2 = temp2 - up + down;
  //sprawdzenie zmiennej i wyświetlenie odpowieniej informacji na wyświetlaczu lcd
  if (temp == 0) {
    if (temp2 > 5) {
      temp2 = 0;
    }
    if (temp2 < 0) {
      temp2 = 5;
    }
    actual(temp2);
    choice((temp2 % 2));
  }
  if (temp == 1) {
    if (temp2 > 1) {
      temp2 = 0;
    }
    if (temp2 < 0) {
      temp2 = 0;
    }
    zalaczone();
  }
  if (temp == 2) {
    if (temp2 > 5) {
      temp2 = 0;
    }
    if (temp2 < 0) {
      temp2 = 5;
    }
    nastawy(temp2);
    choice((temp2 % 2));
    sensor_value = reference[temp2];
  }
  if (temp == 3) {
    if (temp2 > 5) {
      temp2 = 0;
    }
    if (temp2 < 0) {
      temp2 = 5;
    }
    wylaczajaca(temp2);
    choice((temp2 % 2));
    temp_hist = histeresis[temp2];
  }
}
void level_3(bool up, bool down, int ok) {
  if (up == 1 || down == 1 || ok == 1) {
    lcd.clear();
  }

  if (temp == 2) {
    sensor_value = sensor_value + up - down;
    // wyświetlenie informacji o aktualnej nastawie w pierwszym wierszu wyświetlacza
    lcd.setCursor(0, 0);
    lcd.print("Aktualna");
    lcd.setCursor(10, 0);
    lcd.print(reference[temp2]);
    lcd.setCursor(14, 0);
    lcd.write(223);
    lcd.setCursor(15, 0);
    lcd.print("C");
    // wyświetlenie informacji o nowej wartości nastawy w drugim wierszu wyświetlacza
    lcd.setCursor(0, 1);
    lcd.print("Nowa");
    lcd.setCursor(10, 1);
    lcd.print(sensor_value);
    lcd.setCursor(14, 1);
    lcd.write(223);
    lcd.setCursor(15, 1);
    lcd.print("C");
    //wpisanie nowych nastaw do pamięci nieulotnej EEPROM
    if (ok == 1) {
      reference[temp2] = sensor_value;
      EEPROM.write(temp2, sensor_value);
    }
  }
  if (temp == 3) {
    temp_hist = temp_hist + up - down;
    lcd.setCursor(0, 0);
    lcd.print("Histereza");
    lcd.setCursor(10, 0);
    lcd.print(histeresis[temp2]);
    lcd.setCursor(14, 0);
    lcd.write(223);
    lcd.setCursor(15, 0);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Nowa");
    lcd.setCursor(10, 1);
    lcd.print(temp_hist);
    lcd.setCursor(14, 1);
    lcd.write(223);
    lcd.setCursor(15, 1);
    lcd.print("C");
    if (ok == 1) {
      histeresis[temp2] = temp_hist;
      EEPROM.write(temp2 + 6, temp_hist);
    }
  }
}

//wyswietlanie numerow zalaczonych przekaznikow
void zalaczone() {
  lcd.setCursor(1, 0);
  lcd.print("Zalaczone nr.");
  lcd.setCursor(1, 1);
  for (int i = 0; i < 6; i++) {
    if (powered[i] > 0) {
      lcd.print(i + 1);
      lcd.print("  ");
    }
  }
}
//wyswietlanie strzałki na wyświetlaczu dla aktualnie wybranego wiersza
void choice(int row) {
  lcd.setCursor(0, row);
  lcd.print(">");
}
//wyswietlanie strzalek
void arrows() {
  lcd.setCursor(0, 0);
  lcd.print("<");
  lcd.setCursor(15, 0);
  lcd.print(">");
}
//wyswietlanie aktualnej temperatury
void actual(int number) {
  int a = 0; int b = 0;
  if (number < 2) {
    a = 1;
    b = 2;
  }
  if (number == 2 || number == 3) {
    a = 3;
    b = 4;
  }
  if (number == 4 || number == 5) {
    a = 5;
    b = 6;
  }

  lcd.setCursor(1, 0);
  lcd.print("Sensor");
  lcd.print(a);
  lcd.setCursor(10, 0);
  lcd.print(actual_temp[a - 1]);
  lcd.setCursor(14, 0);
  lcd.write(223);
  lcd.setCursor(15, 0);
  lcd.print("C");

  lcd.setCursor(1, 1);
  lcd.print("Sensor");
  lcd.print(b);
  lcd.setCursor(10, 1);
  lcd.print(actual_temp[a]);
  lcd.setCursor(14, 1);
  lcd.write(223);
  lcd.setCursor(15, 1);
  lcd.print("C");
}
//wyswietlenie temperatury zalaczajacej przekazniki
void nastawy(int number) {
  int a = 0; int b = 0;
  if (number < 2) {
    a = 1;
    b = 2;
  }
  if (number == 2 || number == 3) {
    a = 3;
    b = 4;
  }
  if (number == 4 || number == 5) {
    a = 5;
    b = 6;
  }

  lcd.setCursor(1, 0);
  lcd.print("Sensor");
  lcd.print(a);
  lcd.setCursor(10, 0);
  lcd.print(reference[a - 1]);
  lcd.setCursor(14, 0);
  lcd.write(223);
  lcd.setCursor(15, 0);
  lcd.print("C");

  lcd.setCursor(1, 1);
  lcd.print("Sensor");
  lcd.print(b);
  lcd.setCursor(10, 1);
  lcd.print(reference[a]);
  lcd.setCursor(14, 1);
  lcd.write(223);
  lcd.setCursor(15, 1);
  lcd.print("C");
}
//wyswietlenie temperatury wylaczajace przekazniki
void wylaczajaca(int number) {
  int a = 0; int b = 0;
  if (number < 2) {
    a = 1;
    b = 2;
  }
  if (number == 2 || number == 3) {
    a = 3;
    b = 4;
  }
  if (number == 4 || number == 5) {
    a = 5;
    b = 6;
  }

  lcd.setCursor(1, 0);
  lcd.print("Sensor");
  lcd.print(a);
  lcd.setCursor(10, 0);
  lcd.print(reference[a - 1] - histeresis[a - 1]);
  lcd.setCursor(14, 0);
  lcd.write(223);
  lcd.setCursor(15, 0);
  lcd.print("C");

  lcd.setCursor(1, 1);
  lcd.print("Sensor");
  lcd.print(b);
  lcd.setCursor(10, 1);
  lcd.print(reference[a] - histeresis[a]);
  lcd.setCursor(14, 1);
  lcd.write(223);
  lcd.setCursor(15, 1);
  lcd.print("C");
}
//funkcja odczytująca temperaturę aktualna, załączająca przekaźniki
void relay() {
  if (sensors.available()) {
    for (byte i = 0; i < SENSORS_NUM; i++) {
      float temperature = sensors.readTemperature(FA(address[i]));
      Serial.println(temperature);
      actual_temp[i] = temperature;
      if (actual_temp[i] > reference[i]) {
        powered[i] = 1;
        digitalWrite((3 + i), HIGH);  //3 żeby mieć dobry adres pinów przekaźników
      }
      else if (actual_temp[i] + histeresis[i] < reference[i]) {
        powered[i] = 0;
        digitalWrite((3 + i), LOW);
      }
    }
    sensors.request();
  }
  I2C_transmission();
}
//transmisja danych po magistrali I2C do wskazanego adresu
void I2C_transmission() {
  Wire.beginTransmission(1);// dodać do którego adresu ma iść
  for (byte i = 0; i < SENSORS_NUM; i++) {
    Wire.write(actual_temp[i]);
  }
  Wire.endTransmission();
}
