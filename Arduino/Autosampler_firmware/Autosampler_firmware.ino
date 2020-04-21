#include <Arduino.h>
#include <Carousel.h>
#include <Commands.h>
#include <LeadScrew.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Program Settings
const uint8_t eeprom_sequence_count = 4;
bool extended_serial_messages = true;

// Pin definitions
const uint8_t carousel_step = 10;
const uint8_t carousel_dir = 9;
const uint8_t carousel_ena = 8;
const uint8_t carousel_home = 3;

const uint8_t leadscrew_step = 7;
const uint8_t leadscrew_dir = 6;
const uint8_t leadscrew_ena = 5;
const uint8_t leadscrew_home = 2;

const uint8_t liquid_level_pin = 4;
const uint8_t liquid_sensor_power_pin = 11;
const bool liquid_state = false;
const bool sensor_power_on_state = false;

const uint8_t menu_left = 16;
const uint8_t menu_enter = 15;
const uint8_t menu_right = 14;

uint8_t menu_selection = 1;

// Function declarations
void ConfirmSelection();
void Instructions();
bool IsLeadHomed();
uint8_t CarouselOperate(uint8_t pos);
uint8_t LeadscrewOperate(uint8_t pos);
uint8_t Run(uint8_t program);
uint8_t Write(uint8_t program);
uint8_t Delete(uint8_t program);
uint8_t Sleep(uint8_t value);
uint8_t LiquidLevel(uint8_t pos);
uint8_t MessageMode(uint8_t mode);

// Construction:Mechanism name(DIR, STEP, ENA, HOME);
Carousel carousel(carousel_dir, carousel_step, carousel_ena, carousel_home);
LeadScrew lead(leadscrew_dir, leadscrew_step, leadscrew_ena, leadscrew_home);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);  // 0x27 is the I2C bus address for an unmodified backpack
Command command;                                   // Holder to read serial buffer into

