
#ifndef INC_SD_SPI_H_
#define INC_SD_SPI_H_

/***	INCLUDE	**************************************************************************************************************************************************************************************/
/******************************************************************************************************************************************************************************************************/
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "fatfs.h"

/***	DEFINES	***************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************/
#define OK 0
#define ERROR 1

#define CS_SD_GPIO_PORT GPIOA
#define CS_SD_PIN GPIO_PIN_3
#define SD_SELECT HAL_GPIO_WritePin(CS_SD_GPIO_PORT, CS_SD_PIN, GPIO_PIN_RESET)
#define SD_DESELECT HAL_GPIO_WritePin(CS_SD_GPIO_PORT, CS_SD_PIN, GPIO_PIN_SET)
#define LED_Red_ON HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); 															//	LED_Red
#define LED_Red_OFF HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);

/* Card type flags (CardType) */
#define CT_MMC 0x01 /* MMC ver 3 */
#define CT_SD1 0x02 /* SD ver 1 */
#define CT_SD2 0x04 /* SD ver 2 */
#define CT_SDC (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK 0x08 /* Block addressing */

// Definitions for MMC/SDC command
#define CMD0 (0x40+0) // GO_IDLE_STATE
#define CMD1 (0x40+1) // SEND_OP_COND (MMC)
#define ACMD41 (0xC0+41) // SEND_OP_COND (SDC)
#define CMD8 (0x40+8) // SEND_IF_COND
#define CMD9 (0x40+9) // SEND_CSD
#define CMD16 (0x40+16) // SET_BLOCKLEN
#define CMD17 (0x40+17) // READ_SINGLE_BLOCK
#define CMD24 (0x40+24) // WRITE_BLOCK
#define CMD55 (0x40+55) // APP_CMD
#define CMD58 (0x40+58) // READ_OCR

/***	TYPEDEF 	**************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************/
typedef struct sd_info
{
	__IO uint8_t type;			//SD Type
} sd_info_ptr;

/***	EXTERNAL VARIABLES	***********************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************/
extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;
extern FATFS SDFatFs;

/***	VARIABLES	************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************/
char USER_Path[4];
FATFS *fs;
FIL MyFile;
sd_info_ptr sdinfo;
FILINFO sFileInfo;
DIR sDirectory;
char aStringBuffer[60];
uint8_t aBuffer[512];

/***	FUNCTIONS PROTOTIPES	*******************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************/
uint8_t SD_SPI_Init(void);
uint8_t SPIx_WriteRead(uint8_t byte);
uint8_t SPI_ReceiveByte(void);
uint8_t SD_SPI_WaitingForReadiness(void);
uint8_t SD_SPI_Cmd (uint8_t cmd, uint32_t argument);
uint8_t SD_SPI_Read_Block(uint8_t *buff, uint32_t lba);
uint8_t SD_SPI_Write_Block (uint8_t *buff, uint32_t lba);
uint8_t SD_SPI_ReadFile(void);
uint8_t SD_SPI_WriteFile(void);
FRESULT SD_SPI_ReadLongFile(void);
uint8_t SD_SPI_GetFileInfo(void);
void SPI_Release(void);
void SPI_SendByte(uint8_t byte);
void SD_Error_Handler(void);

#endif /* INC_SD_SPI_H_ */
