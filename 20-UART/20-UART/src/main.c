/**
 *	20-UART 
 * Prof. Rafael Corsi
 *
 *    (e.g., HyperTerminal on Microsoft Windows) with these settings:
 *   - 115200 bauds
 *   - 8 bits of data
 *   - No parity
 *   - 1 stop bit
 *   - No flow control
 */

#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

/************************************************************************/
/* Configurações                                                        */
/************************************************************************/

#define STRING_EOL    "\r"
#define STRING_VERSAO "-- "BOARD_NAME" --\r\n" \
					  "-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

#define CONF_UART_BAUDRATE 115200		
#define CONF_UART          CONSOLE_UART

/** 
 * LEDs
 */ 
#define PIN_LED_BLUE	19
#define PORT_LED_BLUE	PIOA
#define ID_LED_BLUE		ID_PIOA
#define MASK_LED_BLUE	(1u << PIN_LED_BLUE)

#define PIN_LED_GREEN	20
#define PORT_LED_GREEN	PIOA
#define ID_LED_GREEN	ID_PIOA
#define MASK_LED_GREEN	(1u << PIN_LED_GREEN)

#define PIN_LED_RED		20
#define PORT_LED_RED	PIOC
#define ID_LED_RED		ID_PIOC
#define MASK_LED_RED	(1u << PIN_LED_RED)

/* Variaveis Globais */
int Freq = 1;
int R = 0;
int B = 0;
int G = 0;

/************************************************************************/
/* Configura UART                                                       */
/************************************************************************/
void config_uart(void){
	
	/* configura pinos */
	gpio_configure_group(PINS_UART0_PIO, PINS_UART0, PINS_UART0_FLAGS);
	
	/* ativa clock */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	
	/* Configuração UART */
	const usart_serial_options_t uart_serial_options = {
		.baudrate   = CONF_UART_BAUDRATE,
		.paritytype = UART_MR_PAR_NO,
		.stopbits   = 0
	};
	
	stdio_serial_init((Usart *)CONF_UART, &uart_serial_options);
}

/* Confugra TC */

static void configure_tc(void)
{
	/*
	* Aqui atualizamos o clock da cpu que foi configurado em sysclk init
	*
	* O valor atual est'a em : 120_000_000 Hz (120Mhz)
	*/
	uint32_t ul_sysclk = sysclk_get_cpu_hz();
	
	/*
	*	Ativa o clock do periférico TC 0
	* 
	*/
	pmc_enable_periph_clk(ID_TC0);

	/*
	* Configura TC para operar no modo de comparação e trigger RC
	* devemos nos preocupar com o clock em que o TC irá operar !
	*
	* Cada TC possui 3 canais, escolher um para utilizar.
	*
	* Configurações de modo de operação :
	*	#define TC_CMR_ABETRG (0x1u << 10) : TIOA or TIOB External Trigger Selection 
	*	#define TC_CMR_CPCTRG (0x1u << 14) : RC Compare Trigger Enable 
	*	#define TC_CMR_WAVE   (0x1u << 15) : Waveform Mode 
	*
	* Configurações de clock :
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK1 : Clock selected: internal MCK/2 clock signal 
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK2 : Clock selected: internal MCK/8 clock signal 
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK3 : Clock selected: internal MCK/32 clock signal 
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK4 : Clock selected: internal MCK/128 clock signal
	*	#define  TC_CMR_TCCLKS_TIMER_CLOCK5 : Clock selected: internal SLCK clock signal 
	*
	*	MCK		= 120_000_000
	*	SLCK	= 32_768		(rtc)
	*
	* Uma opção para achar o valor do divisor é utilizar a funcao
	* tc_find_mck_divisor()
	*/
	tc_init(TC0, 0, TC_CMR_CPCTRG | TC_CMR_TCCLKS_TIMER_CLOCK5);
	
	/*
	* Aqui devemos configurar o valor do RC que vai trigar o reinicio da contagem
	* devemos levar em conta a frequência que queremos que o TC gere as interrupções
	* e tambem a frequencia com que o TC está operando.
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
	tc_write_rc(TC0,0, 32768/Freq);
	
	
	/*
	* Devemos configurar o NVIC para receber interrupções do TC 
	*/
	NVIC_EnableIRQ(ID_TC0);
	
	/*
	* Opções possíveis geradoras de interrupção :
	* 
	* Essas configurações estão definidas no head : tc.h 
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

void TC0_Handler(void)
{
	volatile uint32_t ul_dummy;

	/* Clear status bit to acknowledge interrupt */
	ul_dummy = tc_get_status(TC0,0);
	
	if (Freq != 0){
		if (R == 1){
			if (pio_get_output_data_status(PORT_LED_RED,MASK_LED_RED))
				pio_clear(PORT_LED_RED, MASK_LED_RED);
			else
				pio_set(PORT_LED_RED, MASK_LED_RED);
		}
		if (G == 1){
			if (pio_get_output_data_status(PORT_LED_GREEN,MASK_LED_GREEN))
			pio_clear(PORT_LED_GREEN, MASK_LED_GREEN);
			else
			pio_set(PORT_LED_GREEN, MASK_LED_GREEN);
			
		}
		if (B == 1){
			if (pio_get_output_data_status(PORT_LED_BLUE,MASK_LED_BLUE))
			pio_clear(PORT_LED_BLUE, MASK_LED_BLUE);
			else
			pio_set(PORT_LED_BLUE, MASK_LED_BLUE);			
		}
	}
}

