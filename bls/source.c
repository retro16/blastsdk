#include "bls.h"

src_t src[SRCCNT];
char groups[GRPCNT][NAMELEN]; // Group names
uint64_t groupmask; // Enabled groups

// Global CD boot sources. Treated specially.
char ipname[NAMELEN];
char spname[NAMELEN];
int ipsrc;
int spsrc;

size_t to_symname(char *output, const char *path);

int find_group(const char *name);
int new_group(const char *name);
uint64_t parse_groups(const char *list);
int src_overlap(int s);
int src_phys_overlap(int s);

// Symbol functions
int sym_set_ext(int source, const char *name);
int sym_is_ext(int source, const char *name);

// Source manipulation functions
void src_clear(int srcid); // Delete a source
void src_clear_sym(int srcid); // Delete all symbols of a source
int src_get(const char *name); // Get a source by file name
int src_get_sym(const char *name); // Get a source by symbol name
int src_add(const char *name); // Add a new source and return its index
void src_dirty_deps();
void src_map_addr(); // Compute address for all unallocated sources
void src_map_phys(); // Compute physaddr for all unallocated sources
void get_concat_list(const char *next, int *list); // Get concat list starting at source next
void src_physalign(int s, int ipsrcid); // Align a source on its physical medium
int src_depends(const char *extsym); // Returns the ID of the first source depending on extsym

size_t to_symname(char *output, const char *path)
{
  int sep;
  char *op = output;

  // Parse first character

  if(!path || !*path)
  {
    printf("Error : Empty path\n");
    exit(1);
  }
  if(*path >= '0' && *path <= '9')
  {
    printf("Warning : symbols cannot start with numbers, prepending underscore to [%s]\n", path);
    *op = '_';
    ++op;
    sep = 0;
  }

  if(*path >= 'a' && *path <= 'z')
  {
    *op = *path - 'a' + 'A';
    ++op;
    ++path;
    sep = 0;
  }
  else if(*path >= 'A' && *path <= 'Z')
  {
    *op = *path;
    ++op;
    ++path;
    sep = 0;
  }
  else
  {
    *op = '_';
    ++op;
    ++path;
    sep = 1;
  }

  while(*path)
  {
    if((*path >= '0' && *path <= '9') || (*path >= 'A' && *path <= 'Z') || *path == '_')
    {
      *op = *path;
      ++op;
      sep = 0;
    }
    else if(*path >= 'a' && *path <= 'z')
    {
      *op = *path - 'a' + 'A';
      ++op;
      sep = 0;
    }
    else
    {
      if(!sep)
      {
        *op = '_';
        ++op;
      }
      sep = 1;
    }
    ++path;
  }
  *op = '\0';

  return op - output;
}

int find_group(const char *name)
{
  int g;
  size_t size;
  size = strlen(name);
  if(size > NAMELEN)
  {
    size = NAMELEN;
  }
  for(g = 0; g < GRPCNT; ++g)
  {
    if(strncmp(name, groups[g], size) == 0)
    {
      return g;
    }
  }
  return -1;
}

int new_group(const char *name)
{
  int g;
  size_t size;
  size = strlen(name);
  if(size > NAMELEN)
  {
    size = NAMELEN;
  }
  for(g = 0; g < GRPCNT; ++g)
  {
    if(groups[g][0] == '\0')
    {
      // Empty slot found
      strncpy(groups[g], name, size);
      groupmask |= (1<<g);
      return g;
    }
  }
  return -1;
}

uint64_t parse_groups(const char *list)
{
  uint64_t r = 0;
  int g;
  char sym[1024];
  while(*list)
  {
    parse_sym(sym, &list);
    if(*list)
    {
      ++list;
    }
    if(!sym[0])
    {
      continue;
    }

    // Find a group slot for this name
    g = find_group(sym);
    if(g == -1)
    {
      g = new_group(sym);
    }
    if(g == -1)
    {
      return -1; // Could not parse groups
    }

    // Add to the variable
    r |= (1<<g);
  }
  return r;
}

