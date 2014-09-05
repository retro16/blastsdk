#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <stdlib.h>

typedef struct strip {
  struct strip *next;
  uint32_t offset;
  uint32_t size;
  uint32_t srcoffset;
#define LITTERAL_STRIP ((uint32_t)0xFFFFFFFF)
} strip;

// Returns 1 if strips cover the whole range specified by size, 0 otherwise. strips must be sorted by ascending offsets.
int lz68k_pack_complete(uint32_t size, const strip *strips)
{
  uint32_t current = 0;
  for(; strips; strips = strips->next) {
    if(strips->offset != current) return 0;
    current += strips->size;
  }

  return current == size;
}

void lz68k_out_number(uint32_t number, FILE *dst)
{
  if(number < 128) {
    char n = (char)number;
    fwrite(&n, 1, 1, dst);
    return;
  }

  char dn[2];
  dn[0] = (char)(number >> 8 | 0x80);
  dn[1] = (char)(number & 0xFF);
  fwrite(dn, 1, 2, dst);
}

void strips_dump(const strip *strips)
{
  uint32_t total = 0;
  printf("-----------------\n");
  for(; strips; strips = strips->next)
  {
    printf("%s strip %X-%X", strips->srcoffset == LITTERAL_STRIP ? "Litteral" : "Backcopy", strips->offset, strips->offset + strips->size);
    if(strips->srcoffset != LITTERAL_STRIP)
    {
      printf(" from %X", strips->srcoffset);
    }
    printf("\n");
    total += strips->size;
  }
  printf("\n Total bytes : %u\n", total);
  printf("-----------------\n");
}

void lz68k_pack_output(const char *src, FILE *dst, const strip *strips)
{
  strips_dump(strips);
  uint32_t offset = 0;
  int phase = 0; // 0 = litteral; 1 = backcopy
  uint32_t zll = 0;
  for(; strips; strips = strips->next)
  {
    if(strips->srcoffset == LITTERAL_STRIP)
    {
      if(phase == 1)
      {
        fwrite("\0\0", 1, 2, dst); // Zero-length backcopy
        phase = 0;
//        printf("Zero-length backcopy\n");
      }

      // Output litteral strip
      lz68k_out_number(strips->size, dst);
      fwrite(src + strips->offset, 1, strips->size, dst);
//      printf("Litteral %X-%X\n", strips->offset, strips->offset + strips->size);
    }
    else
    {
      if(phase == 0)
      {
        fwrite("\0", 1, 1, dst); // Zero-length litteral
        phase = 1;
        zll++;
//        printf("Zero-length litteral\n");
      }

      // Output backcopy info
      lz68k_out_number(strips->offset - strips->srcoffset, dst);
      lz68k_out_number(strips->size, dst);
//      printf("Backcopy %X-%X from %X-%X\n", strips->offset, strips->offset + strips->size, strips->srcoffset, strips->srcoffset + strips->size);
    }

    if(offset != strips->offset)
    {
      printf("Strip list not contiguous : offset should be %X instead of %X\n", offset, strips->offset);
      exit(1);
    }
    offset += strips->size;
    phase = 1 - phase;
  }

  if(phase == 1)
  {
    fwrite("\0", 1, 1, dst); // Zero-length litteral
  }

  fwrite("\0", 1, 1, dst); // Zero-offset backcopy : end of stream

  printf("%u zll\n", zll);
}

int lz68k_strips_cover(uint32_t offset, uint32_t size, const strip *strips)
{
  if(!strips)
    return 0;

  // Seek at the beginning of the zone
  for(; strips && strips->offset + strips->size < offset; strips = strips->next);

  if(strips && strips->offset < offset + size)
  {
//    printf(" %X-%X covered by %X-%X\n", offset, offset + size, strips->offset, strips->offset + strips->size);
    return 1;
  }

  return 0;
}

void lz68k_insert_strip(const char *src, strip **strips, uint32_t offset, uint32_t srcoffset, uint32_t stripsize)
{
  strip *s = (strip *)malloc(sizeof(strip));
  s->size = stripsize;
  s->offset = offset;
  s->srcoffset = srcoffset;
  s->next = NULL;

  // Empty list
  if(!*strips)
  {
    *strips = s;
    return;
  }

  // First element
  if((*strips)->offset > offset)
  {
    s->next = *strips;
    *strips = s;
    return;
  }

  strip *prev = *strips;
  strip *c = prev->next;

  // Insert sorted by offset
  while(c)
  {
    if(c->offset > offset)
    {
      if(prev->offset + prev->size > offset || offset + stripsize > c->offset)
      {
        printf("Overlaps : %u(%u) %u(%u) %u(%u)\n", prev->offset, prev->size, offset, stripsize, c->offset, c->size);
      }
      prev->next = s;
      s->next = c;
      return;
    }

    prev = c;
    c = c->next;
  }

  // Insert at the end
  prev->next = s;
}

