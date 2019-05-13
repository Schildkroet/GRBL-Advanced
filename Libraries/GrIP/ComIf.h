/*
  ComIf.h - Communication Interface

  Copyright (c) 2018-2019 Patrick F.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef COMMUNICATIONINTERFACE_H_INCLUDED
#define COMMUNICATIONINTERFACE_H_INCLUDED


#include <stdint.h>


#define IF_USB          0
#define IF_ETH          1


#ifdef __cplusplus
extern "C" {
#endif



/** \brief Initialize communication interface.
 *
 * \param sock Socket number of TCP server.
 * \return None.
 *
 */
void ComIf_Init(uint8_t interface, uint8_t sock);

/** \brief Deinitalize communication interface.
 *
 * \return None.
 *
 */
void ComIf_DeInit(void);


/** \brief Transmit data.
 *
 * \param data Data array to transmit.
 * \param len Length of data array in bytes.
 * \return 0 if success.
 *
 */
uint8_t ComIf_Send(uint8_t *data, uint16_t len);

/** \brief Get data from communication interface.
 *
 * \param data Pointer where to store the data.
 * \param len Max number of bytes to read.
 * \return Number of read bytes. 0 if no bytes were read.
 *
 */
uint16_t ComIf_Receive(uint8_t *data, uint16_t len);

/** \brief Return if data is available.
 *
 * \return Number of bytes available.
 *
 */
uint16_t ComIf_DataAvailable(void);

/** \brief Cyclic update function. Gets data from hardware interface and stores it in internal buffer. Should be called periodically.
 *
 * \return None.
 *
 */
void ComIf_Update(void);


#ifdef __cplusplus
}
#endif


#endif // COMMUNICATIONINTERFACE_H_INCLUDED
