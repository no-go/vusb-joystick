/* Minimal V-USB joystick example. Runs on USBasp hardware.
Copyright (C) 2014 Shay Green
Licensed under GPL v2 or later. See License.txt. */

#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "usbdrv/usbdrv.h"

// Report format: Y, X, buttons (up to 8)
static uint8_t report [3]; // current
static uint8_t report_out [3]; // last sent over USB

static void init_joy( void ) {
    DDRB |= 1<<PB0;    // set PB0 to output LED
    
    DDRC  = 0b00000000; // all input
    PORTC = 0b00111111; // all 6 PULLUP

    // 1111 1010 bin: activate pull-ups except on USB lines
    PORTD = 0b11110000;
}

static void read_joy( void ) {
	report [0] = 0;
	report [1] = 0;
	report [2] = 0;
	
	// X
	if ( ! bit_is_set(PINC, PC3) ) report [0] = -127; // left
	if ( ! bit_is_set(PINC, PC2) ) report [0] = +127; // right
	
	// Y
	if ( ! bit_is_set(PINC, PC4) ) report [1] = +127; // up
	if ( ! bit_is_set(PINC, PC5) ) report [1] = -127; // down
	
	// Buttons
	if ( ! bit_is_set(PINC, PC1) ) report [2] |= 0x01;
	if ( ! bit_is_set(PINC, PC0) ) report [2] |= 0x02;
	if ( ! bit_is_set(PIND, PD7) ) report [2] |= 0x04;
}

// X/Y joystick w/ 8-bit readings (-127 to +127), 8 digital buttons
PROGMEM const char usbHidReportDescriptor [USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
	0x05, 0x01,     // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,     // USAGE (Game Pad)
	0xa1, 0x01,     // COLLECTION (Application)
	0x09, 0x01,     //   USAGE (Pointer)
	0xa1, 0x00,     //   COLLECTION (Physical)
	0x09, 0x30,     //     USAGE (X)
	0x09, 0x31,     //     USAGE (Y)
	0x15, 0x81,     //   LOGICAL_MINIMUM (-127)
	0x25, 0x7f,     //   LOGICAL_MAXIMUM (127)
	0x75, 0x08,     //   REPORT_SIZE (8)
	0x95, 0x02,     //   REPORT_COUNT (2)
	0x81, 0x02,     //   INPUT (Data,Var,Abs)
	0xc0,           // END_COLLECTION
	0x05, 0x09,     // USAGE_PAGE (Button)
	0x19, 0x01,     //   USAGE_MINIMUM (Button 1)
	0x29, 0x08,     //   USAGE_MAXIMUM (Button 8)
	0x15, 0x00,     //   LOGICAL_MINIMUM (0)
	0x25, 0x01,     //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,     // REPORT_SIZE (1)
	0x95, 0x08,     // REPORT_COUNT (8)
	0x81, 0x02,     // INPUT (Data,Var,Abs)
	0xc0            // END_COLLECTION
};

uint8_t usbFunctionSetup( uint8_t data [8] ) {
	usbRequest_t const* rq = (usbRequest_t const*) data;

	if ( (rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS )
		return 0;
	
	switch ( rq->bRequest ) {
        case USBRQ_HID_GET_REPORT: // HID joystick only has to handle this
		usbMsgPtr = (usbMsgPtr_t) report_out;
		return sizeof report_out;
	
	//case USBRQ_HID_SET_REPORT: // LEDs on joystick?
	
        default:
            return 0;
	}
}

static void toggle_led(void) {
    PORTB ^= (1 << PB0);
}

int main( void )
{
	usbInit();
	sei();
	
	init_joy();
	
	for ( ;; )
	{
		usbPoll();
		
		// Don't bother reading joy if previous changes haven't gone out yet.
		// Forces delay after changes which serves to debounce controller as well.
		if ( usbInterruptIsReady() )
		{
			read_joy();
			
			// Don't send update unless joystick changed
			if ( memcmp( report_out, report, sizeof report ) )
			{
				memcpy( report_out, report, sizeof report );
				usbSetInterrupt( report_out, sizeof report_out );
				toggle_led();
			}
		}
	}
	
	return 0;
}