void lz68k_fill_holes(const char *src, uint32_t size, strip **strips)
{
  printf("Filling holes\n");

  // Fill holes with litterals
  strip *curstrip;
  uint32_t offset = 0;
  if(*strips)
  {
    for(curstrip = *strips; curstrip; curstrip = curstrip->next)
    {
      while(curstrip->offset > offset)
      {
        // Hole detected
        uint32_t soffset = offset; // Compute litteral offset
        uint32_t ssize = curstrip->offset - soffset;
        if(ssize > 32737)
        {
          // Cap litterals to 32737
          soffset = curstrip->offset - 32737;
          ssize = 32767;
        }

        strip *old = (strip *)malloc(sizeof(strip));
        memcpy(old, curstrip, sizeof(strip));
        curstrip->next = old;
        curstrip->offset = soffset;
        curstrip->size = ssize;
        curstrip->srcoffset = LITTERAL_STRIP;
      }
      offset = curstrip->offset + curstrip->size;
      if(!curstrip->next) break;
    }
    while(curstrip->offset + curstrip->size < size)
    {
      // Append a last litteral at the end
      strip *next = curstrip->next;
      curstrip->next = (strip *)malloc(sizeof(strip));
      curstrip->next->next = NULL;
      curstrip->next->offset = curstrip->offset + curstrip->size;
      curstrip->next->size = size - (curstrip->offset + curstrip->size);
      if(curstrip->next->size > 32767)
      {
        curstrip->next->offset = curstrip->offset + 32767;
        curstrip->next->size = 32767;
      }

      curstrip->next->srcoffset = LITTERAL_STRIP;

      curstrip = curstrip->next;
    }
  }
  else
  {
    // Could not compress at all !
    // Generate a list of litterals
    // TODO
  }

  printf("Strips finished\n********************\n");
}

/* the Rollsum struct type*/
typedef struct _lz68k_rollsum {
    unsigned long count;               /* count of bytes included in sum */
    unsigned long s1;                  /* s1 part of sum */
    unsigned long s2;                  /* s2 part of sum */
} lz68k_rollsum;

#define ROLLSUM_CHAR_OFFSET 31

#define lz68k_rollsum_init(sum) { \
      (sum).count=(sum).s1=(sum).s2=0; \
}

#define lz68k_rollsum_rotate(sum,out,in) { \
      (sum).s1 += (unsigned char)(in) - (unsigned char)(out); \
      (sum).s2 += (sum).s1 - (sum).count*((unsigned char)(out)+ROLLSUM_CHAR_OFFSET); \
}

#define lz68k_rollsum_rollin(sum,c) { \
      (sum).s1 += ((unsigned char)(c)+ROLLSUM_CHAR_OFFSET); \
      (sum).s2 += (sum).s1; \
      (sum).count++; \
}

#define lz68k_rollsum_rollout(sum,c) { \
      (sum).s1 -= ((unsigned char)(c)+ROLLSUM_CHAR_OFFSET); \
      (sum).s2 -= (sum).count*((unsigned char)(c)+ROLLSUM_CHAR_OFFSET); \
      (sum).count--; \
}

#define lz68k_rollsum_digest(sum) (((sum).s2 << 16) | ((sum).s1 & 0xffff))

#define lz68k_rollsum_compare(sum1, sum2) (lz68k_rollsum_digest(sum1) == lz68k_rollsum_digest(sum2))