int src_overlap(int s1)
{
  int s2;
  for(s2 = 0; s2 < SRCCNT; ++s2)
  {
    if(s1 == s2 || src[s2].type == type_none || src[s2].sym.value.addr == -1 || src[s2].binsize == -1)
    {
      continue;
    }
    if(src[s1].sym.value.chip == src[s2].sym.value.chip && (!groupmask || (src[s1].groups & src[s2].groups & groupmask)))
    {
      // Same chip, at least one common group. Check for overlapping addresses.
      sv_t st1, ed1, st2, ed2;
      st1 = src[s1].sym.value.addr;
      ed1 = src[s1].sym.value.addr + src[s1].binsize;
      st2 = src[s2].sym.value.addr;
      ed2 = src[s2].sym.value.addr + src[s2].binsize;

      if(!(ed2 <= st1 || st2 >= ed1))
      {
        return 1; // Same zone
      }
    }
  }

  return 0;
}

int src_phys_overlap(int s1)
{
  int s2;
  for(s2 = 0; s2 < SRCCNT; ++s2)
  {
    if(s1 == s2 || src[s2].type == type_none || src[s2].physaddr == -1 || src[s2].binsize == -1)
    {
      continue;
    }

    int st1, ed1, st2, ed2;
    st1 = src[s1].physaddr;
    ed1 = src[s1].physaddr + src[s1].binsize;
    st2 = src[s2].physaddr;
    ed2 = src[s2].physaddr + src[s2].binsize;

    if((st1 >= st2 && st1 < ed2) || (ed1 >= st2 && ed1 < ed2) || (st2 >= st1 && st2 < ed1))
    {
      return 1; // Same zone
    }
  }

  return 0;
}


int sym_get_value(chipaddr_t *out, const char *name)
{
  int i;
  int j;
  sym_t *s;

  out->chip = chip_none;
  out->addr = 0;

  char n[NAMELEN + 8];

  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].type == type_none)
    {
      continue;
    }

    strcpy(n, src[i].sym.name);
    j = strlen(src[i].sym.name);

#define CHECK_NAME(suffix) strcpy(&n[j], suffix) && strcmp(name, n) == 0
    if(src[i].sym.value.chip != chip_none && CHECK_NAME(""))
    {
      *out = src[i].sym.value;
      return 1;
    }
    if(src[i].binsize != -1 && CHECK_NAME("_SIZE"))
    {
      out->addr = src[i].binsize;
      return 1;
    }
    if(src[i].physaddr != -1)
    {
      if(scd)
      {
        if(CHECK_NAME("_SECT"))
        {
          out->addr = src[i].physaddr / CDBLOCKSIZE;
          return 1;
        }
        if(CHECK_NAME("_SOFF"))
        {
          out->addr = src[i].physaddr % CDBLOCKSIZE;
          return 1;
        }
      }
      if(CHECK_NAME("_ROM"))
      {
        if(!scd)
        {
          out->chip = pgmchip;
        }
        out->addr = src[i].physaddr;
        return 1;
      }
    }
#undef CHECK_NAME
    
    for(j = 0; j < SYMCNT; ++j)
    {
      s = &src[i].intsym[j];
      if(strcmp(name, s->name) != 0)
      {
        continue;
      }

      // Matching symbol
      *out = s->value;
      return 1;
    }
  }

  return 0;
}

busaddr_t sym_get_addr(const char *name, bus_t bus)
{
  chipaddr_t ca;
  if(!sym_get_value(&ca, name))
  {
    busaddr_t ba;
    ba.bus = bus_none;
    ba.addr = -1;
    ba.bank = -1;
    return ba;
  }
  return chip2bus(ca, bus);
}

