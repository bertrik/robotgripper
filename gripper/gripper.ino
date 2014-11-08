#include <stdint.h>

#include <SPI.h>
#include <RF24.h>

#include <Servo.h>

static Servo myservo;
static RF24 rf( /*ce */ 9, /*cs */ 10);

int pos = 60;                   // variable to store the servo position 
int lowerLimit = 45;
int upperLimit = 117;

static const uint64_t address = 0x66996699LL;   // So that's 0x0066996699

void setup()
{
    /* Serial init */
    Serial.begin(9600);
    Serial.println("Waiting for input (type \"help\" for help)");

    /* NRF init */
    rf.begin();

    rf.enableDynamicPayloads();
    rf.openReadingPipe(1, address);
    rf.startListening();

    /* Servo init */
    myservo.attach(14);         // attaches the servo on pin 9 to the servo object 
}

static boolean readLine(char *line)
{
    uint8_t buf[32];

    if (rf.available()) {
        boolean done = false;
        while (!done) {
            int len = rf.getDynamicPayloadSize();
            done = rf.read(buf, len);
            memcpy(line, (const char *) buf, len);
            line[len] = 0;
        }
        return true;
    }

    return false;
}

void updatePos(int newPos)
{
    if (newPos != pos) {
        if (newPos > upperLimit || newPos < lowerLimit) {
            Serial.print("Reached limit");
            Serial.println(newPos, DEC);
        } else {
            myservo.write(newPos);
            delay(15);
            pos = newPos;
        }
    } else {
        Serial.println("Already at desired position");
    }
}

void sweep()
{
    char line[32];
    int i;
    while (true) {
        int stepsize = 1;
        for (i = lowerLimit; i < upperLimit; i += stepsize) {
            if (i >= upperLimit) {
                delay(1000);
            }
            myservo.write(i);
            delay(15);
        }
        for (i = upperLimit; i > lowerLimit; i -= stepsize) {
            if (i <= lowerLimit) {
                delay(1000);
            }
            myservo.write(i);
            delay(15);
        }
        if (readLine(line)) {
            Serial.println("Aborting sweeping");
            pos = i;
            return;
        }
    }

}

void openPercent(int openPercent)
{
    if (openPercent > 100 || openPercent < 0) {
        Serial.print(openPercent);
        Serial.println(" is not between 0% and 100%!");
        return;
    }
    int newTgt = (openPercent * 0.72) + 45;
    updatePos(roundNbr(newTgt));

}

int roundNbr(float raw)
{
    int result = -1;
    int rawInt = (int) raw;
    float remainder = raw - rawInt;
    if (remainder < 0.5) {
        result = floor(raw);
    } else {
        result = ceil(raw);
    }
    return result;

}

void loop()
{
    char line[32];

    boolean ok = readLine(line);

    if (ok) {
        Serial.print("Received '");
        Serial.print(line);
        Serial.println("'");

        if (isDigit(line[0])) {
            int pc = atoi(line);
            Serial.print("Going to ");
            Serial.print(pc);
            Serial.println("%");
            openPercent(pc);
            return;
        }

        if (strcmp(line, "help") == 0) {
            Serial.println("Available commands:");
            Serial.
                println
                ("\"0 ... 100\" ==> bring gipper in given % position");
            Serial.println("\"open\" ==> open gripper fully");
            Serial.println("\"close\" ==> close gripper fully");
            Serial.
                println
                ("\"sweep\" ==> open/close until serial interruption");
            Serial.println("\"pos\" ==> display current servo position");
        } else if (strcmp(line, "open") == 0) {
            Serial.println("Fully opening");
            openPercent(100);
        } else if (strcmp(line, "close") == 0) {
            Serial.println("Fully closing");
            openPercent(0);
        } else if (strcmp(line, "pos") == 0) {
            Serial.print("Current servo position: ");
            int actual = myservo.read();
            Serial.println(actual);
        } else if (strcmp(line, "sweep") == 0) {
            Serial.println("Endless sweeping (interrupt with command)");
            sweep();
        } else {
            Serial.print("Invalid command: ");
            Serial.println(line);
        }
    }
}
