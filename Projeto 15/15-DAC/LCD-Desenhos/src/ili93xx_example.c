#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"
#include "smc.h"
#include "img.h"
#include <math.h>

/** Chip select number to be set */
#define ILI93XX_LCD_CS      1

struct ili93xx_opt_t g_ili93xx_display_opt;

//! DAC channel used for test
#define DACC_CHANNEL        1 // (PB14)

//! DAC register base for test
#define DACC_BASE           DACC
//! DAC ID for test
#define DACC_ID             ID_DACC

/** Analog control value */
#define DACC_ANALOG_CONTROL (DACC_ACR_IBCTLCH0(0x02) \
| DACC_ACR_IBCTLCH1(0x02) \
| DACC_ACR_IBCTLDACCORE(0x01))

/** The analog voltage reference **/
#define VADREF    (float) 3.3
/** The maximal data value (no sign) */
#define MAX_DIGITAL   ((1 << DACC_RESOLUTION) - 1)
/** The maximal (peak-peak) amplitude value */
#define MAX_AMPLITUDE (float) 5/6
/** The minimal (peak-peak) amplitude value */
#define MIN_AMPLITUDE (float) 1/6

/** Size of the receive buffer and transmit buffer. */
#define BUFFER_SIZE     (100)
/** Reference voltage for ADC,in mv. */
#define VOLT_REF        (3300)
/* Tracking Time*/
#define TRACKING_TIME    1
/* Transfer Period */
#define TRANSFER_PERIOD  1
/* Startup Time*/
#define STARTUP_TIME ADC_STARTUP_TIME_4

#define ADC_POT_CHANNEL 5

char str[30];
  uint16_t i ;
  int clicado = 0;
  uint32_t amp = 4095;
  uint32_t FREQUENCIA = 10000;
  float conversao = (2*3.1415926)/MAX_DIGITAL;

static void configure_tc(float Freq)
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

/**
*  Interrupt handler for TC0 interrupt.
*/
void TC0_Handler(void){
  volatile uint32_t ul_dummy, status;
  uint32_t valorDAC = 1024;
  ul_dummy = tc_get_status(TC0,0);
  UNUSED(ul_dummy);
  
  	adc_start(ADC);
	tc_write_rc(TC0,0, 32768/FREQUENCIA);
  
  /************************************************************************/
  /* Escreve um novo valor no DAC                                         */
  /************************************************************************/
  status = dacc_get_interrupt_status(DACC_BASE);
 // dacc_write_conversion_data(DACC_BASE, valorDAC);
  
  if(i<= MAX_DIGITAL){
	  i++;
	  dacc_write_conversion_data(DACC_BASE, amp/2*sin(i* conversao) + MAX_DIGITAL/2);
  }
  else{
	  i = 0;
	  dacc_write_conversion_data(DACC_BASE, i);
  }
}

static void push_button_handle(uint32_t id, uint32_t mask)
{
	if (clicado == 0){
	clicado = 1;
	ili93xx_set_foreground_color(COLOR_WHITE);
	ili93xx_draw_filled_rectangle(9, 19,29,70);
	ili93xx_set_foreground_color(COLOR_BLACK);
	ili93xx_draw_string(10, 50, (uint8_t *)"->");
	}
	else{
	clicado = 0;
	ili93xx_set_foreground_color(COLOR_WHITE);
	ili93xx_draw_filled_rectangle(9, 19,29,70);
	ili93xx_set_foreground_color(COLOR_BLACK);
	ili93xx_draw_string(10, 20, (uint8_t *)"->");
	}
}

