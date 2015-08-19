#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPI.h>

#include "pitches.h"  //Note de musique donné par l'IDE Arduino

#include <SimpleSDAudio.h>

#define	BLACK           0x0000
#define	BLUE            0x001F
#define BLUEF           0x1111       // 0x1111  bleue froid
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define sclk 13
#define mosi 11
#define cs   10
#define dc   6
#define rst  8

#define SPI_SCK 13
#define SPI_DI  12
#define SPI_DO  11
#define SD_CS    7
#define SD_active 5

const int basPinx = 4;
const int hautPinx = A2;
const int basPiny = 3;
const int hautPiny = A3;
const int entrePin = A1;

int slgx = 0;        //  Coordonné du point sélecteur dans le clavier virtuel en axe X.
int slgy = 11;       //  Coordonné du point sélecteur dans le clavier virtuel en axe Y.
int slgmy = 1;       //  Lingne à la quelle se trouve le sélecteur de ligne dans les menus.
int slgxlast;        //  Coordonné antèrieur du point sélecteur dans le clavier virtuel en axe X.
int slgylast;        //  Coordonné antèrieur du point sélecteur dans le clavier virtuel en axe Y.
int entre = 0;       //  Ouvre se qui est sélectionné.
int horloge = 0;     //  Ouvre menu où se trouve l'heure.
int exploreur = 0;   //  Ouvre l'explorateur de fihier et de dossier dans la carte SD.
int editeur = 0;     //  Ouvre l'éditeur de text.
int executeur = 0;   //  Avec quelle fonction il doir ouvrire le fichier.
int jeuxmenu = 0;    //  Ouvre le menu des jeux.

  int slgjx = 48;
  int slgjy = 32;
  int lastslgjx  = 30; 
  int lastslgjy = 20; 
  int adversaire_x = 2; 
  int adversaire_y = 2; 
  int last_adv_y = 0;
  int deplacement_x = 1;
  int deplacementadv = 3;  // Variable de dificulté
  int adversaire_vague = 0;
  int nmbr_adversaire_vague = 3; // Variable pour la dificultée.
  int point = 0; 
  int lastpoint = 0; 
  int vie = 5; // Variable pour les scores
  int degasubit = 0;  // Point de vie du vaisseau
  int vietotalp = 50;
  int vietotaln = 2;
  int missile = 0; 

#define SD_CS 7

File txtFile;
File root;
File musique;
File musiqueroot;

// Notes lors d'un impacte
int melody[] = {NOTE_C4, NOTE_G3};

int noteDurations[] = {2, 2};

char Caractere[34] = "azertyuiopqsdfghjklmwxcvbn ,.?!:";

// Variable pour les informations extréte des fichiers bmp
int bmpWidth, bmpHeight;
uint8_t bmpDepth, bmpImageoffset;


int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

uint16_t adv1_x = 89; uint16_t adv1_y;
uint16_t adv2_x = 89; uint16_t adv2_y;
uint16_t adv3_x = 89; uint16_t adv3_y;
uint16_t adv4_x = 89; uint16_t adv4_y;
uint16_t adv5_x = 89; uint16_t adv5_y;

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

void setup(void) {
  
   SdPlay.setSDCSPin(7);
  
   if (!SdPlay.init(SSDA_MODE_FULLRATE | SSDA_MODE_STEREO | SSDA_MODE_AUTOWORKER)) {     // Bruitage du jeux plus tard
    Serial.println("Erreur d'initialisation du lecteur audio");
   }    

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(tft.getRotation() + 1);
  
  pinMode(SD_active, OUTPUT);
  digitalWrite(SD_active,HIGH);
  
  Serial.begin(115200);
   
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  
  pinMode(basPinx, INPUT_PULLUP); 
  pinMode(hautPinx, INPUT_PULLUP);    
  pinMode(basPiny, INPUT_PULLUP); 
  pinMode(hautPiny, INPUT_PULLUP); 
  pinMode(entrePin, INPUT_PULLUP); 

  // initialisation de l'écran

  tft.fillScreen(BLACK);
  
  delay(100);

  SD.begin(SD_CS);    

  
  
}

