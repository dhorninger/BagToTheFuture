// *********************************** //
//                                     //
// This project is for testing code    //
// for the MCP in isolation.           //
//                                     //
// *********************************** //

#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// ************** Definitions ************** //

int note_c = 262;
int note_d = 294;
int note_e = 330;
int note_f = 349;
int note_g = 392;
int note_a = 440;
int note_b = 494;

bool language = true;  // language = english

int16_t wavetable[256];
int step;
int offset = 0;
int size = sizeof wavetable / sizeof wavetable[0];

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

// ************** End Definitions ************** //

// ********* Init SPI ********* //

        // PA15 - NSS
        // PB3  - SCK
        // PB4  - MISO
        // PB5  - MOSI

void init_spi1()
{
    RCC->AHBENR   |=  0x00060000;  // Enable clock to Ports A and B
    GPIOA->MODER  &= ~0xc0000000;  // Clear the bits of PA15 for AF
    GPIOB->MODER  &= ~0x00000fc0;  // clear the bits of Port B for AF
    GPIOA->MODER  |=  0x80000000;  // set the bits to 10 for PA15
    GPIOB->MODER  |=  0x00000a80;  // Set the bits to '10' for each pin (alternate function)
    GPIOA->AFR[1] &= ~0xf0000000;  // set them to AF0
    GPIOB->AFR[0] &= ~0x00fff000;  // Set the pins to use AF0
    RCC->APB2ENR  |=  0x00001000;  // Enable clock to SPI Channel 1

    SPI1->CR1 &= ~0x8000;  // 2-line unidirectional mode
    SPI1->CR1 &= ~0x0004;  // master mode
    SPI1->CR1 |=  0x0038;  // baud rate = f / 256
    SPI1->CR1 &= ~0x0002;  // clock is 0 when idle
    SPI1->CR1 &= ~0x0001;  // 1st clock transition is data capture edge
    SPI1->CR2 |= ~0x0f00;  // 16-bit word size
    SPI1->CR2 |=  0x0008;  // automatic NSS pulse generation.

    SPI1->CR1 |=  0x0040;  // Enable the SPI channel 1
}

int main(void)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;  // enable clock to GPIOB
    GPIOB->MODER |= 0x80010000;          // set PB8 and PB15 to output mode
    GPIOB->ODR   |= 0x00000100;          // turn on the LED at PB8

    init_spi1();

    while (1)
    {
        if ((SPI1->DR) != 0x0)
        {
            GPIOB->ODR |= 0x00008000;  // turn on the LED at PB15
        }
        else
        {
            GPIOB->ODR &= ~0x00008000;  // turn off the LED at PB15
        }
    }
}
