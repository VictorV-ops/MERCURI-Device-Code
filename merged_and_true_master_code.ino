//Arduino code for MERCURI device (v2 pins and setup, which is simplified and different from v1)
// Modified code from a lot of places
//based 
//23 October-2025
// Daisy Aguilar, Victor Vargas 

// libraries required for the code 
#include <OneWire.h> 
#include <DallasTemperature.h> 
#include <Wire.h> 
#include <Adafruit_AS7341.h> 

//Defining inputs pins of the relay & temprature sensors
const int RELAY_PIN = 4; 
const int oneWireBus = 8; 

//initiating 12IC, temperature connection, and array for spectro values
OneWire oneWire(oneWireBus); 
DallasTemperature sensors(&oneWire); 
Adafruit_AS7341 as7341; 
uint16_t readings[12]; //apparently the array has to be exactly like this to prevent weird behaviors

//declare var and var types
double temperature1;
double State = 1; // State determines the "phase" of the 30 min cycle for output tracking purposes
bool heatState1 = true; // can be useful if checking heater is on (relay is on)
bool flagState = true; // so the integral reset at the setpoint change only happens once

//setting up the PID controller relevant parameters for one made from scratch. as in this code does not use a library for pid control. no good reason why though.
double previousMillis1, previousMillis2 = 10000; 
unsigned long interval_cycle = 10000; // the pid works on a roughish 10 second cycle, with values being calculated that often
unsigned long outputToUnLong; 
double dt, last_time;
double last_read = 0;  
double integral1, previous1, output1 = 0; 
double kp1, ki1, kd1; 
double setpoint1 = 30; // change as needed, but nominal start is 30
double Pterm1, Iterm1, Dterm1, ItermDiff1; 
 
void setup() { 
  //Values of PID controllers which can be tuned to optimize system
  kp1 = 0.5; 
  ki1= 0.004; 
  kd1 = 0.01; 
  last_time = 0; 

  //starts spectro and sets parameters which can also be tuned to determine sensitivity and integration time for spectrometer, needs optimization for a given system
  //as of 10/23 the issue where these parameters interfered with code clock and therefore PID, have been resolved
  Serial.begin(115200);
  Wire.begin();  
  as7341.begin(); 
  as7341.setATIME(100); 
  as7341.setASTEP(999); 
  as7341.setGain(AS7341_GAIN_512X);

  //this initializes the relay and the temperature sensors, as well as declares some relevant pins for the led and fan functions, which can be turned off here, or later in the code
  sensors.begin(); 
  pinMode(RELAY_PIN, OUTPUT); 
  digitalWrite(RELAY_PIN, LOW); 
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT); 
  digitalWrite(2, HIGH); //led
  digitalWrite(3, LOW); //fan 

  //delay(5000);
  // this kicks off the very first spectral reading
  as7341.startReading();
}

