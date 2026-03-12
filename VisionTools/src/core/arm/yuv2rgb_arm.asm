;------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
;------------------------------------------------------------------------------
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

    AREA Asm, CODE

    EXPORT  NeonYUVToRGBALineFast
    EXPORT  NeonYUVToRGBALine

;------------------------------------------------------------------------------
; NeonYUVToRGBALine
; assumes no scaling
; assumes UV is 1/4 res of Y
; assumes UV is in fact laid out VUVUVUVU... (reversed from UV ordering)
; r0 = width ; must be divisible/32
; r1 = pYBase
; r2 = pUVBase
; r3 = pRGBBase
; 
NeonYUVToRGBALineFast
	; will clobber:
	; r2, r3, r4, r5, r6, r8, q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15
	
	push {r4,r5,r6,r7,r8}
    vpush {q4,q5,q6,q7}
	
	; Move input/output pointers into registers
	mov r4, r1	;%[pYRow] 
	mov r5, r2	;%[pUVRow] 
	mov r6, r3	;%[pRgbRow] 

	; Compute last output adress (so we know when to stop looping)
	mov r8, #4 
	mul r8, r0, r8 ;r0 = %[nYWidthDivisibleByThirtyTwo]
	add r8, r6 

    ; set the 2xByte constants
    mov         r3, #51288          ; rb
    vmov.16     d30[0], r3
    mov         r3, #8696           ; gb
    vmov.16     d30[1], r3
    mov         r3, #47864          ; bb
    vmov.16     d30[2], r3
    mov         r3, #75             ; ys
    vdup.8      d31, r3
    mov         r3, #102            ; vsr
    vdup.16     q8, r3
    mov         r3, #25             ; usg
    vdup.16     q7, r3
    mov         r3, #52             ; vsg
    vdup.16     q6, r3
    mov         r3, #129            ; usb
    vdup.16     q14, r3

    ; load constant alpha channel to q13
    mov r3, 0xff
    vdup.8  q13, r3

	b %F2 
    ; main loop
1 
	; load 16 y values with pointer increment to q10
	vld1.8  {d20-d21}, [r4]!

    ; load 8 deinterleaved u and v values to q11 and q12
	vld2.8  {d22-d25}, [r5] 
	; increment pointer by 16 because of half resolution
	add r5, #16 

    ; r s16: q0,q1
    ; g s16: q2,q3
    ; b s16: q4,q5

    ; g = rb
    vdup.16     q0, d30[0]
    vdup.16     q1, d30[0]
    ; g = gb
    vdup.16     q2, d30[1]
    vdup.16     q3, d30[1]
    ; b = bb
    vdup.16     q4, d30[2]
    vdup.16     q5, d30[2]

    ; compute yp = y*ys expanded to 16bpp
    vmull.u8   q9, d20, d31
    vmull.u8   q10, d21, d31
    ; add to each accum
    vadd.s16    q0, q0, q9
    vadd.s16    q1, q1, q10
    vadd.s16    q2, q2, q9
    vadd.s16    q3, q3, q10
    vadd.s16    q4, q4, q9
    vadd.s16    q5, q5, q10

    ; 'Nearest neighbor upsampling' of u and v to get 16 values
	vorr        q10, q11, q11 
	vzip.8      q11, q10        ; 16 u values in q11
	vorr        q10, q12, q12
	vzip.8      q12, q10        ; 16 v values in q12

    ; expand u values to 16bpp and do accumulations
    vmovl.u8    q9, d22
    vmovl.u8    q10, d23
    ; g -= u*usg
    vmls.s16    q2, q9, q7
    vmls.s16    q3, q10, q7
    ; b += u*usb
    vmla.s16    q4, q9, q14
    vmla.s16    q5, q10, q14

    ; expand v values to 16bpp and do accumulations
    vmovl.u8    q9, d24
    vmovl.u8    q10, d25
    ; r += v*vsr
    vmla.s16    q0, q9, q8
    vmla.s16    q1, q10, q8
    ; g -= v*vsg
    vmls.s16    q2, q9, q6
    vmls.s16    q3, q10, q6

    vqshrun.s16 d24, q0, 6
    vqshrun.s16 d25, q1, 6
    vqshrun.s16 d22, q2, 6
    vqshrun.s16 d23, q3, 6
    vqshrun.s16 d20, q4, 6
    vqshrun.s16 d21, q5, 6

    ; b = q10, g = q11, r = q12, a = q13

	; interleaved store of 16 pixels (with pointer increment)
	vst4.8 { d20, d22, d24, d26 }, [r6]! 
	vst4.8 { d21, d23, d25, d27 }, [r6]! 
