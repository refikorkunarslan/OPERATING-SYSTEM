 #include <stdio.h>
int main()
{
   char prompt[]="OrkunShell >";
    char input_command[256];
     while(1)
     {
	 __asm__(
            "li $v0, 4\n"
            "la $a0, prompt\n"
            "syscall\n"
    	 );

       __asm__(
           " li $v0, 8\n"
            "la $a0, input_command\n"
            "syscall\n"
    	 );
        __asm__(
            "  li $v0, 20 \n"
            "la $a0, input_command\n"
            "syscall \n"
    	 );

      

     }
   
}
   