int sym_set_int(int srcid, const char *name, busaddr_t value)
{
  int i;
  sym_t *s = src[srcid].intsym;
  for(i = 0; i < SYMCNT; ++i)
  {
    if(strcmp(name, s[i].name) == 0) {
      // Symbol already exists : update value
      goto sym_set_int_found;
    }
  }
  for(i = 0; i < SYMCNT; ++i)
  {
    if(s[i].name[0] < 0) {
      // Found a new slot to store this new symbol
      strcpy(s[i].name, name);
      goto sym_set_int_found;
    }
  }

  printf("Internal symbol table full for source %s. Reduce symbol count or recompile blsbuild with greater SYMCNT.\n", src[srcid].name);
  exit(1);
  return -1;

sym_set_int_found:
  if(value.bus == bus_none) {
    // No bus : simple value
    s[i].value.chip = chip_none;
    s[i].value.addr = value.addr;
  } else {
    // Translate pointer to chip address
    chipaddr_t newval = bus2chip(value);
    if(s[i].value.chip != chip_none && s[i].value.addr != newval.addr && src_depends(name) >= 0)
    {
      printf("Error: pointer symbol value %s in %s has changed : %X -> %X.\n", name, src[srcid].name, (u32)s[i].value.addr, (u32)newval.addr);
      exit(1);
    }

    s[i].value = newval;
  }
  return i;
}

int sym_set_int_chip(int srcid, const char *name, chipaddr_t value)
{
  int i;
  sym_t *s = src[srcid].intsym;
  for(i = 0; i < SYMCNT; ++i)
  {
    if(strcmp(name, s[i].name) == 0) {
      // Symbol already exists : update value
      goto sym_set_int_chip_found;
    }
  }
  for(i = 0; i < SYMCNT; ++i)
  {
    if(s[i].name[0] < 0) {
      // Found a new slot to store this new symbol
      strcpy(s[i].name, name);
      goto sym_set_int_chip_found;
    }
  }

  printf("Internal symbol table full for source %s. Reduce symbol count or recompile blsbuild with greater SYMCNT.\n", src[srcid].name);
  exit(1);
  return -1;

sym_set_int_chip_found:
  s[i].value.chip = value.chip;
  s[i].value.addr = value.addr;
  return i;
}

int sym_set_ext(int srcid, const char *name)
{
  int i;
  for(i = 0; i < SYMCNT; ++i)
  {
    char *s = src[srcid].extsym[i];
    if(strcmp(name, s) == 0) {
      return i;
    }
  }
  for(i = 0; i < SYMCNT; ++i)
  {
    char *s = src[srcid].extsym[i];
    if(s[0] < 0) {
      strcpy(s, name);
      return i;
    }
  }

  printf("External symbol table full for source %s. Reduce symbol count or recompile with greater SYMCNT.\n", src[srcid].name);
  exit(1);
}

int sym_is_int(int srcid, const char *name)
{
  int i;
  for(i = 0; i < SYMCNT; ++i)
  {
    char *s = src[srcid].intsym[i].name;
    if(strcmp(name, s) == 0) {
      return 1;
    }
  }

  return 0;
}

int sym_is_ext(int srcid, const char *name)
{
  int i;
  for(i = 0; i < SYMCNT; ++i)
  {
    char *s = src[srcid].extsym[i];
    if(strcmp(name, s) == 0) {
      return 1;
    }
  }

  return 0;
}

// Source manipulation functions

void src_clear(int srcid)
{
  memset(&src[srcid], (char)-1, sizeof(src[srcid]));
}

void src_clear_sym(int srcid)
{
  src_t *s = &(src[srcid]);
  memset(s->extsym, (char)-1, sizeof(s->extsym));
  memset(s->intsym, (char)-1, sizeof(s->intsym));

  char name[NAMELEN];
  int l = strlen(s->sym.name);
  memcpy(name, s->sym.name, l);
  if(s->sym.value.chip != chip_none)
  {
    name[l] = '\0';
    sym_set_int_chip(srcid, name, s->sym.value);
  }
  if(s->binsize >= 0)
  {
    strcpy(&name[l], "_SIZE");
    chipaddr_t a = {chip_none, s->binsize};
    sym_set_int_chip(srcid, name, a);
  }
  if(s->physaddr >= 0)
  {
    strcpy(&name[l], "_ROM");
    chipaddr_t a = {scd ? chip_none : pgmchip, s->physaddr};
    sym_set_int_chip(srcid, name, a);
    if(scd)
    {
      strcpy(&name[l], "_SECT");
      chipaddr_t sa = {chip_none, s->physaddr / 2048};
      sym_set_int_chip(srcid, name, sa);
      strcpy(&name[l], "_SOFF");
      chipaddr_t oa = {chip_none, s->physaddr % 2048};
      sym_set_int_chip(srcid, name, oa);

      int frame = (s->physaddr / 2048) % 75;
      strcpy(&name[l], "_FRAME");
      chipaddr_t fa = {chip_none, (frame / 10) * 0x10 + frame % 10}; // Convert frame to BCD
      sym_set_int_chip(srcid, name, fa);
    }
  }
}

