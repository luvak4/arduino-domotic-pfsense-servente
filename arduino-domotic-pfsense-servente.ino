//================================================================
// Halt, reboot, check status of a pfSense server through Arduino 
// and radio modules. 
// Project is divided in three parts: "keyboard", "servente" and "display".
//
// author: luvak4@gmail.com
//================================================================
//
/////////////////////////////////////
//// this is the "SERVENTE" code ////
/////////////////////////////////////
#include <VirtualWire.h>
// max lenght of my message
const int MSG_LEN = 13;
// position of character to change
const int POSIZIONE_CARATT = 11;
// LEDs
const int pinLED =13;
const int pinLEDrosso =2;
const int pinLEDgiallo =3;
const int pinLEDverde =4;
const int pinLEDblu =5;
// timing loop
int dutyCycle = 0;
unsigned long int Pa;
unsigned long int Pb;
// radio modules
const int transmit_pin = 12; 
const int receive_pin = 11; 
uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
//
int pfSenseInternalStep=1;
int seconds=0;
// msg "received command"
//////////////////////////////////////123456789012 
 char msgTxComandoRicevuto[13]  = "statPFSE0007";
// prefix to transmit
///////////////////////////123456789012
char msgTxStatoServer[13]="statPFSE0000";
//
String stringaRX;
//
//================================
// setup
//================================
void setup() {
  // setup serial to/from pfSense
  Serial.begin(9600);
  Serial.flush();
  // LEDs
  pinMode(pinLED, OUTPUT);
  pinMode(pinLEDrosso, OUTPUT);
  pinMode(pinLEDgiallo, OUTPUT);
  pinMode(pinLEDverde, OUTPUT);
  pinMode(pinLEDblu, OUTPUT);
  // radio tx rx
  vw_set_tx_pin(transmit_pin);
  vw_set_rx_pin(receive_pin);  
  vw_setup(2000);      
  vw_rx_start(); 
}
//================================
// loop
//================================
void loop() {
  //--------------------------------
  // time subdivision
  //--------------------------------
  unsigned long int Qa;
  unsigned long int Qb;
  int DIFFa;
  int DIFFb;
  int Xa;
  int Xb;
  //
  dutyCycle += 1;
  if (dutyCycle > 9){
    dutyCycle = 0;
  }
  if (dutyCycle > 0){
    Qa=millis();
    if (Qa >= Pa){
      DIFFa=Qa-Pa;
      Xa = DIFFa - 25;
      if (Xa >= 0){
	Pa = Qa;
	//--------------------------------
	// every 0.025 Sec
	//--------------------------------
      }
    } else {
      Pa = Qa - Xa;
    }
  } else {  
    Qb=millis();
    if (Qb >= Pb){
      DIFFb=Qb-Pb;
      Xb = DIFFb - 1000;
      if (Xb >= 0){
	Pb = Qb - Xb;
	//--------------------------------
	// every second
	//--------------------------------
	//
	seconds+=1;
	// every 300 seconds automatic ping to google
	if (pfSenseInternalStep==9){
	    if(seconds > 300){
	      pfSenseInternalStep=12;
	      seconds=0;
	    }
	}
	//--------------------------------
	// BEGIN message received
	//--------------------------------
	if (vw_get_message(buf, &buflen)){
    vw_rx_stop(); // disable rx section
	  //
	  stringaRX="";
	  //
	  // retriving which pushbutton
	  // was pressed (ir-keyboard)
	  for (int i = 1; i <= POSIZIONE_CARATT; i++){
	    stringaRX += char(buf[i-1]);
	  }
	  if (stringaRX=="pulsPFSE000"){
	    switch (buf[POSIZIONE_CARATT]){
	    case '1':
	      // HALT
	      txRicevutoComando();
	      if(pfSenseInternalStep==9){
		pfSenseInternalStep=3;
	      }
	      break;
	    case '2':
	      // REBOOT
	      txRicevutoComando();
	      if(pfSenseInternalStep==9){
		pfSenseInternalStep=6;
	      }
	      break;
	    case '3':
	      // EXIT\N *** secondo me non serve
	      seconds=0;
	      //
	      txRicevutoComando();
	      if(pfSenseInternalStep==9){
		pfSenseInternalStep=11;
	      }
	      if(pfSenseInternalStep==1){
		pfSenseInternalStep=11;
	      }
	      break;
	    case '4':
	      // PING
	      txRicevutoComando();
	      if(pfSenseInternalStep==9){
		pfSenseInternalStep=12;
	      }
	      break;
      case '5':
        // \N a capo per sincronizzare
        seconds=0;
        if(pfSenseInternalStep==9){
          pfSenseInternalStep=16;/////////
        }
        if(pfSenseInternalStep==1){
          pfSenseInternalStep=16;/////////
        }
	    }
	  }
   vw_rx_start(); // disable rx section
	}
	//--------------------------------
	// END message received
	//--------------------------------
	//
	ParteSeriale();
      }
    } else {
      Pb = Qb;
    }      
  }
}
/*
  ================================
  ! Phase ! description
  ================================
  !  1    ! PrimoAvvio
  !  2    ! InAccensione
  --------------------------------
  !  3    ! Spegni
  !  4    ! Sp01
  !  5    ! Sp02
  --------------------------------
  !  6    ! Riavvia
  !  7    ! Ri01
  !  8    ! Ri02
  --------------------------------
  !  9    ! Acceso
  ! 10    ! Spento
  ! 11    ! Sync Arduino/pfSsense
  ! 12    ! ping
  ================================
*/ 
void ParteSeriale(){
  switch (pfSenseInternalStep){
  case 1:
    //--------------------------------
    // first start: ignition
    //--------------------------------
    if (Serial.available() > 0) {
      if (Serial.find("PC Engines ALIX")){
	// pfSense is in ignition state
	pfSenseInternalStep=2;
	txPfsenseStatusToTransmit('1');
      }
    }
    break;
  case 2:
    //--------------------------------
    // pfsense is ON (fully functional)
    //--------------------------------
    if (Serial.available() > 0 ) {
      if (Serial.find("Enter an option")){
	pfSenseInternalStep=9;
	txPfsenseStatusToTransmit('2');
      }
    }
    break;	  
  case 3:
    //--------------------------------
    // Request to turn off pfSense
    //--------------------------------
    Serial.write("6\n");
    pfSenseInternalStep=4;
    break;	  	  
  case 4:
    if (Serial.available()) {
      //Do you want to proceed [y|n]?"
      if (Serial.find("proceed")){
	pfSenseInternalStep=5;
	Serial.write("y\n"); // \n = ritorno a capo
	// pfSense in shutdown
	txPfsenseStatusToTransmit('3');
      }
    }
    break;	  
  case 5:
    if (Serial.available()) {
      //The operating system has halted.
      //Please press any key to reboot.
      if (Serial.find("has halted")){
	delay(1000); // for safety
	pfSenseInternalStep=1;
	// pfSense is OFF
	txPfsenseStatusToTransmit('4');
      }
    }
    break;	  
  case 6:
    //--------------------------------
    // reboot
    //--------------------------------
    Serial.write("5\n");
    pfSenseInternalStep=7;
    break;	  	  
  case 7:
    if (Serial.available()) {
      if (Serial.find("proceed")){
	pfSenseInternalStep=8;
	Serial.write("y\n");
	// pfSense in shutdown
	//msgStatoServer = prefissoStatoServer + "3";
	txPfsenseStatusToTransmit('3');
      }
    }
    break;	  
  case 8:
    if (Serial.available()) {
      if (Serial.find("Rebooting")){
	pfSenseInternalStep=1;
	// pfSense OFF
	txPfsenseStatusToTransmit('4');
      }
    }
    break; 
  case 11:
    //--------------------------------
    // sync Arduino -> pfSense
    //--------------------------------
    Serial.write("exit\n");
    delay(2000); 
    Serial.write("\n");
    delay(5000);
    pfSenseInternalStep=2;
    // attempt sync
    txPfsenseStatusToTransmit('0');
    break;        
  case 12:
    //--------------------------------
    // ping on google
    //--------------------------------
    Serial.write("7\n");
    pfSenseInternalStep=13;
    break;   
  case 13:
    if (Serial.available()) {
      if (Serial.find("or IP address")){
	pfSenseInternalStep=14;     
      }
    }
    break;
  case 14:
    Serial.write("www.google.it\n");
    pfSenseInternalStep=15;
    break;   
  case 15:  
    if (Serial.available()) {
      if (Serial.find("0.0% packet")){
	// internet OK
	txPfsenseStatusToTransmit('5');
      } else {
	// internet KO
	txPfsenseStatusToTransmit('6');
      }
      pfSenseInternalStep=16;
    } 
    break;
  case 16:
    // "push a key..."
    Serial.write("\n");
    pfSenseInternalStep=9;
    break;
  }
}
//================================
// send "command receive" to "display"
//================================
void txRicevutoComando(){
  digitalWrite(pinLED, HIGH);
  vw_rx_stop(); // disable rx section
  vw_send((uint8_t *)msgTxComandoRicevuto,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone
  vw_rx_start(); // enable rx section
  digitalWrite(pinLED, LOW);
}
void txPfsenseStatusToTransmit(char charState){
  switch (charState){
  case '0':
    // attempt sync
    turnOFFleds();
    break;
  case '4':
    // pfSense is OFF
    digitalWrite(pinLEDrosso,HIGH);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW);
    break;  
  case '1'://
    // pfSense is in ignition state
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,HIGH);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW); 
    break;
  case '3'://
    // pfSense in shutdown
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,HIGH);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW);  
    break;     
  case '2'://
    // pfSense ON
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,HIGH);
    digitalWrite(pinLEDblu,LOW);
    break; 
  case '6'://
    // internet KO
    turnOFFleds();
    delay(1000);    
    //
    digitalWrite(pinLEDrosso,HIGH);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,HIGH);
    digitalWrite(pinLEDblu,LOW); 
    break;      
  case '5':
    // internet OK
    turnOFFleds();
    delay(1000);    
    //
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,HIGH);
    digitalWrite(pinLEDblu,HIGH);  
    break;                          
  }
  //
  msgTxStatoServer[POSIZIONE_CARATT]=charState;
  vw_rx_stop(); // disable rx section
  vw_send((uint8_t *)msgTxStatoServer,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone
  vw_rx_start(); // enable rx section
  msgTxStatoServer[POSIZIONE_CARATT]='0';
}
//================================
// OFF leds
//================================
void turnOFFleds(){
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW);    
}
