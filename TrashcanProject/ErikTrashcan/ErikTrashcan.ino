/* Wav File Player For E.C.
    Plays Wav files at random triggered by two
    HC-SR04 sensors
*/

// make sure these libraries are only included once!
#include <Audio.h>
#include <Wire.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <SPI.h>
#include <SD.h>

//define ACCELEROMETER  // uncomment this to use the accelerometer

// GUItool: begin automatically generated code

AudioPlaySdWav           playSdWav1;     //xy=89,39.00000762939453
AudioPlaySdWav           playSdWav2;     //xy=90,82.00000762939453
AudioPlaySdWav           playSdWav3;     //xy=86,146
AudioPlaySdWav           playSdWav4;     //xy=86,146
AudioMixer4              mixer2;         //xy=275,143
AudioPlaySdRaw           playSdRaw1;     //xy=261.1666717529297,4109.16667175293
AudioMixer4              mixer1;         //xy=281,64
AudioPlayMemory          playMem1;       //xy=305.1666717529297,4228.16667175293
AudioMixer4              mixer3;         //xy=424,91.00000762939453
AudioOutputI2S           i2s1;           //xy=590,75AudioConnection          patchCord1(playSdWav3, 0, reverb1, 0);
AudioConnection          patchCord2(playSdWav3, 0, mixer2, 0);
AudioConnection          patchCord4(playSdWav3, 1, mixer2, 1);
AudioConnection          patchCord15(playSdWav4, 0, mixer2, 2);
AudioConnection          patchCord16(playSdWav4, 1, mixer2, 3);
AudioConnection          patchCord5(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord6(playSdWav1, 1, mixer1, 1);
AudioConnection          patchCord7(playSdWav2, 0, mixer1, 2);
AudioConnection          patchCord8(playSdWav2, 1, mixer1, 3);
AudioConnection          patchCord11(mixer2, 0, mixer3, 1);
AudioConnection          patchCord12(mixer1, 0, mixer3, 0);
AudioConnection          patchCord13(mixer3, 0, i2s1, 0);
AudioConnection          patchCord14(mixer3, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=427,172
// GUItool: end automatically generated code


// Use these with the audio adaptor board
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

const int speed_of_sound_uS_CM = 29;     // speed of sound microseconds per centimeter
unsigned int SonarCloseness;
unsigned int duration, lastDuration;
unsigned int distance_CM;

const int trigPin1 = 0;
const int echoPin1 = 1;
const int trigPin2 = 3;
const int echoPin2 =  4;

const unsigned long maxDuration = 1400; // around 10 feet
//                                      // the sensor gets flaky at greater distances.

int lastVolumePotVal;
int fileIndex;
unsigned int lastPlayMillis;  //use a long for Arduino Uno, Mega etc
int rangeStart = 0, rangeSCStart = 0, startStopEnable = 0, playArrayStart = 0;
int fileNos;
int distReset1, distReset2, lastDistReset1, lastDistReset2;  // to track whether sensors are blocked

int randomHat(int numberInHat);  // stupid prototype - why?

char fileNameArray[100][3]; // 100 two-digit text numbers
char playFileName[8];

int randArray[50];

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("start");
#ifdef ACCELEROMETER
  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1);
  }
#endif

#ifdef ACCELEROMETER
  Serial.println("LIS3DH found!");
  lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
#endif


  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);


  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(40);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);

  // set up mixer volumes -- see the button tutorial image file for the layout
  mixer1.gain(0, 0.9);
  mixer1.gain(1, 0.9);
  mixer1.gain(2, 0.9);
  mixer1.gain(3, 0.9);
  mixer2.gain(0, 0.5);
  mixer2.gain(1, 0.5);
  mixer2.gain(2, 0.5);
  mixer2.gain(3, 0.5);
  mixer3.gain(0, 0.5);
  mixer3.gain(1, 0.5);
  mixer3.gain(2, 0.5);
  mixer3.gain(3, 0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    // no SD card found, stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      Serial.println("Check to see that you are not using pins 6,7,9,11,12,13,14,18,19,22,23");
      Serial.println("Check the soldering on all Teensy and Audio Shield pins");
      delay(2000);
    }
  }
  else {
    // Read the contents of the SD card
    File root = SD.open("/");
    root.rewindDirectory();
    listFiles(root);
    root.close();
  }

  Serial.print("Number of files on SD card = ");
  Serial.println(fileNos);

  delay(2000); // to read information above
}