void configure_adc(void)
{
	/* Enable peripheral clock. */
	pmc_enable_periph_clk(ID_ADC);
	/* Initialize ADC. */
	/*
	* Formula: ADCClock = MCK / ( (PRESCAL+1) * 2 )
	* For example, MCK = 64MHZ, PRESCAL = 4, then:
	* ADCClock = 64 / ((4+1) * 2) = 6.4MHz;
	*/
	/* Formula:
	*     Startup  Time = startup value / ADCClock
	*     Startup time = 64 / 6.4MHz = 10 us
	*/
	adc_init(ADC, sysclk_get_cpu_hz(), 6400000, STARTUP_TIME);
	/* Formula:
	*     Transfer Time = (TRANSFER * 2 + 3) / ADCClock
	*     Tracking Time = (TRACKTIM + 1) / ADCClock
	*     Settling Time = settling value / ADCClock
	*
	*     Transfer Time = (1 * 2 + 3) / 6.4MHz = 781 ns
	*     Tracking Time = (1 + 1) / 6.4MHz = 312 ns
	*     Settling Time = 3 / 6.4MHz = 469 ns
	*/
	adc_configure_timing(ADC, TRACKING_TIME	, ADC_SETTLING_TIME_3, TRANSFER_PERIOD);

	adc_configure_trigger(ADC, ADC_TRIG_SW, 0);

	//adc_check(ADC, sysclk_get_cpu_hz());

	/* Enable channel for potentiometer. */
	adc_enable_channel(ADC, ADC_POT_CHANNEL);

	/* Enable the temperature sensor. */
	adc_enable_ts(ADC);

	/* Enable ADC interrupt. */
	NVIC_EnableIRQ(ADC_IRQn);

	/* Start conversion. */
	adc_start(ADC);

	//adc_read_buffer(ADC, gs_s_adc_values, BUFFER_SIZE);

	//adc_get_channel_value(ADC, ADC_POT_CHANNEL);

	/* Enable PDC channel interrupt. */
	adc_enable_interrupt(ADC, ADC_ISR_EOC5);
}

void ADC_Handler(void)
{
	
	uint32_t status ;

	status = adc_get_status(ADC);
	
	/* Checa se a interrupção é devido ao canal 5 */
	if ((status & ADC_ISR_EOC5)) {
		
		if (clicado == 0){
		
		if(amp >= adc_get_channel_value(ADC, ADC_POT_CHANNEL) + 10 || (amp <= adc_get_channel_value(ADC, ADC_POT_CHANNEL) - 10)){
			
			amp = adc_get_channel_value(ADC, ADC_POT_CHANNEL);
			
			ili93xx_set_foreground_color(COLOR_WHITE);
			ili93xx_draw_filled_rectangle(29, 19,200,49);
			sprintf(str, "Amp: %d",amp);
			ili93xx_set_foreground_color(COLOR_BLACK);
			ili93xx_draw_string(30, 20, (uint8_t *)str);

		}
		}else{
			if(FREQUENCIA >= 20000/MAX_DIGITAL*adc_get_channel_value(ADC, ADC_POT_CHANNEL) + 20 || FREQUENCIA <= 20000/MAX_DIGITAL*adc_get_channel_value(ADC, ADC_POT_CHANNEL) - 20){
				
				FREQUENCIA = 20000/MAX_DIGITAL*adc_get_channel_value(ADC, ADC_POT_CHANNEL) + 1;
				ili93xx_set_foreground_color(COLOR_WHITE);
				ili93xx_draw_filled_rectangle(29, 49,200,79);
				sprintf(str, "Freq: %d",FREQUENCIA);
				ili93xx_set_foreground_color(COLOR_BLACK);
				ili93xx_draw_string(30, 50, (uint8_t *)str);

			}
		}
	}
}

