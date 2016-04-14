/*
 * onewireslave.h
 *
 * Created: 9/04/2016 6:08:47 PM
 *  Author: Ben Coughlan
 */ 


#ifndef ONEWIRESLAVE_H_
#define ONEWIRESLAVE_H_

/*
 * Start timers and interrupts for one wire slave
 * 
 * uint8_t[8] bus_id is the 8 byte device id this device should respond to.
 */
void onewireslave_start(uint8_t *bus_id);

/*
 * Register callback for bytes received from master.
 *
 * The callback takes the received byte as it's only argument.
 *
 * If the device wishes to transition into a transmit state, this function
 * returns non-zero.  If it needs to receive more bytes, it returns zero.
 * If it returns non-zero it should also setup the next byte to be written
 * (if it hasn't been already) by calling set_txbyte().
 *
 * The callback is called from interrupt context.
 */
void onewireslave_set_received(uint8_t (*callback)(uint8_t));

/*
 * Register callback for bytes sent to master.
 *
 * The callback is called at the end of each byte written to the master.
 * This callback should set up the next byte to be sent by calling set_txbyte().
 *
 * The callback is called form interrupt context.
 */
 
void onewireslave_set_sent(void (*callback)(void));

/*
 * Set the next byte to send to the master.  This must be done before the final
 * bit of the previous byte on the bus, usually from the received and sent callbacks.
 * 
 * The previous byte will be repeated if a new one hasn't been set.
 * 
 * Byte will be sent once this device is in a transmit state, following a non-zero return
 * from the receive callback.
 */
void onewireslave_set_txbyte(uint8_t data);

#endif /* ONEWIRESLAVE_H_ */