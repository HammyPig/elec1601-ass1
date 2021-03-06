// ---------------------------------------------------- DO NOT TOUCH ----------------------------------------------------
#include <SoftwareSerial.h>   // Software Serial Port
#define RxD 7
#define TxD 6
#define ConnStatus A1         // Connection status on the SeeedStudio v1 shield is available on pin A1                              // See also ConnStatusSupported boolean below
#define DEBUG_ENABLED  1
int shieldPairNumber = 11;
boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin
String slaveName = "Slave";                  // This is concatenated with shieldPairNumber later
String masterNameCmd = "\r\n+STNA=Master";   // This is concatenated with shieldPairNumber later
String connectCmd = "\r\n+CONN=";            // This is concatenated with slaveAddr later
int nameIndex = 0;
int addrIndex = 0;
String recvBuf;
String slaveAddr;
String retSymb = "+RTINQ=";   // Start symble when there's any return
SoftwareSerial blueToothSerial(RxD,TxD);
// -----------------------------------------------------------------------------------------------------------------------

void setup()
{
    // ------------------------ BLUETOOTH ------------------------
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);
    // -----------------------------------------------------------

    // Joystick
    pinMode(A5, INPUT);
    pinMode(A4, INPUT);
    pinMode(A3, INPUT);
    // ----------

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Master-Slave connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Slave - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Slave.");

        setupBlueToothConnection();     // Set up the local (master) Bluetooth module
        getSlaveAddress();              // Search for (MAC) address of slave
        makeBlueToothConnection();      // Execute the connection to the slave

        delay(1000);                    // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
}


void loop()
{
    char recvChar;
    boolean attemptingSwitch = false;
    
    while(1)
    {   
        delay(50);

        // Get joystick input
        int x = analogRead(A5);
        int y = analogRead(A4);
        boolean z = analogRead(A3);
        Serial.println(z);
        if (z == false) {
            blueToothSerial.println("!");
            delay(200);
        } else {
          x = map(x, 0, 1023, -100, 100);
          y = map(y, 0, 1023, -100, 100);
          if (x < 10 && x > -10) {
            x = 0;
          }

          if (y < 10 && y > -10) {
            y = 0;
          }
          String joystick = String(x) + "," + String(y);
          Serial.println(joystick);
          blueToothSerial.println(joystick);
        }

        // ------------------

        if(blueToothSerial.available())   // Check if there's any data sent from the remote Bluetooth shield
        {   
            recvChar = blueToothSerial.read();
            Serial.print(recvChar);
        }

        if(Serial.available())            // Check if there's any data sent from the local serial terminal. You can add the other applications here.
        {
            recvChar  = Serial.read();
            Serial.print(recvChar);
            blueToothSerial.print(recvChar);
        }
    }
}

// ---------------------------------------------------- DO NOT TOUCH ----------------------------------------------------
void setupBlueToothConnection()
{
    Serial.println("Setting up the local (master) Bluetooth module.");
    masterNameCmd += shieldPairNumber;
    masterNameCmd += "\r\n";
    blueToothSerial.print("\r\n+STWMOD=1\r\n");      // Set the Bluetooth to work in master mode
    blueToothSerial.print(masterNameCmd);            // Set the bluetooth name using masterNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection is forbidden here
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the master Bluetooth inquire
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    Serial.println("Master is inquiring!");
}

// ---------------------------------------------------- DO NOT TOUCH ----------------------------------------------------
void getSlaveAddress()
{
    slaveName += shieldPairNumber;
    Serial.print("Searching for address of slave: ");
    Serial.println(slaveName);
    slaveName = ";" + slaveName;   // The ';' must be included for the search that follows
    char recvChar;
    while(1)
    {
        if(blueToothSerial.available())
        {  
            recvChar = blueToothSerial.read();
            recvBuf += recvChar;

            nameIndex = recvBuf.indexOf(slaveName);   // Get the position of slave name

            if ( nameIndex != -1 )   // ie. if slaveName was found
            {
                addrIndex = (recvBuf.indexOf(retSymb,(nameIndex - retSymb.length()- 18) ) + retSymb.length());   // Get the start position of slave address
                slaveAddr = recvBuf.substring(addrIndex, nameIndex);   // Get the string of slave address

                Serial.print("Slave address found: ");
                Serial.println(slaveAddr);

                break;  // Only breaks from while loop if slaveName is found
            }
        }
    }
}

// ---------------------------------------------------- DO NOT TOUCH ----------------------------------------------------
void makeBlueToothConnection()
{
    Serial.println("Initiating connection with slave.");
    char recvChar;
    connectCmd += slaveAddr;
    connectCmd += "\r\n";
    int connectOK = 0;       // Flag used to indicate succesful connection
    int connectAttempt = 0;  // Counter to track number of connect attempts
    do
    {
        Serial.print("Connect attempt: ");
        Serial.println(++connectAttempt);
        blueToothSerial.print(connectCmd);   // Send connection command
        recvBuf = "";
        while(1)
        {
            if(blueToothSerial.available())
            {
                recvChar = blueToothSerial.read();
                recvBuf += recvChar;

                if(recvBuf.indexOf("CONNECT:OK") != -1)
                {
                    connectOK = 1;
                    Serial.println("Connected to slave!");
                    blueToothSerial.print("Master-Slave connection established!");
                    break;
                }
                else if(recvBuf.indexOf("CONNECT:FAIL") != -1)
                {
                    Serial.println("Connection FAIL, try again!");
                    break;
                }
            }
        }
    } while (0 == connectOK);
}