#include <Ultrasonic.h>

#include <MozziGuts.h>
#include <Oscil.h>
#include <ControlDelay.h>
#include <mozzi_fixmath.h>
#include <mozzi_midi.h>
#include <tables/sin2048_int8.h>

//Ultrasonic Sensors Pins
#define S1_TRIGGER_PIN  2
#define S1_ECHO_PIN     3

#define S2_TRIGGER_PIN  4
#define S2_ECHO_PIN     5

#define S3_TRIGGER_PIN  6
#define S3_ECHO_PIN     7

#define S4_TRIGGER_PIN  10
#define S4_ECHO_PIN     11

#define SS_TRIGGER_PIN  12
#define SS_ECHO_PIN     13

Ultrasonic ultrasonic1(S1_TRIGGER_PIN, S1_ECHO_PIN);
Ultrasonic ultrasonic2(S2_TRIGGER_PIN, S2_ECHO_PIN);
Ultrasonic ultrasonic3(S3_TRIGGER_PIN, S3_ECHO_PIN);
Ultrasonic ultrasonic4(S4_TRIGGER_PIN, S4_ECHO_PIN);
Ultrasonic ultrasonicSwitch(SS_TRIGGER_PIN, SS_ECHO_PIN);

int sensorPeriod = 0;

//Values return from sensors (Centimeter)
float sensor1CM = 320;
float sensor2CM = 320;
float sensor3CM = 320;
float sensor4CM = 320;
float sensorSCM = 320;

float sensor1LastCM = 320;
float sensor2LastCM = 320;
float sensor3LastCM = 320;
float sensor4LastCM = 320;
float sensorSLastCM = 320;


//Modified values from sensors
float sensorFreq1 = 100; //sensor1
float sensorFreq2 = 100; //sensor2
float sensorFreq3 = 100; //sensor3
float sensorFreq4 = 100; //sensor4

float sensorRatio2 = 2.0;
float sensorRatio4 = 2.0;


// Volume Control
const char INPUT_PIN = 0; // set the input for the knob to analog pin 0
// to convey the volume level from updateControl() to updateAudio()
int volume_knob = 0;

int volume = 0;

unsigned int echo_cells_1 = 32;
unsigned int echo_cells_2 = 60;
unsigned int echo_cells_3 = 127;


#define CONTROL_RATE 64
ControlDelay <128, int> kDelay; // 2seconds

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aModulator(SIN2048_DATA);

// synthesis parameters in fixed point formats
Q8n8 ratio; // unsigned int with 8 integer bits and 8 fractional bits
Q24n8 carrier_freq; // unsigned long with 24 integer bits and 8 fractional bits
Q24n8 mod_freq; // unsigned long with 24 integer bits and 8 fractional bits

// for random notes
Q8n0 octave_start_note = 69;


void setup(){
  ratio = float_to_Q8n8(2.0f);   // define modulation ratio in float and convert to fixed-point
  kDelay.set(echo_cells_1);
  aSin.setFreq(440); // set the frequency
  startMozzi(CONTROL_RATE); // set a control rate of 64 (powers of 2 please)
//  Serial.begin(9600);
}


