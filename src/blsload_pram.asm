BLSLOAD_BUFOFF  ds      2       ; Offset in BLSLOAD_DATA buffer
BLSLOAD_SECTOR  ds      4       ; Sector requested by main CPU
BLSLOAD_COUNT   ds      4       ; Number of sectors requested by main CPU
BLSLOAD_HEADER  ds      4       ; Contains header of read data
BLSLOAD_DATA    ds      $800    ; One sector buffer
        align   2

; vim: ts=8 sw=8 sts=8 et
