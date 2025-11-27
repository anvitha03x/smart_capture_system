#include <Servo.h>

// ------------------ Pin Configuration ------------------
#define MIC1 A0
#define MIC2 A1
#define MIC3 A2
#define MIC4 A3
#define SERVOPIN 9

// ------------------ Servo Configuration ------------------
Servo directionServo;
float currentAngle = 90.0;  // Start facing forward

// ------------------ Microphone Parameters ------------------
int micPins[4] = {MIC1, MIC2, MIC3, MIC4};
int micAngles[4] = {0, 45, 135, 180}; // Approx. mic placement angles

// ------------------ Detection Parameters ------------------
const int sampleCount = 7;       // Median filter samples 
const int loudnessThreshold = 20;
const float energyThreshold = 500; // Speech energy threshold
const unsigned long silenceDelay = 2000; // Return-to-center delay (ms)
const int stabilityCycles = 3;   // Required stable readings
const float moveTolerance = 7.0; // Servo move threshold (degrees)

// ------------------ Filtering Buffers ------------------
const int filterBufferSize = 10;
int micHistory[4][sampleCount] = {0};
int micFilterBuffer[4][filterBufferSize] = {0};
int baseline[4] = {0};
int historyIndex = 0;
int filterIndex = 0;

// Bandpass filter states
float highPassState[4] = {0};
float lowPassState[4] = {0};

// Filter coefficients
const float highPassCoeff = 0.9;
const float lowPassCoeff = 0.1;

// ------------------ Stability Tracking ------------------
float candidateAngle = 90.0;
int stableCount = 0;
unsigned long lastSoundTime = 0;

// ------------------ Helper Functions ------------------
//threshold can be changed by noise levels
// Calculate angular difference (with wrap-around)
float angleDifference(float a, float b) {
  float diff = fabs(a - b);
  return (diff > 180) ? (360 - diff) : diff;
}

// Simple bandpass filter
int bandpassFilter(int micIndex, int inputSample) {
  int prevSample = micFilterBuffer[micIndex][(filterIndex + filterBufferSize - 1) % filterBufferSize];
  highPassState[micIndex] = highPassCoeff * (highPassState[micIndex] + inputSample - prevSample);
  lowPassState[micIndex] = lowPassCoeff * highPassState[micIndex] + (1 - lowPassCoeff) * lowPassState[micIndex];
  micFilterBuffer[micIndex][filterIndex] = inputSample;
  return int(lowPassState[micIndex]);
}

// Median filter across history
int medianFilter(int values[], int size) {
  int sorted[size];
  memcpy(sorted, values, size * sizeof(int));
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        int t = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = t;
      }
    }
  }
  return sorted[size / 2];
}

// ------------------ Setup ------------------
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) pinMode(micPins[i], INPUT);

  // Baseline calibration
  for (int i = 0; i < 4; i++) {
    long sum = 0;
    for (int j = 0; j < 32; j++) sum += analogRead(micPins[i]);
    baseline[i] = sum / 32;
    Serial.print("Baseline Mic "); Serial.print(i + 1);
    Serial.print(": "); Serial.println(baseline[i]);
  }

  directionServo.attach(SERVOPIN);
  directionServo.write(currentAngle);
  lastSoundTime = millis();

  Serial.println("System ready: Sound localization + Speech energy detection");
  Serial.println("Format: Energy | MaxMic | 2ndMic | CandidateAngle | StableCount | ServoAngle");
}

// ------------------ Main Loop ------------------
void loop() {
  int raw[4], filtered[4];
  float speechEnergy = 0;

  // Read mic data and filter
  for (int i = 0; i < 4; i++) {
    raw[i] = analogRead(micPins[i]) - baseline[i];
    if (raw[i] < 0) raw[i] = 0;
    filtered[i] = bandpassFilter(i, raw[i]);
    speechEnergy += filtered[i] * filtered[i];
  }
  filterIndex = (filterIndex + 1) % filterBufferSize;
  
  Serial.print("Mic Values -> ");
  for (int i = 0; i < 4; i++) {
    Serial.print("Mic"); Serial.print(i + 1);
    Serial.print(": "); Serial.print(filtered[i]);
    if (i < 3) Serial.print(" | ");
  }
  Serial.println(); // move to next line
  // Process only if speech is detected
  if (speechEnergy > energyThreshold) {
    lastSoundTime = millis();

    // Update history buffer
    for (int i = 0; i < 4; i++) micHistory[i][historyIndex] = filtered[i];
    historyIndex = (historyIndex + 1) % sampleCount;

    // Median filter per mic
    int medianVals[4];
    for (int i = 0; i < 4; i++) medianVals[i] = medianFilter(micHistory[i], sampleCount);

    // Find top two mics
    int maxVal = -1, secondVal = -1;
    int idx1 = -1, idx2 = -1;
    for (int i = 0; i < 4; i++) {
      if (medianVals[i] > maxVal) {
        secondVal = maxVal; idx2 = idx1;
        maxVal = medianVals[i]; idx1 = i;
      } else if (medianVals[i] > secondVal) {
        secondVal = medianVals[i]; idx2 = i;
      }
    }

    // Only act if loud enough
    if (maxVal > loudnessThreshold) {
      float angle;
      if (secondVal >= 0.85 * maxVal && idx2 != -1) {
        float total = maxVal + secondVal;
        angle = (micAngles[idx1] * (maxVal / total)) + (micAngles[idx2] * (secondVal / total));
      } else {
        angle = micAngles[idx1];
      }

      // Check stability
      if (angleDifference(angle, candidateAngle) < 10) {
        stableCount++;
      } else {
        stableCount = 1;
        candidateAngle = angle;
      }

      // Debug info
      Serial.print(speechEnergy); Serial.print(" | ");
      Serial.print(maxVal); Serial.print(" | ");
      Serial.print(secondVal); Serial.print(" | ");
      Serial.print(candidateAngle); Serial.print(" | ");
      Serial.print(stableCount); Serial.print(" | ");
      Serial.println(currentAngle);

      // Move servo after stable detection
      if (stableCount >= stabilityCycles && angleDifference(candidateAngle, currentAngle) > moveTolerance) {
        candidateAngle = constrain(candidateAngle, 0, 180);
        directionServo.write(candidateAngle);
        currentAngle = candidateAngle;
        stableCount = 0;
        delay(200);
      }
    }
  } 
  else {
    // Return to center if no sound detected for a while
    if (millis() - lastSoundTime > silenceDelay && angleDifference(currentAngle, 90) > 3) {
      Serial.println("Silence detected — returning to center.");
      directionServo.write(90);
      currentAngle = 90;
      candidateAngle = 90;
      stableCount = 0;
      delay(200);
    }
  }
  delay(50);
}