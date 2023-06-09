#include "libGameLogic.h"
#include <cstddef>
#include <cstdio>
#include <dirent.h>
#include <dlfcn.h>
#include <functional>
#include <iostream>
#include <link.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;


int libraryCallback(struct dl_phdr_info* info, size_t size, void* data) {
    const char* libraryName = "/home/liwinux/Downloads/PwnAdventure3/PwnAdventure3/Binaries/Linux/libGameLogic.so";  // Replace with the name of the original library
    if (strcmp(info->dlpi_name, libraryName) == 0) {
        *(void**)data = dlopen(libraryName, RTLD_LAZY);
        return 1;  // Stop iterating after finding the desired library
    }
    return 0;  // Continue iterating
}

void trampoline_SetJumpState(bool state);
void trampoline_SetJumpState(bool state)
{
    printf("chris\n");
}



void* createTrampoline(void* handle, const char* symbol, void* replacementFunction)
{
    // Find the symbol address
    void* symbolAddress = dlsym(handle, symbol);
    if (!symbolAddress)
    {
        fprintf(stderr, "Failed to find the symbol: %s\n", dlerror());
        return nullptr;
    }
	printf("PWNED [+] : Found Symbol \"%s\" at address : %p\n", symbol, symbolAddress);
	// Calculate the size of the jump instruction
	const size_t jumpInstructionSize = 14; // Adjust as needed

    // Change the memory protection to allow writing
    const size_t pageSize = getpagesize();
    uintptr_t symbolPageStart = (uintptr_t)symbolAddress & ~(pageSize - 1);
    mprotect((void*)symbolPageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC);

    // Allocate memory for the trampoline function
    void* trampoline = mmap(nullptr, jumpInstructionSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (trampoline == MAP_FAILED)
    {
        perror("Failed to allocate memory for the trampoline");
        return nullptr;
    }
	printf("PWNED [+] : Address of the trampoline function : %p\n", trampoline);
	// Write the jump instruction (trampoline) at the trampoline address
	unsigned char jumpInstruction[] = {
		0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Mov RAX, <replacementFunction>
		0xFF, 0xE0													// JMP RAX
	};
	memcpy(&jumpInstruction[2], &replacementFunction, sizeof(void*));
    memcpy(trampoline, jumpInstruction, sizeof(jumpInstruction));

	printf("PWNED [+] : Inserting JUMP to %p inside the trampoline function !\n", replacementFunction);

	// Write the jump instruction (trampoline) at the symbol address
	unsigned char symbolJumpInstruction[] = {
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,// Mov RAX, <trampoline>
        0xFF, 0xE0 // JMP RAX
    };
	printf("PWNED [+] : Patching beginning of the symbol %s(%p) with JUMP to trampoline (%p)\n", symbol, symbolAddress, trampoline);
	memcpy(&symbolJumpInstruction[2], &trampoline, sizeof(void *));
	memcpy(symbolAddress, symbolJumpInstruction, sizeof(symbolJumpInstruction));

	printf("PWNED[+] : Symbol got Patched to be redirected to %p !\n", replacementFunction);
	// Restore the original memory protection
	mprotect((void*)symbolPageStart, pageSize, PROT_READ | PROT_EXEC);

    return trampoline;
}

__attribute__((constructor))
void initSharedLib()
{
	printf("libGameLogic.so is loading ...\n");

	dlopen("/home/liwinux/Desktop/pwnie_island/cheat/libGameLogic.so", RTLD_LAZY);		// Loading itself again because the injector unloads it...

	/* Find the start address of the libGameLogic.so function */
    void* handle = nullptr;
    dl_iterate_phdr(libraryCallback, &handle);
    if (!handle) {
        // Handle error if the library handle is not found
        std::cerr << "Failed to find the library handle." << std::endl;
        // Cleanup and return or handle the error accordingly
        return;
    }
    printf("Start Address of the  : %p\n", handle);

        

	void* trampoline = createTrampoline(handle, "_ZN6Player12SetJumpStateEb", (void*)&trampoline_SetJumpState);
    if (!trampoline)
    {
        // Handle error if trampoline setup fails
        std::cerr << "Failed to create trampoline." << std::endl;
        dlclose(handle);
        return;
    }

	printf("libGameLogic.so loaded successfully !\n");
	printf("BY Chris P. (Liwinux) : The Game got PWNED !!!!\n");
	dlclose(handle);
}