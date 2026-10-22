# Guardian M17 — ACK de observación y frontera de ingreso

Estado: CANDIDATO EXTERNO, no ADR admitido ni cierre de gate. Fecha: 2026-10-10.
Baseline: HEAD 717e53c717af1dc31af540bbcadd9b426f9c9535; rama feature/m17-heterogeneous-multinode-assurance; captura posterior a 06E con 571 rutas. No cambia archivos del repositorio.

## Contexto y fuente de verdad

Se leyeron remenber.txt, Evolución-recomendada.txt, el contrato de ingreso 06F y los headers/implementaciones reales de corroboración, codec y NodeLink. El registro de versión no reemplaza las deudas físicas pendientes. El STM32F401 conserva autoridad determinista; NXP supervisa sin permiso automático para actuar. La IA sigue siendo consultiva y futura. Los demás modelos de chips permanecen pendientes de selección documentada por función, interfaces, recursos y riesgos.

Problema: gc_submit consume same_round_ack, pero ningún ACK canónico estaba definido. Un booleano enviado por un peer no es prueba local. Antes de conectar esos módulos se necesita una referencia inequívoca a la observación confirmada.

## Decisión propuesta y alternativas

Propuesta: crear un payload ACK independiente de 60 bytes. No reutilizar HEARTBEAT ni declarar auténtico un paquete porque el codec lo acepta. Mantener campos de contexto y añadir epoch/secuencia de la observación confirmada. Un envelope autenticado posterior atribuirá la identidad del emisor del ACK y protegerá el payload completo.

Alternativas descartadas para este candidato: booleano ACK en la observación (autodeclaración), ACK por round solamente (ambiguo entre mensajes), usar epoch/secuencia del propio ACK como referencia a la observación (conflación), serializar el struct C (padding/endianness), extender la observación existente silenciosamente (rompe compatibilidad).

Los tipos NodeLink 0x07 CORROBORATION y 0x08 CORROBORATION_ACK se proponen como asignaciones; NO se han añadido al enum ni al decoder. El decoder actual debe seguir rechazándolos. Antes de admitir esa extensión se necesita revisar compatibilidad/versionado, transcript autenticado, fixtures y parser hostil. El tamaño 60 cabe en el límite actual de payload de 64 bytes, pero esa relación no demuestra integración.

## Requisitos del candidato

| ID | Requisito | Evidencia local |
|---|---|---|
| ACK-R01 | Encoding fijo little endian, 60 bytes y discriminador explícito | P01 golden vector independiente |
| ACK-R02 | Sin padding transmitido; versión/kind/reservados estrictos | N03 exhaustivo por byte |
| ACK-R03 | generation/policy/profile/round/observation epoch/sequence no cero | N04 |
| ACK-R04 | Referencia a TODOS los campos y 32 bytes commitment | N05 cada bit de bytes 4–59 |
| ACK-R05 | Longitud exacta; capacidad insuficiente no escribe parcialmente | N02/N06 |
| ACK-R06 | Fallos no cambian salida; argumentos null rechazados | N02–N07 |
| ACK-R07 | No autenticación, persistencia ni autoridad concedida por codec | API solo encode/decode/match; revisión de alcance |
| ACK-R08 | Sin truncamiento de enteros del envelope | API uint32 acorde al envelope actual; futuro uint64 requiere nuevo contrato |

## Formato normativo interno propuesto

| Offset | Longitud | Campo |
|---|---|---|
| 0 | 1 | versión 1 |
| 1 | 1 | kind 0xA1, exclusivo del payload candidato |
| 2 | 2 | reservados cero |
| 4 | 4 | generation |
| 8 | 4 | policy |
| 12 | 4 | profile |
| 16 | 4 | round |
| 20 | 4 | epoch de la observación confirmada |
| 24 | 4 | sequence de la observación confirmada |
| 28 | 32 | commitment de la observación confirmada |

Los enteros son little endian. El commitment es opaco: todo-cero es estructuralmente representable y no constituye evidencia válida por sí mismo. Su significado criptográfico/domino/preimagen debe fijarse antes del ingreso integrado. El payload no transporta authenticated, fresh, trusted, authorized ni actuation_allowed.

## Binding posterior obligatorio

