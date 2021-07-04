  .data
prime:
  .asciiz "prime "
space:
  .asciiz "\n"
  .text
  .globl ShowPrimes
ShowPrimes:
  li $s0, 2 
  li $s1, 1000
  li $s2, 0 
loop:

  addi $a0, $s0, 0



  
  jal test_prime


  li $v0, 1
 addi $a0, $s0, 0
  syscall


  li $v0, 4
  la $a0, space
  syscall
 addi $s2, $s2, 1
  addi $s0, $s0, 1 
  beqz $v0, loop 
  addi $s3, $v0, 0

  

  
 
bgt $s1, $s2, loop
 
j main


test_prime:
	li $t0, 2
test_loop:
 
  beq $t0, $a0, test_exit_true 
  div $a0, $t0
  mfhi $t1 
  addi $t0, $t0, 1
  bnez $t1, test_loop

  
  addi $v0, $zero, 0
  jr $ra
test_exit_true:
 li $v0, 4
  la $a0, prime
  syscall
  addi $v0, $zero, 1
  jr $ra
