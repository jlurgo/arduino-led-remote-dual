/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two different nodes,
 * connect the role_pin to ground on one.  The ping node sends the current time to the pong node,
 * which responds by sending the value back.  The ping node can then see how long the whole cycle
 * took.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const int role_pin = A3;
const int buttonPin = 2;     // the number of the pushbutton pin
int estadoAnteriorBoton = 0;
int estadoActualBoton = 0;
const int ledPin =  3;      // the number of the LED pin

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
//
// This is done through the role_pin
//

// The various roles supported by this sketch
typedef enum { nodo_1 = 1, nodo_2 } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Nodo 1", "Nodo 2"};

// The role of the current running sketch
role_e role;

int inByte = 0;

void setup(void)
{
  //
  // Role
  //

  // set up the role pin
  pinMode(role_pin, INPUT);
  digitalWrite(role_pin,HIGH);
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);     
  
  delay(20); // Just to get a solid reading on the role pin

  // read the address pin, establish our role
  if ( ! digitalRead(role_pin) )
    role = nodo_1;
  else
    role = nodo_2;

  //
  // Print preamble
  //

  Serial.begin(57600);
  Serial.setTimeout(50);
  printf_begin();
  printf("ROLE: %s\n\r",role_friendly_name[role]);

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(8);

  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  if ( role == nodo_1 )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }

  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();
}

void loop(void)
{
  // read the state of the pushbutton value:
  estadoActualBoton = digitalRead(buttonPin);
  
  if(estadoActualBoton != estadoAnteriorBoton)
  {
    radio.stopListening();
    //printf("Cambió el boton a %d...",estadoActualBoton);
    bool ok = radio.write( &estadoActualBoton, sizeof(int) );
    
    if (ok)
    //  printf("enviado ok...");
    radio.startListening();
  }
  
  if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      int estado_boton_remoto;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &estado_boton_remoto, sizeof(int) );

        // Spew it
        printf("Got payload %d...",estado_boton_remoto);
      }
      if (estado_boton_remoto == HIGH) {     
        // turn LED on:    
        digitalWrite(ledPin, HIGH);  
      } 
      else {
        // turn LED off:
        digitalWrite(ledPin, LOW); 
  }
    }
  
  estadoAnteriorBoton = estadoActualBoton;
  delay(20);
}

void serialEvent() {
  while (Serial.available()) {
    // get incoming byte:
    radio.stopListening();
    inByte = Serial.parseInt();
    //printf("Llego por serie %d...",inByte);
    bool ok = radio.write( &inByte, sizeof(int) );
    if (ok)
    //  printf("enviado ok...");
    radio.startListening();
    Serial.flush();
  }
}
// vim:cin:ai:sts=2 sw=2 ft=cpp
