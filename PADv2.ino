const int DOUT_Pin = 27;   // data pin
const int SCLK_Pin = 26;   // clock pin

float zeroOffset = 0.00;

float result1 = 0.00;
float result2 = 0.00;
float result3 = 0.00;

int maxFound = 0;
float maxValues[10];
int maxAvg = 10;

float time1 = 0;
float time2 = 0;
int timeCount = 0;
unsigned long timeArray[2];

float peaks[4];

float tempPressure = 0.00;
float tempPressure2 = 0.00;

float baseline = 0.00;
float timeMult = 0.00;

int motorPin = 4;          // motor out pin
int valvePin = 16;

float readPressure() {
  unsigned long start = millis();
  while (digitalRead(DOUT_Pin) == HIGH) {
    if (millis() - start > 500) return 0.00;
  }

  long count = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(SCLK_Pin, HIGH);
    delayMicroseconds(5);
    count = count << 1;
    if (digitalRead(DOUT_Pin) == HIGH) count++;
    digitalWrite(SCLK_Pin, LOW);
    delayMicroseconds(5);
  }

  digitalWrite(SCLK_Pin, HIGH);
  delayMicroseconds(5);
  digitalWrite(SCLK_Pin, LOW);
  delayMicroseconds(5);

  if (count & 0x800000) count |= 0xFF000000;

  return -(float)count / 24000.0;
}


float getCalibratedPressure() {
  return abs(readPressure() - zeroOffset);
}


void calibrateAndInflate() {
  digitalWrite(motorPin, LOW);
  digitalWrite(valvePin, HIGH);
  delay(10000);
  zeroOffset = readPressure();

  digitalWrite(valvePin, LOW);
  digitalWrite(motorPin, HIGH);
  delay(25000);
  digitalWrite(motorPin, LOW);

  delay(2000);
  Serial.println("Starting PAD ...");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Serial Connected.");

  pinMode(DOUT_Pin, INPUT);
  pinMode(SCLK_Pin, OUTPUT);
  digitalWrite(SCLK_Pin, LOW);
  pinMode(motorPin, OUTPUT);
  pinMode(valvePin, OUTPUT);

  calibrateAndInflate();
}

void bubbleSort(float num[]) {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (num[i] < num[j]) {
        float temp;
        temp = num[j];
        num[j] = num[i];
        num[i] = temp;
      }
    }
  }
}

void baseLine() {
  timeCount = 0;
  result1 = result2 = result3 = getCalibratedPressure();
  unsigned long baseStart = millis();

  do {
    result1 = result2;
    result2 = result3;
    result3 = getCalibratedPressure();
    delay(20);

    if (millis() - baseStart > 5000) {
      baseline = result3;
      return;
    }
  } while (!(result2 > result1 && result2 > result3));

  tempPressure = result2;
  peaks[3] = 0;
  unsigned long secondStart = millis();

  do {
    result3 = getCalibratedPressure();
    delay(20);

    if (result3 < (tempPressure + 2) && result3 > (tempPressure - 2)) {
      if (timeCount < 2) {
        timeArray[timeCount] = millis();
        timeCount++;
      }
      if (timeCount == 2) {
        delay(timeArray[1] - timeArray[0]);
        baseline = getCalibratedPressure();
        break;
      }
    }

    if (millis() - secondStart > 5000) {
      baseline = result3;
      break;
    }
  } while (peaks[3] == 0);
}

void loop() {
  baseLine();

  unsigned long adjustStart = millis();
  while (abs(getCalibratedPressure() - baseline) > 2.0 && (millis() - adjustStart < 10000)) {
    float currentP = getCalibratedPressure();

    if (currentP < baseline) {
      timeMult = constrain(20.0 * (baseline - currentP), 30, 500);
      digitalWrite(motorPin, HIGH);
      delay(timeMult);
      digitalWrite(motorPin, LOW);
    }
    else if (currentP > baseline) {
      timeMult = constrain(20.0 * (currentP - baseline), 20, 500);
      digitalWrite(valvePin, HIGH);
      delay(timeMult);
      digitalWrite(valvePin, LOW);
    }
    delay(150);
  }

  Serial.println("Pressure Stabilized.");

  maxFound = 0;
  result1 = result2 = result3 = getCalibratedPressure();

  unsigned long peakStart = millis();
  while (maxFound < 10 && (millis() - peakStart < 15000)) {
    result1 = result2;
    result2 = result3;
    result3 = getCalibratedPressure();
    delay(20);

    if (result2 > (result1 + 0.1) && result2 > (result3 + 0.1)) {
      maxValues[maxFound] = result2;
      maxFound++;
    }
  }


  while (maxFound < 10) {
    maxValues[maxFound] = getCalibratedPressure();
    maxFound++;
  }

  maxFound = 0;
  bubbleSort(maxValues);

  maxAvg = 10;
  float lowestNonAnomaly = 0.00;
  for (int i = 0; i < 2; i++) {
    if (maxValues[i] < 0.9 * maxValues[i + 1]) {
      maxAvg--;
      maxValues[i] = 0;
      lowestNonAnomaly = maxValues[i + 1];
    }
    else {
      lowestNonAnomaly = maxValues[i];
    }
  }

  float upperAnom = (1.1 * lowestNonAnomaly);
  for (int i = 0; i < 10; i++) {
    if (maxValues[i] > upperAnom) {
      maxValues[i] = 0.00;
      maxAvg--;
    }
  }

  float totalMax = 0.00;
  for (int i = 0; i < 10; i++) {
    totalMax = totalMax + maxValues[i];
  }


  if (maxAvg <= 0) {
    maxAvg = 1;
    totalMax = baseline;
  }

  float totalAvg = abs(totalMax / maxAvg);
  Serial.print("Non Anomalous Value Count: ");
  Serial.println(maxAvg);
  Serial.print("Total Value: ");
  Serial.println(totalAvg);


  digitalWrite(valvePin, HIGH);


  while (Serial.available() > 0) {
    Serial.read();
  }

  while (Serial.available() == 0) {
    delay(50);
  }
  while (Serial.available() > 0) {
    Serial.read();
  }

  calibrateAndInflate();
}