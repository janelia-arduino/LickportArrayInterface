// ----------------------------------------------------------------------------
// TEENSY40.cpp
//
//
// Authors:
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#include "TEENSY40.h"


#if defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40)
namespace lickport_array_controller
{
namespace constants
{
const modular_server::HardwareInfo hardware_info =
{
  .name_ptr=&hardware_name,
  .part_number=1240,
  .version_major=1,
  .version_minor=0,
};

// Pins
const size_t sensor_pin = 2;
const size_t led_pin = 2;

// Units

// Properties

// Parameters

// Functions

// Callbacks

// Errors
}
}

#endif
