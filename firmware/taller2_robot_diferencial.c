/*
 * ============================================================================
 *  TALLER PRACTICO II - Control dual de motores y robotica movil diferencial
 *  Universidad de Ibague - Microcontroladores y Sistemas Embebidos
 *  PIC16F877A | MPLAB XC8 | Driver DRV8833
 * ============================================================================
 *
 *  CONEXIONES (PIC -> DRV8833)
 *    RC2 / CCP1 (pin 17) -> AIN1   PWM motor IZQUIERDO
 *    RD0        (pin 19) -> AIN2   direccion motor IZQUIERDO
 *    RC1 / CCP2 (pin 16) -> BIN1   PWM motor DERECHO
 *    RD1        (pin 20) -> BIN2   direccion motor DERECHO
 *    RD2        (pin 21) -> nSLEEP habilitacion del driver
 *
 *  BLUETOOTH HC-05 / HC-06 (solo en MODO_BLUETOOTH = 1)
 *    RC7 / RX   (pin 26) <- TXD del modulo
 *    RC6 / TX   (pin 25) -> RXD del modulo (divisor 1k / 2k a GND)
 *
 *  ICSP: RB6 (PGC) y RB7 (PGD) quedan libres para el PICkit 3.
 *
 *  TABLA DE VERDAD DRV8833 (por canal)
 *    xIN1 xIN2
 *     0    0   Parada libre (coast)
 *     1    0   Avance
 *     0    1   Reversa
 *     1    1   Freno dinamico
 * ============================================================================
 */

/* ---------------------- CONFIGURACION DEL PROYECTO ---------------------- */

#define CRISTAL_20MHZ   1   // 1 = cristal de 20 MHz (codigo del taller)
                            // 0 = cristal de 4 MHz  (lista de materiales)

#define MODO_BLUETOOTH  0   // 0 = secuencia automatica del taller
                            // 1 = control desde el celular por Bluetooth

/* ------------------------------------------------------------------------ */

#if CRISTAL_20MHZ
    #pragma config FOSC = HS            // Cristal de alta velocidad
    #define _XTAL_FREQ    20000000UL
    #define T2_PRESCALER  0b10          // 1:16 -> Fpwm = 1250 Hz
    #define SPBRG_9600    129           // 20 MHz/(16*(129+1)) = 9615 baudios
#else
    #pragma config FOSC = XT            // Cristal de 4 MHz
    #define _XTAL_FREQ    4000000UL
    #define T2_PRESCALER  0b01          // 1:4  -> Fpwm = 1000 Hz
    #define SPBRG_9600    25            // 4 MHz/(16*(25+1))   = 9615 baudios
#endif

#pragma config WDTE  = OFF
#pragma config PWRTE = ON
#pragma config BOREN = OFF
#pragma config LVP   = OFF
#pragma config CPD   = OFF
#pragma config WRT   = OFF
#pragma config CP    = OFF

#include <xc.h>

// Pines de direccion y habilitacion del DRV8833
#define MOTOR_IZQ_DIR   PORTDbits.RD0   // AIN2
#define MOTOR_DER_DIR   PORTDbits.RD1   // BIN2
#define DRV_NSLEEP      PORTDbits.RD2   // nSLEEP

#define VEL_GIRO        600             // Velocidad de giro en la secuencia (0-1000)

typedef enum {
    PARAR = 0,      // Freno dinamico
    ADELANTE,
    ATRAS,
    IZQUIERDA,
    DERECHA
} mov_t;

/* ======================= CAPA 1: HARDWARE (PWM) ========================= */

void PWM_Init(void)
{
    // Pines PWM: RC1 (CCP2) y RC2 (CCP1)
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;

    // Pines de direccion y nSLEEP
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD2 = 0;
    MOTOR_IZQ_DIR = 0;
    MOTOR_DER_DIR = 0;
    DRV_NSLEEP    = 0;              // Driver dormido hasta terminar la configuracion

    // Periodo: 4*(PR2+1) = 1000 cuentas -> ciclo de trabajo en escala 0-1000
    PR2 = 249;

    CCPR1L = 0;
    CCPR2L = 0;
    CCP1CON = 0b00001100;           // CCP1 en modo PWM
    CCP2CON = 0b00001100;           // CCP2 en modo PWM

    TMR2 = 0;
    T2CON = (unsigned char)(0b100 | T2_PRESCALER);   // TMR2ON = 1 + prescaler
}

