# Derbi-Dashboard
Tableau de bord pour moto 50 cm3 Derbi Senda (ou "équivalent)


Le circuit imprimé est le même pour les 3 parties, mais il y a eu quelques oublis
Compteur en 3 parties indépendantes, chacune avec un écran OLED 1.3''
- compteur de vitesse (capteure inductif fixé sur la fourche et qui détecte le passage des têtes de vis du disque de frein). Le compteur kilométrique est stockéen EEPROM. Le compteur journalier est remis à zéro à chaque démarrage)
- compte-tours : fil issu de l'alternateur, avec diode de redressement
- voyants : phares / clignotants / niveau huile (flotteur installé dans le réservoir) / température / essence (avec le capteur d'origine toujours cablé avec son ampoule car il ne fonctionne pas sans)


Oublis sur le circuit imprimé : 
- diviseur de tension par 2 (2 résistances de 10 kohms) sur le fil du feu de route


REMARQUE : le circuit électrique de la moto a été légèrement modifié
- le 12V alternatif (pour les feux) est maintenant sur le 12V issu du régulateur (l'éclairage est stable même au ralenti)
- le 12 alternatif est utilisé pour le compte-tour mais il est redressé simple alternance (une diode) avant d'aller au tableau de bord. Ce signal n'est pas assez propre au ralenti pour être exploité mais il va très bien pour le reste du temps.
- le capteur de niveau d'huile n'est plus utilisé, un flotteur (on/off) est installé sur le bocal (on peut sans doute s'inspirer de la gestion du niveau d'essence pour faire fonctionner le capteur d'origine)

FONCTIONNEMENT DES CAPTEURS
Les capteurs d'huile et d'essence ne fonctionnent pas sans l'ampoule.
Pour l'essence, j'ai mesuré les tensions suivantes (mesurées entre la masse et le point milieu)
  - réservoir vide : environ 5 à 6 V (ce qui laisse 6V pour l'ampoule qui brille un peu)
  - réservoir plein : environ 8 V (il reste donc 4V pour l'ampoule, ce qui est insuffisant)

+12V. ---------
              |
              |
            Ampoule
              |
              |----------------- mesure de la tension
              |
            Capteur
              |
              |
 0V -----------
