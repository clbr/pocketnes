#include "includes.h"

#define POGO_FILEHEAD (*(u8**)0x0203fbfc)
#define POGO_FILETAIL (*(u8**)0x0203fbf8)

/*
#include <stdio.h>
#include <string.h>
#include "gba.h"
#include "minilzo.107/minilzo.h"
#include "cheat.h"
#include "asmcalls.h"
#include "main.h"
#include "ui.h"
#include "sram.h"
*/

extern romheader mb_header;

//const unsigned __fp_status_arm=0x40070000;
EWRAM_BSS u8 *textstart;//points to first NES rom (initialized by boot.s)
EWRAM_BSS u8 *ewram_start;
EWRAM_BSS u8 *end_of_exram;
EWRAM_BSS u32 max_multiboot_size;
EWRAM_BSS u32 oldinput;

EWRAM_BSS char pogoshell_romname[32];	//keep track of rom name (for state saving, etc)
EWRAM_BSS u32 pogoshell_filesize;
#if RTCSUPPORT
EWRAM_BSS char rtc=0;
#endif
EWRAM_BSS char pogoshell=0;
EWRAM_BSS char gameboyplayer=0;
EWRAM_BSS char gbaversion;
const int ne=0x454e;

#if SAVE
EWRAM_BSS extern u8* BUFFER1;
EWRAM_BSS extern u8* BUFFER2;
EWRAM_BSS extern u8* BUFFER3;
#endif

#if GCC
EWRAM_BSS u32 copiedfromrom=0;

int strstr_(const char *str1, const char *str2);

APPEND int main()
{
	//set text_start (before moving the rom)
	extern u8 __rom_end__[];
	extern u8 __eheap_start[];
	u32 end_addr = (u32)(&__rom_end__);
	textstart = (u8*)(end_addr);
	u32 heap_addr = (u32)(&__eheap_start);
	ewram_start = (u8*)heap_addr;
	
	extern u8 MULTIBOOT_LIMIT[];
	u32 multiboot_limit = (u32)(&MULTIBOOT_LIMIT);
	max_multiboot_size = multiboot_limit - heap_addr;
	
	if (end_addr < 0x08000000 && copiedfromrom)
	{
		textstart += (0x08000000 - 0x02000000);

#if COMPY
		//simulate full multiboot for debugging  (DELETE LATER)
		memcpy32((u8*)end_addr, textstart, 0x02040000 - end_addr);
		textstart = (u8*)end_addr;
		copiedfromrom = 0;
#endif	

	}
	C_entry();
	return 0;
}

#endif

APPEND void load_vram_code()
{
	//load VRAM CODE
	extern u8 __vram1_start[], __vram1_lma[], __vram1_end[];
	u32 vram1_size = ((((u8*)__vram1_end - (u8*)__vram1_start) - 1) | 3) + 1;
	memcpy32((u32*)__vram1_start,(const u32*)__vram1_lma,vram1_size);
}

void dummy();

