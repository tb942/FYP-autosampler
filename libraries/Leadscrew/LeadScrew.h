#ifndef Leadscrew_h
#define Leadscrew_h

#include <Arduino.h>
#include "BasicStepperDriver.h"
#include "LiquidCrystal_I2C.h"

class LeadScrew : public BasicStepperDriver {
    const uint8_t lead_positions[3] = {0, 15, 133};
    uint8_t lead_current_pos;
    uint8_t home_pin;
	static bool extended_messages;

   public:
    LeadScrew(short Dir, short Step, short Ena, short Home);

	static void SetVariables(bool extended_serial_messages);
    uint8_t operate(uint8_t next_pos);
	uint8_t LiquidLevel(uint8_t liquid_sensor_power_pin, uint8_t level_sensor_pin, bool liquid_detected_state, bool sensor_power_state, LiquidCrystal_I2C& lcd);
    void home();
    void begin();
    bool is_safe();
};

#endif