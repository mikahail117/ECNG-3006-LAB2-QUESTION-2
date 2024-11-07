/*
 * File:   SD.c
 * Author: Armstrong Subero
 * Processor: SAMD21G18A @ 48MHz, 3.3v
 * Program: Source file for application
 * Compiler: ARM-GCC (v6.3.1, Atmel Studio 7.0)
 * Program Version 1.0
 * Program Description: This file contains source code for 
 *                      using the SD card File Functions
 *
 * Modified From: None
 * 
 * Change History:
 *
 * Author                Rev     Date          Description
 * Armstrong Subero      1.0     04/06/2020    Initial Release
 *
 * Updated on June 04th, 2020, 11:30 AM
 */ 

//////////////////////////////////////////////////////////////////////////
// Include and defines
//////////////////////////////////////////////////////////////////////////
#include "SD.h"
#include "SPI.h"
#include "app.h"
#include "delay.h"
#include "USART3.h"
#include "integer.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "clock.h"
#include "diskio.h"
#include "ff.h"
#include "ffconf.h"

// File system and file handlers
FATFS fs;      // Work area (file system object)
FRESULT fr;    // Result of file operations
FIL fil;       // File object
UINT bw;       // Number of bytes written

// Global variables for SD card operations
uint8_t dataBuffer[512];
uint8_t SD_Type = 0;

// Define SS (Slave Select) pin controls
#define GPIO_MAP_SS                      PORT_PA08
#define GPIO_GROUP_SS                    0
#define SPI_CS_LOW()                     PORT->Group[GPIO_GROUP_SS].OUTCLR.reg = GPIO_MAP_SS;
#define SPI_CS_HIGH()                    PORT->Group[GPIO_GROUP_SS].OUTSET.reg = GPIO_MAP_SS;

// Set SPI to low speed for initialization
void SDCard_InitSpeed(void) {
    SPI_Initialize_Slow();
}

// Set SPI to high speed for operation
void SDCard_RunSpeed(void) {
    SPI_Initialize_Fast();
}

// Control the Slave Select line
void SDCard_SS(uint8_t cs) {
    if (cs == 1) {
        SPI_CS_HIGH();
    } else {
        SPI_CS_LOW();
    }
}

// Wait for the SD card to be ready for reading
uint8_t SDCard_WaitRead(void) {
    uint32_t timeout = 0x00FFFFF;
    uint8_t response;

    while (timeout--) {
        response = SPI_SD_Send_Byte(0xFF);
        if (response == 0xFF) {
            return 0; // Ready
        }
    }
    UART3_Write_Text("Read wait timeout\n");
    return 1; // Timeout
}

// Wait for the SD card to be ready
uint8_t SD_WaitReady(void) {
    uint16_t attempt = 0;
    uint8_t data;

    do {
        data = SPI_SD_Send_Byte(0xFF);
        if (attempt++ == 0xFFFE) {
            UART3_Write_Text("Ready wait timeout\n");
            return 1; // Timeout
        }
    } while (data != 0xFF);

    return 0; // Ready
}

// Write a command to the SD card
uint8_t SDCard_WriteCmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    uint16_t timeout = 512;
    uint8_t response;

    SDCard_SS(1);
    SPI_SD_Send_Byte(0xFF);
    SDCard_SS(0);

    // Send command packet
    SPI_SD_Send_Byte(cmd | 0x40);
    SPI_SD_Send_Byte((uint8_t)(arg >> 24));
    SPI_SD_Send_Byte((uint8_t)(arg >> 16));
    SPI_SD_Send_Byte((uint8_t)(arg >> 8));
    SPI_SD_Send_Byte((uint8_t)arg);
    SPI_SD_Send_Byte(crc);

    // Wait for a response
    do {
        response = SPI_SD_Send_Byte(0xFF);
        timeout--;
    } while ((response == 0xFF) && timeout);

    if (timeout == 0) {
        UART3_Write_Text("Command response timeout\n");
    }

    // Deselect and send one more byte to finalize
    SDCard_SS(1);
    SPI_SD_Send_Byte(0xFF);

    return response; // Return response from SD card
}

// Initialize the SD card
uint8_t SDCard_Init(void) {
    uint8_t response;
    uint16_t retry = 0;

    // Set to low speed for initialization
    SDCard_InitSpeed();
    delay_ms(100);

    // Send initial 80 clock pulses
    for (uint8_t i = 0; i < 10; i++) {
        SPI_SD_Send_Byte(0xFF);
    }

    // Try to reset the SD card
    do {
        response = SDCard_WriteCmd(CMD0, 0x00, 0x95);
        retry++;
        if (retry >= 200) {
            UART3_Write_Text("Failed to reset card\n");
            return STA_NOINIT; // Initialization failed
        }
    } while (response != 1);

    UART3_Write_Text("Card reset successful\n");

    // Check SD card version with CMD8
    response = SDCard_WriteCmd(CMD8, 0x1AA, 0x87);
    if (response == 1) {
        uint16_t counter = 0xFFFF;
        do {
            SDCard_WriteCmd(CMD55, 0, 0xFF);
            response = SDCard_WriteCmd(CMD41, 0x40000000, 0xFF);
            counter--;
            if (counter == 0) {
                UART3_Write_Text("Timeout during initialization\n");
                return STA_NOINIT;
            }
        } while (response);

        response = SDCard_WriteCmd(CMD58, 0, 0xFF);
        if (response != 0x00) {
            UART3_Write_Text("Error reading OCR\n");
            return STA_NOINIT;
        }

        // Read OCR register
        for (uint8_t i = 0; i < 4; i++) {
            dataBuffer[i] = SPI_SD_Read_Byte();
        }

        SD_Type = (dataBuffer[0] & 0x40) ? SD_TYPE_V2HC : SD_TYPE_V2;
        UART3_Write_Text(SD_Type == SD_TYPE_V2HC ? "Card: V2.0 SDHC\n" : "Card Type: V2.0\n");
    } else {
        UART3_Write_Text("Unsupported SD card type\n");
        return STA_NOINIT;
    }

    // Set block length to 512 bytes
    if (SDCard_WriteCmd(CMD16, 512, 0xFF) != 0) {
        UART3_Write_Text("Error setting block length\n");
    }

    UART3_Write_Text("Initialization complete\n");

    // Switch to high speed for normal operation
    SDCard_RunSpeed();
    SDCard_SS(1);

    return 0; // Success
}

