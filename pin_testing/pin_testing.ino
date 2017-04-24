int pinRead = 8;
int pinWrite = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Starting...");
  pinMode(pinRead, INPUT);
  digitalWrite(pinRead, LOW);
  pinMode(pinWrite, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.println("writing L");
  digitalWrite(pinWrite, LOW);
  if ( digitalRead(pinRead) == HIGH ) {
    Serial.println("Reading H");
  } else if ( digitalRead(pinRead) == LOW ) {
    Serial.println("Reading L");
  } else {
    Serial.println( digitalRead(pinRead) );
  }
  
  Serial.println();
  delay(2000);
  
  Serial.println("writing H");
  digitalWrite(pinWrite, HIGH);   
  if ( digitalRead(pinRead) == HIGH ) {
    Serial.println("Reading H");
  } else if ( digitalRead(pinRead) == LOW ) {
    Serial.println("Reading L");
  } else {
    Serial.println( digitalRead(pinRead) );
  }

  Serial.println();
  delay(2000);
}
