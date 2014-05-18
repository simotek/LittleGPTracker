/* $NetBSD: gpmemcpy.S,v 1.3 1997/11/22 03:27:12 mark Exp $ */

/*-
* Copyright (c) 1997 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Neil A. Carson and Mark Brinicombe
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
* This product includes software developed by the NetBSD
* Foundation, Inc. and its contributors.
* 4. Neither the name of The NetBSD Foundation nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
* ``AS IS\'\' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

/* This was modified by Jay Monkman <jmonkman@smoothsmoothie.com> to
*   save and restore r12. This is necessary for RTEMS.
*/
/* #include <machine/asm.h>*/

#define ENTRY(_LABEL) \\
.global _LABEL; _LABEL:


.globl gpmemcpy
gpmemcpy:

stmfd sp!, {r0, r12, lr}
bl _gpmemcpy
ldmfd sp!, {r0, r12, pc}



.globl gpmemmove
gpmemmove:
stmfd sp!, {r0, r12, lr}
bl _gpmemcpy
ldmfd sp!, {r0, r12, pc}



/*
* This is one fun bit of code ...
* Some easy listening music is suggested while trying to understand this
* code e.g. Iron Maiden
*
* For anyone attempting to understand it :
*
* The core code is implemented here with simple stubs for gpmemcpy()
* gpmemmove() and bcopy().
*
* All local labels are prefixed with Lgpmemcpy_
* Following the prefix a label starting f is used in the forward copy code
* while a label using b is used in the backwards copy code
* The source and destination addresses determine whether a forward or
* backward copy is performed.
* Separate bits of code are used to deal with the following situations
* for both the forward and backwards copy.
* unaligned source address
* unaligned destination address
* Separate copy routines are used to produce an optimised result for each
* of these cases.
* The copy code will use LDM/STM instructions to copy up to 32 bytes at
* a time where possible.
*
* Note: r12 (aka ip) can be trashed during the function along with
* r0-r3 although r0-r2 have defined uses i.e. src, dest, len through out.
* Additional registers are preserved prior to use i.e. r4, r5 & lr
*
* Apologies for the state of the comments;-)
*/


/*
_gpmemcpy:
*/
.globl _gpmemcpy
_gpmemcpy:
/* Determine copy direction */
cmp r1, r0
bcc Lgpmemcpy_backwards

moveq r0, #0   /* Quick abort for len=0 */
moveq pc, lr

stmdb sp!, {r0, lr}  /* gpmemcpy() returns dest addr */
subs r2, r2, #4
blt Lgpmemcpy_fl4  /* less than 4 bytes */
ands r12, r0, #3
bne Lgpmemcpy_fdestul  /* oh unaligned destination addr */
ands r12, r1, #3
bne Lgpmemcpy_fsrcul  /* oh unaligned source addr */

Lgpmemcpy_ft8:
/* We have aligned source and destination */
subs r2, r2, #8
blt Lgpmemcpy_fl12  /* less than 12 bytes (4 from above) */
subs r2, r2, #0x14        
blt Lgpmemcpy_fl32  /* less than 32 bytes (12 from above) */
stmdb sp!, {r4}  /* borrow r4 */

/* blat 32 bytes at a time */
/* XXX for really big copies perhaps we should use more registers */
Lgpmemcpy_floop32:
ldmia r1!, {r3, r4, r12, lr}
stmia r0!, {r3, r4, r12, lr}
ldmia r1!, {r3, r4, r12, lr}
stmia r0!, {r3, r4, r12, lr}
subs r2, r2, #0x20        
bge Lgpmemcpy_floop32

cmn r2, #0x10
ldmgeia r1!, {r3, r4, r12, lr} /* blat a remaining 16 bytes */
stmgeia r0!, {r3, r4, r12, lr}
subge r2, r2, #0x10        
ldmia sp!, {r4}  /* return r4 */

Lgpmemcpy_fl32:
adds r2, r2, #0x14        

/* blat 12 bytes at a time */
Lgpmemcpy_floop12:
ldmgeia r1!, {r3, r12, lr}
stmgeia r0!, {r3, r12, lr}
subges r2, r2, #0x0c        
bge Lgpmemcpy_floop12

Lgpmemcpy_fl12:
adds r2, r2, #8
blt Lgpmemcpy_fl4

