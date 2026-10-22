# Guardian M17 — revisión de evidencia 05T

Fecha: 2026-10-08.
Resultado: PASS_HOST_SCOPE_ONLY_ADJUDICATED.

Se revisó el ZIP aportado por Antonio, sin acceso directo al equipo Windows ni nueva ejecución de sus binarios. Su SHA-256 coincide con D3553A6A3A409BCD3FE78E2FD0C6E48A4D39970960EFE18941CAE0CE9A1210FC. CRC del ZIP correcto. Se releyeron remenber.txt y Evolución-recomendada.txt; sus objetivos son hoja de ruta, no prueba de que las etapas físicas estén cerradas.

## Verificaciones realizadas

- El baseline coincide íntegramente con state-after de la incorporación 05S previamente revisada: HEAD 717e53c717af1dc31af540bbcadd9b426f9c9535, rama feature/m17-heterogeneous-multinode-assurance, índice, referencias, estado y 559 identidades de archivo.
- Los 559 archivos fuente incluidos coinciden en longitud y SHA-256 con ese baseline. No hay archivos fuente adicionales en la copia.
- Helper y manifiesto de receta coinciden byte a byte con los artefactos 05T revisados. execution.json registra salida 0 y el hash correcto del helper.
- Los 11 comandos registrados tienen salida 0 y no presentan timeout: un plan del driver, seis compilaciones, dos enlaces y dos ejecuciones.
- Los archivos separados de stdout/stderr coinciden con los textos del resultado JSON. Las seis compilaciones conservan C11, Werror y UNDEBUG, sin diagnósticos warning/error. El stderr de compilación incluye trazas de headers; eso no es un fallo.
- Los seis objetos coinciden con sus hashes. Ambos ejecutables coinciden en tamaño y hash y tienen cabecera PE AMD64.
- Cada ejecución O0/O2 registra exactamente M17_CANONICAL_HEARTBEAT_REJECTIONS=2032 y M17_MCXN947_SUPERVISOR_HOST_TEST=PASS, con stderr vacío y salida 0.

## Qué permite afirmar

La receta Windows aportada valida el comportamiento probado de la lógica portable del supervisor corregido en dos niveles de optimización. Los checks de valores booleanos no canónicos se ejecutan 2.032 veces por nivel. Repetirlos con otra optimización no constituye 4.064 propiedades distintas ni demuestra todas las entradas o fallos posibles.

El rechazo de heartbeat malformado conserva el estado; no introduce por sí solo SAFE_HOLD. La contabilización de pérdidas y la política temporal siguen siendo responsabilidades separadas. Ningún permiso de actuación se añade al supervisor.

## Límites de la evidencia

La preservación final del repositorio Windows fue reportada por el runner y la consola. El ZIP permite contrastar el baseline y los bytes copiados, pero no incluye un segundo snapshot final del repositorio para una comparación independiente antes/después. No se presenta esta revisión como una inspección actual del disco Windows.

Los 29 headers observados tienen identidades registradas; los externos no están incluidos para recomputar sus hashes aquí. Tampoco están incluidos Python, Clang o linker. Sus identidades son las registradas por el runner. La biblioteca estándar Python, bibliotecas de enlace, DLLs y toolchain completa no están cualificadas independientemente.

No se ejecutaron sanitizers en esta receta. No hay build ni arranque MCXN947, conexión física, flash, validación entre placas ni prueba de conformidad legal. Gate01 y los gates físicos, de claves, partición/failover y CI remota permanecen abiertos. No se autoriza commit, push, merge o release por este PASS.

## Próxima frontera

Diseñar el contrato de corroboración y partición multinodo a partir de la arquitectura actual: identidad y pertenencia explícitas, observaciones frescas, discrepancia, pérdida de enlace, particiones y reingreso. Quórum, mayor potencia o silencio nunca conceden autoridad de actuación. La política debe distinguir falta de evidencia de permiso de sustitución.

Primero requisitos y estados/transiciones; después implementación portable y tests positivos, negativos y adversariales en candidato externo. La incorporación al repositorio y la validación Windows serán fronteras separadas. El fortalecimiento físico del F401, la integración de seguridad y la implementación real del nodo NXP siguen pendientes y no se sustituyen por esta lógica host.