/**************** loop start ***************/
void loop() {
  int distance1 =  HC_SRO4read(1);
  delayMicroseconds(300);
  int distance2 =   HC_SRO4read(2);

  if (distance1 < 1200) {
    if (distReset1 == 1) {
      Serial.print("distance1 ");
      Serial.println(distance1);
    }
    distReset1 = 0;
  }
  else {
    distReset1 = 1;
  }

  if (distance2 < 1200) {
    if (distReset2 == 1) {
      Serial.print("distance2 ");
      Serial.println(distance2);
      distReset2 = 0;
    }
  }
  else {
    distReset2 = 1;
  }
  //  Serial.println();

  if ((distance1 < 1200) || (distance2 < 1200)) {
    if (!playSdWav1.isPlaying()) {          // comment this back in if you don't want to interupt the file (retrigger)
      int rh = randomHat(5);
      parseFileName(rh);
      playSdWav1.play(playFileName);
      delay(5); // short delay seems to be necessary or it skips files
      Serial.print("playing file ");
      Serial.println(rh);
      delay(1000);
      distReset1 = 1; // reset files since play represents a cycle
      distReset2 = 1;
    }
  }
}
/****************** loop end ******************/



int smoothed(int input) {
  const int numReadings = 20;

  // Define the number of samples to keep track of. The higher the number, the
  // more the readings will be smoothed, but the slower the output will respond to
  // the input. Using a constant rather than a normal variable lets us use this
  // value to determine the size of the readings array.

  static int readings[20];               // the readings from the analog input
  static int readIndex = 0;              // the index of the current reading
  static int total = 0;                  // the running total
  int average = 0;                       // the average
  static int initialized = 0;

  if (!initialized) {  // make sure array is zeroed out
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readings[thisReading] = 0;
      initialized = 1;
    }
  }

  // this code has one clever trick in it - normally to get an average of twenty elements,
  // we would add up all the elements, and then divide by twenty.
  // But because we know what all the elements are - we can keep track of a running total without
  // having to add up all the elements. Insted we just swap out the one value that changes each time.
  // This saves the computer from having to add up all the elements each time through the loop

  if (initialized) {
    total = (total - readings[readIndex]); // subtract the last reading:
    readings[readIndex] = input;             // input new data into the array
    total = total + readings[readIndex];     // add the reading to the total:
    readIndex = readIndex + 1;               // advance to the next position in the array:

    // if we're at the end of the array...
    if (readIndex >= numReadings) {
      // ...wrap around to the beginning:
      readIndex = 0;
    }

    // calculate the average:
    average = total / numReadings;
    // send it to the computer as ASCII digits
    //    Serial.print("input " );
    //    Serial.print(input);
    //    Serial.print("  total " );
    //    Serial.print(total);
    //    Serial.print("  average = ");
    //    Serial.println(average);
    // delay(1);        // delay in between reads for stability

    return average;
  }
}


