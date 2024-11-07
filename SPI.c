/*
 * File:   SPI.c
 * Author: Armstrong Subero
 * Processor: SAMD21G18A @ 48MHz, 3.3v
 * Program: Source file for application
 * Compiler: ARM-GCC (v6.3.1, Atmel Studio 7.0)
 * Program Version 1.0
 * Program Description: This file contains source code for 
 *                      using the SPI module and SD card 
 *                      communication routines
 *
 * Modified From: None
 * 
 * Change History:
 *
 * Author                Rev     Date          Description
 * Armstrong Subero      1.1     04/06/2020    Added SD Card Support
 * Armstrong Subero      1.0     01/05/2020    Initial Release
 *
 * Updated on June 04th, 2020, 11:30 AM
 */ 


//////////////////////////////////////////////////////////////////////////
// Include and defines
//////////////////////////////////////////////////////////////////////////
#include "app.h"
#include "SPI.h"


#define CS_PIN 21  //

void SPI_Initialize(void) {
	// Existing initialization code...
	
	// Set PA21 as an output and set it high initially
	PORT->Group[0].DIRSET.reg = (1 << CS_PIN);  // Configure PA21 as output
	PORT->Group[0].OUTSET.reg = (1 << CS_PIN);  // Set PA21 high initially (CS inactive)
}

void SPI_SD_Select(void) {
    PORT->Group[0].OUTCLR.reg = (1 << CS_PIN);  // Pull CS low
}

void SPI_SD_Deselect(void) {
    PORT->Group[0].OUTSET.reg = (1 << CS_PIN);  // Pull CS high
}
void SPI1_Initialize(void) {
	// Configure GCLK for SERCOM1 (SPI1)
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SERCOM1_GCLK_ID_CORE) |
	GCLK_CLKCTRL_CLKEN |
	GCLK_CLKCTRL_GEN(0);

	// Wait for synchronization
	while (GCLK->STATUS.bit.SYNCBUSY);

	// Enable the bus clock to SERCOM1
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM1;

	// Set up the SPI control registers for SERCOM1
	SERCOM1->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_MODE_SPI_MASTER |
	SERCOM_SPI_CTRLA_DIPO(3) |  // MISO on PAD3
	SERCOM_SPI_CTRLA_DOPO(0);   // MOSI on PAD0, SCK on PAD1

	SERCOM1->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN;  // Enable receive

	// Set the baud rate
	SERCOM1->SPI.BAUD.reg = ((float)48000000 / (2 * 400000)) - 1;  // 400 kHz for initialization

	// Enable SPI
	SERCOM1->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
	while (SERCOM1->SPI.SYNCBUSY.bit.ENABLE);
}
/***************************************************************************
 * Function:        void SPI_Initialize_Fast(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function initializes the SPI bus at 12 MHz baud
 *                  
 *
 * Note:            Needed for SPI initial slow function
 ***************************************************************************/
