/************************************************************************
* Timer Counter
*
* Exemplo de utiliza��o do modo RC Compare
* Nesse exemplo o uC entra em modo de stand-by e aguarda por tr�s diferentes
* interrupcoes :
*
* 1 - Timer
*	Muda o estado do led em uma frequencia pr� definida (inicial 4Hz)
*
* 2 - Botao 1
*	Altera a frequencia do led em 2Hz para cima
*	
* 3 - Botao 2
*	Altera a frequencia do led em 2Hz para baixo
*
************************************************************************/

#include "asf.h"

#define PIN_PUSHBUTTON_1_MASK	PIO_PB3
#define PIN_PUSHBUTTON_1_PIO	PIOB
#define PIN_PUSHBUTTON_1_ID		ID_PIOB
#define PIN_PUSHBUTTON_1_TYPE	PIO_INPUT
#define PIN_PUSHBUTTON_1_ATTR	PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_RISE_EDGE

#define PIN_PUSHBUTTON_2_MASK	PIO_PC12
#define PIN_PUSHBUTTON_2_PIO	PIOC
#define PIN_PUSHBUTTON_2_ID		ID_PIOC
#define PIN_PUSHBUTTON_2_TYPE	PIO_INPUT
#define PIN_PUSHBUTTON_2_ATTR	PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_FALL_EDGE

#define PIN_LED_BLUE	19
#define PIN_LED_RED		20
#define PIN_LED_GREEN	20
#define PIN_BUTTON		3
#define PIN_BUTTON3		12

/** 
 * Defini��o dos ports
 * Ports referentes a cada pino
 */
#define PORT_LED_BLUE	PIOA
#define PORT_LED_GREEN	PIOA
#define PORT_LED_RED	PIOC
#define PORT_BUT_2		PIOB
#define PORT_BUT_3		PIOC
#define time			100


/**
 * Define os IDs dos perif�ricos associados aos pinos
 */
#define ID_LED_BLUE		ID_PIOA
#define ID_LED_GREEN	ID_PIOA
#define ID_LED_RED		ID_PIOC
#define ID_BUT_2		ID_PIOB
#define ID_BUT_3		ID_PIOC

/**
 *	Define as masks utilziadas
 */
#define MASK_LED_BLUE	(1u << PIN_LED_BLUE)
#define MASK_LED_GREEN	(1u << PIN_LED_GREEN)
#define MASK_LED_RED	(1u << PIN_LED_RED)
#define MASK_BUT_2		(1u << PIN_BUTTON)
#define MASK_BUT_3		(1u << PIN_BUTTON3)


/** IRQ priority for PIO (The lower the value, the greater the priority) */
#define IRQ_PRIOR_PIO    0

#define Freq_Init_Blink 4	//Hz

/**
 *  Handle Interrupcao botao 1
 */
static void Button1_Handler(uint32_t id, uint32_t mask)
{
	tc_stop(TC0,0);
	tc_write_rc(TC0,0, tc_read_rc(TC0,0)*1.1);
	tc_start(TC0,0);

}

/**
 *  Handle Interrupcao botao 2.
 */
static void Button2_Handler(uint32_t id, uint32_t mask)
{
	tc_stop(TC0,0);
	tc_write_rc(TC0,0,tc_read_rc(TC0,0)*0.9);
	tc_start(TC0,0);

}

/**
 *  Interrupt handler for TC0 interrupt. 
 */
void TC0_Handler(void)
{
	volatile uint32_t ul_dummy;

	/* Clear status bit to acknowledge interrupt */
	ul_dummy = tc_get_status(TC0,0);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */

	if(pio_get(PORT_LED_GREEN, PIO_OUTPUT_0,MASK_LED_GREEN)){
		pio_clear(PIOA, (1 << PIN_LED_GREEN));
	}else
	{
		pio_set(PORT_LED_GREEN,MASK_LED_GREEN);
	}

}

/**
 *  \brief Configure the Pushbuttons
 *
 *  Configure the PIO as inputs and generate corresponding interrupt when
 *  pressed or released.
 */
