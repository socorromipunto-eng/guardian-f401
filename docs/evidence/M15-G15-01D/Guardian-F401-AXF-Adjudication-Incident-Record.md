# Guardian F401 — Registro de incidente de adjudicación AXF

Fecha de registro: 2026-08-23
Control: M15 / G15-01D
Estado del incidente: causa identificada; adjudicación de símbolos pendiente de corrección controlada

## Resumen ejecutivo

La compilación y el enlace del firmware terminaron correctamente con Arm Compiler for Embedded 6.24: cero errores, cero advertencias y generación válida del archivo AXF.

La adjudicación posterior no falló por corrupción del firmware ni por ausencia demostrada de símbolos. Falló un control del script que intentó localizar seis símbolos críticos dentro de un informe de `fromelf` que solamente contenía identidad ELF, cabeceras, secciones y metadatos. Ese informe no incluía la tabla de símbolos requerida por el control.

Por tanto:

- El AXF no queda rechazado por este incidente.
- La integridad del AXF, del proyecto, de las fuentes y del repositorio quedó confirmada.
- El control `CRITICAL_SYMBOLS` queda inconcluso, no fallido técnicamente.
- No se autoriza todavía generar HEX ni flashear.
- Debe corregirse exclusivamente el método de extracción y validación de símbolos.

## Evidencia confirmada

| Control | Resultado | Evidencia |
| --- | --- | --- |
| ELF magic | PASS | Archivo reconocido como ELF ARM ejecutable |
| Tipo de archivo | PASS | `ET_EXEC`, máquina `EM_ARM` |
| ABI | PASS | ARM ELF revisión 5, hard-float |
| Toolchain | PASS | `armasm` y `armlink` de Arm Compiler 6.24 |
| `fromelf` informe completo | PASS operativo | Código de salida 0; informe existente |
| `fromelf` totales | PASS operativo | Código de salida 0; informe existente |
| AXF sin cambios | PASS | SHA-256 `EE659ADBA15535179531BC79E4A807DE420F5FAB81E89CBB38F08389567A670D` |
| UVPROJX sin cambios | PASS | SHA-256 `968AC8F8C5FCE7567C44122191136F1C92DBBC0526C68BBF1ED0CE085910EAE9` |
| Fuentes sin cambios | PASS | 0 cambios |
| Salidas de build sin cambios | PASS | 0 cambios |
| Repositorio sin cambios | PASS | `True` |
| HEX ausente | PASS | 0 archivos HEX |
| Símbolos críticos | INCONCLUSO | El informe consultado no contenía tabla de símbolos |

## Símbolos que deben verificarse

1. `Reset_Handler`
2. `main`
3. `SysTick_Handler`
4. `USART2_IRQHandler`
5. `DMA2_Stream0_IRQHandler`
6. `ADC_IRQHandler`

El valor anterior `CRITICAL_SYMBOLS=False, Actual=6` significa que las seis búsquedas no produjeron coincidencias en el informe seleccionado. No demuestra que los símbolos falten del AXF.

## Causa raíz

El script produjo un informe equivalente a una inspección estructural del ELF. La salida observada contiene:

- punto de entrada;
- flags y ABI;
- componentes del compilador/enlazador;
- program headers;
- section headers;
- tamaños y direcciones de secciones.

No contiene una tabla o listado de símbolos. El control posterior aplicó expresiones de búsqueda de símbolos sobre una fuente de evidencia incapaz de responder esa pregunta.

Clasificación de causa: defecto del método de verificación, no defecto demostrado del artefacto.

## Errores de proceso registrados

### 1. Uso inseguro de `.Count` bajo `Set-StrictMode`

Una colección vacía se convirtió en `$null` y el script intentó leer `.Count`. Prevención obligatoria: toda salida que pueda ser cero, uno o muchos elementos debe normalizarse con `@(...)` antes de consultar `.Count`.

### 2. Control de símbolos aplicado al informe incorrecto

