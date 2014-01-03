#include "bls.h"

#include "scdboot_jp.c"
#include "scdboot_us.c"
#include "scdboot_eu.c"

FILE *imgout;

// Output functions
void out_uppadchar(const char *src, char padding, int targetsize); // Output upper-case padded string
void out_padchar(const char *src, char padding, int targetsize); // Output left-aligned string
void out_string(const char *src); // Output C string
void out_data(const unsigned char *src, int size); // Output raw data
void out_fill(char ch, int size); // Output repeated character
void out_hex(const char *src); // Decode hexadecimal and output resulting binary
void out_u32(unsigned int n); // Output unsigned 32 bits number
void out_u16(unsigned int n); // Output unsigned 16 bits number
void out_byte(unsigned char b); // Output single byte
int out_align(int blocksize, unsigned char padding); // Align to next block of blocksize bytes
int out_fillto(long target, unsigned char padding); // Pad with a single character until the given offset
int out_fill16to(long target, int padding); // Pad with a word until the given offset
void out_file(const char *filename); // Append another file to output
void out_cdheader(); // Output the Sega-CD header
void out_genheader(); // Output the genesis header
void out_ipcode();
void out_spcode();
void out_cdbinaries();
void out_genbinaries();
void out_cdimage(); // Output the whole CD image
void out_genimage(); // Output the whole genesis image
void fix_genimage(); // Fix checksum and ROM size fields in the header and enlarge to next power of two


// Output functions

void out_uppadchar(const char *src, char padding, int targetsize)
{
  while(*src && targetsize--) {
    fputc(toupper(*src), imgout);
    ++src;
  }
  if(targetsize < 0) return;
  while(targetsize--) {
    fputc(padding, imgout);
  }
}

void out_padchar(const char *src, char padding, int targetsize)
{
  while(*src && targetsize--) {
    fputc(*src, imgout);
    ++src;
  }
  if(targetsize < 0) return;
  while(targetsize--) {
    fputc(padding, imgout);
  }
}

void out_string(const char *src)
{
  fputs(src, imgout);
}

void out_data(const unsigned char *src, int size)
{
  fwrite(src, size, 1, imgout);
}

void out_fill(char ch, int size)
{
  while(size--) {
    fputc(ch, imgout);
  }
}

void out_hex(const char *src)
{
  char v;
  int p = 2;
  while(*src) {
    int c = *src;
    if(c >= '0' && c <= '9') {
      v <<= 4;
      v |= c - '0';
      if(!--p) {
        fputc(v, imgout);
        p = 2;
      }
    }
    else if(c >= 'A' && c <= 'F') {
      v <<= 4;
      v |= c - 'A' + 10;
      if(!--p) {
        fputc(v, imgout);
        p = 2;
      }
    }
    else if(c >= 'a' && c <= 'f') {
      v <<= 4;
      v |= c - 'a' + 10;
      if(!--p) {
        fputc(v, imgout);
        p = 2;
      }
    }
    ++src;
  }
}

void out_u32(unsigned int n)
{
  fputc((unsigned char)(n >> 24), imgout);
  fputc((unsigned char)(n >> 16), imgout);
  fputc((unsigned char)(n >> 8), imgout);
  fputc((unsigned char)(n), imgout);
}

void out_u16(unsigned int n)
{
  fputc((unsigned char)(n >> 8), imgout);
  fputc((unsigned char)(n), imgout);
}

void out_byte(unsigned char b)
{
  fputc(b, imgout);
}

int out_align(int blocksize, unsigned char padding)
{
  long cur = ftell(imgout);
  long target = (cur + blocksize - 1) / blocksize * blocksize;
  long padlen = target - cur;
  long c = padlen;
  while(c--) {
    fputc(padding, imgout);
  }
  return padlen;
}

int out_fillto(long target, unsigned char padding)
{
  long cur = ftell(imgout);
  if(target < cur) {
    printf("Cannot pad %02X to address %08lx from %08lx.\n", padding, target, cur);
    exit(1);
  }
  long padlen = target - cur;
  long c = padlen;
  while(c--) {
    fputc(padding, imgout);
  }
  return padlen;
}

