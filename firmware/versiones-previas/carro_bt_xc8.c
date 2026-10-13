/*
 * Carro con 2 motorreductores - PIC16F877A + DRV8833 + Bluetooth HC-05/HC-06
 * Compilador: MPLAB XC8
 * Cristal: 4 MHz | UART: 9600 baudios, 8N1
 *
 * CONEXIONES
 *   PIC          DRV8833
 *   RD0  ----->  AIN1   (motor IZQUIERDO = motor A)
 *   RD1  ----->  AIN2
 *   RD2  ----->  BIN1   (motor DERECHO   = motor B)
 *   RD3  ----->  BIN2
 *   RD4  ----->  nSLEEP (en 1)
 *
 *   PIC          HC-05 / HC-06
 *   RC7 (RX) <-  TXD
 *   RC6 (TX) ->  RXD  (con divisor 1k/2k: el HC-05 trabaja a 3.3 V en RXD)
 *   GND      --  GND   | +5V -- VCC
 *
 *   Pulsadores (opcionales, activos en bajo):
 *   RB0 = Adelante | RB1 = Atras | RB2 = Izquierda | RB3 = Derecha
 *
 * COMANDOS POR BLUETOOTH (mayuscula o minuscula)
 *   F = adelante | B = atras | L = izquierda | R = derecha | S = parar
 *   Cualquier otro caracter se ignora.
 *
 * PRIORIDAD: si hay un pulsador presionado, manda el pulsador;
 *            si no, manda el ultimo comando recibido por Bluetooth.
 */

#include <xc.h>

#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000

#define BTN_ADELANTE   PORTBbits.RB0
#define BTN_ATRAS      PORTBbits.RB1
#define BTN_IZQUIERDA  PORTBbits.RB2
#define BTN_DERECHA    PORTBbits.RB3

typedef enum {
    PARAR = 0,
    ADELANTE,
    ATRAS,
    IZQUIERDA,
    DERECHA
} estado_t;

// RD3..RD0 = BIN2 BIN1 AIN2 AIN1
const unsigned char patron[5] = {0x00, 0x05, 0x0A, 0x09, 0x06};

// Estado ordenado por Bluetooth (se modifica dentro de la interrupcion)
volatile estado_t estado_bt = PARAR;

// ---------- Motores ----------
void motores(estado_t e)
{
    PORTD = (PORTD & 0xF0) | patron[e];   // RD4 (nSLEEP) se conserva en 1
}

// ---------- Pulsadores ----------
estado_t leer_pulsadores(void)
{
    if (!BTN_ADELANTE)  return ADELANTE;
    if (!BTN_ATRAS)     return ATRAS;
    if (!BTN_IZQUIERDA) return IZQUIERDA;
    if (!BTN_DERECHA)   return DERECHA;
    return PARAR;
}

// ---------- UART ----------
void uart_init(void)
{
    TRISCbits.TRISC6 = 0;      // TX salida
    TRISCbits.TRISC7 = 1;      // RX entrada

    TXSTAbits.BRGH = 1;        // Alta velocidad
    SPBRG = 25;                // 4 MHz / (16 * (25 + 1)) = 9615 baudios (error 0.16 %)
    TXSTAbits.SYNC = 0;        // Asincrono
    RCSTAbits.SPEN = 1;        // Habilita el puerto serie
    TXSTAbits.TXEN = 1;        // Habilita transmision
    RCSTAbits.CREN = 1;        // Habilita recepcion continua

    PIE1bits.RCIE = 1;         // Interrupcion por dato recibido
    INTCONbits.PEIE = 1;       // Interrupciones de perifericos
    INTCONbits.GIE = 1;        // Interrupciones globales
}

void uart_enviar(char c)
{
    while (!TXSTAbits.TRMT);   // Espera a que el registro de envio quede libre
    TXREG = c;
}

// ---------- Interrupcion: llega un caracter por Bluetooth ----------
void __interrupt() isr(void)
{
    if (PIR1bits.RCIF)
    {
        // Error de sobreescritura: reiniciar el receptor
        if (RCSTAbits.OERR)
        {
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
        }

        char c = RCREG;        // Leer RCREG limpia RCIF

        switch (c)
        {
            case 'F': case 'f': estado_bt = ADELANTE;  break;
            case 'B': case 'b': estado_bt = ATRAS;     break;
            case 'L': case 'l': estado_bt = IZQUIERDA; break;
            case 'R': case 'r': estado_bt = DERECHA;   break;
            case 'S': case 's': estado_bt = PARAR;     break;
            default: break;    // Otros caracteres (luces, velocidad, \r, \n) se ignoran
        }
    }
}

void main(void)
{
    ADCON1 = 0x06;             // Puertos A y E digitales

    TRISB = 0xFF;              // Pulsadores
    OPTION_REGbits.nRBPU = 0;  // Pull-ups internos de PORTB

    TRISD = 0x00;
    PORTD = 0x00;
    PORTDbits.RD4 = 1;         // nSLEEP = 1

    uart_init();

    estado_t actual = PARAR;
    estado_t nuevo, botones;

    motores(PARAR);

    while (1)
    {
        botones = leer_pulsadores();
        nuevo = (botones != PARAR) ? botones : estado_bt;

        if (nuevo != actual)
        {
            motores(PARAR);    // Pausa antes de cambiar de sentido
            __delay_ms(50);
            motores(nuevo);
            actual = nuevo;

            uart_enviar("SFBLR"[nuevo]);   // Eco al celular: confirma el movimiento
        }

        __delay_ms(20);        // Antirrebote de pulsadores
    }
}
