
.align 4

memcpy_neon:
		stmfd 			sp!, {r4-r12,lr}
		mov				lr, r0
		@---------------------
		@eor				r4, r4, r4
		@---------------------

		movs			r3, r2, lsr #8
		beq				cpy.128				
cpy.256:
		@---------------------
		@add				r4, r4, #256
		@---------------------
		vld1.64			{D0, D1, D2, D3}, [r1]!
		vld1.64			{D4, D5, D6, D7}, [r1]!
		vld1.64			{D8, D9, D10, D11}, [r1]!
		vld1.64			{D12, D13, D14, D15}, [r1]!
		vld1.64			{D16, D17, D18, D19}, [r1]!
		vld1.64			{D20, D21, D22, D23}, [r1]!
		vld1.64			{D24, D25, D26, D27}, [r1]!
		vld1.64			{D28, D29, D30, D31}, [r1]!
		
		vst1.64			{D0, D1, D2, D3}, [lr]!
		vst1.64			{D4, D5, D6, D7}, [lr]!
		vst1.64			{D8, D9, D10, D11}, [lr]!
		vst1.64			{D12, D13, D14, D15}, [lr]!
		vst1.64			{D16, D17, D18, D19}, [lr]!
		vst1.64			{D20, D21, D22, D23}, [lr]!
		vst1.64			{D24, D25, D26, D27}, [lr]!
		vst1.64			{D28, D29, D30, D31}, [lr]!

		subs			r3, #1
		bne				cpy.256
		@-----
cpy.128:	
		ands			r3, r2, #0x080
		beq 			cpy.64
		@---------------------
		@add				r4, r4, #128
		@---------------------
		
		vld1.64			{D0, D1, D2, D3}, [r1]!
		vld1.64			{D4, D5, D6, D7}, [r1]!
		vld1.64			{D8, D9, D10, D11}, [r1]!
		vld1.64			{D12, D13, D14, D15}, [r1]!
		
		vst1.64			{D0, D1, D2, D3}, [lr]!
		vst1.64			{D4, D5, D6, D7}, [lr]!
		vst1.64			{D8, D9, D10, D11}, [lr]!
		vst1.64			{D12, D13, D14, D15}, [lr]!
		
cpy.64:
		ands			r3, r2, #0x040
		beq 			cpy.32
		@---------------------
		@add				r4, r4, #64
		@---------------------
		vld1.64			{D0, D1, D2, D3}, [r1]!
		vld1.64			{D4, D5, D6, D7}, [r1]!
		
		vst1.64			{D0, D1, D2, D3}, [lr]!
		vst1.64			{D4, D5, D6, D7}, [lr]!
		
cpy.32:
		ands			r3, r2, #0x020
		beq 			cpy.16	
		@---------------------
		@add				r4, r4, #32
		@---------------------
		
		vld1.64			{D0, D1, D2, D3}, [r1]!
		
		vst1.64			{D0, D1, D2, D3}, [lr]!
		
cpy.16:
		ands			r3, r2, #0x010
		beq 			cpy.8		
		@---------------------
		@add				r4, r4, #16
		@---------------------
		
		vld1.64			{D0, D1}, [r1]!
		
		vst1.64			{D0, D1}, [lr]!
		
cpy.8:
		ands			r3, r2, #0x08
		beq 			cpy.4	
		@---------------------
		@add				r4, r4, #8
		@---------------------
		
		vld1.64			{D0}, [r1]!
		
		vst1.64			{D0}, [lr]!
		
cpy.4:
		ands			r3, r2, #0x04
		beq 			cpy.2		
		@---------------------
		@add				r4, r4, #4
		@---------------------
		
		vld1.32			{D0[0]}, [r1]!
		
		vst1.32			{D0[0]}, [lr]!
		
cpy.2:
		ands			r3, r2, #0x02
		beq 			cpy.1
		@---------------------
		@add				r4, r4, #2
		@---------------------
		
		vld1.16			{D0[0]}, [r1]!
		
		vst1.16			{D0[0]}, [lr]!
			
cpy.1:	
		ands			r3, r2, #0x01
		beq 			endofcpy
		@---------------------
		@add				r4, r4, #1
		@---------------------	
		
		vld1.8			{D0[0]}, [r1]!
		
		vst1.8			{D0[0]}, [lr]!
		
		
endofcpy:	
	
		ldmfd			sp!, {r4-r12, pc}
		
	.global memcpy_neon









