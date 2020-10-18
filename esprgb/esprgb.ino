uint8_t redLed = D6;
uint8_t blueLed = D7;
uint8_t greenLed = D8;

void setup() {
  Serial.begin(115200);
}

void loop() {
  if (Serial.available()) {
    String raw = Serial.readStringUntil('\n');
    const char* p = (const char*) &raw;
    uint32_t num = (uint32_t) strtol(p, NULL, 16);  // Convertir string hexadecimal a entero
    num &= 0x00ffffff;  // Parece que no está sirviendo para nada pero bueno...
    Serial.print("Recibido: ");
    Serial.println(num);
    integerToRGB(num);
  }
}

void integerToRGB(uint32_t value) {
  uint8_t rColor, gColor, bColor;

  rColor = (value & 0xff0000) >> 16;
  gColor = (value & 0xff00) >> 8;
  bColor = value & 0xff;

  analogWrite(redLed, rColor);
  analogWrite(greenLed, gColor);
  analogWrite(blueLed, bColor);
}
