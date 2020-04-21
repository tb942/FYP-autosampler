#include "LeadScrew.h"

bool LeadScrew::extended_messages = false;
const uint8_t Leadscrew::lead_positions[3] = {0, 15, 133};

LeadScrew::LeadScrew(short Dir, short Step, short Ena, short Home)  // Constructor
    : BasicStepperDriver(200, Dir, Step, Ena), home_pin(Home) {     // Source from BasicStepperDriver class - set 200 steps per rotation
    lead_current_pos = 1;
}

void LeadScrew::begin() {
    pinMode(home_pin, INPUT);
    BasicStepperDriver::begin(300, 16);                                                  // Set 300 RPM and 16 microsteps
    BasicStepperDriver::setEnableActiveState(LOW);                                        // Set for TMC2100 driver
    BasicStepperDriver::setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 400, 400);  // Set acceleration
}

bool LeadScrew::is_safe() {
    if (lead_current_pos == 1) {
        return true;
    }
    return false;
}

void LeadScrew::SetVariables(bool extended_serial_messages)
{
	extended_messages = extended_serial_messages;
}

uint8_t LeadScrew::operate(uint8_t next_pos) {
    if (next_pos < 1 || next_pos > 3) {
        return (0);
    }

    bool pause_flag = false;

    int millimetres = lead_positions[next_pos - 1] - lead_positions[lead_current_pos - 1];  // Calculate current millimetre height error
    BasicStepperDriver::enable();                                                           // Enable motor
    BasicStepperDriver::startRotate(-180 * millimetres);                                    // 2mm per turn = 180 degrees per mm

    while (BasicStepperDriver::nextAction()) {                                           // while the motor is turning
        if (BasicStepperDriver::getDirection() == 1 && digitalRead(home_pin) == HIGH) {  // stop when switch pressed
            BasicStepperDriver::stop();
        }
        if (digitalRead(14) == HIGH || digitalRead(15) == HIGH || digitalRead(16) == HIGH) {  // stop when switch released
            pause_flag = true;
        }
    }

    BasicStepperDriver::disable();      // Disabe motor
    this->lead_current_pos = next_pos;  // Update current position

    if (this->lead_current_pos == 1) {  // home if moved to home
        this->home();
    }

    if (pause_flag) {
        pause_flag = false;
        return (2);
    }

    return (1);
}

uint8_t LeadScrew::LiquidLevel(uint8_t liquid_sensor_power_pin, uint8_t level_sensor_pin, bool liquid_detected_state, bool sensor_power_state, LiquidCrystal_I2C& lcd) {
	//this->home();
	digitalWrite(liquid_sensor_power_pin, sensor_power_state);
    operate(2);

    bool pause_flag = false;
    float level = 0;

    int millimetres = lead_positions[2] - lead_positions[1];  // Calculate current millimetre height error
    BasicStepperDriver::enable();                             // Enable motor
	BasicStepperDriver::setRPM(300);
    BasicStepperDriver::startRotate(-180 * millimetres);      // 2mm per turn = 180 degrees per mm

	bool triggered = false;

    while (BasicStepperDriver::nextAction()) {                                                       // while the motor is turning
        if (digitalRead(level_sensor_pin) == liquid_detected_state) {                                        // Stop when liquid detected
			if (!triggered) {
				digitalWrite(liquid_sensor_power_pin, !sensor_power_state);
				level = BasicStepperDriver::steps_remaining;
				triggered = true;
			}

        } else if (digitalRead(14) == HIGH || digitalRead(15) == HIGH || digitalRead(16) == HIGH) {  // Flag pause when switch pressed
            pause_flag = true;
        }
    }

	if (level != 0) {
		BasicStepperDriver::move(level + 20000);

		digitalWrite(liquid_sensor_power_pin, sensor_power_state);
		delay(250);

		BasicStepperDriver::setRPM(20);        // SLOWLY
		BasicStepperDriver::startMove(-30000);      // 2mm per turn = 180 degrees per mm

		level -= 10000;
		bool second_sensed = false;

		while (BasicStepperDriver::nextAction()) {                                                       // while the motor is turning
			if (digitalRead(level_sensor_pin) == liquid_detected_state) {                                        // Stop when liquid detected
				digitalWrite(liquid_sensor_power_pin, !sensor_power_state);
				level += BasicStepperDriver::steps_remaining;
				BasicStepperDriver::stop();                                                      // Save level
				second_sensed = true;

			}
			else if (digitalRead(14) == HIGH || digitalRead(15) == HIGH || digitalRead(16) == HIGH) {  // Flag pause when switch pressed
				pause_flag = true;
			}
		}


		if (second_sensed == false) {
			level = 0;
		}

		BasicStepperDriver::setRPM(300);
	}

	digitalWrite(liquid_sensor_power_pin, !sensor_power_state);

	if (extended_messages) {
		Serial.print("Liquid level = ");
	}

    if (level != 0) {
		float volume = 0.0000000000000002745200745*level*level*level- 0.000000000155922393 * level * level + 0.0003259716194 * level - 9.706485189;
        //Serial.print(volume,2);  // Print ml
		Serial.print(level);

		if (extended_messages) {
			Serial.print("ml");
		}
		else {
			Serial.print(";");
		}

		lcd.PrintToLcd(F("Liquid Level:"), String(level) + "ml");

    } else {
		if (extended_messages) {
			Serial.print("empty");  // If nothing detected - empty
		}
		else {
			Serial.print("0;");
		}
    }

    Serial.print("\n");

	BasicStepperDriver::move(BasicStepperDriver::calcStepsForRotation(long(180*millimetres)) - level);

    BasicStepperDriver::disable();  // Disabe motor

    this->home();

    if (pause_flag) {
        pause_flag = false;
        return (2);
    }

    return (1);
}

void LeadScrew::home() {
    BasicStepperDriver::enable();  // Enable Motor

    while (digitalRead(home_pin) == HIGH) {  // if the switch is pressed raise until it isn't
        BasicStepperDriver::rotate(-180 * 1);
    }

    BasicStepperDriver::startRotate(180 * 135);  // lower towards the switch

    while (BasicStepperDriver::nextAction()) {  // while the motor is turning
        if (digitalRead(home_pin) == HIGH) {    // stop when switch pressed
            BasicStepperDriver::stop();
        }
    }

    delay(200);

    BasicStepperDriver::rotate(-300);  // raise back off switch

    BasicStepperDriver::setRPM(20);        // SLOWLY
    BasicStepperDriver::startRotate(500);  // lower back to switch

    while (BasicStepperDriver::nextAction()) {  // while the motor is turning
        if (digitalRead(home_pin) == HIGH) {    // stop when switch pressed
            BasicStepperDriver::stop();
        }
    }

    BasicStepperDriver::setRPM(300);  // speed up
	BasicStepperDriver::rotate(120); //360 = 2mm, 270 = 1.5mm
    BasicStepperDriver::disable();     // Disable motor
    lead_current_pos = 1;              // set current position to home
}
