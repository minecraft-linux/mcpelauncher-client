.intel_syntax noprefix

.global xbox_shutdown_patch_run_one_hook_1
.global xbox_shutdown_patch_run_one_hook_2

xbox_shutdown_patch_run_one_hook_1:
    mov [esp+8], edx
    mov [esp+4], edi
    sub esp, 8
    push eax
    call xbox_shutdown_patch_run_one_enter
    pop eax
    add esp, 8
ret
xbox_shutdown_patch_run_one_hook_2:
    sub esp, 0xC
    call xbox_shutdown_patch_run_one_exit
    add esp, 0xC
    mov eax, [esi+0xC]
    cmp eax, 2
    jge exit
    add dword ptr [esp+0], 0xA
exit:
ret