void loop() {
  
  lastslgjx = slgjx;
  lastslgjy = slgjy;
  
  touche();
   
  if (slgjx < 12)  { slgjx = 12; } 
  if (slgjx > 93) { slgjx = 93;}
  if (slgjy < 5)  { slgjy = 5; } 
  if (slgjy > 59) { slgjy = 59;} 
  
  Vaisseau(slgjx, slgjy);
  
  if ((lastslgjx != slgjx) || (lastslgjy != slgjy))   //Efface vaisseau.
  {
  Vaisseaufant(lastslgjx , lastslgjy);
  }
  
  point++;
  
  Adversaire1(deplacementadv);
  Adversaire2(deplacementadv);
  Adversaire3(deplacementadv);
  Adversaire4(deplacementadv);
  Adversaire5(deplacementadv);

/*  
  if (lastpoint + 50 < point)  
  {
   lastpoint = point;
   deplacementadv += 1;
  }
   */
   
   Missile();
   
   if (degasubit != 0)
   {
     for (int thisNote = 0; thisNote < 2; thisNote++) 
       {
        int noteDuration = 200 / noteDurations[thisNote];
        tone(9, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(9);
      }
  }
   
   Barre_vie(degasubit);
   
   degasubit = 0;


  }

   
       void Vaisseau(uint16_t vai_x, uint16_t vai_y)
   {
   tft.drawTriangle(vai_x - 8, vai_y + 4 , vai_x - 8, vai_y - 4, vai_x, vai_y, BLUE);
   }
   
       void Vaisseaufant(uint16_t vai_x, uint16_t vai_y)
   {
   tft.drawTriangle(vai_x - 8, vai_y + 4 , vai_x - 8, vai_y - 4, vai_x, vai_y, BLACK);
   }
   
      void Missile()
    {
      
     missile++;
   
     if (missile == 4)
     {
     missile = 0;
      
     tft.drawLine(slgjx, slgjy, 90, slgjy, WHITE);
     }
     
     if(lastslgjy < slgjy)
     {
     tft.fillRect(slgjx, slgjy - 1, 94 - slgjx, slgjy - 4, BLACK);
     }
     
     if(lastslgjy > slgjy)
     {
     tft.fillRect(slgjx, slgjy + 1, 94 - slgjx, slgjy + 4, BLACK);
     } 
    
     
    }
   
       void Adversaire1(uint16_t depla1)
   {
   uint16_t lastadv1_x = (adv1_x) , lastadv1_y = adv1_y;    // Refaire
   
   adv1_x = adv1_x - depla1;
   
        if (adv1_x <= 8)
      {
        adv1_x = 89;
        adv1_y = random(1, 30); 
      }
      
       if ((slgjy > adv1_y - 3) && (slgjy < adv1_y + 3))
      {
        adv1_x = 89;
        adv1_y = random(1, 30); 
        point++;
      } 
      
   if (((slgjx > adv1_x) && (slgjx < adv1_x + 8)) && ((slgjy > adv1_y) && (slgjy < adv1_y + 6)))
   {
   degasubit++;  
   }
      
   tft.drawRoundRect(adv1_x, adv1_y, 8, 6, 2, RED);
   tft.drawRoundRect(lastadv1_x, lastadv1_y, 8, 6, 2, BLACK);

   }
   
      void Adversaire2(uint16_t depla2)
   {
   uint16_t lastadv2_x = adv2_x , lastadv2_y = adv2_y;
   adv2_x = adv2_x - depla2;
   
        if (adv2_x <= 5)
      {
        adv2_x = 89;   
        adv2_y = random(1, 60);               
      }
      
        if ((slgjy > adv2_y - 3) && (slgjy < adv2_y + 3))
      {
        adv2_x = 89;
        adv2_y = random(1, 30); 
        point++;
      }
      
    if (((slgjx > adv2_x) && (slgjx < adv2_x + 8)) && ((slgjy > adv2_y) && (slgjy < adv2_y + 6)))
   {
   degasubit++;  
   }
      
   tft.drawRoundRect(adv2_x, adv2_y, 8, 6, 2, GREEN);
   tft.drawRoundRect(lastadv2_x, lastadv2_y, 8, 6, 2, BLACK);
   }
   
      void Adversaire3(uint16_t depla3)
   {
   uint16_t lastadv3_x = adv3_x , lastadv3_y = adv3_y;  
   adv3_x = adv3_x - depla3;
   
        if (adv3_x <= 6)
      {
        adv3_x = 89;
        adv3_y = random(10, 60); 
      }
      
        if ((slgjy > adv3_y - 3) && (slgjy < adv3_y + 3))
      {
        adv3_x = 89;
        adv3_y = random(1, 30); 
        point++;
      } 
      
   if (((slgjx > adv3_x) && (slgjx < adv3_x + 8)) && ((slgjy > adv3_y) && (slgjy < adv3_y + 6)))
   {
   degasubit++;  
   }
      
   tft.drawRoundRect(adv3_x, adv3_y, 8, 6, 2, WHITE);
   tft.drawRoundRect(lastadv3_x, lastadv3_y, 8, 6, 2, BLACK);
   }
   
      void Adversaire4(uint16_t depla4)
   {
   uint16_t lastadv4_x = adv4_x , lastadv4_y = adv4_y;  
   adv4_x = adv4_x - depla4;
   
        if (adv4_x <= 4)
      {
        adv4_x = 89;
        adv4_y = random(20, 60); 
      }
      
        if ((slgjy > adv4_y - 3) && (slgjy < adv4_y + 3))
      {
        adv4_x = 89;
        adv4_y = random(1, 30); 
        point++;
      } 
      
   if (((slgjx > adv4_x) && (slgjx < adv4_x + 8)) && ((slgjy > adv4_y) && (slgjy < adv4_y + 6)))
   {
   degasubit++;  
   }
      
   tft.drawRoundRect(adv4_x, adv4_y, 8, 6, 2, YELLOW);
   tft.drawRoundRect(lastadv4_x, lastadv4_y, 8, 6, 2, BLACK);
   }
   
      void Adversaire5(uint16_t depla5)
   {
   uint16_t lastadv5_x = adv5_x , lastadv5_y = adv5_y;
   adv5_x = adv5_x - depla5;
   
        if (adv5_x <= 10)
      {
        adv5_x = 89;
        adv5_y = random(30, 60); 
      }
      
        if ((slgjy > adv5_y - 3) && (slgjy < adv5_y + 3))
      {
        adv5_x = 89;
        adv5_y = random(1, 30);
        point++; 
      } 
      
   if (((slgjx > adv5_x) && (slgjx < adv5_x + 8)) && ((slgjy > adv5_y) && (slgjy < adv5_y + 6)))
   {
   degasubit++;  
   }
      
   tft.drawRoundRect(adv5_x, adv5_y, 8, 6, 2, MAGENTA);
   tft.drawRoundRect(lastadv5_x, lastadv5_y, 8, 6, 2, BLACK);
   }
   
   
     void Barre_vie(uint8_t dega)
 {
  vietotalp = vietotalp - dega;
  vietotaln = vietotaln + dega;
  tft.fillRect(4, 7, 3, vietotalp, RED); 

  tft.fillRect(4, 60, 3, -vietotaln, BLACK); 
  dega = 0;
  
      if (vietotalp < 1)  
     { 
      vietotalp = 50; 
      tft.fillScreen(BLACK);
      tft.setCursor(2, 20);
      tft.setTextSize(2);
      tft.setTextColor(BLUEF,BLACK);  
      tft.println("GAME");
      tft.println("OVER!");
      
      delay(2000);
      tft.fillScreen(BLACK);
     }
      
    if (vietotaln > 49)  { vietotaln = 2; }
}  
   
   
   

void touche ()
{

 
     slgxlast = slgx;
     slgylast = slgy; 
    
  int lastvarbasy = 0;
  int varbasy = digitalRead(basPiny);
    
     if (varbasy != lastvarbasy){
      if (varbasy == HIGH)
      {
       slgmy++;
       
       slgjy += 2;
       
       slgy = slgy + 10;
      }
   lastvarbasy = varbasy;
     }
     
    int lastvarhauty = 0;  
    int varhauty = digitalRead(hautPiny);
   
     if (varhauty != lastvarhauty){
      if (varhauty == HIGH)
      {
        slgy = slgy - 10;
        
        slgjy -= 2;
      
        if (horloge == 0 && exploreur == 0)
        {
        if (slgmy == 0)  {slgmy = 5;} 
        if (slgmy == 2)  {slgmy = 7;}
        if (slgmy == 4)  {slgmy = 1;}
        if (slgmy == 6)  {slgmy = 3;}
        }
        
        if (exploreur == 1)
        {
        if (slgmy == 0)  {slgmy = 5;} 
        if (slgmy == 2)  {slgmy = 7;}
        if (slgmy == 4)  {slgmy = 1;}
        if (slgmy == 6)  {slgmy = 3;} 
        }
        
        if (horloge == 1)
        {
        if (slgmy == 0)  {slgmy = 3;}
        if (slgmy == 2)  {slgmy = 5;}
        if (slgmy == 4)  {slgmy = 1;}
        }
              
      }
      
   lastvarhauty = varhauty;
     }
     
     int lastvarbasx = 0;
  int varbasx = digitalRead(basPinx);
    
     if (varbasx != lastvarbasx){
      if (varbasx == HIGH)
      {
        slgx = slgx - 12;
        
        slgjx -= 2;
      }
   lastvarbasx = varbasx;
     }
     
    int lastvarhautx = 0;  
    int varhautx = digitalRead(hautPinx);
   
     if (varhautx != lastvarhautx){
      if (varhautx == HIGH)
      {
         slgx = slgx + 12;
  
         slgjx += 2;
      }
      
   lastvarhauty = varhauty;
     }
     
     
     
     
    int lastvarentre = 0;
    int varentre = digitalRead(entrePin);
   
     if (varentre != lastvarentre){
      if (varentre == HIGH)
      {
       entre++;
      }
       lastvarentre = varentre;
    }
    
    //delay(10);
}


