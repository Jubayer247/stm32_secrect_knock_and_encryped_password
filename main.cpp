/* Detects patterns of knocks and triggers a motor to unlock
   it if the pattern is correct.

   By Steve Hoefer http://grathio.com
   Version 0.1.10.20.10
   Licensed under Creative Commons Attribution-Noncommercial-Share Alike 3.0
   http://creativecommons.org/licenses/by-nc-sa/3.0/us/
   (In short: Do what you want, just be sure to include this line and the four above it, and don't sell it or use it in anything you sell without contacting me.)

   Analog Pin 0: Piezo speaker (connected to ground with 1M pulldown resistor)
   Digital Pin 2: Switch to enter a new code.  Short this to enter programming mode.
   Digital Pin 3: DC gear reduction motor attached to the lock. (Or a motor controller or a solenoid or other unlocking mechanisim.)
   Digital Pin 4: Red LED.
   Digital Pin 5: Green LED.

   Update: Nov 09 09: Fixed red/green LED error in the comments. Code is unchanged.
   Update: Nov 20 09: Updated handling of programming button to make it more intuitive, give better feedback.
   Update: Jan 20 10: Removed the "pinMode(knockSensor, OUTPUT);" line since it makes no sense and doesn't do anything.
 */


//
#include "mbed.h"
#include "keypad.h"
#include "TextLCD.h"
#include "Stepper.h"
#include <string>
#include "DS3231.h"

using namespace std;
//
using namespace mbed;

Keypad::Keypad(PinName col0, PinName col1, PinName col2, PinName col3,
               PinName row0,PinName row1, PinName row2, PinName row3):
    _cols(col0,col1,col2,col3) ,_rows(row0,row1,row2,row3)    {    }

void Keypad::enablePullUp()
{
    _cols.mode(PullUp);
}

bool Keypad::getKeyPressed()
{
    _rows = 0;              // Ground all keys
    if(_cols.read()==0xff)  //Chk if key is pressed
        return false;

    return true;
}

int Keypad::getKeyIndex()
{
    if (!getKeyPressed())
        return -1;

    //Scan rows and cols and return switch index
    for(int i=0; i<4; i++) {
        _rows = ~(0x01 << i);
        for(int j=0; j<4; j++)
            if (  !( (_cols.read()>> j )& 0x1 ))
                return  j + (i*4);
    }
    return -1;
}

char Keypad::getKey()
{
    int k = getKeyIndex();
    if(k != -1)
        return  keys[k];

    return 0;
}

//  c0   c1   c2   c3  r0   r1   r2   r3

//

//



//

Timer timer;



bool unlocked=true;

// Pin definitions
AnalogIn a1(A0);         // Piezo sensor on pin 0.
DigitalIn programSwitch(D11);       // If this is high we program a new code.
DigitalIn lockSwitch(D10);
//const int lockMotor = 3;           // Gear motor used to turn the lock.
DigitalOut yelloLED(PC_10); // Status LED
DigitalOut greenLED(PC_11);
DigitalOut redLED1(PA_13);            // Status LED
DigitalOut redLED2(PC_12);


// Tuning constants.  Could be made vars and hoooked to potentiometers for soft configuration, etc.
const int threshold = 40; //3         // Minimum signal from the piezo to register as a knock
const int rejectValue = 25;         // If an individual knock is off by this percentage of a knock we don't unlock..
const int averageRejectValue = 15; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockTurnTime = 650;      // milliseconds that we run the motor to get it to go a half turn.

const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1200;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
int programButtonPressed = false;   // Flag so we remember the programming button setting at the end of the cycle.


// Green LED on, everything is go.



void lockTheDoor();
void listenToSecretKnock();
void encryptDecrypt(int toEncrypt[]);
bool validateKnock();
void triggerDoorUnlock();
void matchPassword();
//float map(float in, float inMin, float inMax, float outMin, float outMax);
long map(long x, long in_min, long in_max, long out_min, long out_max);
int arr[]= {0,0,0,0};
int test[]= {0,0,0,0};
int stepsPerRevolution=200;


Stepper myStepper(stepsPerRevolution,PB_13,PB_15,PB_14,PB_1);
TextLCD lcd(PC_8,PB_11,PC_6,PC_5,PA_12,PA_11,PB_12);
int h,m,s;
DS3231 rtc(PB_9,PB_8);

int main()
{
    timer.start();
    myStepper.setSpeed(60);
    redLED2=1;
    //char key;
    lcd.gotoxy(1,1);
    lcd.printf("Hello");
    lcd.gotoxy(1,1);
    lcd.clear();
    lcd.printf("Program start.");              // but feel free to comment them out after it's working right.
    greenLED=1;
    while(1) {
        // Listen for any knock at all.
        knockSensorValue = a1.read()*620;
        printf("%d\n",knockSensorValue);
        if (programSwitch==1) { // is the program button pressed?
            programButtonPressed = true;          // Yes, so lets save that state
            redLED1=1;           // and turn on the red light too so we know we're programming.
        } else {
            programButtonPressed = false;
            redLED1=1;
        }

        if (knockSensorValue >=threshold) {
            listenToSecretKnock();
        }
    }
}


