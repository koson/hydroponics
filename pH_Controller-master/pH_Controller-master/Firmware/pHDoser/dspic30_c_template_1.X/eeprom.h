#define ERASE_WORD 0x4044
#define WRITE_WORD 0x4004
#define ADDRESS_HI 0x007F
#define EEPROM_LOW_START_ADDRESS 0xFC00
#define TRUE 1

unsigned short Eeprom_ReadWord(unsigned short  pushAddressOffset) 
{
   unsigned short ushResult;
   register int eedata_addr; 
   register int eedata_val; 
  unsigned short pushAddress;
   pushAddressOffset = pushAddressOffset*2; //word offset , byte addressable so multiply by 2
   pushAddress = pushAddressOffset + EEPROM_LOW_START_ADDRESS;
    
   TBLPAG = ADDRESS_HI; // __builtin_tblpage()
   eedata_addr = (unsigned short)pushAddress; // __builtin_tbloffset()
   __asm__("TBLRDL [%[addr]], %[val]" : [val]"=r"(eedata_val) : [addr]"r"(eedata_addr)); 
   
   ushResult = eedata_val; 
   return (ushResult);
 }
 
void Eeprom_WriteWord(unsigned short  pushAddressOffset, unsigned short value) {
  unsigned short pushAddress;
   pushAddressOffset = pushAddressOffset*2; //word offset , byte addressable so multiply by 2
   pushAddress = pushAddressOffset + EEPROM_LOW_START_ADDRESS;
   TBLPAG = ADDRESS_HI;
   NVMADRU = ADDRESS_HI; // Write address of word to be erased into NVMADRU, NVMADR registers.
   NVMADR = (unsigned short) pushAddress;
 
   NVMCON = ERASE_WORD; // Setup NVMCON register to erase one EEPROM word.
    
   //PROTECT_CODE_FROM_INTERRUPTS_START // Disable interrupts while the KEY sequence is written
      NVMKEY = 0x55; // Write the KEY sequence step1
      NVMKEY = 0xAA; // step2
      NVMCONbits.WR = TRUE; // Start the erase cycle
   //PROTECT_CODE_FROM_INTERRUPTS_STOP // Enable interrupts
    
   while (NVMCONbits.WR == TRUE); // wait for the EEPROMS
    
   NVMCON = WRITE_WORD; // Setup NVMCON register to write one EEPROM word. 
   {
      register int eedata_addr; 
      register int eedata_val; 
       
      eedata_addr = (unsigned short)pushAddress; // write low word of address
      eedata_val = value; // write data
       
      __asm__ volatile ("TBLWTL %[val], [%[addr]]" : [val]"+r"(eedata_val) : [addr]"r"(eedata_addr));
   }
    
    NVMCON = WRITE_WORD;
   //PROTECT_CODE_FROM_INTERRUPTS_START // Disable interrupts while the KEY sequence is written
      NVMKEY = 0x55; // Write the KEY sequence step1
      NVMKEY = 0xAA; // step2
      NVMCONbits.WR = TRUE; // Start the erase cycle
   //PROTECT_CODE_FROM_INTERRUPTS_STOP // Enable interrupts
    
   while (NVMCONbits.WR == TRUE); // wait for the word to be written
    
   // no proper watchdog protection in the 2 while loops
   // no proper check if bytes are written correctly by re-reading them
 }