int src_get(const char *name)
{
  int i;
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].type != type_none && strcmp(src[i].name, name) == 0)
    {
      return i;
    }
  }
  return -1;
}

int src_get_sym(const char *name)
{
  int i;
  int j;
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].type != type_none)
    {
      if(strcmp(src[i].sym.name, name) == 0)
      {
        return i;
      }
      for(j = 0; j < SYMCNT; ++j)
      {
        if(strcmp(src[i].intsym[j].name, name) == 0)
        {
          return i;
        }
      }
    }
  }
  return -1;
}

int src_add(const char *name)
{
  int i;
  if(src_get(name) >= 0)
  {
    printf("Warning : Duplicate source %s\n", name);
  }
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].type == type_none)
    {
      src_clear(i);
      strcpy(src[i].name, name);
      to_symname(src[i].sym.name, name);
      return i;
    }
  }
  return -1;
}

void src_dirty_deps()
{
  int i, k;
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].dirty)
    {
      // Recompile direct dependencies
      for(k = 0; k < SYMCNT; ++k)
      {
        if(k != i && src[i].extsym[k][0] != -1)
        {
          int d; // Dirty source
          d = src_get_sym(src[i].extsym[k]);
          if(d != -1)
            src[d].dirty = 1;
          // If not found : unresolved dependency
        }  
      }
    }
  }
}

