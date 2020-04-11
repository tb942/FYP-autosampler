#include "Commands.h"
#include <EEPROM.h>

uint8_t Command::sequence_count = 4;
bool Command::extended_messages = false;
LiquidCrystal_I2C* Command::lcd = nullptr;
uint8_t Command::command_count = 0;
CommandSettings* Command::available_commands = nullptr;

Command::Command(char t, uint8_t v) : type{t}, value{v} {}

Command::Command(Command& b) : type{b.type}, value{b.value} {}

Command::Command() {  // default constructor - W0 (do nothing)
    Command('W', 0);
}

void Command::LinkDisplay(LiquidCrystal_I2C& lcd_object) {  // Connect references to  display
    lcd = &lcd_object;
}

void Command::SetVariables(uint8_t eeprom_sequence_count,
                           bool extended_serial_messages) {  // Set number of programs and whether to bombard serial messages
    sequence_count = eeprom_sequence_count;
    extended_messages = extended_serial_messages;
}

bool Command::AddCommand(char type, uint8_t (*pEvent)(uint8_t), uint8_t minimum, uint8_t maximum) {
    for (int i = 0; i < command_count; i++) {
        if (available_commands[i].type == type) {
            return (false);
        }
    }

    CommandSettings* temp_available_commands = nullptr;
    temp_available_commands = new CommandSettings[++command_count];

    for (int i = 0; i <= command_count - 2; i++) {
        temp_available_commands[i] = available_commands[i];
    }
    temp_available_commands[command_count - 1] = {type, pEvent, minimum, maximum};

    delete[] available_commands;
    available_commands = nullptr;
    available_commands = temp_available_commands;

    return (true);
}

uint8_t Command::ProgramDelete(uint8_t program_number) {
    if (program_number > sequence_count) {  // If program is out of range, error
        Error(1);
        return (0);
    }

    int sequence_length = EEPROM.length() / sequence_count;  // Calculate program memory space

    for (int address_iterator = sequence_length * (program_number - 1); address_iterator < sequence_length * program_number;
         address_iterator++) {
        EEPROM.update(address_iterator, 0);  // Wipe each address
    }

    return (1);
}

uint8_t Command::ProgramRun(uint8_t program_number) {
    if (program_number > sequence_count) {  // If program is out of range, error
        Error(1);
        return (0);
    }

    int sequence_length = EEPROM.length() / sequence_count;

    Command command_buffer;

    for (int address_iterator = sequence_length * (program_number - 1); address_iterator < sequence_length * program_number;
         address_iterator += sizeof(Command)) {           // For each memory address
        command_buffer.ReadFromEeprom(address_iterator);  // Read EEPROM into command buffer

        if (address_iterator == sequence_length * (program_number - 1)) {  // If first command is...
            if (command_buffer.GetType() == 'P') {                         // A 'P'....
                if (command_buffer.GetValue() == program_number) {         // If first command is the CORRECT 'P'

                    command_buffer.SetType('L');  // Home the lead screw
                    command_buffer.SetValue(1);

                } else {
                    command_buffer.Error(6);
                    return (0);
                }
            } else {
                command_buffer.Error(6);
                return (0);
            }
        } else {                                    // If any following command...
            if (command_buffer.GetType() == 'P') {  // Is a P, program done!
				if (extended_messages) {
					Serial.print(F("End of program\n"));
				}
                return (1);
            }
        }

        command_buffer.Operate();  // Do the read command
    }

    command_buffer.Error(8);  // If no ending 'P' command
    return (0);               // Clear program, error and return
}

