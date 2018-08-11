#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <avr/interrupt.h>


//#define DEBUG
#define input_pin 2   // broche d'entrée du signal pour le compteur

#define OCIEA 1    // ce bit du registre TIMSK1 est bizarrement non défini dans Arduino, alors qu'il est documenté dans la datasheet

#define pulse_max 22000  //temps max entre 2 impulsions. Freq mini= environ 90 Hz, soit 11 ms, soit 22000 tics d'horloge avec la configuration du timer 
#define COEFF 4          // coeff diviseur à appliquer pour transformer la fréquence en régime (dépend du nombre de bobines et d'aimants)

#define alerte_width 48
#define alerte_height 48
static unsigned char alerte_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
   0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0x06, 0x00, 0x00, 0x00, 0x00, 0x60, 0x0c, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x30, 0x18, 0x00, 0x00,
   0x00, 0x00, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x00, 0x00,
   0x00, 0x00, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x60, 0x00, 0x00,
   0x00, 0x00, 0x0c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x86, 0xc3, 0x00, 0x00,
   0x00, 0x00, 0xc6, 0xc7, 0x00, 0x00, 0x00, 0x00, 0xe3, 0x8f, 0x01, 0x00,
   0x00, 0x00, 0xe3, 0x8f, 0x01, 0x00, 0x00, 0x80, 0xe1, 0x0f, 0x03, 0x00,
   0x00, 0x80, 0xe1, 0x0f, 0x03, 0x00, 0x00, 0xc0, 0xc0, 0x07, 0x06, 0x00,
   0x00, 0xc0, 0xc0, 0x07, 0x06, 0x00, 0x00, 0x60, 0xc0, 0x07, 0x0c, 0x00,
   0x00, 0x60, 0xc0, 0x07, 0x0c, 0x00, 0x00, 0x30, 0xc0, 0x07, 0x18, 0x00,
   0x00, 0x30, 0xc0, 0x07, 0x18, 0x00, 0x00, 0x18, 0xc0, 0x07, 0x30, 0x00,
   0x00, 0x18, 0xc0, 0x07, 0x30, 0x00, 0x00, 0x0c, 0xc0, 0x07, 0x60, 0x00,
   0x00, 0x0c, 0xc0, 0x07, 0x60, 0x00, 0x00, 0x06, 0xc0, 0x07, 0xc0, 0x00,
   0x00, 0x06, 0xc0, 0x07, 0xc0, 0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x01,
   0x00, 0x03, 0x00, 0x00, 0x80, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x03,
   0x80, 0x01, 0x80, 0x03, 0x00, 0x03, 0xc0, 0x00, 0xc0, 0x07, 0x00, 0x06,
   0xc0, 0x00, 0xc0, 0x07, 0x00, 0x06, 0x60, 0x00, 0xc0, 0x07, 0x00, 0x0c,
   0x60, 0x00, 0x80, 0x03, 0x00, 0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x18,
   0x30, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x3f,
   0xf8, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


// les variables pour les compteurs : (vitesse, trip, km total)
unsigned int regime = 0;
const unsigned int regime_max=11000;
const unsigned int regime_alerte=8000; 
unsigned int bargraph=0;
// variables pour mesurer le temps entre 2 impulsions
volatile unsigned int pulse=0;

// variables pour bien positionner l'affichage des valeurs sur l'écran (en fonction du nombre de chiffres)
unsigned char pos_regime = 5;


/* Constructor */
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 9, /* dc=*/ 10, /* reset=*/ 12);  //CS non connecté en réalité




void input_pulse(void) {
  pulse=TCNT1;
  TCNT1=0;
#ifdef DEBUG
  Serial.print("Pulse : ");
  Serial.println(pulse);
#endif
 
  
}

 ISR(TIMER1_COMPA_vect) {
    pulse=0;
#ifdef DEBUG
  Serial.print("COMPA !");
#endif
}

void calcul_regime(void)
{
  if (pulse != 0) {
    regime=(int) ( (60 / COEFF) / ((float)pulse * 0.0000005) );
    regime=regime/10; // on arrondi à la dizaine
    regime=regime*10;
  }
  else {
    regime=0;   
  }
}

/* u8g2.begin() is required and will sent the setup/init sequence to the display */
void setup(void) {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  
   // paramétrage de l'écran OLED
  u8g2.begin();
  // animation de boot
  for (int i=0;i<65;i++) {
    u8g2.firstPage();
    do {
      u8g2.drawBox(64-i,0,2*i,64);
    } while ( u8g2.nextPage() );
  }
  for (int i=65;i>=0;i--) {
    u8g2.firstPage();
    do {
      u8g2.drawBox(64-i,0,2*i,64);
    } while ( u8g2.nextPage() );
  }

  
   // configuration entrée/sortie et interruption
  pinMode(input_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(input_pin), input_pulse, RISING);
    // on active l'interruption sur débordement du timer1 par rapport au registre OCR1A (output compare)
  TCCR1B = 0;
  TCCR1A = 0;
  OCR1A =  pulse_max;   // avec un quartz à 16 MHz et prescaler 1024, cette valeur va provoquer un overflow après 11 ms, ce qui correspond à un régime inférieur à celui du ralenti
  //TIMSK1 |= (1 << TOIE1) ;
  TIMSK1 |= (1 << OCIEA) ;
  // on active le mode 4 CTC (timer1 debord quand il atteint la valeur OCR1A)
  TCCR1B |= (1 << WGM12); // mode CTC 4
  // paramétrage du timer1
  TCCR1B |= (1 << CS11);  // prescaler 8 @ 16 MHz .   1 tic = 0,5µs  
  
  TCNT1=0;

}

/* draw something on the display with the `firstPage()`/`nextPage()` loop*/
void loop(void) {
  // AFFICHAGE SUR L'ECRAN
  u8g2.firstPage();
  do {
    
    
    //affichage du bargraph
    u8g2.drawFrame(0,50,128,14);
    bargraph=(int) ((float)regime/(float)regime_max*128);
    u8g2.drawBox(2,52,bargraph,10);

    //affichage de l'alerte !
    if (regime >= regime_alerte) {
      u8g2.setFont(u8g2_font_logisoso24_tn);
      u8g2.setCursor(5, 35);
      u8g2.print(regime);
      u8g2.drawXBM( 80, 0, alerte_width, alerte_height, alerte_bits);
    }
    else
    {
      // affichage du regime
      u8g2.setFont(u8g2_font_logisoso38_tn);
      u8g2.setCursor(5, 43);
      u8g2.print(regime);
    }
       
  } while ( u8g2.nextPage() );
  
  calcul_regime();
  interrupts();
  delay(250);
  noInterrupts();
}
