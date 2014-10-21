 /* Lire .hex de carte SD / micro-SD et programme
 * Un autre Arduino via TTL.
 * Cré à partir du code source de avrdude (licence GPL)
 * Copyright (C) 2002-2004 Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2008 Joerg Wunsch
 * Crée le 26.12.2011 Kevin Osborn
 */

#include <SoftwareSerial.h>
#include "stk500.h"
#include <SD.h>
#include <SPI.h>
#include <Adafruit_GFX.h>  
#include <Adafruit_ST7735.h>
 
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

#define BOOT_BAUD 115200
#define DEBUG_BAUD 19200

#define txPin 4
#define rxPin 5
#define rstPin 6


#define TFT_CS  10 
#define TFT_DC   9 
#define TFT_RST  8

#define ToucheA 19
#define ToucheB 18
#define ToucheGauche 16

int VarTouche = 0;

#define LED1 2   // Activité de la communication I2SD
#define LED2 3
//SoftwareSerial sSerial= SoftwareSerial(rxPin,txPin);
// set up variables using the SD utility library functions:
SdFile root;
File Fichierprog;
avrmem mybuf;
unsigned char mempage[128];

// Pin CS utilisé
const int chipSelect = 7;  


//#define DEBUGPLN sSerial.println
//#define DEBUGP sSerial.print

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

int chargement = 0;
int lastchargement = 0;
float taillefichier = 0.0;
float progression = 0.0;

File Dossierprog;


  int freeRam () {     // Ram libre sur l'arduino
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
  

  
}

void setup() 
{
  
  digitalWrite(LED1,HIGH);
  
  tft.initR(INITR_BLACKTAB);
  
  mybuf.buf = &mempage[0];
  //sSerial.begin(DEBUG_BAUD);

  Serial.begin(BOOT_BAUD);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(rstPin,OUTPUT);
  pinMode(chipSelect,OUTPUT);
  pinMode(LED1,OUTPUT);
  
  pinMode(ToucheA, INPUT);
  pinMode(ToucheB, INPUT);
  pinMode(ToucheGauche, INPUT);
  
  tft.setRotation(3);   // X = 160  Y = 128
  tft.fillScreen(ST7735_BLACK);
  
   tft.setCursor(20, 50); 
   tft.setTextColor(ST7735_RED, ST7735_WHITE);  
   tft.setTextSize(2);
   tft.println (F("GAME WOOD")) ;
   
   tft.setTextSize(1);    
   tft.setCursor(5, 80); 
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE);  
   tft.println (F("Demarrage machine")) ;
   tft.setTextSize(1);
  
    // Vérification de la présence de la carte SD
  if (!SD.begin(chipSelect)) 
  {
   tft.setCursor(5, 90);
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE); 
   tft.println (F("Erreure: Carte non presente")) ;
  }
  else
  {
   tft.setCursor(5, 90);
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE); 
   tft.println (F("Machine configure"));
   }
  
   //delay(1000);
 
}

void loop() {
    
  Dossierprog = SD.open("/jeux/");
  Afficherfichier(Dossierprog); 
  Dossierprog.close();
  
  Dossierprog = SD.open("/jeux/data");
  Chargement();
  Dossierprog.close();
  
  
 //DEBUGP(F("Free Ram: "));   // Afficher la rame libre
 //DEBUGPLN(freeRam());
  
  tft.fillScreen(ST7735_BLACK);  
}


void Afficherfichier(File dir) 
{ 
 uint8_t c;

 while(true)
 { 
   char* Nomprog;

 File Fichier = dir.openNextFile(); 
 
    if (!Fichier) 
    { 
     // Retourner au premier fichier du répertoire
    dir.rewindDirectory();  
    }  
    /*
    for ( uint8_t i = 0 ; i < numTabs ; i ++ ) { 
    Serial . print ( ' \t ' ) ; 
   } */



   tft.fillScreen(ST7735_BLACK); 
   tft.setCursor(40, 54);
   tft.setTextColor(ST7735_RED, ST7735_WHITE);  
   tft.setTextSize(1);
   tft.println (Fichier.name()); 
   
   tft.setCursor(60, 70);
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE);  
   tft.println (Fichier.size(), DEC);
 
   tft.setCursor(1, 90);
   tft.println (F(" <= pour suivant "));
   tft.println (F(" A pour ouvrire "));
   
  while (VarTouche < 1)
   {
    Serial.println(VarTouche);
    VarTouche = Touche(); 
   }
   
      Serial.println(VarTouche);
    delay(100);
      switch(VarTouche) 
      {
         case 6:
           tft.setCursor(90, 5);
           tft.println(F("Jeux suivant"));
           Fichier.close();
           break;
                    
         case 2:
           tft.setCursor(50, 30);
           tft.println(F("Lancement du jeux"));
           blinky(2,200);
           delay(100);
            
           Nomprog = Fichier.name();
           Fichier.close();
           programArduino( strcat("/jeux/", Fichier.name()) );  // Programme qui sera envoyé à l'Arduino esclave
           
           tft.fillScreen(ST7735_BLACK);
                     
           break;
           break;  
        }
        
   VarTouche = 0;             
  }
}




