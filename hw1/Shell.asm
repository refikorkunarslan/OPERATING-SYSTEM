.data

prompt:         .asciiz "OrkunShell > "
input_command:  .space 256
.text
.globl main

main:

    infinite_loop:

        type_prompt:    
            li $v0, 4
            la $a0, prompt
            syscall

        read_command:   
            li $v0, 8
            la $a0, input_command
            syscall

        run_command:
            li $v0, 20
            la $a0, input_command
            syscall
	
    j infinite_loop


