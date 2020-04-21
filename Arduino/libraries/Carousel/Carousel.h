#ifndef Carousel_h
#define Carousel_h

#include <Arduino.h>
#include "BasicStepperDriver.h"

class Carousel : public BasicStepperDriver {
    uint8_t carousel_current_pos;
    uint8_t home_pin;

	bool interlock_enable = false;
	bool (*pinterlock)();

   public:
    Carousel(short Dir, short Step, short Ena, short Home);

    uint8_t operate(uint8_t next_pos);
    void home();
    void begin();
	void AddInterlock(bool (*pEvent)());
};

#endif