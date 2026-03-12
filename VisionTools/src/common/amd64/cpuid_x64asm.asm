; *@@@+++@@@@******************************************************************
; 
;  Copyright (C) Microsoft Corporation. All rights reserved.
; 
; *@@@---@@@@******************************************************************

        TITLE common\cpuid_x64asm.asm

include macamd64.inc

        NESTED_ENTRY CPUID_ECX_Clearedasm, _TEXT

        rex_push_reg rbx                ; save nonvolatile register

        END_PROLOGUE

        mov     r9, rcx                 ; save output buffer address
        mov     eax, edx                ; set CPUID function value
        xor     ecx, ecx                ; clear CPUID function index
        cpuid                           ; get cpu information
        mov     0[r9], eax              ; save specified cpu information
        mov     4[r9], ebx              ;
        mov     8[r9], ecx              ;
        mov     12[r9], edx             ;
        pop     rbx                     ; restore nonvolatile register
        ret                             ; return

        NESTED_END CPUID_ECX_Clearedasm, _TEXT

        end
