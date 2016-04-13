/*
 * onewireslave.h
 *
 * Created: 9/04/2016 6:08:47 PM
 *  Author: Ben Coughlan
 */ 


#ifndef ONEWIRESLAVE_H_
#define ONEWIRESLAVE_H_

void onewireslave_start();
void onewireslave_set_received(uint8_t (*callback)(uint8_t));
void onewireslave_set_sent(void (*callback)(void));
void onewireslave_set_txbyte(uint8_t data);

#endif /* ONEWIRESLAVE_H_ */