int out_fill16to(long target, int padding)
{
  long cur = ftell(imgout);
  if(target < cur) {
    printf("Cannot pad %04X to address %08lx from %08lx.\n", padding, target, cur);
    exit(1);
  }
  long padlen = target - cur;
  if(padlen & 1) {
    printf("Cannot pad %04X to address %08lx from %08lx : odd padding length.\n", padding, target, cur);
    exit(1);
  }
  long c = padlen >> 1;
  while(c--) {
    fputc(padding >> 16, imgout);
    fputc(padding & 0xFF, imgout);
  }
  return padlen;
}

void out_file(const char *filename)
{
  FILE *i = fopen(filename, "rb");
  if(!i) {
    printf("Cannot open input file %s for reading\n", filename);
    exit(1);
  }

  char data[65536];
  while(!feof(i))
  {
    size_t s = fread(data, 1, 65536, i);
    if(!s) { return; }
    if(s != fwrite(data, 1, s, imgout)) {
      printf("Error while writing file %s to image (%lu)\n", filename, s);
      exit(1);
    }
  }

  fclose(i);
}

void out_cdheader()
{
  // Get IP and SP sources (their presence is checked during address mapping)
  src_t *ip = &src[src_get_sym("IP_MAIN")];
  src_t *sp = &src[src_get_sym("SP_MAIN")];
  
  // Identifier
  out_string("SEGADISCSYSTEM  ");

  // Title
  out_uppadchar(title, ' ', 11);
  out_hex("00 00 00 00 01");

  // Author
  out_uppadchar(author, ' ', 11);
  out_hex("00 00 00 00 00");

  // Main 68k program - loaded at 0xFF0000
  if(ip->physaddr - SECCODESIZE != 0x200)
  {
    printf("Error: Sega CD IP misplaced : Placed at 0x%X, should be 0x200.\n", ip->physaddr - SECCODESIZE);
    exit(1);
  }
  int end = align_value(ip->physaddr + ip->binsize, CDBLOCKSIZE);

  if(end <= 0x800)
  {
    // Small IP : fits in the first sector
    out_u32(0x200); // offset
    out_u32(0x600); // size
  }
  else
  {
    // Big IP : needs more than one sector
    out_u32(0x800);
    out_u32(end - 0x800);
  }

  out_u32(0); // End of load chain
  out_u32(0);

  // Sub 68k program - loaded at 0x006000
  out_u32(sp->physaddr - SPHEADERSIZE); // offset
  end = align_value(sp->physaddr + sp->binsize, CDBLOCKSIZE);
  out_u32((end - sp->physaddr) + SPHEADERSIZE); // size
  out_u32(0); // End of load chain
  out_u32(0);

  // Output mastering date
  out_uppadchar(masterdate, ' ', 16);
  out_fill(' ', 160);

  out_padchar("SEGA CD ", ' ', 16);
  out_padchar(copyright, ' ', 16);
  out_padchar(title, ' ', 48);
  out_padchar(title, ' ', 48);
  out_padchar(serial, ' ', 16);
  out_padchar("J", ' ', 16); // Joystick type
  out_fill(' ', 80);
  out_byte(*region);
  out_fill(' ', 15);
}

void out_ipheader()
{
  // Output security code
  switch(*region)
  {
    case 'J':
      out_data(scdboot_jp, sizeof(scdboot_jp));
      // Pad with a bra instruction and 0xFF padding
      out_u16(0x6000); // bra.w
      out_u16(SECCODESIZE - sizeof(scdboot_jp) - 2);
      out_fill((char)0xFF, SECCODESIZE - sizeof(scdboot_jp) - 4);
      break;
    case 'U':
      out_data(scdboot_us, sizeof(scdboot_us));
      break;
    case 'E':
      out_data(scdboot_eu, sizeof(scdboot_eu));
      // Pad with a bra instruction and 0xFF padding
      out_u16(0x6000); // bra.w
      out_u16(SECCODESIZE - sizeof(scdboot_eu) - 2);
      out_fill((char)0xFF, SECCODESIZE - sizeof(scdboot_eu) - 4);
      break;
  }
}

