#include "pio_maua.h"

void _pio_set( Pio *p_pio, const uint32_t ul_mask){
	p_pio->PIO_SODR = ul_mask;
}


void _pio_clear( Pio *p_pio, const uint32_t ul_mask){
	p_pio->PIO_CODR = ul_mask;
}