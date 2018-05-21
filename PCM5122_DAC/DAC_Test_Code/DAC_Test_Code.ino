/*
 * Special Thanks to the following for having awesome sample code and Arduino Libraries that made this doable!
 * 
 * avdweb: http://www.avdweb.nl/ For the super fast AnalogRead Function!
 * Adafruit: https://www.adafruit.com/ For the FFT library that runs on the Arduino Zero. I would not want to code this myself. 
 * Théo Meyer: https://twitter.com/AloyseTech For the Zerodio Library where I was able to figure out how the TC5 Interrupt functioned.
 * 
 */

/*
 * This is the main file for testing the PCM5122 DAC board using the "DAC_PCB_TEST" fixture.
 * 
 * Theory of Operation
 *  1. Wait for button press 
 *  2. Establish I2C connection with PCM5122 PCB
 *  3. Configure the PCM5122 over I2C
 *  4. Setup TC5 to push a 200Hz signal at 8000Hz sampling over I2S
 *  5. Record the output of the PCM5122 with analog input
 *  6. Run FFT and analyze the signal
 *  7. Notify user over Serial if the PCM5122 PCB passes the test
 *  8. Repeat with next PCM5122 PCB
 * 
 */

#include <avdweb_AnalogReadFast.h>
#include <Adafruit_ZeroFFT.h>
#include <arm_common_tables.h>
#include <I2S.h>
#include <Wire.h>

//This is just how much data points will be stored for when samping the output signal from the DAC
#define DATA_SIZE 8192

//All the "important" registers that need to be configured in the PCM5122
#define reg01 0x01
#define reg02 0x02
#define reg04 0x04
#define reg09 0x09
#define reg13 0x0D
#define reg14 0x0E
#define reg19 0x13
#define reg20 0x14 //PLL P
#define reg21 0x15 //PLL J
#define reg22 0x16 //PLL D
#define reg23 0x17
#define reg24 0x18 //PLL R
#define reg27 0x1B //DSP Clk Divider
#define reg28 0x1C //DAC Clk Divider
#define reg29 0x1D //NCP Clock Divider
#define reg30 0x1E //OSR Clock Divider
#define reg37 0x25
#define reg40 0x28
#define reg61 0x3D
#define reg62 0x3E
#define reg65 0x41
#define reg86 0x56
#define reg91 0x5B
#define reg94 0x5E
#define reg95 0x5F
#define reg108 0x6C
#define reg118 0x76

//I2C address for the PCM5122 as it is configured on the PCB design
const int pcm5122 = 0x4D;
//Where the audio output of the PCM5122 DAC are connected to the Arduino Zero
const int audio_in_R = 4;
const int audio_in_L = 5;

//enable and path control the TS3A5018PWR analog switchs. 
const int enable = 6; //active low
const int path = 5;
//pwr_on is connected to the power switcher that turns the power to the PCM5122 DAC board on and off. 
const int pwr_on = 4; //active high
//Button on the fixture
const int btn = 7;

//Samples from the DAC via analog read are stored here
int16_t record_R[DATA_SIZE];
int record_count = 0;


//Change the output frequency, amplitude of signal, and the output rate here
const int frequency = 200; 
const int amplitude = 400; 
const long sampleRate = 8000; 

const int halfWavelength = (sampleRate / frequency) / 2; 

//If you change the output frequency you will need to change the "window" of acceptable frequencies that are passing.
const float freq_window_top = 210.0;
const float freq_window_bot = 190.0;

//If this is > 0 then the PCM5122 DAC failed i2c communitication
int i2cfail = 0;

//Controls the state machine for the test procedure
int state = 0;

//Since the sampling of the signals via analog read is kinda indeterminate in how long it can take we measure the start and stop time of the test
int test_start = 0;
int test_end = 0;

//The Sampling Rate of the recording 
int record_SR = 0;

//Outputs of the FFT function. 
float maxVal = 0;
float maxFreq = 0;

int sample = amplitude; 
long count = 0;

//Setup the basic I/O and Serial terminal.
void setup() 
{
  Wire.begin();

  pinMode(pwr_on,OUTPUT);
  pinMode(path,OUTPUT);
  pinMode(enable,OUTPUT);

  Serial.begin(2000000);
  while (!Serial) 
  {
    ;
  }
}

