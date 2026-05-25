//fixed keyboard issue, now is mostly complete, if i didn't want to keep working on this
// --- 1. DEFINITIONS ---
#define COM1 0x3F8
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

// --- 2. HARDWARE DRIVERS (VGA, SERIAL, KEYBOARD) ---

void bare_metal_print(const char* message) {
    unsigned short* vga_buffer = (unsigned short *)0xB8000;
    static int line_offset = 0;
    for (int i = 0; message[i] != '\0'; i++) {
        vga_buffer[line_offset + i] = (0x0F << 8) | message[i];
    }
    line_offset += 80;
}

void init_serial() {
    asm volatile("outb $0x00, %0" : : "dN"(COM1 + 1));
    asm volatile("outb $0x80, %0" : : "dN"(COM1 + 3));
    asm volatile("outb $0x03, %0" : : "dN"(COM1 + 0));
    asm volatile("outb $0x00, %0" : : "dN"(COM1 + 1));
    asm volatile("outb $0x03, %0" : : "dN"(COM1 + 3));
}

unsigned char read_serial() {
    unsigned char status;
    do { asm volatile("inb %1, %0" : "=a"(status) : "dN"(COM1 + 5)); } while (!(status & 1));
    unsigned char data;
    asm volatile("inb %1, %0" : "=a"(data) : "dN"(COM1));
    return data;
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
    if (scancode == 0x07) return '6'; if (scancode == 0x08) return '7';
    if (scancode == 0x09) return '8'; if (scancode == 0x0A) return '9';
    if (scancode == 0x0B) return '0'; if (scancode == 0x0D) return '=';
    if (scancode == 0x4E) return '+';
    return 0;
}

// --- 3. THE COMPILER & INSTALLER LOGIC ---

void compile_and_run_script(const char* script) {
    unsigned char* exec_buf = (unsigned char*)0x7000;
    if (script[0] == 'i' && script[1] == 'n' && script[2] == 't') {
        int v1 = 0, v2 = 0; bool add = false;
        for (int i = 0; script[i]; i++) {
            if (script[i] == '=') v1 = script[i+1] - '0';
            if (script[i] == '+') { add = true; v2 = script[i+1] - '0'; }
        }
        int res = add ? (v1 + v2) : v1;
        exec_buf[0] = 0xB8; // MOV EAX, imm32
        exec_buf[1] = res & 0xFF; exec_buf[2] = (res >> 8) & 0xFF;
        exec_buf[3] = (res >> 16) & 0xFF; exec_buf[4] = (res >> 24) & 0xFF;
        exec_buf[5] = 0xC3; // RET
        int (*f)() = (int (*)())0x7000;
        int out = f();
        bare_metal_print("JIT SUCCESS. RESULT: ");
        ((unsigned short*)0xB8000)[150] = (0x0A << 8) | (out + '0');
    }
}

void slip_install_package() {
    bare_metal_print("SLIP READY... SEND DATA.");
    unsigned char* pkg = (unsigned char*)0x8000;
    int i = 0;
    while (true) {
        unsigned char c = read_serial();
        if (c == SLIP_END) { if (i > 0) break; else continue; }
        if (c == SLIP_ESC) {
            c = read_serial();
            if (c == SLIP_ESC_END) pkg[i++] = SLIP_END;
            else if (c == SLIP_ESC_ESC) pkg[i++] = SLIP_ESC;
        } else pkg[i++] = c;
    }
    bare_metal_print("PKG LOADED. RUNNING...");
    ((void (*)())0x8000)();
}

// --- 4. MAIN KERNEL ENTRY ---

extern "C" void _start() {
    init_serial();
    bare_metal_print("NULL OS 0.2: JIT + SLIP ACTIVE");

    char buf[64]; int idx = 0; unsigned char last = 0;

    while (true) {
        unsigned char k = read_keyboard_port();

        if (k & 0x80) {
            last = 0;
        }
        else if (k != last && k != 0) {
            last = k;

            if (k == 0x1C) { // ENTER KEY
                buf[idx] = '\0';
                if (buf[0]=='s' && buf[1]=='l' && buf[2]=='i' && buf[3]=='p') {
                    slip_install_package();
                } else {
                    compile_and_run_script(buf);
                }
                idx = 0;
            } else {
                char a = scancode_to_ascii(k);
                if (a && idx < 63) {
                    buf[idx++] = a;
                }
            }
        }

        if (k == 0) last = 0;
    }
}

