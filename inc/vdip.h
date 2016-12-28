void vVdip_pause(void);
void vVdip_very_big_pause(void);

void vSendVdipString(unsigned char *msg);
void vSendVdipBinaryString(unsigned char xdata *puc,int iNumChars);

void vReceiveVdipString(void);
// programma di test comunicazione con modulo vdip
void main_vdip(void);
extern xdata unsigned char ucStringRx_Vdip[150];
void vInitVdip(void);
unsigned char ucHandleVdipStati(void);
void vSendVdipString_NOCR(unsigned char  *msg);
// riceve una stringa binaria composta da almeno uiNumBytes2Receive, seguita dal prompt
void vReceiveVdipBinaryString(unsigned int uiNumBytes2Receive);