subs r2, r2, #4
ldrlt r3, [r1], #4
strlt r3, [r0], #4
ldmgeia r1!, {r3, r12}
stmgeia r0!, {r3, r12}
subge r2, r2, #4

Lgpmemcpy_fl4:
/* less than 4 bytes to go */
adds r2, r2, #4
ldmeqia sp!, {r0, pc}  /* done */

/* copy the crud byte at a time */
cmp r2, #2
ldrb r3, [r1], #1
strb r3, [r0], #1
ldrgeb r3, [r1], #1
strgeb r3, [r0], #1
ldrgtb r3, [r1], #1
strgtb r3, [r0], #1
ldmia sp!, {r0, pc}

/* erg - unaligned destination */
Lgpmemcpy_fdestul:
rsb r12, r12, #4
cmp r12, #2

/* align destination with byte copies */
ldrb r3, [r1], #1
strb r3, [r0], #1
ldrgeb r3, [r1], #1
strgeb r3, [r0], #1
ldrgtb r3, [r1], #1
strgtb r3, [r0], #1
subs r2, r2, r12
blt Lgpmemcpy_fl4  /* less the 4 bytes */

ands r12, r1, #3
beq Lgpmemcpy_ft8  /* we have an aligned source */

/* erg - unaligned source */
/* This is where it gets nasty ... */
Lgpmemcpy_fsrcul:
bic r1, r1, #3
ldr lr, [r1], #4
cmp r12, #2
bgt Lgpmemcpy_fsrcul3
beq Lgpmemcpy_fsrcul2
cmp r2, #0x0c            
blt Lgpmemcpy_fsrcul1loop4
sub r2, r2, #0x0c        
stmdb sp!, {r4, r5}

Lgpmemcpy_fsrcul1loop16:
mov r3, lr, lsr #8
ldmia r1!, {r4, r5, r12, lr}
orr r3, r3, r4, lsl #24
mov r4, r4, lsr #8
orr r4, r4, r5, lsl #24
mov r5, r5, lsr #8
orr r5, r5, r12, lsl #24
mov r12, r12, lsr #8
orr r12, r12, lr, lsl #24
stmia r0!, {r3-r5, r12}
subs r2, r2, #0x10        
bge Lgpmemcpy_fsrcul1loop16
ldmia sp!, {r4, r5}
adds r2, r2, #0x0c        
blt Lgpmemcpy_fsrcul1l4

Lgpmemcpy_fsrcul1loop4:
mov r12, lr, lsr #8
ldr lr, [r1], #4
orr r12, r12, lr, lsl #24
str r12, [r0], #4
subs r2, r2, #4
bge Lgpmemcpy_fsrcul1loop4

Lgpmemcpy_fsrcul1l4:
sub r1, r1, #3
b Lgpmemcpy_fl4

Lgpmemcpy_fsrcul2:
cmp r2, #0x0c            
blt Lgpmemcpy_fsrcul2loop4
sub r2, r2, #0x0c        
stmdb sp!, {r4, r5}

Lgpmemcpy_fsrcul2loop16:
mov r3, lr, lsr #16
ldmia r1!, {r4, r5, r12, lr}
orr r3, r3, r4, lsl #16
mov r4, r4, lsr #16
orr r4, r4, r5, lsl #16
mov r5, r5, lsr #16
orr r5, r5, r12, lsl #16
mov r12, r12, lsr #16
orr r12, r12, lr, lsl #16
stmia r0!, {r3-r5, r12}
subs r2, r2, #0x10        
bge Lgpmemcpy_fsrcul2loop16
ldmia sp!, {r4, r5}
adds r2, r2, #0x0c        
blt Lgpmemcpy_fsrcul2l4

Lgpmemcpy_fsrcul2loop4:
mov r12, lr, lsr #16
ldr lr, [r1], #4
orr r12, r12, lr, lsl #16
str r12, [r0], #4
subs r2, r2, #4
bge Lgpmemcpy_fsrcul2loop4

Lgpmemcpy_fsrcul2l4:
sub r1, r1, #2
b Lgpmemcpy_fl4

Lgpmemcpy_fsrcul3:
cmp r2, #0x0c            
blt Lgpmemcpy_fsrcul3loop4
sub r2, r2, #0x0c        
stmdb sp!, {r4, r5}

