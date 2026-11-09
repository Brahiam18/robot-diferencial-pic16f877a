/*
 * ============================================================================
 *  TALLER PRACTICO II - Control dual de motores y robotica movil diferencial
 *  Universidad de Ibague - Microcontroladores y Sistemas Embebidos
 *  PIC16F877A | MPLAB XC8 | Driver DRV8833
 * ============================================================================
 *
 *  TARJETA: PCB Electronica Digital II 2026B, PIC16F877A en TQFP-44,
 *  cristal de 20 MHz, modulo DRV8833 y zocalo para HC-05.
 *
 *  CONEXIONES (PIC -> modulo DRV8833)       pata TQFP-44 (DIP-40)
 *    RC2 / CCP1 -> MIPWM  PWM motor IZQUIERDO      36 (17)
 *    RD0        -> MIPIN  direccion motor IZQ.     38 (19)
 *    RC1 / CCP2 -> MDPWM  PWM motor DERECHO        35 (16)
 *    RD1        -> MDPIN  direccion motor DER.     39 (20)
 *    RD2        -> nSLEEP (en protoboard)          40 (21)
 *    En la PCB el pin EEP (nSLEEP) del modulo NO esta ruteado: se conecta por
 *    hardware a +5 V con un cable corto (ver informe, seccion de potencia).
 *    En la PCB el motor izquierdo usa IN2 = PWM e IN1 = direccion, por lo que
 *    su sentido electrico esta invertido; se compensa con el montaje en espejo.
 *
 *  BLUETOOTH HC-05 / HC-06 (solo en MODO_BLUETOOTH = 1)
 *    RC7 / RX  <- TXD del modulo                    1 (26)
 *    RC6 / TX  -> RXD del modulo (divisor 1k/2k)   44 (25)
 *
 *  LEDs DE DIAGNOSTICO DE LA TARJETA (modo Bluetooth)
 *    RB5 -> LED1 (R4)   RB4 -> LED2 (R5)   RB3/PGM -> LED3 (R6)  15, 14, 11
 *
 *  ICSP: MCLR (18), RB6/PGC (16), RB7/PGD (17) libres para el PICkit 3.
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

#define MODO_BLUETOOTH  1   // 0 = secuencia automatica del taller
                            // 1 = control desde el celular (Dabble Gamepad o letras)

#define GIRO_SOBRE_EJE  0   // 1 = giros con ruedas en sentidos opuestos (requiere que
                            //     RD0 y RD1 NO esten unidas en la tarjeta)
                            // 0 = giros con pivote: una rueda avanza y la otra se detiene;
                            //     RD0 y RD1 siempre iguales (seguro aunque esten unidas)

#define TARJETA_CURSO   1   // 1 = PCB del curso: EEP (nSLEEP) va a 5 V por hardware.
                            //     RD2 y RD3 quedan como ENTRADAS: en la tarjeta estan
                            //     unidas a la linea de IN1 y pelearian con RD0.
                            // 0 = protoboard: nSLEEP controlado por RD2 (pulso de activacion)

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

#pragma config WDTE  = ON     // Watchdog: reinicia el PIC si se cuelga (~2.3 s)
#pragma config PWRTE = ON
#pragma config BOREN = ON     // Brown-out: reinicio limpio si VDD < ~4 V
#pragma config LVP   = OFF
#pragma config CPD   = OFF
#pragma config WRT   = OFF
#pragma config CP    = OFF

#include <xc.h>

// Pines de direccion y habilitacion del DRV8833
// Bits del puerto D: RD0 = direccion izquierda, RD1 = direccion derecha, RD2 = nSLEEP
#define BIT_DIR_IZQ     0x01
#define BIT_DIR_DER     0x02
#define BIT_NSLEEP      0x04

/*
 * Copia en memoria ("sombra") del puerto D.
 * En los PIC16 no hay registro LAT: cambiar un solo bit (BSF/BCF) lee TODOS los
 * pines del puerto y los vuelve a escribir. Si un pin esta cargado y se lee
 * distinto de lo que se ordeno, su valor se pierde. Escribiendo siempre el
 * puerto completo desde esta copia, cada pin conserva el valor ordenado.
 */