void setup() {
    carousel.AddInterlock(IsLeadHomed);               // Set leadscrew homed as carousel interlock
    Command::AddCommand('C', CarouselOperate, 1, 8);  // Set commands
    Command::AddCommand('L', LeadscrewOperate, 1, 3);
    Command::AddCommand('V', LiquidLevel, 1, 8);
    Command::AddCommand('R', Run, 1, eeprom_sequence_count);
    Command::AddCommand('W', Write, 1, eeprom_sequence_count);
    Command::AddCommand('D', Delete, 1, eeprom_sequence_count);
    Command::AddCommand('S', Sleep, 1, 99);
    Command::AddCommand('M', MessageMode, 1, 2);
    Command::SetVariables(eeprom_sequence_count, extended_serial_messages);  // Pass compilation settings
    LeadScrew::SetVariables(extended_serial_messages);
    Command::LinkDisplay(lcd);                                               // Pass display control

    Serial.begin(9600);

    pinMode(menu_left, INPUT);  // Initialise pins
    pinMode(menu_enter, INPUT);
    pinMode(menu_right, INPUT);

    pinMode(liquid_level_pin, INPUT);
    pinMode(liquid_sensor_power_pin, OUTPUT);
    digitalWrite(liquid_sensor_power_pin, !sensor_power_on_state);

    lead.begin();      // Initialise lead screw
    carousel.begin();  // Initialise carousel

    // Activate LCD module
    lcd.begin(16, 2);  // As 16 x 2 LCD module
    lcd.setBacklightPin(3, POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.PrintToLcd(F("Autosampler"));  // Splash screen

    lead.home();  // Home lead screw

    if (extended_serial_messages) {
        Serial.print(F("Leadscrew homed\n"));
    }

    lcd.PrintToLcd(F("Leadscrew homed"));

    carousel.home();  // home carousel

    if (extended_serial_messages) {
        Serial.print(F("Carousel homed\n"));
    }

    lcd.PrintToLcd(F("Carousel homed"));

    Instructions();

    Sleep(1);
}

void loop() {
    lcd.PrintToLcd(F("Program"), "Selected: " + String(menu_selection));

    if (Serial.available()) {
        command.GetCommandSerial();  // Read one command
        command.Operate();           // Handle it
    }

    if (digitalRead(menu_enter)) {
        while (digitalRead(menu_enter)) {
        }

        ConfirmSelection();

    } else if (digitalRead(menu_left)) {
        while (digitalRead(menu_left)) {
        }

        menu_selection--;
        if (menu_selection == 0) {
            menu_selection = 4;
        }

    } else if (digitalRead(menu_right)) {
        while (digitalRead(menu_right)) {
        }

        menu_selection++;
        if (menu_selection == 5) {
            menu_selection = 1;
        }
    }
}

void ConfirmSelection() {
    lcd.PrintToLcd(F("Confirm program:"), String(menu_selection) + " Yes(L) / No(R)");
    while (!digitalRead(menu_left) && !digitalRead(menu_right)) {
    }

    if (digitalRead(menu_left)) {
        while (digitalRead(menu_left)) {
        }
        Command program('R', menu_selection);
        program.Operate();
    }
}

void Instructions() {
    if (extended_serial_messages) {
        Serial.print(F("\n\nAutosampler commands format:\n"));
        Serial.print(F("           \"ANN;\"\n\n"));
        Serial.print(F("A = Command, NN = value\n      options:\n\n"));
        Serial.print(F("\"C\"/\"c\" == Carousel\n"));
        Serial.print(F("Valid positions: 1-8\n\n"));
        Serial.print(F("----------OR----------\n\n"));
        Serial.print(F("\"L\"/\"l\" == Leadscrew\n"));
        Serial.print(F("Valid positions: 1-3\n\n"));
        Serial.print(F("----------OR----------\n\n"));
        Serial.print(F("\"V\"/\"v\" == Level Value in tube\n"));
        Serial.print(F("Valid positions: 1-8\n\n"));
        Serial.print(F("----------OR----------\n\n"));
        Serial.print(F("\"S\"/\"s\" == Sleep\n"));
        Serial.print(F("Valid lengths: 1-99\n\n"));
        Serial.print(F("----------OR----------\n\n"));
        Serial.print(F("\"W\"/\"w\" == Write\n"));
        Serial.print(F("Valid lengths: 1-4\n\n"));
        Serial.print(F("----------OR----------\n\n"));
        Serial.print(F("\"R\"/\"r\" == Run\n"));
        Serial.print(F("Valid lengths: 1-4\n\n"));
        Serial.print(F("----------OR----------\n\n"));
        Serial.print(F("\"D\"/\"d\" == Delete\n"));
        Serial.print(F("Valid lengths: 1-4\n\n"));
    }
}

bool IsLeadHomed() { return (lead.is_safe()); }

uint8_t CarouselOperate(uint8_t pos) { return (carousel.operate(pos)); }

uint8_t LeadscrewOperate(uint8_t pos) { return (lead.operate(pos)); }

uint8_t Run(uint8_t program) { return (command.ProgramRun(program)); }

uint8_t Write(uint8_t program) { return (command.ProgramWrite(program)); }

uint8_t Delete(uint8_t program) { return (command.ProgramDelete(program)); }

uint8_t Sleep(uint8_t value) {
    uint8_t pause_flag = 1;
    unsigned long start = millis();

    while (millis() < start + (1000 * (unsigned long)value)) {
        if (digitalRead(14) == HIGH || digitalRead(15) == HIGH || digitalRead(16) == HIGH) {  // flag pause if button pressed
            pause_flag = 2;
        }
    }

    return (pause_flag);
}

uint8_t LiquidLevel(uint8_t pos) {
    if(lead.is_safe()){
      carousel.operate(pos);
    } else {
      lead.operate(1);
      carousel.operate(pos);
    }
    return (lead.LiquidLevel(liquid_sensor_power_pin, liquid_level_pin, liquid_state, sensor_power_on_state, lcd));
}

uint8_t MessageMode(uint8_t mode) {
  if(mode == 2){
    extended_serial_messages = true;
  }
  else {
    extended_serial_messages = false;
  }
  
  Command::SetVariables(eeprom_sequence_count, extended_serial_messages);
  LeadScrew::SetVariables(extended_serial_messages);
  
  return(1);
}
