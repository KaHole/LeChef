#include "nRF24L01.h" //NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24
#include "RF24.h"
#include "SPI.h"

enum Pins { pinX = 2, pinY = 1, pinTrigger = 2 };

RF24 radio(9, 10); // NRF24L01 used SPI pins + Pin 9 and 10 on the NANO
const uint64_t pipe = 0xE6E6E6E6E6E6; // Needs to be the same for communicating between 2 NRF24L01 

void setup(void) {
    Serial.begin(9600);
    pinMode(pinTrigger, INPUT_PULLUP);

    radio.begin(); // Start the NRF24L01
    radio.setChannel(95);
    radio.openWritingPipe(pipe); // Get NRF24L01 ready to transmit
}

uint32_t data;
uint32_t placer;

void loop(void) {
    /* Read inputs and shift bits in place. */
    data = analogRead(pinX);
    placer = analogRead(pinY);
    data = data << 10 | placer;

    placer = digitalRead(pinTrigger) == LOW;
    data = data << 1 | placer;

    radio.write(&data, sizeof(data));
}