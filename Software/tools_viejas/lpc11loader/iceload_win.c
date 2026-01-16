#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
//#include "jedec.c"
#include "chips.c"
// ------------------------------------------------------------------------
HANDLE hComm;
DCB dcb = {0};
// ------------------------------------------------------------------------
void PrintCommState(DCB dcb)
{
 //  Print some of the DCB structure values
  _tprintf( TEXT("\nBaudRate = %d, ByteSize = %d, Parity = %d, StopBits = %d\n"), 
              dcb.BaudRate, 
              dcb.ByteSize, 
              dcb.Parity,
              dcb.StopBits );
}
// ------------------------------------------------------------------------
int AbrirCOM(char *COM_PORT, int baud)
{
 hComm = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE,
                    0, 0, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, 0);
 if(hComm ==INVALID_HANDLE_VALUE)
  { return -1; } else { printf("Serial port %s successfully reconfigured.", COM_PORT);  }
 //DCB dcb;
 dcb.DCBlength=sizeof(dcb);

 if(!GetCommState(hComm, &dcb))
 { printf("Error 2, puerto serie\n"); return -2; }
// dcb.BaudRate = (DWORD) CBR_115200;
 dcb.BaudRate = (DWORD) baud;
 dcb.ByteSize = (BYTE) 8;
 dcb.Parity   = (BYTE) NOPARITY;
 dcb.StopBits = (BYTE) ONESTOPBIT;

 PrintCommState(dcb);

 if(!SetCommState(hComm, &dcb))
 { printf("Error 3, SetCommStatus\n"); return -3; }

 COMMTIMEOUTS timeouts={0};
 timeouts.ReadIntervalTimeout = 50;
 timeouts.ReadTotalTimeoutConstant = 50;
 timeouts.ReadTotalTimeoutMultiplier = 10;
 timeouts.WriteTotalTimeoutConstant = 50;
 timeouts.WriteTotalTimeoutMultiplier = 10;

 if(!SetCommTimeouts(hComm,&timeouts))
 { printf("Error 4, timeouts\n"); return -4; }

 return 1;
}
// ------------------------------------------------------------------------
int CambiaVelocidad(int baud)
{
 if(!GetCommState(hComm, &dcb))
 { printf("Error 2, puerto serie\n"); return -2; }
 dcb.BaudRate = (DWORD) CBR_115200;
 dcb.BaudRate = (DWORD) baud;
 dcb.ByteSize = (BYTE) 8;
 dcb.Parity   = (BYTE) NOPARITY;
 dcb.StopBits = (BYTE) ONESTOPBIT;

 PrintCommState(dcb);

 if(!SetCommState(hComm, &dcb))
 { printf("Error 3, SetCommStatus\n"); return -3; }
 return 0;
}
// ------------------------------------------------------------------------
void CerrarCOM()
{
 CloseHandle(hComm);
 hComm = NULL;
 return;
}
// ------------------------------------------------------------------------
void usleep(__int64 usec) 
{ 
 HANDLE timer; 
 LARGE_INTEGER ft; 

 ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

 timer = CreateWaitableTimer(NULL, TRUE, NULL); 
 SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
 WaitForSingleObject(timer, INFINITE); 
 CloseHandle(timer); 
}  
// ------------------------------------------------------------------------
// Reads one byte from serial port
int ugetch()
{
   unsigned char buf[16];
   int m = 1;
   DWORD p =0,n = 0, it=50;

   while( m && it) // Hasta que no recibas algo no retorna
    {
     if(!ReadFile(hComm, &buf[p], m, &n, NULL))
	 { printf("Error (%d)\n",GetLastError);return 0;};
     p += n;
     m -= n;
	 it--; // timeout
	// printf("%d",it);
    }
   if(it==0) {/*printf("Timeout\n");*/buf[0]=-1;};
   return buf[0];		
	
}
// ------------------------------------------------------------------------
// Writes one byte to serial port
void uputch(unsigned char d)
{
 DWORD Count;
 //write(serial_fd,&d,1);	
 WriteFile(hComm, &d, 1 , &Count, 0);
}
// ------------------------------------------------------------------------
// multi-byte write to serial port
void swrite(unsigned char *pdata,unsigned int len)
{
 int j,i=0;
 DWORD Count;	
 do	{
  //j=write(serial_fd,&pdata[i],len);
  //i+=j; len-=j;
  WriteFile(hComm, &pdata[i], len, &Count, 0);
  i+=Count; len-=Count;		
 } while(len);
}
// ------------------------------------------------------------------------
// multi-byte read from serial port
void sread(unsigned char *pdata,unsigned int len)
{
 int j,i=0;
 DWORD Count;
 do	{		
  //j=read(serial_fd,&pdata[i],len);
  //i+=j; len-=j;
  j = ReadFile(hComm, &pdata[i], len, &Count, 0);
  i+=Count; len-=Count;
 // if(!j && GetLastError()!=ERROR_IO_PENDING ) return ;
 } while(len);
}

