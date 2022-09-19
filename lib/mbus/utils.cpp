/***********************************************************************************
    Filename: utils.cpp
***********************************************************************************/

#include "utils.hpp"


//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
//  void dumpHex(uint8_t* data, uint8_t len, bool newLine) 
//
//  DESCRIPTION:
//      Print data buffer in HEX 
//
//  ARGUMENTS: 
//      uint8_t* data  - Data to perform the CRC-16 operation on.
//      uint8_t len    - Length to print
//      bool newLine   - Should add new line at the end
//-------------------------------------------------------------------------------------------------------
void dumpHex(uint8_t* data, uint8_t len, char *buff, bool revert) {
  bool newLine = true;
  char buffHex[3];
  for (uint8_t i = 0; i < len; i++) {
    uint8_t j = i;
    if (revert) 
      j = len-i;
    sprintf(buffHex, "%02X", data[j]);
     strcpy(buff, buffHex);
     buff += 2;
  }
}