unsigned char sombra_d = 0;

void Puerto_D(unsigned char mascara, unsigned char valor)
{
    if (valor) sombra_d |= mascara;
    else       sombra_d &= (unsigned char)~mascara;
    PORTD = sombra_d;
}

// Cambia las dos lineas de direccion en una sola escritura
void Direcciones(unsigned char izq, unsigned char der)
{
    sombra_d &= (unsigned char)~(BIT_DIR_IZQ | BIT_DIR_DER);
    if (izq) sombra_d |= BIT_DIR_IZQ;
    if (der) sombra_d |= BIT_DIR_DER;
    PORTD = sombra_d;
}

#define MOTOR_IZQ_DIR(v)  Puerto_D(BIT_DIR_IZQ, (v))
#define MOTOR_DER_DIR(v)  Puerto_D(BIT_DIR_DER, (v))
#define DRV_NSLEEP(v)     Puerto_D(BIT_NSLEEP, (v))

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

    // Pines de direccion (y nSLEEP en protoboard)
    sombra_d = 0;                   // Direcciones en 0 (y driver dormido en protoboard)
    PORTD = sombra_d;               // antes de habilitar las salidas
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
#if TARJETA_CURSO
    TRISDbits.TRISD2 = 1;           // Entrada: unida a IN1 en la tarjeta, no debe pelear
    TRISDbits.TRISD3 = 1;           // Entrada: idem (fuga hacia la linea de IN1)
#else
    TRISDbits.TRISD2 = 0;           // nSLEEP controlado por el PIC
#endif

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
#if TARJETA_CURSO
    __delay_ms(2);                  // EEP ya esta en 5 V: solo se espera tWAKE
#else
    DRV_NSLEEP(0);
    __delay_ms(1);
    DRV_NSLEEP(1);
    __delay_ms(2);
#endif
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
        MOTOR_IZQ_DIR(0);                   // AIN2 = 0
        Set_Duty_Motor_Izq(speed);          // AIN1 = PWM   (10 avance / 00 libre)
    } else {
        MOTOR_IZQ_DIR(1);                   // AIN2 = 1
        Set_Duty_Motor_Izq(1000 - speed);   // AIN1 = PWM invertido (01 reversa / 11 freno)
    }
}

void Motor_Derecho(unsigned int speed, unsigned char dir)
{
    if (dir == 1) {
        MOTOR_DER_DIR(0);                   // BIN2 = 0
        Set_Duty_Motor_Der(speed);          // BIN1 = PWM
    } else {
        MOTOR_DER_DIR(1);                   // BIN2 = 1
        Set_Duty_Motor_Der(1000 - speed);
    }
}

// Freno dinamico: xIN1 = 1, xIN2 = 1 en ambos canales
void Freno_General(void)
{
    Direcciones(1, 1);
    Set_Duty_Motor_Izq(1000);
    Set_Duty_Motor_Der(1000);
}

// Parada libre: xIN1 = 0, xIN2 = 0 (salidas en alta impedancia)
void Parada_Libre(void)
{
    Direcciones(0, 0);
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
#if GIRO_SOBRE_EJE
        case IZQUIERDA: Motor_Izquierdo(v, 0); Motor_Derecho(v, 1); break;
        case DERECHA:   Motor_Izquierdo(v, 1); Motor_Derecho(v, 0); break;
#else   // Giro con pivote: una rueda suelta, la otra avanza (RD0 = RD1 = 0)
        case IZQUIERDA: Motor_Izquierdo(0, 1); Motor_Derecho(v, 1); break;
        case DERECHA:   Motor_Izquierdo(v, 1); Motor_Derecho(0, 1); break;
#endif
        default:        Freno_General();                            break;
    }
}

#if !MODO_BLUETOOTH
/* ================= MODO 0: SECUENCIA AUTOMATICA DEL TALLER ============== */

