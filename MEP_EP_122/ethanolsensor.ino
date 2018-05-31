const double M = 0.00501486;
const double Voffset = 100.0;

const int VgasPin = A0; 
const int VrefPin = A1;
const int VtempPin = A2;

double VgasValue = 0;
double VrefValue = 0;
double VtempValue = 0;

double tempC = 0;

double result = 0;
double ppm = 0;

void setup()
{
    pinMode(VgasPin, INPUT);
    pinMode(VrefPin, INPUT);
    pinMode(VtempPin, INPUT);

    Particle.variable("Ethanol_PPM", ppm);
    Particle.variable("Vgas", VgasValue);
    Particle.variable("Vref", VrefValue);
    Particle.variable("Vtemp", VtempValue);
}

void loop() 
{
    for(int i = 0; i < 100; i++)
    {
      VgasValue = analogRead(VgasPin)* 4.9;
      VrefValue = analogRead(VrefPin)* 4.9;
      VtempValue = analogRead(VtempPin)* 4.9;
    
      tempC = (29 * (VtempValue * 0.001)) - 18.0;
      
      result += (1/(M * (1+(tempC * 0.0025)))) * ((VgasValue - VrefValue - Voffset) * 0.001) + (0.14 * tempC);
      
      delay(40);
    }
    
    ppm = result / 100;
    result = 0;
    
    if(ppm < 140)
    {
        Particle.publish("output_color", "green");
    }
    else if(ppm < 300)
    {
        Particle.publish("output_color", "yellow");
    }
    else
    {
        Particle.publish("output_color", "red");
    }
    delay(1000);
}