uint8_t Command::ProgramWrite(uint8_t program_number) {
    if (program_number > sequence_count) {  // If program is out of range, error
        Error(1);
        return (0);
    }

    ProgramDelete(program_number);  // Clear the chosen program memory

    int sequence_length = EEPROM.length() / sequence_count;

    Command command_buffer;

    for (int address_iterator = sequence_length * (program_number - 1); address_iterator < sequence_length * program_number;
         address_iterator += sizeof(Command)) {  // For each memory address

        while (Serial.available() == false) {
        }
        if (command_buffer.GetCommandSerial() == false) {  // Read serial into command buffer
            ProgramDelete(program_number);
            serialFlush();
            return (0);
        }

        command_buffer.WriteToEeprom(address_iterator);  // Write EEPROM with command buffer

        if (address_iterator == sequence_length * (program_number - 1)) {  // If first Command...
            if (command_buffer.GetType() == 'P') {                         // Is a 'P'...
                if (command_buffer.GetValue() == program_number) {         // Is the CORRECT 'P'
                    // continue;                                              // Accept it
                } else {  // Else clear program, error and return
                    ProgramDelete(program_number);
                    serialFlush();
                    command_buffer.Error(6);
                    return (0);
                }
            } else {  // Else clear program, error and return
                ProgramDelete(program_number);
                serialFlush();
                command_buffer.Error(6);
                return (0);
            }
        } else {  // If any following command...
            if (command_buffer.GetType() == 'R' || command_buffer.GetType() == 'W' ||
                command_buffer.GetType() == 'D') {  // Is an EEPROM command (illegal)
                ProgramDelete(program_number);      // Clear program, error and return
                serialFlush();
                command_buffer.Error(7);
                return (0);
            }
            if (command_buffer.GetType() == 'P') {  // Is a P, program done!
                return (1);
            }
        }

		if (extended_messages) {
			Serial.print(F("OK "));
		}
		command_buffer.PrintToSerial();
        Serial.print(F("\n"));
    }

    ProgramDelete(program_number);  // If no ending 'P' command
    serialFlush();                  // Clear program, error and return
    command_buffer.Error(8);
    return (0);
}

void Command::PrintToSerial() {  // Output command to serial
    Serial.print(type);
    Serial.print(value);
}

bool Command::GetCommandSerial() {                 // Read command (capitalised) from serial
    String command = Serial.readStringUntil(';');  // Read one command

    type = command[0] & ~(0x20);  // Capitalised first char
    value = command.substring(1).toInt();

    if (command.length() > 3 || command.length() < 2) {  // Check input length
        Error(2);
        return false;
    }

    return true;
}

void Command::WriteToEeprom(unsigned int address) {  // Store command at EEPROM address
    EEPROM.put(address, *this);
    return;
}

void Command::ReadFromEeprom(unsigned int address) {  // Retrieve command from EEPROM address
    EEPROM.get(address, *this);
    return;
}

void Command::SetValue(uint8_t v) { value = v; }

uint8_t Command::GetValue() { return (value); }

void Command::SetType(char t) { type = t; }

char Command::GetType() { return (type); }

bool Command::Operate() {
    if (value == 0) {  // If toInt failed, Error and return
        Error(5);
        return (false);
    }

    bool function_success = false;

    lcd->PrintToLcd(F("Command starting:"), "\"" + String(type) + String(value) + "\"");  // Display begin diagnostic

    if (extended_messages) {
        Serial.print(F("Command starting: \""));  // Anounce completed command to serial
		PrintToSerial();
        Serial.print(F("\"\n"));
    }

    for (int i = 0; i < command_count; i++) {
        if (type == available_commands[i].type) {
            if (value > available_commands[i].maximum_value || value < available_commands[i].minimum_value) {
                Error(1);
                return (false);
            }

            switch (available_commands[i].pEvent(value)) {
                case 0:
                    Error(1);
                    return (false);

                case 3:  // If interlock not OK error and return
                    Error(4);
                    return (false);

                case 2:  // If button was pressed
                    Pause();

                default:
                    function_success = true;
                    break;
            }
        }
    }

    if (function_success) {
        lcd->PrintToLcd(F("Command completed:"), "\"" + String(type) + String(value) + "\"");

		if (extended_messages) {
			Serial.print(F("Command completed: \""));  // Anounce completed command to serial
			PrintToSerial();
			Serial.print(F("\"\n"));
		} else {
			PrintToSerial();
			Serial.print(";\n");
		}
        return (true);
    }

    Error(3);
    return (false);
}

