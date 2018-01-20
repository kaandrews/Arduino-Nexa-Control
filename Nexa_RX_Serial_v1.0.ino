/*
 * The purpose of this sketch is to receive and decode signals from the Nexa
 * brand of products, including remote control dimmers and switches.
 * 
 * The protocol looks like this:
 * Protocol information from: http://tech.jolowe.se/home-automation-rf-protocols/
 * 
 * Sent packet description:
 * S HHHH HHHH HHHH HHHH HHHH HHHH HHGO CCUU P
 * 
 * S = Sync bit.
 * H = The first 26 bits are transmitter unique codes, and it is this code that the receiver "learns" to recognize.
 * G = Group code. Set to 0 for on, 1 for off.
 * O = On/Off bit. Set to 0 for on, 1 for off.
 * C = Channel bits. 4 units per channel
 * Channel #1 = 11, #2 = 10, #3 = 01, #4 = 00.
 * U = Unit bits. Device to be turned on or off.
 * Unit #1 = 11, #2 = 10, #3 = 01, #4 = 00.
 * P = Pause bit.
 * 
 * For every button press several identical packets are sent. For my transmitters this was 5 or 8.
 * 
 * T = 250us
 * Sync bit is T high + 10T low
 * 1 bit is T high + T low
 * 0 bit is T high + 5T low
 * Pause bit is T high + 40T low
 * 
 * T  Duration (us)
 * 1  250
 * 5  1250
 * 10 2500
 * 40 10000
 */

// Arduino Pin Connections
const byte interruptPin = 2; // Connects to RX of 433 MHz receiver

// RF Receive parameters
const int iTus = 250; // microseconds pulse length
const int iTol = 70;  // microseconds tolerance for detection, works for my 2 transmitters

// Array for storing received timings
unsigned long lRX[20];
const int iRXSize = sizeof(lRX)/4;

// Variables for RX signal processing
unsigned int iCurPos = 0;     // Current position in the RX array
unsigned int iSyncCount = 0;  // Number of sync bits found so far
String sOutput = "";          // Stores raw binary RX data
boolean bEven = false;        // Used to store whether an odd or even numbered bit
boolean bStarted = false;     // Indicates if the RX has collected >=1 time point in lPrevUS
boolean bRun = false;         // Indicates if RX should run or not
//int iBitCount;

// Global Variables for storing decoded data
unsigned long lAddress = 0;
byte byGroup = 0;
byte byOn = 0;
byte byChannel = 0;
byte byUnit = 0;

// Serial connection
boolean newData = false;      // Indicates if new serial command is received
const byte numChars = 32;
char receivedChars[numChars]; // array to hold received data

void setup() {
  // RX pin, tried INPUT and INPUT_PULLUP, doesn't make much difference
  // A pull down resistor may work better
  pinMode(interruptPin, INPUT);

  // Serial configuration and commands intro
  Serial.begin(9600);
  Serial.flush();
  Serial.println("Commands: S = Start capture, T = Stop Capture, P = Print Data");
}

void loop() {
  // Receive commands by serial
  recvWithEndMarker();

  // Process Commands
  processSerial();

  // If a signal has been received terminate capture
  if (bStarted && !bRun){
    detachInterrupt(digitalPinToInterrupt(interruptPin));
    Serial.println("Signal captured");
    Serial.println("Commands: S = Start, T = Stop, P = Print");
    bStarted = false;
  }

}

// Serial communication section
void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
 
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    } else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void processSerial() {
  if (newData == true) {
    switch (receivedChars[0]){
    case 'S':
      // Start or restart receiving
      
      // Blank previous results
      for (int i=0; i<iRXSize; i++) {
        lRX[i] = 0;
      }

      iCurPos = 0;
      iSyncCount = 0;
      sOutput = "";
      
      Serial.println("Starting capture");
      Serial.println("Commands: S = Start, T = Stop, P = Print");
      bRun = true;
      attachInterrupt(digitalPinToInterrupt(interruptPin),rxSignal, CHANGE);
      break;
    case 'T':
      // Terminate capture manually
      bRun = false;
      bStarted = false;
      detachInterrupt(digitalPinToInterrupt(interruptPin));
      Serial.println("Terminating capture");
      Serial.println("Commands: S = Start, T = Stop, P = Print");
      break;
    case 'P':
      // Print collected data
      String sFilter = "";
      String sAddress = "";
      lAddress = 0;

      // Convert from transmission format to bits e.g. 01 = 0, 10 = 1
      sFilter = "";
      sAddress = "";
      
      // Take every other bit
      for (int i = 0; i < sOutput.length(); i += 2){
        sFilter = sFilter + sOutput.charAt(i);
      }
      
      // Get the 26 bits for the address
      sAddress = sFilter.substring(0,26);

      // Convert from binary string to long interger
      char cTempAddress[27];
      sAddress.toCharArray(cTempAddress,27);
      lAddress = strtol(cTempAddress,NULL,2);
      
      // Get the Group bit
      byGroup = sFilter.substring(26,27).toInt();

      // Get on/off bit
      byOn = sFilter.substring(27,28).toInt();

      // Channel bits
      if(sFilter.charAt(28) == '1'){
        if(sFilter.charAt(29) == '1'){
          byChannel = 1;
        } else {
          byChannel = 2;
        }
      } else {
        if(sFilter.charAt(29) == '1'){
          byChannel = 3;
        } else {
          byChannel = 4;
        }
      }

      // Unit bits
      if(sFilter.charAt(30) == '1'){
        if(sFilter.charAt(31) == '1'){
          byUnit = 1;
        } else {
          byUnit = 2;
        }
      } else {
        if(sFilter.charAt(31) == '1'){
          byUnit = 3;
        } else {
          byUnit = 4;
        }
      }

      /*
      // Use to see the stored buffer and current location for troubleshooting the bit lengths
      for (int i=0; i<iRXSize; i++) {
        Serial.print(i,DEC);
        Serial.print(": ");
        Serial.println(lRX[i],DEC);
      }

      Serial.print("Current Position: ");
      Serial.println(iCurPos,DEC);
      Serial.print("Sync bits: ");
      Serial.println(iSyncCount,DEC);
      Serial.println("Found: " + sOutput);
      */
      
      Serial.println("Binary: " + sFilter);
      Serial.print("Transmitter code: ");
      Serial.println(lAddress, DEC);
      Serial.print("Group: ");
      if(byGroup == 0){
        Serial.println("Yes");
      } else {
        Serial.println("No");
      }
      Serial.print("On/Off: ");
      if(byOn == 0){
        Serial.println("On");
      } else {
        Serial.println("Off");
      }
      Serial.print("Channel: ");
      Serial.println(byChannel);
      Serial.print("Unit: ");
      Serial.println(byUnit);
      Serial.println("Commands: S = Start, T = Stop, P = Print");
      break;
    }
    
    newData = false;
  }
}

