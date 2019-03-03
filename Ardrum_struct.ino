#define midichannel 1       // MIDI channel from 0 to 15 (+1 in "real world")
#define pins 15
#define sensibility 700
#define hitadd 100

boolean VelocityFlag = true; // Velocity ON (true) or OFF (false)
boolean HHPedalKickFlag = true;

//*******************************************************************************************************************
// Pad struct
//*******************************************************************************************************************
typedef struct
{
  int Pin; //Arduino pin
  byte Note; //MIDI notes from 0 to 127 (Mid C = 60)
  int CutOff; //Minimum Analog value to cause a drum hit
  int MinimumValue; //MInimum MIDI value
  int MaxPlayTime; //Cycles before a 2nd hit is allowed 
  boolean Digital; //Digital or Analog
  boolean Enable; 

  boolean activePad; //pad currently playing (1)
  int PinPlayTime; //Counter since pad started to play (0)
} Pad;

//*******************************************************************************************************************
// User settable variables
//*******************************************************************************************************************
int generalThreshold = 10; //mass

Pad pads[pins] = {
  //pin   Note    CutOff  MinValue    MaxTime   Digital   Enabled
  { A0,   60,     5,      60,         60,        0,       1,        1, 0 }, //int hihat   = 0;
  { 4,    44,     50,     60,         100,       1,       1,        1, 0 }, //int hhPedal = 1;
  { A1,   38,     5,      90,         60,        0,       1,        1, 0 }, //int snare   = 2;
  { A2,   47,     20,     100,        60,        0,       1,        1, 0 }, //int tomb1   = 3;
  { A3,   45,     10,     100,        60,        0,       1,        1, 0 }, //int tomb2   = 4;
  { A4,   43,     10,     100,        60,        0,       1,        1, 0 }, //int tomb3   = 5;
  { A6,   49,     20,     100,        60,        0,       1,        1, 0 }, //int crash1  = 6;
  { A7,   57,     20,     100,        60,        0,       1,        1, 0 }, //int crash2  = 7; //hay q soldar
  { 2,    50,     0,      0,          200,       1,       1,        1, 0 }, //int choke1  = 8;
  { 3,    58,     0,      0,          200,       1,       false,    1, 0 }, //int choke2  = 9; //falta espacio pa conectar
  { A8,   51,     10,     60,         60,        0,       1,        1, 0 }, //int ride    = 10;
  { A5,   53,     30,     80,         60,        0,       1,        1, 0 }, //int bell    = 11;
  { A9,   36,     180,    100,        100,       0,       1,        1, 0 }, //int kick1   = 12;
  { 4,    36,     0,      0,          250,       1,       false,    1, 0 }, //int kickHH  = 13;
  { 8,    0,      0,      0,          1000,      1,       1,        1, 0 }  //int switchHHK  = 14;
};

int hihat=0, hhPedal=1, snare=2, tomb1=3, tomb2=4, tomb3=5, crash1=6, crash2=7, choke1=8, choke2=9, ride=10, bell=11, kick1=12, kickHH=13, switchHHK=14; 

//*******************************************************************************************************************
// Internal Use Variables
//*******************************************************************************************************************
int pinRead;
byte status1;
int pin = 0;     
int hitavg = 0;

//*******************************************************************************************************************
// Setup
//*******************************************************************************************************************
void setup() 
{
  Serial.begin(115200); // connect to the serial port 115200

  for(int pin = 0; pin < pins; pin++)  
  {
    if (HHPedalKickFlag == true)
    {
      pads[hhPedal].Enable = false;
      pads[kickHH].Enable = true;
    }
    
    if (pads[pin].Digital == false) pads[pin].CutOff += generalThreshold; 
    else if (pads[pin].Enable) pinMode(pads[pin].Pin, INPUT);
  }
}

//*******************************************************************************************************************
// Main Program
//*******************************************************************************************************************