// the loop controls the active function of the device and the basic gist is that every time it runs, it CHECKS a few things based on the clock time and DOES things if certain thresholds are met
void loop() { 
  unsigned long now = millis(); // taking the timestamp at the start of each loop

  bool timeOutFlag = yourTimeOutCheck();
  if(as7341.checkReadingProgress() || timeOutFlag ) //this if checks if the reading that was previously triggered has finished. if so it gets the readings and stores them in the array. it also starts another reading.
  {
    if(timeOutFlag)
    {} //Recover/restart/retc.

    //Serial.println("\nAha, the reading we started a few cycles back is finished, here it is:");
    //IMPORTANT: make sure readings is a uint16_t array of size 12, otherwise strange things may happen
    as7341.getAllChannels(readings);  //Calling this any other time may give you old data
    //printReadings();

    //Serial.println("Guess we'll start another reading right away but do some work in the meantime\n");
    as7341.startReading();
  }

  // pull temp from ds18b20
  sensors.requestTemperatures(); 
  float rawTemp1 = sensors.getTempCByIndex(0);

   //Adjusting temperature of sensor due to offset recommended by Daisy
  float offset1 = 0.5; 
  temperature1 = rawTemp1 + offset1;

  //PID calculation every roughly 10s cycle
  if (now - last_time >= interval_cycle){ 
    dt = (now - last_time); 
    last_time = now; 
    double actual1 = temperature1; 
    double error1 = setpoint1 - actual1; 
    output1 = pid1(error1);
    outputToUnLong = output1;  
    digitalWrite(RELAY_PIN, HIGH); 
    }
    
  //looping implementation of PID, basically if time in loop has passed calculated output it tells the heater to turn off for whats left of the cycle
      double diff1 = now - last_time; //check time in cycle
      if (diff1 >= outputToUnLong){   // check if time in cycle is passed the threshold
        digitalWrite(RELAY_PIN, LOW); 
        }

  if(last_read - readings[3] >= 30){  // this is for the purpose of eliminating (display-wise) the data artefact caused by the weird signal deepression (might actually be LED side not spectro-side)
    readings[3] = last_read;          // has the unintended effect of really smoothing out the signal during cycles with high relay-on time (high deprssion)
  }
         
  Serial.print(now); //this prints the ongoing clock timer in millis and it starts the moment the board is powered on
  Serial.print(",");
  Serial.print(readings[3]); // this prints our desired f4 515 nm channel, note: the subtracted background is variable
  Serial.print(","); 
  //if(State == 1 || State == 3){ 
    //Serial.print(as7341.getChannel(AS7341_CHANNEL_480nm_F3)); 
    //} else{ Serial.print(" "); 
    //} 
   //Serial.print(","); 
   //if(State == 1 || State == 3){ 
    //Serial.print(as7341.getChannel(AS7341_CHANNEL_515nm_F4)); 
    //} else{ Serial.print(" "); 
    //} 
   //Serial.print(","); 
   //if(State == 1 || State == 3){ 
    //Serial.print(as7341.getChannel(AS7341_CHANNEL_555nm_F5)); 
    //} else{ Serial.print(" "); 
    //} 
   //Serial.print(","); 
   //if(State == 1 || State == 3){ 
    //Serial.print(as7341.getChannel(AS7341_CHANNEL_CLEAR)); 
    //} else{ Serial.print(" "); 
    //} 
   //Serial.print(","); 
   Serial.print(temperature1); // this prints not the temperature
   Serial.print(","); 
   Serial.println(outputToUnLong);// this prints the output calculated by the pid loop, just to keep track of it if needed

   last_read = readings[3]; // to keep track of the previous reading, after it has printed out the data

  // setpoint switching at 10 min and state determination for serial data printout
  if(now > 600000 && flagState == true){ 
    setpoint1 = 53.00; // note remember to test with this like 1.5-2 deg lower coz of the channel probe test was like 64 and prolly too hot
    integral1 = 0; 
    kp1 = 0.9; 
    ki1 = 0.01; 
    kd1 = 0.0; 
    State = 2;
    flagState = false; 
    } 
  if(temperature1 > 60){ 
    State = 3; 
    } 
 } 

//PID controller function, which is honestly likely subpar from a library because I do use some approx (riemann sum the integral) and have no knowledge of code optimization, but it works well i think
// it sets output limits and deals with the issue of integral accumulation
double pid1(double error1) { 
  double proportional1 = error1 * 1000; 
  integral1 += error1 * dt; 
  double derivative1 = (error1 - previous1) / dt; 
  previous1 = error1; 
  double Pterm1 = kp1 * proportional1; 
  double Iterm1 = ki1 * integral1; 
  double Dterm1 = kd1 * derivative1; 
  double output1 = Pterm1 + Iterm1 + Dterm1; 
  if (output1 > 9000){ 
    if (ki1 > 0){ 
      ItermDiff1 = output1 - 9000; 
      integral1 -= ItermDiff1 / ki1; 
      } 
      output1 = 9000; 
      } else{ output1 = output1; 
      } 
   if (output1 < 1000){ 
    if (ki1 > 0){ 
      ItermDiff1 = 1000 - output1; 
      integral1 += ItermDiff1 / ki1; 
      } 
      output1 = 1000; 
      } else{ output1 = output1; 
      } 
      return output1; 
      }

bool yourTimeOutCheck()
{
  //Fill this in to prevent the possibility of getting stuck forever if you missed the result, or whatever. not currently in use yet but might be helpful so am keeping it.
  // functions not in use were mostly copied from adafruit github or leftover from previous versions so thats why theyre here and not in use
  return false;
}

// not used in actual operation but since it prints all the channels, it is useful for integration time and sensitivity calibrations once we get to that step.
void printReadings()
{
  Serial.print("ADC0/F1 415nm : "); //most of these will be commented out during normal operation
  Serial.println(readings[0]);
  Serial.print("ADC1/F2 445nm : ");
  Serial.println(readings[1]);
  Serial.print("ADC2/F3 480nm : ");
  Serial.println(readings[2]);
  Serial.print("ADC3/F4 515nm : ");
  Serial.println(readings[3]);
  Serial.print("ADC0/F5 555nm : ");

  /* 
  // we skip the first set of duplicate clear/NIR readings
  Serial.print("ADC4/Clear-");
  Serial.println(readings[4]);
  Serial.print("ADC5/NIR-");
  Serial.println(readings[5]);
  */
  
  Serial.println(readings[6]);
  Serial.print("ADC1/F6 590nm : ");
  Serial.println(readings[7]);
  Serial.print("ADC2/F7 630nm : ");
  Serial.println(readings[8]);
  Serial.print("ADC3/F8 680nm : ");
  Serial.println(readings[9]);
  Serial.print("ADC4/Clear    : ");
  Serial.println(readings[10]);
  Serial.print("ADC5/NIR      : ");
  Serial.println(readings[11]);
}