void updateControl(){
  
  // read the variable resistor for volume
  int sensorKnobValue = mozziAnalogRead(INPUT_PIN); // value is 0-1023  
  // map it to an 8 bit range for efficient calculations in updateAudio
  volume_knob = map(sensorKnobValue, 0, 1023, 0, 255); 
  
  static Q16n16 last_note = octave_start_note;
  
//  Note: Avoid using many Serial.print(); it would affect sound
  
// Ultrasonic Sensors
  sensorPeriod++; 
  if (sensorPeriod==1) // Turn on and off volume
  {
    long microsecS = ultrasonicSwitch.timing();
    sensorSCM = ultrasonicSwitch.convert(microsecS, Ultrasonic::CM);
    if (sensorSCM<1) sensorSCM = sensorSLastCM;
    sensorSLastCM = sensorSCM;
//    Serial.print("\nVOLUME: ");
//    Serial.print(volume);
//    Serial.print("\nVOLUME_KNOB: ");
//    Serial.print(volume_knob);
//    Serial.print("\nsensorSCM: ");
//    Serial.print(sensorSCM);
//    Serial.print("\nsensor1CM: ");
//    Serial.print(sensor1CM);
//    Serial.print("\nsensor2CM: ");
//    Serial.print(sensor2CM);
//    Serial.print("\nsensor3CM: ");
//    Serial.print(sensor3CM);
//    Serial.print("\nsensor4CM: ");
//    Serial.print(sensor4CM);
  }
  else if (sensorPeriod==10)
  {
    long microsec1 = ultrasonic1.timing();
    sensor1CM = ultrasonic1.convert(microsec1, Ultrasonic::CM);
    if (sensor1CM==0) sensor1CM = 320;
    if (sensor1CM>320) sensor1CM = 320;
    sensor1CM = (sensor1CM + sensor1LastCM)/2;
    sensorFreq1 = map(sensor1CM, 0, 320, 2000, 10);
    sensor1LastCM = sensor1CM;

  }
  else if (sensorPeriod==20)
  {
    long microsec2 = ultrasonic2.timing();
    sensor2CM = ultrasonic2.convert(microsec2, Ultrasonic::CM);
    if (sensor2CM==0) sensor2CM = 320;
    if(sensor2CM<100)
      sensorRatio2 = 1;
    else if(sensor2CM<150)
      sensorRatio2 = 2;
    else if(sensor2CM<300)
      sensorRatio2 = 3;
    else
      sensorRatio2 = 4;
  }
  else if (sensorPeriod==30)
  {
    long microsec3 = ultrasonic3.timing();
    sensor3CM = ultrasonic3.convert(microsec3, Ultrasonic::CM);
    if (sensor3CM==0) sensor3CM = 320;
    if (sensor3CM>320) sensor3CM = 320;
    sensor3CM = (sensor3CM + sensor3LastCM)/2;
    sensorFreq3 = map(sensor3CM, 0, 320, 2000, 10);
    sensor3LastCM = sensor3CM;
  }
  else if (sensorPeriod==40)
  {
    long microsec4 = ultrasonic4.timing();
    sensor4CM = ultrasonic4.convert(microsec4, Ultrasonic::CM);
    if (sensor4CM==0) sensor4CM = 320;
    if(sensor4CM<100)
      sensorRatio4 = 1;
    else if(sensor4CM<150)
      sensorRatio4 = 2;
    else if(sensor4CM<300)
      sensorRatio4 = 3;
    else
      sensorRatio4 = 4;
  }
  else if(sensorPeriod>50)
  {
    sensorPeriod=0;
  }
  
  
  if (sensorSCM<150)
  {
    if (volume<volume_knob)
    {
      if( volume<20)
        volume+=1;
      else
        volume+=5;
    }
    else if (volume>volume_knob)
      volume = volume_knob;
  }
  else if (sensorSCM>150&&((sensor1CM<100&&sensor1CM>0)||(sensor2CM<100&&sensor2CM>0)||(sensor3CM<100&&sensor3CM>0)||(sensor4CM<100&&sensor4CM>0)))
  {
    if (volume<0.8*volume_knob)
    {
      if( volume<20)
        volume+=1;
      else
        volume+=5;
    }
    else if (volume>0.8*volume_knob)
      volume = 0.8*volume_knob;
  }  
  else 
  {
    volume-=5;
    if(volume<2) volume = 0;
  }
  
  ratio = float_to_Q8n8((sensorRatio2+sensorRatio4)/2);
  
  float sFreq = 0.2*(sensorFreq1+sensorFreq2+sensorFreq3+sensorFreq4);
  
  aSin.setFreq(sFreq);
  aSin1.setFreq(kDelay.next(sFreq));
  aSin2.setFreq(kDelay.read(echo_cells_2));
  aSin3.setFreq(kDelay.read(echo_cells_3));  
  
//  // convert midi to frequency
//  Q16n16 midi_note = Q8n0_to_Q16n16(last_note); 
//  carrier_freq = Q16n16_to_Q24n8(Q16n16_mtof(midi_note));
//
  carrier_freq = float_to_Q24n8(sFreq);

//  // calculate modulation frequency to stay in ratio with carrier
  mod_freq = (carrier_freq * ratio)>>8; // (Q24n8   Q8n8) >> 8 = Q24n8
//
//    // set frequencies of the oscillators
//  aCarrier.setFreq_Q24n8(carrier_freq);

//  mod_freq = float_to_Q24n8(440.0f);
  aModulator.setFreq_Q24n8(mod_freq);
  
}


int updateAudio(){
  long mod = 128u+ aModulator.next();
  return (
  
  (mod*
  
  (3*((int)aSin.next()+aSin1.next()+(aSin2.next()>>1)+(aSin3.next()>>2)) >>3)
 
  >>8)
  
  *volume)>>8;
}


void loop(){
  audioHook();
 
}
