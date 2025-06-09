void printf(char *str)
{
    unsigned short *videomemory = (unsigned short *)0xb8000;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        videomemory[i] = (videomemory[i] & 0xFF00) | str[i];
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
    char *str = (char *)"Hello World";
    printf(str);

    while (1)
        ;
}