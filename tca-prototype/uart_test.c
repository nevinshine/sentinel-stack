void _start() {
  volatile char *uart = (volatile char *)0x10000000;
  const char *str = "Hello from UART!\n";
  while (*str) {
    *uart = *str++;
  }
  while (1)
    ;
}
