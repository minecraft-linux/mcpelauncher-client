.intel_syntax noprefix

.section .text
.global xbox_shutdown_patch_run_one_hook_1
.type xbox_shutdown_patch_run_one_hook_1, @function
.global xbox_shutdown_patch_run_one_hook_2
.type xbox_shutdown_patch_run_one_hook_2, @function

xbox_shutdown_patch_run_one_hook_1:
    mov [esp+8], edx
    mov [esp+4], edi
    push eax
    call xbox_shutdown_patch_run_one_enter
    pop eax
ret
xbox_shutdown_patch_run_one_hook_2:
    call xbox_shutdown_patch_run_one_exit
    mov eax, [esi+0xC]
    cmp eax, 2
    jge exit
    add dword ptr [esp+0], 0xA
exit:
ret
