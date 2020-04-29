#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>


int16_t wavetable[256];
int step;
int offset = 0;
int size = sizeof wavetable / sizeof wavetable[0];


// ************************* Definitions ************************* //

uint16_t dispmem[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};

// **************************** Code ***************************** //

void dma_setup5();

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void cmd(char b) {
    while((SPI2 -> SR & SPI_SR_TXE) == 0)
        ; // wait for the transmit buffer to be empty

    // Deposit the data in the argument into the SPI channel 2 data register
    SPI2 -> DR = b;
}

void data(char b) {
    while((SPI2 -> SR & SPI_SR_TXE) == 0)
        ; // wait for the transmit buffer to be empty

    // Deposit the data in the argument into the SPI channel 2 data register + 0x200
    SPI2 -> DR = 0x200 + b;
}

/* ********* Init SPI ********* //
        // PB11 - DC Sel.
        // PB12 - NSS
        // PB13 - SCK
        // PB14 - MISO
        // PB15 - MOSI*/

void init_spi_lcd() {
    // ------- for small screen ------- //

    RCC->AHBENR   |=  0x00040000;  // Enable clock to Port B
    GPIOB->MODER  &= ~0xcf000000;  // Clear the bits
    GPIOB->MODER  |=  0x8a000000;  // Set the bits to '10' for each pin (alternate function)
    GPIOB->AFR[1] &= ~0xf0ff0000;  // Set the pins to use AF0
    RCC->APB1ENR  |=  0x00004000;  // Enable clock to SPI Channel 2

    SPI2->CR1 |=  0xc03c;  // bidirectional mode, bidirectional output, master, lowest baud rate
    SPI2->CR1 &= ~0x0003;  // clock is 0 when idle, 1st transition is data capture edge
    SPI2->CR2 |=  0x0f00;  // clear data size bits
    SPI2->CR2 &= ~0x0600;  // data size is 10 bits
    SPI2->CR2 |=  0x000c;  // slave select output enable, generates NSS pulse

    SPI2->CR1 |=  0x0040;  // enable SPI ch. 2
}


void dma_setup3(int data)
{
    RCC->AHBENR |= 0x1; // DMA clock enable

    SPI1->CR2 |= 0x2; // (TXDMAEN)

    DMA1_Channel3->CCR &= ~0x1;                   // ensure DMA is turned off
    DMA1_Channel3->CPAR = (uint32_t) (&(SPI1->DR)); // copy to SPI2_DR
    DMA1_Channel3->CMAR = (uint32_t) (data);        // copy from dispmem
    DMA1_Channel3->CNDTR = 34;

    DMA1_Channel3->CCR &= ~0x0c00;  // clear MSIZE bits
    DMA1_Channel3->CCR |=  0x0400;  // MSIZE 01: 16-bits
    DMA1_Channel3->CCR &= ~0x0300;  // clear PSIZE bits
    DMA1_Channel3->CCR |=  0x0100;  // PSIZE 01: 16-bits
    DMA1_Channel3->CCR |=  0x0010;  // read from memory (DIR)
    DMA1_Channel3->CCR |=  0x0080;  // enable MINC
    DMA1_Channel3->CCR &= ~0x0040;  // disable PINC
    DMA1_Channel3->CCR &= ~0x3000;  // channel low priority (PL)
    DMA1_Channel3->CCR &= ~0x0020;  // enable circular mode (CIRC)
    DMA1_Channel3->CCR &= ~0x0002;  // disable TCI
    DMA1_Channel3->CCR &= ~0x0004;  // disable HTI
    DMA1_Channel3->CCR &= ~0x4000;  // disable MEM2MEM

    // modify SPI channel 1 so that a DMA request is made whenever the transmitter buffer is empty


    DMA1_Channel3->CCR |= 0x1;  // enable DMA channel 3
}


