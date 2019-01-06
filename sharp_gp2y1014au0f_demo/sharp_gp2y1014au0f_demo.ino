////////////////////////////////////////////////////////////////////////////
// Sharp GP2Y1014AU0F Dust Sensor
//
// Original code from "sharpsensoruser" on https://github.com/sharpsensoruser/ - NICE WORK
// There is also a detailed description how the sensor work and some background on dust sensing https://github.com/sharpsensoruser/sharp-sensor-demos/wiki
//
// Modification to the program by "Igor Mesarec" Jan 2018
// igormesarec/sharp-sensor-demos
//
// Connections on GP2Y1014
// Pin 1 - starting from the pin closer to the sensor edge
// Pin 1 - V-Led
// Pin 2 - LED-GND
// Pin 3 - LED
// Pin 4 - S-GND
// Pin 5 - Vo
// Pin 6 - Vcc
//
// Board Connection:
//   GP2Y1014    Arduino
//   V-LED       Between *R1 (150ohms) and **C1 (150uF) to 5v (arduino)
//   LED-GND     C1 and GND
//   LED         Pin 7
//   S-GND       GND
//   Vo          A5
//   Vcc         5V
// *R1 - is connected betwen V-LED and 5V
// **C1 - is connected betwen Led-GND and V-LED
//
// connection scheme:
// ──────────────────   GP1Y1014 Conector  ───────────────────
// ┌────┬───────────┬─────┬──────┬─────┬─────┬──────┐
//          1                    2         3           4         5          6
//       V─LED               LED─GND     LED       S─GND       Vo        Vcc
//          │                   │         │          │        │         │
//          │                   │         │          │        │         │
//          ├───(+)C1(─)───┤         │          │        │         │
//          │      [220uF]      │         │          │        │         │
//          │                   │         │          │        │         │
//          R1                   │         │          │        │         │
//      [150 ohms]               │         │          │        │         │
//          │                   │         │          │        │         │
//         5V                   GND      PIN 7       GND         A5         5V
// └────┴───────────┴─────┴──────┴─────┴─────┴──────┘
//  ──────────────────   ARDUINO (UNO)  ────────────────────
//
// Sensor information: dest sensing from 0 ug/m3 and 500 ug/m3
// Voc is 0.6 Volts for dust free acordind sensor spec
// sensitivity in units of V per 100ug/m3 (K = 0.5)--> 0.5 Volts/(100 ug/m3)
//
// LED Pulse Drive Circuit:
// LED pulsed on once every 10ms, pulse duration or width of 0.32ms. 
// Once the LED is turned on sample the resulting analog output voltage after 0.28ms   
//
// Serial monitor setting:
//   9600 baud
/////////////////////////////////////////////////////////////////////////////


// IGOR - dodaj da računa stanje tudi za o,6V Voc - in potem računa actual value 

// Choose program options.
//#define PRINT_RAW_DATA  //uncomet if you wont raw data displayed in serial monitor
#define USE_AVG   //calculate average value in N mesared values

// Arduino pin numbers.
const int sharpLEDPin = 7;   // Arduino digital pin 7 connect to sensor LED.
const int sharpVoPin = A5;   // Arduino analog pin 5 connect to sensor Vo.

// For averaging last N raw voltage readings.
#ifdef USE_AVG
#define N 100
static unsigned long VoRawTotal = 0;
static int VoRawCount = 0;
#endif // USE_AVG

// Set the typical output voltage in Volts when there is zero dust. 
static float Voc = 0.6;  //Voc is 0.6 Volts for dust free acordind sensor spec
static float VocT = 0.6;  //Self calibration overvrite the Coc with minimum measered value - this variable you need to have the treshold for the sensor  (facturi calibration)

// Use the typical sensitivity in units of V per 100ug/m3.
const float K = 0.5;
  
/////////////////////////////////////////////////////////////////////////////

