/*
 * onewireslave.c
 *
 * Created: 9/04/2016 6:09:03 PM
 *  Author: Ben Coughlan
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define CMD_SEARCH_ROM 0xF0
#define CMD_MATCH_ROM 0x55
#define CMD_SKIP_ROM 0xCC
#define CMD_READ_ROM 0x33
#define CMD_ALARM_SEARCH 0xEC

typedef enum {
	WAIT_RESET,	//When this device was deselected and is waiting for a reset pulse
	START_PRES,	//This device is responding to a reset pulse with a presence pulse
	END_PRES,
	WRITE,	//The master is writing to the bus
	READ //The master is reading from the bus
} state_t;

volatile state_t state;	//the current state of this onewire device
volatile uint8_t bit_count;
volatile uint8_t current_byte;
volatile uint8_t ROM_command;
volatile uint8_t rom_matched;
volatile uint8_t id_index;
volatile uint8_t read_val;
volatile uint8_t tx_byte;
volatile uint8_t *id;
	
uint8_t (*_callback_byte_received)(uint8_t byte);	//called at the end of each byte received from master after ROM commands
void (*_callback_byte_sent)(void);	//called at the end of each byte sent to master

#define set_timer(us_inc) {OCR0A=us_inc; TCCR0B=(1<<CS01)|(1<<CS00);}
#define reset_timer() {	GTCCR|=(1<<PSR0); TCNT0=0x0; TIFR=(1<<OCF0A)|(1<<OCF0B); TCCR0B=(1<<CS01)|(1<<CS00);}
#define stop_timer() TCCR0B = 0
#define pull_down() DDRB |= (1<<DDB1)
#define release() DDRB &= ~(1<<DDB1)
#define pin_high() (PINB & (1<<PINB1)) >> PINB1

void onewireslave_set_received(uint8_t (*callback)(uint8_t)) {
	_callback_byte_received = callback;
}

void onewireslave_set_sent(void (*callback)(void)) {
	_callback_byte_sent = callback;
}

void onewireslave_set_txbyte(uint8_t data) {
	tx_byte = data;
}

/**
 * these function form a sort of hierachy with process_bit() as the root, and the branching
 * determined by the values of ROM_command and rom_matched.  These are reset to 0 on every
 * reset pulse, then each bit received from the master is sent through process_bit() to 
 * whichever function is currently being executed.
 *
 * This will process all ROM commands and then forward bytes on to application code to deal
 * with function commands and other data transfer.
 */
 
void do_match_rom(uint8_t val) {
	//assume state == WRITE
	uint8_t id_bit_val = (id[id_index] >> bit_count++) & 0x01; //LSB first
	if(val != id_bit_val) {
		//deselected
		state = WAIT_RESET;
	} else if(bit_count == 8) {
		if(id_index == 0) {
			rom_matched = 0x01;
		}
		bit_count = 0;
		id_index--;
	}
}

void do_search_rom(uint8_t val) {
	if(state == WRITE) {
		if(!val == !(read_val & 0x01)) {
			state = READ;
			if(bit_count == 8) {
				if(id_index == 0) {
					rom_matched = 0x01;
				}
				bit_count = 0;
				id_index--;
			}
			read_val = (id[id_index] >> bit_count) & 0x01;
			
		} else {
			//we were deselected
			state = WAIT_RESET;
		}
	} else {
		read_val = ~read_val;
		if(read_val == ((id[id_index] >> bit_count) & 0x01)) {
			state = WRITE;
			bit_count++;
		}
	}
}

void do_read_rom() {
	read_val = ((id[id_index] >> bit_count) & 0x01);
	if(++bit_count == 8) {
		if(id_index == 0) {
			rom_matched = 0x01;
		}
		bit_count = 0;
		id_index--;
	}
}

//TODO expose alarm condition API
volatile uint8_t alarm_condition = 0;
void do_alarm_search(uint8_t val) {
	if(alarm_condition) {
		do_search_rom(val);
	} else {
		state = WAIT_RESET;
	}
}