void init_spi_esp() {
    // PA15 - NSS
    // PB3 - SCK
    // PB4 - MISO
    // PB5 - MOSI
	RCC->AHBENR   |=  RCC_AHBENR_GPIOAEN;
	RCC->AHBENR   |=  RCC_AHBENR_GPIOBEN;// Enable clock to Ports A and B
    GPIOA->MODER  &= ~GPIO_MODER_MODER15;  // Clear the bits of PA15 for AF
    GPIOA->MODER  |=  0xA8000000;  // set the bits to 10 for PA15
    GPIOB->MODER  |=  0x00000a80;  // Set the bits to '10' for each pin (alternate function)
    GPIOA->AFR[1] &= ~0xf0000000;  // set them to AF0
    GPIOB->AFR[0] &= ~0x00fff000;  // Set the pins to use AF0
    RCC->APB2ENR  |=  RCC_APB2ENR_SPI1EN;  // Enable clock to SPI Channel 1

    //SPI1->CR1 |= SPI_CR1_BIDIMODE;  // bidirectional mode
    //SPI1->CR1 |= SPI_CR1_BIDIOE;    // bidirectional output
    SPI1->CR1 |=  SPI_CR1_MSTR;  // master mode
    SPI1->CR1 |=  0x0038;  // baud rate = f / 256
    SPI1->CR1 &= ~SPI_CR1_CPOL;  // clock is 0 when idle
    SPI1->CR1 &= ~SPI_CR1_CPHA;  // 1st clock transition is data capture edge
    SPI1->CR2 &= 0x0700;  // 8-bit word size
    SPI1->CR2 |=  SPI_CR2_SSOE;  // Configure slave select output enable
    SPI1->CR2 |=  SPI_CR2_NSSP;  // automatic NSS pulse generation.
    SPI1->CR2 |= SPI_CR2_SSOE;
    SPI1->CR2 |= SPI_CR2_RXNEIE;
    SPI1->CR2 |= SPI_CR2_FRXTH;

    SPI1->CR1 |=  SPI_CR1_SPE;  // Enable the SPI channel 1

}



void send_data(int data)
{
    while((SPI1->SR & SPI_SR_TXE) == 0)   // wait for the transmit buffer to be empty
    	;

    // Deposit the data in the argument into the SPI channel 1 data register
    //SPI1->DR &= 0x0000;

    SPI1->DR = data;
}


void dma_setup5()
{
    RCC->AHBENR |= 0x1; // DMA clock enable

    DMA1_Channel5->CCR &= ~0x1; // ensure DMA is turned off
    DMA1_Channel5->CMAR = (uint32_t) dispmem; // copy from dispmem
    DMA1_Channel5->CPAR = (uint32_t) &(SPI2 -> DR); // copy to SPI2_DR
    DMA1_Channel5->CNDTR = 34;

    DMA1_Channel5->CCR &= ~0x0c00;  // clear MSIZE bits
    DMA1_Channel5->CCR |=  0x0400;  // MSIZE 01: 16-bits
    DMA1_Channel5->CCR &= ~0x0300;  // clear PSIZE bits
    DMA1_Channel5->CCR |=  0x0100;  // PSIZE 01: 16-bits
    DMA1_Channel5->CCR |=  0x0010;  // read from memory (DIR)
    DMA1_Channel5->CCR |=  0x0080;  // enable MINC
    DMA1_Channel5->CCR &= ~0x0040;  // disable PINC
    DMA1_Channel5->CCR &= ~0x3000;  // channel low priority (PL)
    DMA1_Channel5->CCR &= ~0x0020;  // enable circular mode (CIRC)
    DMA1_Channel5->CCR &= ~0x0002;  // disable TCI
    DMA1_Channel5->CCR &= ~0x0004;  // disable HTI
    DMA1_Channel5->CCR &= ~0x4000;  // disable MEM2MEM

    // modify SPI channel 2 so that a DMA request is made whenever the transmitter buffer is empty
    SPI2->CR2 |= 0x2; // (TXDMAEN)

    DMA1_Channel5->CCR |= 0x1;  // enable DMA channel 5
}


