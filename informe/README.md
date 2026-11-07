# Informes (LaTeX)

Informes del **Taller Práctico II: Control dual de motores y robótica móvil diferencial**.

| Archivo | Qué es |
| --- | --- |
| `main.tex` | **Informe técnico completo** (portafolio): arquitectura, potencia, cálculos, firmware, protocolo Dabble, programación, depuración en hardware y anexos de código |
| `informe_docente.tex` | **Informe corto para el docente** (4 páginas): montaje, cálculos de la guía, código, demostración e instrucciones de uso |

> Estado: **borrador (v0.2)**. El contenido técnico está completo; faltan fotografías,
> capturas de simulación y mediciones. Todo lo pendiente aparece en rojo en el PDF.

## Estructura

| Ruta | Qué contiene |
| --- | --- |
| `main.tex` | Informe completo: paquetes y orden de las secciones |
| `informe_docente.tex` | Informe corto (un solo archivo) |
| `config.tex` | Datos del informe (título, docente, versión), colores, estilos y macros |
| `secciones/` | Una sección por archivo, para editar en paralelo sin conflictos |
| `figuras/arquitectura.tex` | Diagrama de bloques dibujado en TikZ |
| `figuras/` | Aquí van las fotografías y capturas (`.png` / `.jpg`) |

Los anexos leen directamente los archivos de `../firmware/`, así que cualquier cambio en el
firmware aparece solo en el informe.

## Compilar

Desde esta carpeta (`informe/`), con TeX Live o MiKTeX:

```bash
latexmk -pdf main.tex              # informe completo
latexmk -pdf informe_docente.tex   # informe corto
```

o, sin latexmk, `pdflatex main.tex` dos veces (la segunda arma el índice y las referencias).

Paquetes usados: `babel` (español), `lmodern`, `siunitx`, `booktabs`, `tabularx`, `tikz`,
`listings`, `hyperref`, `fancyhdr`, `titlesec`, `tocloft`, `enumitem`, `float`, `caption`.

## Cómo editar

- **Datos generales** (docente, versión, fecha): `config.tex`.
- **Integrantes**: `secciones/00-portada.tex` (informe completo) y el encabezado de `informe_docente.tex`.
- **Contenido pendiente**: se marca con `\pendiente{...}` y sale en rojo. Para ver todo lo que falta:

  ```bash
  grep -rn "pendiente" secciones/
  ```

- **Agregar una foto o captura**: guardar la imagen en `figuras/` y reemplazar la línea
  `\figurapendiente{...}{etiqueta}` correspondiente por:

  ```latex
  \begin{figure}[H]
    \centering
    \includegraphics[width=0.8\textwidth]{figuras/nombre-imagen.png}
    \caption{Descripción de la imagen.}
    \label{etiqueta}
  \end{figure}
  ```

- Guardar siempre los archivos en **UTF-8** (el texto lleva tildes y eñes).

## Pendiente para la versión final

- [ ] Nombres de los integrantes y del docente
- [ ] Fotografía de la tarjeta montada
- [ ] Capturas de Proteus: esquemático, osciloscopio y terminal virtual
- [ ] Mediciones de la sección de pruebas (Proteus y hardware) y resultado de cada prueba
- [ ] Demostración con EEP a 5 V y baterías 18650
- [ ] Logo de la universidad en la portada (opcional)