// Finds all copy strips
void lz68k_gen_strip(const char *src, uint32_t size, strip **strips, uint32_t patsize)
{
  strip s;
  uint32_t matches = 0, longmatches = 0;

  if(size <= patsize)
  {
    return;
  }

  lz68k_rollsum dstsum;
  lz68k_rollsum srcsum;

  // Start scanning from the end of the buffer
  uint32_t dstoff = size;

  while(dstoff > patsize)
  {
    s.size = 0;
    uint32_t srcoff = dstoff;

    lz68k_rollsum_init(dstsum);
    lz68k_rollsum_init(srcsum);

    // Insert patsize chars in both sums
    uint32_t i;
    for(i = 0; i < patsize; ++i)
    {
      --dstoff;
      --srcoff;
      lz68k_rollsum_rollin(dstsum, src[dstoff]);
      lz68k_rollsum_rollin(srcsum, src[srcoff]);
    }

    // Scan src backwards
    while(srcoff != 0)
    {
      --srcoff;
      lz68k_rollsum_rollout(srcsum, src[srcoff + patsize]);
      lz68k_rollsum_rollin(srcsum, src[srcoff]);

      if(lz68k_rollsum_compare(dstsum, srcsum))
      {
        // Potential match
        uint32_t dcheck = dstoff + patsize - 1;
        uint32_t scheck = srcoff + patsize - 1;
        uint32_t matchsize = 1;
        for(;;)
        {
          if(!scheck || !dcheck) break;
          if(dstoff + patsize > 32767 && scheck < dstoff + patsize - 32767) break;
          if(src[dcheck] != src[scheck])
          {
            // Undo last cursor movement
            ++dcheck;
            ++scheck;
            --matchsize;
            break;
          }
          ++matchsize;
          --dcheck;
          --scheck;
        }
        if(matchsize > 32767)
        {
          printf("match size too large\n");
          exit(1);
        }
        if(matchsize >= patsize && matchsize > s.size)
        {

            // dcheck and scheck point at the first byte of the match strip
            if(matchsize != dstoff + patsize - dcheck)
            {
              printf("Error : matchsize = %u, dstoff + patsize - dcheck = %u, dcheck = %u\n",matchsize , dstoff + patsize - dcheck, dcheck); exit(1);
            }
          // Found a better match starting at scheck
//          if(s.size) printf("Improved match from %u to %u\n", s.size, matchsize);
          s.srcoffset = scheck;
          s.offset = dcheck;
          s.size = matchsize;
        }
        if(scheck < srcoff)
        {
          srcoff = scheck;
        }
      }
    }

    // Reached beginning : move dstoff and retry
    if(s.size)
    {
      ++matches;
      if(s.size > 15)
      {
        ++longmatches;
      }
      s.next = *strips;
      *strips = (strip *)malloc(sizeof(strip));
      memcpy(*strips, &s, sizeof(strip));
      dstoff -= s.size - patsize;
    }
    else
    {
      dstoff += patsize - 1;
    }
  }

//  strips_dump(*strips);
  printf("%u matches, %u long (%f)\n", matches, longmatches, (float)longmatches / (float)matches);
}

void lz68k_gen_strips(const char *src, uint32_t size, strip **strips)
{
//  lz68k_gen_strip(src, size, strips, 16);
  lz68k_gen_strip(src, size, strips, 4);
}

/*
// Generates strips for a given size
void lz68k_gen_strips_size(const char *src, uint32_t size, strip **strips, uint32_t stripsize, uint32_t step)
{
  printf("Searching for matches of size %u ", stripsize); fflush(stdout);
  if(stripsize >= size)
  {
    // Strips cannot be larger than the whole data
    printf("Too large\n");
    return;
  }

  // Generate hashes
  uint32_t hashes = (size - stripsize) / step;
  uint32_t *adler = (uint32_t *)malloc(sizeof(uint32_t) * hashes);
  uint32_t hash;
  for(hash = 0; hash < hashes; ++hash)
  {
    uint32_t offset = hash * step;
    adler[hash] = adler32(adler32(0, 0, 0), (unsigned char *)(src + offset), stripsize);
  }

  printf(": "); fflush(stdout);

  uint32_t matchcount = 0;

  for(hash = hashes - 1; hash > 0; --hash)
  {
    if(lz68k_strips_cover(hash * step, stripsize, *strips)) continue;

    uint32_t minsrchash = 0;
    if(hash * step > 32767)
    {
      minsrchash = hash - 32767 / step;
    }

    uint32_t srchash;
    // Search matching data
    for(srchash = hash - 1; srchash < hash; --srchash)
    {
      if(adler[srchash] == adler[hash] && memcmp(src + srchash * step, src + hash * step, stripsize) == 0)
      {
        // Data match
        uint32_t offset = hash * step;
        uint32_t srcoffset = srchash * step;

        ++matchcount;

//        printf("Match\n", srcoffset, offset, stripsize);
        lz68k_insert_strip(src, strips, offset, srcoffset, stripsize);
        break;
      }
    }
  }

  printf("%u\n", matchcount);

  free(adler);
}

void lz68k_gen_strips(const char *src, uint32_t size, strip **strips)
{
  // Find repeated data over the whole file

  uint32_t stripsize = 4096;

  for(; stripsize > 1024 && !lz68k_pack_complete(size, *strips); stripsize -= 256)
  {
    lz68k_gen_strips_size(src, size, strips, stripsize, 32);
  }

  for(; stripsize > 64 && !lz68k_pack_complete(size, *strips); stripsize -= 64)
  {
    lz68k_gen_strips_size(src, size, strips, stripsize, 8);
  }

  for(; stripsize > 16 && !lz68k_pack_complete(size, *strips); stripsize -= 8)
  {
    lz68k_gen_strips_size(src, size, strips, stripsize, 1);
  }

  for(; stripsize > 4 && !lz68k_pack_complete(size, *strips); stripsize -= 1)
  {
    lz68k_gen_strips_size(src, size, strips, stripsize, 1);
  }

  strips_dump(*strips);
}
*/