void encryptDecrypt(int toEncrypt[])
{
    char key[3] = {'K', 'C', 'Q'}; //Any chars will work, in an array of any size

    for (int i = 0; i < 4; i++)
        test[i] = toEncrypt[i] ^ key[i % (sizeof(key) / sizeof(char))];
}


void generate()
{
    rtc.readTime(&h,&m,&s);
    arr[0]=rand()%10;
    arr[1]=rand()%10;
    arr[2]=map(h,0,24,0,9);
    arr[2]=map(s,0,60,0,9);
    encryptDecrypt(arr);
    printf(" %d%d%d%d \n",test[0],test[1],test[2],test[3]);
    printf(" %d%d%d%d \n",arr[0],arr[1],arr[2],arr[3]);
    lcd.clear();
    lcd.gotoxy(1,1);
    lcd.printf("Cipher");
    lcd.printf(" %d%d%d%d \n",test[0],test[1],test[2],test[3]);
    matchPassword();
}



void lockTheDoor()
{
    if(unlocked) {
        redLED2=1;
        greenLED=0;
        myStepper.step(stepsPerRevolution);
        yelloLED=0;
        redLED2=1;
        unlocked=false;
    }
}






// Records the timing of knocks.
void listenToSecretKnock()
{
    lcd.clear();
    lcd.gotoxy(1,1);
    lcd.printf("knock starting");

    int i = 0;
    // First lets reset the listening array.
    for (i=0; i<maximumKnocks; i++) {
        knockReadings[i]=0;
    }

    int currentKnockNumber=0;                     // Incrementer for the array.
    int startTime=timer.read_ms();                       // Reference for when this knock started.
    int now=timer.read_ms();

    greenLED=0;                 // we blink the LED for a bit as a visual indicator of the knock.
    if (programButtonPressed==true) {
        redLED1=0;                         // and the red one too if we're programming a new knock.
    }
    wait_ms(knockFadeTime);                                 // wait for this peak to fade before we listen to the next one.
    greenLED=1;
    if (programButtonPressed==true) {
        redLED1=1;
    }
    do {
        //listen for the next knock or wait for it to timeout.
        knockSensorValue = a1.read()*620;
        if (knockSensorValue >=threshold) {                  //got another knock...
            //record the wait_ms time.
            lcd.clear();
            lcd.gotoxy(1,1);
            lcd.printf("knock.");

            now=timer.read_ms();
            knockReadings[currentKnockNumber] = now-startTime;
            currentKnockNumber ++;                             //increment the counter
            startTime=now;
            // and reset our timer for the next knock
            greenLED=0;
            if (programButtonPressed==true) {
                redLED1=0;                       // and the red one too if we're programming a new knock.
            }
            wait_ms(knockFadeTime);                              // again, a little delay to let the knock decay.
            greenLED=1;
            if (programButtonPressed==true) {
                redLED1=1;
            }
        }

        now=timer.read_ms();

        //did we timeout or run out of knocks?
    } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));

    //we've got our knock recorded, lets see if it's valid
    if (programButtonPressed==false) {
        // for(int i=0;i<maximumKnocks;i++){
//            printf("%d %d \n",secretCode[i],knockReadings[i]);
//
//            }
//            wait(10);

        // only if we're not in progrmaing mode.
        if (validateKnock() == true) {
            printf("knock matched \n");
            //wait(5);
            generate();
        } else {
            lcd.clear();
            lcd.gotoxy(1,1);
            lcd.printf("Secret knock failed.");

            greenLED=0;          // We didn't unlock, so blink the red LED as visual feedback.
            for (i=0; i<4; i++) {
                redLED1=1;
                wait_ms(100);
                redLED1=0;
                wait_ms(100);
            }
            greenLED=1;
        }
    } else { // if we're in programming mode we still validate the lock, we just don't do anything with the lock
        validateKnock();
        // and we blink the green and red alternately to show that program is complete.
        lcd.clear();
        lcd.gotoxy(1,1);
        lcd.printf("New lock stored.");
        redLED1=0;
        greenLED= 1;
        for (i=0; i<3; i++) {
            wait_ms(100);
            redLED1= 1;
            greenLED= 0;
            wait_ms(100);
            redLED1=0;
            greenLED= 1;
        }
    }
}


// this method will checks the password receive from keypad

