// ----------------------------------------------------------------------------
// LickportArrayController.cpp
//
//
// Authors:
// Peter Polidoro peter@polidoro.io
// ----------------------------------------------------------------------------
#include "../LickportArrayController.h"


using namespace lickport_array_controller;

LickportArrayController::LickportArrayController()
{
}

void LickportArrayController::setup()
{
  // Parent Setup
  DigitalController::setup();

  // Reset Watchdog
  resetWatchdog();

  // Variable Setup

  // Set Device ID
  modular_server_.setDeviceName(constants::device_name);

  // Pin Setup
  for (size_t lickport=0; lickport<constants::LICKPORT_COUNT; ++lickport)
  {
    pinMode(constants::lickport_pins[lickport],OUTPUT);
  }

  // Add Hardware
  modular_server_.addHardware(constants::hardware_info,
    pins_);

  // Pins
  modular_server::Pin & change_pin = modular_server_.createPin(constants::change_pin_name,
    constants::change_pin_number);
  change_pin.setModeDigitalInput();

  modular_server::Pin & lick_detected_pin = modular_server_.createPin(constants::lick_detected_pin_name,
    constants::lick_detected_pin_number);
  lick_detected_pin.setModePulseRising();

  modular_server::Pin & sync_pin = modular_server_.createPin(constants::sync_pin_name,
    constants::sync_pin_number);
  sync_pin.setModePulseFalling();

  // Add Firmware
  modular_server_.addFirmware(constants::firmware_info,
    properties_,
    parameters_,
    functions_,
    callbacks_);

  // Properties
  modular_server::Property & channel_count_property = modular_server_.property(digital_controller::constants::channel_count_property_name);
  channel_count_property.disableFunctors();
  channel_count_property.setDefaultValue(constants::lickport_count);
  channel_count_property.setRange(constants::lickport_count,constants::lickport_count);
  channel_count_property.reenableFunctors();

  // Parameters
  modular_server::Parameter & lickport_parameter = modular_server_.createParameter(constants::lickport_parameter_name);
  lickport_parameter.setRange(constants::lickport_min,constants::lickport_count-1);

  modular_server::Parameter & lickports_parameter = modular_server_.createParameter(constants::lickports_parameter_name);
  lickports_parameter.setRange(constants::lickport_min,constants::lickport_count-1);
  lickports_parameter.setArrayLengthRange(constants::lickports_array_length_min,constants::lickport_count-1);

  modular_server::Parameter & dispense_duration_parameter = modular_server_.createParameter(constants::dispense_duration_parameter_name);
  dispense_duration_parameter.setRange(constants::dispense_duration_min,constants::dispense_duration_max);
  dispense_duration_parameter.setUnits(digital_controller::constants::ms_units);

  modular_server::Parameter & dispense_durations_parameter = modular_server_.createParameter(constants::dispense_durations_parameter_name);
  dispense_durations_parameter.setRange(constants::lickport_min,constants::lickport_count-1);
  dispense_durations_parameter.setArrayLengthRange(constants::dispense_durations_array_length_min,constants::lickport_count-1);

  setChannelCountHandler();

  // Functions
  modular_server::Function & dispense_lickport_for_duration_function = modular_server_.createFunction(constants::dispense_lickport_for_duration_function_name);
  dispense_lickport_for_duration_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&LickportArrayController::dispenseLickportForDurationHandler));
  dispense_lickport_for_duration_function.addParameter(lickport_parameter);
  dispense_lickport_for_duration_function.addParameter(dispense_duration_parameter);

  modular_server::Function & dispense_lickports_for_duration_function = modular_server_.createFunction(constants::dispense_lickports_for_duration_function_name);
  dispense_lickports_for_duration_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&LickportArrayController::dispenseLickportsForDurationHandler));
  dispense_lickports_for_duration_function.addParameter(lickports_parameter);
  dispense_lickports_for_duration_function.addParameter(dispense_duration_parameter);

  modular_server::Function & dispense_all_lickports_for_duration_function = modular_server_.createFunction(constants::dispense_all_lickports_for_duration_function_name);
  dispense_all_lickports_for_duration_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&LickportArrayController::dispenseAllLickportsForDurationHandler));
  dispense_all_lickports_for_duration_function.addParameter(dispense_duration_parameter);

  modular_server::Function & dispense_lickports_for_durations_function = modular_server_.createFunction(constants::dispense_lickports_for_durations_function_name);
  dispense_lickports_for_durations_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&LickportArrayController::dispenseLickportsForDurationsHandler));
  dispense_lickports_for_durations_function.addParameter(lickports_parameter);
  dispense_lickports_for_durations_function.addParameter(dispense_durations_parameter);

  // Callbacks
  modular_server::Callback & check_lick_status_callback = modular_server_.createCallback(constants::check_lick_status_callback_name);
  check_lick_status_callback.attachFunctor(makeFunctor((Functor1<modular_server::Pin *> *)0,*this,&LickportArrayController::checkLickStatusHandler));
  check_lick_status_callback.attachTo(change_pin,modular_server::constants::pin_mode_interrupt_falling);

  setAllChannelsOff();

  lick_sensor_.begin();
  lick_sensor_.reset();
  check_lick_status_ = false;
}

