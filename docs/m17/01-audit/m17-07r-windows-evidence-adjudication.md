# Guardian M17 — Adjudicación de evidencia Windows 07R

Resultado: PASS_EXACT_RECORDED_REFERENCE_HOST_SCOPE_ONLY.
Alcance: revisión de evidencia capturada; el revisor no volvió a ejecutar los binarios Windows ni accedió al equipo o repositorio local del propietario.

## Identidad y baseline

Archivo: Guardian-M17-07R-Seven-Stage-Ingress-Windows-Validation-83822d514e4f4524a70ea8dc6e1ba61c.zip

SHA256: `3B35FD213CA7961E9A1753A0D85668D3BCE2C154959A5607F157CDD95127A3E1`

HEAD capturado: `0f2d55c722fc74dabba90a53c83a24ec7d62dbb2`.
Rama capturada: `feature/m17-authenticated-ingress-state-machine`.
585 identidades de archivos. Estado limpio y staging vacío según la captura.

## Comprobaciones realizadas

- CRC del ZIP correcto; 183 entradas, sin duplicados ni rutas fuera de la raíz de evidencia esperada.
- Nombres internos de ZIP Windows normalizados únicamente para su inspección; no se modificaron los bytes contenidos.
- Conjunto completo de entradas reconstruido y comprobado: sin archivos extras o faltantes respecto a payloads, estados, logs, objetos, ejecutables y mutantes documentados.
- Los 30 payloads coinciden en longitud y SHA256 con el manifiesto y con el candidato exacto entregado en 07R.
- Los 50 comandos tienen logs stdout/stderr que coinciden exactamente con su representación en windows-result.json. Se conservaron las terminaciones de línea nativas.
- Cuatro ejecuciones positivas: integración O0, integración O2, regresión del verificador O0 y regresión del verificador O2. Todas registran exit0, stderr vacío y los marcadores exactos esperados.
- 23 objetos COFF AMD64 y 13 imágenes PE AMD64: identidades/hash y arquitectura comprobados.
- Nueve mutantes: preimagen exacta, sustitución exacta y bytes CRLF de escritura Windows comprobados; compilación y enlace exitosos, seguidos de exit1 con fallos de aserción del harness. Un fallo de compilación o un crash no se habría contado como detección.
- Estado anterior y posterior comparado por ruta, longitud y hash: los 585 archivos conservados; HEAD, rama, refs, entradas e identidad del índice conservados. También coinciden con el baseline incluido en el runner entregado.
- Los 32 requisitos padres permanecen documentados; ninguno cierra un gate de producción.

## Resultados registrados

| Medida | Resultado |
|---|---:|
| Casos host de optimización | 4/4 PASS |
| Evaluaciones de comprobaciones por caso de integración | 12.331 |
| Grupos de escenarios por caso de integración | 51 |
| Alteraciones de un bit rechazadas por caso de integración | 2.432 |
| Combinaciones de fallos del proveedor por caso de integración | 24 |
| Comprobaciones por caso de regresión del verificador | 8.394 |
| Mutantes detectados mediante aserciones | 9/9 |

Las evaluaciones de comprobaciones incluyen validaciones repetidas de preservación y no equivalen a escenarios de ataque distintos. Las dos optimizaciones tampoco representan dos implementaciones criptográficas independientes.

Mutantes: firma, asociación de identidad, revocación, semántica, frescura, correspondencia del ACK, expiración del ACK, lectura de confirmación y publicación prematura.

## Decisión y afirmación permitida

La evidencia respalda el funcionamiento integrado de las siete etapas para el perfil de referencia implementado: un peer configurado, una ronda inmutable y un pendiente local, bajo los contratos de un propietario serializado, reloj local confiable y proveedor de persistencia/ancla simulado.

Puede describirse como un candidato C integrado validado en host Windows con firmas reales, comprobaciones negativas y fallos de persistencia simulados. No debe describirse como Guardian multinodo completo, persistencia física calificada, quórum implementado, seguridad operacional certificada o v0.17 terminada.

El informe Windows mantiene explícitamente false: modificación del repositorio, calificación de target, cierre de gates, ingreso completo de producción, calificación completa de toolchain, conformidad normativa y transporte PQC. Estas exclusiones son coherentes con los archivos revisados.

## Deudas que permanecen abiertas

1. Proveedor físico de almacenamiento y ancla antirrollback, con evidencia real ante pérdida de energía.
2. Reloj confiable y presupuestos de recursos/stack/WCET sobre los targets.
3. Auditoría de aceptación/rechazo, asociación con transcript/transacción y política ante fallo de auditoría.
4. Integración con gc_report/quórum, varios peers y transición de ronda/epoch/claves mediante migración o rejoin revisados.
5. Ejecución de regresiones completas existentes e incorporación a CI con hashes y recetas actualizados.
6. Asociación del build NXP ya reportado con sus artefactos exactos, ejecución en MCXN947 y evidencia de ambos extremos físicos con F401.
7. Evidencia física pendiente del F401: arranque, medición, watchdog, recursos y comportamiento ante fallos.
8. Confianza/procedencia de claves de producción, actualización segura y separación de firma PQC de artefactos respecto al transporte Ed25519 V2.
9. Aplicabilidad y evaluación de los requisitos normativos al producto/despliegue real; no se hereda conformidad por ejecutar pruebas host.

Las referencias NASA, CISA, NIST, FAA/DO-178C y UE conservan el estado de aplicabilidad registrado en el candidato. Esta revisión no es IV&V independiente ni una evaluación completa de conformidad de esos marcos.

## Siguiente límite

Diseñar la admisión de los archivos exactos de código, pruebas y documentación del candidato validado, incluyendo esta adjudicación y el registro de deudas. La admisión deberá verificar primero el estado vivo del repositorio. No se ha efectuado admisión, staging, commit, push, programación de placas ni release en esta revisión. Los gates 01, 13, 14 y 16 permanecen abiertos.
