//-----------------------------------------------------------
// GESTION DU Pseudo CLAVIER 8x8
//-----------------------------------------------------------

#include "Key.h"
#include "Keypad.h"

// GESTION des LED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

bool etatchange = false;

//define the cymbols on the buttons of the keypads
char hexaKeys[8][8] = {
  {'1', '2', '3', '4', '5', '6', '7', '8'},
  {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'},
  {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'},
  {'&', 'B', '#', '{', '(', '-', 'B', '_'},
  {'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'},
  {'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'},
  {'q', 'r', 's', 't', 'u', 'v', 'w', 'x'},
  {'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X'}
};

//byte rowPins[8] = {12, 11, 10, 9, 8, 7, 6, 13}; 
//byte colPins[8] = {14, 15, 16, 17, 4, 3, 2, 5}; 

//LES BONNES PIN POUR MON LICHESSBOARD tout en un 50 vert
byte rowPins[8] = {12, 11, 10, 9, 8, 7, 6, 5}; 
byte colPins[8] = {13, 2, 3, 4, 17, 16, 15, 14}; 

byte NewbitMap[8] = {0, 0, 0, 0, 0, 0, 0, 0};

//initialize an instance of class NewKeypad
Keypad ChessBoard = Keypad( makeKeymap(hexaKeys), rowPins, colPins, 8, 8);

unsigned long startTime;

String strPos = "0.0.0.0.0.0.0.0.";
int nbPiece = 0;

//-----------------------------------------------------------
// effectue les permutations de bits pour renvoyer les bonnes signatures plateau
//-----------------------------------------------------------
void PermuToBOARD() {
  byte TempoBitMap = 0;
  strPos = "";
  nbPiece=0;
  for (int i = 7; i >= 0; i--)
  {
    for (int b = 7; b >= 0; b--)
    {
      // bitRead(ChessBoard.bitMap[gauche/droite b/7-b],blancs/noirs i/7-i)           
      bitWrite( TempoBitMap, b, bitRead(ChessBoard.bitMap[7 - b], 7-i) );   
      nbPiece=nbPiece+bitRead(ChessBoard.bitMap[7-b],7-i);
    }
    strPos = strPos + TempoBitMap;
    strPos = strPos + ".";
    NewbitMap[7 - i] = TempoBitMap;    
  }    
}

//-----------------------------------------------------------
// animation de départ lorsque la position de départ est OK
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
//-----------------------------------------------------------
byte onePiece() {
  
  for (int i = 0; i < 8; i++) {        
    for (int j = 0; j < 8; j++) {
      if (bitRead(ChessBoard.bitMap[i], j)==1){        
           
        matrix.drawPixel(i, j, LED_ON);            
        
      }
    matrix.writeDisplay();  // write the changes we just made to the display
    }
  } 
  return 0;
}


//******************************************************************************************
//                          SETUP
//******************************************************************************************
void setup()
{
  //initialisation du port serie
  Serial.begin(115200);
  //on dit bonjour
  Serial.println("HELLO WORLD");  
  delay(100);

  //anti rebond et appuis long
  ChessBoard.setDebounceTime(10);
  ChessBoard.setHoldTime(2000);
  
  
  //initialise la matrice
  matrix.begin(0x70);  
  matrix.setRotation(3);
  matrix.clear();      
  matrix.writeDisplay();
  //matrix.setBrightness(16);
  
  //récupère nombre de pièce
  ChessBoard.getKeys();
  PermuToBOARD();  

  //animpation je suis pret
  ImReady();

  // moins de 3 pièces "mode dessin"
  while (nbPiece < 3)
  {
    if ( ChessBoard.getKeys() )
    {
      PermuToBOARD();           
      onePiece();
      if (nbPiece==0){
        matrix.clear();
        matrix.writeDisplay();
      }
    }
  }    

  //-----------------------------------------------------------
  // Allume les cases qui contiennent une piece
  // lorsuqu'il y a moins de 16 pieces
  // une piece sur les 4 cases du centre permet de zapper ce mode
  //-----------------------------------------------------------  
  while ((nbPiece < 17) and (strPos !="0.0.0.24.24.0.0.0."))
  {
    if ( ChessBoard.getKeys() )
    {
      PermuToBOARD();           
      matrix.clear();
      matrix.drawBitmap(0, 0, NewbitMap, 8, 8, LED_ON);
      matrix.writeDisplay();      
      Serial.println(strPos);               
      Serial.println(nbPiece);     
    }
  }    

  // inversion de la signature à la 17° piece
  // alllume maintenant les cases inoccupées de la position de départ 
  for (int i=0;i<8;i++) { NewbitMap[i]=abs(195-NewbitMap[i]); } 
  matrix.clear();
  matrix.drawBitmap(0, 0, NewbitMap, 8, 8, LED_ON);
  matrix.writeDisplay();     
      
  //-----------------------------------------------------------
  //  Allume les cases de la position de départ
  //  qui ne contiennent pas de piece
  // une piece sur les 4 cases du centre permet de zapper ce mode
  //-----------------------------------------------------------
  while ((strPos !="195.195.195.195.195.195.195.195.") and (strPos !="0.0.0.24.24.0.0.0."))
  {
    if ( ChessBoard.getKeys() )
    {
      PermuToBOARD();
      //inversion de la signature
      for (int i=0;i<8;i++) { NewbitMap[i]=abs(195-NewbitMap[i]); }
      matrix.clear();
      matrix.drawBitmap(0, 0, NewbitMap, 8, 8, LED_ON);
      matrix.writeDisplay();      
      Serial.println(strPos);               
      Serial.println(nbPiece);     
    }
  }  
    
  // il est pret lance l'animation
  ImReady();  
}

//******************************************************************************************
//                          LOOP
//******************************************************************************************

void loop() {
  char carlu = 0;

  // si on recoit des infos sur le port serie

  while (Serial.available() > 0)
  { // tant que des caractères sont en attente
    delay(10);
    carlu = Serial.read();
    //Serial.print(carlu); //puis on le renvoi à l’expéditeur tel quel
    if (carlu == 'c')
    {
      matrix.clear();
    }
    if (carlu == 'o')
    {
      matrix.clear();
      int ligne = Serial.parseInt();
      int colonne = Serial.parseInt();
      if (ligne > 0 && ligne < 9 && colonne > 0 && colonne < 9)
      {
        matrix.drawPixel(8 - ligne, colonne - 1, LED_ON);
      }
      ligne = Serial.parseInt();
      colonne = Serial.parseInt();
      if (ligne > 0 && ligne < 9 && colonne > 0 && colonne < 9)
      {
        matrix.drawPixel(8 - ligne, colonne - 1, LED_ON);
      };
    }
    matrix.writeDisplay();
  }

  if ( ChessBoard.getKeys() )
  {
    PermuToBOARD();
   
    //affiche les infos sur le port serie
    Serial.println( millis() - startTime ); 
    Serial.println(strPos);                 
    startTime = millis();
  }
}

