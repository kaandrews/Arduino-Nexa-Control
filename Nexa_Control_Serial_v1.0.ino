
/*
 * Program to control Nexa 433 MHz remote control sockets/dimmer
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

const byte rfTransmitPin = 4;  //RF Transmitter pin = digital pin 4

const int usDelay = 250;  // Basic time unit of transmitted signal
const byte numChars = 32;
char receivedChars[numChars]; // array to hold received data
boolean newData = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(rfTransmitPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  Serial.flush();

  Serial.println("Send command as: ID (0-67108863),Group(T/F), TurnOn(T/F), Channel(1-4), Unit(1-4) e.g. 1234,F,T,1,1");
}

/*  Codes from Nexa 10 button transmitter NEYCT-705
 *  Switch 1
 *  Identifier
 *  10111101000000000111110001 = 49545713
 *  
 *  Btn Gr  Ch  Unit
 *  1   F   11  11
 *  2   F   11  10
 *  3   F   11  01
 *  4   F   11  00
 *  G   T   11  11
 *  
 *  Switch 2
 *  Identifier
 *  10111101000000000111110001
 *  
 *  Btn Gr  Ch  Unit
 *  1   F   10  11
 *  2   F   10  10
 *  3   F   10  01
 *  4   F   10  00
 *  G   T   10  11
 *  
 *  Switch 3
 *  Identifier
 *  10111101000000000111110001
 *  
 *  Btn Gr  Ch  Unit
 *  1   F   01  11
 *  2   F   01  10
 *  3   F   01  01
 *  4   F   01  00
 *  G   T   01  11
 *  
 *  Switch 4
 *  Identifier
 *  10111101000000000111110001
 *  
 *  Repeats 5 times
 *  
 *  Btn Gr  Ch  Unit
 *  1   F   00  11
 *  2   F   00  10
 *  3   F   00  01
 *  4   F   00  00
 *  G   T   00  11
 *  
 *  Codes from Nexa Timer Transmitter TMT-918
 *  Identifier
 *  11000100111110010010110001 = 51635377
 *  
 *  Repeats 8 times
 *  
 *  Opt   Gr  Ch  Unit
 *  1     F   11  11
 *  2     F   11  10
 *  3     F   11  01
 *  4     F   11  00
 *  5     F   10  11
 *  6     F   10  10
 *  7     F   10  01
 *  8     F   10  00
 *  9     F   01  11
 *  10    F   01  10
 *  11    F   01  01
 *  12    F   01  00
 *  13    F   00  11
 *  14    F   00  10
 *  15    F   00  01
 *  16    F   00  00
 *  
 *  26 binary digit number is between 0 - 67108863
 *  
 */

