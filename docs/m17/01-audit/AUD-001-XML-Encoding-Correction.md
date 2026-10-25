# AUD-001 — Corrección de declaración XML y doble revisión

Estado: FIX_VERIFIED_XML_BYTES_KEIL_QUALIFICATION_PENDING.
Base: b7e3c95425603f8d5e4599fd745c6d3acd672c2d. Rama: fix/m17-keil-xml-encoding.

Archivo: `firmware/MDK-ARM/Guardian-F401.uvprojx`. El cambio autorizado es exclusivamente `encoding="utf-16"` a `encoding="utf-8"` en la primera declaración. Se preservan los bytes restantes, los 628 CRLF, las 35 entradas FileName y la configuración XML. No se cambia startup, linker, optimización o pertenencia de fuentes.

Preimagen SHA-256: `3590482DA2959CF2D633F34D523A4CB1426F9F7B67344E6B8073302339ADB0BE`.
Postimagen SHA-256: `BD18F9B5B12F7CF5B3D3A47BE4B79E5F3224A1E9BE4BCC97F98B2CAC2AB57247`.
ZIP de ejecución: `Guardian-M17-08-XML-Correct-32b888016d2b466db16ddff9296b5c46.zip`.
ZIP SHA-256: `63F64B3052449EC9B92B2320B87399A733595DBA2446A9CF0B8B59DE071359A7`.

La primera revisión verifica hashes, tamaños, UTF-8, parseo desde bytes y árbol XML equivalente. La segunda aplica un contrato de prefijo/sufijo que preserva todos los bytes fuera de la declaración y rechaza ocho variantes alteradas. Son dos métodos en el mismo trabajo; no se afirma IV&V o dos revisores independientes.

08C verificó mecánicamente el archivo de ejecución y la coincidencia con el estado local. Los otros 601 archivos conservan sus identidades, así como HEAD, refs e índice durante la corrección. La auditoría global previa conserva su baseline histórico; este documento es una disposición posterior.

No se ejecutó Keil ni firmware. La interoperabilidad con la herramienta Keil, los gates de producción y los otros 19 hallazgos siguen pendientes. Esta evidencia no publica v0.17, no cierra deudas físicas ni establece conformidad normativa.

Estos documentos son candidatos externos. Su admisión, staging, commit firmado y push requieren sus fronteras respectivas. No repetir la corrección XML.