2 
	cmp r6, r8 
	blt %B1 

    vpop {q4,q5,q6,q7}
	pop {r4,r5,r6,r7,r8}
	bx	lr
	
;-------------------------------------------------------------------------------------------------	
    
;-------------------------------------------------------------------------------------------------	
; NeonYUVToRGBALine
; assumes no scaling
; assumes UV is 1/4 res of Y
; assumes UV is in fact laid out VUVUVUVU... (reversed from UV ordering)
; r0 = width ; must be divisible/32
; r1 = pYBase
; r2 = pUVBase
; r3 = pRGBBase
; 
NeonYUVToRGBALine
	; will clobber:
	; r2, r3, r4, r5, r6, r8, q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15
	
	push {r4,r5,r6,r7,r8}
    vpush {q4,q5,q6,q7}
	
	; +--------------------------+
	; | Prepare various pointers |
	; +--------------------------+

	; Move input/output pointers into registers
	mov r4, r1	;%[pYRow] 
	mov r5, r2	;%[pUVRow] 
	mov r6, r3	;%[pRgbRow] 

	; Compute last output adress (so we know when to stop looping)
	mov r8, #4 
	mul r8, r0, r8 ;r0 = %[nYWidthDivisibleByThirtyTwo]
	add r8, r6 


	; +------------------------------------+
	; | Initialize variables - Q15, Q0, Q1 |
	; +------------------------------------+

	; Q15 = [128, 128, 128, 128] (4x32 bit = 128 bit)
	mov.w r3, #128 
	vdup.32 q15, r3  

	; D0[0] = 298 (1x16 bit)
	mov r3, #298 
	vmov.16 d0[0], r3 

	; D0[1] = 516 (1x16 bit)
	mov r3, #516 
	vmov.16 d0[1], r3 

	; D0[2] = -100 (1x16 bit)
	mov r3, #-100 
	vmov.16 d0[2], r3 

	; D0[3] = 208 (1x16 bit)
	mov r3, #208 
	vmov.16 d0[3], r3 

	; D1[0] = 409 (1x16 bit)
	movw r3, #409 
	vmov.16 d1[0], r3 

	; D2 = [16, 16, 16, 16, 16, 16, 16, 16] (8x8 bit = 64 bit)
	mov r3, #16 
	vdup.8 d2, r3 

	; D3 = [128, 128, 128, 128, 128, 128, 128, 128] (8x8 bit = 64 bit)
	mov r3, #128 
	vdup.8 d3, r3 


	; +-----------+
	; | Main loop |
	; +-----------+

	b %F2 
