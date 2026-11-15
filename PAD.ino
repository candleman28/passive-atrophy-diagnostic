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
  for (int i = 0;i < 24;i++) {
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

  return (float)count / 24000.0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
Serial.println("Serial Connected.");


  pinMode(DOUT_Pin, INPUT);
  pinMode(SCLK_Pin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(valvePin, OUTPUT);

  digitalWrite(valvePin, HIGH);
  delay(10000);
  zeroOffset = readPressure();
  digitalWrite(valvePin, LOW);
  digitalWrite(motorPin, HIGH);
  delay(25000);
  digitalWrite(motorPin, LOW);

  delay(2000);
  Serial.println("Starting PAD ...");
  float mmHg = readPressure();
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
  do {
      result1 = result2;
      result2 = result3;
      result3 = readPressure() - zeroOffset;
    
    } while (!(result2 > result1 && result2 > result3));
    tempPressure = result2;
    peaks[3] = 0;
    do {
      result3 = readPressure() - zeroOffset;

      if (result3 < (tempPressure + 2) && result3 > (tempPressure - 2)) {
        if (timeCount < 2) {
          timeArray[timeCount] = millis();
          timeCount++;
        }
        if (timeCount == 2) {
          delay(timeArray[1] - timeArray[0]);
          baseline = readPressure() - zeroOffset;
          break;
        }
      }
    } while (peaks[3] == 0);
}

void loop() {
    baseLine();
    
    unsigned long adjustStart = millis();
    while (abs((readPressure() - zeroOffset) - baseline) > 2.0 && (millis() - adjustStart < 10000)) {
      float currentP = readPressure() - zeroOffset;

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

    result1 = 0;
    result2 = 0;
    result3 = 0;
    
   while (maxFound < 10) {
      result1 = result2;
      result2 = result3;
      result3 = readPressure() - zeroOffset;

      if (result2 > result1 && result2 > result3) {
      maxValues[maxFound] = result2;
      maxFound++;
      }
    }
    maxFound = 0;
    bubbleSort(maxValues); // sorts

    maxAvg = 10;
    // check for anomalies
    float lowestNonAnomaly = 0.00;
    for (int i = 0;i < 2;i++) {
      if (maxValues[i] < 0.9 * maxValues[i + 1]) {
        maxAvg--;
        maxValues[i] = 0;
        lowestNonAnomaly = maxValues[i + 1];
      }
      else {
        lowestNonAnomaly = maxValues[i];
      }
    }

    // find upper limit
    float upperAnom = (1.1 * lowestNonAnomaly);
    for (int i = 0;i < 10;i++) {
      if (maxValues[i] > upperAnom) {
        maxValues[i] = 0.00;
        maxAvg--;
      }
    }

    // average out non-anomalous maximums
    float totalMax = 0.00;
    for (int i = 0;i < 10;i++) {
      totalMax = totalMax + maxValues[i];
    }
    float totalAvg = (float(totalMax / maxAvg));
    Serial.println(maxAvg);
    Serial.println(totalAvg);
    while (true) {
      digitalWrite(valvePin, HIGH);
    }
}