void Command::Error(uint8_t error) {
    /*
     *  Error:
     *    0 == Generic
     *    1 == Command value out of range
     *    2 == Command too short/long
     *    3 == No command character found
     *    4 == Carousel move attempt without leadscrew homed
     *    5 == No valid int found in command
     *    6 == First EEPROM command not correct P
     *    7 == Illegal command (Eeprom) in program
     *    8 == Last EEPROM command not P
     *
     */

    switch (error) {
        case 0:
            Serial.print(F("E0"));
			if (extended_messages) {
				Serial.print(F("-Error!"));
			}
			else {
				Serial.print(F(";"));
			}
            lcd->PrintToLcd(F("E0, Generic Error"));
            break;

        case 1:
            Serial.print(F("E1"));
			if (extended_messages) {
				Serial.print(F(" - "));
				switch (type) {
				case 'C':
				case 'c':
					Serial.print(F("Carousel"));
					break;
				case 'L':
				case 'l':
					Serial.print(F("Leadscrew"));
					break;
				case 'W':
				case 'w':
					Serial.print(F("Wait"));
					break;
				case 'E':
				case 'e':
					Serial.print(F("EEPROM"));
					break;
				default:
					break;
				}
				Serial.print(F(" operate failed - int "));
				PrintToSerial();
				Serial.print(F(" out of range!"));
			}
			else {
				Serial.print(F(";"));
			}
            lcd->PrintToLcd("E1, val: \"" + String(value) + "\"", F("out of range"));
            break;

        case 2:
            Serial.print(F("E2"));
			if (extended_messages) {
				Serial.print(F(" - Command: \""));
				PrintToSerial();
				Serial.print(F("\" length out of range!"));
			}
			else {
				Serial.print(F(";"));
			}
            lcd->PrintToLcd("E2, \"" + String(type) + String(value) + "\"", F("Length Range"));
            break;

        case 3:
            Serial.print(F("E3"));
			if (extended_messages) {
				Serial.print(F(" - No command found in: \""));
				PrintToSerial();
				Serial.print(F("\""));
			}
			else {
				Serial.print(F(";"));
			}
            lcd->PrintToLcd("E3, \"" + String(type) + String(value) + "\"", F("No command found"));
            break;

        case 4:
            Serial.print(F("E4"));
			if (extended_messages) {
				Serial.print(F(" - Leadscrew not homed - cannot complete command \""));
				PrintToSerial();
				Serial.print(F("\""));
			}
			else {
				Serial.print(F(";"));
			}
            lcd->PrintToLcd(F("E4"), F("Home leadscrew"));
            break;

        case 5:
            Serial.print(F("E5"));
			if (extended_messages) {
				Serial.print(F(" - No valid int found in command: \"" ));
				PrintToSerial();
				Serial.print(F("\""));
			}
			else {
				Serial.print(F(";"));
			}
            lcd->PrintToLcd("E5, \"" + String(type) + String(value) + "\"");
            break;

        case 6:
            Serial.print(F("E6"));
			if (extended_messages) {
				Serial.print(F(" - First command not correct P command - \""));
				PrintToSerial();
				Serial.print(F("\""));
			}
			else {
				Serial.print(F(";"));
			}
            break;

        case 7:
            Serial.print(F("E7"));
			if (extended_messages) {
				Serial.print(F(" - Illegal command (EEPROM) in program - \""));
				PrintToSerial();
				Serial.print(F("\""));
			}
			else {
				Serial.print(F(";"));
			}
            break;

        case 8:
            Serial.print(F("E8"));
			if (extended_messages) {
				Serial.print(F(" - Last command not P0 - \"" ));
				PrintToSerial();
				Serial.print(F("\""));
			}
			else {
				Serial.print(F(";"));
			}
            break;

        default:
            break;
    }

    Serial.print(F("\n"));  // New line
}

void Command::Pause() {
    lcd->PrintToLcd(F("Paused: Press"), F("any button"));
    while (digitalRead(14) != HIGH && digitalRead(15) != HIGH && digitalRead(16) != HIGH) {
    }
    while (digitalRead(14) == HIGH || digitalRead(15) == HIGH || digitalRead(16) == HIGH) {
    }
}

void Command::serialFlush() {
    while (Serial.available() > 0) {
        Serial.read();
    }
}
