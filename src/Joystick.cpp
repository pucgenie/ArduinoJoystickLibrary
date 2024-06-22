/*
  Joystick.cpp

  Copyright (c) 2015-2017, Matthew Heironimus

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Joystick.h"

#if defined(_USING_DYNAMIC_HID)

#define JOYSTICK_REPORT_ID_INDEX 7
#define JOYSTICK_AXIS_MINIMUM 0
#define JOYSTICK_AXIS_MAXIMUM 65535
#define JOYSTICK_SIMULATOR_MINIMUM 0
#define JOYSTICK_SIMULATOR_MAXIMUM 65535

#define BUTTONVALUES_SIZE(buttonCount) (buttonCount + 7) / 8

#if 0
struct Page1 {
  uint16_t usage_page;
  uint16_t usage;
  uint16_t collection;
  uint16_t report_id;
};
struct HidReportDescriptor {
   struct Page1 page1;
};
#endif


Joystick_::Joystick_(
	const uint8_t hidReportId,
	const uint8_t joystickType,
	const uint8_t buttonCount
	#ifndef Joystick_DISABLE_HATSWITCH
	, const uint8_t hatSwitchCount
	#endif
	#ifndef Joystick_DISABLE_AXISES
	, const uint8_t includeAxisFlags,
	const uint8_t includeSimulatorFlags
	#endif
	#ifndef Joystick_DISABLE_AUTOSEND
	, const bool initAutoSendState
	#endif
	) :
		#ifndef Joystick_DISABLE_AUTOSEND
			_autoSendState(initAutoSendState),
		#endif
		#ifndef Joystick_DISABLE_HATSWITCH
			_hatSwitchCount(hatSwitchCount),
			_hatSwitchValues(new int16_t[hatSwitchCount]),
		#endif
		#ifndef Joystick_DISABLE_AXISES
			_includeAxisFlags(includeAxisFlags),
			_includeSimulatorFlags(includeSimulatorFlags),
		#endif
		_buttonCount(buttonCount)
{
		// Build Joystick HID Report Description

	// Button Calculations
	uint8_t buttonPaddingBits;
	{
		const uint8_t buttonsInLastByte = _buttonCount % 8;
		buttonPaddingBits = (buttonsInLastByte > 0)
		? 8 - buttonsInLastByte
		: 0;
	}
	
	#ifndef Joystick_DISABLE_AXISES
	// Axis Calculations
    uint8_t axisCount = 0;
    for(int i = 1; i != 0; i <<= 1)
        if(_includeAxisFlags & i)
            axisCount++;
		
	uint8_t simulationCount = 0;
    for(int i = 1; i != 0; i <<= 1)
        if(_includeSimulatorFlags & i)
            simulationCount++;
	#endif
		
    // TODO: It's a struct with multiple VLAs. Good luck.
    uint8_t tempHidReportDescriptor[142];
    int hidReportDescriptorSize = 0;

    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE (Joystick - 0x04; Gamepad - 0x05; Multi-axis Controller - 0x08)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = joystickType;

    // COLLECTION (Application)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // REPORT_ID (Default: 3)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x85;
    tempHidReportDescriptor[hidReportDescriptorSize++] = hidReportId;
	
	if (_buttonCount > 0) {

		// USAGE_PAGE (Button)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;

		// USAGE_MINIMUM (Button 1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x19;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// USAGE_MAXIMUM (Button 32)            
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x29;
		tempHidReportDescriptor[hidReportDescriptorSize++] = _buttonCount;

		// LOGICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// LOGICAL_MAXIMUM (1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// REPORT_SIZE (1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// REPORT_COUNT (# of buttons)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
		tempHidReportDescriptor[hidReportDescriptorSize++] = _buttonCount;

		// UNIT_EXPONENT (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x55;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// UNIT (None)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// INPUT (Data,Var,Abs)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

		if (buttonPaddingBits > 0) {
			
			// REPORT_SIZE (1)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

			// REPORT_COUNT (# of padding bits)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
			tempHidReportDescriptor[hidReportDescriptorSize++] = buttonPaddingBits;
					
			// INPUT (Const,Var,Abs)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;
			
		} // Padding Bits Needed

	} // Buttons

	if (
		#ifndef Joystick_DISABLE_AXISES
			(axisCount > 0) || 
		#endif
		#ifndef Joystick_DISABLE_HATSWITCH
			(_hatSwitchCount > 0) || 
		#endif
		// let compiler optimize that away, no more #if defined() || defined()
		false) {
	
		// USAGE_PAGE (Generic Desktop)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;
		
	}
	#ifndef Joystick_DISABLE_HATSWITCH
	if (_hatSwitchCount > 0) {

		// USAGE (Hat Switch)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x39;

		// LOGICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// LOGICAL_MAXIMUM (7)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x07;

		// PHYSICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// PHYSICAL_MAXIMUM (315)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x46;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x3B;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// UNIT (Eng Rot:Angular Pos)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x14;

		// REPORT_SIZE (4)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x04;

		// REPORT_COUNT (1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;
						
		// INPUT (Data,Var,Abs)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
		
		if (_hatSwitchCount > 1) {
			
			// USAGE (Hat Switch)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x39;

			// LOGICAL_MINIMUM (0)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

			// LOGICAL_MAXIMUM (7)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x07;

			// PHYSICAL_MINIMUM (0)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

			// PHYSICAL_MAXIMUM (315)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x46;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x3B;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

			// UNIT (Eng Rot:Angular Pos)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x14;

			// REPORT_SIZE (4)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x04;

			// REPORT_COUNT (1)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;
							
			// INPUT (Data,Var,Abs)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
		
		} else {
		
			// Use Padding Bits
		
			// REPORT_SIZE (1)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

			// REPORT_COUNT (4)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x04;
					
			// INPUT (Const,Var,Abs)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;
			
		} // One or Two Hat Switches?

	} // Hat Switches
	#endif

	#ifndef Joystick_DISABLE_AXISES
	if (axisCount > 0) {
	
		// USAGE (Pointer)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// LOGICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// LOGICAL_MAXIMUM (65535)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x27;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0XFF;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0XFF;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// REPORT_SIZE (16)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

		// REPORT_COUNT (axisCount)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
		tempHidReportDescriptor[hidReportDescriptorSize++] = axisCount;
						
		// COLLECTION (Physical)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		if (_includeAxisFlags & JOYSTICK_INCLUDE_X_AXIS) {
			// USAGE (X)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x30;
		}

		if (_includeAxisFlags & JOYSTICK_INCLUDE_Y_AXIS) {
			// USAGE (Y)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x31;
		}
		
		if (_includeAxisFlags & JOYSTICK_INCLUDE_Z_AXIS) {
			// USAGE (Z)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x32;
		}
		
		if (_includeAxisFlags & JOYSTICK_INCLUDE_RX_AXIS) {
			// USAGE (Rx)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x33;
		}
		
		if (_includeAxisFlags & JOYSTICK_INCLUDE_RY_AXIS) {
			// USAGE (Ry)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x34;
		}
		
		if (_includeAxisFlags & JOYSTICK_INCLUDE_RZ_AXIS) {
			// USAGE (Rz)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
		}
		
		// INPUT (Data,Var,Abs)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
		
		// END_COLLECTION (Physical)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
		
	} // X, Y, Z, Rx, Ry, and Rz Axis	
	
	if (simulationCount > 0) {
	
		// USAGE_PAGE (Simulation Controls)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
		
		// LOGICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// LOGICAL_MAXIMUM (65535)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x27;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0XFF;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0XFF;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// REPORT_SIZE (16)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

		// REPORT_COUNT (simulationCount)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
		tempHidReportDescriptor[hidReportDescriptorSize++] = simulationCount;

		// COLLECTION (Physical)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		if (_includeSimulatorFlags & JOYSTICK_INCLUDE_RUDDER) {
			// USAGE (Rudder)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xBA;
		}

		if (_includeSimulatorFlags & JOYSTICK_INCLUDE_THROTTLE) {
			// USAGE (Throttle)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xBB;
		}

		if (_includeSimulatorFlags & JOYSTICK_INCLUDE_ACCELERATOR) {
			// USAGE (Accelerator)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xC4;
		}

		if (_includeSimulatorFlags & JOYSTICK_INCLUDE_BRAKE) {
			// USAGE (Brake)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xC5;
		}

		if (_includeSimulatorFlags & JOYSTICK_INCLUDE_STEERING) {
			// USAGE (Steering)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xC8;
		}

		// INPUT (Data,Var,Abs)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
		
		// END_COLLECTION (Physical)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
	
	} // Simulation Controls
	#endif

    // END_COLLECTION
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

	// Create a copy of the HID Report Descriptor template that is just the right size
	uint8_t * const customHidReportDescriptor = new uint8_t[hidReportDescriptorSize];
	memcpy(customHidReportDescriptor, tempHidReportDescriptor, hidReportDescriptorSize);
	
	// Register HID Report Description
	DynamicHID().AppendDescriptor(new DynamicHIDSubDescriptor(customHidReportDescriptor, hidReportDescriptorSize, false));
	
	#ifndef Joystick_DATA_SIZE
		// Calculate HID Report Size
		_hidReportSize = 1 + BUTTONVALUES_SIZE(_buttonCount);
		#ifndef Joystick_DISABLE_HATSWITCH
			_hidReportSize += (_hatSwitchCount > 0) ? 1 : 0;
		#endif
		#ifndef Joystick_DISABLE_AXISES
			_hidReportSize += axisCount * sizeof(uint16_t);
			_hidReportSize += simulationCount * sizeof(uint16_t);
		#endif
		_data = new uint8_t[_hidReportSize];
	#endif
	_data[0] = hidReportId;
	
	#ifndef Joystick_DISABLE_AXISES
	// Initialize Joystick State
	_xAxis = 0;
	_yAxis = 0;
	_zAxis = 0;
	_xAxisRotation = 0;
	_yAxisRotation = 0;
	_zAxisRotation = 0;
	_throttle = 0;
	_rudder = 0;
	_accelerator = 0;
	_brake = 0;
	_steering = 0;
	#endif
	#ifndef Joystick_DISABLE_HATSWITCH
	for (int index = _hatSwitchCount; index --> 0 ;) {
		_hatSwitchValues[index] = JOYSTICK_HATSWITCH_RELEASE;
	}
	#endif

}

void Joystick_::setButton(uint8_t button, uint8_t value)
{
	if (value == 0)
	{
		releaseButton(button);
	}
	else
	{
		pressButton(button);
	}
}
void Joystick_::pressButton(uint8_t button)
{
    if (button >= _buttonCount) return;

    int index = button / 8;
    int bit = button % 8;

	bitSet(_data[index+1], bit);
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::releaseButton(uint8_t button)
{
    if (button >= _buttonCount) return;

    int index = button / 8;
    int bit = button % 8;

    bitClear(_data[index+1], bit);
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}

#ifndef Joystick_DISABLE_AXISES
void Joystick_::setXAxis(int32_t value)
{
	_xAxis = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setYAxis(int32_t value)
{
	_yAxis = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setZAxis(int32_t value)
{
	_zAxis = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}

void Joystick_::setRxAxis(int32_t value)
{
	_xAxisRotation = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setRyAxis(int32_t value)
{
	_yAxisRotation = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setRzAxis(int32_t value)
{
	_zAxisRotation = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}

void Joystick_::setRudder(int32_t value)
{
	_rudder = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setThrottle(int32_t value)
{
	_throttle = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setAccelerator(int32_t value)
{
	_accelerator = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setBrake(int32_t value)
{
	_brake = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
void Joystick_::setSteering(int32_t value)
{
	_steering = value;
  #ifndef Joystick_DISABLE_AUTOSEND
	if (_autoSendState) sendState();
  #endif
}
#endif

#ifndef Joystick_DISABLE_HATSWITCH
	void Joystick_::setHatSwitch(int8_t hatSwitchIndex, int16_t value) {
		if (hatSwitchIndex >= _hatSwitchCount) {
			// pucgenie: wtf, fails silently
	return;
		}
		
		_hatSwitchValues[hatSwitchIndex] = value;
		#ifndef Joystick_DISABLE_AUTOSEND
			if (_autoSendState) sendState();
		#endif
	}
#endif

uint8_t Joystick_::buildAndSet16BitValue(bool includeValue, int32_t value, int32_t valueMinimum, int32_t valueMaximum, int32_t actualMinimum, int32_t actualMaximum, uint8_t dataLocation[]) 
{
	int32_t convertedValue;
	uint8_t highByte;
	uint8_t lowByte;
	int32_t realMinimum = min(valueMinimum, valueMaximum);
	int32_t realMaximum = max(valueMinimum, valueMaximum);

	if (includeValue == false) return 0;

	if (value < realMinimum) {
		value = realMinimum;
	}
	if (value > realMaximum) {
		value = realMaximum;
	}

	if (valueMinimum > valueMaximum) {
		// Values go from a larger number to a smaller number (e.g. 1024 to 0)
		value = realMaximum - value + realMinimum;
	}

	convertedValue = map(value, realMinimum, realMaximum, actualMinimum, actualMaximum);

	highByte = (uint8_t)(convertedValue >> 8);
	lowByte = (uint8_t)(convertedValue & 0x00FF);
	
	dataLocation[0] = lowByte;
	dataLocation[1] = highByte;
	
	return 2;
}

uint8_t Joystick_::buildAndSetAxisValue(bool includeAxis, int32_t axisValue, int32_t axisMinimum, int32_t axisMaximum, uint8_t dataLocation[]) 
{
	return buildAndSet16BitValue(includeAxis, axisValue, axisMinimum, axisMaximum, JOYSTICK_AXIS_MINIMUM, JOYSTICK_AXIS_MAXIMUM, dataLocation);
}

uint8_t Joystick_::buildAndSetSimulationValue(bool includeValue, int32_t value, int32_t valueMinimum, int32_t valueMaximum, uint8_t dataLocation[]) 
{
	return buildAndSet16BitValue(includeValue, value, valueMinimum, valueMaximum, JOYSTICK_SIMULATOR_MINIMUM, JOYSTICK_SIMULATOR_MAXIMUM, dataLocation);
}

int Joystick_::sendState()
{
	int index = BUTTONVALUES_SIZE(_buttonCount);

#ifndef Joystick_DISABLE_HATSWITCH
	// Set Hat Switch Values
	if (_hatSwitchCount > 0) {
		
		// Calculate hat-switch values
		uint8_t convertedHatSwitch[JOYSTICK_HATSWITCH_COUNT_MAXIMUM] {8, 8};
		for (int hatSwitchIndex = _hatSwitchCount; hatSwitchIndex --> 0 ;) {
      convertedHatSwitch[hatSwitchIndex] = (_hatSwitchValues[hatSwitchIndex] < 0)
        ? 8
        : (_hatSwitchValues[hatSwitchIndex] % 360) / 45;
		}

		// Pack hat-switch states into a single byte
		_data[index++] = (convertedHatSwitch[1] << 4) | (B00001111 & convertedHatSwitch[0]);
	
	} // Hat Switches
	#endif

#ifndef Joystick_DISABLE_AXISES
	// Set Axis Values
	index += buildAndSetAxisValue(_includeAxisFlags & JOYSTICK_INCLUDE_X_AXIS, _xAxis, _xAxisMinimum, _xAxisMaximum, &(_data[index]));
	index += buildAndSetAxisValue(_includeAxisFlags & JOYSTICK_INCLUDE_Y_AXIS, _yAxis, _yAxisMinimum, _yAxisMaximum, &(_data[index]));
	index += buildAndSetAxisValue(_includeAxisFlags & JOYSTICK_INCLUDE_Z_AXIS, _zAxis, _zAxisMinimum, _zAxisMaximum, &(_data[index]));
	index += buildAndSetAxisValue(_includeAxisFlags & JOYSTICK_INCLUDE_RX_AXIS, _xAxisRotation, _rxAxisMinimum, _rxAxisMaximum, &(_data[index]));
	index += buildAndSetAxisValue(_includeAxisFlags & JOYSTICK_INCLUDE_RY_AXIS, _yAxisRotation, _ryAxisMinimum, _ryAxisMaximum, &(_data[index]));
	index += buildAndSetAxisValue(_includeAxisFlags & JOYSTICK_INCLUDE_RZ_AXIS, _zAxisRotation, _rzAxisMinimum, _rzAxisMaximum, &(_data[index]));
	
	// Set Simulation Values
	index += buildAndSetSimulationValue(_includeSimulatorFlags & JOYSTICK_INCLUDE_RUDDER, _rudder, _rudderMinimum, _rudderMaximum, &(_data[index]));
	index += buildAndSetSimulationValue(_includeSimulatorFlags & JOYSTICK_INCLUDE_THROTTLE, _throttle, _throttleMinimum, _throttleMaximum, &(_data[index]));
	index += buildAndSetSimulationValue(_includeSimulatorFlags & JOYSTICK_INCLUDE_ACCELERATOR, _accelerator, _acceleratorMinimum, _acceleratorMaximum, &(_data[index]));
	index += buildAndSetSimulationValue(_includeSimulatorFlags & JOYSTICK_INCLUDE_BRAKE, _brake, _brakeMinimum, _brakeMaximum, &(_data[index]));
	index += buildAndSetSimulationValue(_includeSimulatorFlags & JOYSTICK_INCLUDE_STEERING, _steering, _steeringMinimum, _steeringMaximum, &(_data[index]));
#endif

	return DynamicHID().SendReport(_data,
		#ifdef Joystick_DATA_SIZE
			Joystick_DATA_SIZE
		#else
			_hidReportSize
		#endif
	);
}

#endif
