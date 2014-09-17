#include <bdp.h>
#include <bda.h>

void main()
{
  VDPSECURITY(); // Unlock VDP if necessary
  BDA_INIT(); // Initialize the debugger agent
  for(;;) {
    BDP_WRITE("Hello C world\n", 14); // Send data to BDB
    ENTER_MONITOR(); // Go to monitor mode to wait for debug commands
  }
}
