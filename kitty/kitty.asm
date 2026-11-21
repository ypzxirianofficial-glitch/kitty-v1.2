.code

kitty_ntreadvirtualmemory PROC
	mov r10, rcx
	mov eax, 63
	syscall
	ret
kitty_ntreadvirtualmemory ENDP

kitty_ntwritevirtualmemory PROC
	mov r10, rcx
	mov eax, 58
	syscall
	ret
kitty_ntwritevirtualmemory ENDP

kitty_ntprotectvirtualmemory PROC
	mov r10, rcx
	mov eax, 50
	syscall
	ret
kitty_ntprotectvirtualmemory ENDP

kitty_ntopenprocess PROC
	mov r10, rcx
	mov eax, 38
	syscall
	ret
kitty_ntopenprocess ENDP

kitty_ntclose PROC
	mov r10, rcx
	mov eax, 15
	syscall
	ret
kitty_ntclose ENDP

END