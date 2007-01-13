#ifndef _LC89510_H
#define _LC89510_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	unsigned char Buffer[(32 * 1024 * 2) + 2352];
//	unsigned int Host_Data;		// unused
//	unsigned int DMA_Adr;		// 0A
//	unsigned int Stop_Watch;	// 0C
	unsigned int COMIN;
	unsigned int IFSTAT;
	union
	{
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} DBC;
	union
	{
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} DAC;
	union
	{
		struct
		{
			unsigned char B0;
			unsigned char B1;
			unsigned char B2;
			unsigned char B3;
		} B;
		unsigned int N;
	} HEAD;
	union
	{
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} PT;
	union
	{
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} WA;
	union
	{
		struct
		{
			unsigned char B0;
			unsigned char B1;
			unsigned char B2;
			unsigned char B3;
		} B;
		unsigned int N;
	} STAT;
	unsigned int SBOUT;
	unsigned int IFCTRL;
	union
	{
		struct
		{
			unsigned char B0;
			unsigned char B1;
			unsigned char B2;
			unsigned char B3;
		} B;
		unsigned int N;
	} CTRL;
} CDC;

typedef struct
{
//	unsigned short Fader;	// 34
//	unsigned short Control;	// 36
//	unsigned short Cur_Comm;// unused

	// "Receive status"
	unsigned short Status;
	unsigned short Minute;
	unsigned short Seconde;
	unsigned short Frame;
	unsigned char  Ext;
} CDD;


extern int CDC_Decode_Reg_Read;


void LC89510_Reset(void);
unsigned short Read_CDC_Host(int is_sub);
void Update_CDC_TRansfer(int which);
void CDC_Update_Header(void);

unsigned char CDC_Read_Reg(void);
void CDC_Write_Reg(unsigned char Data);

void CDD_Export_Status(void);
void CDD_Import_Command(void);

#ifdef __cplusplus
};
#endif

#endif
