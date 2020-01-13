/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#include "fxemu.h"
#include "fxinst.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* The FxChip Emulator's internal variables */
struct FxRegs_s GSU = FxRegs_s_null;

void FxCacheWriteAccess(uint16 vAddress)
{
    if((vAddress & 0x00f) == 0x00f)
	GSU.vCacheFlags |= 1 << ((vAddress&0x1f0) >> 4);
}

void FxFlushCache()
{
    GSU.vCacheFlags = 0;
    GSU.vCacheBaseReg = 0;
    GSU.bCacheActive = FALSE;
}

void fx_flushCache()
{
    GSU.vCacheFlags = 0;
    GSU.bCacheActive = FALSE;
}

void fx_updateRamBank(uint8 Byte)
{
	// Update BankReg and Bank pointer
    GSU.vRamBankReg = (uint32)Byte & (FX_RAM_BANKS-1);
    GSU.pvRamBank = GSU.apvRamBank[Byte & 0x3];
}

static void fx_readRegisterSpace()
{
    int i;
    uint8 *p;
    static uint32 avHeight[] = { 128, 160, 192, 256 };
    static uint32 avMult[] = { 16, 32, 32, 64 };

    /* Update R0-R15 */
    p = GSU.pvRegisters;
    for(i=0; i<16; i++)
    {
	GSU.avReg[i] = *p++;
	GSU.avReg[i] += ((uint32)(*p++)) << 8;
    }

    /* Update other registers */
    p = GSU.pvRegisters;
    GSU.vStatusReg = (uint32)p[GSU_SFR];
    GSU.vStatusReg |= ((uint32)p[GSU_SFR+1]) << 8;
    GSU.vPrgBankReg = (uint32)p[GSU_PBR];
    GSU.vRomBankReg = (uint32)p[GSU_ROMBR];
    GSU.vRamBankReg = ((uint32)p[GSU_RAMBR]) & (FX_RAM_BANKS-1);
    GSU.vCacheBaseReg = (uint32)p[GSU_CBR];
    GSU.vCacheBaseReg |= ((uint32)p[GSU_CBR+1]) << 8;

    /* Update status register variables */
    GSU.vZero = !(GSU.vStatusReg & FLG_Z);
    GSU.vSign = (GSU.vStatusReg & FLG_S) << 12;
    GSU.vOverflow = (GSU.vStatusReg & FLG_OV) << 16;
    GSU.vCarry = (GSU.vStatusReg & FLG_CY) >> 2;
    
    /* Set bank pointers */
    GSU.pvRamBank = GSU.apvRamBank[GSU.vRamBankReg & 0x3];
    GSU.pvRomBank = GSU.apvRomBank[GSU.vRomBankReg];
    GSU.pvPrgBank = GSU.apvRomBank[GSU.vPrgBankReg];

    /* Set screen pointers */
    GSU.pvScreenBase = &GSU.pvRam[ USEX8(p[GSU_SCBR]) << 10 ];
    i = (int)(!!(p[GSU_SCMR] & 0x04));
    i |= ((int)(!!(p[GSU_SCMR] & 0x20))) << 1;
    GSU.vScreenHeight = GSU.vScreenRealHeight = avHeight[i];
    GSU.vMode = p[GSU_SCMR] & 0x03;
    if(i == 3)
      GSU.vScreenSize = 32768;
    else
      GSU.vScreenSize = GSU.vScreenHeight * 4 * avMult[GSU.vMode];
   if (GSU.vPlotOptionReg & 0x10)
      GSU.vScreenHeight = 256; /* OBJ Mode (for drawing into sprites) */
   if (GSU.pvScreenBase + GSU.vScreenSize > GSU.pvRam + (GSU.nRamBanks * 65536))
      GSU.pvScreenBase =  GSU.pvRam + (GSU.nRamBanks * 65536) - GSU.vScreenSize;
    
    fx_apfOpcodeTable[0x24c] = fx_apfOpcodeTable[0x04c] = fx_apfPlotTable[GSU.vMode];
    fx_apfOpcodeTable[0x14c] = fx_apfOpcodeTable[0x34c] = fx_apfPlotTable[GSU.vMode + 5];
    
    if(GSU.vMode != GSU.vPrevMode || GSU.vPrevScreenHeight != GSU.vScreenHeight || GSU.vSCBRDirty)
       fx_computeScreenPointers ();
}

void fx_dirtySCBR()
{
	GSU.vSCBRDirty = TRUE;
}

void fx_computeScreenPointers ()
    {
	int i, j, condition, mask, result;
	uint apvIncrement, vMode, xIncrement;
	GSU.vSCBRDirty = FALSE;

  /* Make a list of pointers to the start of each screen column*/
   vMode = GSU.vMode;
   condition = vMode - 2;
   mask = (condition | -condition) >> 31;
   result = (vMode & mask) | (3 & ~mask);
   vMode = result + 1;
   GSU.x[0] = 0;
   GSU.apvScreen[0] = GSU.pvScreenBase;
   apvIncrement = vMode << 4;

   if(GSU.vScreenHeight == 256)
   {
      GSU.x[16] = vMode << 12;
      GSU.apvScreen[16] = GSU.pvScreenBase + (vMode << 13);
      apvIncrement <<= 4;
      xIncrement = vMode << 4;

      for(i = 1, j = 17 ; i < 16 ; i++, j++)
      {
         GSU.x[i] = GSU.x[i - 1] + xIncrement;
         GSU.apvScreen[i] = GSU.apvScreen[i - 1] + apvIncrement;
         GSU.x[j] = GSU.x[j - 1] + xIncrement;
         GSU.apvScreen[j] = GSU.apvScreen[j - 1] + apvIncrement;
      }
   }
   else
   {
      xIncrement = (vMode * GSU.vScreenHeight) << 1;
      for(i = 1 ; i < 32 ; i++)
      {
         GSU.x[i] = GSU.x[i - 1] + xIncrement;
         GSU.apvScreen[i] = GSU.apvScreen[i - 1] + apvIncrement;
           }
	}
	GSU.vPrevMode = GSU.vMode;
	GSU.vPrevScreenHeight = GSU.vScreenHeight;
    }

