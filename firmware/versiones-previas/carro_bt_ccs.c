/*
 * Carro con 2 motorreductores - PIC16F877A + DRV8833 + Bluetooth HC-05/HC-06
 * Compilador: CCS C
 * Mismas conexiones que la version XC8:
 *   RD0=AIN1, RD1=AIN2 (motor izq.) | RD2=BIN1, RD3=BIN2 (motor der.) | RD4=nSLEEP
 *   RC7(RX) <- TXD del HC-05 | RC6(TX) -> RXD del HC-05 (con divisor 1k/2k)
 *   RB0..RB3 = pulsadores opcionales (Adelante, Atras, Izquierda, Derecha)
 * Comandos: F adelante, B atras, L izquierda, R derecha, S parar
 */

#include <16F877A.h>
#fuses XT, NOWDT, PUT, NOBROWNOUT, NOLVP, NOPROTECT
#use delay(clock=4000000)
#use rs232(baud=9600, xmit=PIN_C6, rcv=PIN_C7, ERRORS)

#define PARAR      0
#define ADELANTE   1
#define ATRAS      2
#define IZQUIERDA  3
#define DERECHA    4

const int8 patron[5] = {0x00, 0x05, 0x0A, 0x09, 0x06};
const char eco[5]    = {'S', 'F', 'B', 'L', 'R'};

volatile int8 estado_bt = PARAR;

#int_rda
void rx_bluetooth(void)
{
   char c = getc();

   switch (c)
   {
      case 'F': case 'f': estado_bt = ADELANTE;  break;
      case 'B': case 'b': estado_bt = ATRAS;     break;
      case 'L': case 'l': estado_bt = IZQUIERDA; break;
      case 'R': case 'r': estado_bt = DERECHA;   break;
      case 'S': case 's': estado_bt = PARAR;     break;
      default: break;
   }
}

void motores(int8 e)
{
   output_d(0x10 | patron[e]);   // 0x10 mantiene nSLEEP en 1
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
   int8 nuevo, botones;

   setup_adc_ports(NO_ANALOGS);
   port_b_pullups(TRUE);
   set_tris_b(0xFF);
   set_tris_d(0x00);

   motores(PARAR);

   enable_interrupts(INT_RDA);
   enable_interrupts(GLOBAL);

   while (TRUE)
   {
      botones = leer_pulsadores();
      nuevo = (botones != PARAR) ? botones : estado_bt;

      if (nuevo != actual)
      {
         motores(PARAR);
         delay_ms(50);
         motores(nuevo);
         actual = nuevo;
         putc(eco[nuevo]);
      }

      delay_ms(20);
   }
}
