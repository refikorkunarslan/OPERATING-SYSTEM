.data

     text:  .asciiz "Enter a number: "
     factor: .asciiz "factors of "
     is: .asciiz " is "
     comma: .asciiz ","
.text

.globl Factorize

 Factorize:
    # Printing out the text
    li $v0, 4
    la $a0, text
    syscall

    # Getting user input
    li $v0, 5
    syscall


    # Moving the integer input to another register
    move $t2, $v0

      li $v0, 4
    la $a0, factor
    syscall

      # Printing out the number
    li $v0, 1
    move $a0, $t2
    syscall


      li $v0, 4
    la $a0, is
    syscall



    addi $t0, $zero, 1

    jal Loop
   
     # Printing out the number
    li $v0, 1
    move $a0, $t2
    syscall


  
    j main

Loop:
div $t2, $t0
mfhi $t3
beq $t3, $zero, print
addi $t0, $t0, 1
slt $t1, $t0, $t2
bne $t1, $zero, Loop
jr $ra

print :
  


     # Printing out the number
    li $v0, 1
    move $a0, $t0
    syscall
      li $v0, 4
    la $a0, comma
    syscall
    addi $t0, $t0, 1

    j Loop