void SPI_Initialize_Fast(void) {
    // Wait Sync
    while (SERCOM1->SPI.SYNCBUSY.bit.ENABLE);

    /* -------------------------------------------------
     * 1) Enable bus clock to APBC mask
     */
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM1;                               

    /* -------------------------------------------------
     * 2) select SPI clock
     */
    GCLK->CLKCTRL.reg = 
    GCLK_CLKCTRL_ID(SERCOM1_GCLK_ID_CORE) |                
    GCLK_CLKCTRL_CLKEN | 
    GCLK_CLKCTRL_GEN(0);

    while (GCLK->STATUS.bit.SYNCBUSY);

    /* -------------------------------------------------
     * 3) setup pins
     */ 
    // Configure PA16 (MOSI)
    PORT->Group[0].WRCONFIG.reg =
        PORT_WRCONFIG_WRPINCFG |            // Enables the configuration of PINCFG
        PORT_WRCONFIG_WRPMUX |              // Enables the configuration of the PMUX for the selected pins
        PORT_WRCONFIG_PMUXEN |              // Enables the PMUX for the pins
        PORT_WRCONFIG_PMUX(MUX_PA16C_SERCOM1_PAD0) |  // PA16 for SERCOM1 PAD0 (MOSI)
        PORT_WRCONFIG_HWSEL |               // Selects the correct pin configurations for 16-31
        PORT_WRCONFIG_PINMASK((uint16_t)((PORT_PA16) >> 16)); // Pin PA16 configuration

    // Configure PA19 (MISO)
    PORT->Group[0].WRCONFIG.reg =
        PORT_WRCONFIG_WRPINCFG |
        PORT_WRCONFIG_WRPMUX |
        PORT_WRCONFIG_PMUXEN |
        PORT_WRCONFIG_PMUX(MUX_PA19C_SERCOM1_PAD3) |  // PA19 for SERCOM1 PAD3 (MISO)
        PORT_WRCONFIG_HWSEL |
        PORT_WRCONFIG_PINMASK((uint16_t)((PORT_PA19) >> 16)); // Pin PA19 configuration

    // Configure PA17 (SCK)
    PORT->Group[0].WRCONFIG.reg =
        PORT_WRCONFIG_WRPINCFG |
        PORT_WRCONFIG_WRPMUX |
        PORT_WRCONFIG_PMUXEN |
        PORT_WRCONFIG_PMUX(MUX_PA17C_SERCOM1_PAD1) |  // PA17 for SERCOM1 PAD1 (SCK)
        PORT_WRCONFIG_HWSEL |
        PORT_WRCONFIG_PINMASK((uint16_t)((PORT_PA17) >> 16)); // Pin PA17 configuration

    // Set the CS pin as output
    PORT->Group[0].DIRSET.reg = (1 << CS_PIN);
    PORT->Group[0].OUTSET.reg = (1 << CS_PIN);  // Set CS high initially

    /* -------------------------------------------------
     * 4) Configure SPI Module
     */ 
    // Disable the SERCOM SPI module 
    SERCOM1->SPI.CTRLA.bit.ENABLE = 0;

    // Wait for synchronization 
    while (SERCOM1->SPI.SYNCBUSY.bit.SWRST);

    // Perform a software reset 
    SERCOM1->SPI.CTRLA.bit.SWRST = 1;
    
    // Wait for synchronization 
    while (SERCOM1->SPI.CTRLA.bit.SWRST);

    // Wait for synchronization
    while (SERCOM1->SPI.SYNCBUSY.bit.SWRST || SERCOM1->SPI.SYNCBUSY.bit.ENABLE);

    SERCOM1->SPI.CTRLA.reg = 
        SERCOM_SPI_CTRLA_MODE_SPI_MASTER |   // set SPI Master Mode
        SERCOM_SPI_CTRLA_DIPO(3) |           // PAD3 is used as data input (MISO)
        SERCOM_SPI_CTRLA_DOPO(0);            // PAD0 is used as data output (MOSI), PAD1 is SCK

    SERCOM1->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN;      // Enable SPI Receive enable

    /* -------------------------------------------------
     * 5) Set the baud rate
     */ 
    uint32_t BAUD_REG =  ((float)48000000 / (float)(2 * 12000000)) - 1; // Calculate BAUD value
    SERCOM1->SPI.BAUD.reg = SERCOM_SPI_BAUD_BAUD(BAUD_REG);

    /* -------------------------------------------------
     * 6) Enable SPI Module
     */ 
    SERCOM1->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE; // Enable the SPI
    while (SERCOM1->SPI.SYNCBUSY.bit.ENABLE);
}

/***************************************************************************
 * Function:        void SPI_Initialize_Slow(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function initializes the SPI bus at 400 kHz baud
 *                  
 *
 * Note:            Needed for SPI initial slow function
 ***************************************************************************/
