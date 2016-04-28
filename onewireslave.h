/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Ben Coughlan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