/************************************************************************/
/* Display Menu                                                         */
/************************************************************************/
static void display_menu(void)
{
	puts(" 1 : exibe novamente esse menu \n\r"
		 " 2 : Ativa o LED AZUL \n\r"
		 " 3 : Desliga o LED AZUL \n\r "
		 " 4 : Ativa o LED VERDE \n\r"
		 " 5 : Desliga o LED VERDE \n\r "
		 " 6 : Ativa o LED VERMELHO \n\r"
		 " 7 : Desliga o LED VERMELHO \n\r ");
}


void interpretarCodigo(char s[],int i){
	if (s[0] != 'R' && s[0] != 'G' && s[0] != 'B'){
		puts("Led não definido \n\r");
		return;
	}
	if (s[1] != '0' && s[1] != '1'){
		puts("Acender ou Apagar não definido \n\r");
		return;
	}
	switch (s[0]){
		case 'R':
		if (s[1] == '0'){
			pio_clear(PORT_LED_RED, MASK_LED_RED);
			puts("Led VERMELHO OFF \n\r");
			R = 0;
		}else
		if (s[1] == '1'){
			pio_set(PORT_LED_RED, MASK_LED_RED);
			puts("Led VERMELHO ON \n\r");
			R = 1;
		}else
		printf("Opcao nao definida: %s \n\r", s[1]);
		
		break;
		case 'G':
		if (s[1] == '0'){
			pio_set(PORT_LED_GREEN, MASK_LED_GREEN);
			puts("Led VERDE OFF \n\r");
			G = 0;
		}else
		if (s[1] == '1'){
			pio_clear(PORT_LED_GREEN, MASK_LED_GREEN);
			puts("Led VERDE ON \n\r");
			G = 1;
		}else
		printf("Opcao nao definida: %s \n\r", s[1]);
		break;
		case 'B':
		if (s[1] == '0'){
			pio_set(PORT_LED_BLUE, MASK_LED_BLUE);
			puts("Led AZUL OFF \n\r");
			B = 0;
		}else
		if (s[1] == '1'){
			pio_clear(PORT_LED_BLUE, MASK_LED_BLUE);
			puts("Led AZUL ON \n\r");
			B = 1;
		}else
		printf("Opcao nao definida: %s \n\r", s[1]);
		break;
		default:
		printf("Opcao nao definida: %s \n\r", s[0]);
	}
}


/************************************************************************/
/* Main                                                                 */
/************************************************************************/
int main(void)
{
	uint8_t uc_key = '0';

	/* Initialize the system */
	sysclk_init();
	board_init();
	
	/** Configura o timer */
	configure_tc();

	/* Configure LED 1 */
	pmc_enable_periph_clk(ID_LED_BLUE);
	pio_set_output(PORT_LED_BLUE  , MASK_LED_BLUE	,1,0,0);
	
	pmc_enable_periph_clk(ID_LED_GREEN);
	pio_set_output(PORT_LED_GREEN , MASK_LED_GREEN  ,1,0,0);
	
	pmc_enable_periph_clk(ID_LED_RED);
	pio_set_output(PORT_LED_RED	  , MASK_LED_RED	,1,0,0);

	/* Initialize debug console */
	config_uart();
	
	/* frase de boas vindas */
	puts(" ---------------------------- \n\r"
	 	 " Bem vindo terraquio !		\n\r"
		 " ---------------------------- \n\r");
		 
	/* display main menu */
	//display_menu();
	
	/** Enable peripheral clock */
	pmc_enable_periph_clk(ID_SMC);
	
	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	/* desativa LED VERMELHO */
	pio_clear(PORT_LED_RED, MASK_LED_RED);
	
	int i = 0;
	char s[8];

	while (1) {
		uc_key = '0';
		while (uc_key != ';'){
		usart_serial_getchar((Usart *)CONSOLE_UART, &uc_key);
			s[i++] = uc_key;
		}	
			interpretarCodigo(s,i);
/*		switch (uc_key) {
			case '1':
				display_menu();
				break;
			case '2':
				pio_clear(PORT_LED_BLUE, MASK_LED_BLUE);
				puts("Led AZUL ON \n\r");
				break;
			case '3' :
				pio_set(PORT_LED_BLUE, MASK_LED_BLUE);
				puts("Led AZUL OFF \n\r");
				break;
			case '4':
				pio_clear(PORT_LED_GREEN, MASK_LED_GREEN);
				puts("Led VERDE ON \n\r");
				break;
			case '5' :
				pio_set(PORT_LED_GREEN, MASK_LED_GREEN);
				puts("Led VERDE OFF \n\r");
				break;
			case '6':
				pio_set(PORT_LED_RED, MASK_LED_RED);
				puts("Led VERMELHO ON \n\r");
				break;
			case '7' :
				pio_clear(PORT_LED_RED, MASK_LED_RED);
				puts("Led VERMELHO OFF \n\r");
				break;
			default:
				printf("Opcao nao definida: %d \n\r", uc_key);
		}	*/
		sprintf(s,"");
		i=0;
	}
}