Lgpmemcpy_fsrcul3loop16:
mov r3, lr, lsr #24
ldmia r1!, {r4, r5, r12, lr}
orr r3, r3, r4, lsl #8
mov r4, r4, lsr #24
orr r4, r4, r5, lsl #8
mov r5, r5, lsr #24
orr r5, r5, r12, lsl #8
mov r12, r12, lsr #24
orr r12, r12, lr, lsl #8
stmia r0!, {r3-r5, r12}
subs r2, r2, #0x10        
bge Lgpmemcpy_fsrcul3loop16
ldmia sp!, {r4, r5}
adds r2, r2, #0x0c        
blt Lgpmemcpy_fsrcul3l4

Lgpmemcpy_fsrcul3loop4:
mov r12, lr, lsr #24
ldr lr, [r1], #4
orr r12, r12, lr, lsl #8
str r12, [r0], #4
subs r2, r2, #4
bge Lgpmemcpy_fsrcul3loop4

Lgpmemcpy_fsrcul3l4:
sub r1, r1, #1
b Lgpmemcpy_fl4

Lgpmemcpy_backwards:
add r1, r1, r2
add r0, r0, r2
subs r2, r2, #4
blt Lgpmemcpy_bl4  /* less than 4 bytes */
ands r12, r0, #3
bne Lgpmemcpy_bdestul  /* oh unaligned destination addr */
ands r12, r1, #3
bne Lgpmemcpy_bsrcul  /* oh unaligned source addr */

Lgpmemcpy_bt8:
/* We have aligned source and destination */
subs r2, r2, #8
blt Lgpmemcpy_bl12  /* less than 12 bytes (4 from above) */
stmdb sp!, {r4, lr}
subs r2, r2, #0x14  /* less than 32 bytes (12 from above) */
blt Lgpmemcpy_bl32

/* blat 32 bytes at a time */
/* XXX for really big copies perhaps we should use more registers */
Lgpmemcpy_bloop32:
ldmdb r1!, {r3, r4, r12, lr}
stmdb r0!, {r3, r4, r12, lr}
ldmdb r1!, {r3, r4, r12, lr}
stmdb r0!, {r3, r4, r12, lr}
subs r2, r2, #0x20        
bge Lgpmemcpy_bloop32

Lgpmemcpy_bl32:
cmn r2, #0x10            
ldmgedb r1!, {r3, r4, r12, lr} /* blat a remaining 16 bytes */
stmgedb r0!, {r3, r4, r12, lr}
subge r2, r2, #0x10        
adds r2, r2, #0x14        
ldmgedb r1!, {r3, r12, lr} /* blat a remaining 12 bytes */
stmgedb r0!, {r3, r12, lr}
subge r2, r2, #0x0c        
ldmia sp!, {r4, lr}

