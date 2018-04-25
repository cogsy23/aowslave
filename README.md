# Another OneWire Slave

An implementation of the OneWire protocol for custom built slave devices.  There are already a couple of these around online
but most seem tailored to replicating existing OneWire devices or are tied in with the Aruino runtime.  I wanted something
that provided the basics of communicating over OneWire from the slave.

This implementation easily fits within 2k of program memory, and I've successfully tested 5 devices on the same bus.

* Completely interrupt driven and no use of delays.
* Uses Timer0, any PCINT capable pin and their assosciated interrupt vectors.
* Support for SearchROM, MatchROM, SkipROM commands.

## Supported Devices

* ATtiny85

I've only tested on devices that I've had a use for, but if you manage to get it running on another or
would like help porting to another AVR please let me know.

## Example

This example will wait until this device is selected (either explicitely or with a skipROM command) and then
recieves a "read" command (0xBB).  It then outputs a counter continuously as long as the master provides READ slots.

```c
#include "aowslave/onewireslave.h"
uint8_t bus_id[8] = {0xFD, 0x00, 0x95, 0xD7, 0xF9, 0xF7, 0x6F, 0xC0}; //CRC, Family, Serial LSB..MSB
uint8_t data_byte = 0x00;

uint8_t byte_received(uint8_t data) {
  switch(data) {
    case 0xBB:
      data_byte = 0x00; //reset the counter
      onewireslave_set_txbyte(data_byte);
      return 1; //when you want to start writing bytes
    default:
      return 0; //when you still want to receive bytes
  }
}

void byte_sent() {
  //increment the counter and start transmitting the new value.
  onewireslave_set_txbyte(data_byte++);
  //If we don't call set_txbyte(), we will just repeat the last byte
}

int main(void)
{
  //setup the callbacks
  onewireslave_set_received(byte_received);
  onewireslave_set_sent(byte_sent);
  //start the onewire bus
  onewireslave_start(bus_id);
	
  while(1) {}
}
```

## Notes
* CRCs for device IDs can be calculated with http://www.datastat.com/sysadminjournal/maximcrc.cgi
* Fast recovery pulses push the limits of the ISR timing capabilities.  The examples I've seen say recovery pulses can be
as little as 1uS long.  With a typical 4k7 pullup the bus capacitance will often struggle with such short pulses any way.
My testing has been comfortable with 8uS pulses.
* Long ISRs at the wrong time will caused missed edges.  This implementation should recover on the next reset pulse.  Typical
slave devices can be driven completely by the sent/receive callbacks which, while still being in ISR context, have more
relaxed timing than other asynchronous interrupts.

## TODO List
* CRC Support.
* Select which PIN to use when starting.
* Support different clock speeds.
* Allow returning to WRITE slots after READ slots.