void SPI_Initialize_Slow(void) {
    // Wait Sync
    while (SERCOM1->SPI.SYNCBUSY.bit.ENABLE);

    /* -------------------------------------------------
     * 1) Enable bus clock to APBC mask
     */
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM1;                               

    /* -------------------------------------------------
     * 2) select SPI clock
     */
    GCLK->CLKCTRL.reg = 
    GCLK_CLKCTRL_ID(SERCOM1_GCLK_ID_CORE) |                
    GCLK_CLKCTRL_CLKEN | 
    GCLK_CLKCTRL_GEN(0);

    while (GCLK->STATUS.bit.SYNCBUSY);

    /* -------------------------------------------------
     * 3) setup pins
     */ 
    // Configure PA16 (MOSI)
    PORT->Group[0].WRCONFIG.reg =
        PORT_WRCONFIG_WRPINCFG |
        PORT_WRCONFIG_WRPMUX |
        PORT_WRCONFIG_PMUXEN |
        PORT_WRCONFIG_PMUX(MUX_PA16C_SERCOM1_PAD0) |
        PORT_WRCONFIG_HWSEL |
        PORT_WRCONFIG_PINMASK((uint16_t)((PORT_PA16) >> 16));

    // Configure PA19 (MISO)
    PORT->Group[0].WRCONFIG.reg =
        PORT_WRCONFIG_WRPINCFG |
        PORT_WRCONFIG_WRPMUX |
        PORT_WRCONFIG_PMUXEN |
        PORT_WRCONFIG_PMUX(MUX_PA19C_SERCOM1_PAD3) |
        PORT_WRCONFIG_HWSEL |
        PORT_WRCONFIG_PINMASK((uint16_t)((PORT_PA19) >> 16));

    // Configure PA17 (SCK)
    PORT->Group[0].WRCONFIG.reg =
        PORT_WRCONFIG_WRPINCFG |
        PORT_WRCONFIG_WRPMUX |
        PORT_WRCONFIG_PMUXEN |
        PORT_WRCONFIG_PMUX(MUX_PA17C_SERCOM1_PAD1) |
        PORT_WRCONFIG_HWSEL |
        PORT_WRCONFIG_PINMASK((uint16_t)((PORT_PA17) >> 16));

}


/*******************************************************************************
 * Function:        uint8_t SPI_Exchange8bit(uint8_t data)
 *
 * PreCondition:    The SPI Bus Must be Initialized
 *
 * Input:           The data we want to send
 *
 * Output:          The data Exchanged via SPI
 *
 * Side Effects:    None
 *
 * Overview:        This function exchanges a byte of data on the SPI Bus
 *                  
 *
 * Note:            
 *
 ******************************************************************************/
uint8_t SPI_Exchange8bit(uint8_t data)
{
	while(SERCOM1->SPI.INTFLAG.bit.DRE == 0);
	SERCOM1->SPI.DATA.reg = data;
	
	while(SERCOM1->SPI.INTFLAG.bit.RXC == 0);
	return (uint8_t)SERCOM1->SPI.DATA.reg;
}


/*******************************************************************************
 * Function:        uint8_t SPI_SD_Send_Byte(uint8_t byte_val)
 *
 * PreCondition:    The SPI Bus Must be Initialized
 *
 * Input:           The data we want to send
 *
 * Output:          Data Exchanged on SPI Transfer
 *
 * Side Effects:    None
 *
 * Overview:        This function sends a byte of data to the SD Card
 *                  
 *
 * Note:            
 *
 ******************************************************************************/
uint8_t SPI_SD_Send_Byte(uint8_t byte_val)
{
	uint8_t data;
	
	data = SPI_Exchange8bit(byte_val);
	return data;
}



/*******************************************************************************
 * Function:        uint8_t SPI_SD_Read_Byte(void)
 *
 * PreCondition:    SPI Bus Must Be Initialized
 *
 * Input:           None
 *
 * Output:          Data Read from SPI Bus
 *
 * Side Effects:    None
 *
 * Overview:        This function reads a byte of data from the SPI Module
 *                  
 *
 * Note:            
 *
 ******************************************************************************/
uint8_t SPI_SD_Read_Byte(void)
{
	uint8_t data;

	data = SPI_Exchange8bit(0xff);
	return data;
}