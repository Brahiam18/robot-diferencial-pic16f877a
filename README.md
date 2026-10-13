# Robot móvil diferencial con PIC16F877A y DRV8833

Plataforma móvil de dos ruedas controlada por un **PIC16F877A** y un doble puente H **DRV8833**.
Controla la velocidad de cada rueda por PWM de hardware, el sentido de giro y el frenado dinámico,
y se maneja desde el celular por **Bluetooth (HC-05/HC-06)**.

Proyecto del Taller Práctico II de *Microcontroladores y Sistemas Embebidos*, Programa de Ingeniería
Electrónica, Universidad de Ibagué.

> **Estado:** en desarrollo. El firmware y el informe técnico (v0.1) están completos; faltan las
> mediciones en hardware, fotografías y capturas de simulación.

## Características

- **PWM de hardware** en CCP1 y CCP2 con Timer2 como base de tiempo única: **1.25 kHz** y
  **1000 niveles** de ciclo de trabajo (≈10 bits).
- **Control de sentido y freno dinámico** con dos líneas por motor (PWM directo en avance,
  invertido en reversa).
- **Habilitación segura del driver** con pulso en nSLEEP después de configurar los periféricos.
- **Rampas de aceleración** y freno antes de cada inversión para evitar caídas de voltaje y
  reinicios del microcontrolador.
- **Control por Bluetooth** con recepción por interrupción, recuperación de *overrun*, 11 niveles
  de velocidad y eco de confirmación.
- **Firmware en capas** (hardware → motor → navegación) con selección en compilación del cristal
  (20 MHz o 4 MHz) y del modo (secuencia de prueba o control remoto).

## Hardware

| Componente | Función |
| --- | --- |
| PIC16F877A | Microcontrolador (cristal de 20 MHz o 4 MHz) |
| DRV8833 | Doble puente H MOSFET, 2.7–10.8 V, 1.5 A por canal |
| 2 × motorreductor DC | Ruedas motrices (tracción diferencial) |
| HC-05 / HC-06 | Módulo Bluetooth serie, 9600 baudios |
| LM7805 | Regulador de 5 V para la lógica |
| Batería ≤ 10 V | Alimentación de los motores |
| PICkit 3 | Programación ICSP |

### Conexiones

| Pin PIC | Señal | Destino |
| --- | --- | --- |
| RC2 / CCP1 (17) | PWM motor izquierdo | AIN1 |
| RD0 (19) | Dirección motor izquierdo | AIN2 |
| RC1 / CCP2 (16) | PWM motor derecho | BIN1 |
| RD1 (20) | Dirección motor derecho | BIN2 |
| RD2 (21) | Habilitación | nSLEEP |
| RC7 / RX (26) | UART RX | TXD del HC-05 |
| RC6 / TX (25) | UART TX | RXD del HC-05 (divisor 1 kΩ / 2 kΩ) |

## Firmware

El código principal es [`firmware/taller2_robot_diferencial.c`](firmware/taller2_robot_diferencial.c)
(MPLAB XC8). Dos líneas al inicio del archivo configuran la compilación:

```c
#define CRISTAL_20MHZ   1   // 1 = 20 MHz (HS, prescaler 1:16) | 0 = 4 MHz (XT, prescaler 1:4)
#define MODO_BLUETOOTH  0   // 0 = secuencia de prueba del taller | 1 = control desde el celular
```

**Modo 0 – secuencia de prueba:** avance y reversa con rampas, giro a la izquierda y a la derecha
sobre su eje, y freno dinámico entre maniobras.

**Modo 1 – Bluetooth:** compatible con la app *Bluetooth RC Controller* (Android) o cualquier
terminal serie a 9600 baudios.

| Comando | Acción |
| --- | --- |
| `F` / `B` | Adelante / atrás |
| `L` / `R` | Giro izquierda / derecha |
| `S` | Freno |
| `0`–`9` | Velocidad 0 %–90 % |
| `q` | Velocidad 100 % |

La carpeta [`firmware/versiones-previas`](firmware/versiones-previas) guarda las primeras versiones
(control todo/nada sin PWM, en XC8 y CCS C).

## Simulación

En Proteus el DRV8833 se reemplaza por un **L293D** (misma tabla de verdad por canal), el HC-05 por
una **Virtual Terminal** a 9600 baudios, y el PWM se verifica con el osciloscopio en RC2 y RC1.

## Informe técnico

La carpeta [`informe/`](informe) contiene el informe en LaTeX: arquitectura, etapa de potencia,
normas de montaje, justificación matemática (Tcy, PR2, frecuencia PWM, resolución de 10 bits,
UART), diseño del firmware, pruebas y decisiones de diseño. Instrucciones para compilarlo en
[`informe/README.md`](informe/README.md).

## Estructura del repositorio

```
.
├── firmware/
│   ├── taller2_robot_diferencial.c   # Firmware principal (XC8)
│   └── versiones-previas/            # Versiones sin PWM (XC8 y CCS)
└── informe/                          # Informe técnico en LaTeX
    ├── main.tex
    ├── config.tex
    ├── secciones/
    └── figuras/
```

## Autor

**Brahiam Alberto Campiño Romero** — Ingeniería Electrónica, Universidad de Ibagué.
