.global get_context
get_context:
    
  # Save the return address and stack pointer.
  movq (%rsp), %r8
  movq %r8, 8*0(%rdi)
  leaq 8(%rsp), %r8
  movq %r8, 8*1(%rdi)

  # Save preserved registers.
  movq %rbx, 8*2(%rdi)
  movq %rbp, 8*3(%rdi)
  movq %r12, 8*4(%rdi)
  movq %r13, 8*5(%rdi)
  movq %r14, 8*6(%rdi)
  movq %r15, 8*7(%rdi)

  # Return.
  xorl %eax, %eax
  ret


.global set_context
set_context:

    #we should return to the address set with {get, swap} context
    movq 8*0(%rdi), %r8


    #load the new stack pointer
    movq 8*1(%rdi), %rsp

    #load preserved registers
    movq 8*2(%rdi), %rbx
    movq 8*3(%rdi), %rbp
    movq 8*4(%rdi), %r12
    movq 8*5(%rdi), %r13
    movq 8*6(%rdi), %r14
    movq 8*7(%rdi), %r15

    #push RIP to stack for RET
    pushq %r8

    #finally return
    xorl %eax, %eax
    ret
    

.globl swap_context
swap_context:
    # save return address
    movq (%rsp), %r8
    movq %r8, 8*0(%rdi)
    leaq 8(%rsp), %r8
    movq %r8, 8*1(%rdi)

    #save preserved registers
    movq %rbx, 8*2(%rdi)
    movq %rbp, 8*3(%rdi)
    movq %r12, 8*4(%rdi)
    movq %r13, 8*5(%rdi)
    movq %r14, 8*6(%rdi)
    movq %r15, 8*7(%rdi)

    #should return to the address set with get/swap_context
    movq 8*0(%rsi), %r8

    #load new stack pointer
    movq 8*1(%rsi), %rsp

    #load preserved registers
    movq 8*2(%rsi), %rbx
    movq 8*3(%rsi), %rbp
    movq 8*4(%rsi), %r12
    movq 8*5(%rsi), %r13
    movq 8*6(%rsi), %r14
    movq 8*7(%rsi), %r15

    #push RIP to stack for RET
    pushq %r8

    #return
    xorl %eax, %eax
    ret


