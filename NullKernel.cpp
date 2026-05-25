// Null OS Kernel
// I really do wonder how many changes I'm going to make to this repository
asm(".code16gcc");
// --- 1. DEFINITIONS & CONFIGURATION ---
#define COM1 0x3F8
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

// Hardware Disk Ports (Primary ATA Bus)
#define STATUS_REG 0x1F7
#define COMMAND_REG 0x1F7
#define DATA_REG 0x1F0

// Visual Theme Palette: White text on a classic blue background
#define COLOR_DEFAULT 0x1F

// --- 2. MEMORY MANAGEMENT (THE RAM PEN) ---
static unsigned char* heap_pointer = (unsigned char*)0x10000;

void* null_malloc(unsigned int size) {
    void* allocated_ptr = (void*)heap_pointer;
    heap_pointer += size; 
    return allocated_ptr;
}

// --- 3. PERSISTENT PACKAGE REGISTRY ---
struct InstalledApp {
    char name[12];         
    void (*entry_point)(); 
    unsigned int size;   
    unsigned int disk_lba; 
};

static InstalledApp registry[10];
static int app_count = 0;

// --- 4. HARDWARE DISK CONTROLLER (STORAGE DRIVER) ---

// Wait for the storage drive to finish processing and become ready
void ata_wait_ready() {
    unsigned char status;
    do {
        asm volatile("inb %1, %0" : "=a"(status) : "dN"((unsigned short)STATUS_REG));
    } while ((status & 0x80) || !(status & 0x08)); // Loop while BUSY or until NOT READY
}

// Write 512 bytes of memory to a specific physical sector on the drive
void write_disk_sector(unsigned int LBA, const unsigned char* buffer) {
    asm volatile("outb %0, %1" : : "a"((unsigned char)1), "dN"((unsigned short)0x1F2)); // Sector count
    asm volatile("outb %0, %1" : : "a"((unsigned char)(LBA & 0xFF)), "dN"((unsigned short)0x1F3));
    asm volatile("outb %0, %1" : : "a"((unsigned char)((LBA >> 8) & 0xFF)), "dN"((unsigned short)0x1F4));
    asm volatile("outb %0, %1" : : "a"((unsigned char)((LBA >> 16) & 0xFF)), "dN"((unsigned short)0x1F5));
    asm volatile("outb %0, %1" : : "a"((unsigned char)(((LBA >> 24) & 0x0F) | 0xE0)), "dN"((unsigned short)0x1F6));
    asm volatile("outb %0, %1" : : "a"((unsigned char)0x30), "dN"((unsigned short)COMMAND_REG)); // Command 0x30 = Write

    ata_wait_ready();

    for (int i = 0; i < 256; i++) {
        unsigned short word = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        asm volatile("outw %0, %1" : : "a"(word), "dN"((unsigned short)DATA_REG));
    }
}

// Read 512 bytes from a physical disk sector back into a system memory buffer
void read_disk_sector(unsigned int LBA, unsigned char* buffer) {
    asm volatile("outb %0, %1" : : "a"((unsigned char)1), "dN"((unsigned short)0x1F2)); // Sector count
    asm volatile("outb %0, %1" : : "a"((unsigned char)(LBA & 0xFF)), "dN"((unsigned short)0x1F3));
    asm volatile("outb %0, %1" : : "a"((unsigned char)((LBA >> 8) & 0xFF)), "dN"((unsigned short)0x1F4));
    asm volatile("outb %0, %1" : : "a"((unsigned char)((LBA >> 16) & 0xFF)), "dN"((unsigned short)0x1F5));
    asm volatile("outb %0, %1" : : "a"((unsigned char)(((LBA >> 24) & 0x0F) | 0xE0)), "dN"((unsigned short)0x1F6));
    asm volatile("outb %0, %1" : : "a"((unsigned char)0x20), "dN"((unsigned short)COMMAND_REG)); // Command 0x20 = Read

    ata_wait_ready();

    for (int i = 0; i < 256; i++) {
        unsigned short word;
        asm volatile("inw %1, %0" : "=a"(word) : "dN"((unsigned short)DATA_REG));
        buffer[i * 2] = word & 0xFF;
        buffer[i * 2 + 1] = (word >> 8) & 0xFF;
    }
}

// --- 5. CORE HARDWARE INTERFACES (VGA, SERIAL, KEYBOARD) ---

void bare_metal_print(const char* message) {
    unsigned short* vga_buffer = (unsigned short *)0xB8000;
    static int current_row = 1;
    static bool initialized = false;

    if (!initialized) {
        for (int i = 0; i < 80 * 25; i++) {
            vga_buffer[i] = (COLOR_DEFAULT << 8) | ' ';
        }
        const char* header = " [x86]  Null OS v0.3  |  Persistent Storage Active  |  Standalone System ";
        for (int i = 0; header[i] != '\0' && i < 80; i++) {
            vga_buffer[i] = (0x70 << 8) | header[i];
        }
        initialized = true;
    }

    int offset = current_row * 80;
    for (int i = 0; i < 80; i++) vga_buffer[offset + i] = (COLOR_DEFAULT << 8) | ' ';
    for (int i = 0; message[i] != '\0' && i < 80; i++) {
        vga_buffer[offset + i] = (COLOR_DEFAULT << 8) | message[i];
    }

    current_row++;
    if (current_row >= 25) current_row = 1;
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

bool mystrcmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return false;
        i++;
    }
    return s1[i] == s2[i];
}