///-----------------------------------------------------

// Write a 32-bit word (big endian)
void uputw(unsigned int d)
{
	unsigned char a;

	uputch(d>>24);
	uputch((d>>16)&0xff);
	uputch((d>>8)&0xff);
	uputch(d&0xff);
}

// Write a 24-bit word (big endian)
void uputw3(unsigned int d)
{
	unsigned char a;

	uputch((d>>16)&0xff);
	uputch((d>>8)&0xff);
	uputch(d&0xff);
}

// Read a 32-bit word (big endian)
int ugetw()
{
    int i,j;
    unsigned char a;

	j=ugetch();	if (j==-1) return -1; i =j<<24; 
	j=ugetch();	if (j==-1) return -1; i|=j<<16; 
	j=ugetch();	if (j==-1) return -1; i|=j<<8; 
	j=ugetch();	if (j==-1) return -1; i|=j; 		
	
    return i;
}

// ------------------------------------------------------------------------
// Value with multiplier (ASCII) to integer
int amtoi(char *p)
{
	int val=0;
	char a;

	while(1) {
		a=*p++;
		if (a>='0' && a<='9') {val*=10; val+=a-'0';}
		else if (a=='k' || a=='K') return val<<10;
		else if (a=='M') return val<<20;
		else return val;
	}
}


