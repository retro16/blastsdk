#include <bdp.h>

void main()
{
  VDPSECURITY(); // Unlock VDP if necessary
  for(;;) {
    BDP_WRITE("Hello C world\n", 14); // Send data to BDB
    ENTER_MONITOR(); // Go to monitor mode to wait for debug commands
  }
}