static void fx_writeRegisterSpace()
{
    int i;
    uint8 *p;
    
    p = GSU.pvRegisters;
    for(i=0; i<16; i++)
    {
	*p++ = (uint8)GSU.avReg[i];
	*p++ = (uint8)(GSU.avReg[i] >> 8);
    }

    /* Update status register */
    if( USEX16(GSU.vZero) == 0 ) SF(Z);
    else CF(Z);
    if( GSU.vSign & 0x8000 ) SF(S);
    else CF(S);
    if(GSU.vOverflow >= 0x8000 || GSU.vOverflow < -0x8000) SF(OV);
    else CF(OV);
    if(GSU.vCarry) SF(CY);
    else CF(CY);
    
    p = GSU.pvRegisters;
    p[GSU_SFR] = (uint8)GSU.vStatusReg;
    p[GSU_SFR+1] = (uint8)(GSU.vStatusReg>>8);
    p[GSU_PBR] = (uint8)GSU.vPrgBankReg;
    p[GSU_ROMBR] = (uint8)GSU.vRomBankReg;
    p[GSU_RAMBR] = (uint8)GSU.vRamBankReg;
    p[GSU_CBR] = (uint8)GSU.vCacheBaseReg;
    p[GSU_CBR+1] = (uint8)(GSU.vCacheBaseReg>>8);
}

/* Reset the FxChip */
void FxReset(struct FxInit_s *psFxInfo)
{
    int i;
    
    /* Clear all internal variables */
    memset((uint8*)&GSU,0,sizeof(struct FxRegs_s));

    /* Set default registers */
    GSU.pvSreg = GSU.pvDreg = &R0;

    /* Set RAM and ROM pointers */
    GSU.pvRegisters = psFxInfo->pvRegisters;
    GSU.nRamBanks = psFxInfo->nRamBanks;
    GSU.pvRam = psFxInfo->pvRam;
    GSU.nRomBanks = psFxInfo->nRomBanks;
    GSU.pvRom = psFxInfo->pvRom;
    GSU.vPrevScreenHeight = ~0;
    GSU.vPrevMode = ~0;

    /* The GSU can't access more than 2mb (16mbits) */
    if(GSU.nRomBanks > 0x20)
	GSU.nRomBanks = 0x20;
    
    /* Clear FxChip register space */
    memset(GSU.pvRegisters,0,0x300);

    /* Set FxChip version Number */
    GSU.pvRegisters[0x3b] = 0;

    /* Make ROM bank table */
    for(i=0; i<256; i++)
    {
	uint32 b = i & 0x7f;
	if (b >= 0x40)
	{
	    if (GSU.nRomBanks > 1)
		b %= GSU.nRomBanks;
	    else
		b &= 1;

	    GSU.apvRomBank[i] = &GSU.pvRom[ b << 16 ];
	}
	else
	{
	    b %= GSU.nRomBanks * 2;
	    GSU.apvRomBank[i] = &GSU.pvRom[ (b << 16) + 0x200000];
	}
    }

    /* Make RAM bank table */
    for(i=0; i<4; i++)
    {
	GSU.apvRamBank[i] = &GSU.pvRam[(i % GSU.nRamBanks) << 16];
	GSU.apvRomBank[0x70 + i] = GSU.apvRamBank[i];
    }
    
    /* Start with a nop in the pipe */
    GSU.vPipe = 0x01;


    fx_readRegisterSpace();
}

static uint8 fx_checkStartAddress()
{
    /* Check if we start inside the cache */
    if(GSU.bCacheActive && R15 >= GSU.vCacheBaseReg && R15 < (GSU.vCacheBaseReg+512))
	return TRUE;
   
    /*  Check if we're in an unused area */
    if(GSU.vPrgBankReg < 0x40 && R15 < 0x8000)
	return FALSE;
    if(GSU.vPrgBankReg >= 0x60 && GSU.vPrgBankReg <= 0x6f)
	return FALSE;
    if(GSU.vPrgBankReg >= 0x74)
	return FALSE;

    /* Check if we're in RAM and the RAN flag is not set */
    if(GSU.vPrgBankReg >= 0x70 && GSU.vPrgBankReg <= 0x73 && !(SCMR&(1<<3)) )
	return FALSE;

    /* If not, we're in ROM, so check if the RON flag is set */
    if(!(SCMR&(1<<4)))
	return FALSE;
    
    return TRUE;
}

/* Execute until the next stop instruction */
void FxEmulate(uint32 nInstructions)
{
    /* Read registers and initialize GSU session */
    fx_readRegisterSpace();

    /* Check if the start address is valid */
    if(!fx_checkStartAddress())
    {
	CF(G);
	fx_writeRegisterSpace();
	return;
    }

    /* Execute GSU session */
    CF(IRQ);

    fx_run(nInstructions);

    /* Store GSU registers */
    fx_writeRegisterSpace();
}