Se asumió que el informe general incluía símbolos sin validar previamente su contenido. Prevención obligatoria: cada control debe declarar qué evidencia necesita y verificar que esa evidencia tiene la capacidad semántica de demostrarlo.

### 3. Ventana transitoria del proceso

La ejecución de `fromelf` produjo una ventana visible que abrió y cerró. No fue µVision, compilación adicional ni flasheo. Prevención obligatoria: las herramientas de inspección no interactivas deben ejecutarse ocultas y con stdout/stderr redirigidos a archivos controlados.

### 4. Texto con codificación dañada

Se observó texto como `adjudicaciÃ³n`, compatible con una incompatibilidad de UTF-8 en Windows PowerShell 5.1. Prevención obligatoria: los scripts destinados a Windows PowerShell 5.1 deben guardarse como UTF-8 con BOM o utilizar mensajes ASCII controlados.

## Reglas obligatorias para el siguiente script

Antes de entregarlo o ejecutarlo, el script corregido deberá superar dos revisiones independientes:

### Revisión estática

- Ningún `.Count` sobre una expresión no normalizada.
- Ninguna dependencia implícita de `$?` para procesos nativos; capturar código de salida explícito.
- Rutas resueltas con `-LiteralPath`.
- Hashes esperados fijados y verificados antes de operar.
- Codificación compatible con Windows PowerShell 5.1.
- Ninguna compilación, enlace, generación HEX o flasheo.

### Revisión adversarial

- Probar mentalmente resultados con 0, 1 y varios elementos.
- Distinguir “símbolo ausente” de “fuente de evidencia sin símbolos”.
- Confirmar que la herramienta elegida genera realmente una tabla de símbolos.
- Capturar stdout, stderr y exit code sin ventana visible.
- Preservar los informes fallidos anteriores como evidencia de aprendizaje.
- Fallar cerrado si cambia el AXF, el UVPROJX, las fuentes o el repositorio.

## Criterio de cierre

El incidente podrá cerrarse únicamente cuando un control corregido:

1. use una salida que incluya explícitamente símbolos;
2. identifique por separado cada uno de los seis símbolos críticos;
3. conserve el AXF con el SHA-256 esperado;
4. mantenga sin cambios proyecto, fuentes y repositorio;
5. no genere HEX y no realice flashing;
6. produzca evidencia y manifiesto reproducibles;
7. sea revisado por una persona antes de cualquier etapa posterior.

## Estado formal

`AXF_INTEGRITY_CONFIRMED_SYMBOL_EVIDENCE_METHOD_REQUIRES_CORRECTION`

Compilación adicional: False
Enlace adicional: False
Generación HEX: False
Flashing: False
Modificación del repositorio: False

## Controlled closure adjudication

- Human decision: `APPROVED`
- Final result: `PASS`
- State: `AXF_SYMBOL_ADJUDICATION_CLOSED_PASS`
- AXF SHA-256: `EE659ADBA15535179531BC79E4A807DE420F5FAB81E89CBB38F08389567A670D`
- UVPROJX SHA-256: `968AC8F8C5FCE7567C44122191136F1C92DBBC0526C68BBF1ED0CE085910EAE9`
- Symbol evidence inventory SHA-256: `5FC5F78AB564199A46EE16F19CD9FF2BC824E0195811DE8890E95E54DB4E6150`
- Symbol control manifest SHA-256: `04497D940F48268F2C265C73186F80DB68CB765CA8CDA05EDEA55AA8775DB606`

Critical symbols confirmed exactly once in the controlled review:

- `Reset_Handler`
- `main`
- `SysTick_Handler`
- `USART2_IRQHandler`
- `DMA2_Stream0_IRQHandler`
- `ADC_IRQHandler`

Adjudication conclusion: the earlier failure was caused by an incomplete symbol-verification method. The controlled Arm `fromelf` symbol-table review confirmed the required symbols. No firmware defect was established by that failed verification attempt.

This closure step did not compile, link, generate HEX, flash hardware, push, or merge.