APPEND void C_entry()
{
	int i;
	u32 temp;
	
#if RTCSUPPORT
	vu16 *timeregs=(u16*)0x080000c8;
	*timeregs=1;
	if(*timeregs & 1) rtc=1;
#endif
	
#if !COMPY
	temp=(u32)POGO_FILEHEAD;
	pogoshell=((temp & 0xFE000000) == 0x08000000)?1:0;
	if (pogoshell)
	{
		u32 tail = (u32)POGO_FILETAIL;
		pogoshell_filesize = tail - temp;
	}
#else
	pogoshell = 0;
	pogoshell_filesize = 0;
#endif
	gbaversion=CheckGBAVersion();

#if SAVE
	BUFFER1 = ewram_start;
	BUFFER2 = BUFFER1+0x10000;
	BUFFER3 = BUFFER2+0x20000;
#endif

#if !COMPY
	if(pogoshell)
	{
		//find the filename
		volatile char *s=(volatile char*)0x0203fc08;
		//0203FC08 contains argv[0], 00, then argv[1].
		//advance past first null
		while (*s++ != 0);
		//advance past second null
		while (*s++ != 0);
		//return to slash or null
		s-=2;
		while (*s != '/' && *s != 0)
		{
			s--;
		}
		s++;
		//copy rom name
		bytecopy(pogoshell_romname,s,32);
		
		//check for PAL-like filename
		if(strstr_(s,"(E)") || strstr_(s,"(e)"))		//Check if it's a European rom.
			emuflags |= PALTIMING;
		else
			emuflags &= ~PALTIMING;
		
		//set ROM address
		textstart=POGO_FILEHEAD-sizeof(romheader);
		
		//So it will try to detect roms when loading state
		roms=1;
		
#if MULTIBOOT
		memcpy(mb_header.name,pogoshell_romname,32);
#endif
	}
#endif
	
	bool wantToSplash = false;
	const u16* splashImage = NULL;
	
#if ROMMENU
	if (!pogoshell)
	{
		u8 *p;
		u32 nes_id=0x1a530000+ne;
		if (find_nes_header(textstart)==NULL)
		{
			wantToSplash = true;
			splashImage = (const u16*)textstart;
			textstart+=76800;
		}

		i=0;
		p=find_nes_header(textstart);
		while(p && *(u32*)(p+48)==nes_id)
		{
			//count roms
			p+=*(u32*)(p+32)+48;
			p=find_nes_header(p);
			i++;
		}
		roms=i;
	}
#else
	roms = 1;
#endif
	
	scaling = 3;  //default mode is scaled with spirtes
	
#if !GCC
	ewram_start = (u8*)	&Image$$RO$$Limit;
	if (ewram_start>=(u8*)0x08000000)
	{
		ewram_start=(u8*)0x02000000;
	}
#endif
	end_of_exram = (u8*)&END_OF_EXRAM;
	
	okay_to_run_hdma = 0;
	//Set interrupt handlers to dummy function, and set IRQ vector
	irqInit();
	//key,vblank,timer1,timer2,timer3,serial interrupt enable
	irqEnable(IRQ_KEYPAD | IRQ_VBLANK | IRQ_TIMER1 | IRQ_TIMER2 | IRQ_TIMER3 | IRQ_SERIAL);
	//Disable vcount and hblank interrupts in DISPSTAT, but enable them in REG_IE
	REG_DISPSTAT &= ~(LCDC_HBL | LCDC_VCNT);
	REG_IE |= IRQ_VCOUNT | IRQ_HBLANK;
	//Warning: VRAM code must be loaded before interrupt handlers are installed, so we install them happens after we load the VRAM overlay.
	
	if (!wantToSplash)
	{
		pocketnes_fade_from_white();
	}
	
	#if !COMPY
	memset32((u32*)0x6000000,0,0x18000);  //clear vram (fixes graphics junk)
	#endif
	//Warning: VRAM code must be loaded at some point
	
	//load font+palette
	loadfont();
	loadfontpal();
	ui_x=0x100;
#if ROMMENU
	move_ui();
#endif
//	REG_BG2HOFS=0x0100;		//Screen left
	REG_BG2CNT=0x0400;	//16color 512x256 CHRbase0 SCRbase6 Priority0
	
	extern void init_speed_hacks();
	init_speed_hacks();
	
	//PPU_init();
	build_chr_decode();
	#if CRASH
	crash_disabled = 1;
	#endif
	
#if ROMMENU
	if (roms == 0)
	{
		#if !COMPY
		get_ready_to_display_text();
		cls(3);
		ui_x=0;
		move_ui();
		drawtext(0,"No ROMS found!",0);
		drawtext(1,"Use PocketNES Menu Maker",0);
		drawtext(2,"to build a compilation ROM,",0);
		drawtext(3,"or use Pogoshell with a",0);
		drawtext(4,"supported flash cartridge.",0);
		#endif
		while (1)
		{
			waitframe();
		}
	}
	if (splashImage != NULL)
	{
		splash(splashImage);
		memset32((u32*)0x6000000,0,0x18000);  //clear vram (fixes graphics junk)
		get_ready_to_display_text();
		loadfont();
		ui_x=0;
		move_ui();
	}
#else
	roms = 1;
#endif
//	REG_WININ=0xFFFF;
//	REG_WINOUT=0xFFFB;
//	REG_WIN0H=0xFF;
//	REG_WIN0V=0xFF;
	
	//Must Load VRAM code before installing interrupt handlers
	load_vram_code();
	spriteinit();
	stop_dma_interrupts();
	
	irqSet(IRQ_VCOUNT,vcountinterrupt);
	irqSet(IRQ_HBLANK,hblankinterrupt);
	irqSet(IRQ_VBLANK,vblankinterrupt);
	irqSet(IRQ_TIMER1,timer1interrupt);
	irqSet(IRQ_TIMER2,timer_2_interrupt);
	irqSet(IRQ_TIMER3,timer_3_interrupt);
	irqSet(IRQ_SERIAL,serialinterrupt);  //this handler is not actually in vram, but call it here anyway	
	
	#if COMPY
		build_byte_reverse_table();
	#endif
	
	#if SAVE
		lzo_init();	//init compression lib for savestates
	#endif
	
//	LZ77UnCompVram(&font,(u16*)0x6002400);
//	memcpy((void*)0x5000080,&fontpal,64);
	
	
	#if SAVE
		readconfig();
	#endif
	jump_to_rommenu();
}

