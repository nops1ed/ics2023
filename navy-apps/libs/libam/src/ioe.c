#include <am.h>

bool ioe_init() {
  return true;
}

void ioe_read (int reg, void *buf) { 
  ioe_read(reg, buf);  
}
void ioe_write(int reg, void *buf) { 
  ioe_write(reg, buf);
}
