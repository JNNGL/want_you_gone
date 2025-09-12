bits 16

section .boot

bpb:
    jmp stage1_entry
    times 3-($-$$) db 0
    .bpb_oem_name:          db 'NONE    '
    .bpb_sector_size:       dw 512
    .bpb_sects_per_cluster: db 0
    .bpb_reserved_sects:    dw 0
    .bpb_fat_count:         db 0
    .bpb_root_dir_entries:  dw 0
    .bpb_sector_count:      dw 0
    .bpb_media_type:        db 0
    .bpb_sects_per_fat:     dw 0
    .bpb_sects_per_track:   dw 18
    .bpb_heads_count:       dw 2
    .bpb_hidden_sects:      dd 0
    .bpb_sector_count_big:  dd 0
    .bpb_drive_num:         db 0
    .bpb_reserved:          db 0
    .bpb_signature:         db 0
    .bpb_volume_id:         dd 0
    .bpb_volume_label:      db 'NONE       '
    .bpb_filesystem_type:   times 8 db 0

%include "build/constants.asm"

stage1_entry:
    jmp 0x00:setup
setup:
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov sp, 0x7C00
    cld

    mov [bios_disk], dl

    mov eax, 1
    mov bx, stage2
    mov cx, (bootloader_size - 1) / 512
    xor dx, dx
    call read_disk16

    jmp stage2_entry

print16:
    push ax
    push si
.string_loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .string_loop
.done:
    pop si
    pop ax
    ret

println16:
    push si
    call print16
    mov si, newline
    call print16
    pop si
    ret
newline:    db 13, 10, 0

read_disk16:
    cmp cx, 127
    jbe .read_whole
    pusha
    mov cx, 127
    call read_disk16
    popa
    add eax, 127
    add dx, 4064
    sub cx, 127
.read_whole:
    mov [dap_lba_lower], eax
    mov [dap_num_sectors], cx
    mov [dap_buf_segment], dx
    mov [dap_buf_offset], bx
    mov dl, [bios_disk]
    mov si, dap
    mov ah, 0x42
    int 0x13
    jc .error
    ret
.error:
    mov si, disk_read_error
    call println16
.loop:
    hlt
    jmp .loop
disk_read_error:    db 'Error reading disk.', 0

global bios_disk
bios_disk:   db 0

global dap
global dap_num_sectors
global dap_buf_offset
global dap_buf_segment
global dap_lba_lower
global dap_lba_upper

dap:                db 0x10
                    db 0
dap_num_sectors:    dw 0
dap_buf_offset:     dw 0
dap_buf_segment:    dw 0
dap_lba_lower:      dd 0
dap_lba_upper:      dd 0

times 510-($-$$) db 0
dw 0xAA55

extern boot_main

stage2:
stage2_entry:
    call enable_a20
    jc a20_fail

    cli
    push ds

    lgdt [gdtinfo]

    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pmode
pmode:
    mov bx, 0x10
    mov ds, bx

    and al, 0xFE
    mov cr0, eax
    jmp 0x00:unreal
unreal:
    pop ds
    sti

    call boot_main

a20_fail:
    mov si, a20_fail_message
    call println16
    jmp .loop
.loop:
    hlt
    jmp .loop

gdtinfo:
    dw gdt_end - gdt - 1
    dd gdt
gdt:        dd 0, 0
codedesc:   db 0xFF, 0xFF, 0, 0, 0, 10011010b, 00000000b, 0
flatdesc:   db 0xFF, 0xFF, 0, 0, 0, 10010010b, 11001111b, 0
gdt_end:

a20_fail_message:   db 'Failed to enable the A20 line.', 0

get_a20_state:
    pushf
    push ds
    push es
    push di
    push si
    cli

    xor ax, ax
    mov es,ax

    not ax
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax
    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al
    pop ax
    mov byte [es:di], al

    mov ax, 0
    je .exit
    mov ax, 1
.exit:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

query_a20_support:
    push bx
    clc

    mov ax, 0x2403
    int 0x15
    jc .error

    test ah, ah
    jnz .error

    mov ax, bx
    pop bx
    ret
.error:
    stc
    pop bx
    ret

enable_a20_keyboard_controller:
    cli

    call .wait_io1
    mov al, 0xAD
    out 0x64, al

    call .wait_io1
    mov al, 0xD0
    out 0x64, al

    call .wait_io2
    in al, 0x60
    push eax

    call .wait_io1
    mov al, 0xD1
    out 0x64, al

    call .wait_io1
    pop eax
    or al, 2
    out 0x60, al

    call .wait_io1
    mov al, 0xAE
    out 0x64, al

    sti
    ret
.wait_io1:
    in al, 0x64
    test al, 2
    jnz .wait_io1
    ret
.wait_io2:
    in al, 0x64
    test al, 1
    jz .wait_io2
    ret

enable_a20:
    clc
    pusha
    mov bh, 0

    call get_a20_state
    jc .fast_gate

    test ax, ax
    jnz .done

    call query_a20_support
    mov bl, al
    test bl, 1
    jnz .keyboard_controller

    test bl, 2
    jnz .fast_gate
.bios_int:
    mov ax, 0x2401
    int 0x15
    jc .fast_gate
    test ah, ah
    jnz .failed
    call get_a20_state
    test ax, ax
    jnz .done
.fast_gate:
    in al, 0x92
    test al, 2
    jnz .done

    or al, 2
    and al, 0xFE
    out 0x92, al

    call get_a20_state
    test ax, ax
    jnz .done

    test bh, bh
    jnz .failed
.keyboard_controller:
    call enable_a20_keyboard_controller
    call get_a20_state
    test ax, ax
    jnz .done

    mov bh, 1
    test bl, 2
    jnz .fast_gate
    jmp .failed
.failed:
    stc
.done:
    popa
    ret

gdtr:   dw 0
        dd 0

extern pmain
global protected_mode
protected_mode:
    mov [gdtr], ax
    mov [gdtr + 2], edx

    cli
    lgdt [gdtr]
    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp 0x08:.pmode
bits 32
.pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    call pmain

align 512

global disk_buffer
disk_buffer:
    times 8192 db 0