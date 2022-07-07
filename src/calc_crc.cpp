#include <string.h>
#include <stdio.h>

/* buld upload string for habub 
    txLine is output srting for habhub
    rxline is input string received from tracker
    */

char Hex(int Character)
{
  char HexTable[] = "0123456789ABCDEF";
  
  return HexTable[Character];
}


// Tag 7 byte CRC plus null on end of targetString

int calcCRC(char *targetString)
{    

    int Count, i, j;
    unsigned int CRC;

    Count = strlen(targetString);

    CRC = 0xffff;           // Seed
   
     for (i = 0; i < Count; i++)
     {   // For speed, repeat calculation instead of looping for each bit
        CRC ^= (((unsigned int)targetString[i]) << 8);
        for (j=0; j<8; j++)
        {
            if (CRC & 0x8000)
                CRC = (CRC << 1) ^ 0x1021;
            else
                CRC <<= 1;
        }
     }

    targetString[Count++] = '*';
    targetString[Count++] = Hex((CRC >> 12) & 15);
    targetString[Count++] = Hex((CRC >> 8) & 15);
    targetString[Count++] = Hex((CRC >> 4) & 15);
    targetString[Count++] = Hex(CRC & 15);
  	targetString[Count++] = '\n';  
	targetString[Count++] = '\0';
	
	return strlen(targetString) + 1;
}


int BuildSentence(char *txLine,char * rxLine, int txLineMaxLen)
{
    
    // prepend $$ to output string
    int slen=0;
    int start = 0;
    // skip any $ symbols in raw string
    while(rxLine[start]=='$' && (start < 2)){
        start++;
    }
    
    if(start == 0){
        return 0;
    }

    snprintf(txLine,
            txLineMaxLen,
            "$$%s",&rxLine[start]);

    // make sure we can fit crc on end of string
//    if(txLineMaxLen > strlen(txLine) + 8 ){
        // crc all characters after the $$ at start of string
  //      slen = calcCRC(&txLine[2]);
  //  }
    return strlen(txLine);
    //return slen;
}