void LickportArrayController::update()
{
  // Parent Update
  DigitalController::update();

  checkLickStatus();
}

void LickportArrayController::dispenseLickportForDuration(uint8_t lickport,
  uint32_t dispense_duration)
{
  uint32_t lickports = 0;
  lickports |= (1 << lickport);
  addPwm(lickports,
    constants::dispense_power,
    constants::dispense_delay,
    dispense_duration,
    dispense_duration,
    constants::dispense_count);
}

void LickportArrayController::dispenseAllLickportsForDuration(uint32_t dispense_duration)
{
  for (uint8_t lickport=0; lickport<constants::lickport_count; ++lickport)
  {
    dispenseLickportForDuration(lickport,dispense_duration);
  }
}

void LickportArrayController::dispenseLickportsForDuration(const Lickports & lickports,
  uint32_t dispense_duration)
{
  for (uint8_t lickport : lickports)
  {
    dispenseLickportForDuration(lickport,dispense_duration);
  }
}

void LickportArrayController::dispenseLickportsForDurations(const Lickports & lickports,
  const DispenseDurations dispense_durations)
{
  if (lickports.size() != dispense_durations.size())
  {
    return;
  }
  for (uint8_t index=0; index<lickports.size(); ++index)
  {
    dispenseLickportForDuration(lickports[index],dispense_durations[index]);
  }
}

double LickportArrayController::setChannelToPower(size_t lickport,
  double power)
{
  if (power > digital_controller::constants::power_mid)
  {
    power = digital_controller::constants::power_max;
    digitalWrite(constants::lickport_pins[lickport],HIGH);
  }
  else
  {
    power = digital_controller::constants::power_min;
    digitalWrite(constants::lickport_pins[lickport],LOW);
  }
  return power;
}

LickportArrayController::Lickports LickportArrayController::jsonArrayToLickports(ArduinoJson::JsonArray lickports_json_array)
{
  Lickports lickports;
  for (uint8_t lickport : lickports_json_array)
  {
    lickports.push_back(lickport);
  }
  return lickports;
}

void LickportArrayController::checkLickStatus()
{
  if (check_lick_status_)
  {
    check_lick_status_ = false;
    LickSensorStatus lick_sensor_status = lick_sensor_.getStatus();
    if (lick_sensor_status.any_key_touched)
    {
      modular_server::Pin & lick_detected_pin = modular_server_.pin(constants::lick_detected_pin_name);
      lick_detected_pin.setValue(constants::lick_detected_pulse_duration);
    }
  }
}

void LickportArrayController::checkLickStatusHandler(modular_server::Pin * pin_ptr)
{
  check_lick_status_ = true;
}

void LickportArrayController::dispenseLickportForDurationHandler()
{
  long lickport;
  modular_server_.parameter(constants::lickport_parameter_name).getValue(lickport);

  long dispense_duration;
  modular_server_.parameter(constants::dispense_duration_parameter_name).getValue(dispense_duration);

  dispenseLickportForDuration(lickport,dispense_duration);
}

void LickportArrayController::dispenseLickportsForDurationHandler()
{
  ArduinoJson::JsonArray lickports_json_array;
  modular_server_.parameter(constants::lickports_parameter_name).getValue(lickports_json_array);
  Lickports lickports = jsonArrayToLickports(lickports_json_array);

  long dispense_duration;
  modular_server_.parameter(constants::dispense_duration_parameter_name).getValue(dispense_duration);

  dispenseLickportsForDuration(lickports,dispense_duration);
}

void LickportArrayController::dispenseLickportsForDurationsHandler()
{
  ArduinoJson::JsonArray lickports_json_array;
  modular_server_.parameter(constants::lickports_parameter_name).getValue(lickports_json_array);
  Lickports lickports = jsonArrayToLickports(lickports_json_array);

  ArduinoJson::JsonArray dispense_durations_json_array;
  modular_server_.parameter(constants::dispense_durations_parameter_name).getValue(dispense_durations_json_array);
  DispenseDurations dispense_durations = jsonArrayToDispenseDurations(dispense_durations_json_array);

  dispenseLickportsForDuration(lickports,dispense_durations);
}

void LickportArrayController::dispenseAllLickportsForDurationHandler()
{
  long dispense_duration;
  modular_server_.parameter(constants::dispense_duration_parameter_name).getValue(dispense_duration);

  dispenseAllLickportsForDuration(dispense_duration);
}