void delay_ms_custom(unsigned int ms)
{
    while (ms--) {
        CLRWDT();
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
/* ============ MODO 1: CONTROL POR BLUETOOTH (Dabble Gamepad + letras) ======= */
/*
 * Acepta dos tipos de comandos por la UART (HC-05 a 9600 baudios):
 *
 * 1) App Dabble -> modulo "Gamepad" (Android con HC-05, iPhone con HM-10)
 *    Trama: FF 01 <funcion> 01 02 <botones> <dato> 00
 *      funcion 0x01 = modo digital (cruceta):  dato = bits UP0 DOWN1 LEFT2 RIGHT3
 *      funcion 0x02 = modo joystick:           dato = AAAAARRR
 *      funcion 0x03 = modo acelerometro:       igual que joystick
 *                                              angulo = AAAAA*15 grados, radio = RRR (0..7)
 *      botones: START0 SELECT1 TRIANGULO2 CIRCULO3 X4 CUADRADO5
 *
 *    Cruceta  : arriba/abajo/izq/der y diagonales (curvas)
 *    Joystick : velocidad y giro proporcionales (mezcla diferencial)
 *    Triangulo: +10 % velocidad   X: -10 % velocidad
 *    Circulo  : velocidad 100 %   Cuadrado (mantener): freno
 *
 * 2) Letras (Bluetooth RC Controller, Serial Bluetooth Terminal, etc.)
 *      F adelante | B atras | L izquierda | R derecha | S freno
 *      '0'..'9' velocidad 0 % .. 90 % | 'q' velocidad 100 %
 */

#define VEL_INICIAL   300   // Velocidad inicial (0-1000): arranca lento. Se sube
                            // con el triangulo de Dabble (+10 %) o con el circulo (100 %)
#define VEL_MINIMA    300   // PWM minimo para vencer la friccion del motor (0 = sin ajuste)
#define FAILSAFE_MS   500   // Frena si se pierde la senal por este tiempo (0 = desactivado).
                            // Solo actua si la app repite los datos mientras se mantiene
                            // un boton (se detecta solo); ademas, siempre frena si la UART
                            // detecta un error de trama (el HC-05 se reinicio o se desconecto).

// Seno x1000 para angulos multiplos de 15 grados (0, 15, ..., 345)
const int SENO[24] = {    0,  259,  500,  707,  866,  966, 1000,  966,
                        866,  707,  500,  259,    0, -259, -500, -707,
                       -866, -966,-1000, -966, -866, -707, -500, -259 };

// ---------- Datos compartidos con la interrupcion ----------
volatile unsigned char hay_letra = 0;      // Llego una letra
volatile char          letra = 0;
volatile unsigned char hay_gamepad = 0;    // Llego una trama de Dabble
volatile unsigned char gp_funcion = 0, gp_botones = 0, gp_dato = 0;
volatile unsigned char pedir_id = 0;       // Dabble pidio la identificacion de la tarjeta
volatile unsigned char dato_recibido = 0;  // Failsafe: llego al menos un byte
volatile unsigned char error_trama = 0;    // Failsafe: varios errores de trama seguidos
unsigned char ferr_seguidos = 0;           // Solo lo usa la ISR
#define FERR_PARA_FRENAR  3                // Errores de trama consecutivos para frenar
volatile unsigned char cnt_tramas = 0;     // Diagnostico: tramas de Dabble recibidas
volatile unsigned char cnt_letras = 0;     // Diagnostico: bytes sueltos (letras) recibidos

// ---------- LEDs de diagnostico de la tarjeta ----------
// LED1 (RB5): cambia de estado con cada trama de Dabble valida
// LED2 (RB4): cambia de estado con cada letra / byte suelto
// LED3 (RB3): encendido mientras alguna rueda tiene orden de moverse;
//             un destello corto = la UART cambio de velocidad (baudios)
// Al arrancar el PIC los tres parpadean 3 veces (si lo hacen al presionar un
// boton, el PIC se esta reiniciando: problema de alimentacion).
#define DIAGNOSTICO   1
#define LED_TRAMA     PORTBbits.RB5
#define LED_LETRA     PORTBbits.RB4
#define LED_MOTOR     PORTBbits.RB3

// ---------- Deteccion automatica de baudios ----------
// Si llegan muchos bytes sin sentido (o errores de trama), la UART prueba la
// siguiente velocidad hasta que las tramas de Dabble o las letras se reciban bien.
#define BAUD_AUTO        1
#define UMBRAL_INVALIDOS 24

#if CRISTAL_20MHZ   // BRGH = 1:  baud = 20 MHz / (16 (SPBRG + 1))
const unsigned char TABLA_SPBRG[5] = { 129,  32,  64,  21,  10 };
//                                    9600 38400 19200 57600 115200  (error <= 1.4 %)
#define N_BAUDIOS 5
#else               // 4 MHz: solo 9600 y 19200 tienen error aceptable
const unsigned char TABLA_SPBRG[2] = { 25, 12 };
#define N_BAUDIOS 2
#endif

volatile unsigned char indice_baud = 0;
volatile unsigned char cambios_baud = 0;   // Diagnostico: veces que cambio la velocidad
unsigned char invalidos = 0;               // Solo lo usa la ISR

void Byte_Invalido(unsigned char peso)
{
#if BAUD_AUTO
    invalidos += peso;
    if (invalidos >= UMBRAL_INVALIDOS) {
        invalidos = 0;
        indice_baud++;
        if (indice_baud >= N_BAUDIOS) indice_baud = 0;
        SPBRG = TABLA_SPBRG[indice_baud];
        cambios_baud++;
    }
#endif
}

// ---------- Estado del parser de tramas (solo lo usa la ISR) ----------
enum { E_LIBRE, E_MODULO, E_FUNCION, E_NARGS, E_LARGO, E_DATOS, E_FIN };
unsigned char p_estado = E_LIBRE;
unsigned char p_modulo, p_funcion, p_nargs, p_arg, p_largo, p_idx;
unsigned char p_d0, p_d1;

void UART_Init(void)
{
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;

    SPBRG = TABLA_SPBRG[0];                 // Empieza en 9600
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

// Procesa un byte recibido: tramas de Dabble o letras sueltas
void Procesar_Byte(unsigned char c)
{
    dato_recibido = 1;
    ferr_seguidos = 0;                      // Llego un byte sano: no es un reinicio del HC-05

    switch (p_estado) {
    case E_LIBRE:
        if (c == 0xFF) {                    // Inicio de trama de Dabble
            p_estado = E_MODULO;
        } else {                            // Byte suelto
            cnt_letras++;
            if (c == 'F' || c == 'B' || c == 'L' || c == 'R' || c == 'S' ||
                c == 'f' || c == 'b' || c == 'l' || c == 'r' || c == 's' ||
                c == 'q' || (c >= '0' && c <= '9')) {
                letra = (char)c;            // Comando valido
                hay_letra = 1;
                invalidos = 0;
            } else if (c != '\r' && c != '\n') {
                Byte_Invalido(1);           // Byte sin sentido
            }
        }
        break;

    case E_MODULO:  p_modulo  = c; p_estado = E_FUNCION; break;
    case E_FUNCION: p_funcion = c; p_estado = E_NARGS;   break;

    case E_NARGS:
        p_nargs = c;
        p_arg = 0;
        p_d0 = 0;
        p_d1 = 0;
        p_estado = (p_nargs == 0) ? E_FIN : E_LARGO;
        break;

    case E_LARGO:
        p_largo = c;
        p_idx = 0;
        if (p_largo == 0) {
            p_arg++;
            if (p_arg >= p_nargs) p_estado = E_FIN;
        } else {
            p_estado = E_DATOS;
        }
        break;

    case E_DATOS:
        if (p_arg == 0) {                   // Solo interesan los 2 bytes del 1er argumento
            if (p_idx == 0) p_d0 = c;
            if (p_idx == 1) p_d1 = c;
        }
        p_idx++;
        if (p_idx >= p_largo) {
            p_arg++;
            p_estado = (p_arg >= p_nargs) ? E_FIN : E_LARGO;
        }
        break;

    case E_FIN:
        if (c != 0x00) {
            Byte_Invalido(6);               // Trama corrupta
        } else {                            // Trama completa y valida
            invalidos = 0;
            if (p_modulo == 0x01) {         // Modulo Gamepad
                gp_funcion = p_funcion;
                gp_botones = p_d0;
                gp_dato    = p_d1;
                hay_gamepad = 1;
                cnt_tramas++;
            } else if (p_modulo == 0x00 && p_funcion == 0x03) {
                pedir_id = 1;               // Solicitud de identificacion de tarjeta
            }
        }
        p_estado = E_LIBRE;
        break;

    default:
        p_estado = E_LIBRE;
        break;
    }
}

void __interrupt() ISR(void)
{
    if (PIR1bits.RCIF) {
        if (RCSTAbits.OERR) {               // Recuperacion de overrun
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
            p_estado = E_LIBRE;
        }
        if (RCSTAbits.FERR) {               // Error de trama: baudios equivocados
            (void)RCREG;                    // o el HC-05 se reinicio (linea TX en 0 V)
            p_estado = E_LIBRE;
            // Un error aislado es ruido de los motores; varios seguidos indican
            // que la linea TX del HC-05 cayo (reinicio o desconexion).
            if (ferr_seguidos < 255) ferr_seguidos++;
            if (ferr_seguidos >= FERR_PARA_FRENAR) error_trama = 1;
            Byte_Invalido(3);
        } else {
            Procesar_Byte(RCREG);
        }
    }
}

// Respuesta a Dabble: "tarjeta generica" (igual que la libreria oficial)
void Enviar_ID_Tarjeta(void)
{
    const unsigned char trama[10] = {0xFF, 0x00, 0x03, 0x01, 0x04,
                                     0x05, 0x01, 0x05, 0x02, 0x00};
    unsigned char i;
    for (i = 0; i < 10; i++) UART_Enviar((char)trama[i]);
}

// ---------- Control de las ruedas ----------
int  obj_izq = 0, obj_der = 0;      // Objetivo normalizado (-1000..1000)
unsigned int vel_max = VEL_INICIAL; // Escala de velocidad (0..1000)
signed char  sig_izq = 0, sig_der = 0;

signed char Signo(int v)
{
    if (v > 0) return 1;
    if (v < 0) return -1;
    return 0;
}

int Limitar(int v)
{
    if (v > 1000)  return 1000;
    if (v < -1000) return -1000;
    return v;
}

// Escala por vel_max y compensa la zona muerta del motor
unsigned int Ajustar_PWM(int v)
{
    unsigned long s;
    if (v < 0) v = -v;
    s = (unsigned long)v * vel_max / 1000UL;
    if (s == 0) return 0;
    return (unsigned int)(VEL_MINIMA + s * (1000UL - VEL_MINIMA) / 1000UL);
}

void Rueda_Izquierda(int v)
{
    unsigned int p = Ajustar_PWM(v);
    if (p == 0) Motor_Izquierdo(0, 0);          // xIN1 = xIN2 = 1 -> freno
    else        Motor_Izquierdo(p, (v > 0) ? 1 : 0);
}

void Rueda_Derecha(int v)
{
    unsigned int p = Ajustar_PWM(v);
    if (p == 0) Motor_Derecho(0, 0);
    else        Motor_Derecho(p, (v > 0) ? 1 : 0);
}

void Aplicar_Ruedas(void)
{
    int iz = obj_izq, de = obj_der;
    signed char si, sd;

#if !GIRO_SOBRE_EJE
    // Giro con pivote: nunca se piden sentidos opuestos, de modo que las dos
    // lineas de direccion (RD0 y RD1) quedan siempre con el mismo valor.
    // La rueda que deberia ir hacia atras se detiene y la otra sigue.
    if ((iz > 0 && de < 0) || (iz < 0 && de > 0)) {
        if (iz < 0) iz = 0;
        else        de = 0;
    }
#endif

    si = Signo(iz);
    sd = Signo(de);

    // Si alguna rueda invierte su sentido, frena 30 ms antes (protege driver y motor)
    if ((sig_izq != 0 && si == -sig_izq) || (sig_der != 0 && sd == -sig_der)) {
        Freno_General();
        __delay_ms(30);
    }
    sig_izq = si;
    sig_der = sd;

    if (si == 0 && sd == 0) {
        Freno_General();
    } else {
        // Una rueda detenida toma la misma linea de direccion que la otra:
        //   si la otra avanza (direccion = 0) -> parada libre (0-0)
        //   si la otra retrocede (direccion = 1) -> freno (1-1)
        if (si == 0) Motor_Izquierdo(0, (sd > 0) ? 1 : 0);
        else         Rueda_Izquierda(iz);
        if (sd == 0) Motor_Derecho(0, (si > 0) ? 1 : 0);
        else         Rueda_Derecha(de);
    }
}

// ---------- Interpretacion de comandos ----------
void Comando_Letra(char c)
{
    if (c >= '0' && c <= '9') {
        vel_max = (unsigned int)(c - '0') * 100;
        Aplicar_Ruedas();
        return;
    }
    switch (c) {
        case 'F': case 'f': obj_izq =  1000; obj_der =  1000; break;
        case 'B': case 'b': obj_izq = -1000; obj_der = -1000; break;
        case 'L': case 'l': obj_izq = -1000; obj_der =  1000; break;
        case 'R': case 'r': obj_izq =  1000; obj_der = -1000; break;
        case 'S': case 's': obj_izq =     0; obj_der =     0; break;
        case 'q':           vel_max = 1000; Aplicar_Ruedas(); return;
        default: return;                    // Ignora otros caracteres
    }
    Aplicar_Ruedas();
    UART_Enviar(c);                         // Eco de confirmacion
}

unsigned char botones_previos = 0;

// ---------- Deteccion de envio continuo para el failsafe ----------
// Si la app repite el mismo comando mientras el carro se mueve, envia datos de
// forma continua y el failsafe por tiempo se puede usar sin riesgo.
unsigned char envio_continuo = 0;
unsigned char ult_a = 0, ult_b = 0, ult_c = 0, repeticiones = 0;

void Registrar_Comando(unsigned char a, unsigned char b, unsigned char c)
{
    if (a == ult_a && b == ult_b && c == ult_c && (obj_izq != 0 || obj_der != 0)) {
        if (repeticiones < 255) repeticiones++;
        if (repeticiones >= 3) envio_continuo = 1;
    } else {
        repeticiones = 0;
    }
    ult_a = a; ult_b = b; ult_c = c;
}

void Comando_Gamepad(unsigned char funcion, unsigned char botones, unsigned char dato)
{
    unsigned char nuevos = botones & (unsigned char)~botones_previos;  // Flancos de subida
    botones_previos = botones;

    // Botones de velocidad (actuan una vez por pulsacion)
    if (nuevos & (1 << 2)) {                                 // Triangulo: +10 %
        vel_max = (vel_max <= 900) ? vel_max + 100 : 1000;
    }
    if (nuevos & (1 << 4)) {                                 // X: -10 %
        vel_max = (vel_max >= 200) ? vel_max - 100 : 100;
    }
    if (nuevos & (1 << 3)) {                                 // Circulo: 100 %
        vel_max = 1000;
    }

    if (botones & (1 << 5)) {                                // Cuadrado: freno
        obj_izq = 0;
        obj_der = 0;
    } else if (funcion == 0x01) {                            // Cruceta (digital)
        unsigned char arriba = dato & 0x01, abajo = dato & 0x02;
        unsigned char izq    = dato & 0x04, der   = dato & 0x08;
        if (arriba || abajo) {
            int s = arriba ? 1000 : -1000;
            obj_izq = s;
            obj_der = s;
            if (izq) obj_izq = s * 4 / 10;                   // Curva a la izquierda
            if (der) obj_der = s * 4 / 10;                   // Curva a la derecha
        } else if (izq) {
            obj_izq = -1000; obj_der = 1000;                 // Giro sobre su eje
        } else if (der) {
            obj_izq = 1000;  obj_der = -1000;
        } else {
            obj_izq = 0;     obj_der = 0;
        }
    } else if (funcion == 0x02 || funcion == 0x03) {         // Joystick o acelerometro
        unsigned char k = dato >> 3;                         // Angulo / 15
        unsigned char r = dato & 0x07;                       // Radio 0..7
        int vx, vy;
        if (k > 23 || r == 0) {
            obj_izq = 0;
            obj_der = 0;
        } else {
            vy = (int)((long)r * SENO[k] / 7);               // Componente adelante/atras
            vx = (int)((long)r * SENO[(k + 6) % 24] / 7);    // Componente izq/der (coseno)
            obj_izq = Limitar(vy + vx);                      // Mezcla diferencial
            obj_der = Limitar(vy - vx);
        }
    } else {
        return;
    }
    Aplicar_Ruedas();
}

#if FAILSAFE_MS > 0
// Se llama una vez por milisegundo desde el lazo principal
unsigned int ms_sin_datos = 0;

void Failsafe_1ms(void)
{
    // 1) Error de trama: el HC-05 se reinicio o se desconecto -> freno inmediato
    if (error_trama) {
        error_trama = 0;
        if (obj_izq != 0 || obj_der != 0) {
            obj_izq = 0;
            obj_der = 0;
            Aplicar_Ruedas();
        }
    }
    // 2) Sin datos durante FAILSAFE_MS (solo si la app envia de forma continua)
    if (dato_recibido) {
        dato_recibido = 0;
        ms_sin_datos = 0;
    } else if (ms_sin_datos < 0xFFFF) {
        ms_sin_datos++;
    }
    if (envio_continuo && ms_sin_datos >= FAILSAFE_MS && (obj_izq != 0 || obj_der != 0)) {
        obj_izq = 0;
        obj_der = 0;
        Aplicar_Ruedas();
    }
}
#endif

void main(void)
{
    char c;
    unsigned char f, b, d, i;
    unsigned char vistas_tramas = 0, vistas_letras = 0, vistos_cambios = 0;

    ADCON1 = 0x06;
#if DIAGNOSTICO
    TRISBbits.TRISB3 = 0;                   // LEDs como salidas (requiere LVP = OFF)
    TRISBbits.TRISB4 = 0;
    TRISBbits.TRISB5 = 0;
    for (i = 0; i < 3; i++) {               // Senal de arranque / reinicio
        CLRWDT();
        LED_TRAMA = 1; LED_LETRA = 1; LED_MOTOR = 1;
        __delay_ms(150);
        LED_TRAMA = 0; LED_LETRA = 0; LED_MOTOR = 0;
        __delay_ms(150);
    }
#endif
    PWM_Init();
    Parada_Libre();
    DRV8833_Wake();
    UART_Init();
    Freno_General();

    while (1) {
        CLRWDT();                           // "Alimenta" al watchdog en cada vuelta
        if (hay_gamepad) {
            INTCONbits.GIE = 0;             // Copia atomica de la ultima trama
            f = gp_funcion; b = gp_botones; d = gp_dato;
            hay_gamepad = 0;
            INTCONbits.GIE = 1;
            Comando_Gamepad(f, b, d);
            Registrar_Comando(f, b, d);
        }
        if (hay_letra) {
            INTCONbits.GIE = 0;
            c = letra;
            hay_letra = 0;
            INTCONbits.GIE = 1;
            Comando_Letra(c);
            Registrar_Comando(0xEE, (unsigned char)c, 0);
        }
        if (pedir_id) {
            pedir_id = 0;
            Enviar_ID_Tarjeta();
        }

#if DIAGNOSTICO
        if (cnt_tramas != vistas_tramas) { vistas_tramas = cnt_tramas; LED_TRAMA = !LED_TRAMA; }
        if (cnt_letras != vistas_letras) { vistas_letras = cnt_letras; LED_LETRA = !LED_LETRA; }
        if (cambios_baud != vistos_cambios) {       // Destello corto al cambiar de baudios
            vistos_cambios = cambios_baud;
            LED_MOTOR = 1; __delay_ms(60); LED_MOTOR = 0; __delay_ms(60);
        }
        LED_MOTOR = (obj_izq != 0 || obj_der != 0) ? 1 : 0;
#endif

#if FAILSAFE_MS > 0
        Failsafe_1ms();
#endif
        __delay_ms(1);
    }
}
#endif
