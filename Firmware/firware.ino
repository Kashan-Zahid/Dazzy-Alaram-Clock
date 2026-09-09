/*
  Dazzy-Alarm Clock Firmware
  Board: XIAO ESP32-C3 / WEMOS C3 Mini

  Button mapping:
    SW4 = GPIO 5 -> Alarm ON/OFF
    SW3 = GPIO 4 -> Stop/Snooze alarm
    SW2 = GPIO 3 -> Increase hour
    SW1 = GPIO 2 -> Increase minute

  Buzzer:
    GPIO 8

  No external libraries required.
*/

const int SW4_PIN = 5;
const int SW3_PIN = 4;
const int SW2_PIN = 3;
const int SW1_PIN = 2;

const int BUZZER_PIN = 8;

// --------------------------------------------------
// CLOCK
// --------------------------------------------------

int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

unsigned long lastSecond = 0;

// --------------------------------------------------
// ALARM
// --------------------------------------------------

int alarmHour = 7;
int alarmMinute = 0;

bool alarmEnabled = true;
bool alarmRinging = false;

unsigned long alarmStartedAt = 0;

// Alarm duration: 60 seconds
const unsigned long ALARM_DURATION = 60000;

// --------------------------------------------------
// SNOOZE
// --------------------------------------------------

bool snoozeActive = false;
unsigned long snoozeUntil = 0;

const unsigned long SNOOZE_TIME = 5 * 60 * 1000UL;

// --------------------------------------------------
// BUTTON DEBOUNCE
// --------------------------------------------------

unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_TIME = 250;

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);
  pinMode(SW4_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("==============================");
  Serial.println("       BLARE ALARM CLOCK");
  Serial.println("==============================");

  Serial.println("Clock started.");
  Serial.println("Alarm: 07:00");
  Serial.println("Alarm enabled.");

  lastSecond = millis();
}

// --------------------------------------------------
// MAIN LOOP
// --------------------------------------------------

void loop() {

  updateClock();

  checkButtons();

  checkAlarm();

  updateBuzzer();

  delay(10);
}

// --------------------------------------------------
// CLOCK UPDATE
// --------------------------------------------------

void updateClock() {

  unsigned long now = millis();

  if (now - lastSecond >= 1000) {

    lastSecond += 1000;

    currentSecond++;

    if (currentSecond >= 60) {

      currentSecond = 0;
      currentMinute++;

      if (currentMinute >= 60) {

        currentMinute = 0;
        currentHour++;

        if (currentHour >= 24) {
          currentHour = 0;
        }
      }
    }

    printTime();
  }
}

// --------------------------------------------------
// PRINT CLOCK
// --------------------------------------------------

void printTime() {

  Serial.print("Time: ");

  if (currentHour < 10)
    Serial.print("0");

  Serial.print(currentHour);
  Serial.print(":");

  if (currentMinute < 10)
    Serial.print("0");

  Serial.print(currentMinute);
  Serial.print(":");

  if (currentSecond < 10)
    Serial.print("0");

  Serial.print(currentSecond);

  Serial.print(" | Alarm: ");

  if (alarmEnabled)
    Serial.print("ON ");
  else
    Serial.print("OFF");

  Serial.print(" ");

  if (alarmHour < 10)
    Serial.print("0");

  Serial.print(alarmHour);
  Serial.print(":");

  if (alarmMinute < 10)
    Serial.print("0");

  Serial.println(alarmMinute);
}

// --------------------------------------------------
// BUTTON HANDLING
// --------------------------------------------------

void checkButtons() {

  unsigned long now = millis();

  // Debounce
  if (now - lastButtonPress < DEBOUNCE_TIME) {
    return;
  }

  // ------------------------------------------------
  // SW4 - ALARM ON/OFF
  // ------------------------------------------------

  if (digitalRead(SW4_PIN) == LOW) {

    alarmEnabled = !alarmEnabled;

    // Stop ringing if alarm disabled
    if (!alarmEnabled) {
      stopAlarm();
    }

    Serial.print("Alarm ");

    if (alarmEnabled)
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");

    lastButtonPress = now;
    return;
  }

  // ------------------------------------------------
  // SW3 - STOP / SNOOZE
  // ------------------------------------------------

  if (digitalRead(SW3_PIN) == LOW) {

    if (alarmRinging) {

      // Activate 5 minute snooze
      snoozeActive = true;
      snoozeUntil = millis() + SNOOZE_TIME;

      stopAlarm();

      Serial.println("SNOOZE: 5 minutes");
    }
    else {

      Serial.println("SW3 pressed - no alarm active");
    }

    lastButtonPress = now;
    return;
  }

  // ------------------------------------------------
  // SW2 - INCREASE ALARM HOUR
  // ------------------------------------------------

  if (digitalRead(SW2_PIN) == LOW) {

    alarmHour++;

    if (alarmHour >= 24)
      alarmHour = 0;

    Serial.print("Alarm hour: ");

    if (alarmHour < 10)
      Serial.print("0");

    Serial.println(alarmHour);

    lastButtonPress = now;
    return;
  }

  // ------------------------------------------------
  // SW1 - INCREASE ALARM MINUTE
  // ------------------------------------------------

  if (digitalRead(SW1_PIN) == LOW) {

    alarmMinute += 5;

    if (alarmMinute >= 60) {

      alarmMinute = 0;
      alarmHour++;

      if (alarmHour >= 24)
        alarmHour = 0;
    }

    Serial.print("Alarm time: ");

    if (alarmHour < 10)
      Serial.print("0");

    Serial.print(alarmHour);

    Serial.print(":");

    if (alarmMinute < 10)
      Serial.print("0");

    Serial.println(alarmMinute);

    lastButtonPress = now;
    return;
  }
}

// --------------------------------------------------
// ALARM CHECK
// --------------------------------------------------

void checkAlarm() {

  if (!alarmEnabled)
    return;

  // -----------------------------------------------
  // Check snooze
  // -----------------------------------------------

  if (snoozeActive) {

    if (millis() >= snoozeUntil) {

      snoozeActive = false;
      alarmRinging = true;

      alarmStartedAt = millis();

      Serial.println("SNOOZE FINISHED!");
      Serial.println("ALARM RINGING!");

    }

    return;
  }

  // -----------------------------------------------
  // Normal alarm
  // -----------------------------------------------

  if (!alarmRinging &&
      currentHour == alarmHour &&
      currentMinute == alarmMinute &&
      currentSecond == 0) {

    alarmRinging = true;
    alarmStartedAt = millis();

    Serial.println();
    Serial.println("==============================");
    Serial.println("       ALARM RINGING!");
    Serial.println("==============================");
  }

  // -----------------------------------------------
  // Automatic alarm timeout
  // -----------------------------------------------

  if (alarmRinging) {

    if (millis() - alarmStartedAt >= ALARM_DURATION) {

      stopAlarm();

      Serial.println("Alarm automatically stopped.");
    }
  }
}

// --------------------------------------------------
// BUZZER
// --------------------------------------------------

void updateBuzzer() {

  if (!alarmRinging) {

    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  // Beep pattern
  // 500ms ON / 500ms OFF

  unsigned long elapsed =
      millis() - alarmStartedAt;

  if ((elapsed / 500) % 2 == 0) {

    digitalWrite(BUZZER_PIN, HIGH);

  } else {

    digitalWrite(BUZZER_PIN, LOW);
  }
}

// --------------------------------------------------
// STOP ALARM
// --------------------------------------------------

void stopAlarm() {

  alarmRinging = false;

  digitalWrite(BUZZER_PIN, LOW);
}