// Helper functions to print a data value to the serial monitor.
void printValue(String text, unsigned int value, bool isLast = false) {
  Serial.print(text);
  Serial.print("=");
  Serial.print(value);
  if (!isLast) {
    Serial.print(", ");
  }
}
void printFValue(String text, float value, String units, bool isLast = false) {
  Serial.print(text);
  Serial.print("=");
  Serial.print(value);
  Serial.print(units);
  if (!isLast) {
    Serial.print(", ");
  }
    else {
    Serial.println();
  }
}

/////////////////////////////////////////////////////////////////////////////

// Arduino setup function.
void setup() {
  // Set LED pin for output.
  pinMode(sharpLEDPin, OUTPUT);
  
  // Start the hardware serial port for the serial monitor.
  Serial.begin(9600);
  
  // Wait two seconds for startup.
  delay(2000);
  Serial.println("");
  Serial.println("GP2Y1014AU0F Demo");
  Serial.println("=================");
  VocT = Voc; //corect the VocT (to have alwas data for the initioal Voc
}

// Arduino main loop.
void loop() {  
  // Turn on the dust sensor LED by setting digital pin LOW.
  digitalWrite(sharpLEDPin, LOW);

  // Wait 0.28ms before taking a reading of the output voltage as per spec.
  delayMicroseconds(280);

  // Record the output voltage. This operation takes around 100 microseconds.
  int VoRaw = analogRead(sharpVoPin);
  
  // Turn the dust sensor LED off by setting digital pin HIGH.
  digitalWrite(sharpLEDPin, HIGH);

  // Wait for remainder of the 10ms cycle = 10000 - 280 - 100 microseconds.
  delayMicroseconds(9620);
  
  // Print raw voltage value (number from 0 to 1023).
  #ifdef PRINT_RAW_DATA
  printValue("VoRaw", VoRaw, true);
  Serial.println("");
  #endif // PRINT_RAW_DATA
  
  // Use averaging if needed.
  float Vo = VoRaw;
  #ifdef USE_AVG
  VoRawTotal += VoRaw;
  VoRawCount++;
  if ( VoRawCount >= N ) {
    Vo = 1.0 * VoRawTotal / N;
    VoRawCount = 0;
    VoRawTotal = 0;
  } else {
    return;
  }
  #endif // USE_AVG

  // Compute the output voltage in Volts.
  Vo = Vo / 1024.0 * 5.0;
  printFValue("Vo", Vo*1000.0, "mV");
  printFValue("Voc", Voc*1000.0, "mV"); // Serial print the runtime min value for the Voc - minimal measered value of the Voc during the runtime - this is 0 dust

  //------------ Dust density calculated by Voc set in the begining - like in sensor spec  ------------
  // Calculate Dust Density in units of ug/m3 for the initial Voc (as in sensor specs)
  float dV = Vo - VocT;
  if ( dV < 0 ) {
    dV = 0;
  }
  float dustDensity = dV / K * 100.0;
  String SPT1 = ""; // add this to serial print the set Voc
  SPT1 = ("Dust Density(Voc ");
  SPT1 += VocT;
  SPT1 += (" V)");
  
  printFValue(SPT1, dustDensity, "ug/m3", false); // Print the values
  dV = 0; // Reset the dV to 0 for the next calculation
  dustDensity = 0; // Reset the dustDensity to 0 for the next calculation
  
  //------------ Dust density calculated by lowest output voltage during runtime  ------------
  // Convert to Dust Density in units of ug/m3. 
  // During runtime, the Voc value is updated dynamically whenever a lower output voltage is sense. 
  // this cover the dust sensing below the sensors specefied range from 0 to Voc (0,6V) as specified in the beginin
  dV = Vo - Voc;
  if ( dV < 0 ) {
    dV = 0;
    Voc = Vo;
  }
  dustDensity = dV / K * 100.0;
  SPT1 = "";
  SPT1 = ("Dust Density(Voc ");
  SPT1 += Voc;
  SPT1 += (" V)");
  printFValue(SPT1, dustDensity, "ug/m3", true);
  Serial.println("");
  
} 
