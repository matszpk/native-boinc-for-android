; AVX detection based on section 2.2 of "Intel Advanced Vector Extensions Programming Reference"
;
; MASM source needed for 64 bit builds with MicroSoft tools


PUBLIC avxSupported


_TEXT         SEGMENT

avxSupported  PROC

      mov    eax, 1
      cpuid
      and    ecx, 018000000H
      cmp    ecx, 018000000H    ; check both OSXSAVE and AVX feature flags
      jne    NOSUPPORT
      xor    ecx, ecx           ; specify 0 for XFEATURE_ENABLED_MASK register
      xgetbv                    ; result in EDX:EAX
      and    eax, 06H
      cmp    eax, 06H           ; check OS has enabled both XMM and YMM state support
      jne    NOSUPPORT
      mov    eax, 1
      jmp    DONE

    NOSUPPORT:
      mov    eax, 0

    DONE:
      ret

avxSupported  ENDP

_TEXT         ENDS

END