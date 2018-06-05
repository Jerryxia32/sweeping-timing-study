  .text
  .global multi_tag_read
multi_tag_read:
  rdtscp
  movl  $8, %eax
delay2:
  dec %eax
  jnz delay2

  sfence
  retq
