# Robot móvil diferencial con PIC16F877A y DRV8833

Plataforma móvil de dos ruedas controlada por un **PIC16F877A** y un doble puente H **DRV8833**.
Controla la velocidad de cada rueda por PWM de hardware, el sentido de giro y el frenado dinámico,
y se maneja desde un celular Android con un **control tipo gamepad** (app Dabble) por **Bluetooth (HC-05)**.

Proyecto del Taller Práctico II de *Microcontroladores y Sistemas Embebidos*, Programa de Ingeniería
Electrónica, Universidad de Ibagué.

> **Estado:** funcional. El carro funcionó completo en la tarjeta del curso y obtuvo el
> **segundo lugar** en la competencia del curso (noviembre de 2026). Los informes (v0.2) están
> en borrador: faltan fotografías, capturas de simulación y mediciones.

## Resultado

[![Video del robot en funcionamiento](https://img.youtube.com/vi/f3ZtwNBpTU8/hqdefault.jpg)](https://youtube.com/shorts/f3ZtwNBpTU8)

**[Ver el video en YouTube](https://youtube.com/shorts/f3ZtwNBpTU8)**: avance, retroceso, giros y
cambio de velocidad desde el mando de Dabble por Bluetooth.

- **Segundo lugar** en la competencia del curso.
- Fallas resueltas en el montaje: pin EEP (nSLEEP) sin conexión en la tarjeta, corto entre IN1 y
  RD2/RD3, lectura-modificación-escritura del puerto D (resuelta con un registro sombra) y
  velocidad del HC-05 distinta a la del PIC (detección automática de baudios).

**Mejoras para una próxima versión**

- Movimientos más suaves: hoy los cambios de velocidad son bruscos.
- Dirección tipo carro: girar mientras avanza (curvas) en lugar de girar sobre una rueda.
- Reemplazar los retardos bloqueantes (`__delay_ms`) por temporizadores para un control más dinámico.
- Mejorar el diseño mecánico (contrapesos) y la calidad de las soldaduras.

## Características

- **PWM de hardware** en CCP1 y CCP2 con Timer2 como base de tiempo única: **1.25 kHz** y
  **1000 niveles** de ciclo de trabajo (≈10 bits).
- **Control de sentido y freno dinámico** con dos líneas por motor (PWM directo en avance,
  invertido en reversa).
- **Habilitación segura del driver** con pulso en nSLEEP después de configurar los periféricos (montaje en protoboard; en la tarjeta, EEP va a 5 V por hardware).
- **Rampas de aceleración** y freno antes de cada inversión para evitar caídas de voltaje y
  reinicios del microcontrolador.
- **Control tipo gamepad** con la app Dabble: cruceta, joystick proporcional con mezcla diferencial
  y botones de velocidad. El protocolo binario se decodifica con una máquina de estados en la
  interrupción de la UART; también acepta letras (`F B L R S`).
- **Detección automática de baudios** del módulo Bluetooth (9600 a 115200).
- **Freno por pérdida de señal**, watchdog y reinicio por bajo voltaje: si el HC-05 se desconecta o
  la batería cae, el carro se detiene en lugar de seguir sin control.
- **Diagnóstico con los LEDs** de la tarjeta (tramas recibidas, bytes no reconocidos, orden de motor).
- **Firmware en capas** (hardware → motor → navegación) con selección en compilación del cristal
  (20 MHz o 4 MHz) y del modo (secuencia de prueba o control remoto).

## Hardware

| Componente | Función |
| --- | --- |
| Tarjeta del curso | PCB Electrónica Digital II 2026B: PIC16F877A en TQFP-44, módulo DRV8833, zócalo HC-05, 3 LEDs, ICSP |
| PIC16F877A | Microcontrolador (cristal de 20 MHz) |
| DRV8833 | Doble puente H MOSFET, 2.7–10.8 V, 1.5 A por canal |
| 2 × motorreductor DC | Ruedas motrices (tracción diferencial) |
| HC-05 / HC-06 | Módulo Bluetooth serie, 9600 baudios |
| LM7805 | Regulador de 5 V para la lógica |
| Batería ≤ 10 V | 2 × 18650 (7.4 V). Una pila de 9 V **no** alcanza para los motores |
| PICkit 3 | Programación ICSP |

### Conexiones

| Pin PIC | Pata TQFP-44 (DIP-40) | Señal | Destino |
| --- | --- | --- | --- |
| RC2 / CCP1 | 36 (17) | PWM motor izquierdo | MIPWM |
| RD0 | 38 (19) | Dirección motor izquierdo | MIPIN |
| RC1 / CCP2 | 35 (16) | PWM motor derecho | MDPWM |
| RD1 | 39 (20) | Dirección motor derecho | MDPIN |
| RD2 | 40 (21) | Habilitación (solo protoboard) | nSLEEP |
| RC7 / RX | 1 (26) | UART RX | TXD del HC-05 |
| RC6 / TX | 44 (25) | UART TX | RXD del HC-05 (divisor 1 kΩ / 2 kΩ) |
| RB5 / RB4 / RB3 | 15 / 14 / 11 | LEDs de diagnóstico | LED1 / LED2 / LED3 |

> **Importante en la tarjeta del curso:** el pin **EEP (nSLEEP)** del módulo DRV8833 no está
> conectado. Sin habilitarlo, el driver queda dormido y los motores no se mueven. Hay que unir EEP a
> **5 V de la lógica** (por ejemplo, el VCC del zócalo del HC-05) con un cable directo. Una
> resistencia de 10 kΩ **no sirve**: forma un divisor con el pull-down del módulo y deja EEP en
> ~1.4 V, por debajo del umbral de 2.5 V.

## Firmware

| Archivo | Configuración de MPLAB X | Contenido |
| --- | --- | --- |
| [`taller2_robot_diferencial.c`](firmware/taller2_robot_diferencial.c) | `completo` | Firmware principal |
| [`codigo_docente.c`](firmware/codigo_docente.c) | `try1` | Código de la guía del taller |
| [`prueba_leds.c`](firmware/prueba_leds.c) | `leds` | Prueba de montaje con los 3 LEDs |

Al inicio del firmware principal se configura la compilación:

```c
#define CRISTAL_20MHZ   1    // 1 = 20 MHz (HS, prescaler 1:16) | 0 = 4 MHz (XT, prescaler 1:4)
#define MODO_BLUETOOTH  1    // 0 = secuencia de prueba del taller | 1 = control desde el celular
#define VEL_MINIMA    300    // PWM minimo para vencer la friccion del motor
#define FAILSAFE_MS   500    // Freno por perdida de senal
```

**Modo 0 – secuencia de prueba:** avance y reversa con rampas, giro a la izquierda y a la derecha
sobre su eje, y freno dinámico entre maniobras.

**Modo 1 – control desde el celular** con la app **Dabble** (Android, módulo *Gamepad*):

| Control | Acción |
| --- | --- |
| Cruceta ↑ ↓ ← → | Adelante, atrás, giro sobre su eje; en diagonal, curva |
| Joystick | Velocidad y giro proporcionales |
| △ / ✕ / ○ | +10 % / −10 % / 100 % de velocidad |
| □ (mantener) | Freno |
| Letras `F B L R S`, `0`–`9`, `q` | Compatibilidad con apps de terminal |

El HC-05 usa Bluetooth Clásico y **no funciona con iPhone**. Para iPhone se reemplaza por un
módulo BLE (HM-10 o JDY-33) sin cambiar el firmware.

La carpeta [`firmware/versiones-previas`](firmware/versiones-previas) guarda las primeras versiones
(control todo/nada sin PWM, en XC8 y CCS C).

**Programación:** MPLAB X **v6.20** (la última compatible con el PICkit 3) con XC8, y la aplicación
*PICkit 3 Programmer* con *Manual Device Select*. El conector ICSP debe ir con la flecha del pin 1
hacia la marca de la tarjeta: al revés aplica ~13 V en RB3/PGM y daña el PIC.

## Simulación

En Proteus el DRV8833 se reemplaza por un **L293D** (misma tabla de verdad por canal), el HC-05 por
una **Virtual Terminal** a 9600 baudios, y el PWM se verifica con el osciloscopio en RC2 y RC1.

## Informe técnico

La carpeta [`informe/`](informe) contiene dos informes en LaTeX:

- **Informe técnico completo** (`main.tex`): arquitectura, etapa de potencia, montaje, justificación
  matemática (Tcy, PR2, PWM, resolución de 10 bits, UART, joystick), firmware y protocolo Dabble,
  programación, depuración en hardware (10 fallas con su diagnóstico) y anexos de código.
- **Informe corto para el docente** (`informe_docente.tex`): 4 páginas con lo que pide la guía e
  instrucciones de uso.

Instrucciones para compilarlos en [`informe/README.md`](informe/README.md).

## Estructura del repositorio

```
.
├── firmware/
│   ├── taller2_robot_diferencial.c   # Firmware principal (XC8)
│   ├── codigo_docente.c              # Código de la guía
│   ├── prueba_leds.c                 # Prueba de montaje
│   └── versiones-previas/            # Versiones sin PWM (XC8 y CCS)
└── informe/                          # Informe técnico en LaTeX
    ├── main.tex                      # Informe completo
    ├── informe_docente.tex           # Informe corto
    ├── config.tex
    ├── secciones/
    └── figuras/
```

## Autor

**Brahiam Alberto Campiño Romero** — Ingeniería Electrónica, Universidad de Ibagué.
