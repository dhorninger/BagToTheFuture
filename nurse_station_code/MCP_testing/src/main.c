/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <math.h>

int16_t wavetable[256];
int step;
int offset = 0;
int size = sizeof wavetable / sizeof wavetable[0];

int language = 0;

void dma_setup5();

uint16_t dispmem[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};

void nano_wait(unsigned int n)
{
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

// *************** Code for Comms *************** //

void init_spi_comm()
{
    // PA15 - NSS
    // PB3 - SCK
    // PB4 - MISO
    // PB5 - MOSI
    RCC->AHBENR   |=  RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;  // Enable clock to Ports A and B
    GPIOA->MODER  &= ~GPIO_MODER_MODER15;  // Clear the bits of PA15 for AF
    GPIOB->MODER  &= ~0x00000fc0;  // clear the bits of Port B for AF
    GPIOA->MODER  |=  0x80000000;  // set the bits to 10 for PA15
    GPIOB->MODER  |=  0x00000a80;  // Set the bits to '10' for each pin (alternate function)
    GPIOA->AFR[1] &= ~0xf0000000;  // set them to AF0
    GPIOB->AFR[0] &= ~0x00fff000;  // Set the pins to use AF0
    RCC->APB2ENR  |=  RCC_APB2ENR_SPI1EN;  // Enable clock to SPI Channel 1

    SPI1->CR1 &= ~SPI_CR1_BIDIMODE;  // 2-line unidirectional mode
    SPI1->CR1 |=  SPI_CR1_MSTR;  // master mode
    SPI1->CR1 |=  0x0038;  // baud rate = f / 256
    SPI1->CR1 &= ~SPI_CR1_CPOL;  // clock is 0 when idle
    SPI1->CR1 &= ~SPI_CR1_CPHA;  // 1st clock transition is data capture edge
    SPI1->CR2 |= ~0x0f00;  // 16-bit word size
    SPI1->CR2 |=  SPI_CR2_SSOE;  // Configure slave select output enable
    SPI1->CR2 |=  SPI_CR2_NSSP;  // automatic NSS pulse generation.

    SPI1->CR1 |=  SPI_CR1_SPE;  // Enable the SPI channel 1
}

void dma_setup3(int data)
{
    RCC->AHBENR |= 0x1; // DMA clock enable

    DMA1_Channel3->CCR &= ~0x1;                   // ensure DMA is turned off
    DMA1_Channel3->CMAR = (uint32_t) data;        // copy from dispmem
    DMA1_Channel3->CPAR = (uint32_t) &(SPI1->DR); // copy to SPI2_DR
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
    SPI1->CR2 |= 0x2; // (TXDMAEN)

    DMA1_Channel3->CCR |= 0x1;  // enable DMA channel 3
}

void send_data(int data)
{
    while((SPI1->SR & SPI_SR_TXE) == 0)
        ; // wait for the transmit buffer to be empty

    // Deposit the data in the argument into the SPI channel 1 data register
    SPI1->DR = data;

    dma_setup3(data);
}

// *************** Setup Button *************** //

void setup_button()
{
    RCC->AHBENR |= 0x00000020;  // enable clock to port A
}

// *************** Setup Light **************** //

void setup_light()
{
    RCC->AHBENR  |= 0x00040000;  // enable clock to port B
    GPIOB->MODER |= 0x00400000;  // set pin 11 to output mode
}

int main()
{
    setup_button();
    setup_light();
    init_spi_comm();

    int data = 0b0110001110101101;

    while(1)
    {
        send_data(data);
    }

    return(0);
}


