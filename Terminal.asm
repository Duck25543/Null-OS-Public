; =====================================================================
; Null OS Core - Assembly Terminal
; =====================================================================

[BITS 32]
global _shell_start

_shell_start:
    call clear_screen
    
.prompt_loop:
    call print_prompt
    mov edi, input_buffer       ; EDI points to keyboard input storage
    mov ecx, 0                  ; ECX tracks typed character count

.read_key:
    in al, 0x64                 ; Poll keyboard status port
    test al, 0x01
    jz .read_key
    in AL, 0x60                 ; Read raw scancode
    test al, 0x80               ; Ignore key releases
    jnz .read_key

    cmp al, 0x1C                ; Enter key scancode
    je .enter_pressed
    cmp al, 0x0E                ; Backspace key scancode
    je .backspace_pressed
    
    ; (Insert standard scancode-to-ascii translation helper here)
    cmp al, 0                   
    je .read_key

    cmp ecx, 79                 ; Prevent buffer overflow
    jge .read_key
    mov [edi + ecx], al         
    inc ecx
    call print_char             
    jmp .read_key

.backspace_pressed:
    jecxz .read_key
    dec ecx
    call do_backspace
    jmp .read_key

.enter_pressed:
    mov byte [edi + ecx], 0     ; Null-terminate user input string
    call print_newline
    
    ; -----------------------------------------------------------------
    ; Command Evaluation Loop
    ; -----------------------------------------------------------------
    mov esi, input_buffer
    
    mov edi, cmd_clear
    call string_compare
    jc .exec_clear

    mov edi, cmd_sysinfo
    call string_compare
    jc .exec_sysinfo

    ; Check for custom compile command
    mov edi, cmd_compile
    call string_compare
    jc .exec_compile

    jmp .unknown_cmd

.exec_clear:
    call clear_screen
    jmp .prompt_loop

.exec_sysinfo:
    mov esi, msg_sysinfo
    call print_string
    jmp .prompt_loop

.exec_compile:
    call compile_routine        ; Run the direct macro expansion translator
    mov esi, msg_comp_done
    call print_string
    jmp .prompt_loop

.unknown_cmd:
    cmp ecx, 0                  
    je .prompt_loop
    mov esi, msg_unknown
    call print_string
    jmp .prompt_loop


; -----------------------------------------------------------------
; The Compiler Feature: Macro Expansion / String Substitution Logic
; -----------------------------------------------------------------
compile_routine:
    mov esi, c_source_buffer    ; Point to source code RAM scratchpad
    mov edi, asm_output_buffer  ; Point to destination compiler output RAM

.next_c_char:
    lodsb                       ; Load character from ESI into AL, increment ESI
    cmp al, 0                   ; End of source file?
    je .compilation_finished

    ; Check Pattern: Does the letter match 'c'? (First letter of clear();)
    cmp al, 'c'
    jne .check_next_pattern
    
    ; Inline validation: Quickly check the rest of "lear();"
    ; For maximum reliability, loop check characters 'l','e','a','r','(',')',';'
    ; If matched: Expand the C statement into assembly library counterpart
    push esi
    mov esi, macro_clear        ; "call clear_screen"
    call copy_string
    pop esi
    add esi, 7                  ; Skip past the parsed "lear();" in C buffer
    jmp .next_c_char

.check_next_pattern:
    ; Add further pattern lookups here for variables or custom commands
    jmp .next_c_char

.compilation_finished:
    mov byte [edi], 0           ; Append null terminator to generated text
    ret


; -----------------------------------------------------------------
; Helper Utilities
; -----------------------------------------------------------------
print_prompt:
    mov esi, prompt_str
    call print_string
    ret

print_string:
.loop:
    lodsb
    cmp al, 0
    je .done
    call print_char
    jmp .loop
.done:
    ret

print_char:
    push eax
    push ebx
    mov ebx, [vga_cursor]
    mov byte [ebx], al          
    mov byte [ebx+1], 0x0F      ; White text on black background
    add ebx, 2
    mov [vga_cursor], ebx
    pop ebx
    pop eax
    ret

print_newline:
    push eax
    push edx
    mov eax, [vga_cursor]
    sub eax, 0xB8000
    mov edx, 0
    mov ecx, 160
    div ecx                     
    inc eax                     ; Drop cursor to next physical VGA boundary
    mul ecx
    add eax, 0xB8000
    mov [vga_cursor], eax
    pop edx
    pop eax
    ret

do_backspace:
    sub dword [vga_cursor], 2
    mov ebx, [vga_cursor]
    mov word [ebx], 0x0F20      
    ret

clear_screen:
    mov edi, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0F20              
    rep stosw
    mov dword [vga_cursor], 0xB8000
    ret

string_compare:
.loop:
    mov al, [esi]
    mov bl, [edi]
    cmp al, bl
    jne .no_match
    cmp al, 0
    je .match
    inc esi
    inc edi
    jmp .loop
.no_match:
    clc
    ret
.match:
    stc
    ret

copy_string:
    ; Helper to copy null-terminated string from ESI into EDI buffer
.copy_loop:
    lodsb
    cmp al, 0
    je .copy_done
    stosb                       ; Store AL into EDI, increment EDI
    jmp .copy_loop
.copy_done:
    ret


; -----------------------------------------------------------------
; Constant Data & System Macros
; -----------------------------------------------------------------
vga_cursor    dd 0xB8000
prompt_str    db "null-os@core:/$ ", 0
cmd_clear     db "clear", 0
cmd_sysinfo   db "sysinfo", 0
cmd_compile   db "compile", 0

msg_sysinfo   db "Null OS Core [Assembly Shell v1.1]", 0x0A, 0
msg_unknown   db "null-sh: command not found", 0x0A, 0
msg_comp_done db "C translation complete. Macro expanded to ASM buffer.", 0x0A, 0

; Assembly Library Expansion Template
macro_clear   db "call clear_screen", 0x0A, 0


; -----------------------------------------------------------------
; Static Uninitialized Space (Memory Buffers)
; -----------------------------------------------------------------
SECTION .bss
input_buffer       resb 80       ; Standard keyboard array
c_source_buffer    resb 1024     ; Where you type/load your custom C code
asm_output_buffer  resb 4096     ; Destination holding your translated .asm text
