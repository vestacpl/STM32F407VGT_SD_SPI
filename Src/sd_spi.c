/***	INCLUDE	**************************************************************************************************************************************************************************************/
/******************************************************************************************************************************************************************************************************/
#include <sd_spi.h>

/***	FUNCTIONS	***********************************************************************************************************************************************************************************/
/******************************************************************************************************************************************************************************************************/
uint8_t SD_SPI_Init(void)
{
	uint8_t vCmd;
	int16_t vCounter;
	//uint32_t vTmpPrc;
	sdinfo.type = 0;
	uint8_t aArray[4];

	HAL_Delay(250);														// SD voltage stability delay

	/*vTmpPrc = hspi2.Init.BaudRatePrescaler;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; 		//156.25 kbbs*/

	HAL_SPI_Init(&hspi2);

	SD_DESELECT;
	for(vCounter = 0; vCounter<10; vCounter++) // 80 pulse bit. Set SPI as SD card interface
		SPI_Release();

	/*hspi2.Init.BaudRatePrescaler = vTmpPrc;
	HAL_SPI_Init(&hspi2);*/

	SD_SELECT;
	if (SD_SPI_Cmd(CMD0, 0) == 1) // Enter Idle state
		{
			SPI_Release();
			if (SD_SPI_Cmd(CMD8, 0x1AA) == 1) // SDv2
				{
					for (vCounter = 0; vCounter < 4; vCounter++)
						aArray[vCounter] = SPI_ReceiveByte();
					if (aArray[2] == 0x01 && aArray[3] == 0xAA) // The card can work at vdd range of 2.7-3.6V
						{
							for (vCounter = 12000; (vCounter && SD_SPI_Cmd(ACMD41, 1UL << 30)); vCounter--)	{;}	 // Wait for leaving idle state (ACMD41 with HCS bit)
							if (vCounter && SD_SPI_Cmd(CMD58, 0) == 0)
								{ // Check CCS bit in the OCR
									for (vCounter = 0; vCounter < 4; vCounter++) 	aArray[vCounter] = SPI_ReceiveByte();
									sdinfo.type = (aArray[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; // SDv2 (HC or SC)
								}
						}
				}
			else		//SDv1 or MMCv3
				{
					if (SD_SPI_Cmd(ACMD41, 0) <= 1)
						{
							sdinfo.type = CT_SD1; vCmd = ACMD41; // SDv1
						}
						else
						{
							sdinfo.type = CT_MMC; vCmd = CMD1; // MMCv3
						}
					for (vCounter = 25000; vCounter && SD_SPI_Cmd(vCmd, 0); vCounter--) ; // Wait for leaving idle state
					if ( ! vCounter || SD_SPI_Cmd(CMD16, 512) != 0) // Set R/W block length to 512
					sdinfo.type = 0;
				}
		}
	else
		{
			return 1;
		}

	return 0;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SPIx_WriteRead(uint8_t byte)
{
  uint8_t vReceivedByte = 0;
  if(HAL_SPI_TransmitReceive(&hspi2, (uint8_t*) &byte, (uint8_t*) &vReceivedByte, 1, 0x1000) != HAL_OK)
  {
  	SD_Error_Handler();
  }
  return vReceivedByte;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
void SPI_SendByte(uint8_t byte)
{
  SPIx_WriteRead(byte);
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SPI_ReceiveByte(void)
{
  uint8_t byte = SPIx_WriteRead(0xFF);
  return byte;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
void SPI_Release(void)
{
  SPIx_WriteRead(0xFF);
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_WaitingForReadiness(void)
{
	uint8_t vResult;
	uint16_t vCount = 0;

	do {
				 vResult = SPI_ReceiveByte();
				 vCount++;
	} while ( (vResult != 0xFF) && (vCount < 0xFFFF) );

	if (vCount >= 0xFFFF) return ERROR;

	  return (vResult == 0xFF) ? OK: ERROR;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_Cmd(uint8_t cmd, uint32_t argument)
{
  uint8_t vByte, vResult;

	// ACMD is the command sequence of CMD55-CMD?
	if (cmd & 0x80)
	{
		cmd &= 0x7F;
		vResult = SD_SPI_Cmd(CMD55, 0);
		if (vResult > 1) return vResult;
	}

	// Select the card
	SD_DESELECT;
	SPI_ReceiveByte();
	SD_SELECT;
	SPI_ReceiveByte();

	// Send a command packet
	SPI_SendByte(cmd); // Start + Command index
	SPI_SendByte((uint8_t)(argument >> 24)); // Argument[31..24]
	SPI_SendByte((uint8_t)(argument >> 16)); // Argument[23..16]
	SPI_SendByte((uint8_t)(argument >> 8)); // Argument[15..8]
	SPI_SendByte((uint8_t)argument); // Argument[7..0]
	vByte = 0x01; // Dummy CRC + Stop

	if (cmd == CMD0) {vByte = 0x95;} // Valid CRC for CMD0(0)
	if (cmd == CMD8) {vByte = 0x87;} // Valid CRC for CMD8(0x1AA)
	SPI_SendByte(vByte);

  // Receive a command response
  vByte = 10; // Wait for a valid response in timeout of 10 attempts
  do {
    		vResult = SPI_ReceiveByte();
  } while ((vResult & 0x80) && --vByte);

  return vResult;

}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_Read_Block(uint8_t *buff, uint32_t lba)
{
  uint8_t vResult = 0;
  uint16_t vCounter = 0;

	vResult = SD_SPI_Cmd (CMD17, lba);
	if (vResult) return 5; //	Error

	SPI_Release();

  do{
				vResult=SPI_ReceiveByte();
				vCounter++;
  } while ((vResult != 0xFE) && (vCounter < 0xFFFF)); // Wait till mark(0xFE) is received
  if (vCounter >= 0xFFFF) return 5;	 //	 Error

  for (vCounter = 0; vCounter<512; vCounter++) buff[vCounter]=SPI_ReceiveByte(); // Write data to the buffer
  SPI_Release(); // Skip CRC
  SPI_Release();

  return 0;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_Write_Block (uint8_t *buff, uint32_t lba)
{
  uint8_t vResult;
  uint16_t vCounter;

  vResult = SD_SPI_Cmd(CMD24, lba);

  if(vResult != 0x00) return 6; // Error

  SPI_Release();
  SPI_SendByte (0xFE); // Send transmission start mark
  for (vCounter = 0; vCounter<512; vCounter++) SPI_SendByte(buff[vCounter]); // Write data to the SD
  SPI_Release();  // Skip CRC
  SPI_Release();
  vResult = SPI_ReceiveByte();
  if((vResult & 0x05) != 0x05) return 6; // Error  (datasheet p. 111)

  vCounter = 0;
  do {
				vResult=SPI_ReceiveByte();
				vCounter++;
  } while ( (vResult != 0xFF)&&(vCounter<0xFFFF) );		//Wait till BUSY mode is finished
  if (vCounter>=0xFFFF) return 6;		// Error

  return 0;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_ReadFile(void)
{
	if(f_mount(&SDFatFs, (TCHAR const*)USER_Path, 0))
		{
		SD_Error_Handler();
		}
		else
		{
			if(f_open(&MyFile, "STM32.txt", FA_READ))
			{
				SD_Error_Handler();
			}
			else
			{
				SD_SPI_ReadLongFile();
				f_close(&MyFile);
			}
		}
	return 0;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
FRESULT SD_SPI_ReadLongFile(void)
{
  uint16_t vTemp = 0;
  uint32_t vIndex = 0;
  uint32_t vFileSize = MyFile.obj.objsize;
  uint32_t vBytesReadCounter;

	do
	{
		if(vFileSize < 512)
		{
			vTemp = vFileSize;
		}
		else
		{
			vTemp = 512;
		}
		vFileSize -= vTemp;

		f_lseek(&MyFile, vIndex);
		f_read(&MyFile, aBuffer, vTemp, (UINT *)&vBytesReadCounter);

		vIndex += vTemp;
	}
	while(vFileSize > 0);

  	return FR_OK;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_WriteFile(void)
{
	FRESULT vResult;
	uint8_t aWriteBuffer[] = "Added text!";
	uint32_t vBytesWritteCounter;

	if(f_mount(&SDFatFs, (TCHAR const*)USER_Path, 0))
		{
			SD_Error_Handler();
		}
		else
		{
			if(f_open(&MyFile, "STM32Second.txt", FA_CREATE_ALWAYS | FA_WRITE))
			{
				SD_Error_Handler();
			}
			else
			{
				vResult = f_write(&MyFile, aWriteBuffer, sizeof(vBytesWritteCounter), (void*)&vBytesWritteCounter);
				if((vBytesWritteCounter == 0) || (vResult))
				{
					SD_Error_Handler();
				}
				f_close(&MyFile);
			}
		}
	return 0;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
uint8_t SD_SPI_GetFileInfo(void)
{
	uint8_t vResult;
	DWORD free_clusters, free_sectors, total_sectors;

	if(f_mount(&SDFatFs, (TCHAR const*)USER_Path, 0))
	{
		SD_Error_Handler();
	}
	else
	{
		vResult = f_opendir(&sDirectory, "/");		// "/" - directory name to open

		if (vResult == FR_OK)
		{
			while(1)
			{
				vResult = f_readdir(&sDirectory, &sFileInfo);

				if ((vResult == FR_OK) && (sFileInfo.fname[0]))
				{
					HAL_UART_Transmit(&huart2, (uint8_t*)sFileInfo.fname, strlen((char*)sFileInfo.fname), 0x1000);

					if(sFileInfo.fattrib & AM_DIR)
					{
						HAL_UART_Transmit(&huart2, (uint8_t*)"  [DIR]", 7, 0x1000);
					}
				}
				else break;

				HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 0x1000);
			}
		}
		f_closedir(&sDirectory);
	}

	f_getfree("/", &free_clusters, &fs);

	sprintf(aStringBuffer, "free_clusters: %lu\r\n", free_clusters);
	HAL_UART_Transmit(&huart2, (uint8_t*)aStringBuffer, strlen(aStringBuffer), 0x1000);

	sprintf(aStringBuffer,"n_fatent: %lu\r\n",fs->n_fatent);
	HAL_UART_Transmit(&huart2, (uint8_t*)aStringBuffer, strlen(aStringBuffer), 0x1000);

	sprintf(aStringBuffer,"fs_csize: %d\r\n",fs->csize);
	HAL_UART_Transmit(&huart2, (uint8_t*)aStringBuffer, strlen(aStringBuffer), 0x1000);

	total_sectors = (fs->n_fatent - 2) * fs->csize;
	sprintf(aStringBuffer, "total_sectors: %lu\r\n", total_sectors);
	HAL_UART_Transmit(&huart2, (uint8_t*)aStringBuffer, strlen(aStringBuffer), 0x1000);

	free_sectors = free_clusters * fs->csize;
	sprintf(aStringBuffer, "free_sectors: %lu\r\n", free_sectors);
	HAL_UART_Transmit(&huart2, (uint8_t*)aStringBuffer, strlen(aStringBuffer), 0x1000);

	sprintf(aStringBuffer, "%lu KB total drive space.\r\n%lu KB available.\r\n", (free_sectors / 2), (total_sectors / 2));
	HAL_UART_Transmit(&huart2, (uint8_t*)aStringBuffer, strlen(aStringBuffer), 0x1000);

	return 0;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
void SD_Error_Handler(void)
{
	LED_Red_ON;
	while(1);
}

/****************************************************************************************************************/
/****************************************************************************************************************/


