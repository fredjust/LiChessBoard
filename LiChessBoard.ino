//-----------------------------------------------------------
// GESTION DU Pseudo CLAVIER 8x8
//-----------------------------------------------------------

// GESTION des LED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
Adafruit_8x8matrix matrix = Adafruit_8x8matrix();


//LES BONNES PIN POUR MON LICHESSBOARD tout en un 50 vert
byte rowPins[8] = {12, 11, 10, 9, 8, 7, 6, 5}; 
byte colPins[8] = {13, 2, 3, 4, 17, 16, 15, 14}; 

//état du plateau contenant les 8 bytes
byte BitMap[8] = {0, 0, 0, 0, 0, 0, 0, 0};

byte NewBitMap[8] = {0, 0, 0, 0, 0, 0, 0, 0};
byte OldBitMap[8][8]; //pour sauvegarder l'ancien état et allumer la derniere case dans le mode LED

byte modeLED=0; //mode LED : allume les deux dernières cases modifiées

//dernière case modifiée
byte lasti=0;   
byte lastj=0;

//stock s'il est nécessaire d'éffacer
bool doDEL=false;

//temps du dernier changement
unsigned long startTime;

//la chaine a passer via USB
String strPos = "0.0.0.0.0.0.0.0.";
int nbPiece = 0; // nombre de pièce sur l'échiquier

//temps minimum en milli seconde avant rescan de l'échiquier
byte const debounceTime =10;

//*********************************************************
// SCAN LE CLAVIER
//*********************************************************
bool getKeys() 
{
  bool keyActivity = false; 

  if ( (millis()-startTime)>debounceTime ) //si le dernier scan n'est pas trop récent
  {               
    // RE INITIALISE LES PORTS A CHAQUE FOIS 
    // CELA PERMET DE LES UTILISER AVEC UN AUTRE MATERIEL
    for (byte r=0; r<8; r++) { pinMode(rowPins[r],INPUT_PULLUP); }  
          
      for (byte c=0; c<8; c++)     
      {
        pinMode(colPins[c],OUTPUT);
        digitalWrite(colPins[c], LOW);  // Begin column pulse output.
        for (byte r=0; r<8; r++) 
        {
          if (!digitalRead(rowPins[r])!=bitRead(BitMap[r],c)) { keyActivity=true; }   //inverser r et c pour inverser ligne colonne   
          bitWrite(BitMap[r], c, !digitalRead(rowPins[r]));  
        }         
        // REMET LE PORT EN ETAT POUR LA PROCHAINE LECTURE
        digitalWrite(colPins[c],HIGH);
        pinMode(colPins[c],INPUT);
      }
     
  }
  return keyActivity;
}

//-----------------------------------------------------------
// effectue les permutations de bits pour renvoyer les bonnes signatures plateau
// varie en fonction des PINS utilisées
//-----------------------------------------------------------
void PermuToBOARD() {
  byte TempoBitMap = 0;
  strPos = "";
  nbPiece=0;
  for (int i = 7; i >= 0; i--)
  {
    for (int b = 7; b >= 0; b--)
    {
      //sauvegarde de l'ancien état
      OldBitMap[i][b]=bitRead(BitMap[i], b);        
     
      bitWrite( TempoBitMap, b, bitRead(BitMap[7 - b], 7-i) );   
      nbPiece=nbPiece+bitRead(BitMap[7-b],7-i);
    }
    strPos = strPos + TempoBitMap;
    strPos = strPos + ".";
    NewBitMap[7 - i] = TempoBitMap;               
  }    
}

//-----------------------------------------------------------
// animation de départ lorsque la position de départ est OK
// allume succésivement les lignes 1 à 8 puis 8 à 1
//-----------------------------------------------------------
byte ImReady() {
  for (int i = 0; i < 8; i++) {
    matrix.clear();
    for (int j = 0; j < 8; j++) {
      matrix.drawPixel(7 - i, j, LED_ON);
    }
    matrix.writeDisplay();  // write the changes we just made to the display
    delay(100);
  }
  for (int i = 0; i < 8; i++) {
    matrix.clear();
    for (int j = 0; j < 8; j++) {
      matrix.drawPixel(i, j, LED_ON);
    }
    matrix.writeDisplay();  // write the changes we just made to the display
    delay(100);
  }
  matrix.clear();
  matrix.writeDisplay();
  return 0;
}

//-----------------------------------------------------------
// Allume les cases ou une piece est passée
// mode dessin du début si le nombre de pièce est de 2
//-----------------------------------------------------------
byte onePiece() {
  
  for (int i = 0; i < 8; i++) {        
    for (int j = 0; j < 8; j++) {
      if (bitRead(BitMap[i], j)==1){        
           
        matrix.drawPixel(i, j, LED_ON);            
        
      }
    matrix.writeDisplay();  // write the changes we just made to the display
    }
  } 
  return 0;
}