int HC_SRO4read(int sensNo) {
  // the sensor gets flaky at greater distances.

  if (sensNo == 1) {
    digitalWrite(trigPin1, HIGH);
    delayMicroseconds(4);
    digitalWrite(trigPin1, LOW);
    delayMicroseconds(300);  // wait as long as possible for transmit transducer to stop ringing
    // before looking for a return pulse
    // if you get zeros in your output reduce 300 by a little, say 250
    duration = pulseIn(echoPin1, HIGH, maxDuration);
    // third parameter is the timeout - the sensor
    // has a really long timeout that can slow down your
    // loop significantly if sensor doesn't find an echo
    if (duration == 0) duration = maxDuration;  // eliminate zero if sensor times out
  }

  if (sensNo == 2) {
    digitalWrite(trigPin2, HIGH);
    delayMicroseconds(4);
    digitalWrite(trigPin2, LOW);
    delayMicroseconds(300);  // wait as long as possible for transmit transducer to stop ringing
    // before looking for a return pulse
    // if you get zeros in your output reduce 300 by a little, say 250
    duration = pulseIn(echoPin2, HIGH, maxDuration);
    // third parameter is the timeout - the sensor
    // has a really long timeout that can slow down your
    // loop significantly if sensor doesn't find an echo
    if (duration == 0) duration = maxDuration;  // eliminate zero if sensor times out
  }

  return duration;
  //  Serial.print(duration);
  //  Serial.print("\t"); // print tab
  //  Serial.print(distance_CM);
  //  Serial.print("\t"); // print tab
  //  Serial.println(noiseFilter(duration));   // noise filter uses raw duration val, return CM

  // delay(50);  // use at least 50 ms delay
}



/* RandomHat
  Paul Badger 2007 - updated for Teensy compile 2017
  choose one from a hat of n consecutive choices each time through loop
  Choose each number exactly once before reseting and choosing again
*/

#define randomHatStartNum 0  // starting number in hat
#define randomHatEndNum 25    // ending number in hat - end has to be larger than start  


int randomHat(int numberInHat) {
  int thePick;		//this is the return variable with the random number from the pool
  int theIndex;
  static int currentNumInHat = 0;


  if  (currentNumInHat == 0) {                  // hat is emply - all have been choosen - fill up array again
    for (int i = 0 ; i < numberInHat; i++) {    // Put 1 TO numberInHat in array - starting at address 0.
      if (randomHatStartNum < randomHatEndNum) {
        randArray[i] = randomHatStartNum + i;
      }
    }
    currentNumInHat = abs(numberInHat);   // reset current Number in Hat
    // Serial.print(" hat is empty ");
    // if something should happen when the hat is empty do it here
  }

  theIndex = random(currentNumInHat);	                 //choose a random index from number in hat
  thePick = randArray[theIndex];
  randArray[theIndex] =  randArray[currentNumInHat - 1]; // copy the last element in the array into the the empty slot
  //                                                     // as the the draw is random this works fine, and is faster
  //                                                     // the previous version. - from a reader suggestion on this page
  currentNumInHat--;    // decrement number in hat
  return thePick;
}

void listFiles(File dir) {
  fileNos = 0;
  while (true){
    File entry = dir.openNextFile();
    if (!entry) {
      Serial.println("NO MORE FILES!");
      // no more files
      Serial.print("fileNos " );
      Serial.println(fileNos);
      for (int i = 0; i < fileNos; i++) {
        Serial.print(fileNameArray[i]);
        Serial.println(", ");
      }
      Serial.println();
      break;
    }
    else {
      // convert to string to make it easier to work with...
      String entryName = (String)entry.name();
      Serial.println(entryName);

      if ((entryName.charAt(0) >= '0') && (entryName.charAt(0) <= '9')) {
        fileNameArray[fileNos][0] = (char)entryName.charAt(0);

        if ((entryName.charAt(1) >= '0') && (entryName.charAt(1) <= '9')) {
          fileNameArray[fileNos][1] = (char)entryName.charAt(1);
        }
        fileNos++;
      }
      entry.close();
    }
  }
}

void parseFileName(int arrNo) {
  //file names are 8.3 format - eight characters only, '.' , "wav" extension
  for (int i = 0; i < 8; i++) {
    playFileName[i] = 0;
  }
  int charNo = 0;
  for (int i = 0; i < 2; i++) {
    if ((fileNameArray[arrNo][i] >= '0') && (fileNameArray[arrNo][i] <= '9')) {
      playFileName[i] = fileNameArray[arrNo][i];
      charNo++;
    }
  }
  playFileName[charNo] = '.';
  playFileName[charNo + 1] = 'w';
  playFileName[charNo + 2] = 'a';
  playFileName[charNo + 3] = 'v';
  Serial.println(playFileName);
}