void src_map_addr()
{
  int i;
  int s;

  int ipsrcid;
  int spsrcid;

  if(!scd) {
    // Map main at the beginning of the cart
    if((s = src_get_sym("MAIN")) != -1)
    {
      if(src[s].sym.value.chip != pgmchip)
      {
        printf("MAIN symbol not on cartridge : aborting.\n");
        exit(1);
      }
      if(src[s].sym.value.addr == -1)
      {
        // Map main right after header
        src[s].sym.value.addr = chip_start(pgmchip, bus_main, -1);
      }
    }
  }
  else
  {
    ipsrcid = src_get_sym("IP_MAIN");
    if(ipsrcid == -1)
    {
      printf("Error: IP_MAIN not found.\n");
      exit(1);
    }
    src_t *ip = &src[ipsrcid];
    if(ip->sym.value.chip != chip_ram)
    {
      printf("Error : IP not in RAM.\n");
      exit(1);
    }

    // Place IP after security code.
    ip->sym.value.addr = SECCODESIZE;


    spsrcid = src_get_sym("SP_MAIN");
    if(spsrcid == -1)
    {
      printf("Error: SP_MAIN not found.\n");
      exit(1);
    }
    src_t *sp = &src[spsrcid];
    if(sp->sym.value.chip != chip_pram)
    {
      printf("Error: SP not in program RAM.\n");
      exit(1);
    }

    // Place SP after its header.
    sp->sym.value.addr = 0x6000 + SPHEADERSIZE;
  }

  // Allocate unmapped objects with a first fit algorithm (TODO : improve this)
  for(s = 0; s < SRCCNT; ++s) if(src[s].type != type_none)
  {
    if(src[s].binsize == -1)
    {
      printf("Could not determine binary size of source %s\n", src[s].name);
      exit(1);
    }

    if(src[s].sym.value.chip == chip_none)
    {
      continue;
    }

    int concats[SRCCNT+1];
    int hasconcat = 0;
    if(src[s].sym.value.addr == -1 || src[s].mapped == -1)
    {
      // Detect source concatenations
      concats[0] = -1;
      if(src[s].concat[0] != (char)-1)
      {
        hasconcat = 1;
        // Get concats for this source
        get_concat_list(src[s].concat, concats);

        // Recompute binsize with concats
        int *c;
        for(c = concats; *c != -1; ++c)
        {
          if(src[*c].binsize == -1)
          {
            printf("Could not determine binary size of source %s\n", src[*c].name);
            exit(1);
          }
          src[s].binsize += src[*c].binsize;
        }
      }
    }

    int lowest = 0x1000000;
    if(src[s].sym.value.addr != -1)
    {
      lowest = src[s].sym.value.addr;
      goto addr_map_done;
    }

    // Try to allocate at the beginning of the chip
    src[s].sym.value.addr = chip_start(src[s].sym.value.chip, src[s].bus, src[s].bank);
    if(src[s].align > -1)
    {
      src[s].sym.value.addr = align_value(src[s].sym.value.addr, src[s].align);
    }
    else
    {
      chip_align(&src[s].sym.value);
    }

    if(src[s].sym.value.addr + src[s].binsize < chip_size(src[s].sym.value.chip, src[s].bus, src[s].bank) && !src_overlap(s))
    {
      // Allocation succedded
      lowest = src[s].sym.value.addr;
    }

    // Allocate unallocated object
    if(lowest == 0x1000000) for(i = 0; i < SRCCNT; ++i) if(i != s)
    {
      if(src[i].type != type_none && src[i].binsize >= 0 && src[i].sym.value.addr != -1 && src[i].sym.value.chip == src[s].sym.value.chip)
      {
        // Try placing s after i
        src[s].sym.value.addr = src[i].sym.value.addr + src[i].binsize;
        if(src[s].align > -1)
        {
          src[s].sym.value.addr = align_value(src[s].sym.value.addr, src[s].align);
        }
        else
        {
          chip_align(&src[s].sym.value);
        }
        if(src[s].sym.value.addr + src[s].binsize < chip_size(src[s].sym.value.chip, src[s].bus, src[s].bank) && !src_overlap(s))
        {
          // s does not overlap i and is inside chip space. Select this address as good
          if(src[s].sym.value.addr < lowest)
          {
            lowest = src[s].sym.value.addr;
          }
        }
      }
    }

    if(lowest == 0x1000000)
    {
      printf("Could not allocate source %s : not enough room\n", src[s].name);
      exit(1);
    }
    src[s].sym.value.addr = lowest;

    // Map concatenated resources
addr_map_done:
    src[s].mapped = 1;
    if(hasconcat)
    {
      int *c;
      int a = lowest + src[s].binsize;
      for(c = concats; *c != -1; ++c);
      while(--c >= concats)
      {
        a -= src[*c].binsize;
        if(src[*c].sym.value.chip == src[s].sym.value.chip)
        {
          src[*c].sym.value.addr = a;
          src[*c].mapped = 1;
        }
        src[*c].concataddr.chip = src[s].sym.value.chip;
        src[*c].concataddr.addr = a;
      }
    }
  }
}