void setup_timer2(int freq)
{
    RCC->APB1ENR |= 0x1;  // Enable the system clock for timer 2

    TIM2->CR1  &= ~0x0070;  // Set timer so that it is non-centered and up-counting
    TIM2->PSC   = 2 - 1;    // Set the Prescaler output to 24MHz (48MHz/2)
    TIM2->ARR   = (((24000000) / freq) / size) - 1;    // Auto-reload at 213 so that it reloads at 112676 Hz
    TIM2->DIER |=  0x0001;  // Enable timer 2 interrupt (set UIE bit)
    TIM2->CR1  |=  0x0001;  // Start the timer

    NVIC->ISER[0] = 1 << TIM2_IRQn;  // Enable timer 2 interrupt
}

void TIM2_IRQHandler()
{
    TIM2->SR     &= ~0x1;  // Acknowledge the interrupt
    DAC->DHR12R1  = wavetable[offset];
    DAC->SWTRIGR |=  0x1;  // Software trigger the DAC

    offset += step;

    if (offset >= size)
    {
        offset -= size;
    }
}


void setup_DAC()
{

    RCC->APB1ENR |=  0x20000000;  // Enable clock to DAC
    DAC->CR      &= ~0x00000001;  // Disable DAC first.
    RCC->AHBENR  |=  0x00020000;  // Enable clock to GPIOA
    GPIOA->MODER |=  0x00000300;  // Set PA4 for Analog mode
    DAC->CR      |=  0x00000038;  // Enable software trigger
    DAC->CR      |=  0x00000004;  // Enable trigger
    DAC->CR      |=  0x00000001;  // Enable DAC Channel 1
}


void init_sound()
{
    setup_DAC();
    // Initialize the sine wave
    for(int i = 0; i < size; i++)
    {
        wavetable[i] = (32767 * sin(i * 2 * M_PI / size) + 32768) / 16;
    }

    step = 1;
    int count;

    for (count = 0; count < 2; count++)
    {
        setup_timer2(440);
        nano_wait(50000000);
        TIM2->CR1 &= ~0x1;
        nano_wait(50000000);
    }
}


void generic_lcd_startup(void) {
    nano_wait(100000000); // Give it 100ms to initialize
    cmd(0x38);  // 0011 NF00 N=1, F=0: two lines
    cmd(0x0c);  // 0000 1DCB: display on, no cursor, no blink
    cmd(0x01);  // clear entire display
    nano_wait(6200000); // clear takes 6.2ms to complete
    cmd(0x02);  // put the cursor in the home position
    cmd(0x06);  // 0000 01IS: set display to increment
}


void display1(const char *s)
{
    // Place the cursor on the beginning of the first line (offset 0).
    cmd(0x80 + 0);
    int x;
    for(x=0; x<16; x+=1)
    {
        if (s[x])
        {
            int my_val = s[x];
            my_val |= 0x200;
            dispmem[x+1] = my_val;
        }

        else
            break;
    }

    for(   ; x<16; x+=1)
    {
        dispmem[x+1] = 0x220;
    }

    dma_setup5();
}

void display2(const char *s) {
    // Place the cursor on the beginning of the second line (offset 64).
    cmd(0x80 + 64);
    int x;
    for(x=0; x<16; x+=1)
    {
        if (s[x])
        {
            int my_val = s[x];
            my_val |= 0x200;
            dispmem[x+18] = my_val;
        }

        else
            break;
    }

    for(   ; x<16; x+=1)
    {
        dispmem[x+18] = 0x220;
    }

    dma_setup5();
}


void adc_init(void) {
	//RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA -> MODER |= GPIO_MODER_MODER1;

	RCC -> APB2ENR |= (1<<9);
	RCC -> CR2 |= RCC_CR2_HSI14ON;
	while(!(RCC -> CR2 & RCC_CR2_HSI14RDY));

	ADC1 -> CHSELR = 0;
	ADC1 -> CHSELR |= ADC_CHSELR_CHSEL4;

	//ANALOG WATCHDOG ENABLE
	ADC1 -> CFGR1 |= ADC_CFGR1_CONT;
	ADC1 -> CFGR1 |= 0x10000000;
	ADC1 -> CFGR1 |= ADC_CFGR1_AWDEN;
	ADC1 -> CFGR1 |= ADC_CFGR1_AWDSGL;
	ADC1 -> TR = 0xfff << 16; //3V high threshold
	ADC1 -> TR = 0x89; //0.1V low threshold

	ADC1 -> CR |= ADC_CR_ADEN;
	while(!(ADC1 -> ISR & ADC_ISR_ADRDY));
	while((ADC1 -> CR & ADC_CR_ADSTART));

	return;
}


