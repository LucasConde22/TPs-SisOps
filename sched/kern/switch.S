/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;

/*
	void context_switch(struct Trapframe *tf);

	struct Trapframe {
		struct PushRegs tf_regs; 4
		uint16_t tf_es; 6
		uint16_t tf_padding1; 8
		uint16_t tf_ds; 
		uint16_t tf_padding2;
		uint32_t tf_trapno;
	-- below here defined by x86 hardware
		uint32_t tf_err;
		uintptr_t tf_eip;
		uint16_t tf_cs;
		uint16_t tf_padding3;
		uint32_t tf_eflags;
	--below here only when crossing rings, such as from user to kernel
		uintptr_t tf_esp;
		uint16_t tf_ss;
		uint16_t tf_padding4;
	} __attribute__((packed));
*/
context_switch: 
	// ACLARACIÓN: No usamos 'popal' para que sea más legible lo que hace cada parte del código.

	mov 4(%esp), %eax // Cargo en eax el puntero al trapframe, los primeros 4 bytes son la dirección de retorno.
	mov %eax, %esp // Vuelvo a cargar el puntero a esp, pero esta vez el "puntero correcto". 
				   // eax lo usamos auxiliarmente, porque luego lo tenemos que restaurar.
	// Carga todo lo de los registros generales restaura edi, esi, ebp, ebx, edx, ecx y eax.
	pop %edi
	pop %esi
	pop %ebp
	add $4, %esp
	pop %ebx
	pop %edx
	pop %ecx
	pop %eax

	// Pero tambíen necesito restaurar es y ds, entonces:
	pop %es
	pop %ds

	add $8, %esp // Salto trapno y traperr porque son simplemente datos que no nos interesa restaurar.
	iret // Esta va a restautar lo que falta: cs, eip y esp.
spin:
	jmp spin
