.section .apitable_trampolines, "ax"
.global BootloaderAPI_Trampolines
BootloaderAPI_Trampolines:

	BootloaderAPI_ErasePage_Trampoline:
		jmp BootloaderAPI_ErasePage
	BootloaderAPI_WritePage_Trampoline:
		jmp BootloaderAPI_WritePage
	BootloaderAPI_FillWord_Trampoline:
		jmp BootloaderAPI_FillWord
	BootloaderAPI_ReadSignature_Trampoline:
		jmp BootloaderAPI_ReadSignature
	BootloaderAPI_ReadFuse_Trampoline:
		jmp BootloaderAPI_ReadFuse
	BootloaderAPI_ReadLock_Trampoline:
		jmp BootloaderAPI_ReadLock
	BootloaderAPI_WriteLock_Trampoline:
		jmp BootloaderAPI_WriteLock
	BootloaderAPI_WriteData_Trampoline:
		jmp BootloaderAPI_WriteData
	BootloaderAPI_WriteUserSignature_Trampoline:
		jmp BootloaderAPI_WriteUserSignature
	BootloaderAPI_UNUSED3:
		ret
	BootloaderAPI_UNUSED4:
		ret
	BootloaderAPI_UNUSED5:
		ret



.section .apitable_jumptable, "ax"
.global BootloaderAPI_JumpTable
BootloaderAPI_JumpTable:

	rjmp BootloaderAPI_ErasePage_Trampoline
	rjmp BootloaderAPI_WritePage_Trampoline
	rjmp BootloaderAPI_FillWord_Trampoline
	rjmp BootloaderAPI_ReadSignature_Trampoline
	rjmp BootloaderAPI_ReadFuse_Trampoline
	rjmp BootloaderAPI_ReadLock_Trampoline
	rjmp BootloaderAPI_WriteLock_Trampoline
	rjmp BootloaderAPI_WriteData_Trampoline
	rjmp BootloaderAPI_WriteUserSignature_Trampoline
	rjmp BootloaderAPI_UNUSED3 ; UNUSED ENTRY 3
	rjmp BootloaderAPI_UNUSED4 ; UNUSED ENTRY 4
	rjmp BootloaderAPI_UNUSED5 ; UNUSED ENTRY 5



.section .apitable_signatures, "ax"
.global BootloaderAPI_Signatures
BootloaderAPI_Signatures:

	.long BOOT_START_ADDR ; Start address of the bootloader
	.word 0xCECA ; Signature
	.word 0xDECA ; Signature
