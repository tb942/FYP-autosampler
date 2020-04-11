#include "Carousel.h"

Carousel::Carousel(short Dir, short Step, short Ena, short Home)
    : BasicStepperDriver(200, Dir, Step, Ena), home_pin(Home) {  // Source from BasicStepperDriver class - set 200 steps per rotation
    carousel_current_pos = 1;
}

void Carousel::begin() {
    pinMode(home_pin, INPUT);
    BasicStepperDriver::begin(500, 16);                                               // Set 500 RPM and 16 microsteps
    BasicStepperDriver::setEnableActiveState(LOW);                                   // Set for TMC2100 driver
    BasicStepperDriver::setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 500, 500);  // Set acceleration
    BasicStepperDriver::enable();                                                     // Enable position holding
}

uint8_t Carousel::operate(uint8_t next_pos) {
    if (interlock_enable == true && pinterlock() == false) {
        return (3);
    }

    if (next_pos < 1 || next_pos > 8) {
        return (0);
    }

    bool pause_flag = false;

    int carousel_position_error = next_pos - this->carousel_current_pos;  // Calculate rotational position error

    if (carousel_position_error > 4) {
        carousel_position_error -= 8;
    } else if (carousel_position_error < -4) {
        carousel_position_error += 8;
    }

    carousel_position_error = carousel_position_error % 8;

    this->startRotate(carousel_position_error * 3 * 45);  // Rotate difference degrees * 3 (3:1 gearing) to desired position

    while (BasicStepperDriver::nextAction()) {                                                // while the motor is turning
        if (digitalRead(14) == HIGH || digitalRead(15) == HIGH || digitalRead(16) == HIGH) {  // stop when switch released
            pause_flag = true;
        }
    }

    this->carousel_current_pos = next_pos;  // Update current position

    if (this->carousel_current_pos == 1) {    // If position zero
        if (digitalRead(home_pin) == HIGH) {  // And homing switch not released
            this->home();                     // Home
        }
    }

    if (pause_flag) {
        pause_flag = false;
        return (2);
    }

    return (1);
}

void Carousel::home() {
    long overshoot_steps = 0;
	long remaining_steps = 0;
	long completed_steps = 0;

    if (digitalRead(home_pin) == HIGH) {
        BasicStepperDriver::startRotate(3 * 360);  // full 360 degree carousel rotation (3:1 gearing)

        while (BasicStepperDriver::nextAction()) {  // while the motor is turning
            if (digitalRead(home_pin) == LOW) {     // stop when switch released
                overshoot_steps = (BasicStepperDriver::steps_to_brake);
                remaining_steps = (BasicStepperDriver::steps_remaining);
                completed_steps = (BasicStepperDriver::step_count);
                BasicStepperDriver::startBrake();
            }
        }
    }

    float error_steps = (overshoot_steps < remaining_steps) ? overshoot_steps : remaining_steps;
    error_steps = (error_steps < completed_steps) ? error_steps : completed_steps;

    BasicStepperDriver::move((-1 * error_steps) - (16 * 9));  // rotate back past overshoot

    BasicStepperDriver::setRPM(5);             // SLOWLY
    BasicStepperDriver::startRotate(360 * 3);  // rotate upto 360 degrees (3:1 gearing)

    while (BasicStepperDriver::nextAction()) {  // while the motor is turning
        if (digitalRead(home_pin) == LOW) {     // stop when switch released
            BasicStepperDriver::stop();
        }
    }

    BasicStepperDriver::setRPM(500);  // set operational speed
    this->carousel_current_pos = 1;   // set current position to home
}

void Carousel::AddInterlock(bool (*pEvent)()) {
    pinterlock = pEvent;
    interlock_enable = true;
}