// Line Buffer is set up in global SRAM
#define LINELENGTH 50
unsigned char linebuffer[LINELENGTH];
unsigned char linemembuffer[16];
int readPage(File input, avrmem *buf)
{
  int len;
  int address;
  int total_len =0;
  // grab 128 bytes or less (one page)
  for (int i=0 ; i < 8; i++){
    len = readIntelHexLine(input, &address, &linemembuffer[0]);
    if (len < 0)
      break;
    else
      total_len=total_len+len;
    if (i==0)// first record determines the page address
      buf->pageaddress = address;
    memcpy((buf->buf)+(i*16), linemembuffer, len);
  }
  buf->size = total_len;
  return total_len;
  
}
// read one line of intel hex from file. Return the number of databytes
// Since the arduino code is always sequential, ignore the address for now.
// If you want to burn bootloaders, etc. we'll have to modify to 
// return the address.

// INTEL HEX FORMAT:
// :<8-bit record size><16bit address><8bit record type><data...><8bit checksum>


int readIntelHexLine(File input, int *address, unsigned char *buf){
  unsigned char c;
  int i=0;
  while (true){
    if (input.available()){
      
      chargement++;
      
      if (chargement >= (lastchargement + 100))
      {
      lastchargement = chargement;
      
      progression = chargement / taillefichier;
      progression = progression * 100;
             
      tft.setCursor(10, 32);
      tft.print(progression);
      tft.println(F(" %   "));

      }
      
      if ((chargement == taillefichier) || ((chargement + 70) > taillefichier))  // Ne fonctionne pas /!\
      {
      tft.setCursor(10, 32);
      tft.println(F("100 %   "));
      }
      
      c = input.read();
      // this should handle unix or ms-dos line endings.
      // break out when you reach either, then check
      // for lf in stream to discard
      if ((c == 0x0d)|| (c == 0x0a))
        break;
      else
        linebuffer[i++] =c;
    }
    else return -1; //end of file
  }
  linebuffer[i]= 0; // terminate the string
  //peek at the next byte and discard if line ending char.
  if (input.peek() == 0xa)
    input.read();
  int len = hex2byte(&linebuffer[1]);
  *address = (hex2byte(&linebuffer[3]) <<8) |
               (hex2byte(&linebuffer[5]));
  int j=0;
  for (int i = 9; i < ((len*2)+9); i +=2){
    buf[j] = hex2byte(&linebuffer[i]);
    j++;
  }
  return len;
}
unsigned char hex2byte(unsigned char *code){
  unsigned char result =0;

  if ((code[0] >= '0') && (code[0] <='9')){
    result = ((int)code[0] - '0') << 4;
  }
  else if ((code[0] >='A') && (code[0] <= 'F')) {
    result = ((int)code[0] - 'A'+10) << 4;
  }
  if ((code[1] >= '0') && (code[1] <='9')){
    result |= ((int)code[1] - '0');
  }
  else if ((code[1] >='A') && (code[1] <= 'F'))  
    result |= ((int)code[1] -'A'+10);  
return result;
}

