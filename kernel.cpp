#include "types.h"
#include "gdt.h"

void printf(char *str)
{
    unsigned short *videomemory = (unsigned short *)0xb8000;
    static int cursor_pos = 0; // Keep track of cursor position

    for (int i = 0; str[i] != '\0'; ++i)
    {
        // White text on black background (0x0F = white, 0x00 = black)
        videomemory[cursor_pos] = (0x0F << 8) | str[i];
        cursor_pos++;

        // Wrap around if we exceed screen width (80 characters)
        if (cursor_pos >= 80 * 25)
        {
            cursor_pos = 0;
        }
    }
}

// Clear screen function
void clear_screen()
{
    unsigned short *videomemory = (unsigned short *)0xb8000;

    // Clear entire screen (80x25 = 2000 characters)
    for (int i = 0; i < 80 * 25; i++)
    {
        videomemory[i] = (0x0F << 8) | ' '; // White space on black background
    }
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; ++i)
    {
        (*i)();
    }
}

extern "C" void kernelmain(void *multiboot_structure, unsigned int magicnumber)
{
    // First, clear the screen
    clear_screen();

    // Print our message
    char *str = (char *)"Hello World from My Kernel!\n";
    printf(str);

    GlobalDescriptorTable gdt;

    while (1)
        ;
}