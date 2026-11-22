#include "Arduino.h"
unsigned long sim_ms = 0;
SerialClass Serial;
EspClass ESP;
size_t Print::print(const String& s) { return write(s.c_str()); }