void configure_LCD(void){
  /** Enable peripheral clock */
  pmc_enable_periph_clk(ID_SMC);

  /** Configure SMC interface for Lcd */
  smc_set_setup_timing(SMC, ILI93XX_LCD_CS, SMC_SETUP_NWE_SETUP(2)
  | SMC_SETUP_NCS_WR_SETUP(2)
  | SMC_SETUP_NRD_SETUP(2)
  | SMC_SETUP_NCS_RD_SETUP(2));
  smc_set_pulse_timing(SMC, ILI93XX_LCD_CS, SMC_PULSE_NWE_PULSE(4)
  | SMC_PULSE_NCS_WR_PULSE(4)
  | SMC_PULSE_NRD_PULSE(10)
  | SMC_PULSE_NCS_RD_PULSE(10));
  smc_set_cycle_timing(SMC, ILI93XX_LCD_CS, SMC_CYCLE_NWE_CYCLE(10)
  | SMC_CYCLE_NRD_CYCLE(22));
  #if ((!defined(SAM4S)) && (!defined(SAM4E)))
  smc_set_mode(SMC, ILI93XX_LCD_CS, SMC_MODE_READ_MODE
  | SMC_MODE_WRITE_MODE
  | SMC_MODE_DBW_8_BIT);
  #else
  smc_set_mode(SMC, ILI93XX_LCD_CS, SMC_MODE_READ_MODE
  | SMC_MODE_WRITE_MODE);
  #endif
  /** Initialize display parameter */
  g_ili93xx_display_opt.ul_width = ILI93XX_LCD_WIDTH;
  g_ili93xx_display_opt.ul_height = ILI93XX_LCD_HEIGHT;
  g_ili93xx_display_opt.foreground_color = COLOR_BLACK;
  g_ili93xx_display_opt.background_color = COLOR_WHITE;

  /** Switch off backlight */
  aat31xx_disable_backlight();

  /** Initialize LCD */
  ili93xx_init(&g_ili93xx_display_opt);

  /** Set backlight level */
  aat31xx_set_backlight(AAT31XX_AVG_BACKLIGHT_LEVEL);

  ili93xx_set_foreground_color(COLOR_WHITE);
  ili93xx_draw_filled_rectangle(0, 0, ILI93XX_LCD_WIDTH,
  ILI93XX_LCD_HEIGHT);
  /** Turn on LCD */
  ili93xx_display_on();
  ili93xx_set_cursor_position(0, 0);
};

void configure_botao(void)
{
	pmc_enable_periph_clk(PIN_PUSHBUTTON_1_ID);
	pio_set_input(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_MASK, PIN_PUSHBUTTON_1_ATTR);
	pio_set_debounce_filter(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_MASK, 10);
	pio_handler_set(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_ID,PIN_PUSHBUTTON_1_MASK, PIN_PUSHBUTTON_1_ATTR ,push_button_handle);
	pio_enable_interrupt(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_MASK);
	NVIC_SetPriority((IRQn_Type) PIN_PUSHBUTTON_1_ID, 0);
	NVIC_EnableIRQ((IRQn_Type) PIN_PUSHBUTTON_1_ID);
}


/**
* \brief Application entry point for smc_lcd example.
*
* \return Unused (ANSI-C compatibility).
*/
int main(void)
{
  sysclk_init();
  board_init();

  configure_adc();
  configure_botao();
  configure_LCD();
  configure_tc(FREQUENCIA);

  ili93xx_draw_pixmap(0,
            ILI93XX_LCD_HEIGHT-100-1,
            240-1,
            100-1,
            &image_data_maua[0]);
  
  /************************************************************************/
  /* DAC                                                                  */
  /************************************************************************/
  
  /* Enable clock for DACC */
  sysclk_enable_peripheral_clock(DACC_ID);
  
  /* Reset DACC registers */
  dacc_reset(DACC_BASE);

  /* Half word transfer mode */
  dacc_set_transfer_mode(DACC_BASE, 0);

  /* selects channel */
  dacc_set_channel_selection(DACC_BASE, DACC_CHANNEL);

  /* Enable output channel DACC_CHANNEL */
  dacc_enable_channel(DACC_BASE, DACC_CHANNEL);

  /* Set up analog current */
  dacc_set_analog_control(DACC_BASE, DACC_ANALOG_CONTROL);
  
  sprintf(str, "Amp: %d",amp);
  ili93xx_set_foreground_color(COLOR_BLACK);
  ili93xx_draw_string(30, 20, (uint8_t *)str);
  ili93xx_draw_string(10, 20, (uint8_t *)"->");
  sprintf(str, "Freq: %d",FREQUENCIA);
  ili93xx_set_foreground_color(COLOR_BLACK);
  ili93xx_draw_string(30, 50, (uint8_t *)str);
 
  
  
 
  while (1) {
  	/*
      delay_us(100);
      if(i<= MAX_DIGITAL){
        dacc_write_conversion_data(DACC_BASE, i++);
	  }
      else{
        i = 0;
}
*/
  }
}
