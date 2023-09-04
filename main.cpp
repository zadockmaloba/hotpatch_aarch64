
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>

void hotpatch(void *target, void *replacement) {
    // Align the target address to an 8-byte boundary
    uintptr_t aligned_target = ((uintptr_t)target + 7) & ~7;

    // Calculate the page address containing the target
    void *page = (void *)(aligned_target & ~0xFFF);

    assert(((uintptr_t)aligned_target & 0x07) == 0); // Ensure 8-byte alignment

    // Make the page writable and executable
    mprotect(page, 4096, PROT_WRITE | PROT_EXEC);

    // Calculate the relative offset for the branch instruction
    uintptr_t rel = (uintptr_t)replacement - aligned_target;

    // Generate the AARCH64 branch instruction
    // AARCH64 uses the B instruction
    uint32_t instr = 0xE0000000; // B opcode
    instr |= ((rel >> 2) & 0x03FFFFFF); // Offset in words, not bytes

    // Write the instruction to the target address
    *(uint32_t *)aligned_target = instr;

    // Restore the page protection to executable only
    mprotect(page, 4096, PROT_EXEC);
}

int main() {
    // Example usage:
    int original_function() {
        return 42;
    }

    int new_function() {
        return 84;
    }

    hotpatch(original_function, new_function);

    // Test the patched function
    printf("Original function: %d\n", original_function());

    return 0;
}