// Pulso de activacion del DRV8833: flanco 0 -> 1 en nSLEEP y espera tWAKE (~1 ms)
void DRV8833_Wake(void)
{
    DRV_NSLEEP = 0;
    __delay_ms(1);
    DRV_NSLEEP = 1;
    __delay_ms(2);
}

// Ciclo de trabajo CCP1 (motor izquierdo), 0-1000
void Set_Duty_Motor_Izq(unsigned int duty)
{
    if (duty > 1000) duty = 1000;
    CCPR1L = (unsigned char)(duty >> 2);    // 8 bits altos
    CCP1CONbits.CCP1X = (duty >> 1) & 1;    // bit 1
    CCP1CONbits.CCP1Y = duty & 1;           // bit 0
}

// Ciclo de trabajo CCP2 (motor derecho), 0-1000
void Set_Duty_Motor_Der(unsigned int duty)
{
    if (duty > 1000) duty = 1000;
    CCPR2L = (unsigned char)(duty >> 2);
    CCP2CONbits.CCP2X = (duty >> 1) & 1;
    CCP2CONbits.CCP2Y = duty & 1;
}

/* ===================== CAPA 2: CONTROL DE CADA MOTOR ==================== */

// speed: 0-1000 | dir: 1 = avance, 0 = reversa
void Motor_Izquierdo(unsigned int speed, unsigned char dir)
{
    if (dir == 1) {
        MOTOR_IZQ_DIR = 0;                  // AIN2 = 0
        Set_Duty_Motor_Izq(speed);          // AIN1 = PWM   (10 avance / 00 libre)
    } else {
        MOTOR_IZQ_DIR = 1;                  // AIN2 = 1
        Set_Duty_Motor_Izq(1000 - speed);   // AIN1 = PWM invertido (01 reversa / 11 freno)
    }
}

void Motor_Derecho(unsigned int speed, unsigned char dir)
{
    if (dir == 1) {
        MOTOR_DER_DIR = 0;                  // BIN2 = 0
        Set_Duty_Motor_Der(speed);          // BIN1 = PWM
    } else {
        MOTOR_DER_DIR = 1;                  // BIN2 = 1
        Set_Duty_Motor_Der(1000 - speed);
    }
}

// Freno dinamico: xIN1 = 1, xIN2 = 1 en ambos canales
void Freno_General(void)
{
    MOTOR_IZQ_DIR = 1;
    MOTOR_DER_DIR = 1;
    Set_Duty_Motor_Izq(1000);
    Set_Duty_Motor_Der(1000);
}

// Parada libre: xIN1 = 0, xIN2 = 0 (salidas en alta impedancia)
void Parada_Libre(void)
{
    MOTOR_IZQ_DIR = 0;
    MOTOR_DER_DIR = 0;
    Set_Duty_Motor_Izq(0);
    Set_Duty_Motor_Der(0);
}

/* ================== CAPA 3: NAVEGACION DIFERENCIAL ====================== */

/*
 * Giro sobre su propio eje:
 *   IZQUIERDA -> rueda izquierda en reversa, rueda derecha en avance
 *   DERECHA   -> rueda izquierda en avance,  rueda derecha en reversa
 */
void Mover(mov_t m, unsigned int v)
{
    switch (m) {
        case ADELANTE:  Motor_Izquierdo(v, 1); Motor_Derecho(v, 1); break;
        case ATRAS:     Motor_Izquierdo(v, 0); Motor_Derecho(v, 0); break;
        case IZQUIERDA: Motor_Izquierdo(v, 0); Motor_Derecho(v, 1); break;
        case DERECHA:   Motor_Izquierdo(v, 1); Motor_Derecho(v, 0); break;
        default:        Freno_General();                            break;
    }
}

#if !MODO_BLUETOOTH
/* ================= MODO 0: SECUENCIA AUTOMATICA DEL TALLER ============== */

void delay_ms_custom(unsigned int ms)
{
    while (ms--) {
        __delay_ms(1);
    }
}

