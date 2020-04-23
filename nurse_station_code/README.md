# BagToTheFuture
ECE49022 Senior Design Team 11 2020

## Master_Control_Program_49022
This folder contains all of the files for the nurse's station without the SPI connection to the patient room.

## MCP_testing
This folder contains the files for the nurse's station with the SPI connection to the patient room. For simplicity, this is a direct wired connection between the master and slave STM32F051R8T6 boards.

## SPI_Slave_Testing
This folder contains the files for the patient room. There is an LED connected to PB8 that will remain turned on as long as the board is powered to make sure that the GPIO is outputing properly. There is another LED connected to PB15 that should turn on whenever data is being recieved and put into the SPI data register.