//-----------------------------------------------------------
// Allume la case qui vient de changer
// permet d'allumer le dernier mouvement en mode LED = 1
//-----------------------------------------------------------
byte lastSquare() {
  
  for (int i = 0; i < 8; i++) {        
    for (int j = 0; j < 8; j++) {
      if (bitRead(BitMap[i], j)!=OldBitMap[i][j]) {  
        matrix.clear();  
        matrix.drawPixel(i, j, LED_ON); 
        if (OldBitMap[i][j]==0) {matrix.drawPixel(lasti, lastj, LED_ON); }
        if (lasti!=i) {lasti=i;};
        if (lastj!=j) {lastj=j;};
      }    
    }
  } 
  doDEL=true;
  matrix.writeDisplay();  // write the changes we just made to the display
  delay(10);
  return 0;
}

//*********************************************************************
//                          SETUP
//*********************************************************************
void setup()
{
  //initialisation du port serie
  Serial.begin(115200);
  //on dit bonjour
  Serial.println("HELLO WORLD");  
  delay(100);


  
  //initialise la matrice
  matrix.begin(0x70);  
  matrix.setRotation(3);
  matrix.clear();      
  matrix.writeDisplay();
  //matrix.setBrightness(16);
  
  //récupère nombre de pièce
  getKeys();
  PermuToBOARD();  

  //animpation je suis pret
  ImReady();

  // moins de 3 pièces "mode dessin"
  while (nbPiece < 3)
  {
    if ( getKeys() )
    {
      PermuToBOARD();           
      onePiece();
      if (nbPiece==0){
        matrix.clear();
        matrix.writeDisplay();
      }
    }
  }    
 
  // Allume les cases qui contiennent une piece
  // lorsuqu'il y a moins de 16 pieces
  // une piece sur les 4 cases du centre permet de zapper ce mode
   while ((nbPiece < 17) and (strPos !="0.0.0.24.24.0.0.0."))
  {
    if ( getKeys() )
    {
      PermuToBOARD();           
      matrix.clear();
      matrix.drawBitmap(0, 0, NewBitMap, 8, 8, LED_ON);
      matrix.writeDisplay();      
      Serial.println(strPos);               
      Serial.println(nbPiece);  
      //passe en mode LED si on place uniquement les 4 tours  
      if (strPos=="129.0.0.0.0.0.0.129.")
      {
        modeLED=1;
        Serial.println("LED MODE 1");  
        delay(100);
      }
    }
  }    

  // inversion de la signature à la 17° piece
  // alllume maintenant les cases inoccupées de la position de départ 
  for (int i=0;i<8;i++) { NewBitMap[i]=abs(195-NewBitMap[i]); } 
  matrix.clear();
  matrix.drawBitmap(0, 0, NewBitMap, 8, 8, LED_ON);
  matrix.writeDisplay();     
      
  
  // Allume les cases de la position de départ
  // qui ne contiennent pas de piece
  // une piece sur les 4 cases du centre permet de zapper ce mode  
  // passe en mode led s'il manque les 4 tours
  while ((strPos !="195.195.195.195.195.195.195.195.") and (strPos !="0.0.0.24.24.0.0.0."))
  {
    if ( getKeys() )
    {
      //passe en mode led s'il manque les 4 tours
      if (strPos=="66.195.195.195.195.195.195.66.")
      {
        modeLED=1;
        Serial.println("LED MODE 1");  
        delay(100);
      }
      PermuToBOARD();
      //inversion de la signature
      for (int i=0;i<8;i++) { NewBitMap[i]=abs(195-NewBitMap[i]); }
      matrix.clear();
      matrix.drawBitmap(0, 0, NewBitMap, 8, 8, LED_ON);
      matrix.writeDisplay();      
      Serial.println(strPos);               
      Serial.println(nbPiece);     
    }
  }  
    
  // il est pret lance l'animation
  ImReady();  
  Serial.println("I M READY");
}

//*********************************************************************
//                          LOOP
//*********************************************************************

void loop() {
  char carlu = 0;

  // si on recoit des infos sur le port serie
  while (Serial.available() > 0)
  { // tant que des caractères sont en attente
    delay(10);
    carlu = Serial.read();
    if (carlu == 'c') { //on recoit 'c' on efface tout
      matrix.clear();
    }
    if (carlu == 'o') { // on revoit 'o' les coordonnée de deux cases a allumer vont suivre
      matrix.clear();
      //permiere case
      int ligne = Serial.parseInt();
      int colonne = Serial.parseInt();
      if (ligne > 0 && ligne < 9 && colonne > 0 && colonne < 9)
      {
        matrix.drawPixel(8 - ligne, colonne - 1, LED_ON);
      }
      //deuxième case
      ligne = Serial.parseInt();
      colonne = Serial.parseInt();
      if (ligne > 0 && ligne < 9 && colonne > 0 && colonne < 9)
      {
        matrix.drawPixel(8 - ligne, colonne - 1, LED_ON);
      };
    }
    matrix.writeDisplay();
  }

  //si y'a du changement
  if ( getKeys() )
  {
    if (modeLED==1) {lastSquare();}    
    PermuToBOARD();   
    //affiche les infos sur le port serie
    Serial.println( millis() - startTime ); 
    Serial.println(strPos);                 
    startTime = millis();
  }

  //efface une fois au bout de 5 secondes les cases allumées en mode LED 1
  if (doDEL && modeLED==1) {   
    if ( (millis() - startTime) >5000 ) {
      matrix.clear();
      matrix.writeDisplay();
      doDEL=false;      
    }
  }
}