void jump_to_rommenu()
{
#if GCC
	extern u8 __sp_usr[];
	u32 newstack=(u32)(&__sp_usr);
	__asm__ volatile ("mov sp,%0": :"r"(newstack));
#else
	__asm {mov r0,#0x3007f00}		//stack reset
	__asm {mov sp,r0}
#endif
	rommenu();
	run(1);
	while (true);
}

void pocketnes_fade_from_white()
{
	//Do the fade before anything else so we can fade to black from Pogoshell's screen.  Doesn't work in newest pogoshell anymore.
	if(REG_DISPCNT==FORCE_BLANK)	//is screen OFF?
		REG_DISPCNT=0;				//screen ON
	*MEM_PALETTE=0x7FFF;			//white background
	REG_BLDCNT=0x00ff;				//brightness decrease all
	for(int i=0;i<17;i++) {
		REG_BLDY=i;					//fade to black
		waitframe();
	}
	*MEM_PALETTE=0;					//black background (avoids blue flash when doing multiboot)
	REG_DISPCNT=0;					//screen ON, MODE0
}

//show splash screen
APPEND void splash(const u16 *image)
{
	int i;
	u32 lastJoypad;
	u32 currentJoypad;
	u32 keysHit;

	REG_DISPCNT=FORCE_BLANK;	//screen OFF
	*MEM_PALETTE=0x7FFF;			//white background
	memcpy32((u32*)MEM_VRAM,(const u32*)image,240*160*2);
	waitframe();
//	REG_BG2CNT=0x0000;
	REG_DISPCNT=BG2_EN|MODE3;
	for(i=16;i>=0;i--) {	//fade from white
		setbrightnessall(i);
		waitframe();
	}
	currentJoypad = REG_P1;
	for(i=0;i<150;i++) {	//wait 2.5 seconds
		waitframe();
		lastJoypad = currentJoypad;
		currentJoypad = REG_P1;
		if (currentJoypad==0x030f)
		{
			gameboyplayer=1;
			gbaversion=3;
		}
		else
		{
			keysHit = (~currentJoypad) & (lastJoypad ^ currentJoypad);
			if (keysHit & 0x30F)
			{
				i = 150;
			}
		}

	}
}

#if COMPY
APPEND void build_byte_reverse_table()
{
	extern const u8 byte_reverse_table_init[256];
	extern u8 byte_reverse_table[256];

	//u8 *byte_reverse_table = 0x02000100;
	memcpy32(byte_reverse_table, byte_reverse_table_init, 0x100);
}

#endif

//0x02000000


int strlen_(const char *str)
{
	int len = 0;
	for (;;)
	{
		if (*str == 0) return len;
		str++;
		len++;
	}
}

//newlib's version is way too big (over 1k vs 32 bytes!)
int strstr_(const char *str1, const char *str2)
{
	int len1 = strlen_(str1);
	int len2 = strlen_(str2);
	int l = len1 - len2;
	for (int i=0; i < l; i++)
	{
		int j;
		for (j = 0; j < len2; j++)
		{
			if (str1[i + j] != str2[j])
			{
				break;
			}
		}
		if (j == len2)
		{
			return i;
		}
	}
	return 0;
}
