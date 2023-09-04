
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/mman.h>

#include "plthook/plthook.h"

//__attribute__ ((aligned(8)))
__attribute__ ((noinline))

size_t plt_hotpatch(void *original_function, void *new_function) {
    plthook_t *plthook;
    
    if (plthook_open_by_address(&plthook, reinterpret_cast<void*>(original_function)) != 0) {
        printf("plthook_open error: %s\n", plthook_error());
        return -1;
    }
    if (plthook_replace(plthook, "recv", (void*)new_function, (void**)&original_function) != 0) {
        printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return -1;
    }
#ifndef WIN32
    // The address passed to the fourth argument of plthook_replace() is
    // available on Windows. But not on Unixes. Get the real address by dlsym().
    //recv_func = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_DEFAULT, "recv");
#endif
    plthook_close(plthook);
}

size_t plt_hotpatch_2(void *original_function, void *new_function) {
    plthook_t *plthook;
    
    /*if (plthook_open_by_address(&plthook, reinterpret_cast<void*>(original_function)) != 0) {
        printf("plthook_open error: %s\n", plthook_error());
        return -1;
    }*/
    if (plthook_replace_builtin(plthook, (void*)new_function, (void**)&original_function) != 0) {
        printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return -1;
    }
#ifndef WIN32
    // The address passed to the fourth argument of plthook_replace() is
    // available on Windows. But not on Unixes. Get the real address by dlsym().
    //recv_func = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_DEFAULT, "recv");
#endif
    plthook_close(plthook);
}

void aarch64_hotpatch(void *target, void *replacement) {

    auto page_size = getpagesize();

    auto aligned_target = reinterpret_cast<std::size_t>(target) & (~(page_size - 1));

    // Align the target address to an 8-byte boundary
    // uintptr_t aligned_target = ((uintptr_t)target + 7) & ~7;

    assert(((uintptr_t)aligned_target & 0x07) == 0); // Ensure 8-byte alignment

    // Calculate the page address containing the target
    void *page = reinterpret_cast<void*>(target);

    // Make the page writable and executable
    if(mprotect(page, page_size, PROT_WRITE | PROT_EXEC) != 0) {
	    std::abort;
    }
    std::cout << "Memory protected \n";

    // Calculate the relative offset for the branch instruction
    uintptr_t rel = (uintptr_t)replacement - (uintptr_t)target;

    std::cout << "Relative distance: " << rel << "\n";

    // Generate the AARCH64 branch instruction
    // AARCH64 uses the B instruction
    uint32_t instr = 0xE0000000; // B opcode
    instr |= ((rel >> 2) & 0x03FFFFFF); // Offset in words, not bytes

    std::cout << "Attempting to patch target: " << target << "\n";
    // Write the instruction to the target address
    // *(uint32_t *)aligned_target = instr;
    //memcpy(target, replacement + 4, rel);
    union {
        uint8_t bytes[8];
        uint64_t value;
    } instruction = {{0xe9, rel >> 0, rel >> 8, rel >> 16, rel >> 24}};
    *(uint64_t *)target = instruction.value;
    std::cout << "Patched target \n";

    // Restore the page protection to executable only
    mprotect(page, page_size, PROT_EXEC);
}

void x86_hotpatch(void *target, void *replacement) {
    assert(((uintptr_t)target & 0x07) == 0);
    void *page = (void *)((uintptr_t)target & ~0xfff);
    mprotect(page, 4096, PROT_WRITE | PROT_EXEC);
    uint32_t rel = (char *)replacement - (char *)target - 5;
    union {
        uint8_t bytes[8];
        uint64_t value;
    } instruction = {{0xe9, 0}};
    *(uint64_t *)target = instruction.value;
    mprotect(page, 4096, PROT_EXEC);
}

int original_function() {
	return 42;
}

int new_function() {
	return 84;
}

int main() {
    // Example usage:

    plt_hotpatch((void*)&original_function, (void*)new_function);

    x86_hotpatch((void*)original_function, (void*)new_function);
    //aarch64_hotpatch((void*)&original_function, (void*)new_function);
    // Test the patched function
    printf("Original function: %d\n", original_function());

    return 0;
}