1 


	; +--------+
	; | Load y |
	; +--------+

	; Load y (Q6) with pointer increment
	vld1.8 {d12-d13}, [r4]!       


	; +---------------------------+
	; | c' = (y - 16) * 298 + 128 |
	; +---------------------------+

	; c = y - 16, long (Q13, Q14) 
	vsubl.u8 q13, d12, d2 
	vsubl.u8 q14, d13, d2 

	; c' = c * 298 + 128 - final result stored in (Q2, Q3, Q4, Q5)
	vmull.s16 q2, d26, d0[0] 
	vmull.s16 q3, d27, d0[0] 
	vmull.s16 q4, d28, d0[0] 
	vmull.s16 q5, d29, d0[0] 

	vadd.s32 q2, q15 
	vadd.s32 q3, q15 
	vadd.s32 q4, q15 
	vadd.s32 q5, q15 


	; +--------------+
	; | Load u and v |
	; +--------------+

	; Load u and v (Q7, Q8)
	vld2.8 {d14-d17}, [r5] 

	; Only increment pointer by 16 because of half resolution
	add r5, #16 

	; 'Nearest neighbor upsampling' of u and v vector (since they're only half resolution)
	vorr q6, q7, q7 
	vzip.8 q7, q6 

	vorr q6, q8, q8 
	vzip.8 q8, q6 


	; +-------------+
	; | e = v - 128 |
	; | d = u - 128 |
	; +-------------+

	; e = v - 128, long (Q9, Q10)
	vsubl.u8 q9,  d14, d3 
	vsubl.u8 q10, d15, d3 

	; d = u - 128, long (Q11, Q12)
	vsubl.u8 q11, d16, d3 
	vsubl.u8 q12, d17, d3 


	; +------------------------------------+
	; | b = (298 * c + 516 * d + 128) >> 8 |
	; +------------------------------------+

	; b = 516 * d
	vmull.s16 q6,  d22, d0[1] 
	vmull.s16 q7,  d23, d0[1] 
	vmull.s16 q8,  d24, d0[1] 
	vmull.s16 q13, d25, d0[1] 

	; b += c'
	vadd.i32 q6,  q6,  q2 
	vadd.i32 q7,  q7,  q3 
	vadd.i32 q8,  q8,  q4 
	vadd.i32 q13, q13, q5 

	; b >>= 8 (narrowed 32 bit -> 16 bit, results stored in Q6 and Q7)
	vshrn.i32 d12, q6,  #8 
	vshrn.i32 d13, q7,  #8 
	vshrn.i32 d14, q8,  #8 
	vshrn.i32 d15, q13, #8 

	; Narrow 16 bit -> 8 bit + clamp to 0-255 range - result in Q14
	vqmovun.s16 d28, q6 
	vqmovun.s16 d29, q7 


	; +----------------------------------------------+
	; | g = (298 * c - 100 * d - 208 * e + 128) >> 8 |
	; +----------------------------------------------+

	; g = -100 * d
	vmull.s16 q6,   d22, d0[2] 
	vmull.s16 q7,   d23, d0[2] 
	vmull.s16 q8,   d24, d0[2] 
	vmull.s16 q13,  d25, d0[2] 

	; g -= 208 * e
	vmlsl.s16 q6,  d18, d0[3] 
	vmlsl.s16 q7,  d19, d0[3] 
	vmlsl.s16 q8,  d20, d0[3] 
	vmlsl.s16 q13, d21, d0[3] 

	; g += c'
	vadd.i32 q6,  q6,  q2 
	vadd.i32 q7,  q7,  q3 
	vadd.i32 q8,  q8,  q4 
	vadd.i32 q13, q13, q5 

	; g >>= 8 (narrowed 32 bit -> 16 bit, results stored in Q6 and Q7) 
	vshrn.i32 d12, q6,  #8 
	vshrn.i32 d13, q7,  #8 
	vshrn.i32 d14, q8,  #8 
	vshrn.i32 d15, q13, #8 

	; Narrow 16 bit -> 8 bit + clamp to 0-255 range - result in Q11
	vqmovun.s16 d22, q6 
	vqmovun.s16 d23, q7 


	; +------------------------------------+
	; | r = (298 * c + 409 * e + 128) >> 8 |
	; +------------------------------------+

	; r = 409 * e, long
	vmull.s16 q6,  d18, d1[0] 
	vmull.s16 q7,  d19, d1[0] 
	vmull.s16 q8,  d20, d1[0] 
	vmull.s16 q13, d21, d1[0] 

	; r += c'
	vadd.i32 q6,  q6,  q2 
	vadd.i32 q7,  q7,  q3 
	vadd.i32 q8,  q8,  q4 
	vadd.i32 q13, q13, q5 

	; r >>= 8 (narrowed 32 bit -> 16 bit, results stored in Q6 and Q7) 
	vshrn.i32 d12, q6,  #8 
	vshrn.i32 d13, q7,  #8 
	vshrn.i32 d14, q8,  #8 
	vshrn.i32 d15, q13, #8 

	; Narrow 16 bit -> 8 bit + clamp to 0-255 range - result in Q12
	vqmovun.s16 d24, q6 
	vqmovun.s16 d25, q7 


	; +-------------------+
	; | Prepare for store |
	; +-------------------+

	; Set Q13 = alpha = 255
	mov.w r3, #255 
	vdup.8 q13, r3 

	; Copy r to Q10
	vorr q10, q12, q12 

	; Copy b to Q12
	vorr q12, q14, q14  


	; +-----------------------------------------------------+
	; | Now we have R=Q10, G=Q11, B=Q12, A=Q13 - store them |
	; +-----------------------------------------------------+

	; Interleaved store (with pointer increment)
	vst4.8 { d20, d22, d24, d26 }, [r6]! 
	vst4.8 { d21, d23, d25, d27 }, [r6]! 


	; +-----------+
	; | Main loop |
	; +-----------+

2 
	cmp r6, r8 
	blt %B1 


	; +--------+
	; | Output |
	; +--------+
	;: ; No output


	; +-------+
	; | Input |
	; +-------+
	;: [pYRow]r(pYRow), [pUVRow]r(pUVRow), [pRgbRow]r(pRgbRow), [nYWidthDivisibleByThirtyTwo]r(nYWidthDivisibleByThirtyTwo)

	; +---------+
	; | Clobber |
	; +---------+
	;: memory, r3, r4, r5, r6, r8, q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15

    vpop {q4,q5,q6,q7}
	pop {r4,r5,r6,r7,r8}
	bx	lr
	
;-------------------------------------------------------------------------------------------------	
    END