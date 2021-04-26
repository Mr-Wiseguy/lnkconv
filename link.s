.section .text, "ax"

.incbin "out_text.bin"

.include "functions.s"

.section .data

.incbin "out_data.bin"

.section .rodata

.incbin "out_rodata.bin"

.section .bss

.include "bss_length.s"
