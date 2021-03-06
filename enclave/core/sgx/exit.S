// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include "asmdefs.h"
#include "asmcommon.inc"

//==============================================================================
//
// void oe_asm_exit(uint64_t arg1, uint64_t arg2)
//
// Registers:
//     RDI - arg1
//     RSI - arg2
//
// Purpose:
//     Restores user registers and executes the EEXIT instruction to leave the
//     enclave and return control to the host. This function is called for two
//     reasons:
//
//         (1) To perform an ERET (ECALL return)
//         (2) To perform an OCALL
//
// Tasks:
//
//      (1) Determines whether the caller is performing a "clean exit"
//          or a "nested exit". ECALLs and OCALLs can be nested so
//          we define DEPTH as the number of ECALL stack frames. A
//          DEPTH of zero indicates no ECALL stack frames remain and
//          that no ECALLs are pending.
//
//      (2) If this is a nested exit, then save the enclave registers
//          on the enclave stack and save the stack pointer in the
//          thread data structure (td_t.last_sp)
//
//      (3) If this a clean exit, then store zero in td_t.last_sp, forcing
//          oe_enter() to recompute it on next entry.
//
//      (4) Clear enclave registers to avoid leaking data to the host.
//
//      (5) Restore the host registers from the thread data structure
//          (td_t).
//
//      (6) Execute the SGX EEXIT instruction, exiting the enclave and
//          returning control to the host.
//
//==============================================================================

.globl oe_asm_exit
.hidden oe_asm_exit
.type oe_asm_exit, @function
oe_asm_exit:
.cfi_startproc

.determine_exit_type:

    // Check the depth of the ECALL stack (zero for clean exit)
    // exit-type-check.
    mov %gs:td_depth, %r8
    cmp $0, %r8
    je .clean_exit

.nested_exit:
    // Stop speculative execution at fallthrough of conditional
    // exit-type-check.
    lfence 

    // Save host registers and stack pointer (will be restored on reentry)
    push %r12
    push %r13
    push %r14
    push %r15

    // Construct the oe_ocall_context_t(rbp, return address).
    mov (4*OE_WORD_SIZE)(%rsp), %r12
    push %r12
    push %rbp
    mov	%rsp, %r12

    // Save the parameters of oe_asm_exit into stack.
    push %rdi
    push %rsi

    // Notify a nested exit happens.
    // oe_notify_nested_exit_start(rdi=arg1, rsi=ocall_context)
    mov %r12, %rsi
    call oe_notify_nested_exit_start

    // Restore the parameters of oe_asm_exit from stack.
    pop %rsi
    pop %rdi

    mov %rsp, %gs:td_last_sp

    jmp .clear_enclave_registers

.clean_exit:
    // Stop speculative execution at target of conditional jump
    // after exit-type-check.
    lfence

    // Clear the td_t.last_sp field (force oe_enter to calculate stack pointer)
    movq $0, %gs:td_last_sp

.clear_enclave_registers:

    // Clear these so information will not be leaked to host
    oe_cleanup_registers

.restore_host_registers:

    mov %gs:td_host_rcx, %rcx
    mov %gs:td_host_rsp, %rsp
    mov %gs:td_host_rbp, %rbp

.execute_eexit:

    // Check td_t.simulate flag
    // simulate-flag-check.
    mov %gs:td_simulate, %rax
    cmp $0, %rax
    jz .execute_eexit_instruction

.execute_eexit_sim:
    // Stop speculative execution at fallthrough of conditional
    // simulate-flag-check.
    lfence

    // Jump to return address:
    mov $1, %rax
    jmp *%rcx
    ud2

.execute_eexit_instruction:
    // Stop speculative execution at target of conditional jump
    // after simulate-flag-check.
    lfence
    
    // EEXIT(RAX=EEXIT, RBX=RETADDR, RCX=AEP, RDI=ARG1, RSI=ARG2)
    mov %rcx, %rbx
    mov $ENCLU_EEXIT, %rax
    ENCLU
    ud2

.forever:
    jmp .forever

.cfi_endproc

.size oe_asm_exit, .-oe_asm_exit

//==============================================================================
//
// This function is wrapper of oe_asm_exit. It is needed to stitch the host
// stack and enclave stack together.
//
// N.B: Don't change the function name, otherwise debugger can't work. GDB 
// depends on this hardcode function name when does stack walking for split
// stack.
//==============================================================================

.globl __morestack
.type __morestack, @function

__morestack:
.cfi_startproc
    push %rbp
.cfi_def_cfa_offset     16
.cfi_offset             rbp, -16
    mov %rsp, %rbp
.cfi_def_cfa_register   rbp

    call oe_asm_exit

    leave
    ret
.cfi_endproc

.size __morestack, .-__morestack
