#include "pio_maua.h"

/**
 * \brief Configure one or more pin(s) of a PIO controller as outputs, with
 * the given default value. 
 * 
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure.
 * \param ul_default_level Default level on the pin(s).
 * \param ul_pull_up_enable Indicates if the pin shall have its pull-up
 * activated.
 */
void _pio_set_output(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_default_level, const uint32_t ul_pull_up_enable){
	p_pio->PIO_WPMR = 0;
	p_pio->PIO_PER = ul_mask;
	p_pio->PIO_OER = ul_mask;
	if(ul_default_level){
		_pio_set(p_pio,ul_mask);
	}else{
		_pio_clear(p_pio,ul_mask);
	}
	_pio_pull_up(p_pio,ul_mask,ul_pull_up_enable);
}


/**
 * \brief Configure one or more pin(s) or a PIO controller as inputs.
 * Optionally, the corresponding internal pull-up(s) and glitch filter(s) can
 * be enabled.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure as input(s).
 * \param ul_attribute PIO attribute(s).
 */
void _pio_set_input( 	Pio *p_pio, 
                    	const uint32_t ul_mask,
            	   	const uint32_t ul_attribute);
					   

void _pio_pull_up(	Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_up_enable){
	if(ul_pull_up_enable == 1){
		p_pio->PIO_PUER = ul_mask;	
	}
	if(ul_pull_up_enable == 0){
		p_pio->PIO_PUDR = ul_mask;
	}
}					

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