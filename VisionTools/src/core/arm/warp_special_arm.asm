;---------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
;----------------------------------------------------------------------------
    AREA Asm, CODE

    EXPORT  WarpSpecialPixel_BilinearByte4_Neon_Asm
    EXPORT  WarpSpecialSpan_by4_BilinearByte4_Neon_Asm
    EXPORT  WarpSpecialSpan_by4_BilinearByte1_Neon_Asm
    EXPORT  WarpSpecialSpan_by4_BilinearByte2_Neon_Asm

;
; Calling Convention:
;   first four 32bpp operands in r0..r3
;   preserve r4..r8, r10..r11 - r9 is platform specific 
;   r13 is stack ptr
;   r12 is scratch
;   don't use r14,r15
;
;   preserve q4..q7 only for Neon
;
;-------------------------------------------------------------------------------------------------    
; WarpSpecialPixel_BilinearByte4_Neon_Asm
; r0 = pDestPix
; r1 = srcPixStride
; r2 = pSrcPixBase
; r3 = pSpanIteratorInfo
; 
WarpSpecialPixel_BilinearByte4_Neon_Asm

    ; save clobbered registers
    push {r4,r5,r6,r7}
    
    ; load u,v
    ldr         r4,[r3,#0x0]    ; u
    ldr         r5,[r3,#0x4]    ; v

    uxth        r1,r1           ; not sure why this is needed...

    ; compute source addresses and load source pixels AB and CD
    ubfx        r6,r4,#16,#16   ; addrv
    ubfx        r7,r5,#16,#16   ; addrv
    mla         r6,r7,r1,r6     ; addru += addrv*srcPixStride
    add         r6,r2,r6,lsl #2 ; pixaddrAB
    vld1.8      {d16},[r6]      ; load AB
    add         r6,r6,r1,lsl #2 ; pixaddrCD = pixaddrAB+srcPixStride
    vld1.8      {d18},[r6]      ; load CD

    ; compute fractional weights for each sample
    ubfx        r4,r4,#8,#8     ; fracu
    ubfx        r5,r5,#8,#8     ; fracv
    mov         r6,0x0080       ; half
    smlabb      r7,r4,r5,r6     ; .8*.8 = .16 + half
    ubfx        r7,r7,#8,#8     ; wd = u*v
    sub         r6,r5,r7        ; wc = v-(u*v)
    sub         r5,r4,r7        ; wb = v-(u*v)
    rsb         r4,0x0100       ; 1-u
    sub         r4,r4,r6        ; 1-u-v+(u*v)
    vmov.u16    d0[0],r4        ; move to neon regs
    vmov.u16    d0[1],r5
    vmov.u16    d0[2],r6
    vmov.u16    d0[3],r7

    ; expand source pixels to uint16x8_t
    vmovl.u8    q8,d16
    vmovl.u8    q9,d18

    ; scale 16bpp source pixels by weights
    vmul.i16    d16,d16,d0[0]
    vmla.i16    d16,d17,d0[1]
    vmla.i16    d16,d18,d0[2]
    vmla.i16    d16,d19,d0[3]

    ; do rounding shift
    vrshr.s16   d16,d16,#8

    ; compact result, store, and bump dest pointer
    vmovn.i16   d16,q8          ; pack result
    vst1.32     {d16[0]},[r0]!  ; store and bump dest pointer

    ; restore clobbered registers
    pop {r4,r5,r6,r7}
    bx    lr
;-------------------------------------------------------------------------------------------------    

;-------------------------------------------------------------------------------------------------    
; WarpSpecialSpan_by4_BilinearByte4_Neon_Asm
; r0 = pDestPix
; r1 = w4 (number of 4-pixel groups to process)
; r2 = pSrcPixBase
; r3 = pSpanIteratorInfo
; 
WarpSpecialSpan_by4_BilinearByte4_Neon_Asm
    ; save clobbered registers
    push {r4,r5,r6,r7,r8,r9,r10}
    vpush {q4,q5,q6,q7}
    
    ; ::r4 = srcPixStride
    ldr         r4,[r3,#0x10]

    ; ::r5,r6 = u, du
    ldr         r5,[r3,#0x0]
    ldr         r6,[r3,#0x8]

    ; ::r7,r8 = v, dv
    ldr         r7,[r3,#0x4]
    ldr         r8,[r3,#0xc]

    ; ::d0,d1 = ufrac4, du4
    vmov.u16    d0[0],r5
    add         r9,r5,r6
    vmov.u16    d0[1],r9
    add         r9,r9,r6
    vmov.u16    d0[2],r9 
    add         r9,r9,r6
    vmov.u16    d0[3],r9 
    lsl         r9,r6,2
    vdup.u16    d1,r9

    ; ::d2,d3 = vfrac4, dv4
    vmov.u16    d2[0],r7
    add         r9,r7,r8
    vmov.u16    d2[1],r9
    add         r9,r9,r8
    vmov.u16    d2[2],r9 
    add         r9,r9,r8
    vmov.u16    d2[3],r9 
    lsl         r9,r8,2
    vdup.u16    d3,r9   

    ; :: d8 = 0x0100
    vmov.i16    d8,#0x0100

1
    ; ---- start of per-pixel loop ----

    ; compute source addresses and load source pixels AB and CD for dest 0
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #2     ; pixaddrAB
    vld1.8      {d16},[r9]          ; load AB
    add         r9,r9,r4,lsl #2     ; pixaddrCD = pixaddrAB+srcPixStride
    vld1.8      {d18},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute source addresses and load source pixels AB and CD for dest 1
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #2     ; pixaddrAB
    vld1.8      {d20},[r9]          ; load AB
    add         r9,r9,r4,lsl #2     ; pixaddrCD = pixaddrAB+srcPixStride
    vld1.8      {d22},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute fractional weights for each sample
    vshr.u16    d4,d0,#8            ; ufrac>>8
    vshr.u16    d6,d2,#8            ; vfrac>>8
    vmul.u16    d7,d4,d6            ; ufrac*vfrac
    vrshr.u16   d7,d7,#8            ; wd4 (after rounding shift back to 8 bits)
    vsub.u16    d6,d6,d7            ; wc4
    vsub.u16    d5,d4,d7            ; wb4
    vsub.u16    d4,d8,d4            ; 1-ufrac
    vsub.u16    d4,d4,d6            ; wa4 = 1-ufrac-wc4

    ; step vector u,v
    vadd.u16    d0,d0,d1
    vadd.u16    d2,d2,d3

    ; compute source addresses and load source pixels AB and CD for dest 2
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #2     ; pixaddrAB
    vld1.8      {d24},[r9]          ; load AB
    add         r9,r9,r4,lsl #2     ; pixaddrCD = pixaddrAB+srcPixStride
    vld1.8      {d26},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute source addresses and load source pixels AB and CD for dest 3
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #2     ; pixaddrAB
    vld1.8      {d28},[r9]          ; load AB
    add         r9,r9,r4,lsl #2     ; pixaddrCD = pixaddrAB+srcPixStride
    vld1.8      {d30},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; process 1st two pixels - expand, scale by weights, then pack and store to lower of qreg
    vmovl.u8    q8,d16
    vmovl.u8    q9,d18
    vmovl.u8    q10,d20
    vmovl.u8    q11,d22
    vmul.i16    d16,d16,d4[0]
    vmla.i16    d16,d17,d5[0]
    vmul.i16    d17,d20,d4[1]
    vmla.i16    d17,d21,d5[1]
    vmla.i16    d16,d18,d6[0]
    vmla.i16    d17,d22,d6[1]
    vmla.i16    d16,d19,d7[0]
    vmla.i16    d17,d23,d7[1]
    vrshr.s16   q8,q8,#8
    vmovn.i16   d16,q8
    ; process 2nd two pixels - expand, scale by weights, then pack and store to upper of qreg
    vmovl.u8    q12,d24
    vmovl.u8    q13,d26
    vmovl.u8    q14,d28
    vmovl.u8    q15,d30
    vmul.i16    d18,d24,d4[2]
    vmul.i16    d19,d28,d4[3]
    vmla.i16    d18,d25,d5[2]
    vmla.i16    d19,d29,d5[3]
    vmla.i16    d18,d26,d6[2]
    vmla.i16    d19,d30,d6[3]
    vmla.i16    d18,d27,d7[2]
    vmla.i16    d19,d31,d7[3]
    vrshr.s16   q9,q9,#8
    vmovn.i16   d17,q9
    ; store all 4 result pixels
    vst1.32     {d16,d17},[r0]!

    ; ---- end of per-pixel loop ----
    sub         r1, #1
    cmp         r1, #0
    ble         %F2     ; label 1 is too far to reach with 16bit thumb instruction set
    b           %B1
2

    ; restore clobbered registers
    vpop {q4,q5,q6,q7}
    pop {r4,r5,r6,r7,r8,r9,r10}
    bx    lr
; WarpSpecialSpan_by4_BilinearByte4_Neon_Asm
;-------------------------------------------------------------------------------------------------    

;-------------------------------------------------------------------------------------------------    
; WarpSpecialSpan_by4_BilinearByte1_Neon_Asm
; r0 = pDestPix
; r1 = w4 (number of 4-pixel groups to process)
; r2 = pSrcPixBase
; r3 = pSpanIteratorInfo
; 
WarpSpecialSpan_by4_BilinearByte1_Neon_Asm
    ; save clobbered registers
    push {r4,r5,r6,r7,r8,r9,r10}
    vpush {q4,q5,q6,q7}
    
    ; ::r4 = srcPixStride
    ldr         r4,[r3,#0x10]

    ; ::r5,r6 = u, du
    ldr         r5,[r3,#0x0]
    ldr         r6,[r3,#0x8]

    ; ::r7,r8 = v, dv
    ldr         r7,[r3,#0x4]
    ldr         r8,[r3,#0xc]

    ; ::d0,d1 = ufrac4, du4
    vmov.u16    d0[0],r5
    add         r9,r5,r6
    vmov.u16    d0[1],r9
    add         r9,r9,r6
    vmov.u16    d0[2],r9 
    add         r9,r9,r6
    vmov.u16    d0[3],r9 
    lsl         r9,r6,2
    vdup.u16    d1,r9

    ; ::d2,d3 = vfrac4, dv4
    vmov.u16    d2[0],r7
    add         r9,r7,r8
    vmov.u16    d2[1],r9
    add         r9,r9,r8
    vmov.u16    d2[2],r9 
    add         r9,r9,r8
    vmov.u16    d2[3],r9 
    lsl         r9,r8,2
    vdup.u16    d3,r9   

    ; ::d8 = 0x0100
    vmov.i16    d8,#0x0100

1
    ; ---- start of per-pixel loop ----

    ; compute source addresses and load source pixels AB and CD for dest 0
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9            ; pixaddrAB
    vld2.8      {d16[0],d17[0]},[r9]; load AB
    add         r9,r9,r4     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.8      {d18[0],d19[0]},[r9]; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute source addresses and load source pixels AB and CD for dest 1
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9     ; pixaddrAB
    vld2.8      {d16[1],d17[1]},[r9]          ; load AB
    add         r9,r9,r4     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.8      {d18[1],d19[1]},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute fractional weights for each sample
    vshr.u16    d4,d0,#8            ; ufrac>>8
    vshr.u16    d6,d2,#8            ; vfrac>>8
    vmul.u16    d7,d4,d6            ; ufrac*vfrac
    vrshr.u16   d7,d7,#8            ; wd4 (after rounding shift back to 8 bits)
    vsub.u16    d6,d6,d7            ; wc4
    vsub.u16    d5,d4,d7            ; wb4
    vsub.u16    d4,d8,d4            ; 1-ufrac
    vsub.u16    d4,d4,d6            ; wa4 = 1-ufrac-wc4

    ; step vector u,v
    vadd.u16    d0,d0,d1
    vadd.u16    d2,d2,d3

    ; compute source addresses and load source pixels AB and CD for dest 2
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9     ; pixaddrAB
    vld2.8      {d16[2],d17[2]},[r9]          ; load AB
    add         r9,r9,r4     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.8      {d18[2],d19[2]},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute source addresses and load source pixels AB and CD for dest 3
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9     ; pixaddrAB
    vld2.8      {d16[3],d17[3]},[r9]          ; load AB
    add         r9,r9,r4     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.8      {d18[3],d19[3]},[r9]          ; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; process 1st two pixels - expand, scale by weights, then pack and store to lower of qreg
    vmovl.u8    q10,d16
    vmovl.u8    q11,d17
    vmovl.u8    q12,d18
    vmovl.u8    q13,d19
    vmul.i16    d20,d20,d4
    vmla.i16    d20,d22,d5
    vmla.i16    d20,d24,d6
    vmla.i16    d20,d26,d7
    vrshr.s16   d20,d20,#8
    vmovn.i16   d20,q10
    ; store all 4 result pixels
    vst1.32     {d20[0]},[r0]!

    ; ---- end of per-pixel loop ----
    sub         r1, #1
    cmp         r1, #0
    bgt         %B1 

    ; restore clobbered registers
    vpop {q4,q5,q6,q7}
    pop {r4,r5,r6,r7,r8,r9,r10}
    bx    lr
; WarpSpecialSpan_by4_BilinearByte1_Neon_Asm
;-------------------------------------------------------------------------------------------------    

;-------------------------------------------------------------------------------------------------    
; WarpSpecialSpan_by4_BilinearByte2_Neon_Asm
; r0 = pDestPix
; r1 = w4 (number of 4-pixel groups to process)
; r2 = pSrcPixBase
; r3 = pSpanIteratorInfo
; 
WarpSpecialSpan_by4_BilinearByte2_Neon_Asm
    ; save clobbered registers
    push {r4,r5,r6,r7,r8,r9,r10}
    vpush {q4,q5,q6,q7}
    
    ; ::r4 = srcPixStride
    ldr         r4,[r3,#0x10]

    ; ::r5,r6 = u, du
    ldr         r5,[r3,#0x0]
    ldr         r6,[r3,#0x8]

    ; ::r7,r8 = v, dv
    ldr         r7,[r3,#0x4]
    ldr         r8,[r3,#0xc]

    ; ::d0,d1 = ufrac4, du4
    vmov.u16    d0[0],r5
    add         r9,r5,r6
    vmov.u16    d0[1],r9
    add         r9,r9,r6
    vmov.u16    d0[2],r9 
    add         r9,r9,r6
    vmov.u16    d0[3],r9 
    lsl         r9,r6,2
    vdup.u16    d1,r9

    ; ::d2,d3 = vfrac4, dv4
    vmov.u16    d2[0],r7
    add         r9,r7,r8
    vmov.u16    d2[1],r9
    add         r9,r9,r8
    vmov.u16    d2[2],r9 
    add         r9,r9,r8
    vmov.u16    d2[3],r9 
    lsl         r9,r8,2
    vdup.u16    d3,r9   

    ; ::d8 = 0x0100
    vmov.i16    d8,#0x0100
1
    ; ---- start of per-pixel loop ----

    ; compute source addresses and load source pixels AB and CD for dest 0
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #1     ; pixaddrAB
    vld2.16     {d16[0],d17[0]},[r9]; load AB
    add         r9,r9,r4,lsl #1     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.16     {d18[0],d19[0]},[r9]; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute source addresses and load source pixels AB and CD for dest 1
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #1     ; pixaddrAB
    vld2.16     {d16[1],d17[1]},[r9]; load AB
    add         r9,r9,r4,lsl #1     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.16     {d18[1],d19[1]},[r9]; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute fractional weights for each sample
    vshr.u16    d4,d0,#8            ; ufrac>>8
    vshr.u16    d28,d2,#8           ; vfrac>>8
    vmul.u16    d30,d4,d28          ; ufrac*vfrac
    vrshr.u16   d30,d30,#8          ; wd4 (after rounding shift back to 8 bits)
    vsub.u16    d28,d28,d30         ; wc4
    vsub.u16    d6,d4,d30           ; wb4
    vsub.u16    d4,d8,d4            ; 1-ufrac
    vsub.u16    d4,d4,d28           ; wa4 = 1-ufrac-wc4

    vmov        d5,d4               ; interleave weights so qreg = |w0 w0 w1 w1 w2 w2 w3 w3|
    vzip.16     d4,d5               ; for scaling 2 band samples 
    vmov        d7,d6
    vzip.16     d6,d7
    vmov        d31,d30
    vzip.16     d30,d31
    vmov        d29,d28
    vzip.16     d28,d29   

    ; step vector ufraction,vfraction
    vadd.u16    d0,d0,d1
    vadd.u16    d2,d2,d3

    ; compute source addresses and load source pixels AB and CD for dest 2
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #1     ; pixaddrAB
    vld2.16     {d16[2],d17[2]},[r9]; load AB
    add         r9,r9,r4,lsl #1     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.16     {d18[2],d19[2]},[r9]; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; compute source addresses and load source pixels AB and CD for dest 3
    ubfx        r9,r5,#16,#16       ; addru
    ubfx        r10,r7,#16,#16      ; addrv
    mla         r9,r10,r4,r9        ; addru += addrv*srcPixStride
    add         r9,r2,r9,lsl #1     ; pixaddrAB
    vld2.16     {d16[3],d17[3]},[r9]; load AB
    add         r9,r9,r4,lsl #1     ; pixaddrCD = pixaddrAB+srcPixStride
    vld2.16     {d18[3],d19[3]},[r9]; load CD
    adds        r5,r5,r6            ; step scalar u
    adds        r7,r7,r8            ; step scalar v

    ; process 1st two pixels - expand, scale by weights, then pack and store to lower of qreg
    vmovl.u8    q10,d16
    vmovl.u8    q11,d17
    vmovl.u8    q12,d18
    vmovl.u8    q13,d19
    vmul.i16    q10,q10,q2
    vmla.i16    q10,q11,q3
    vmla.i16    q10,q12,q14
    vmla.i16    q10,q13,q15
    vrshr.s16   q10,q10,#8
    vmovn.i16   d20,q10
    vst1.16     {d20},[r0]!

    ; ---- end of per-pixel loop ----
    sub         r1, #1
    cmp         r1, #0
    ble         %F2     ; label 1 is too far to reach with 16bit thumb instruction set
    b           %B1
2

    ; restore clobbered registers
    vpop {q4,q5,q6,q7}
    pop {r4,r5,r6,r7,r8,r9,r10}
    bx    lr
; WarpSpecialSpan_by4_BilinearByte2_Neon_Asm
;-------------------------------------------------------------------------------------------------    

    END