void get_rom_command(uint8_t val) {
	//assume we're in WRITE
	current_byte = (current_byte >> 1) | (val!=0 ? 0x80 : 0x00);
	if(++bit_count == 8) {
		ROM_command = current_byte;
		id_index = 7;
		bit_count = 0;
		//decide if we need to start reading/writing
		switch (current_byte) {
			case CMD_SEARCH_ROM: do_search_rom(0); break;
			case CMD_ALARM_SEARCH: do_alarm_search(0); break;
			case CMD_SKIP_ROM: rom_matched = 0x01; break;
			case CMD_READ_ROM:
				state = READ;
				do_read_rom();
				break;
			default: break;
		}
	}
}

/**
 * If we're still selected then start processing bits as higher levels functions.
 * This will start in WRITE, and send a byte at a time to user callback.  If the callback
 * returns non-zero, then we switch to READ and start transmitting tx_byte.
 */
void do_function_bytes(uint8_t val) {
	if(state == WRITE) {
		current_byte = (current_byte >> 1) | (val!=0 ? 0x80 : 0x00);
		if(++bit_count == 8) {
			bit_count = 0;
			if (_callback_byte_received) {
				if(_callback_byte_received(current_byte)) {
					read_val = tx_byte & 0x01;
					state = READ;
				}
			}
		}
	} else {
		if(++bit_count == 8) {
			bit_count = 0;
			if (_callback_byte_sent) {
				_callback_byte_sent();
			}
		}
		
		read_val = (tx_byte >> bit_count) & 0x01;
	}
}

void process_bit(uint8_t val) {
	if(rom_matched) {
		do_function_bytes(val);
	} else {
		switch(ROM_command) {
			case 0x00:           get_rom_command(val); break;
			case CMD_SEARCH_ROM: do_search_rom(val); break;
			case CMD_ALARM_SEARCH: do_alarm_search(val); break;
			case CMD_MATCH_ROM: do_match_rom(val); break;
			case CMD_READ_ROM: do_read_rom(val); break;
			case CMD_SKIP_ROM:
			default:
				state = WAIT_RESET;	//unknown ROM command
				break;
		}
	}
}

ISR(__vector_PCINT0_FALLING) {
	reset_timer();
	switch(state) {
		case START_PRES:
			set_timer(15); //presence pulse duration
			state = END_PRES;
			break;
		case WRITE:
			set_timer(2);
			break;
		case READ:
			set_timer(2);
			if(read_val & 0x01) { //send the LSB of read_val
				release();
			} else {
				pull_down();
			}
			break;
		default: set_timer(255);
	}
}

ISR(__vector_PCINT0_RISING) {
	switch(state) {
		case START_PRES:
			GTCCR |= (1<<PSR0);
			TCNT0 = 0x0;
			set_timer(2); break;
		case END_PRES: state = WRITE; stop_timer();
		default: break;
	}
}

//this samples the pin and conditionally jumps to the rising or falling ISR
ISR(PCINT0_vect, ISR_NAKED) {
	asm (
		"SBIS %[port], 1\t\n"
		"RJMP __vector_PCINT0_FALLING\t\n"
		"RJMP __vector_PCINT0_RISING\t\n"
		:: [port] "I"(_SFR_IO_ADDR(PINB)) :
	);
}

//COMPA used for reading/writing pin timings
ISR(TIMER0_COMPA_vect) {
	volatile uint8_t pin_val = pin_high();

	switch(state) {
		case START_PRES: pull_down(); break;
		case END_PRES: release(); break;
		case WRITE:
		case READ:
			release();
			sei();
			process_bit(pin_val);
		default: break;
	}
}

//COMPB used to detect reset pulse
//this also fires once when the bus is left idle for too long
ISR(TIMER0_COMPB_vect) {
	if(pin_high() == 0) {
		bit_count = 0;
		ROM_command = 0;
		id_index = 0;
		state = START_PRES;
		read_val = 0;
		rom_matched = 0;
	}
	stop_timer();
	release();
}

void onewireslave_start(uint8_t *bus_id) {
	id = bus_id;
	state = WAIT_RESET;
	
	OCR0B = 58;	//480uS reset timer (actually 468uS)
	
	//setup interrupt
	GIMSK |= 1 << PCIE; //enable
	PCMSK |= 1 << PCINT1; //mask
	GIFR |= 1 << PCIF; //clear
	TIMSK |= (1<<OCIE0A) | (1<<OCIE0B);
	sei();
}