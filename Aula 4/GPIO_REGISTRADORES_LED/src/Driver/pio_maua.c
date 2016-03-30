#include "pio_maua.h"

void _pio_set( Pio *p_pio, const uint32_t ul_mask){
	p_pio->PIO_SODR = ul_mask;
}


void _pio_clear( Pio *p_pio, const uint32_t ul_mask){
	p_pio->PIO_CODR = ul_mask;
}

uint32_t _pio_get_output_data_status(const Pio *p_pio, const uint32_t ul_mask){
	if ((p_pio->PIO_PDSR & ul_mask)  > 0){
		return 1;
	}else{
		return 0;
		}
	}