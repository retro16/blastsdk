BLSLOAD_BUFOFF  ds      2       ; Offset of data in last sector
BLSLOAD_HEADER  ds      4       ; Contains header of read data
BLSLOAD_ADDR    ds      4       ; Contains destination address
BLSLOAD_SIZE    ds      4       ; Contains destination size
BLSLOAD_SECTOR  ds      4       ; Contains sector number
BLSLOAD_COUNT   ds      4       ; Contains sector count
BLSLOAD_DEST    ds      1
        align   2