int adc2_read(void) {
	ADC1 -> CHSELR = 0;
	ADC1 -> CHSELR |= ADC_CHSELR_CHSEL2;
	while(!(ADC1 -> ISR & ADC_ISR_ADRDY));
	ADC1 -> CR |= ADC_CR_ADSTART;
	while(!(ADC1 -> ISR & ADC_ISR_EOC));

	return(ADC1 -> DR);
}



void gpio_set(void) {

	RCC->AHBENR |= (1 << 19);
	GPIOC->MODER |= (1 << 16);//Set pin C8 as output
	GPIOC->OTYPER &= ~(1 << 8);//Set pin 8 output as internal push-pull

	GPIOC->MODER |= (1 << 18);//Set pin C9 as output
	GPIOC->OTYPER &= ~(1 << 9);//Set pin 9 output as internal push-pull

	// enable user PA0 button
	RCC->AHBENR |= (1 << 17); /* Enable GPIOA clock */

}


void test_sound() {
    setup_DAC();
    for(int i = 0; i < size; i++)
    {
        wavetable[i] = (32767 * sin(i * 2 * M_PI / size) + 32768) / 16;
    }

    step = 1;
    int count;

    for (count = 0; count < 10; count++)
    {
        setup_timer2(440);
        nano_wait(100000000);
        TIM2->CR1 &= ~0x1;
    }
}


int main(void) {

	int rm_lvl = 0b1100100;

	init_spi_lcd();
	init_spi_esp(rm_lvl);
	generic_lcd_startup();

	adc_init();
	gpio_set();

	setup_DAC();
	init_sound();

	float adc_alert_curr = 0;
	int rm_num = 10;
	int rm_rat = 250;
	char string[16];
	float adc_dec = 0;

	float const1 = 36.891;
	float const2 = 10.672;

	int count = 101;
	int lang_select = 0;

	dma_setup3(rm_lvl);
	send_data(rm_num);  //send room number to SPI channel

	send_data(rm_rat);  //send flow rate to SPI channel

	while(1) {

		while(count >= rm_lvl) {

			if(GPIOA -> IDR & (1<<0)) {
				if(lang_select == 1) {
					lang_select = 0;
					count = 100;
					break;
				}
				else {
					lang_select = 1;
					count = 100;
					break;
				}
			}

			adc_alert_curr = adc2_read();
			adc_dec = 3 * adc_alert_curr / 4095;
			rm_lvl = const1 * adc_dec - const2;

			count = rm_lvl;

			if((GPIOA -> IDR && (1<<11)) & (rm_lvl <= 20)) {

				if((GPIOA -> IDR & (1<<10)) && (rm_lvl <= 6)) {

					if(lang_select == 0) {
						display1("Nurse must      ");
						display2("change IV now   ");
						nano_wait(100000000);
					}
					else {
						display1("Enfermera debe  ");
						display2("cambiar IV ahora");
						nano_wait(100000000);
					}
				}
				else {
					if(lang_select == 0) {
						display1("Nurse station   ");
						display2("has been alerted");
						nano_wait(100000000);
						test_sound();
					}
					else {
						display1("Estacion de en-");
						display2("fermas alertada");
						nano_wait(100000000);
						test_sound();
					}

				}
			}

			if(rm_lvl > 20) {
				sprintf(string, "%d   %d    %d", rm_num, rm_lvl, rm_rat);

				if(lang_select == 0) {
					display1("ROOM LEVEL  RATE");
					display2(string);
					nano_wait(100000000);
				}
				else {
					display1("SALA NIVEL RITMO");
					display2(string);
					nano_wait(100000000);
				}
			}
			send_data(rm_lvl); //update SPI channel with current fluid level

		}
	}


	for(;;)  asm("wfi");
}



