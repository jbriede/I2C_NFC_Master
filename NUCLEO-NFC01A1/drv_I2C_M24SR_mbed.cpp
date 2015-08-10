/**
  ******************************************************************************
  * @file    drv_I2C_M24SR_mbed.cpp
  * @author  MMY Application Team
  * @version V1.0.0
  * @date    19-March-2014
  * @brief   This file provides a set of functions needed to manage the I2C of
	*				   the M24SR device.
 ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MMY-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "drv_I2C_M24SR.h"
#include "mbed.h"

#define M24SR_SDA_PIN                                                       D14
#define M24SR_SCL_PIN                                                       D15
#define M24SR_GPO_PIN                                                       D12
#define M24SR_RFDIS_PIN                                                     D11

#if defined	__MBED__					  /* F4 */
	#define M24SR_I2C_SPEED_10														10000
	#define M24SR_I2C_SPEED_100														100000
	#define M24SR_I2C_SPEED_400														400000
	#define M24SR_I2C_SPEED_1000												M24SR_I2C_SPEED_400 // 1MHz I2C not supported by mbed
#else
	#error "Use only for mbed"
#endif
	
#define M24SR_I2C_SPEED				M24SR_I2C_SPEED_400

/* MBED objects */
I2C i2c(I2C_SDA, I2C_SCL);
DigitalOut pRFD(M24SR_RFDIS_PIN);
DigitalIn pGPO(M24SR_GPO_PIN);

static uint8_t						uSynchroMode = M24SR_WAITINGTIME_POLLING;

/**
  * @brief  This function initializes the M24SR_I2C interface
	* @retval None  
  */
void M24SR_I2CInit(void)
{
	i2c.frequency(M24SR_I2C_SPEED);
	pRFD.write(0);
	pGPO.mode(PullNone);
}

/**
  * @brief  this functions configure I2C synchronization mode
	* @param  mode : 
  * @retval None
  */
void M24SR_SetI2CSynchroMode(uint8_t mode)
{
#if defined (I2C_GPO_SYNCHRO_ALLOWED)
	uSynchroMode = mode;
#else
	if(mode == M24SR_WAITINGTIME_GPO || mode == M24SR_INTERRUPT_GPO)
		uSynchroMode = M24SR_WAITINGTIME_POLLING;
	else
		uSynchroMode = mode;
#endif /*  I2C_GPO_SYNCHRO_ALLOWED */
}

/**
  * @brief  This functions polls the I2C interface
  * @retval M24SR_STATUS_SUCCESS : the function is succesful
	* @retval M24SR_ERROR_I2CTIMEOUT : The I2C timeout occured. 
  */
int8_t M24SR_PollI2C(void)
{
	int status = 1;
	uint32_t nbTry = 0;
	char buffer;
	// 
	wait_ms(M24SR_POLL_DELAY);
	while (status != 0 && nbTry < M24SR_I2C_POLLING)
	{
		status = i2c.read(M24SR_ADDR, &buffer, 1, false);
		nbTry++;
	}
	if (status == 0)
		return M24SR_STATUS_SUCCESS;
	else
	{
		return M24SR_ERROR_I2CTIMEOUT;
	}
}

/**
  * @brief  This functions reads a response of the M24SR device
	* @param  NbByte : Number of byte to read (shall be >= 5)
  * @param  pBuffer : Pointer to the buffer to send to the M24SR
  * @retval M24SR_STATUS_SUCCESS : The function is succesful
	* @retval M24SR_ERROR_I2CTIMEOUT : The I2C timeout occured. 
  */
int8_t M24SR_ReceiveI2Cresponse(uint8_t NbByte, uint8_t *pBuffer)
{
	if (i2c.read(M24SR_ADDR, (char*)pBuffer, NbByte, false) == 0)
		return M24SR_STATUS_SUCCESS;
	else
		return M24SR_ERROR_I2CTIMEOUT;
}

/**
  * @brief  This functions sends the command buffer 
	* @param  NbByte : Number of byte to send
  * @param  pBuffer : pointer to the buffer to send to the M24SR
  * @retval M24SR_STATUS_SUCCESS : the function is succesful
	* @retval M24SR_ERROR_I2CTIMEOUT : The I2C timeout occured. 
  */
int8_t M24SR_SendI2Ccommand(uint8_t NbByte ,uint8_t *pBuffer)
{
	if (i2c.write(M24SR_ADDR, (char*)pBuffer, NbByte, false) == 0)
	{
		return M24SR_STATUS_SUCCESS;
	}
	else
	{
		return M24SR_ERROR_I2CTIMEOUT;
	}
}

/**
  * @brief  This functions returns M24SR_STATUS_SUCCESS when a response is ready
  * @retval M24SR_STATUS_SUCCESS : a response of the M24LR is ready
	* @retval M24SR_ERROR_DEFAULT : the response of the M24LR is not ready
  */
int8_t M24SR_IsAnswerReady(void)
{
	uint16_t status ;
	uint32_t retry = 0xFFFFF;
	uint8_t stable = 0;
	
	switch (uSynchroMode)
	{
			case M24SR_WAITINGTIME_POLLING :
				errchk(M24SR_PollI2C ( ));
				return M24SR_STATUS_SUCCESS;
			
			case M24SR_WAITINGTIME_TIMEOUT :
				// M24SR FWI=5 => (256*16/fc)*2^5=9.6ms but M24SR ask for extended time to program up to 246Bytes.
				// can be improved by 
				wait_ms(80);	
				return M24SR_STATUS_SUCCESS;

			case M24SR_WAITINGTIME_GPO :
				do
				{
					if(pGPO.read() == 0)
					{
						stable ++;
					}
					retry --;						
				}
				while(stable <5 && retry>0);
				if(!retry)
					goto Error;				
				return M24SR_STATUS_SUCCESS;
				
			default : 
				return M24SR_ERROR_DEFAULT;
		}

Error :
		return M24SR_ERROR_DEFAULT;
}

/**
  * @brief  This function enable or disable RF communication
	* @param	OnOffChoice: GPO configuration to set
  */
void M24SR_RFConfig_Hard(uint8_t OnOffChoice)
{
	/* Disable RF */
	if ( OnOffChoice != 0 )
	{	
		pRFD = 0;
	}
	else
	{	
		pRFD = 1;
	}
}

