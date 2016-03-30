#include "pio_maua.h"

/**
 * \brief Configure PIO internal pull-up.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 * \param ul_pull_up_enable Indicates if the pin(s) internal pull-up shall be
 * configured.
 */
void _pio_pull_up(	Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_up_enable){
	if(ul_pull_up_enable == 1){
	p_pio->PIO_PUER = ul_mask;	
	}
	if(ul_pull_up_enable == 0){
	p_pio->PIO_PUDR = ul_mask;
	}
}					

/**
 * \brief Configure PIO pin internal pull-down.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 * \param ul_pull_down_enable Indicates if the pin(s) internal pull-down shall
 * be configured.
 */
void _pio_pull_down( Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_down_enable){
	if(ul_pull_down_enable == 1){
		p_pio->PIO_PPDER = ul_mask;
	}
	if(ul_pull_down_enable == 0){
		p_pio->PIO_PPDDR = ul_mask;
	}
}		
					
					
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