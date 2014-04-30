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


#define LED1 2   // Activité de la communication I2SD
#define LED2 3
SoftwareSerial sSerial= SoftwareSerial(rxPin,txPin);
// set up variables using the SD utility library functions:
SdFile root;
File Fichierprog;
avrmem mybuf;
unsigned char mempage[128];

// Pin CS utilisé
const int chipSelect = 7;  

#define DEBUGPLN sSerial.println
#define DEBUGP sSerial.print

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
  sSerial.begin(DEBUG_BAUD);

  Serial.begin(BOOT_BAUD);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(rstPin,OUTPUT);
  pinMode(chipSelect,OUTPUT);
  pinMode(LED1,OUTPUT);
    // Vérification de la présence de la carte SD
  if (!SD.begin(chipSelect)) 
  {
    DEBUGPLN(F("Card failed, or not present"));
    return;
  }
   
  Serial.println(F("Carte initialisee"));
  
  tft.setRotation(3);   // X = 160  Y = 128
  tft.fillScreen(ST7735_BLACK);

  Affichimage("woof.bmp", 0, 0);

  delay(100);
  tft.fillScreen(ST7735_BLACK);
 
}

void loop() {
    
  Dossierprog = SD.open("/jeux/");
  Afficherfichier(Dossierprog); 
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
 // char Nomimage[32] = {};
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
   } 
    */
    
  /*
   if (Fichier.isDirectory()) 
   {  
   tft.setCursor(0, 5); 
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE);  
   tft.setTextSize(1);
   tft.print ("Ouverture du Dossier:") ; 
   tft.print ( "/" ) ; 
   tft.print (Fichier.name()) ;
   Afficherfichier(Fichier) ; 
   } else { 
   // Fichier a un poid, un dossier non
   tft.setCursor(0, 5);
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE);  
   tft.setTextSize(1);
   tft.print ( " \t \t " ) ; 
   tft.println (Fichier.size(), DEC ) ; 
  }*/



   // Affichimage(strcat(strncat(Nomimage, Nomprog , strlen(Nomprog) - 3),"BMP"),0,0);
   
   tft.fillScreen(ST7735_BLACK); 
   tft.setCursor(40, 54);
   tft.setTextColor(ST7735_RED, ST7735_WHITE);  
   tft.setTextSize(1);
   tft.println (Fichier.name()); 
   
   tft.setCursor(60, 70);
   tft.setTextColor(ST7735_BLUE, ST7735_WHITE);  
   tft.println (Fichier.size(), DEC);
 
   tft.setCursor(1, 90);
   tft.println (F("=> S pour suivant"));
   tft.println (F("=> P pour precedent"));
   tft.println (F("=> E pour ouvrire"));

    while(!Serial.available());
    
    c = Serial.read();
    
      switch(c) 
      {
         case 's':
           tft.setCursor(90, 5);
           tft.println(F("Jeux suivant"));
           Fichier.close();
           break;
           
           
         case 'p':
           tft.setCursor(10, 5);
           tft.println(F("Jeux précédent"));
           break;
           
         case 'e':
           tft.setCursor(50, 30);
           tft.println(F("Lancement du jeux"));
           blinky(2,200);
           delay(100);
            
           Nomprog = Fichier.name();
           Fichier.close();
           programArduino(strcat("/jeux/", Fichier.name()) );  // Programme qui sera envoyé à l'Arduino esclavev
  
           break;
           break;   
        }

             
  }
}



#define BUFFPIXEL 2

void Affichimage(char *filename, uint8_t x, uint8_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.println(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.Color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(F(" ms"));
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
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
  DEBUGP(F("software major: "));
  DEBUGPLN(major);
  stk500_getparm(Parm_STK_SW_MINOR, &minor);
  DEBUGP(F("software Minor: "));
  DEBUGPLN(minor);
if (SD.exists(filename)){
    Fichierprog = SD.open(filename, FILE_READ);
    
    taillefichier = Fichierprog.size();
    
  }
  else{
    DEBUGP(filename);
    DEBUGPLN(F(" doesn't exist"));
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
    error(ERRORNOPGMR);
    return -1;
  }
  return 0;
}
int stk500_drain()
{
  while (Serial.available()> 0)
  {  
    DEBUGP(F("draining: "));
    DEBUGPLN(Serial.read(),HEX);
  }
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
        error1(ERRORPROTOSYNC,resp[0]);
    stk500_drain();
    return -1;
  }

  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_OK) {
    error1(ERRORNOTOK,resp[0]);
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
      error(ERRORNOSYNC);
      return -1;
    }
   if (stk500_getsync() < 0)
      return -1;
      
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
    return -2;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  v = buf[0];

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_FAILED) {
    error1(ERRORPARMFAILED,v);
    return -3;
  }
  else if (buf[0] != Resp_STK_OK) {
    error1(ERRORNOTOK,buf[0]);
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
    DEBUGPLN(F("memsize too small for sig byte read"));
    return -1;
  }

  buf[0] = Cmnd_STK_READ_SIGN;
  buf[1] = Sync_CRC_EOP;

  stk500_send(buf, 2);

  if (stk500_recv(buf, 5) < 0)
    return -1;
  if (buf[0] == Resp_STK_NOSYNC) {
    error(ERRORNOSYNC);
	return -1;
  } else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
	return -2;
  }
  if (buf[4] != Resp_STK_OK) {
    error1(ERRORNOTOK,buf[4]);
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
      error(ERRORNOSYNC);
      return -1;
    }
    if (stk500_getsync() < 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC, buf[0]);
    return -1;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }

  error1(ERRORPROTOSYNC, buf[0]);
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
        error(ERRORNOSYNC);
        return -3;
     }
    else if (cmd_buf[0] != Resp_STK_INSYNC) {

     error1(ERRORPROTOSYNC, cmd_buf[0]);
      return -4;
    }
    
    if (stk500_recv(cmd_buf, 1) < 0)
      exit(1);
    if (cmd_buf[0] != Resp_STK_OK) {
    error1(ERRORNOTOK,cmd_buf[0]);

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
      error(ERRORNOSYNC);
      return -1;
    }
    if (stk500_getsync()< 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
    return -1;
  }

  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }
  else if (buf[0] == Resp_STK_NODEVICE) {
    error(ERRORNODEVICE);
    return -1;
  }

  if(buf[0] == Resp_STK_FAILED)
  {
      error(ERRORNOPROGMODE);
	  return -1;
  }


  error1(ERRORUNKNOWNRESP,buf[0]);

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
      error(ERRORNOSYNC);
      return;
    }
    if (stk500_getsync() < 0)
      return;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
    return;
  }

  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return;
  }
  else if (buf[0] == Resp_STK_NODEVICE) {
    error(ERRORNODEVICE);
    return;
  }

  error1(ERRORUNKNOWNRESP,buf[0]);

  return;
}
//original avrdude error messages get copied to ram and overflow, wo use numeric codes.
void error1(int errno,unsigned char detail){
  DEBUGP(F("error: "));
  DEBUGP(errno);
  DEBUGP(F(" detail: 0x"));
  DEBUGPLN(detail,HEX);
}


void error(int errno){
  DEBUGP(F("error" ));
  DEBUGPLN(errno);
}
void dumphex(unsigned char *buf,int len)
{
  for (int i = 0; i < len; i++)
  {
    if (i%16 == 0)
      DEBUGPLN();
    DEBUGP(buf[i],HEX);DEBUGP(" ");
  }
  DEBUGPLN();
}