void loop() 
{
  
  if (HHPedalKickFlag == true)
  {
    pads[hhPedal].Enable = false;
    pads[kickHH].Enable = true;
  }
  else
  {
    pads[hhPedal].Enable = true;
    pads[kickHH].Enable = false;
  }
  
  for(int pin = 0; pin < pins; pin++)  
  {  
    if (pads[pin].Enable == true)
    {
      hitavg = read(pin);

      // read the input pin
      if(hitavg > pads[pin].CutOff)
      {
        if((pads[pin].activePad == false))
        {
          if(VelocityFlag == true)
          {           
            if      (pin == ride)                     hitavg = Ride(hitavg, pin);
            else if (pin == choke1 || pin == choke2)  hitavg = Choke(hitavg, pin);
            else if (pin == kickHH)                   hitavg = HHPedalKick(hitavg, pin);
            else                                      hitavg = Normal(hitavg, pin);
            
            if (hitavg > 127) {hitavg = 127;} //Upper range
            else if (hitavg < pads[pin].MinimumValue) {hitavg = pads[pin].MinimumValue;} //Lower range
          }
          else
          {
            hitavg = 127;
          }
  
          MIDI_TX(144,pads[pin].Note,hitavg); //note on
  
          pads[pin].PinPlayTime = 0;
          pads[pin].activePad = true;
        }
        else
        {
          pads[pin].PinPlayTime ++;
        }
      }
      else if((pads[pin].activePad == true))
      {
        pads[pin].PinPlayTime ++;
        if(pads[pin].PinPlayTime > pads[pin].MaxPlayTime)
        {
          pads[pin].activePad = false;
          MIDI_TX(144,pads[pin].Note,0);
        }
      }
    }
  } 
}

//*******************************************************************************************************************
// HHPedal
//*******************************************************************************************************************
int HHPedalStatus(int hitavg, int pin)
{
  //open
  if (hitavg == LOW) 
  {
     pads[hihat].Note = 60;
     return 0;    
  }
  
  //close
  hitavg = 0;
  if (pads[hihat].Note == 60) 
  {
    hitavg = 100;
  }

  pads[hihat].Note = 62;
  return hitavg;
}

int HHPedalKick(int hitavg, int pin)
{
  if (hitavg == HIGH) 
  {
     return 120;    
  }
  return 0;
}


//*******************************************************************************************************************
// Choke
//*******************************************************************************************************************
int Choke(int hitavg, int pin)
{
  if (hitavg == HIGH) 
  {
     return 1;    
  }
  return 0;
}

//*******************************************************************************************************************
// Normal
//*******************************************************************************************************************
int Normal(int hitavg, int pin)
{
  //Serial.print("hitavg = ");Serial.println(hitavg);
  //Serial.print("hitavg + hitadd + 1 - pads[pin].CutOff = ");Serial.println(hitavg + hitadd + 1 - pads[pin].CutOff);
  
  return log (hitavg + hitadd + 1 - pads[pin].CutOff) / log(1024 - sensibility) * 127;
}

//*******************************************************************************************************************
// Ride
//*******************************************************************************************************************
int Ride(int hitavg, int pin)
{
    hitavg = log (hitavg + hitadd + 1 - pads[pin].CutOff) / log(1024 - sensibility) * 127;
    
    if (hitavg <= 127) 
    {
      pads[pin].Note = 51;
      return hitavg;
    }
    else
    {
      pads[pin].Note = 59;
      return hitavg * 0.8 + 10;
    }
}

//*******************************************************************************************************************
// switchHHKStatus
//*******************************************************************************************************************
int switchHHKStatus(int hitavg, int pin)
{
  if (hitavg == LOW) HHPedalKickFlag = false;
  else HHPedalKickFlag = true;
  return 0;
}

//*******************************************************************************************************************
// Read Analog and Digital
//*******************************************************************************************************************
int read(int pin)
{
  int hitavg;
  if (pads[pin].Digital == true)
  {
    hitavg = digitalRead(pads[pin].Pin);
    if (pin == hhPedal) hitavg = HHPedalStatus(hitavg, pin);
    else if (pin == switchHHK) hitavg = switchHHKStatus(hitavg, pin);
  }
  else
  {
    hitavg = analogRead(pads[pin].Pin);
  }
  return hitavg;
}

//*******************************************************************************************************************
// Transmit MIDI Message
//*******************************************************************************************************************
void MIDI_TX(byte MESSAGE, byte PITCH, byte VELOCITY) 
{
  status1 = MESSAGE + midichannel;
  Serial.write(status1);
  Serial.write(PITCH);
  Serial.write(VELOCITY);
}