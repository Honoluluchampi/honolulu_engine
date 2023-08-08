void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  while (Serial.available() == 0) {}
  uint8_t password = Serial.read();

  int value = analogRead(A0);

  uint8_t h = value / 100 + '0';
  uint8_t t = (value % 100) / 10 + '0';
  uint8_t o = value % 10 + '0';

  uint8_t val[5] = {password, h, t, o, password};
  Serial.write(val, 5);
  Serial.flush();
}
