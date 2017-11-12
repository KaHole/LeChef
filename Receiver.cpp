#include "nRF24L01.h" // NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24 !!importer zippen
#include "RF24.h"
#include "SPI.h"

const bool FORWARD = true;
const bool BACKWARD = false;

enum Pins { enable_left = 3, left_forwards = 4, left_backwards = 6
            enable_right = 5, right_forwards = 8, right_backwards = 7
            laser_pin = 2 }

/*------------------------------------------------------------------------------------------------------*/

class Engine {

    short enablePin;
    short forwardPin;
    short backwardPin;
    
    int speed = 0;
    bool forward = FORWARD;
    bool inverted = false;

public:

    Engine(short ePin, short fPin, short bPin, bool inv) {
        enablePin = ePin; forwardPin = fPin; backwardPin = bPin; inverted = inv;

        pinMode(enablePin, OUTPUT);
        pinMode(forwardPin, OUTPUT);
        pinMode(backwardPin, OUTPUT);

        stop();
    }

    void setSpeed(int speed) { this->speed = speed; } /*mellom 0-255*/ 

    void setDirection(bool forward) { this->forward = forward; }

    void stop() {
        digitalWrite(forwardPin, HIGH);
        digitalWrite(backwardPin, HIGH);
    }

    void excecute() {

        if (speed < 70) {
          stop();
          return;
        }

        bool actualForward = inverted ? !forward : forward;
        
        if (actualForward) {
            digitalWrite(forwardPin, HIGH);
            digitalWrite(backwardPin, LOW);
        } else {
            digitalWrite(forwardPin, LOW);
            digitalWrite(backwardPin, HIGH);        
        }

        analogWrite(enablePin, speed);
    }
};

class Laser {
    
        int pin;
        bool firing = false;
    
    public:
        Laser(int n) {
            pin = n;
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW);
        }
    
        void fire() {
            if (firing) return;
            digitalWrite(pin, HIGH);
            firing = true;
        }
        
        void stop() {
            if (!firing) return;
            digitalWrite(pin, LOW);
            firing = false;
        }
    };

/*------------------------------------------------------------------------------------------------------*/

/*Data from radio-controller*/
struct Ctrl {
    unsigned short x;
    unsigned short y;
    bool trigger;

    uint32_t receive;
    uint32_t data;
};

class LeChef {

public:
    Engine *leftEngine;
    Engine *rightEngine;
    Laser *laser;
    Ctrl ctrl;

    LeChef() {
        leftEngine = new Engine(enable_left, left_forwards, left_backwards, true);
        rightEngine = new Engine(enable_right, right_forwards, right_backwards, true);
        laser = new Laser(laser_pin);
    }

    void update();
    void runActions();
    void bitshift();
};

void LeChef::update() {

    if (ctrl.receive != ctrl.data) {

        ctrl.data = ctrl.receive;
        bitshift();
        runActions();
    }
}

void LeChef::runActions() {

    if (ctrl.trigger) {
        laser->fire();
    } else {
        laser->stop();
    }

    int inY = ctrl.y - 512;

    if (inY < 0) {
        inY *= -1; //Give us a universal absolute speed.

        leftEngine->setDirection(BACKWARD);
        rightEngine->setDirection(BACKWARD);
    } else {
        leftEngine->setDirection(FORWARD);
        rightEngine->setDirection(FORWARD); 
    }

    int ySpeed = round( ( inY / 512.0 ) * 255.0 );

    leftEngine->setSpeed(ySpeed);
    rightEngine->setSpeed(ySpeed);

    /* Turning */

    /*360 turns*/
    if (ctrl.x > 534 || ctrl.x < 490) { //Deadzone
        int inX = ctrl.x - 512;

        if (inX < 0) {
            inX *= -1;

            int xSpeed = round( ( inX / 512.0 ) * 255.0 );
            rightEngine->setDirection(FORWARD);
            leftEngine->setDirection(BACKWARD);
            rightEngine->setSpeed(xSpeed);
            leftEngine->setSpeed(xSpeed);

        } else {

            int xSpeed = round( ( inX / 512.0 ) * 255.0 );
            rightEngine->setDirection(BACKWARD);
            leftEngine->setDirection(FORWARD);
            rightEngine->setSpeed(xSpeed);
            leftEngine->setSpeed(xSpeed);
        }
    }
    
    /*Power turns*/
    /*
    if (ctrl.x > 534 || ctrl.x < 490) { //Deadzone
        int inX = ctrl.x - 512;

        if (inX < 0) {
            inX *= -1;

            int xSlowdown = round( ( inX / 512.0 ) * 255.0 );
            rightEngine->setSpeed(ySpeed - xSlowdown);

        } else {
            int xSlowdown = round( ( inX / 512.0 ) * 255.0 );
            leftEngine->setSpeed(ySpeed - xSlowdown);
        }
    }*/

    leftEngine->excecute();
    rightEngine->excecute();
}

void LeChef::bitshift() {
    ctrl.trigger = ctrl.data & 1;
    ctrl.y = ctrl.data >> 1 & 1023;
    ctrl.x = ctrl.data >> 11 & 1023;
}

/*------------------------------------------------------------------------------------------------------*/

/* Arduino / Radio code*/
RF24 radio(9, 10);
const uint64_t pipe = 0xE6E6E6E6E6E6;

LeChef *leChef;

void setup(void) {
    //Serial.begin(9600);
    
    radio.begin(); // Start the NRF24L01
    radio.setChannel(95);
    radio.openReadingPipe(1, pipe); // Get NRF24L01 ready to receive
    radio.startListening(); // Listen to see if information received

    leChef = new LeChef();

    leChef->leftEngine->stop();
    leChef->rightEngine->stop();  
}

void loop(void) {
    
    while (radio.available()) {
        radio.read(&(leChef->ctrl.receive), sizeof(leChef->ctrl.receive));
        leChef->update();
    }

    /* //some tests
    leChef->leftEngine->setSpeed(255);
    leChef->rightEngine->setSpeed(255);
    leChef->rightEngine->excecute();
    leChef->leftEngine->excecute();
    */
    /*
    analogWrite(3, 255);
    analogWrite(5, 255);
    
    digitalWrite(4, LOW);
    digitalWrite(6, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    delay(2000);
    */
}