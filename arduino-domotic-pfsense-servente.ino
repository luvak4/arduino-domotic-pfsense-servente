//================================================================
// Halt, reboot, check status of a pfSense server through Arduino 
// and radio modules. 
// Project is divided in three parts: "keyboard", "servente" and "display".
//
// author: luvak4@gmail.com
//================================================================
// System hardware is divided into 3 parts:
// "servente": communicates via serial RS-232 with pfSense
// "keyboard": performs a remote control. It has 4 buttons. Give commands.
// "display": receive "servente" signals (and other devices, eventually) 
// and display its status on LCM or light.
//
// "servente": receive radio-commands from "keyboard" and transmit response to "display"
// "keyboard": transmit radio-commands to "servente"
// "display": receive radio-commands from "servente"
//
// !----------!               !----------!               !---------!
// ! keyboard !--> tx   rx >--! servente !--> tx   rx >--! display !
// !----------!               !          !               !---------!
//                            !          !
//                            !          !               !---------!
//                            !          !--<> rs232 <>--!         !
//                            !          !               ! pfSense !
//                            !----------!               !---------!
//////////////////////////////////////////
//// this is the "servente" schematic ////
//////////////////////////////////////////
//
//                   !----------!
// radio rx (11) >---!          !---> radio tx (12)
//                   ! servente !
// RS-232 rx (1) >---!          !---> RS-232 tx (0)
//                   !----------!
//
/////////////////////////////////////
//// this is the "servente" code ////
/////////////////////////////////////
#include <VirtualWire.h>
// 
int pfSenseInternalStep=1;
int seconds=0;
// for my timing
int dutyCycle = 0;
unsigned long int Pa;
unsigned long int Pb;
// radio modules
const int transmit_pin = 12; 
const int receive_pin = 11; 
uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
// commands that can be receive
//////////////////////////////123456789012
const String msgPulsSpegni  ="pulsPFSE0001";
const String msgPulsRiavvia ="pulsPFSE0002";
const String msgPulsSync    ="pulsPFSE0003";
const String msgPulsPing    ="pulsPFSE0004";
// commands that can be transmit
//////////////////////////////////////123456789012 
const char msgTxComandoRicevuto[13]  = "statPFSE0011";
///////////////////////////123456789012
char msgTxStatoServer[13]="statPFSE0000";
//================================
// setup
//================================
void setup() {
  // setup serial to/from pfSense
  Serial.begin(9600);
  Serial.flush();
  pinMode(13, OUTPUT);
   // impostazione TX e RX
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
	    }
	}
	//--------------------------------
	// message received
	//--------------------------------
	if (vw_get_message(buf, &buflen)){
	  String stringaRX="";
 
	  // retriving message
	  for (int i = 0; i < buflen; i++){
	    stringaRX += char(buf[i]);
	  }
   //Serial.print(stringaRX);
      if(stringaRX==msgPulsSpegni){
       digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);     
    }
	  // only if pfSense is full-started
	  // i can receive command from "keyboard"
	  if(pfSenseInternalStep==9){
	    if(stringaRX==msgPulsSpegni){
              txRicevutoComando();	      
	      pfSenseInternalStep=3;
	    }
	    if(stringaRX==msgPulsRiavvia){
              txRicevutoComando();	      
	      pfSenseInternalStep=6;
	    }
	    if(stringaRX==msgPulsSync){
              txRicevutoComando();
	      pfSenseInternalStep=11;
	    }
	    if(stringaRX==msgPulsPing){
              txRicevutoComando();
	      pfSenseInternalStep=12;
	    }
	  }
	  // Only if FIRST START OF ARDUINO and
	  // PF-SENSE FULL-STARTED
	  // pushing sync button in "kayboard"
	  // synchronizes Arduino with pfSense
	  if(pfSenseInternalStep==1){
	    if(stringaRX==msgPulsSync){
	      pfSenseInternalStep=11;
	    }
	  }
	}
	//--------------------------------
	// every second
	// routine that read "Phase status" and
	// read/write RS232 and
	// perform relate action 
	//--------------------------------
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
    // ON
    //--------------------------------
    if (Serial.available() > 0 ) {
      if (Serial.find("Enter an option")){
	pfSenseInternalStep=9;
	//msgStatoServer = prefissoStatoServer + "2";
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
    Serial.write("exit\n\n");
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
  }
}
//================================
// send "command receive" to "display"
//================================
void txRicevutoComando(){
  vw_rx_stop(); // disable rx section
  vw_send((uint8_t *)msgTxComandoRicevuto,13);
  vw_wait_tx(); // Wait until the whole message is gone
  vw_rx_start(); // enable rx section
}
void txPfsenseStatusToTransmit(char charState){
  msgTxStatoServer[11]=charState;
  vw_rx_stop(); // disable rx section
  vw_send((uint8_t *)msgTxStatoServer,13);
  vw_wait_tx(); // Wait until the whole message is gone
  vw_rx_start(); // enable rx section
  msgTxStatoServer[11]='0';
}

/*
//-------------------------
// delay with prime numbers
//-------------------------
int NuovoRitardo(){
  static contatore;
  int risposta;
  int mieiRitardi[]={113, 127, 139, 151, 163, 179, 191, 211, 223, 239, 251, 263, 277, 293, 307, 331, 347, 359, 373, 389, 401, 419, 431, 443, 457, 479, 491, 503, 521, 541, 557}
  if (contatore > 16){
     contatore =0;
  }
  if (contatore < 0){
     contatore =0;
  }	
  risposta=mieiRitardi[contatore];
  contatore+=1;
  return risposta;
}
*/
