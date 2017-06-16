#ifndef _BOOT_H_
#define _BOOT_H_

#define	PROGRAM_LENGTH_ADDR	0x1fc

void writeProgramStartDownloadFlag(void);
void writeProgramFinishDownloadFlag(void);
void checkFirstStart(void);
void sendString(char* str);

#endif