// Right now there is only one file.
void programArduino(char * filename)
{
  
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setTextWrap(true);
  
  tft.println(F("=> Programation commencer"));
  tft.println(F("=>Chargement de :"));
  tft.println(filename);
  
  digitalWrite(rstPin,HIGH);

  unsigned int major=0;
  unsigned int minor=0;
  delay(100);
   toggle_Reset();
   delay(10);
   stk500_getsync();
   stk500_getparm(Parm_STK_SW_MAJOR, &major);

   stk500_getparm(Parm_STK_SW_MINOR, &minor);

if (SD.exists(filename)){
    Fichierprog = SD.open(filename, FILE_READ);
    
    taillefichier = Fichierprog.size();
    
  }
  else{
    tft.println(F("Fichier non existant"));
    return;
  }
  //enter program mode
  
  tft.println(F("=> Envoie programme"));
  
  stk500_program_enable();

  while (readPage(Fichierprog,&mybuf) > 0){
    stk500_loadaddr(mybuf.pageaddress>>1);
    stk500_paged_write(&mybuf, mybuf.size, mybuf.size);
  }

  // could verify programming by reading back pages and comparing but for now, close out
  
  tft.println(F("=> Verification programme"));
  
  chargement = 0;
  lastchargement = 0;
  progression = 0;
    
  stk500_disable();
  delay(10);
  toggle_Reset();
  Fichierprog.close();
  blinky(4,500);
  
  tft.println(F("=> Programation terminee"));
   
}
void blinky(int times, long delaytime){
  for (int i = 0 ; i < times; i++){
    digitalWrite(LED1,HIGH);
    delay(delaytime);
    digitalWrite(LED1, LOW);
    delay (delaytime);
  }
  
}
void toggle_Reset()
{
  digitalWrite(rstPin, LOW);
  delayMicroseconds(1000);
  digitalWrite(rstPin,HIGH);
}
static int stk500_send(byte *buf, unsigned int len)
{

  Serial.write(buf,len);
}
static int stk500_recv(byte * buf, unsigned int len)
{
  int rv;

 
  rv = Serial.readBytes((char *)buf,len);
  if (rv < 0) {
    return -1;
  }
  return 0;
}
int stk500_drain()
{
  return 1;
}
int stk500_getsync()
{
  byte buf[32], resp[32];

  /*
   * get in sync */
  buf[0] = Cmnd_STK_GET_SYNC;
  buf[1] = Sync_CRC_EOP;
  
  /*
   * First send and drain a few times to get rid of line noise 
   */
  
  stk500_send(buf, 2);
  stk500_drain();
  stk500_send(buf, 2);
  stk500_drain();
  
  stk500_send(buf, 2);
  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_INSYNC) {
    stk500_drain();
    return -1;
  }

  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_OK) {

    return -1;
  }
  return 0;
}
static int stk500_getparm(unsigned parm, unsigned * value)
{
  byte buf[16];
  unsigned v;
  int tries = 0;

 retry:
  tries++;
  buf[0] = Cmnd_STK_GET_PARAMETER;
  buf[1] = parm;
  buf[2] = Sync_CRC_EOP;

  stk500_send(buf, 3);

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {

      return -1;
    }
   if (stk500_getsync() < 0)
      return -1;
      
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {

    return -2;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  v = buf[0];

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_FAILED) {

    return -3;
  }
  else if (buf[0] != Resp_STK_OK) {

    return -3;
  }

  *value = v;

  return 0;
}
/* read signature bytes - arduino version */
static int arduino_read_sig_bytes(AVRMEM * m)
{
  unsigned char buf[32];

  /* Signature byte reads are always 3 bytes. */

  if (m->size < 3) {
    return -1;
  }

  buf[0] = Cmnd_STK_READ_SIGN;
  buf[1] = Sync_CRC_EOP;

  stk500_send(buf, 2);

  if (stk500_recv(buf, 5) < 0)
    return -1;
  if (buf[0] == Resp_STK_NOSYNC) {

	return -1;
  } else if (buf[0] != Resp_STK_INSYNC) {

	return -2;
  }
  if (buf[4] != Resp_STK_OK) {

    return -3;
  }

  m->buf[0] = buf[1];
  m->buf[1] = buf[2];
  m->buf[2] = buf[3];

  return 3;
}