// --- 6. RUNTIME LAYER (JIT COMPILER & INSTALLER MECHANICS) ---

void compile_and_run_script(const char* script) {
    unsigned char* exec_buf = (unsigned char*)0x7000;
    if (script[0] == 'i' && script[1] == 'n' && script[2] == 't') {
        int v1 = 0, v2 = 0; bool add = false;
        for (int i = 0; script[i]; i++) {
            if (script[i] == '=' && script[i+1] >= '0' && script[i+1] <= '9') v1 = script[i+1] - '0';
            if (script[i] == '+' && script[i+1] >= '0' && script[i+1] <= '9') { add = true; v2 = script[i+1] - '0'; }
        }
        int res = add ? (v1 + v2) : v1;
        exec_buf[0] = 0xB8; // MOV EAX, imm32
        exec_buf[1] = res & 0xFF; exec_buf[2] = (res >> 8) & 0xFF;
        exec_buf[3] = (res >> 16) & 0xFF; exec_buf[4] = (res >> 24) & 0xFF;
        exec_buf[5] = 0xC3; // RET

        int (*f)() = (int (*)())0x7000;
        int out = f();
        bare_metal_print("JIT EXECUTED SCRIPT SUCCESSFULLY.");
        ((unsigned short*)0xB8000)[230] = (0x1A << 8) | (out + '0');
    }
}

void slip_install_package(const char* app_name) {
    if (app_count >= 10) {
        bare_metal_print("ERROR: MAXIMUM REGISTRY CAPACITY REACHED.");
        return;
    }

    bare_metal_print("SLIP RECEIVER WAITING FOR PACKET STREAM...");

    unsigned char* pkg_buffer = (unsigned char*)null_malloc(512); 

    int i = 0;
    while (true) {
        unsigned char c = read_serial();
        if (c == SLIP_END) { if (i > 0) break; else continue; }
        if (c == SLIP_ESC) {
            c = read_serial();
            if (c == SLIP_ESC_END) pkg_buffer[i++] = SLIP_END;
            else if (c == SLIP_ESC_ESC) pkg_buffer[i++] = SLIP_ESC;
        } else {
            if (i < 512) pkg_buffer[i++] = c;
        }
    }

    unsigned int target_sector = 100 + app_count;

    write_disk_sector(target_sector, pkg_buffer);

    int name_len = 0;
    for (; app_name[name_len] != '\0' && name_len < 11; name_len++) {
        registry[app_count].name[name_len] = app_name[name_len];
    }
    registry[app_count].name[name_len] = '\0';
    registry[app_count].entry_point = (void (*)())pkg_buffer;
    registry[app_count].size = i;
    registry[app_count].disk_lba = target_sector;

    app_count++;
    bare_metal_print("INSTALL COMPLETE. PROGRAM FLUSHED PERMANENTLY TO HARD DISK SECTOR.");
}

void handle_system_command(const char* cmd) {
    if (cmd[0] == 'i' && cmd[1] == 'n' && cmd[2] == 's' && cmd[3] == 't' && cmd[4] == ' ') {
        slip_install_package(&cmd[5]);
        return;
    }

    if (mystrcmp(cmd, "list")) {
        if (app_count == 0) {
            bare_metal_print("REGISTRY UNPOPULATED.");
            return;
        }
        bare_metal_print("--- INSTALLED ON DEVICE ---");
        for (int i = 0; i < app_count; i++) {
            bare_metal_print(registry[i].name);
        }
        return;
    }

    for (int i = 0; i < app_count; i++) {
        if (mystrcmp(cmd, registry[i].name)) {
            bare_metal_print("CONTEXT SWITCH INTO DYNAMIC APPLICATION RUNTIME...");

            read_disk_sector(registry[i].disk_lba, (unsigned char*)registry[i].entry_point);

            registry[i].entry_point();
            bare_metal_print("CONTEXT RECOVERY. KERNEL RE-SECURED CONTROL THREAD.");
            return;
        }
    }

    compile_and_run_script(cmd);
}

// --- 7. KERNEL ENTRY POINT LOOP ---

extern "C" void _start() {
    init_serial();
    bare_metal_print("NULL OS BOOT COMPLETE.");

    char buf[64]; int idx = 0; unsigned char last = 0;

    while (true) {
        unsigned char k = read_keyboard_port();

        if (k & 0x80) {
            last = 0;
        }
        else if (k != last && k != 0) {
            last = k;

            if (k == 0x1C) {
                buf[idx] = '\0';
                handle_system_command(buf);
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

