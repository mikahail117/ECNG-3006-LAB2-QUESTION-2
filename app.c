/*
 * File:   app.c
 * Author: Armstrong Subero
 * Processor: SAMD21G18A @ 48MHz, 3.3v
 * Program: Source file for application
 * Compiler: ARM-GCC (v6.3.1, Atmel Studio 7.0)
 * Program Version 1.0
 * Program Description: This file contains your application source
 *
 * Modified From: None
 * 
 * Change History:
 *
 * Author             Rev     Date          Description
 * Armstrong Subero   1.1     06/04/2020    Updated for SD Card Specific Functions
 * Armstrong Subero   1.0     26/05/2020    Initial Release.
 * 
 * Updated on June 04, 2020, 02:47 PM
 */

//////////////////////////////////////////////////////////////////////////
// Include and defines
//////////////////////////////////////////////////////////////////////////

// Standard includes
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Primitives
#include "app.h"
#include "clock.h"
#include "USART3.h"
#include "SPI.h"

// FatFS Includes
#include "SD.h"
#include "diskio.h"
#include "ff.h"
#include "integer.h"
#include "ffconf.h"

// file handlers
FATFS fs;     // our work area
FRESULT FR;   // error results
FIL fil;      // file object
UINT bw;      // variable for pointer to number of bytes

// our data file
char data_file[12]="Data.txt";


/*******************************************************************************
 * Function:        void AppInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine takes care of all of the 1-time hardware/software
 *                  initialization that is required.
 *
 * Note:            This was taken from the _00_LED_ON provided by
 *                  Microchip Technology Inc. 
 *
 ******************************************************************************/
void AppInit(void)
{
	/*	Clock initialization (CPU, AHB, APBx, Asynchronous Peripheral Clocks)
		The System RC Oscillator (RCSYS) provides the source for the main clock
		at chip startup. It is set to 1MHz.
	*/
	ClocksInit();
	
	// Assign SS as OUTPUT
	REG_PORT_DIR0 = PORT_PA08;
	
	// Set SS OFF
	REG_PORT_OUTCLR0 = PORT_PA08;
} // AppInit()


/***************************************************************************
 * Function:        void AppRun(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function contains your main application
 *                  
 *
 * Note:
 *
 ***************************************************************************/
void AppRun(void)
{
	// Initialize the UART at 9600 baud
	UART3_Init(9600);
	delay_ms(500);
	
	// Initialize SPI
	UART3_Write_Text("Initializing SPI in slow mode\n");
	SPI_Initialize_Slow();
	UART3_Write_Text("SPI initialization done\n");
	delay_ms(500);
	
	//////////////////////////////////////////////////////////////////////////
	// Writing to File
	//////////////////////////////////////////////////////////////////////////
	
	// Start to Init the SD Card
	UART3_Write_Text("Starting SD card initialization\n");
	
	// Initialize the SD Card
    if (SDCard_Init() != 0) {
		UART3_Write_Text("SD card initialization failed\n");
		while (1);
	}
	UART3_Write_Text("SD card initialization done\n");
	
	// Start to mount the SD card
	UART3_Write_Text("Mounting file system\n");
	
	// Mount the file
	FR = f_mount(&fs, data_file, 0);
	
	// Finish Mounting
	UART3_Write_Text("File system mounted\n");
	
	// Error with mount
	if (FR) {
		UART3_Write_Text("Error mounting file system\n");
		while (1);
	}
	
	// Start Open
	UART3_Write_Text("Opening file for writing\n");
	
	// Open the SD Card for Writing
	FR = f_open(&fil, data_file, FA_WRITE | FA_OPEN_ALWAYS);
	
	// Check if the file was opened successfully
	if (FR) {
		UART3_Write_Text("Error opening file\n");
		while (1);
	}
	UART3_Write_Text("File opened successfully\n");
	
	// Open File to write some CSV Data
	UART3_Write_Text("Writing to file\n");
	FR = f_write(&fil, "Data1 ,Data2 ,Data3 ,Data4 \r\n", 29, &bw); 
	
	// Check for writing error
	if (FR) {
		UART3_Write_Text("Error writing to file\n");
		while (1);
	}
	UART3_Write_Text("Writing completed\n");
	
	// Close the file
	UART3_Write_Text("Closing file\n");
	FR = f_close(&fil);
	
	// Check for closing error
	if (FR) {
		UART3_Write_Text("Error closing file\n");
		while (1);
	}
	UART3_Write_Text("File closed successfully\n");
	
    UART3_Write_Text("Successful Write File Done!\n");

    //////////////////////////////////////////////////////////////////////////
	// Reading Files
	//////////////////////////////////////////////////////////////////////////
	UART3_Write_Text("Starting to read file\n");
	
    // Open the SD Card for reading
    FR = f_open(&fil, data_file, FA_READ);
	
    // Check if the file was opened successfully
    if (FR) {
        UART3_Write_Text("Error opening file for reading\n");
        while (1);
    }
    UART3_Write_Text("File opened for reading\n");
	
	// Print Read Contents
	UART3_Write_Text("The file contains: \n");
	char line[100]; /* Line buffer */
	
	/* Read every line and display it */
	while (f_gets(line, sizeof line, &fil)) {
		UART3_Write_Text(line);
	}
	
	UART3_Write_Text("Reading completed\n");
	
	// Close the file
	UART3_Write_Text("Closing file after reading\n");
	FR = f_close(&fil);
	
	// Check for closing error
	if (FR) {
		UART3_Write_Text("Error closing file after reading\n");
		while (1);
	}
	UART3_Write_Text("File closed successfully after reading\n");
	
	while (1) {
		// Infinite loop to keep the application running
	}
} // AppRun()
