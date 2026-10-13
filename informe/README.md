# Informe técnico (LaTeX)

Informe del **Taller Práctico II: Control dual de motores y robótica móvil diferencial**.

> Estado: **borrador (v0.1)**. El contenido técnico está completo; faltan fotografías,
> capturas de simulación y mediciones. Todo lo pendiente aparece en rojo en el PDF.

## Estructura

| Ruta | Qué contiene |
| --- | --- |
| `main.tex` | Documento principal: paquetes y orden de las secciones |
| `config.tex` | Datos del informe (título, docente, versión), colores, estilos y macros |
| `secciones/` | Una sección por archivo, para editar en paralelo sin conflictos |
| `figuras/arquitectura.tex` | Diagrama de bloques dibujado en TikZ |
| `figuras/` | Aquí van las fotografías y capturas (`.png` / `.jpg`) |

El anexo de código lee directamente `../firmware/taller2_robot_diferencial.c`, así que cualquier
cambio en el firmware aparece solo en el informe.

## Compilar

Desde esta carpeta (`informe/`), con TeX Live o MiKTeX:

```bash
latexmk -pdf main.tex
```

o, sin latexmk, `pdflatex main.tex` dos veces (la segunda arma el índice y las referencias).

Paquetes usados: `babel` (español), `lmodern`, `siunitx`, `booktabs`, `tabularx`, `tikz`,
`listings`, `hyperref`, `fancyhdr`, `titlesec`, `enumitem`, `float`, `caption`.

## Cómo editar

- **Datos generales** (docente, versión, fecha): `config.tex`.
- **Integrantes**: `secciones/00-portada.tex`.
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
- [ ] Fotografía del montaje en protoboard
- [ ] Capturas de Proteus: esquemático, osciloscopio y terminal virtual
- [ ] Mediciones de la sección 8 (Proteus y hardware) y resultado de cada prueba
- [ ] Logo de la universidad en la portada (opcional)