void loop() 
{
  switch(state)
  {
    /*
     * Waiting to Start Test
     *  1. Reset all varables used during the test 
     *  2. Wait till the button is pressed
     *  3. Once button pressed power up the PCM5122 PCB
     */
    case 0:
    {
      Serial.println("Waiting to Start...");
      i2cfail = 0;
      test_start = 0;
      test_end = 0;
      record_SR = 0;
      maxVal = 0;
      maxFreq = 0;
      sample = amplitude; 
      count = 0;
      record_count = 0;

      while(digitalRead(btn));
      state = 1;

      power_on_device();
      delay(200);
      
      break;
    }
    /*
     * Initilaize the DAC
     * If this fails then the board is bad. 
     */
    case 1:
    {
      Serial.println("Initialize DAC.");
      pcm5122_init();
      
      if( i2cfail > 0 )
      {
        state = 9; //I2C Fail
      }
      else 
      {
        state = 2;
      }
      break;
    }
    /*
     * Initialize I2S
     * If this fails then the board is bad 
     */
    case 2:
    {
      Serial.println("Initialize I2S.");
      if (!I2S.begin( I2S_PHILIPS_MODE, sampleRate, 16))
      { 
        state = 8; //I2S Fail
      }
      else
      {
        state = 3;
      }
      break;
    }
    /*
     * Test Starting since initilization of the PCM5122 was completed
     * Take the start time of the test.
     */
    case 3:
    {
      Serial.println("Starting Test...");
      
      tcConfigure(sampleRate);
      delay(2000);

      test_start = micros();
      state = 4;
      
      break;
    }
    /*
     * Record the output of the DAC
     */
    case 4:
    {     
      record_R[record_count] = analogReadFast(audio_in_R);

      //When our recording array is full diable the timer 
      if(record_count >= DATA_SIZE-1)
      {
        test_end = micros();     
        tcDisable();  
        state = 5;
      }

      record_count++;
      
      break;
    }
    /*
     * This portion does the FFT on the recorded data
     */
    case 5:
    { 
      Serial.println("Running Analysis..."); 
      ZeroFFT(record_R, 2048);
      //This figures out the sampling rate (roughly) of the recording of the DAC 
      record_SR = DATA_SIZE / (float((test_end - test_start))/1000000);

      for(int i=0; i<2048/2; i++)
      {
        //Serial.print(FFT_BIN(i, record_SR , 1024));
        //Serial.print(" Hz: ");
        //Serial.println(record_R[i]);
        
        if(record_R[i] > maxVal)
        {
          maxVal = record_R[i];
          maxFreq = FFT_BIN(i, record_SR , 2048);
        }
      }
      Serial.print("Peak Freq: ");
      Serial.println(maxFreq);
      Serial.print("Amplitude: ");
      Serial.println(maxVal);
      state = 6;
      break;
    }
    /*
     * See if the frequency is in bounds 
     */
    case 6:
    {
      if((maxFreq > freq_window_bot) & (maxFreq < freq_window_top))
      {
        state = 7;
      }
      else
      {
        Serial.println("Frequency Out of Range.");
        state = 8;
      }
      
      break;
    }
    case 7:
    {
      Serial.println("Unit Passes.");
      state = 10;
      break;
    }
    case 8:
    {
      Serial.println("I2S Failure. Unit Failed.");
      state = 10;
      break;
    }
    case 9:
    {
      Serial.println("I2C Failure. Unit Failed.");
      state = 11;
      break;
    }
    case 10:
    {
      I2S.end();
      state = 11;
      break;
    }
    case 11:
    {
      power_off_device();
      delay(200);
      Serial.println("Safe to remove device.");
      Serial.println(" ");
      state = 0;
      break;
    }
    default:
    {
      Serial.println("what?");
    }
  }
}

/*
 * TC5_Handler
 * This is the timer interupt handler for outputing the correct frequency to the DAC
 */
void TC5_Handler (void)
{
  if (count % halfWavelength == 0)
  {
    sample = -1 * sample;
  }
  
  I2S.write(sample);
  I2S.write(sample);

  count++;

  //reset the interrupt flag
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

/*
 * power_off_device
 * turns off the p cahnnel mosfet and data signal lines
 */
void power_off_device()
{
  digitalWrite(pwr_on,LOW);  
  delay(10);
  digitalWrite(enable,HIGH);
  digitalWrite(path,LOW);
}

/*
 * power_on_device
 * turns on the p cahnnel mosfet and data signal lines
 */
void power_on_device()
{
  digitalWrite(enable,LOW);
  digitalWrite(path,HIGH);
  delay(10);
  digitalWrite(pwr_on,HIGH); 
}

/*
 * pcm5122_init
 * I2C commands that initilize the pcb5122 and allow it to output audio
 */
void pcm5122_init()
{
  Wire.beginTransmission(pcm5122);
  Wire.write(reg02);
  Wire.write(0x10);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg01);
  Wire.write(0x11);
  i2cfail += Wire.endTransmission(true);
  
  Wire.beginTransmission(pcm5122);
  Wire.write(reg02);
  Wire.write(0x00);
  i2cfail += Wire.endTransmission(true);
  
  Wire.beginTransmission(pcm5122);
  Wire.write(reg13);
  Wire.write(0x10);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg14);
  Wire.write(0x10);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg37);
  Wire.write(0x7D);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg61);
  Wire.write(0x00);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg62);
  Wire.write(0x00);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg65);
  Wire.write(0x00);
  i2cfail += Wire.endTransmission(true);

  Wire.beginTransmission(pcm5122);
  Wire.write(reg02);
  Wire.write(0x00);
  i2cfail += Wire.endTransmission(true);
}

/*
 * The below functions are to configure the TC5 timer and setup a timed interrupt 
 */
void tcConfigure(uint32_t sampleRate)
{
  // Enable GCLK for TCC2 and TC5 (timer counter input clock)
  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
  while (GCLK->STATUS.bit.SYNCBUSY);

  tcReset();

  // Set Timer counter Mode to 16 bits
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

  // Set TC5 mode as match frequency
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;

  TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sampleRate - 1);
  while (tcIsSyncing());
  
  // Configure interrupt request
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  // Enable the TC5 interrupt request
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  while (tcIsSyncing());
} 

void tcReset()
{
  // Reset TCx
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing());
  while (TC5->COUNT16.CTRLA.bit.SWRST);
}

bool tcIsSyncing()
{
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

void tcDisable()
{
  // Disable TC5
  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (tcIsSyncing());
}

