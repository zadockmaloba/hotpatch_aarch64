
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/mman.h>

void hotpatch(void *target, void *replacement) {

    auto page_size = getpagesize();

    auto aligned_target = reinterpret_cast<std::size_t>(target) & (~(page_size - 1));

    // Align the target address to an 8-byte boundary
    // uintptr_t aligned_target = ((uintptr_t)target + 7) & ~7;

    // Calculate the page address containing the target
    void *page = reinterpret_cast<void*>(aligned_target);

    assert(((uintptr_t)aligned_target & 0x07) == 0); // Ensure 8-byte alignment

    // Make the page writable and executable
    if(mprotect(page, page_size, PROT_WRITE | PROT_EXEC) != 0) {
	    std::abort;
    }
    std::cout << "Memory protected \n";

    // Calculate the relative offset for the branch instruction
    uintptr_t rel = (uintptr_t)replacement - aligned_target;

    std::cout << "Relative distance: " << rel << "\n";

    // Generate the AARCH64 branch instruction
    // AARCH64 uses the B instruction
    uint32_t instr = 0xE0000000; // B opcode
    instr |= ((rel >> 2) & 0x03FFFFFF); // Offset in words, not bytes

    std::cout << "Attempting to patch target: " << aligned_target << "\n";
    // Write the instruction to the target address
    // *(uint32_t *)aligned_target = instr;
    memcpy(target, replacement, rel);
    std::cout << "Patched target \n";

    // Restore the page protection to executable only
    mprotect(page, page_size, PROT_EXEC);
}

int original_function() {
	return 42;
}

int new_function() {
	return 84;
}

int main() {
    // Example usage:
    void* f1 = reinterpret_cast<void*>((std::size_t&)original_function);
    void* f2 = reinterpret_cast<void*>((std::size_t&)new_function);

    hotpatch(f1, f2);

    // Test the patched function
    printf("Original function: %d\n", original_function());

    return 0;
}
