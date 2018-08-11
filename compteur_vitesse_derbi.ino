#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <avr/interrupt.h>
#include <EEPROM.h>

//#define DEBUG
// choix entre 2 modes de mesure de la rotation de la roue
// DETECTE_TIERS_DISQUE : on mesure un 1/3 de tour de roue grace au disque de frein (en 3 parties)
// DETECTE_TIERS_2_TETES_VIS : on mesure 1/3 de tour de roue grace au 6 vis de fixation du disque (3 paires de vis) : il y a donc des impulsions supplémentaires à gérer
#define DETECTE_TIERS_DISQUE
//#define DETECTE_TIERS_2_TETES_VIS


// pour programmer initialement le total kilométrique à la valeur de l'ancien compteur à cable
// il faudra transférer le firmware 1 fois avec ce #define, puis ensuite sans. L'EEPROM sera alors programmée avec cette valeur.
//#define INIT_COMPTEUR 10580 

#define OCIEA 1    // ce bit du registre TIMSK1 est bizarrement non défini dans Arduino, alors qu'il est documenté dans la datasheet

// périmètre de la roue en mm
const float tour_roue = 2136;
const unsigned int pulse_1kmh = 40050; // nombre de pulse d'horloge correspondant à 1, à 99, et 0 km/havec les réglages du timer ci-dessous (dans setup)
const unsigned int pulse_99kmh = 405; 
const unsigned int pulse_0kmh = 65535;
#define input_pin 2   // broche d'entrée du signal pour le compteur

// les variables pour les compteurs : (vitesse, trip, km total)
volatile unsigned int vitesse=0;
volatile float trip = 0;
volatile float total = 0;
float old_total = 0;

// variables pour mesurer le temps entre 2 impulsions
volatile unsigned int pulse;
volatile byte compte_pulse = 0;
// variables pour bien positionner l'affichage des valeurs sur l'écran (en fonction du nombre de chiffres)
unsigned char pos_vitesse = 34;
unsigned char pos_total = 8;
unsigned char pos_trip = 75;


/* Constructor */
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 9, /* dc=*/ 10, /* reset=*/ 12);



void calcul_vitesse(void) {
    //calcul des vitesses
   if (pulse >=pulse_99kmh) {vitesse=pulse_1kmh / pulse;} // on evite la division par 0 en cas de débordement du timer (pulse vaut 0 dans ce cas)
    else {vitesse=99;}
    if (vitesse < 10) {
      pos_vitesse = 33;
    } else {
      pos_vitesse = 4;
    }

    //calcul du trip
    if (trip > 999.9) {
      trip = 0;
    }
    if (trip <= 9.9) {
      pos_trip = 86;
    }
    if (trip >= 10) {
      pos_trip = 80;
    }
    if (trip >= 100) {
      pos_trip = 74;
    }


    //calcul du total
    if (total > 99999) {
      total = 0;
    }
    if (total > 9) {
      pos_total = 23;
    } else {
      pos_total = 29;
    }
    if (total > 99) {
      pos_total = 17;
    }
    if (total > 999) {
      pos_total = 11;
    }
    if (total > 9999) {
      pos_total = 5;
    }
}



void input_pulse(void) {

#ifdef DETECTE_TIERS_2_TETES_VIS
  compte_pulse++;
  if (compte_pulse == 1) { // c'est la 1ère impulsion, on lance le timer1 à partir de 0
    TCNT1 = 0;
  }
  if (compte_pulse == 3) {
    pulse = TCNT1;
    compte_pulse = 0;
    trip += tour_roue / 3000000 ;  // conversion mm en km + facteur 1/3 car on compte les tiers de tours de roue
    total += tour_roue / 3000000;
  }
#endif
#ifdef DETECTE_TIERS_DISQUE
    pulse=TCNT1;
    TCNT1=0;
    trip += tour_roue / 3000000 ;  // conversion mm en km + facteur 1/3 car on compte les tiers de tours de roue
    total += tour_roue / 3000000;

#endif
#ifdef DEBUG
  Serial.print("Pulse ");
  Serial.print(compte_pulse);
  Serial.print(" : ");
  Serial.println(TCNT1);
  Serial.println(total);
  Serial.println(trip);
#endif
}

//ISR(TIMER1_OVF_vect) {
//#ifdef DEBUG
//  Serial.println("OVF !");
//#endif
//}

ISR(TIMER1_COMPA_vect) {
#ifdef DEBUG
  Serial.println("COMPA !");
#endif
  // on roule très lentement !!!!
  pulse = pulse_0kmh;
  compte_pulse = 0;
  // on en profite pour mémoriser les nouvelles données (compteur total) dans l'EEPROM
  // on mémorise le total s'il a augmenté de plus de 500 m
  EEPROM.get(0, old_total);
  if (total - old_total >= 0.5) {
    old_total = total;
    EEPROM.put(0, total);
  }
#ifdef INIT_COMPTEUR
  EEPROM.put(0, (float) INIT_COMPTEUR);
#endif

}

/* u8g2.begin() is required and will sent the setup/init sequence to the display */
void setup(void) {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  // on lit le compteur kilométrique total en EEPROM (adresse 0)
  EEPROM.get(0, total);
  old_total = total;

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

  // on active l'interruption sur débordement du timer1 par rapport au registre OCR1A (output compare)
  TCCR1B = 0;
  TCCR1A = 0;
  OCR1A =  pulse_1kmh;   // avec un quartz à 16 MHz et prescaler 1024, cette valeur va provoquer un overflow après 2,5632s, ce qui correspond à moins de 1 km/h avec la roue de 68 cm de diamètre (on mesure 1/3 de tour de roue)
  //TIMSK1 |= (1 << TOIE1) ;
  TIMSK1 |= (1 << OCIEA) ;
  // on active le mode 4 CTC (timer1 debord quand il atteint la valeur OCR1A)
  TCCR1B |= (1 << WGM12); // mode CTC 4
  // on divise la fréquence d'horloge par 1024 @ 16 MHz : 1 tic=64 µs
  TCCR1B |= (1 << CS12);
  TCCR1B |= (1 << CS10);
  
  TCNT1=0;
  // configuration entrée/sortie et interruption
  pinMode(input_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(input_pin), input_pulse, FALLING);

}

/* draw something on the display with the `firstPage()`/`nextPage()` loop*/
void loop(void) {

  


  // AFFICHAGE SUR L'ECRAN

  u8g2.firstPage();

  do {

    // affichage de l'unité de mesure de la vitesse
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(75, 17, "km/h");


    // affichage dela vitesse
    u8g2.setFont(u8g2_font_logisoso50_tn);
    u8g2.setCursor(pos_vitesse, 50);
    u8g2.print(vitesse);

    // affichage des unités des compteurs kilométriques
    u8g2.setFont(u8g2_font_courB08_tf);
    u8g2.drawStr(42, 64, "km");
    u8g2.drawStr(115, 64, "km");

    // affichage du trip
    u8g2.setCursor(pos_trip, 64);
    u8g2.print(trip);

    //affichage du total
    u8g2.setCursor(pos_total, 64);
    u8g2.print((unsigned int)total);

 #ifdef DEBUG
 // pour debug : affichage du la valeur pulse
    u8g2.setCursor(70, 32);
    u8g2.print((unsigned int)pulse);
 #endif
  } while ( u8g2.nextPage() );
 
  delay(750);
  calcul_vitesse();

}







