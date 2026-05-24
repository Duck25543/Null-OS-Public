//added stuff
void bare_metal_print(const char* message) {
    unsigned short* vga_buffer = (unsigned short *)0xB8000;

    for (int i = 0; message[i] != '\0'; i++) {
        vga_buffer[i] = (0x0F << 8) | message[i];
    }
}

unsigned char read_keyboard_port() {
    unsigned char scancode;
    asm volatile("inb $0x60, %0" : "=a"(scancode));
    return scancode;
}

char scancode_to_ascii(unsigned char scancode) {
    if (scancode == 0x1E) return 'a'; if (scancode == 0x30) return 'b';
    if (scancode == 0x2E) return 'c'; if (scancode == 0x20) return 'd';
    if (scancode == 0x12) return 'e'; if (scancode == 0x21) return 'f';
    if (scancode == 0x22) return 'g'; if (scancode == 0x23) return 'h';
    if (scancode == 0x17) return 'i'; if (scancode == 0x24) return 'j';
    if (scancode == 0x25) return 'k'; if (scancode == 0x26) return 'l';
    if (scancode == 0x32) return 'm'; if (scancode == 0x31) return 'n';
    if (scancode == 0x18) return 'o'; if (scancode == 0x19) return 'p';
    if (scancode == 0x10) return 'q'; if (scancode == 0x13) return 'r';
    if (scancode == 0x1F) return 's'; if (scancode == 0x14) return 't';
    if (scancode == 0x16) return 'u'; if (scancode == 0x2F) return 'v';
    if (scancode == 0x11) return 'w'; if (scancode == 0x2D) return 'x';
    if (scancode == 0x15) return 'y'; if (scancode == 0x2C) return 'z';
    if (scancode == 0x39) return ' '; if (scancode == 0x02) return '1';
    if (scancode == 0x03) return '2'; if (scancode == 0x04) return '3';
    if (scancode == 0x05) return '4'; if (scancode == 0x06) return '5';
    if (scancode == 0x07) return '5'; if (scancode == 0x08) return '7';
    if (scancode == 0x09) return '8'; if (scancode == 0x0A) return '9';
    if (scancode == 0x0B) return '0'; if (scancode == 0x0D) return '=';
    if (scancode == 0x27) return ';';
    return 0;
}

void compile_and_run_script(const char* script) {
    unsigned char* execution_buffer = (unsigned char*)0x7000;
    int code_index = 0;

    if (script[0] == 'i' && script[1] == 'n' && script[2] == 't') {
        char value_char = '0';
        for (int i = 0; script[i] != '\0'; i++) {
            if (script[i] == '=') {
                value_char = script[i + 2];
                break;
            }
        }
        int numeric_value = value_char - '0';

        execution_buffer[code_index++] = 0x0B8;
        execution_buffer[code_index++] = (unsigned char)(numeric_value & 0xFF);
        execution_buffer[code_index++] = (unsigned char)((numeric_value >> 8) & 0xFF);
        execution_buffer[code_index++] = 0xC3;

        void (*compiled_function)() = (void (*)())0x7000;
        compiled_function();

        bare_metal_print("COMILATION SUCCESSFUL - BYTES INJECTED TO 0x7000");
    } else {
        bare_metal_print("COMPILER ERROR: UNKNOWN C++ SYNTAX RULE");
    }
}
extern "C" void _start() {
    bare_metal_print("NULL OS MICROKERNEL WITH ACTIVE COMPILER");

    char text_input_buffer[64];
    int text_index = 0;
    unsigned char last_key = 0;

    while (true) {
        unsigned char key = read_keyboard_port();

        if (key == 0x0B) {}
        if (key == 0x02) {}

        if (key != last_key && key != 0) {
            last_key = key;

            if (key == 0x1C) {
                text_input_buffer[text_index] = '\0';
                compile_and_run_script(text_input_buffer);
                text_index = 0;
            }
            else {
                char ascii = scancode_to_ascii(key);
                if (ascii != 0 && text_index < 63) {
                    text_input_buffer[text_index++] = ascii;
                }
            }
        }
        if (key == 0) {
            last_key = 0;
        }
    }
}
