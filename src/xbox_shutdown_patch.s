.intel_syntax noprefix

.global xbox_shutdown_patch_run_one_hook_1
.global xbox_shutdown_patch_run_one_hook_2
.global xbox_shutdown_patch_run_one_hook_1_1_9
.global xbox_shutdown_patch_run_one_hook_2_1_9

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
ret

xbox_shutdown_patch_run_one_hook_1_1_9:
    mov [esp+8], ecx
    mov [esp+4], edi
    sub esp, 8
    push eax
    call xbox_shutdown_patch_run_one_enter
    pop eax
    add esp, 8
ret
xbox_shutdown_patch_run_one_hook_2_1_9:
    sub esp, 0xC
    call xbox_shutdown_patch_run_one_exit
    add esp, 0xC
    mov ecx, [esp+0x5C]
    mov eax, [ecx+0xC]
ret