// Interpret RX signal


// RF signal receive
void rxSignal(){
  
  static unsigned long lPrevUS;
  static int iBitCount;

  // If should be running
  if (bRun){
    if (!bStarted){
      // If this is the first time to run
      lPrevUS = micros();
      bStarted = true;
      iBitCount = 0;
    } else {
      // Calculate the time since last call and store
      unsigned long lCurUS = micros();
      unsigned long ldiffUS = lCurUS - lPrevUS;

      iCurPos = (iCurPos + 1) % iRXSize;
      
      lRX[iCurPos] = ldiffUS;

      lPrevUS = lCurUS;

      // If on the 2nd sync bit start to decode
      if (iSyncCount == 2){
        // Start to decode the bits after the 2nd Sync bit is received
        // The transmitters send multiple copies of the signal, typically 5 or 8
        
        // Check if too many bits received
        if (iBitCount <= 64) {
          // Invert Even/Odd bit marker
          bEven = !bEven;

          // If current bit is even then start to analyse
          if(bEven){
            // Check for a 1 found
            if(checkBit(iCurPos,1,1)){
              sOutput = sOutput + '1';
              iBitCount++;
            } else {
              // Check if a 0 found
              if(checkBit(iCurPos,1,5)){
                sOutput = sOutput + '0';
                iBitCount++;
              } else {
                // Check if an end bit is found and terminate
                if(checkEnd(iCurPos,1,40)){
                  // Terminate the capture
                  bRun = false;
                } else {
                  // Junk received, reset
                  bEven = false;
                  iSyncCount = 0;
                  iBitCount = 0;
                }
              }
            }
          }
        } else {
          // Too many bits before pause bit - Junk received, reset
          bEven = false;
          iSyncCount = 0;
          iBitCount = 0;
        }
      } else {
        // Check a sync bit found
        if (checkBit(iCurPos,1,10)){
          // Increment the counter
          iSyncCount++;
          
          // Record that this is the end of the sync bit (an even number bit)
          bEven = true;
        }
      }
    }
  }
}

// Detect if a bit matching the pattern supplied has been found
boolean checkBit(int iPos,int b1, int b2){
  boolean bResult = false;
  
  // Check if the current pulse is b2*T us and the previous is around b1*T us
  if (lRX[iPos] <= (b2 * iTus + b2 * iTol) && lRX[iPos] >= (b2 * iTus - b2 * iTol)){
    // Wrap back to end if at the start
    int iPrevPos = (iPos == 0) ? iRXSize-1 : iPos - 1;
    
    if (lRX[iPrevPos] <= (iTus + b1 * iTol) && lRX[iPrevPos] >= (iTus - b1 * iTol)){
      // Found a matching bit
      bResult = true;
    }
  }

  return bResult;
}

// Detect if an end Pause bit has been received
boolean checkEnd(int iPos,int b1, int b2){
  int iPrevPos = 0;
  boolean bResult = false;
  
  // Check if the current pulse is longer than b2*T us and the previous is b1*T us
  if (lRX[iPos] >= (b2*iTus-b2*iTol)){
    
    // Wrap back to end if at the start
    iPrevPos = (iPos == 0) ? iRXSize-1 : iPos - 1;
    
    if (lRX[iPrevPos] <= (iTus+b1*iTol) && lRX[iPrevPos] >= (iTus-b1*iTol)){
      // Found a matching bit
      bResult = true;
    }
  }

  return bResult;
}