void matchPassword()
{
    wait(10);
    lcd.clear();
    lcd.gotoxy(1,1);
    //lcd.print("Enter");
    lcd.printf("Passord :");
    printf("Passord :\n");
    lcd.gotoxy(1,2);
    //lcd.print("Passord");
    //lcd.setCursor(0, 0);
    int inputs[4]= {1,2,3,4};
    //int given[4]={1,2,3,4};
    int i=0;
    int starttime = timer.read_ms();
    int endtime = starttime;
    Keypad keypad(D6,D7,D8,D9, D2,D3,D4,D5);
    keypad.enablePullUp();
    while (((endtime - starttime) <=60000) && i<=3) { // do this loop for up to 1000mS
        char key = keypad.getKey();
        if(key != KEY_RELEASED) {
            lcd.printf("%c",key);
            printf("%c",key);
            inputs[i]=key-48;
            i++;
            // pc.printf("%c\r\n",key);
            wait(0.6);
        }
        //loopcount = loopcount+1;
        endtime = timer.read_ms();
    }
    bool b=true;
    if(i==4) {
        for(i=0; i<=3; i++) {
            if(inputs[i]!=arr[i]) {
                b=false;
                break;
            }
        }
    } else {
        b=false;
    }
    if(b) {
        lcd.clear();
        lcd.gotoxy(1,1);
        lcd.printf("Password");
        printf("Password");
        lcd.gotoxy(1,2);
        lcd.printf("Accepted");
        printf("Accepted");
        lcd.gotoxy(1,1);
        wait_ms(100);
        triggerDoorUnlock();
    } else {
        lcd.clear();
        lcd.gotoxy(1,1);
        lcd.printf("Wrong");
        printf("Wrong");
        lcd.gotoxy(1, 2);
        lcd.printf("Password");
        lcd.gotoxy(1,1);
        wait_ms(100);
        lcd.clear();
        lcd.printf("Knock");
    }

}




// Runs the motor (or whatever) to unlock the door.
void triggerDoorUnlock()
{
    lcd.clear();
    lcd.gotoxy(1,1);
    lcd.printf("Door unlocked!");
    printf("Door unlocked!");
    int i=0;
    myStepper.step(-stepsPerRevolution);
    greenLED=1;            // And the green LED too.

    wait_ms (lockTurnTime);                    // Wait a bit.

    //digitalWrite(lockMotor, LOW);            // Turn the motor off.

    // Blink the green LED a few times for more visual feedback.
    for (i=0; i < 5; i++) {
        greenLED= 0;
        wait_ms(100);
        greenLED= 1;
        wait_ms(100);
    }
    unlocked=true;
    wait_ms(10000);
    lcd.clear();
    lcd.gotoxy(1,1);
    lcd.printf("Knock");
    greenLED=1;
    redLED2=0;
    yelloLED=1;
    redLED1=0;
    wait(10);
    lockTheDoor();

}






// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
// todo: break it into smaller functions for readability.
bool validateKnock()
{
    int i=0;

    // simplest check first: Did we get the right number of knocks?
    int currentKnockCount = 0;
    int secretKnockCount = 0;
    int maxKnockInterval = 0;                     // We use this later to normalize the times.

    for (i=0; i<maximumKnocks; i++) {
        if (knockReadings[i] > 0) {
            currentKnockCount++;
        }
        if (secretCode[i] > 0) {                    //todo: precalculate this.
            secretKnockCount++;
        }

        if (knockReadings[i] > maxKnockInterval) {  // collect normalization data while we're looping.
            maxKnockInterval = knockReadings[i];
        }
    }

    // If we're recording a new knock, save the info and get out of here.
    if (programButtonPressed==true) {
        for (i=0; i<maximumKnocks; i++) { // normalize the times
            secretCode[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);
        }
        // And flash the lights in the recorded pattern to let us know it's been programmed.
        greenLED= 0;
        redLED1= 0;
        wait_ms(1000);
        greenLED= 1;;
        redLED1=1;
        wait_ms(50);
        for (i = 0; i < maximumKnocks ; i++) {
            greenLED= 0;
            redLED1= 0;
            // only turn it on if there's a delay
            if (secretCode[i] > 0) {
                wait_ms( map(secretCode[i],0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was.  Roughly.
                greenLED= 1;
                redLED1=1;
            }
            wait_ms(50);
        }
        return false;     // We don't unlock the door when we are recording a new knock.
    }

    if (currentKnockCount != secretKnockCount) {
        // return false;
    }

    /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
        (ie: if you do the same pattern slow or fast it should still open the door.)
        This makes it less picky, which while making it less secure can also make it
        less of a pain to use if you're tempo is a little slow or fast.
    */
    int totaltimeDifferences=0;
    int timeDiff=0;
    for (i=0; i<maximumKnocks; i++) { // Normalize the times
        knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);
        timeDiff = abs(knockReadings[i]-secretCode[i]);
        // for(int i=0;i<maximumKnocks;i++){
        printf("%d %d \n",secretCode[i],knockReadings[i]);

        //     }
        //   wait(10);
        if (timeDiff > rejectValue) { // Individual value too far out of whack
            return false;
        }
        totaltimeDifferences += timeDiff;
    }
// wait(10);
    // It can also fail if the whole thing is too inaccurate.
    if (totaltimeDifferences/secretKnockCount>averageRejectValue) {
        return false;
    }

    return true;

}


long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}