////////////////////////////////////////////////////////////////////////////////
// 				MAIN
////////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv)
{
 int i,j,k,l,len,maxlen,dir,com,divi;
 char *fname="flash.bin",*fdev="/dev/ttyACM0";
 static unsigned char buf[1<<24];	// Buffer (whole flash size 16MB)
 FILE *fp;
 int baud=3000000;
 int paridad=0;
 int sl, fdin, fdlog, crlf=1;
 char a,oa,*logn=NULL,fn[256];
 
 DWORD Count;
 int Status;
 char comname[32];
 
 memset(buf,0xFF,1<<24);	// buffer filled with 0xFF
 // Command line parsing
 com=len=dir=divi=0;

 for (i=1;i<argc;i++) 
 {
  if (argv[i][0]=='-') 
  {
   switch(argv[i][1]) 
   {
	case 'd':	if (i<argc-1) fdev=argv[++i]; break;
	case 'b':	com='b'; break;
	case 'r':	com='r'; if (i<argc-1) fname=argv[++i]; break;
	case 'P':	com='P'; goto rdfilein;
	case 'p':	com='p';
rdfilein:	if ((fp=fopen(argv[++i],"rb"))==NULL) 
				{perror("fopen(read)"); exit(1);}
			len=fread(buf,1,1<<24,fp);
			fclose(fp);
			break;
	case 'q':	com='q'; goto rdfilein;					
	case 'C':	com='C'; goto rdfilein;
	case 'c':	com='c'; goto rdfilein;
	case 'l':	len=amtoi(argv[++i]); break;
	case 'a':	dir=amtoi(argv[++i]); break;
	case 's':   baud=atoi(argv[++i]); break;
	case 't':	com='t'; break;
	case 'f':
			divi=atoi(argv[++i]); 
			divi=(48 + divi/2)/divi;
			break;
	case 'g':	logn=argv[++i]; break;	
	default:
			printf("Use %s <options>, with <options>=\n",argv[0]);
			printf("-d <device>   serial Device to use (/dev/ttyUSB...)\n");
			printf("-c <file>     Configure the FPGA with file (bitstream to config RAM)\n");
			printf("              and then enter a serial terminal\n");
			printf("-C <file>     Configure the FPGA with file (bitstream to config RAM)\n");
			printf("              without terminal\n");
			printf("-b            Bulk erase: Erase all the SPI Flash\n");
			printf("-p <file>     Program flash (required sectors erased before programming)\n");
			printf("-P <file>     Program flash (without sector erasing)\n");
			printf("-r <file>     Read SPI Flash to file\n");
			printf("-a <address>  start Address for programming and reading (defaults to 0)\n");
			printf("-l <length>   Length of the data for reading (defaults to all Flash)\n");
            printf("              sufixes ..k (*1024) and ..M (*1024k) can be used with\n");
			printf("              addresses and/or lengths\n");
			printf("-t            just start a serial Terminal\n");
			printf("-s <baud>     Speed for the serial terminal\n");
			printf("-f <MHz>      Frequency to apply to FPGA pin #21 (defaults to 12MHz)\n");
			printf("              possible frequencies: 48MHz/N (N integer)\n");
			printf("              (not supported by the PRGICE board)\n");
			printf("-g <file>     loG terminal data to file\n");
			break;
   }
  }
 }

 // Open serial port
 printf("\n--------------------- LPC-FPGA loader --------------------\n");
 //printf("Device: %s\n",fdev);
 
 // Search for COM port
 for( i=30; i>1; i-- ) 
 {
  sprintf(comname,"\\\\.\\COM%d",i+1);
  if(AbrirCOM(comname,baud)>0) goto L1;
 }
 printf("*** Puerto serie no encontrado\n");
 exit(2);
L1:
 sprintf(comname,"\\\\.\\COM%d",i+1);
   
 // Flush input  
 FlushFileBuffers(hComm);
	
 // Pins: reset: /DTR, bootloader: /RTS 
 // Reset L, Bootloader H 
 Status = EscapeCommFunction(hComm, SETDTR);  // DTR alto
 //Status = EscapeCommFunction(hComm, CLRDTR);  // DTR bajo
 //Status = EscapeCommFunction(hComm, SETRTS);  // RTS alto
 Status = EscapeCommFunction(hComm, CLRRTS);  // RTS bajo
 usleep(100000);
	
 // Reset H 
 Status = EscapeCommFunction(hComm, CLRDTR);  // DTR bajo
 usleep(100000);
 // Flush input  
 FlushFileBuffers(hComm);
	 
 uputch('A');	 
// WriteFile(hComm, "A", 1 , &Count, 0);
 //printf("Enviada la A como %d - %d\n", 'A', Count);
 Count =0 ;
 /*
 if ((serial_fd=open(fdev,O_RDWR|O_NOCTTY))==-1) {perror("open"); exit(1);}
        // set attributes
        bzero(&term,sizeof(term)); // todos los flags en cero por defecto
        term.c_cflag = B3000000 | CS8 | CLOCAL | CREAD; // baud, bits, sin control de flujo, RX on
		term.c_cc[VMIN]=1;
        if (tcsetattr(serial_fd,TCSANOW,&term)) { fprintf(stderr,"Error.tcsetattr\n"); exit(1); }
        // Flush input 
        if (tcflush(serial_fd,TCIFLUSH)) { fprintf(stderr,"Error.tcflush\n"); exit(1); }

        // RESET
		// Pins: reset: /DTR, bootloader: /RTS
        if(ioctl(serial_fd, TIOCMGET, &i)) {fprintf(stderr,"Error.ioctl TIOCMGET\n"); exit(1);}
        i|=TIOCM_DTR|TIOCM_RTS; // Reset L Bootloader L
        if(ioctl(serial_fd, TIOCMSET, &i)) {fprintf(stderr,"Error.ioctl TIOCMSET\n"); exit(1);}
        usleep(100000);
        i&=~TIOCM_RTS;        // bootloader H
        if(ioctl(serial_fd, TIOCMSET, &i)) {fprintf(stderr,"Error.ioctl TIOCMSET\n"); exit(1);}
        usleep(100000);
        i&=~TIOCM_DTR;        // Reset H
        if(ioctl(serial_fd, TIOCMSET, &i)) {fprintf(stderr,"Error.ioctl TIOCMSET\n"); exit(1);}
        usleep(100000);
		
		// Flush input 
        if (tcflush(serial_fd,TCIFLUSH)) { fprintf(stderr,"Error.tcflush\n"); exit(1); }

		// Autobaud
		uputch('A');
		*/
 while(1) 
 {
  i=ugetch();
  if (i==0x00) continue;
  if (i==0x80) continue;
  if (i==0xC0) continue;
  if (i==0xE0) continue;
  if (i==0xF0) continue;
  if (i==0xF8) continue;
  if (i==0xFC) continue;
  if (i==0xFE) continue;
  if (i==0xFF) continue;

  if (i=='A') {printf("Autobaud OK\n"); break;}
  printf("Autobaud Error\n\n"); exit(1);
 }

  // Firmware Version
 uputch('V');
 j=ugetch(); k=ugetch();
 printf("Firmware rev: %d.%02d\n",j,k);
 
 // Flash ID/size
 uputch('I');
 i =ugetch()<<16;
 i|=ugetch()<<8;
 i|=ugetch();
 
 j=(i>>16);
 //printf("Flash ID=0x%06x: %s,",i,(j<127)?jedec_id[j]:"Desconocido");	
 printf("Flash ID=0x%06x: %s,",i,chipid(i));	
 maxlen=1<<(i&0xff);	// Flash Size
 printf(" %d KBytes\n",maxlen>>10);
 
 if (divi) {
 	uputch('F');
 	uputch(divi);
 	if (ugetch()) printf("Error on Frequency set (pin 21)\n");
 	else printf("Pin 21: %f MHz\n",(float)48.0/divi);
 }
 
 
 switch(com) {
 case 'b':	// Bulk Erase
 	printf("Bulk Erase Flash, be patient ..."); fflush(stdout);
 	uputch('B');
 	ugetch();
 	uputch('T');
 	j=ugetw(); k=ugetw();
 	printf("(%.3f s)\n",((float)j)*1e-6);
 	goto final;
 
 case 'r':	// Read Flash
 	if (!len) len=maxlen;
 	com=101;
 	printf("Read Flash addr=0x%06x len=%d bytes >%s\n",dir,len,fname); 
 	uputch('R');
 	uputw3(dir);	// Base Address
 	uputw3(len);	// Block length
 	i=0; k=len;
 	do {
 		j=k; if(j>256) j=256;
 		sread(&buf[i],j);
 		i+=j;
 		k-=j;
 		l=((i+1)*100/len);
 		if (l!=com) {com=l; printf("\r %3d%%",l); fflush(stdout);}
 		uputch('-'); 
 	} while (k);
 
 	uputch('T');
 	j=ugetw(); k=ugetw();
 	if (j>=1000000) printf("  (%.2f s)\n",((float)j)*1e-6);
 	else if (j>1000) printf("  (%.2f ms)\n",((float)j)*1e-3);
 	else printf("  (%d us)\n",j);
 	if((fp=fopen(fname,"wb"))==NULL) {perror("fopen(write)"); exit(1);}
 	fwrite(buf,1,len,fp);
 	fclose(fp);
 	break;
 
 case 'P':
 case 'p':		// SPI Flash Programming
 	len=(len+255)&0x00FFFF00;	// pad to multiple of 256
 	if (dir&0xff) {printf("Error: Address must be multiple of 256\n\n"); exit(1);}
 	printf("Program Flash addr=0x%06x len=%d bytes\n",dir,len);
 	if (com=='p') {
 		i=dir>>16;			// Start Sector
 		j=(dir+len-1)>>16;	// End Sector
 		for (;i<=j;i++) {
 			printf("\r  Erasing sector %d",i); fflush(stdout);
 			uputch('E');uputch(i);
 			if (k=ugetch()) {printf("error = %d\n", k); exit(2);};
 		}
 		uputch('T');
 		j=ugetw(); k=ugetw();
 		printf("  (sector erase: ");
 		if (j>=1000000) printf("%.2f s)\n",((float)j)*1e-6);
 		else if (j>1000) printf("%.2f ms)\n",((float)j)*1e-3);
 		else printf("%d us)\n",j);
 	}
 	printf("  Programming\n"); 
 	j=len; k=0; com=101;
 	do {
 		uputch('P');
 		uputch(dir>>16);
 		uputch(dir>>8);
 		//for (i=0;i<256;i++) uputch(buf[k++]);
 		swrite(&buf[k],256); k+=256;
 		if (l=ugetch()) { printf("\n In: %c (%u)\n",l,l); exit(2);}
 		dir+=256; j-=256;
 		i=100-(j*100/len);
 		if (i!=com) {com=i; printf("\r  %3d%% %6d",i,j); fflush(stdout);}
 	} while (j);
 	uputch('T');
 	j=ugetw(); k=ugetw();
 	printf("  (page data: %d us, page program: %dus)\n",j,k);
 	break;
 
 case 'q':
 	printf("Config FPGA len=%d bytes (Flash SQI)\n",len);
 	uputch('Q');
 	goto labconf;
 	
 case 'C':
 case 'c':	//	Config RAM (load directly to FPGA)
 	printf("Config FPGA len=%d bytes\n",len);
 	uputch('C');
labconf:	
    uputw3(len);
 	if (ugetch()!=1) exit(2);	// Wait for FPGA RESET
 	swrite(buf,len);	// Send Image
 
 	if (ugetch()) {printf("Error: CDONE not set\n\n"); exit(2);}
 	if (com=='c') goto terminal;
 	goto final;
 	
 case 't':
 	uputch('X');
 	if (ugetch()!='X') {printf("FPGA exec error\n\n"); exit(1);}
 	goto terminal;
 
 default:
 	printf("Nothing to do: Hasta luego Lucas :)\n");
 	break;
 }
 
 uputch('X');
 if (ugetch()!='X') {printf("FPGA exec error\n\n"); exit(1);}
final:
 printf("----------------------------------------------------------\n"); 
 exit(0);

/////////////////////////////////////////////////////////////////////////////////////
//                                TERMINAL
/////////////////////////////////////////////////////////////////////////////////////
terminal: 
 // Terminal serie ----------------------------------------------------
 printf("Serial terminal **(115200bps)\n");
 CambiaVelocidad(CBR_115200);
 usleep(100);
 char c[256]; 

 do
 {
  // Read from serial port  
  c[0]=0;
  j = ReadFile(hComm, &c[0], 1, &Count, 0);
  if(j&& Count) printf("%c",c[0]);
  // Read from keyboard
  if(_kbhit()) 
  {
   c[0] = _getch();   
   uputch(c[0]);
   c[1] = c[0];
  }
 } while(c[0]!=27 && c[1]!=27); 
 CerrarCOM();
 return 0;

/*
        for (i=0;baudtable[i][0];i++) {
            if (baud==baudtable[i][0]) break;
        }
        if (!baudtable[i][0]) {printf("Serial speed unsupported\n"); exit(0);}

		tcgetattr(serial_fd,&term);
        term.c_cflag&=~CBAUD;
        term.c_cflag|=baudtable[i][1];
		if (paridad) {
			term.c_cflag|=PARENB;
			if (paridad&1) term.c_cflag|=PARODD;
		}

        if (tcsetattr(serial_fd,TCSANOW,&term)) {
                fprintf(stderr,"Error.tcsetattr\n");
                exit(0);
        }
		// Flush input 
        if (tcflush(serial_fd,TCIFLUSH)) { fprintf(stderr,"Error.tcflush\n"); exit(1); }

		printf("-----------------------------------------------\n");
        printf("Terminal: Speed: %d baud\n",baud);		
        printf("-----------------------------------------------\n");
        // non blocking uart
        i=fcntl(serial_fd,F_GETFL); fcntl(serial_fd,F_SETFL,i|O_NONBLOCK);

        // STDIN non-blocking, no canon, no eco
        i=fcntl(0,F_GETFL); fcntl(0,F_SETFL,i|O_NONBLOCK);
        tcgetattr(0,&term);
        term.c_lflag&=~ICANON;
        term.c_lflag&=~ECHO;
        if (tcsetattr(0,TCSANOW,&term)) {
                fprintf(stderr,"Error.tcsetattr\n");
                exit(0);
        }

		// Create log file if requested
		if (logn) {
			if ((fdlog=creat(logn,0666))<=0) {perror("creat log"); exit(0);}
		}

        fdin=0; // reading from stdin by default
        for (;;) {
                sl=2; // Sleep Counter
                if ((j=read(serial_fd,buf,1024))>0) { // data from serial port
					write(1,buf,j);
					if (logn) write(fdlog,buf,j); // to Log file
                } else sl--;
				oa=a;
                j=read(fdin,&a,1);	// data from stdin or file
                if (j==0 && fdin) {
                        close(fdin);
                        fdin=0; // file end: go back to stdin
                }
                if (j==1) {
                        switch (a) {
                        case    27:	// <esc><esc> => exit program
								if (oa==27) {
	                                // set STDIN as before
    	                            tcgetattr(0,&term);
    	                            term.c_lflag|=ICANON;
    	                            term.c_lflag|=ECHO;
    	                            if (tcsetattr(0,TCSANOW,&term))
    	                            	fprintf(stderr,"Error.tcsetattr\n");
									if(logn) close(fdlog);
    	                            exit(0);
								}
								break;
                        case    'V'-64: a=3; // <ctrl>-V translated as <ctrl>-C
                                break;
                        case    '\n':   if (crlf) a='\r'; // LF -> CR,LF si requested
                                break;
                        case    'F'-64: 	// <ctrl>-F => "type" File
								printf("\nInclude File: ");
                                fflush(stdout);
                                tcgetattr(0,&term);	// stdin blocking and echo on
                                term.c_lflag|=ECHO;
                                tcsetattr(0,TCSANOW,&term);
                                i=fcntl(0,F_GETFL); fcntl(0,F_SETFL,i&(~O_NONBLOCK));
								// get the file name from stdin
                                fgets(fn,255,stdin); fn[strlen(fn)-1]=0;

								if ((fp=fopen(fn,"rb"))!=NULL) {
									// blocking uart
							        i=fcntl(serial_fd,F_GETFL); fcntl(serial_fd,F_SETFL,i&(~O_NONBLOCK));
									while(1) {
										j=fread(buf,1,(1<<12),fp);
										if (j) swrite(buf,j);
										if(feof(fp)) break;
									}
									fclose(fp);
									// non-blocking uart
							        i=fcntl(serial_fd,F_GETFL); fcntl(serial_fd,F_SETFL,i|O_NONBLOCK);
								}

								// stdin non-blocking, no echo
                                fcntl(0,F_SETFL,i|O_NONBLOCK);
                                term.c_lflag&=~ECHO;
                                tcsetattr(0,TCSANOW,&term);
								
                                continue;
                        case    'T'-64: 	// <ctrl>-T => "tape play" File
								printf("\nInclude tape file (.rle): ");
                                fflush(stdout);
                                tcgetattr(0,&term);	// stdin blocking and echo on
                                term.c_lflag|=ECHO;
                                tcsetattr(0,TCSANOW,&term);
                                i=fcntl(0,F_GETFL); fcntl(0,F_SETFL,i&(~O_NONBLOCK));
								// get the file name from stdin
                                fgets(fn,255,stdin); fn[strlen(fn)-1]=0;

								if ((fp=fopen(fn,"rb"))!=NULL) {
									// blocking uart
							        i=fcntl(serial_fd,F_GETFL); fcntl(serial_fd,F_SETFL,i&(~O_NONBLOCK));
							        i=fread(buf,1,sizeof(buf),fp);
									fclose(fp);
							        swrite(buf,4096);
							        j=4096;
									do {
										read(serial_fd,&a,1);	// wait
										swrite(&buf[j],2048);
										j+=2048;
										if ((j&(3<<11))==0) {printf("\r(%2d%%)",(j*100)/i); fflush(stdout);}
									} while (j<i);

									// non-blocking uart
							        i=fcntl(serial_fd,F_GETFL); fcntl(serial_fd,F_SETFL,i|O_NONBLOCK);
								}

								// stdin non-blocking, no echo
                                fcntl(0,F_SETFL,i|O_NONBLOCK);
                                term.c_lflag&=~ECHO;
                                tcsetattr(0,TCSANOW,&term);
								
                                continue;
                        }
                        uputch(a); 		
                        usleep(1000); 	// delay between characters (do not flood the receiver)
						// CR,LF: longer delay
                        if (a=='\r') {uputch('\n'); usleep(5000);}
                } else sl--;
                if (!sl) usleep(10000); // Sleep process if no data is available
        }
*/
}