static int stk500_loadaddr(unsigned int addr)
{
  unsigned char buf[16];
  int tries;

  tries = 0;
 retry:
  tries++;
  buf[0] = Cmnd_STK_LOAD_ADDRESS;
  buf[1] = addr & 0xff;
  buf[2] = (addr >> 8) & 0xff;
  buf[3] = Sync_CRC_EOP;


  stk500_send(buf, 4);

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {

      return -1;
    }
    if (stk500_getsync() < 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {

    return -1;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }


  return -1;
}
static int stk500_paged_write(AVRMEM * m, 
                              int page_size, int n_bytes)
{
  // This code from avrdude has the luxury of living on a PC and copying buffers around.
  // not for us...
 // unsigned char buf[page_size + 16];
 unsigned char cmd_buf[16]; //just the header
  int memtype;
 // unsigned int addr;
  int block_size;
  int tries;
  unsigned int n;
  unsigned int i;
  int flash;

  // Fix page size to 128 because that's what arduino expects
  page_size = 128;
  //EEPROM isn't supported
  memtype = 'F';
  flash = 1;


    /* build command block and send data separeately on arduino*/
    
    i = 0;
    cmd_buf[i++] = Cmnd_STK_PROG_PAGE;
    cmd_buf[i++] = (page_size >> 8) & 0xff;
    cmd_buf[i++] = page_size & 0xff;
    cmd_buf[i++] = memtype;
    stk500_send(cmd_buf,4);
    stk500_send(&m->buf[0], page_size);
    cmd_buf[0] = Sync_CRC_EOP;
    stk500_send( cmd_buf, 1);

    if (stk500_recv(cmd_buf, 1) < 0)
      exit(1); // errr need to fix this... 
    if (cmd_buf[0] == Resp_STK_NOSYNC) {

        return -3;
     }
    else if (cmd_buf[0] != Resp_STK_INSYNC) {


      return -4;
    }
    
    if (stk500_recv(cmd_buf, 1) < 0)
      exit(1);
    if (cmd_buf[0] != Resp_STK_OK) {


      return -5;
    }
  

  return n_bytes;
}
#ifdef LOADVERIFY //maybe sometime? note code needs to be re-written won't work as is
static int stk500_paged_load(AVRMEM * m, 
                             int page_size, int n_bytes)
{
  unsigned char buf[16];
  int memtype;
  unsigned int addr;
  int a_div;
  int tries;
  unsigned int n;
  int block_size;

  memtype = 'F';


  a_div = 1;

  if (n_bytes > m->size) {
    n_bytes = m->size;
    n = m->size;
  }
  else {
    if ((n_bytes % page_size) != 0) {
      n = n_bytes + page_size - (n_bytes % page_size);
    }
    else {
      n = n_bytes;
    }
  }

  for (addr = 0; addr < n; addr += page_size) {
//    report_progress (addr, n_bytes, NULL);

    if ((addr + page_size > n_bytes)) {
	   block_size = n_bytes % page_size;
	}
	else {
	   block_size = page_size;
	}
  
    tries = 0;
  retry:
    tries++;
    stk500_loadaddr(addr/a_div);
    buf[0] = Cmnd_STK_READ_PAGE;
    buf[1] = (block_size >> 8) & 0xff;
    buf[2] = block_size & 0xff;
    buf[3] = memtype;
    buf[4] = Sync_CRC_EOP;
    stk500_send(buf, 5);

    if (stk500_recv(buf, 1) < 0)
      exit(1);
    if (buf[0] == Resp_STK_NOSYNC) {
      if (tries > 33) {
        error(ERRORNOSYNC);
        return -3;
      }
      if (stk500_getsync() < 0)
	return -1;
      goto retry;
    }
    else if (buf[0] != Resp_STK_INSYNC) {
      error1(ERRORPROTOSYNC, buf[0]);
      return -4;
    }

    if (stk500_recv(&m->buf[addr], block_size) < 0)
      exit(1);

    if (stk500_recv(buf, 1) < 0)
      exit(1);

    if (buf[0] != Resp_STK_OK) {
        error1(ERRORPROTOSYNC, buf[0]);
        return -5;
      }
    }
  

  return n_bytes;
}
#endif

/*
 * issue the 'program enable' command to the AVR device
 */
static int stk500_program_enable()
{
  unsigned char buf[16];
  int tries=0;

 retry:
  
  tries++;

  buf[0] = Cmnd_STK_ENTER_PROGMODE;
  buf[1] = Sync_CRC_EOP;

  stk500_send( buf, 2);
  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {

      return -1;
    }
    if (stk500_getsync()< 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {

    return -1;
  }

  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }
  else if (buf[0] == Resp_STK_NODEVICE) {

    return -1;
  }

  if(buf[0] == Resp_STK_FAILED)
  {

	  return -1;
  }




  return -1;
}

static void stk500_disable() 
{
  unsigned char buf[16];
  int tries=0;

 retry:
  
  tries++;

  buf[0] = Cmnd_STK_LEAVE_PROGMODE;
  buf[1] = Sync_CRC_EOP;

  stk500_send( buf, 2);
  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {

      return;
    }
    if (stk500_getsync() < 0)
      return;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {

    return;
  }

  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return;
  }
  else if (buf[0] == Resp_STK_NODEVICE) {

    return;
  }
  return;
}
//original avrdude error messages get copied to ram and overflow, wo use numeric codes.


int Touche()
{
 
if (digitalRead(ToucheA) == 1)
{
 return 2;
}

if (digitalRead(ToucheB) == 1)
{
 return 4;
}

if (digitalRead(ToucheGauche) == 1)
{
 return 6;
}

 return 0;
 
}



void Chargement()
{
 char* Donne;
 int i = 0;
  while(Serial.available())
  {
   Donne[i++] = Serial.read();
  } 
    
  programArduino( strcat("/jeux/data", Donne) );
  
  i = 0;
  Donne = 0;
}
  