// Rampa de aceleracion: 0 -> 1000 en pasos de 50 cada 100 ms (2 s)
void Rampa_Subir(mov_t m)
{
    unsigned int v;
    for (v = 0; v <= 1000; v += 50) {
        Mover(m, v);
        delay_ms_custom(100);
    }
}

// Rampa de desaceleracion: 1000 -> 50
void Rampa_Bajar(mov_t m)
{
    unsigned int v;
    for (v = 1000; v > 0; v -= 50) {
        Mover(m, v);
        delay_ms_custom(100);
    }
}

void Secuencia_Taller(void)
{
    // 1-2. Avance con rampa de aceleracion y desaceleracion
    Rampa_Subir(ADELANTE);
    delay_ms_custom(1000);
    Rampa_Bajar(ADELANTE);
    Freno_General();
    delay_ms_custom(500);

    // 3-4. Reversa con rampa de aceleracion y desaceleracion
    Rampa_Subir(ATRAS);
    delay_ms_custom(1000);
    Rampa_Bajar(ATRAS);
    Freno_General();
    delay_ms_custom(500);

    // 5. Giro a la izquierda sobre su eje
    Mover(IZQUIERDA, VEL_GIRO);
    delay_ms_custom(1000);
    Freno_General();
    delay_ms_custom(500);

    // 6. Giro a la derecha sobre su eje
    Mover(DERECHA, VEL_GIRO);
    delay_ms_custom(1000);

    // Freno total y pausa antes de repetir
    Freno_General();
    delay_ms_custom(2000);
}

void main(void)
{
    ADCON1 = 0x06;          // Puertos A y E digitales
    PWM_Init();
    Parada_Libre();
    DRV8833_Wake();

    while (1) {
        Secuencia_Taller();
    }
}

#else
/* ==================== MODO 1: CONTROL POR BLUETOOTH ===================== */
/*
 * Comandos (app "Bluetooth RC Controller" u otra terminal serie):
 *   F adelante | B atras | L izquierda | R derecha | S freno
 *   '0'..'9'   velocidad 0 % .. 90 %  |  'q' velocidad 100 %
 */

volatile mov_t        cmd_bt = PARAR;
volatile unsigned int vel_bt = 700;     // Velocidad inicial 70 %

void UART_Init(void)
{
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;

    SPBRG = SPBRG_9600;
    TXSTAbits.BRGH = 1;
    TXSTAbits.SYNC = 0;
    RCSTAbits.SPEN = 1;
    TXSTAbits.TXEN = 1;
    RCSTAbits.CREN = 1;

    PIE1bits.RCIE   = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;
}

void UART_Enviar(char c)
{
    while (!TXSTAbits.TRMT);
    TXREG = c;
}

void __interrupt() ISR(void)
{
    if (PIR1bits.RCIF) {
        if (RCSTAbits.OERR) {           // Recuperacion de overrun
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
        }

        char c = RCREG;

        if (c >= '0' && c <= '9') {
            vel_bt = (unsigned int)(c - '0') * 100;
        } else {
            switch (c) {
                case 'F': case 'f': cmd_bt = ADELANTE;  break;
                case 'B': case 'b': cmd_bt = ATRAS;     break;
                case 'L': case 'l': cmd_bt = IZQUIERDA; break;
                case 'R': case 'r': cmd_bt = DERECHA;   break;
                case 'S': case 's': cmd_bt = PARAR;     break;
                case 'q':           vel_bt = 1000;      break;
                default: break;
            }
        }
    }
}

void main(void)
{
    mov_t        actual = PARAR, m;
    unsigned int vel_actual = 0, v;

    ADCON1 = 0x06;
    PWM_Init();
    Parada_Libre();
    DRV8833_Wake();
    UART_Init();

    while (1) {
        // Copia atomica de las variables que modifica la interrupcion
        INTCONbits.GIE = 0;
        m = cmd_bt;
        v = vel_bt;
        INTCONbits.GIE = 1;

        if (m != actual) {
            Freno_General();            // Frena antes de cambiar de sentido
            __delay_ms(50);
            Mover(m, v);
            actual = m;
            vel_actual = v;
            UART_Enviar("SFBLR"[m]);    // Eco: confirma el movimiento
        } else if (v != vel_actual) {
            Mover(m, v);                // Solo cambio de velocidad
            vel_actual = v;
        }
    }
}
#endif