void lz68k_pack(const char *src, uint32_t size, FILE *dst)
{
  strip *strips = NULL;

  lz68k_gen_strips(src, size, &strips);
  lz68k_fill_holes(src, size, &strips);
  lz68k_pack_output(src, dst, strips);

  // Free strips
  while(strips)
  {
    strip *old = strips;
    strips = strips->next;
    free(old);
  }
}

uint32_t lz68k_unpack(const char *src, char *dst)
{
  uint32_t litcount;
  uint32_t backcount;
  uint32_t backoffset;
  uint32_t unpacked = 0;

  for(;;) {
    // Read litteral count
    litcount = *src;
    ++src;

    if(litcount >= 128) {
      litcount &= 0x7F;
      litcount <<= 8;
      litcount |= (unsigned char)*src;
      ++src;
    }

//    printf("Litteral %X\n", litcount);

    // Copy litteral
    while(litcount--) {
      *dst = *src;
      ++dst;
      ++src;
      ++unpacked;
    }

    // Read backcopy offset
    backoffset = *src;
    ++src;

    // Backoffset == 0 means end of stream
    if(!backoffset) return unpacked;

    if(backoffset >= 128) {
      backoffset &= 0x7F;
      backoffset <<= 8;
      backoffset |= (unsigned char)*src;
      ++src;
    }

    // Read backcopy count
    backcount = *src;
    ++src;

//    printf("Backcopy %X size=%X\n", backoffset, backcount);

    if(backcount >= 128) {
      backcount &= 0x7F;
      backcount <<= 8;
      backcount |= (unsigned char)*src;
      ++src;
    }

    while(backcount--) {
      *dst = *(dst - backoffset);
      ++dst;
      ++unpacked;
    }
  }
}

int main(int argc, char **argv)
{
  if(argc < 3)
  {
    fprintf(stderr, "Usage: %s input output\n", argv[0]);
    return 1;
  }

  FILE *infile = fopen(argv[1], "rb");
  fseek(infile, 0, SEEK_END);
  uint32_t insize = ftell(infile);
  fseek(infile, 0, SEEK_SET);

  char *indata = (char *)malloc(insize);
  fread(indata, 1, insize, infile);

  fclose(infile);

  FILE *outfile = fopen(argv[2], "wb");
  lz68k_pack(indata, insize, outfile);
  printf("Packed data :\n  insize = %u\n outsize = %u\n factor = %f\n", (unsigned int)insize, (unsigned int)ftell(outfile), (float)ftell(outfile) / (float)insize);
  fclose(outfile);

  if(argc >= 4)
  {
    uint32_t verifsize = insize;
    char *outdata = (char *)malloc(verifsize * 10);

    infile = fopen(argv[2], "rb");
    fseek(infile, 0, SEEK_END);
    insize = ftell(infile);
    char *data = (char *)malloc(insize);
    fseek(infile, 0, SEEK_SET);

    fread(data, 1, insize, infile);

    fclose(infile);
    uint32_t outsize = lz68k_unpack(data, outdata);

    if(outsize != verifsize)
    {
      uint32_t i;
      for(i = 0; i < (outsize > verifsize ? verifsize : outsize); ++i)
      {
        if(indata[i] != outdata[i])
        {
          printf("First error at %X\n", i);
          break;
        }
      }
      printf("Error : Output %X bytes instead of %X\n", outsize, verifsize);
    }
    outfile = fopen(argv[3], "wb");
    fwrite(outdata, 1, outsize, outfile);
    fclose(outfile);
  }

  return 0;
}
