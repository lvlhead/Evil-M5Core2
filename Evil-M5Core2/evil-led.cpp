/*
    Evil-M5Core2 - WiFi Network Testing and Exploration Tool

    Copyright (c) 2024 7h30th3r0n3
    Copyright (c) 2024 lvlhead

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Disclaimer:
    This tool, Evil-M5Core2, is developed for educational and ethical testing purposes only.
    Any misuse or illegal use of this tool is strictly prohibited. The creator of Evil-M5Core2
    assumes no liability and is not responsible for any misuse or damage caused by this tool.
    Users are required to comply with all applicable laws and regulations in their jurisdiction
    regarding network testing and ethical hacking.
*/

#include "evil-led.h"

EvilLED::EvilLED() {
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LED_PIXELS, LED_PIN, NEO_GRB);
}

void EvilLED::init() {
    pixels.begin();
}

void EvilLED::showDelay(int delayVal) {
    // delayVal: delay in ms
    pixels.show();
    delay(delayVal);
}

void EvilLED::off(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
}

void EvilLED::red(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(255, 0, 0));
}

void EvilLED::blue(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(0, 255, 0));
}

void EvilLED::green(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 255));
}

void EvilLED::white(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(255, 255, 255));
}

void EvilLED::pattern1() {
    // Set LED bar to red starting from center
    red(4);
    red(5);
    showDelay(100);

    red(3);
    red(6);
    showDelay(100);

    red(2);
    red(7);
    showDelay(100);

    red(1);
    red(8);
    showDelay(100);

    red(0);
    red(9);
    showDelay(100);
}

void EvilLED::pattern2() {
    // Set LED bar to off starting from center
    off(4);
    off(5);
    showDelay(50);

    off(3);
    off(6);
    showDelay(50);

    off(2);
    off(7);
    showDelay(50);

    off(1);
    off(8);
    showDelay(50);

    off(0);
    off(9);
    showDelay(50);
}

void EvilLED::pattern3() {
    // Animate red LED outwards starting from the center
    // 0 1 2 3 4 5 6 7 8 9
    //         R R
    //       R R R R
    //       R     R
    //     R R     R R
    //     R         R
    //   R R         R R
    //   R             R
    // R R             R R
    // R                 R
    red(4);
    red(5);
    showDelay(50);

    red(3);
    red(6);
    showDelay(50);

    off(4);
    off(5);
    showDelay(50);

    red(2);
    red(7);
    showDelay(50);

    off(3);
    off(6);
    showDelay(50);

    red(1);
    red(8);
    showDelay(50);

    off(2);
    off(7);
    showDelay(50);

    red(0);
    red(9);
    showDelay(50);

    off(1);
    off(8);
    pixels.show();
    delay(50);

    off(0);
    off(9);
    showDelay(50);
}

void EvilLED::pattern4() {
    // Animate red LED inward starting from the outside
    // 0 1 2 3 4 5 6 7 8 9
    // R                 R
    //
    //   R             R
    //
    //     R         R
    //
    //       R     R
    //
    //         R R
    red(0);
    red(9);
    showDelay(50);

    off(0);
    off(9);
    showDelay(50);

    red(1);
    red(8);
    showDelay(50);

    off(1);
    off(8);
    showDelay(50);

    red(2);
    red(7);
    showDelay(50);

    off(2);
    off(7);
    showDelay(50);

    red(3);
    red(6);
    showDelay(50);

    off(3);
    off(6);
    pixels.show();
    delay(50);

    red(4);
    red(5);
    showDelay(50);

    off(4);
    off(5);
    showDelay(50);
}

void EvilLED::pattern5() {
    // Blink the two outside LEDs red
    red(0);
    red(9);
    showDelay(50);

    off(0);
    off(9);
    pixels.show();
}

void EvilLED::pattern6() {
    // Blink the two outside LEDs red
    white(0);
    white(9);
    showDelay(50);

    off(0);
    off(9);
    pixels.show();
}

void EvilLED::pattern7() {
    // Blink the two outside LEDs green
    green(0);
    green(9);
    showDelay(50);

    off(0);
    off(9);
    pixels.show();
}

void EvilLED::pattern8() {
    // Set the two outside LEDs red
    red(0);
    red(9);
    pixels.show();
}

void EvilLED::pattern9() {
    // Turn the two outside LEDs off
    off(0);
    off(9);
    pixels.show();
}