static void configure_buttons(void)
{
	
	pmc_enable_periph_clk(ID_BUT_2);
	pmc_enable_periph_clk(ID_BUT_3);
	
	/**
	* Configura entrada
	*/ 
	pio_set_input(PORT_BUT_2, MASK_BUT_2, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_input(PORT_BUT_3, MASK_BUT_3, PIO_PULLUP | PIO_DEBOUNCE);
	
	/*
	 * Configura divisor do clock para debounce
	 */
	pio_set_debounce_filter(PORT_BUT_2, MASK_BUT_2, time);
	pio_set_debounce_filter(PORT_BUT_3, MASK_BUT_3, time);
	
	/* 
	*	Configura interrup��o para acontecer em borda de descida.
	*/
	pio_handler_set(PORT_BUT_2, 
					ID_BUT_2,
					MASK_BUT_2,  
					PIO_IT_FALL_EDGE,
					Button2_Handler);
					
	pio_handler_set(PORT_BUT_3,
					ID_BUT_3,
					MASK_BUT_3,
					PIO_IT_FALL_EDGE,
					Button1_Handler);
				
	/*
	*	Ativa interrup��o no perif�rico B porta do bot�o
	*/	
	pio_enable_interrupt(PORT_BUT_2, MASK_BUT_2);
	pio_enable_interrupt(PORT_BUT_3, MASK_BUT_3);
	
	
	/*
	*	Configura a prioridade da interrup��o no pORTB
	*/
	NVIC_SetPriority((IRQn_Type) ID_BUT_2,3 );
	NVIC_SetPriority((IRQn_Type) ID_BUT_3,3 );

	
	/*
	*	Ativa interrup��o no port B
	*/
	NVIC_EnableIRQ((IRQn_Type) ID_BUT_2);
		NVIC_EnableIRQ((IRQn_Type) ID_BUT_3);
}


/**
 *  Configure Timer Counter 0 to generate an interrupt every 250ms.
 */
// [main_tc_configure]
static void configure_tc(void)
{
	/*
	* Aqui atualizamos o clock da cpu que foi configurado em sysclk init
	*
	* O valor atual est'a em : 120_000_000 Hz (120Mhz)
	*/
	uint32_t ul_sysclk = sysclk_get_cpu_hz();
	
	/*
	*	Ativa o clock do perif�rico TC 0
	* 
	*/
	pmc_enable_periph_clk(ID_TC0);

	/*
	* Configura TC para operar no modo de compara��o e trigger RC
	* devemos nos preocupar com o clock em que o TC ir� operar !
	*
	* Cada TC possui 3 canais, escolher um para utilizar.
	*
	* Configura��es de modo de opera��o :
	*	#define TC_CMR_ABETRG (0x1u << 10) : TIOA or TIOB External Trigger Selection 
	*	#define TC_CMR_CPCTRG (0x1u << 14) : RC Compare Trigger Enable 
	*	#define TC_CMR_WAVE   (0x1u << 15) : Waveform Mode 
	*
	* Configura��es de clock :
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK1 : Clock selected: internal MCK/2 clock signal 
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK2 : Clock selected: internal MCK/8 clock signal 
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK3 : Clock selected: internal MCK/32 clock signal 
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK4 : Clock selected: internal MCK/128 clock signal
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK5 : Clock selected: internal SLCK clock signal 
	*
	*	MCK		= 120_000_000
	*	SLCK	= 32_768		(rtc)
	*
	* Uma op��o para achar o valor do divisor � utilizar a funcao
	* tc_find_mck_divisor()
	*/
	tc_init(TC0, 0, TC_CMR_CPCTRG | TC_CMR_TCCLKS_TIMER_CLOCK5);
	
	/*
	* Aqui devemos configurar o valor do RC que vai trigar o reinicio da contagem
	* devemos levar em conta a frequ�ncia que queremos que o TC gere as interrup��es
	* e tambem a frequencia com que o TC est� operando.
	*
	* Devemos configurar o RC para o mesmo canal escolhido anteriormente.
	*	
	*   ^ 
	*	|	Contador (incrementado na frequencia escolhida do clock)
	*   |
	*	|	 	Interrupcao	
	*	|------#----------- RC
	*	|	  /
	*	|   /
	*	| /
	*	|-----------------> t
	*
	*
	*/
	tc_write_rc(TC0,0, 32768/4);
	
	
	/*
	* Devemos configurar o NVIC para receber interrup��es do TC 
	*/
	NVIC_EnableIRQ(ID_TC0);
	
	/*
	* Op��es poss�veis geradoras de interrup��o :
	* 
	* Essas configura��es est�o definidas no head : tc.h 
	*
	*	#define TC_IER_COVFS (0x1u << 0)	Counter Overflow 
	*	#define TC_IER_LOVRS (0x1u << 1)	Load Overrun 
	*	#define TC_IER_CPAS  (0x1u << 2)	RA Compare 
	*	#define TC_IER_CPBS  (0x1u << 3)	RB Compare 
	*	1
	*	#define TC_IER_LDRAS (0x1u << 5)	RA Loading 
	*	#define TC_IER_LDRBS (0x1u << 6)	RB Loading 
	*	#define TC_IER_ETRGS (0x1u << 7)	External Trigger 
	*/
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
	
	tc_start(TC0, 0);
}


/************************************************************************/
/* Main Code	                                                        */
/************************************************************************/
int main(void)
{
	/* Initialize the SAM system */
	sysclk_init();

	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;

	/** Configura o timer */
	configure_tc();
	
	/* Configura os bot�es */
	configure_buttons();
	
	/* configura leds */
	/**
	* Ativa clock nos perif�ricos
	*/
	pmc_enable_periph_clk(ID_LED_BLUE);
	pmc_enable_periph_clk(ID_LED_GREEN);		// Redundante mas n�o tem problema !
	pmc_enable_periph_clk(ID_LED_RED);

	pio_set_output(PORT_LED_BLUE  , MASK_LED_BLUE	,1,0,0);
	pio_set_output(PORT_LED_GREEN , MASK_LED_GREEN  ,1,0,0);
	pio_set_output(PORT_LED_RED	  , MASK_LED_RED	,1,0,0);
	
	
	
	
	while (1) {
		
		/* Entra em modo sleep */
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
		
	}
}