void out_spheader()
{
  src_t *sp = &src[src_get_sym("SP_MAIN")];

  // 0x00 Program name
  out_padchar("MAIN", ' ', 11);
  out_byte(0);

  // 0x0C Version
  out_u16(0);

  // 0x0E Type
  out_u16(0);
  
  // 0x10 Next header
  out_u32(0);

  // 0x14 Length of packet
  out_u32(sp->binsize + SPHEADERSIZE);

  // 0x18 Entry point table start
  out_u32(0x20);
  out_u32(0x20);

  // 0x20 Entry points
  
  busaddr_t spinit = sym_get_addr("SP_INIT", bus_sub);
  busaddr_t spmain = sym_get_addr("SP_MAIN", bus_sub);
  busaddr_t level2 = sym_get_addr("INT_SUB_LEVEL2", bus_sub);

  if(spinit.bus == bus_sub)
  {
    if(src_get_sym("SP_INIT") != src_get_sym("SP_MAIN"))
    {
      printf("Error: SP_INIT not in the same source as SP_MAIN.\n");
      exit(1);
    }
  }
  else
  {
    // No init entry point. Use main.
    spinit = spmain;
  }
  out_u16(spinit.addr - 0x6020);
  out_u16(spmain.addr - 0x6020);

  if(level2.bus == bus_sub)
  {
    out_u16(level2.addr - 0x6020);
  }
  else
  {
    out_u16(0);
  }

  out_u16(0);

  // 0x24 End of SP header
}

