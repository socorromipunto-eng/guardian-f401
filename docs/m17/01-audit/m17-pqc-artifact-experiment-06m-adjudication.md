# Guardian M17 — Adjudicación de evidencia pública 06M

Estado: PASS_EVIDENCE_CONSISTENCY_HOST_SCOPE. Fecha: 2026-10-12.

## Resultado y frontera

El paquete conserva evidencia de un experimento ejecutado de firma y verificación ML-DSA-87 sobre un manifiesto de investigación. Sus resultados son coherentes con los bytes, comandos y controles registrados. Esta revisión no vuelve a ejecutar la verificación criptográfica y no demuestra validación por una implementación independiente.

## Identidades y comprobaciones

- zip_sha256: 1C915EDF7F8CC77EDEEB9CAA9078B08580EB4CD366B496C1BABA4AA1F0C1A504
- entry_count: 35
- source_paths: 575
- commands: 11
- positive_controls: 2
- negative_controls: 4
- signature_bytes: 4627
- public_der_bytes: 2614
- public_key_sha256: 77C74C7D8919A758076B54F639EB9BC6D3696CD6F2C1B99904578A873E2A2337
- manifest_sha256: 4E5D16E48EB6F61654AE2ADFF3F11346F5F57ABE7B61D35061AB049761A88C42
- signature_sha256: 185437AD47BB88CCE207065831BDAB0A9DF2552657EE21D07101331A7E9F7C0B
- artifact_sha256: 3CDBFBE32D66F254C62D00354D57E91F7BDEBFA876BD6BA7585FCD6A4B73410B
- review: PASS_EVIDENCE_CONSISTENCY_HOST_SCOPE
- cryptographic_reexecution_here: False
- independent_implementation_validation: False
- private_key_material_observed: False
- private_acl_independently_observed: False

Se verificaron los 35 nombres sin duplicados o escape de rutas, CRC del ZIP, once pares de logs brutos contra command-results.json, orden de comandos y estados before/after idénticos al baseline 06L de 575 rutas. Ambas verificaciones positivas registran exit 0 y Signature Verified Successfully; cuatro negativos registran exit 1, Signature Verification Failure y diagnóstico específico ML-DSA-87 del proveedor. Las operaciones de generación/exportación/firma registran exit 0 y stdout/stderr vacíos.

La firma tiene 4627 bytes. Ambas claves DER tienen estructura SubjectPublicKeyInfo con OID 2.16.840.1.101.3.4.3.19, sin parámetros adicionales y 2592 bytes de clave pública. Esto verifica la representación, no una aprobación criptográfica o confianza de producción. Las claves públicas son distintas.

## Bytes firmados y artefacto

Manifiesto de 14 campos ordenados UTF-8 sin BOM, LF y newline final. El hash del manifiesto, firma, clave pública y política coincide con sus registros. El artefacto es una copia de guardian_corroboration_ack.c de 2723 bytes, SHA-256 3CDBFBE32D66F254C62D00354D57E91F7BDEBFA876BD6BA7585FCD6A4B73410B. Coincide con el archivo incorporado en 06H y se identifica como snapshot de worktree sin commit; source_HEAD es contexto de base, no prueba de que ese código esté en ese commit.

Cada variante alterada cambia únicamente el bit inferior del primer byte de su original, preservando longitud y resto de bytes. El artefacto alterado falla la comparación de hash; esto es separado de la verificación de firma del manifiesto. El contexto de firma es GUARDIAN-M17-PQC-ARTIFACT-RESEARCH-V1. Se revisaron los argumentos que vinculan cada negativo a manifiesto, firma, clave o contexto alternativos.

## Privacidad y límites

No hay entradas .pem, carpeta privada o marcadores PEM de clave privada en el ZIP. La estructura de las dos claves DER es pública. La ausencia de material privado en este paquete no constituye inspección independiente de la ACL o carpeta privada del equipo Windows; el runner comprobó esa ACL y la carpeta no fue compartida.

El experimento usa el mismo proveedor para firmar y verificar; no es KAT, interoperabilidad entre implementaciones, validación FIPS/CMVP ni conformidad CNSA. La revisión local de bytes/logs no es repetición criptográfica independiente. No hay imagen de firmware, instalación autenticada en F401/NXP, confianza provisionada, anti-rollback de actualización ni cualificación física demostrados por este experimento.

## Revisiones y decisión

Human Readability: capacidad demostrada limitada a experimento host documentado. Devil's Advocate: firma y verificación del mismo proveedor pueden compartir defectos; incluir una clave pública en el paquete no la convierte en ancla de confianza. Technical Destruction: cuatro negativos seleccionados no sustituyen corpus completo, parser hostil, downgrade, truncamiento, revocación o pruebas de política.

Se acepta registrar el resultado en documentación M17, manteniendo abiertos los registros de requisitos candidatos y los gates 01/13/14/16. No se modifican firmware, claves de producción, release o tag. La ejecución Windows de 06J sigue pendiente de evidencia. La firma postcuántica no reemplaza las deudas físicas del F401, implementación target MCXN947 o integración NodeLink.

## Próximas dependencias

Ingreso documental exacto; firma/verificación con vectores o implementación independiente; política de ancla de confianza y ciclo de claves; selección y medición target de verificador; actualización segura, rollback y recuperación; integración en CI y revisión de afirmaciones. La subida de documentos a GitHub no ha ocurrido: requiere stages/commit/push de alcance revisado, sin incluir automáticamente los demás cambios M17 del worktree.
