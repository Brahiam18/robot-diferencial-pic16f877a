/*
 * Carro con 2 motorreductores - PIC16F877A + puente H DRV8833
 * Compilador: MPLAB XC8
 * Cristal: 4 MHz
 *
 * CONEXIONES
 *   PIC          DRV8833
 *   RD0  ----->  AIN1   (motor IZQUIERDO = motor A)
 *   RD1  ----->  AIN2
 *   RD2  ----->  BIN1   (motor DERECHO   = motor B)
 *   RD3  ----->  BIN2
 *   RD4  ----->  nSLEEP (debe estar en 1 para que el driver funcione)
 *
 *   Pulsadores (activos en bajo, a GND, con pull-up):
 *   RB0 = Adelante | RB1 = Atras | RB2 = Izquierda | RB3 = Derecha
 *   Sin pulsar nada -> el carro se detiene.
 *
 * TABLA DE VERDAD DRV8833 (por motor)
 *   xIN1 xIN2
 *    0    0   -> libre (detenido)
 *    1    0   -> adelante
 *    0    1   -> atras
 *    1    1   -> freno
 */

#include <xc.h>

// ---------- Bits de configuracion ----------
#pragma config FOSC = XT        // Cristal 4 MHz
#pragma config WDTE = OFF       // Watchdog apagado
#pragma config PWRTE = ON       // Power-up timer
#pragma config BOREN = OFF      // Brown-out apagado
#pragma config LVP = OFF        // Programacion en bajo voltaje apagada (RB3 libre)
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000

// ---------- Pulsadores ----------
#define BTN_ADELANTE   PORTBbits.RB0
#define BTN_ATRAS      PORTBbits.RB1
#define BTN_IZQUIERDA  PORTBbits.RB2
#define BTN_DERECHA    PORTBbits.RB3

// ---------- Estados del carro ----------
typedef enum {
    PARAR = 0,
    ADELANTE,
    ATRAS,
    IZQUIERDA,
    DERECHA
} estado_t;

/*
 * Patron de salida en RD3..RD0 = BIN2 BIN1 AIN2 AIN1
 *   PARAR     : A libre,    B libre    -> 0000 = 0x00
 *   ADELANTE  : A adelante, B adelante -> 0101 = 0x05
 *   ATRAS     : A atras,    B atras    -> 1010 = 0x0A
 *   IZQUIERDA : A adelante, B atras    -> 1001 = 0x09
 *   DERECHA   : A atras,    B adelante -> 0110 = 0x06
 */
const unsigned char patron[5] = {0x00, 0x05, 0x0A, 0x09, 0x06};

void motores(estado_t e)
{
    // Se conserva el nibble alto (RD4 = nSLEEP en 1) y se cambia RD3..RD0
    PORTD = (PORTD & 0xF0) | patron[e];
}

estado_t leer_pulsadores(void)
{
    if (!BTN_ADELANTE)  return ADELANTE;
    if (!BTN_ATRAS)     return ATRAS;
    if (!BTN_IZQUIERDA) return IZQUIERDA;
    if (!BTN_DERECHA)   return DERECHA;
    return PARAR;
}

void main(void)
{
    // Puertos A y E como digitales (apaga el ADC)
    ADCON1 = 0x06;

    // PORTB entradas con pull-ups internos
    TRISB = 0xFF;
    OPTION_REGbits.nRBPU = 0;

    // PORTD salidas
    TRISD = 0x00;
    PORTD = 0x00;
    PORTDbits.RD4 = 1;          // nSLEEP = 1 -> DRV8833 activo

    estado_t actual = PARAR;
    estado_t nuevo;

    motores(PARAR);

    while (1)
    {
        nuevo = leer_pulsadores();

        if (nuevo != actual)
        {
            // Pausa corta antes de cambiar de sentido (protege motores y driver)
            motores(PARAR);
            __delay_ms(50);
            motores(nuevo);
            actual = nuevo;
        }

        __delay_ms(20);         // antirrebote simple
    }
}
