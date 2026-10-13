/*
 * Carro con 2 motorreductores - PIC16F877A + puente H DRV8833
 * Compilador: CCS C (PIC C Compiler)
 * Mismas conexiones que la version XC8:
 *   RD0=AIN1, RD1=AIN2 (motor izquierdo) | RD2=BIN1, RD3=BIN2 (motor derecho)
 *   RD4=nSLEEP | RB0=Adelante, RB1=Atras, RB2=Izquierda, RB3=Derecha
 */

#include <16F877A.h>
#fuses XT, NOWDT, PUT, NOBROWNOUT, NOLVP, NOPROTECT
#use delay(clock=4000000)

#define PARAR      0
#define ADELANTE   1
#define ATRAS      2
#define IZQUIERDA  3
#define DERECHA    4

// RD3..RD0 = BIN2 BIN1 AIN2 AIN1
const int8 patron[5] = {0x00, 0x05, 0x0A, 0x09, 0x06};

void motores(int8 e)
{
   output_d(0x10 | patron[e]);   // 0x10 mantiene nSLEEP (RD4) en 1
}

int8 leer_pulsadores(void)
{
   if (!input(PIN_B0)) return ADELANTE;
   if (!input(PIN_B1)) return ATRAS;
   if (!input(PIN_B2)) return IZQUIERDA;
   if (!input(PIN_B3)) return DERECHA;
   return PARAR;
}

void main()
{
   int8 actual = PARAR;
   int8 nuevo;

   setup_adc_ports(NO_ANALOGS);
   port_b_pullups(TRUE);
   set_tris_b(0xFF);
   set_tris_d(0x00);

   motores(PARAR);

   while (TRUE)
   {
      nuevo = leer_pulsadores();

      if (nuevo != actual)
      {
         motores(PARAR);
         delay_ms(50);
         motores(nuevo);
         actual = nuevo;
      }

      delay_ms(20);
   }
}