Lgpmemcpy_bl12:
adds r2, r2, #8
blt Lgpmemcpy_bl4
subs r2, r2, #4
ldrlt r3, [r1, #-4]!
strlt r3, [r0, #-4]!
ldmgedb r1!, {r3, r12}
stmgedb r0!, {r3, r12}
subge r2, r2, #4

Lgpmemcpy_bl4:
/* less than 4 bytes to go */
adds r2, r2, #4
moveq pc, lr   /* done */

/* copy the crud byte at a time */
cmp r2, #2
ldrb r3, [r1, #-1]!
strb r3, [r0, #-1]!
ldrgeb r3, [r1, #-1]!
strgeb r3, [r0, #-1]!
ldrgtb r3, [r1, #-1]!
strgtb r3, [r0, #-1]!
mov pc, lr

/* erg - unaligned destination */
Lgpmemcpy_bdestul:
cmp r12, #2

/* align destination with byte copies */
ldrb r3, [r1, #-1]!
strb r3, [r0, #-1]!
ldrgeb r3, [r1, #-1]!
strgeb r3, [r0, #-1]!
ldrgtb r3, [r1, #-1]!
strgtb r3, [r0, #-1]!
subs r2, r2, r12
blt Lgpmemcpy_bl4  /* less than 4 bytes to go */
ands r12, r1, #3
beq Lgpmemcpy_bt8  /* we have an aligned source */

/* erg - unaligned source */
/* This is where it gets nasty ... */
Lgpmemcpy_bsrcul:
bic r1, r1, #3
ldr r3, [r1, #0]
cmp r12, #2
blt Lgpmemcpy_bsrcul1
beq Lgpmemcpy_bsrcul2
cmp r2, #0x0c            
blt Lgpmemcpy_bsrcul3loop4
sub r2, r2, #0x0c        
stmdb sp!, {r4, r5, lr}

Lgpmemcpy_bsrcul3loop16:
mov lr, r3, lsl #8
ldmdb r1!, {r3-r5, r12}
orr lr, lr, r12, lsr #24
mov r12, r12, lsl #8
orr r12, r12, r5, lsr #24
mov r5, r5, lsl #8
orr r5, r5, r4, lsr #24
mov r4, r4, lsl #8
orr r4, r4, r3, lsr #24
stmdb r0!, {r4, r5, r12, lr}
subs r2, r2, #0x10        
bge Lgpmemcpy_bsrcul3loop16
ldmia sp!, {r4, r5, lr}
adds r2, r2, #0x0c        
blt Lgpmemcpy_bsrcul3l4

Lgpmemcpy_bsrcul3loop4:
mov r12, r3, lsl #8
ldr r3, [r1, #-4]!
orr r12, r12, r3, lsr #24
str r12, [r0, #-4]!
subs r2, r2, #4
bge Lgpmemcpy_bsrcul3loop4

Lgpmemcpy_bsrcul3l4:
add r1, r1, #3
b Lgpmemcpy_bl4

Lgpmemcpy_bsrcul2:
cmp r2, #0x0c            
blt Lgpmemcpy_bsrcul2loop4
sub r2, r2, #0x0c        
stmdb sp!, {r4, r5, lr}

Lgpmemcpy_bsrcul2loop16:
mov lr, r3, lsl #16
ldmdb r1!, {r3-r5, r12}
orr lr, lr, r12, lsr #16
mov r12, r12, lsl #16
orr r12, r12, r5, lsr #16
mov r5, r5, lsl #16
orr r5, r5, r4, lsr #16
mov r4, r4, lsl #16
orr r4, r4, r3, lsr #16
stmdb r0!, {r4, r5, r12, lr}
subs r2, r2, #0x10        
bge Lgpmemcpy_bsrcul2loop16
ldmia sp!, {r4, r5, lr}
adds r2, r2, #0x0c        
blt Lgpmemcpy_bsrcul2l4

Lgpmemcpy_bsrcul2loop4:
mov r12, r3, lsl #16
ldr r3, [r1, #-4]!
orr r12, r12, r3, lsr #16
str r12, [r0, #-4]!
subs r2, r2, #4
bge Lgpmemcpy_bsrcul2loop4

Lgpmemcpy_bsrcul2l4:
add r1, r1, #2
b Lgpmemcpy_bl4

Lgpmemcpy_bsrcul1:
cmp r2, #0x0c            
blt Lgpmemcpy_bsrcul1loop4
sub r2, r2, #0x0c        
stmdb sp!, {r4, r5, lr}

Lgpmemcpy_bsrcul1loop32:
mov lr, r3, lsl #24
ldmdb r1!, {r3-r5, r12}
orr lr, lr, r12, lsr #8
mov r12, r12, lsl #24
orr r12, r12, r5, lsr #8
mov r5, r5, lsl #24
orr r5, r5, r4, lsr #8
mov r4, r4, lsl #24
orr r4, r4, r3, lsr #8
stmdb r0!, {r4, r5, r12, lr}
subs r2, r2, #0x10        
bge Lgpmemcpy_bsrcul1loop32
ldmia sp!, {r4, r5, lr}
adds r2, r2, #0x0c        
blt Lgpmemcpy_bsrcul1l4

Lgpmemcpy_bsrcul1loop4:
mov r12, r3, lsl #24
ldr r3, [r1, #-4]!
orr r12, r12, r3, lsr #8
str r12, [r0, #-4]!
subs r2, r2, #4
bge Lgpmemcpy_bsrcul1loop4

Lgpmemcpy_bsrcul1l4:
add r1, r1, #1
b Lgpmemcpy_bl4
