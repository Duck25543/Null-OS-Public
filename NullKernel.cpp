//updated the compiler and now it's a kernel, cool
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

extern "C" void _start() {
    bare_metal_print("NULL OS - RUNNING NAITIVE 16-BIT C++");

    while (true) {
        unsigned char key = read_keyboard_port();

        if (key == 0x0B) {

        }
        if (key == 0x02) {

        }
    }
}

