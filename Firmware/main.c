//USB stack
#include "jtr\usb_stack_globals.h"    // USB stack only defines Not function related.
#include "descriptors.h"	// JTR Only included in main.c
#include "configwords.h"	// JTR only included in main.c

#define ROW1	PORTCbits.RC2 
#define ROW2	PORTCbits.RC1 
#define ROW3	PORTAbits.RA5
#define ROW4	PORTCbits.RC0 

#define COL1	PORTAbits.RA1
#define COL2	PORTAbits.RA0
#define COL3	PORTBbits.RB7
#define COL4	PORTBbits.RB6
#define COL5	PORTBbits.RB5
#define COL6	PORTBbits.RB4
#define COL7	PORTBbits.RB3
#define COL8	PORTBbits.RB2
#define COL9	PORTBbits.RB1
#define COL10	PORTBbits.RB0



extern BYTE usb_device_state;

void USBSuspend(void);
void SetupBoard(void);
void UpdateBar(void);

volatile unsigned int bar[4];

void UpdateBar() {
	static unsigned char row=0;
	unsigned int leds;

	// Turn off row driver FET
	if (row==0) ROW1=0;
	else if (row==1) ROW2=0;
	else if (row==2) ROW3=0;
	else if (row==3) ROW4=0;
	
	// Turn off all leds
	COL1=0;
	COL2=0;
	PORTB=0; // COL3..10

	// Select next row
	row=(row+1)&0x03;

	// Turn on the leds for the new row
	leds=bar[row];
	if (leds&0x001) PORTAbits.RA1=1;
	if (leds&0x002) PORTAbits.RA0=1;
	if (leds&0x004) PORTBbits.RB7=1;
	if (leds&0x008) PORTBbits.RB6=1;
	if (leds&0x010) PORTBbits.RB5=1;
	if (leds&0x020) PORTBbits.RB4=1;
	if (leds&0x040) PORTBbits.RB3=1;
	if (leds&0x080) PORTBbits.RB2=1;
	if (leds&0x100) PORTBbits.RB1=1;
	if (leds&0x200) PORTBbits.RB0=1;

	// Turn on row driver FET
	if (row==0) ROW1=1;
	else if (row==1) ROW2=1;
	else if (row==2) ROW3=1;
	else if (row==3) ROW4=1;
}


void main(void) {
	int dly;
	int col=0;
	unsigned char RecvdByte;
	

    SetupBoard(); //setup the hardware, customize for your hardware

    initCDC(); // setup the CDC state machine
    usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs, USB_NUM_STRINGS); // initialize USB. TODO: Remove magic with macro
    usb_start(); //start the USB peripheral

    EnableUsbPerifInterrupts(USB_TRN + USB_SOF + USB_UERR + USB_URST);
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    EnableUsbGlobalInterrupt(); // Only enables global USB interrupt. Chip interrupts must be enabled by the user (PIC18)
	
// Wait for USB to connect
	bar[0]=0x201;
	bar[1]=0;
	bar[2]=0;
	bar[3]=0x201;
    do {
		UpdateBar();
    } while (usb_device_state < CONFIGURED_STATE);
    usb_register_sof_handler(CDCFlushOnTimeout); // Register our CDC timeout handler after device configured
	bar[0]=0;
	bar[1]=0;
	bar[2]=0;
	bar[3]=0;

// Main loop
    for(;;) {
		dly=1000; while (dly--);
		UpdateBar();
		if (poll_getc_cdc(&RecvdByte)) {
			if (RecvdByte==255) col=0;
			else {
				bar[col]=0x55;
				if (RecvdByte==0) bar[col]=0;
				if (RecvdByte>0 && RecvdByte<=10) bar[col]=0x001;
				if (RecvdByte>10 && RecvdByte<=20) bar[col]=0x003;
				if (RecvdByte>20 && RecvdByte<=30) bar[col]=0x007;
				if (RecvdByte>30 && RecvdByte<=40) bar[col]=0x00F;
				if (RecvdByte>40 && RecvdByte<=50) bar[col]=0x01F;
				if (RecvdByte>50 && RecvdByte<=60) bar[col]=0x03F;
				if (RecvdByte>60 && RecvdByte<=70) bar[col]=0x07F;
				if (RecvdByte>70 && RecvdByte<=80) bar[col]=0x0FF;
				if (RecvdByte>80 && RecvdByte<=90) bar[col]=0x1FF;
				if (RecvdByte>90 && RecvdByte<=100) bar[col]=0x3FF;
				col++;
			}
		} 
	}	

}




void SetupBoard() {
	int dly;

	//all pins digital
    ANCON0 = 0xFF;                  
    ANCON1 = 0b00011111;// updated for lower power consumption. See datasheet page 343                  

	// All ports as outputs
	TRISA=0;
	TRISB=0;
	TRISC=0;

	// Set all pins low
	PORTA=0;
	PORTB=0;
	PORTC=0;

	OSCTUNEbits.PLLEN = 1;  //enable PLL
	while(dly--); //wait for lock
}


void USBSuspend(void){}


//PIC18F style interrupts with remapping for bootloader
//	Interrupt remap chain
//
//This function directs the interrupt to
// the proper function depending on the mode
// set in the mode variable.
//USB stack on low priority interrupts,

#define REMAPPED_RESET_VECTOR_ADDRESS			0x800
#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x808
#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x818

#pragma interruptlow InterruptHandlerLow nosave= PROD, PCLATH, PCLATU, TBLPTR, TBLPTRU, TABLAT, section (".tmpdata"), section("MATH_DATA")
void InterruptHandlerLow(void) {
    usb_handler();
    ClearGlobalUsbInterruptFlag();
}

#pragma interrupt InterruptHandlerHigh nosave= PROD, PCLATH, PCLATU, TBLPTR, TBLPTRU, TABLAT, section (".tmpdata"), section("MATH_DATA")
void InterruptHandlerHigh(void) { //Also legacy mode interrupt.
	usb_handler();
    ClearGlobalUsbInterruptFlag();
}

//these statements remap the vector to our function
//When the interrupt fires the PIC checks here for directions
#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS

void Remapped_High_ISR(void) {
    _asm goto InterruptHandlerHigh _endasm
}

#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
void Remapped_Low_ISR(void) {
    _asm goto InterruptHandlerLow _endasm
}

//relocate the reset vector
extern void _startup(void);

#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS

void _reset(void) {
    _asm goto _startup _endasm
}


//set the initial vectors so this works without the bootloader too.
#pragma code HIGH_INTERRUPT_VECTOR = 0x08
void High_ISR(void) {
    _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
}

#pragma code LOW_INTERRUPT_VECTOR = 0x18
void Low_ISR(void) {
    _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
}
