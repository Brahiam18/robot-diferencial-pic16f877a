/*
 * ============================================================================
 *  PRUEBA DE MONTAJE - LEDs en secuencia
 *  PIC16F877A | MPLAB XC8 | Cristal de 20 MHz
 * ============================================================================
 *
 *  MODO_UNIVERSAL = 0 (por defecto)
 *    Secuencia en los tres LEDs de la tarjeta: RB5, RB4 y RB3.
 *
 *  MODO_UNIVERSAL = 1
 *    No hace falta saber a que pines van los LEDs. El programa enciende el
 *    mismo bit en TODOS los puertos a la vez (bit 0 de PORTA..PORTE, luego
 *    bit 1, ... hasta bit 7). Cualquier LED se prende cuando le llega su
 *    turno, asi que los tres se ven encender uno tras otro.
 *
 *    ANTES DE PROBAR: desconecte la bateria de los motores y retire el HC-05,
 *    porque este modo tambien mueve los pines del driver y de la UART.
 *
 * ============================================================================
 */

#define MODO_UNIVERSAL  0
#define TIEMPO_MS       300     // Tiempo que dura encendido cada paso

// ---------- Solo para MODO_UNIVERSAL = 0: pines de los LEDs ----------
// Segun el esquematico de la tarjeta (Electronica Digital II - 2026B):
//   LED1 - R4 - red A - pata 15 TQFP - RB5
//   LED2 - R5 - red B - pata 14 TQFP - RB4
//   LED3 - R6 - red C - pata 11 TQFP - RB3/PGM (requiere LVP = OFF)
#define LED1        PORTBbits.RB5
#define LED2        PORTBbits.RB4
#define LED3        PORTBbits.RB3
#define LED1_TRIS   TRISBbits.TRISB5
#define LED2_TRIS   TRISBbits.TRISB4
#define LED3_TRIS   TRISBbits.TRISB3

// ---------- Bits de configuracion (cristal de 20 MHz) ----------
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 20000000UL

#include <xc.h>

void esperar(unsigned int ms) {
    while (ms--) {
        __delay_ms(1);
    }
}

#if MODO_UNIVERSAL

void escribir_todos(unsigned char valor) {
    PORTA = valor;
    PORTB = valor;
    PORTC = valor;
    PORTD = valor;
    PORTE = valor;
}

void main(void) {
    unsigned char bit;

    ADCON1 = 0x06;      // Todos los pines de PORTA y PORTE como digitales
    CMCON  = 0x07;      // Comparadores apagados

    TRISA = 0x00;       // Todos los puertos como salidas
    TRISB = 0x00;
    TRISC = 0x00;
    TRISD = 0x00;
    TRISE = 0x00;

    escribir_todos(0x00);

    // Al encender: todo prendido 1 s para comprobar que los 3 LEDs funcionan
    escribir_todos(0xFF);
    esperar(1000);
    escribir_todos(0x00);
    esperar(500);

    while (1) {
        // Recorre los bits 0..7 en todos los puertos a la vez
        for (bit = 0; bit < 8; bit++) {
            escribir_todos((unsigned char)(1 << bit));
            esperar(TIEMPO_MS);
        }
        escribir_todos(0x00);
        esperar(TIEMPO_MS);
    }
}

#else

void main(void) {
    ADCON1 = 0x06;
    CMCON  = 0x07;

    LED1_TRIS = 0;
    LED2_TRIS = 0;
    LED3_TRIS = 0;
    LED1 = 0;
    LED2 = 0;
    LED3 = 0;

    while (1) {
        // Secuencia: LED1 -> LED2 -> LED3
        LED1 = 1; LED2 = 0; LED3 = 0;
        esperar(TIEMPO_MS);
        LED1 = 0; LED2 = 1; LED3 = 0;
        esperar(TIEMPO_MS);
        LED1 = 0; LED2 = 0; LED3 = 1;
        esperar(TIEMPO_MS);

        // Los tres encendidos y luego apagados
        LED1 = 1; LED2 = 1; LED3 = 1;
        esperar(TIEMPO_MS);
        LED1 = 0; LED2 = 0; LED3 = 0;
        esperar(TIEMPO_MS);
    }
}

#endif
