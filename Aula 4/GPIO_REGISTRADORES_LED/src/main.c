/**
 * IMT - Rafael Corsi
 * 
 * PIO - 07
 *  Configura o PIO do SAM4S (Banco A, pino 19) para operar em
 *  modo de output. Esse pino está conectado a um LED, que em 
 *  lógica alta apaga e lógica baixa acende.
*/

#include <asf.h>
#include "Driver/pmc_maua.h"
#include "Driver/pio_maua.h"

/*
 * Prototypes
 */

/** 
 * Definição dos pinos
 * Pinos do uC referente aos LEDS.
 *
 * O número referente ao pino (PIOAxx), refere-se ao
 * bit que deve ser configurado no registrador para alterar
 * o estado desse bit específico.
 *
 * exe : O pino PIOA_19 é configurado nos registradores pelo bit
 * 19. O registrador PIO_SODR configura se os pinos serão nível alto.
 * Nesse caso o bit 19 desse registrador é referente ao pino PIOA_19
 *
 * ----------------------------------
 * | BIT 19  | BIT 18  | ... |BIT 0 |
 * ----------------------------------
 * | PIOA_19 | PIOA_18 | ... |PIOA_0|
 * ----------------------------------
 */

#define TEMPO 100

#define PIN_LED_BLUE 19
#define PIN_LED_GREEN 20
#define PIN_LED_RED 20

#define PIN_PUSHBUTTON_1 3

#define MASK_LED_BLUE (1 << PIN_LED_BLUE)

#define BOTAO_1_APERTADO 0
#define BOTAO_1_ABERTO 1


/** 
 * Definição dos ports
 * Ports referentes a cada pino
 */
#define PORT_LED_BLUE PIOA
#define PORT_LED_GREEN PIOA
#define PORT_LED_RED PIOC
#define PORT_PUSHBUTTON_1 PIOB




/**
 * Main function
 * 1. configura o clock do sistema
 * 2. desabilita wathdog
 * 3. ativa o clock para o PIOA
 * 4. ativa o controle do pino ao PIO
 * 5. desabilita a proteção contra gravações do registradores
 * 6. ativa a o pino como modo output
 * 7. coloca o HIGH no pino
 */

int main (void)
{

	/**
	* Inicializando o clock do uP
	*/
	sysclk_init();
	
	/** 
	*  Desabilitando o WathDog do uP
	*/
	WDT->WDT_MR = WDT_MR_WDDIS;
		
	// 29.17.4 PMC Peripheral Clock Enable Register 0
	// 1: Enables the corresponding peripheral clock.
	// ID_PIOA = 11 - TAB 11-1
	_pmc_enable_clock_periferico(ID_PIOA);
	_pmc_enable_clock_periferico(ID_PIOB);
	_pmc_enable_clock_periferico(ID_PIOC);
	
	// 31.6.46 PIO Write Protection Mode Register
	// 0: Disables the write protection if WPKEY corresponds to 0x50494F (PIO in ASCII).
	PIOA->PIO_WPMR = 0;
	PIOC->PIO_WPMR = 0;
	PIOB->PIO_WPMR = 0;

	 //31.6.1 PIO Enable Register
	// 1: Enables the PIO to control the corresponding pin (disables peripheral control of the pin).	
	PIOA->PIO_PER = (1 << PIN_LED_BLUE );
	PIOA->PIO_PER = (1 << PIN_LED_GREEN );
	PIOC->PIO_PER = (1 << PIN_LED_RED );
	PIOB->PIO_PER = (1 << PIN_PUSHBUTTON_1);

	
	// 31.5.6: PIO disable buffer
	PIOB->PIO_ODR = (1 << PIN_PUSHBUTTON_1);

	
	//31.5.6: Pull up enable
	PIOB->PIO_PUER = (1 << PIN_PUSHBUTTON_1);	
	
	//PIOB->PIO_IFDR = (1 << PIN_PUSHBUTTON_1);
	PIOB->PIO_IFSCER = (1 << PIN_PUSHBUTTON_1);
	

	// 31.6.4 PIO Output Enable Register
	// 1: Enables the output on the I/O line.
	PIOA->PIO_OER =  (1 << PIN_LED_BLUE );
	PIOA->PIO_OER =  (1 << PIN_LED_GREEN );
	PIOC->PIO_OER =  (1 << PIN_LED_RED );

	// 31.6.10 PIO Set Output Data Register
	// 1: Sets the data to be driven on the I/O line.
	
	
	/* Biblioteca dos LED's
	
	//Manual do SAM4S-EK2 sessão 4.3.15 
	*Para acender os LED's verde(PA19) e azul(PA20), é necessário um baixo nivel.
	*Para acender o LED vermelho(PC20), é necessário um alto nível
	
	//Apagar os LED's
	PIOA->PIO_SODR = (1 << PIN_LED_BLUE );
	PIOA->PIO_SODR = (1 << PIN_LED_GREEN );
	PIOC->PIO_CODR = (1 << PIN_LED_RED );
	
	//Acender os LED's
	PIOA->PIO_CODR = (1 << PIN_LED_BLUE );
	PIOA->PIO_CODR = (1 << PIN_LED_GREEN );
	PIOC->PIO_SODR = (1 << PIN_LED_RED );
	
	
	*/

	/**
	*	Loop infinito
	*/
		while(1){
			
			/* WHile para verificar se o botão USRPB1 foi pressionado */
			
	//		if ( ((PIOB->PIO_PDSR >> PIN_PUSHBUTTON_1) & 1)  == 0){
			if (_pio_get_output_data_status(PIOB,1 << PIN_PUSHBUTTON_1 ) == BOTAO_1_APERTADO){
				_pio_clear(PIOA, MASK_LED_BLUE);
				_pio_set(PIOC, 1 << PIN_LED_RED );			
				delay_ms(200);
			}
			else {
				_pio_set(PIOA, MASK_LED_BLUE);
				_pio_clear(PIOC,1 << PIN_LED_RED);
				delay_ms(200);
			} 
						
			

            /*
             * Utilize a função delay_ms para fazer o led piscar na frequência
             * escolhida por você.
             */
            //delay_ms();
			
			//LED's dançando
			
			/* 
			
			for(int i = 0; i<4;i++){
			
			PIOC->PIO_SODR = (1 << PIN_LED_RED );
			PIOA->PIO_SODR = (1 << PIN_LED_BLUE );
			PIOA->PIO_SODR = (1 << PIN_LED_GREEN );
			delay_ms(TEMPO);
			
			PIOC->PIO_CODR = (1 << PIN_LED_RED );
			PIOA->PIO_CODR = (1 << PIN_LED_BLUE );
			delay_ms(TEMPO);
			
			PIOA->PIO_SODR = (1 << PIN_LED_BLUE );
			PIOA->PIO_CODR = (1 << PIN_LED_GREEN );
			delay_ms(TEMPO);
			
			}
			
			for(int i = 0; i<2;i++){
				PIOA->PIO_CODR = (1 << PIN_LED_BLUE );
				PIOA->PIO_CODR = (1 << PIN_LED_GREEN );
				PIOC->PIO_SODR = (1 << PIN_LED_RED );
				delay_ms(TEMPO*2);
				
				PIOA->PIO_SODR = (1 << PIN_LED_BLUE );
				PIOA->PIO_SODR = (1 << PIN_LED_GREEN );
				PIOC->PIO_CODR = (1 << PIN_LED_RED );
				delay_ms(TEMPO*2);
			}
			*/
	
	}
}