void src_map_phys()
{
  int s, i;
  int physsize = scd ? (CDBLOCKCNT * CDBLOCKSIZE) : chip_size(pgmchip, bus_main, -1);

  int ipsrcid;


  if(!scd)
  {
    // Map program sources to physical addresses
    for(s = 0; s < SRCCNT; ++s)
    {
      if(src[s].type != type_none && src[s].binsize > 0 && src[s].sym.value.chip == pgmchip)
      {
        src[s].physaddr = src[s].sym.value.addr;
      }
    }
  }
  else
  {
    // Get IP and SP sources (their presence is checked during address mapping)
    ipsrcid = src_get_sym("IP_MAIN");
    src_t *ip = &src[ipsrcid];
    src_t *sp = &src[src_get_sym("SP_MAIN")];

    // Leave room for security code and SP header
    ip->binsize += SECCODESIZE;
    sp->binsize += SPHEADERSIZE;
  }

  // Map remaining sources
  for(s = 0; s < SRCCNT; ++s)
  {
    if(src[s].type == type_none || src[s].binsize <= 0 || src[s].physaddr >= 0 || !src[s].store)
    {
      continue;
    }

    int lowest = 0x7FFFFFFF;

    // Try to map at the beginning of the image
    src[s].physaddr = scd ? CDHEADERSIZE : ROMHEADERSIZE;
    if(!src_phys_overlap(s))
    {
      lowest = src[s].physaddr;
    }

    // Allocate unallocated object
    if(lowest == 0x7FFFFFFF) for(i = 0; i < SRCCNT; ++i) if(i != s)
    {
      if(src[i].type == type_none || src[i].binsize <= 0 || src[i].physaddr == -1)
      {
        continue;
      }

      // Try placing s after i
      src[s].physaddr = src[i].physaddr + src[i].binsize;
      src_physalign(s, ipsrcid);

      if(src[s].physaddr + src[s].binsize < physsize && !src_phys_overlap(s))
      {
        // s does not overlap i and is inside chip space. Select this address as good
        if(src[s].physaddr < lowest)
        {
          lowest = src[s].physaddr;
          break;
        }
      }
    }

    if(lowest == 0x7FFFFFFF)
    {
      printf("Could not place source %s on physical medium : not enough room\n", src[s].name);
      exit(1);
    }
    src[s].physaddr = lowest;

    // Map concatenated objects after this one
    int concats[SRCCNT+1];
    concats[0] = -1;
    if(src[s].concat[0] != (char)-1)
    {
      // Get concats for this source
      get_concat_list(src[s].concat, concats);

      // Concatenate other sources
      int *c;
      int p = src[s].physaddr + src[s].binsize;
      for(c = concats; *c != -1; ++c);
      while(--c >= concats)
      {
        p -= src[*c].binsize;
        if(src[*c].physaddr > -1 && src[*c].physaddr != p)
        {
          printf("Error: concat of %s gave different logical and physical mapping : %X -> %X.\n", src[*c].name, src[*c].physaddr, p);
        }
        src[*c].physaddr = p;
      }
      printf("\n");
    }
  }

  if(scd)
  {
    // Get IP and SP sources (their presence is checked during address mapping)
    src_t *ip = &src[src_get_sym("IP_MAIN")];
    src_t *sp = &src[src_get_sym("SP_MAIN")];

    // Leave room for security code and SP header
    ip->binsize -= SECCODESIZE;
    ip->physaddr += SECCODESIZE;
    
    sp->binsize -= SPHEADERSIZE;
    sp->physaddr += SPHEADERSIZE;
  }
}

void get_concat_list(const char *next, int *list)
{
  int s;
  for(s = 0; s < SRCCNT; ++s)
  {
    if(src[s].type == type_none || src[s].binsize <= 0)
    {
      continue;
    }
    if(strcmp(src[s].name, next) != 0)
    {
      continue;
    }

    list[0] = s;
    list[1] = -1;

    if(src[s].concat[0] != (char)-1)
    {
      // This source also has a concat : continue
      get_concat_list(src[s].concat, list + 1);
    }
  }
}

void src_physalign(int s, int ipsrcid)
{
  if(src[s].physalign == -1)
  {
    if(scd && s != ipsrcid)
    {
      // Align to CD-ROM block if spans more than one block
      if((src[s].physaddr) / CDBLOCKSIZE != (src[s].physaddr + src[s].binsize) / CDBLOCKSIZE)
      {
        src[s].physaddr = align_value(src[s].physaddr, CDBLOCKSIZE);
      }
    }
    else
    {
      // Align to word boundary
      if(src[s].physaddr & 1)
      {
        ++src[s].physaddr;
      }
    }
  }
  else
  {
    src[s].physaddr = align_value(src[s].physaddr, src[s].physalign);
  }
}

int src_depends(const char *extsym)
{
  int s;
  for(s = 0; s < SRCCNT; ++s)
  {
    if(src[s].type != type_none)
    {
      int e;
      for(e = 0; e < SYMCNT; ++e)
      {
        if(strcmp(extsym, src[s].extsym[e]) == 0)
        {
          return s;
        }
      }
    }
  }
  return -1;
}