void out_genheader()
{
  busaddr_t v; // Temporary value for vector
  busaddr_t sp; // Stack pointer segment start
  chipaddr_t spsize; // Stack pointer segment size
  busaddr_t pc; // Main entry point
  busaddr_t interrupt; // Default interrupt entry point
  int i;

  sp = sym_get_addr("MAINSTACK", bus_main);
  if(sp.addr == -1) {
    sp.addr = 0;
  } else {
    if(!sym_get_value(&spsize, "MAINSTACK_SIZE") || spsize.addr == -1) {
      printf("MAINSTACK_SIZE not found : using 000000 as default stack pointer.\n");
      sp.addr = 0;
    } else {
      sp.addr = (sp.addr + spsize.addr) & 0xFFFFFF;
    }
  }

  pc = sym_get_addr("MAIN", bus_main);
  if(pc.addr == -1) {
    pc = sym_get_addr("MAIN_ASM", bus_main);
    if(pc.addr == -1) {
      printf("Warning: Could not find MAIN entry point, using "STRINGIFY(GENHEADERLEN)" as default entry point. Your image will probably crash at startup.\n");
      pc.addr = ROMHEADERSIZE;
    }
  }

  interrupt = sym_get_addr("G_INTERRUPT", bus_main);
  if(interrupt.addr == -1) {
    interrupt = sym_get_addr("INTERRUPT", bus_main);
  }
  if(interrupt.addr == -1) {
    printf("Warning: Could not find G_INTERRUPT entry point, using MAIN(0x%06X) as default entry point. Unknown interrupts will reset the program with wrong SP value.\n", (u32)pc.addr);
    interrupt = pc;
  }

  printf("Initial SP : %06X\nInitial PC : %06X\n", (unsigned int)sp.addr, (unsigned int)pc.addr);
  out_u32(sp.addr); // 0x00
  out_u32(pc.addr); // 0x04
  
#define INTVECT(name) if((v = sym_get_addr("G_HWINT_"#name, bus_main)).addr != -1) out_u32(v.addr); else \
                      if((v = sym_get_addr("G_INT_"#name, bus_main)).addr != -1) out_u32(v.addr); else out_u32(interrupt.addr);
#define INTVECT2(name, name2) if((v = sym_get_addr("G_HWINT_"#name, bus_main)).addr != -1) out_u32(v.addr); else if((v = sym_get_addr("G_HWINT_"#name2, bus_main)).addr != -1) out_u32(v.addr); else \
                              if((v = sym_get_addr("G_INT_"#name, bus_main)).addr != -1) out_u32(v.addr); else if((v = sym_get_addr("G_INT_"#name2, bus_main)).addr != -1) out_u32(v.addr); else out_u32(interrupt.addr);
  INTVECT(BUSERR) // 0x08
  INTVECT(ADDRERR) // 0x0C
  INTVECT(ILL) // 0x10
  INTVECT(ZDIV) // 0x14
  INTVECT(CHK) // 0x18
  INTVECT(TRAPV) // 0x1C
  INTVECT(PRIV) // 0x20
  INTVECT(TRACE) // 0x24
  INTVECT(LINEA) // 0x28
  INTVECT(LINEF) // 0x2C
  for(i = 0; i < 12; ++i) out_u32(interrupt.addr); // 0x30 .. 0x060
  INTVECT(SPURIOUS) // 0x60
  INTVECT(LEVEL1) // 0x64
  INTVECT2(LEVEL2,PAD) // 0x68
  INTVECT(LEVEL3) // 0x6C
  INTVECT2(LEVEL4,HBLANK) // 0x70
  INTVECT(LEVEL5) // 0x74
  INTVECT2(LEVEL6,VBLANK) // 0x78
  INTVECT(LEVEL7) // 0x7C
  INTVECT(TRAP00) // 0x80
  INTVECT(TRAP01)
  INTVECT(TRAP02)
  INTVECT(TRAP03)
  INTVECT(TRAP04)
  INTVECT(TRAP05)
  INTVECT(TRAP06)
  INTVECT(TRAP07) // 0x9C
  INTVECT(TRAP08) // 0xA0
  INTVECT(TRAP09)
  INTVECT(TRAP10)
  INTVECT(TRAP11)
  INTVECT(TRAP12)
  INTVECT(TRAP13)
  INTVECT(TRAP14)
  INTVECT(TRAP15) // 0xBC
  for(i = 0; i < 16; ++i) out_u32(interrupt.addr);
#undef INTVECT
#undef INTVECT2

  // 0x100
  switch(pgmchip) {
    case chip_cart:
    default:
      out_padchar("SEGA MEGA DRIVE", ' ', 16);
      break;
    case chip_pram:
      out_padchar("PRAM MEGA DRIVE", ' ', 16);
      break;
    case chip_wram:
      out_padchar("WRAM MEGA DRIVE", ' ', 16);
      break;
    case chip_ram:
      out_padchar("MRAM MEGA DRIVE", ' ', 16);
      break;
  }
  out_padchar(copyright, ' ', 16);
  out_padchar(title, ' ', 48);
  out_padchar(title, ' ', 48);
  out_padchar(serial, ' ', 14);
  out_u16(0); // Checksum - will be fixed by fix_genimage()
  out_padchar("J", ' ', 16); // Joystick type
  out_u32(0); // ROM start address
  out_u32(0); // ROM end address - will be fixed by fix_genimage()
  out_fill(' ', 72);
  out_byte(*region);
  out_fill(' ', 15);

  // 0x200
}

void out_binaries()
{
  int i, j;
  int s = 0;
  int minaddr = -1;
  int maxaddr;
  int lastaddr;
  int headeroffset;

  if(pgmchip == chip_cart)
  {
    headeroffset = 0;
  }
  else
  {
    headeroffset = ROMHEADERSIZE;
  }

  src_t *ip = NULL;
  src_t *sp = NULL;

  if(scd)
  {
    // Adjust IP and SP
    ip = &src[src_get_sym("IP_MAIN")];
    sp = &src[src_get_sym("SP_MAIN")];

    ip->binsize += SECCODESIZE;
    ip->physaddr -= SECCODESIZE;

    sp->binsize += SPHEADERSIZE;
    sp->physaddr -= SPHEADERSIZE;
  }


  for(j = 0; j < SRCCNT; ++j)
  {
    if(src[j].type == type_none || src[j].physaddr == -1 || src[j].binsize == 0 || !src[j].store)
    {
      continue;
    }

    maxaddr = 0x7FFFFFFF;
    for(i = 0; i < SRCCNT; ++i)
    {
      if(src[i].type != type_none && src[i].physaddr != -1 && src[i].binsize != 0 && src[i].physaddr < maxaddr && src[i].physaddr > minaddr && src[i].store)
      {
        s = i;
        maxaddr = src[i].physaddr;
      }
    }
    minaddr = src[s].physaddr;
    printf("%s : %06X -> %06X\n", src[s].name, src[s].physaddr + headeroffset, (unsigned int)(src[s].physaddr + headeroffset + src[s].binsize - 1));
    out_fillto(minaddr + headeroffset, (char)-1);

    if(&src[s] == ip)
    {
      // Output IP header (security code)
      out_ipheader();
    }
    else if(&src[s] == sp)
    {
      // Output SP header
      out_spheader();
    }
    if(src[s].binname[0])
    {
      out_file(src[s].binname);
    }
    lastaddr = src[s].physaddr + src[s].binsize;
  }

  out_fillto(lastaddr + headeroffset, (char)-1);
}

void out_genimage()
{
  int i;
  printf("\n----------\nPass 1 : compute object sizes and test if changed\n");
  if(compute_size_all()) // Compile one time to get binary sizes and check for file updates
  {
    printf("\n----------\nPass 2 : map dependencies\n");
    src_dirty_deps();
    src_map_addr();
    src_map_phys();

    printf("Mapping :\n            source      size  chip       addr             phys\n\n");
    for(i = 0; i < SRCCNT; ++i)
    {
      if(src[i].type != type_none)
      {
        printf("  %16s  %08X", src[i].name, (unsigned int)src[i].binsize);
        if(src[i].sym.value.addr != -1)
          printf("  %4s  %06X-%06X", chip_name(src[i].sym.value.chip), (unsigned int)src[i].sym.value.addr, (unsigned int)(src[i].sym.value.addr + src[i].binsize - 1));
        if(src[i].store)
          printf("  %08X-%08X", (unsigned int)src[i].physaddr, (unsigned int)(src[i].physaddr + src[i].binsize - 1));
        else
          printf("                  ");
        printf("\n");
      }
    }
    printf("\n----------\nPass 3 : compile with correct offsets\n");
    compile_all(); // Compile a second time with final memory mapping
    printf("\n----------\nPass 4 : link\n");
    link_all(); // Link binaries together with final object addresses
    printf("\n----------\nPass 5 : output file\n");
    imgout = fopen(output, "w+b");
    if(!imgout) {
      printf("Could not open image file for writing.\n");
      exit(1);
    }
    if(scd == 1 || scd == 2)
    {
      out_cdheader();
    }
    else
    {
      out_genheader();
    }
    out_binaries();
    if(scd)
    {
      if(ftell(imgout) < 0x96000)
      {
        // Pad to 600k to workaround the BIOS 1 reading bug and comply to CD-ROM minimum track length
        out_fillto(0x96000, 0);
      }
      if(ftell(imgout) % 2048)
      {
        // Pad to next 2k sector
        out_fillto((ftell(imgout) / 2048 + 1) * 2048, 0);
      }
    }
    else
    {
      fix_genimage();
    }
    fclose(imgout);
  }
  printf("\n----------\nblsbuild finished.\n");
}

void fix_genimage()
{
  int checksum = 0;
  int c;
  int size;
  int minsize;

  // Compute ROM size
  fseek(imgout, 0, SEEK_END);
  minsize = ftell(imgout);
  size = nextpoweroftwo(minsize);
  if(size < 0x200 || size > 0x400000) {
    printf("Invalid image size\n");
    exit(1);
  }
 
  // Compute checksum
  fseek(imgout, 0x200, SEEK_SET);
  while((c = fgetc(imgout)) != EOF) {
    checksum += c;
  }
  // Fix checksum
  fseek(imgout, 0x18E, SEEK_SET);
  out_u16(checksum & 0xFFFF);

  // Fix ROM end address
  fseek(imgout, 0x1A4, SEEK_SET);
  out_u32(size - 1);

  // Pad image to next power of two
/*  fseek(imgout, 0, SEEK_END);
  out_fill((char)-1, size - minsize);*/

  printf("Fix ROM : checksum = %04X, size = %06X -> %06X\n", checksum & 0xFFFF, minsize, size);
}