void loop() {
  // Receive commands by serial
  recvWithEndMarker();

  // Process Commands
  //processSerial();
  processParseSerial();
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

/*
void processSerial() {
  // Assumes the data has arrived in the format "T,T,1,1"
  // Anything else will cause weirdness
  
  if (newData == true) {
    
    boolean tGroup = (receivedChars[0] == 'T') ? true : false;

    boolean tOn = (receivedChars[2] == 'T') ? true : false;

    int tChan = String(receivedChars[4]).toInt();

    int tUnit = String(receivedChars[6]).toInt();
    
    nexaTransmit(uniqueID, tGroup, tOn, tChan, tUnit);

    Serial.println("Sent: " + uniqueID + ", " + tGroup + ", " + tOn + ", " + tChan + ", " + tUnit);
    
    newData = false;
  }
}
*/

void processParseSerial() {
  // Assumes the data has arrived in the format "12345,T,T,1,1"
  // Partly defensive, could do with more error checking
  
  if (newData == true) {
    //Set up the variables to be populated from Serial input
    String stID = "";
    boolean btGroup = false;
    boolean btOn = false;
    int itChan = 0;
    int itUnit = 0;

    // variables for parsing the string
    int iStart = 0;
    int iEnd = 0;

    // Boolean check if all parsed OK
    boolean bOK = true;
    String sError = "";
    
    // Convert char array to String object
    String sInput(receivedChars);

    // Extract ID
    iEnd = sInput.indexOf(",",iStart);

    // If there is a , in the string
    if(iEnd > 0){

      // Copy first parameter into String
      stID = sInput.substring(iStart,iEnd);

      // Check if all are digits
      for (int i=0; i < stID.length(); i++) {
        if (!isDigit(stID.charAt(i))){
          Serial.println("4");
          bOK = false;
          sError = "Only numbers can be entered for the ID";
        } 
      }

      if (bOK){
        // Check if the number is within range
        if (stID.toInt() > 67108863 || stID.toInt() < 0){
          bOK = false;
          sError = "Please enter ID between 0 and 67108863";
        }

        if (bOK){
          //Convert number to binary string
          stID = String(sInput.substring(iStart,iEnd).toInt(),BIN);

          // Pad string to 26 bits
          int iLen = 26 - stID.length();
    
          String sFill = "";
    
          for (int i = 0; i< iLen; i++){
            sFill = "0" + sFill;
          }
    
          stID = sFill + stID;
    
          iStart = iEnd + 1;
      
          // Extract Group
          iEnd = sInput.indexOf(",",iStart);
    
          if (iEnd > 0) {
            btGroup = (sInput.substring(iStart,iEnd) == "T") ? true : false;
        
            iStart = iEnd + 1;
        
            // Extract On Off
            iEnd = sInput.indexOf(",",iStart);
        
            if (iEnd > 0) {
              btOn = (sInput.substring(iStart,iEnd) == "T") ? true : false;
          
              iStart = iEnd + 1;
          
              // Extract Channel
              iEnd = sInput.indexOf(",",iStart);
          
              if (iEnd > 0) {
                itChan = sInput.substring(iStart,iEnd).toInt();
            
                iStart = iEnd + 1;
            
                // Extract Unit
                itUnit = sInput.substring(iStart).toInt();
              } else {
                bOK = false;
                sError = "Missing parameter";
              }
            } else {
              bOK = false;
              sError = "Missing parameter";
            }
          } else {
            bOK = false;
            sError = "Missing parameter";
          }
        } else {
          bOK = false;
          sError = "Missing parameter";
        }
      }
    } else {
      bOK = false;
      sError = "Wrong format, no delimiter found.";
    }


    if (bOK) {
      nexaTransmit(stID, btGroup, btOn, itChan, itUnit);

      Serial.println("Sent: " + stID + " " + btGroup + " " + btOn + " " + itChan + " " + itUnit);
    } else {
      Serial.println("Error: " + sError);
    }
    
    newData = false;
  }
}

// 433 MHz transmit section
void nexaTransmit(String sTransCode, boolean bGroup, boolean bOn, int iChan, int iUnit){
  // Turn on LED
  digitalWrite(LED_BUILTIN, HIGH);

  // Send the same signal 5 times
  for (int i=0;i<5;i++){
    sendSync();
    
    // Send unique transmitter code
    for (int j=0;j<26;j++){
      switch (sTransCode.charAt(j)){
      case '0':
        send0();
        break;
      case '1':
        send1();
        break;
      }
    }
  
    // Send Group
    if(bGroup) {
      send0();
    } else {
      send1();
    }
  
    // Send On/Off
    if(bOn){
      send0();
    } else {
      send1();
    }
  
    // Send channel bits
    switch (iChan){
    case 1:
      send1();
      send1();
      break;
    case 2:
      send1();
      send0();
      break;
    case 3:
      send0();
      send1();
      break;
    case 4:
      send0();
      send0();
    }
  
    // Send Unit
    switch (iUnit){
    case 1:
      send1();
      send1();
      break;
    case 2:
      send1();
      send0();
      break;
    case 3:
      send0();
      send1();
      break;
    case 4:
      send0();
      send0();
    }
  
    // Send Pause
    sendPause();
  }

  // Turn off LED
  digitalWrite(LED_BUILTIN, LOW);
}

void sendSync(){
  //Serial.println("sendSync");
  //Send HIGH signal
  digitalWrite(rfTransmitPin, HIGH);     
  delayMicroseconds(usDelay);

  //Send LOW signal
  digitalWrite(rfTransmitPin, LOW);
  delayMicroseconds(10*usDelay);
}

void send1(){
  //Serial.println("send1");
  //Send HIGH signal
  digitalWrite(rfTransmitPin, HIGH);     
  delayMicroseconds(usDelay);

  //Send LOW signal
  digitalWrite(rfTransmitPin, LOW);
  delayMicroseconds(usDelay);

    //Send HIGH signal
  digitalWrite(rfTransmitPin, HIGH);     
  delayMicroseconds(usDelay);

  //Send LOW signal
  digitalWrite(rfTransmitPin, LOW);
  delayMicroseconds(5*usDelay);
}

void send0(){
  //Serial.println("send0");
  //Send HIGH signal
  digitalWrite(rfTransmitPin, HIGH);     
  delayMicroseconds(usDelay);

  //Send LOW signal
  digitalWrite(rfTransmitPin, LOW);
  delayMicroseconds(5*usDelay);

  //Send HIGH signal
  digitalWrite(rfTransmitPin, HIGH);     
  delayMicroseconds(usDelay);

  //Send LOW signal
  digitalWrite(rfTransmitPin, LOW);
  delayMicroseconds(usDelay);
}

void sendPause(){
  //Serial.println("sendPause");
  //Send HIGH signal
  digitalWrite(rfTransmitPin, HIGH);     
  delayMicroseconds(usDelay);

  //Send LOW signal
  digitalWrite(rfTransmitPin, LOW);
  delayMicroseconds(40*usDelay);
}
