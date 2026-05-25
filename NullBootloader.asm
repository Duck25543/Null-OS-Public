[bits 16]    
[org 0x7c00]    

KERNEL_OFFSET equ 0x1000 ; The memory location where it will load the kernel

boot_init:
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    
    ; Save the boot drive number provided by the BIOS in DL
    mov [BOOT_DRIVE], dl

    mov si, banner_text
    call print_string

prompt_init:
    mov si, prompt_symbol
    call print_string
    xor cx, cx
    xor edx, edx

input_loop:
    mov ah, 0x00
    int 0x16
    cmp al, 0x0D
    je execute_line
    cmp al, '0'
    jl input_loop
    cmp al, '1'
    jg input_loop

    cmp cx, 32
    jl continue_print
    jge input_loop

continue_print:
    mov ah, 0x0E
    int 0x10
    inc cx
    push ax
    sub al, '0'
    shl edx, 1
    or dl, al
    mov ax, cx
    mov bl, 8
    div bl
    mov bh, ah
    pop ax
    
    cmp bh, 0
    je print_space      ; Jump explicitly to print space if remainder is 0
    jmp check_done      ; Otherwise, skip printing a space safely

print_space:
    mov al, ' '
    mov ah, 0x0E
    int 0x10

check_done:
    jmp input_loop

execute_line:
    mov ah, 0x0E
    mov al, 0x0A
    int 0x10
    mov al, 0x0D
    int 0x10

    ; --- Your Evaluator Checks ---
    cmp edx, 15            ; Command '1111' triggers clear
    je trigger_clear

    cmp edx, 255           ; NEW COMMAND: '11111111' triggers Null OS boot
    je trigger_kernel_boot

    jmp prompt_init

trigger_clear:
    mov ax, 0x0003
    int 0x10
    jmp prompt_init

trigger_kernel_boot:
    mov si, load_msg
    call print_string
    
    ; 1. Load the kernel from disk to RAM
    xor ax, ax             ; Safe segment normalization
    mov es, ax
    mov bx, KERNEL_OFFSET  ; Destination pointer
    mov dh, 15             
    mov dl, [BOOT_DRIVE]   ; Use saved boot disk index
    call disk_load

    ; 2. Disable interrupts before switching CPU modes
    cli                    
    lgdt [gdt_descriptor]  ; Load Global Descriptor Table

    ; 3. Flip bit 0 of CR0 to enter Protected Mode
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 4. Far jump into 32-bit code space to flush CPU pipeline
    jmp CODE_SEG:init_pm

; --- DISK READING UTILITY ---
disk_load:
    push dx
    mov ah, 0x02           ; BIOS Read sectors function
    mov al, dh             ; Number of sectors to read
    mov ch, 0x00           ; Cylinder 0
    mov dh, 0x00           ; Head 0
    mov cl, 0x02           ; Sector 2 (Sector 1 is this bootloader)
    int 0x13
    jc disk_error          ; Jump if error carry flag set
    pop dx
    ret

disk_error:
    mov si, disk_err_msg
    call print_string
    jmp $

print_string:
    mov ah, 0x0E
.string_loop:
    lodsb
    cmp al, 0
    je .string_done
    int 0x10
    jmp .string_loop
.string_done:
    ret

; ==========================================================
; GDT (GLOBAL DESCRIPTOR TABLE) FOR 32-BIT PROTECTED MODE
; ==========================================================
gdt_start:
gdt_null: 
    dd 0x0, 0x0
gdt_code: 
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data: 
    dw 0xffff, 0x0
    db 0x0, 10010012b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ==========================================================
; 32-BIT PROTECTED MODE HANDOFF ENTRY
; ==========================================================
[bits 32]
init_pm:
    mov ax, DATA_SEG       ; Point all segment data selectors to GDT Data
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000       ; Position stack pointer safely away from kernel code
    jmp KERNEL_OFFSET      ; Jump directly to the first byte of kernel 

; ==========================================================
; DATA SEGMENT
; ==========================================================
BOOT_DRIVE:    db 0
banner_text:   db "====================================", 0x0D, 0x0A, \
                  "             NULL OS                ", 0x0D, 0x0A, \
                  "    ENTER 11111111 TO LAUNCH        ", 0x0D, 0x0A, \
                  "====================================", 0x0D, 0x0A, 0
prompt_symbol: db "$ ", 0
load_msg:      db "LOADING KERNEL TRACKERS...", 0x0D, 0x0A, 0
disk_err_msg:  db "DISK SECTOR CRITICAL SEVERE READ ERROR", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55
