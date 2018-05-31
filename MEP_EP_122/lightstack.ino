#define GREEN D0
#define RED D1
#define YELLOW D2

String receivedValue;

void setup() {
    Particle.subscribe("output_color", respondToColor);
    Particle.variable("receivedVal", receivedValue);
    
    pinMode(GREEN, OUTPUT);
    pinMode(YELLOW, OUTPUT);
    pinMode(RED, OUTPUT);
    
    digitalWrite(GREEN, HIGH);
    delay(200);
    
    digitalWrite(YELLOW, HIGH);
    delay(200);
    
    digitalWrite(RED, HIGH);
    
    delay(2000);
    
    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(RED, LOW);
}

void loop() {
}

void respondToColor(const char *event, const char *data) {
    receivedValue = data;
    
    if (strcmp(data,"green") == 0) {
        digitalWrite(GREEN, HIGH);
        digitalWrite(YELLOW, LOW);
        digitalWrite(RED, LOW);
    } else if (strcmp(data,"yellow") == 0) {
        digitalWrite(YELLOW, HIGH);
        digitalWrite(GREEN, LOW);
        digitalWrite(RED, LOW);
    } else if (strcmp(data,"red") == 0) {
        digitalWrite(RED, HIGH);   
        digitalWrite(GREEN, LOW);
        digitalWrite(YELLOW, LOW);
    }
}