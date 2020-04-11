#ifndef Commands_h
#define Commands_h

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "Carousel.h"
#include "LeadScrew.h"

struct CommandSettings {
	char type;
	uint8_t(*pEvent)(uint8_t);
	uint8_t minimum_value;
	uint8_t maximum_value;
};

class Command {
   private:
    char type;
    uint8_t value;
    static uint8_t sequence_count;
    static bool extended_messages;
    static LiquidCrystal_I2C *lcd;
	static uint8_t command_count;
	static CommandSettings* available_commands;

    void Pause();
    void serialFlush();

   public:
    Command(char t, uint8_t v);
    Command(Command &b);
    Command();

    static void LinkDisplay(LiquidCrystal_I2C &lc);
    static void SetVariables(uint8_t eeprom_sequence_count, bool extended_serial_messages);
	static bool AddCommand(char type, uint8_t(*pEvent)(uint8_t), uint8_t minimum, uint8_t maximum);
	uint8_t ProgramDelete(uint8_t program_number);
	uint8_t ProgramRun(uint8_t program_number);
	uint8_t ProgramWrite(uint8_t program_number);
    void PrintToSerial();
    bool GetCommandSerial();
	void WriteToEeprom(unsigned int address);
	void ReadFromEeprom(unsigned int address);
    void SetValue(int v);
    int GetValue();
    void SetType(char t);
    char GetType();
    bool Operate();
    void Error(uint8_t error);
};

#endif  // !Commands_h