/*
 * codigo_docente.c - Taller Practico II: control dual de motores con PWM
 * PIC16F877A | MPLAB XC8 | Driver DRV8833 | Cristal de 20 MHz
 *
 * Codigo entregado por el docente. Solo se retiraron los encabezados del PDF
 * ("Taller practico I / Electronica Digital II / ...") que quedaron pegados
 * dentro del codigo al copiarlo, y se ordeno la indentacion.
 */

// Configuracion de fusibles (cristal de 20 MHz)
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

// Definicion de pines de direccion para el DRV8833
#define MOTOR_IZQ_DIR PORTDbits.RD0
#define MOTOR_DER_DIR PORTDbits.RD1

void PWM_Init(void) {
    // Configurar pines RC1 (CCP2) y RC2 (CCP1) como salidas PWM
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;

    // Configurar pines de direccion RD0 y RD1 como salidas
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;

    // Periodo del PWM a 1.25 kHz (PR2 = 249)
    PR2 = 249;

    // Activar modo PWM en CCP1 y CCP2
    CCP1CON = 0b00001100;
    CCP2CON = 0b00001100;

    // Configurar Timer2 (Prescaler 1:16, TMR2ON = 1)
    T2CON = 0b00000110;
}

// Carga del ciclo de trabajo para CCP1 (Motor Izquierdo / LED Izq, 0-1000)
void Set_Duty_Motor_Izq(unsigned int duty) {
    if (duty > 1000) duty = 1000;
    CCPR1L = (unsigned char)(duty >> 2);
    CCP1CONbits.CCP1X = (duty >> 1) & 1;
    CCP1CONbits.CCP1Y = duty & 1;
}

// Carga del ciclo de trabajo para CCP2 (Motor Derecho / LED Der, 0-1000)
void Set_Duty_Motor_Der(unsigned int duty) {
    if (duty > 1000) duty = 1000;
    CCPR2L = (unsigned char)(duty >> 2);
    CCP2CONbits.CCP2X = (duty >> 1) & 1;
    CCP2CONbits.CCP2Y = duty & 1;
}

// Control del Motor Izquierdo (speed: 0-1000, dir: 1=Avance, 0=Reversa)
void Motor_Izquierdo(unsigned int speed, unsigned char dir) {
    if (dir == 1) {
        MOTOR_IZQ_DIR = 0;                  // AIN2 = 0
        Set_Duty_Motor_Izq(speed);          // AIN1 = PWM
    } else {
        MOTOR_IZQ_DIR = 1;                  // AIN2 = 1
        Set_Duty_Motor_Izq(1000 - speed);   // Inversion de ciclo para PWM en nivel bajo
    }
}

// Control del Motor Derecho (speed: 0-1000, dir: 1=Avance, 0=Reversa)
void Motor_Derecho(unsigned int speed, unsigned char dir) {
    if (dir == 1) {
        MOTOR_DER_DIR = 0;                  // BIN2 = 0
        Set_Duty_Motor_Der(speed);          // BIN1 = PWM
    } else {
        MOTOR_DER_DIR = 1;                  // BIN2 = 1
        Set_Duty_Motor_Der(1000 - speed);
    }
}

void Freno_General(void) {
    MOTOR_IZQ_DIR = 1;
    MOTOR_DER_DIR = 1;
    Set_Duty_Motor_Izq(1000);               // AIN1=1, AIN2=1 -> Brake
    Set_Duty_Motor_Der(1000);               // BIN1=1, BIN2=1 -> Brake
}

void delay_ms_custom(unsigned int ms) {
    while (ms--) {
        __delay_ms(1);
    }
}

void main(void) {
    unsigned int velocidad;

    PWM_Init();

    while (1) {

        // --- 1. PRUEBA DE AVANCE CON RAMPA DE ACELERACION ---
        for (velocidad = 0; velocidad <= 1000; velocidad += 50) {
            Motor_Izquierdo(velocidad, 1);
            Motor_Derecho(velocidad, 1);
            delay_ms_custom(100);           // Incrementa la velocidad cada 100 ms
        }
        delay_ms_custom(1000);              // Mantiene velocidad maxima 1 segundo

        // --- 2. PRUEBA DE AVANCE CON RAMPA DE DESACELERACION ---
        for (velocidad = 1000; velocidad > 0; velocidad -= 50) {
            Motor_Izquierdo(velocidad, 1);
            Motor_Derecho(velocidad, 1);
            delay_ms_custom(100);
        }

        // Freno total breve
        Freno_General();
        delay_ms_custom(500);

        // --- 3. PRUEBA DE REVERSA CON RAMPA DE ACELERACION ---
        for (velocidad = 0; velocidad <= 1000; velocidad += 50) {
            Motor_Izquierdo(velocidad, 0);  // dir = 0 (Reversa)
            Motor_Derecho(velocidad, 0);    // dir = 0 (Reversa)
            delay_ms_custom(100);
        }
        delay_ms_custom(1000);              // Mantiene velocidad maxima 1 segundo en reversa

        // --- 4. PRUEBA DE REVERSA CON RAMPA DE DESACELERACION ---
        for (velocidad = 1000; velocidad > 0; velocidad -= 50) {
            Motor_Izquierdo(velocidad, 0);
            Motor_Derecho(velocidad, 0);
            delay_ms_custom(100);
        }

        // Freno total y pausa antes de repetir el ciclo
        Freno_General();
        delay_ms_custom(2000);
    }
}