El ACK tendrá su propio sender, epoch y sequence en el envelope NodeLink; los offsets 20/24 refieren a la observación original, NO al cursor de replay del ACK. El receptor debe guardar localmente pending(peer esperado, producer/key configurados, generation, policy, profile, round, observation epoch/sequence, commitment, vencimiento). Debe verificar identidad y firma del envelope, binding semántico y frescura persistente antes de comparar la referencia. gc_ack_match solo compara referencia: aceptar un ACK de otro emisor con los mismos campos es un fallo del integrador, no una propiedad que esta API prevenga.

La confirmación no vuelve verdadera la observación, no concede permiso de actuador y no reemplaza el quorum/roles de la política. Un ACK perdido, duplicado, tardío, de otra generación o de un peer no autorizado no debe contar. Política propuesta: pending consumido una sola vez; después de reset se exige rejoin si no puede reconstruirse íntegramente su binding y frescura. No transmitir de nuevo una acción física durante reconciliación.

## Transacción durable pendiente

Orden propuesto: decode bounded -> autenticar -> validar semántica y contexto -> verificar referencia ACK pendiente -> prevalidar corroboración en copia -> commit durable de cursor/ancla con readback -> publicar copia RAM -> guardar evidencia. Exclusión y tiempos deben medirse en el target. Un corte entre commit durable y publicación RAM exige rejoin/reconciliación explícita. Este candidato no implementa ese commit ni afirma tolerancia real a cortes. Los conflictos que gc_submit latcha requieren política de persistencia/evidencia propia, no simplemente descartar su retorno no-OK.

## Implementación y validación

Tres candidatos: guardian_corroboration_ack.h, guardian_corroboration_ack.c y m17_corroboration_ack_host_test.c. No se modifica gc_submit ni NodeLink existente. El código tiene memoria fija, no heap/recursión y serializa en un buffer local antes de escribir salida, admitiendo overlap documentado. La comparación del commitment no contiene salida temprana por byte; no se afirma constant-time del binario ni se trata el commitment como secreto.

Linux GCC O0/O2: 3235 assertions por ejecución; cero warnings con -Werror y las flags registradas. Ejecución adicional con UndefinedBehaviorSanitizer: PASS. No ASan, target, firma de producción ni corte físico en esta evidencia. Cuatro mutantes seleccionados eliminando bindings de epoch/secuencia/commitment y reservados fueron detectados por assertions. No es una campaña completa de mutación.

El runner 06G genera un fixture nuevo en Downloads, conserva estado exacto del repositorio, compila y ejecuta solo esta receta en Windows O0/O2 con Python/Clang/LLD previamente observados, captura logs separados e identidades y verifica el ZIP byte a byte. Ningún candidato se incorpora al worktree. La ejecución Windows sigue pendiente hasta recibir y adjudicar sus logs. No se ha ejecutado el parser PowerShell 5.1 aquí; el launcher lo verifica en Windows antes de ejecutar.

## Revisión crítica

Human Readability: ACK de recepción no equivale a autorización; nombres epoch/sequence especifican observation. Devil's Advocate: una firma válida puede atestar una mentira y dos nodos pueden compartir fallos; este componente no resuelve esas amenazas. Technical Destruction: golden bytes, longitudes, null, capacidad, campos cero, cada bit de referencia, discriminadores y mutantes seleccionados. Limitaciones explícitas: sender binding, key provenance, freshness, pending consumption, durable transaction, timeout, races y cryptographic commitment aún no están implementados por este codec.

## Mapa de continuidad sin promoción

1. Este candidato resuelve la representación y comparación exacta de ACK en host.
2. Sigue la extensión versionada del envelope/auth transcript y el ingreso autenticado con anti-replay, pending ACK y corroboración transaccional.
3. F401: build exacta, flash/RAM/stack/WCET, watchdog, provisioning/entropía, adquisición/calibración, arranque y fallos físicos.
4. MCXN947: board/SDK/target, build/boot y enlace físico ST–NXP.
5. Chips adicionales: ADR de funciones, selección y evidencia propia antes de contar nodos operativos.
6. Retención/evidencia encadenada, actualización/recovery, suministro, CI exacta y revisión de claims.

No se cierran G01/G13/G14/G16 ni ninguno de los otros gates. Las obligaciones UE y los estándares de aseguramiento mantienen su mapeo pendiente; este ADR no crea ni interpreta nuevas obligaciones legales y no reclama certificación. La norma aeronáutica concreta no está establecida por estos dos documentos; NASA no es un sustituto automático de esa selección.
