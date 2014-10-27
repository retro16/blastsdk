#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bls.h"

#define ISOHEADER (16 * (CDBLOCKSIZE))

static char current_date[17];
static u8 current_date_byte[7];

static void set_current_date() {
  time_t now_time;
  time(&now_time);
  struct tm now;
  gmtime_r(&now_time, &now);

  strftime(current_date, 17, "%Y%m%d%H%M%S00", &now);
  // The string termination character (\0) will be interpreted as UTC timezone
  
  current_date_byte[0] = now.tm_year;
  current_date_byte[1] = now.tm_mon + 1;
  current_date_byte[2] = now.tm_mday;
  current_date_byte[3] = now.tm_hour;
  current_date_byte[4] = now.tm_min;
  current_date_byte[5] = now.tm_sec == 60 ? 59 : now.tm_sec; // Ignore leap second
  current_date_byte[6] = 0;
}


static u32 blsiso_gen_date(FILE *of)
{
  fwrite(current_date, 1, 17, of);
  return 17;
}


static u32 blsiso_gen_date_byte(FILE *of)
{
  fwrite(current_date_byte, 1, 7, of);
  return 7;
}


static u32 blsiso_gen_null_date(FILE *of)
{
  fwrite("0000000000000000", 1, 17, of);
  return 17;
}


static u32 blsiso_gen_null_date_byte(FILE *of)
{
  fwrite("\x00\x00\x00\x00\x00\x00", 1, 7, of);
  return 7;
}


static u32 blsiso_gen_number_721(FILE *of, u32 value)
{
  fputc((value >> 0) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);

  return 2;
}


static u32 blsiso_gen_number_722(FILE *of, u32 value)
{
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 0) & 0xFF, of);

  return 2;
}


static u32 blsiso_gen_number_72(FILE *of, u32 value, int msb)
{
  if(msb) {
    return blsiso_gen_number_722(of, value);
  }
  return blsiso_gen_number_721(of, value);
}


static u32 blsiso_gen_number_723(FILE *of, u32 value)
{
  fputc((value >> 0) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 0) & 0xFF, of);

  return 4;
}


static u32 blsiso_gen_number_731(FILE *of, u32 value)
{
  fputc((value >> 0) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 16) & 0xFF, of);
  fputc((value >> 24) & 0xFF, of);

  return 4;
}


static u32 blsiso_gen_number_732(FILE *of, u32 value)
{
  fputc((value >> 24) & 0xFF, of);
  fputc((value >> 16) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 0) & 0xFF, of);

  return 4;
}


static u32 blsiso_gen_number_73(FILE *of, u32 value, int msb)
{
  if(msb) {
    return blsiso_gen_number_732(of, value);
  }
  return blsiso_gen_number_731(of, value);
}


static u32 blsiso_gen_number_733(FILE *of, u32 value)
{
  fputc((value >> 0) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 16) & 0xFF, of);
  fputc((value >> 24) & 0xFF, of);
  fputc((value >> 24) & 0xFF, of);
  fputc((value >> 16) & 0xFF, of);
  fputc((value >> 8) & 0xFF, of);
  fputc((value >> 0) & 0xFF, of);

  return 8;
}


static u32 blsiso_gen_string(FILE *of, const char *string, u32 length, int a, int padding)
{
  u32 i;
  u32 o = 0;
  int separator = 0;
  if(!string) {
    string = "";
  }
  for(i = 0; o < length && string[i]; ++i)
  {
    int valid = 0;
    if(string[i] >= '0' && string[i] <= '9') { valid = 1; }
    else if(string[i] >= 'A' && string[i] <= 'Z') { valid = 1; }
    else if(a && (string[i] == '.' || string[i] == ' ' || string[i] == '-')) { valid = 1; }
    else if(string[i] >= 'a' && string[i] <= 'z') { valid = a ? 1 : 2; }

    if(valid && separator) {
      fputc('_', of);
      ++o;
      if(o >= length) {
        break;
      }
    }

    if(valid == 1) {
      fputc(string[i], of);
      ++o;
    } else if(valid == 2) {
      fputc(string[i] - 'a' + 'A', of);
      ++o;
    }

    separator = !valid;
  }

  if(separator && o < length) {
    fputc('_', of);
    ++o;
  }

  if(padding >= 0) {
    while(o < length) {
      fputc(padding, of);
      ++o;
    }
  }

  return length;
}


static int is_file(const char *path)
{
  struct stat s;
  if(stat(path, &s) < 0) {
    return 0;
  }

  return (s.st_mode & S_IFMT) == S_IFREG;
}


// Generate a record in a directory.
// Revision should be ';1' most of the time
static u32 blsiso_gen_record(FILE *of, int directory, const char *filename, const char *revision, u32 sector, u32 recordsize)
{
  u32 namelen = strlen(filename) + strlen(revision);
  if(namelen == 0) {
    namelen = 1;
  }
  u32 sz = namelen + 33;
  u32 odd = 0;
  if(sz & 1) {
    odd = 1;
  }

  fputc(sz + odd, of);
  fputc(0, of);
  blsiso_gen_number_733(of, sector);
  blsiso_gen_number_733(of, recordsize);
  blsiso_gen_date_byte(of);
  fputc(directory ? 2 : 0, of);
  fputc(0, of);
  fputc(0, of);
  blsiso_gen_number_723(of, 1);
  fputc(namelen, of);

  if(!*filename) {
    fputc(0, of);
  } else if(*filename == 1) {
    fputc(1, of);
  } else {
    const char *dot = strchr(filename, '.');
    if(dot && strchr(dot + 1, '.')) {
      fprintf(stderr, "Invalid file name %s: more than one extension separator. ISO9660 file names must be 8.3\n", filename);
      exit(1);
    }
    if((dot && dot - filename > 8) || (!dot && strlen(filename) > 8)) {
      fprintf(stderr, "Invalid file name %s: too long. ISO9660 file names must be 8.3\n", filename);
      exit(1);
    }
    if(dot && strlen(dot + 1) > 3) {
      fprintf(stderr, "Invalid file name %s: extension too long. ISO9660 file names must be 8.3\n", filename);
      exit(1);
    }
    blsiso_gen_string(of, filename, dot ? ((dot - filename > 8) ? 8 : (dot - filename)) : 8, 0, -1);
    if(dot) {
      fputc('.', of);
      blsiso_gen_string(of, dot + 1, 3, 0, -1);
    }
    fprintf(of, "%s", revision);
  }

  if(sz & 1) {
    fputc(0, of);
    ++sz;
  }

  return sz;
}


static u32 blsiso_gen_terminator(FILE *of)
{
  fwrite("\xFF""CD001\x01", 1, 8, of);
  u32 i;
  for(i = 0; i < CDBLOCKSIZE - 8; ++i) {
    fputc(0, of);
  }
  return CDBLOCKSIZE;
}


#define ROOTSECTOR 0x13


static u32 blsiso_gen_primary_volume_descriptor(FILE *of, const char *name, const char *author, const char *copyright, u32 volsize, u32 rootsize)
{

  u32 i;
  fwrite("\x01""CD001\x01\x00", 1, 8, of);

  blsiso_gen_string(of, "SEGA_MEGA_CD", 32, 0, ' '); // 0x08 System identifier
  blsiso_gen_string(of, name, 32, 1, ' '); // 0x28 Volume identifier
  blsiso_gen_string(of, "", 8, 0, 0); // 0x48 Unused
  blsiso_gen_number_733(of, volsize / CDBLOCKSIZE); // 0x50 Volume space size
  blsiso_gen_string(of, "", 32, 0, 0); // 0x58 Unused
  blsiso_gen_number_723(of, 1); // 0x78 Volume set size
  blsiso_gen_number_723(of, 1); // 0x7C Volume sequence number
  blsiso_gen_number_723(of, CDBLOCKSIZE); // 0x80 CD block size
  blsiso_gen_number_733(of, 0x0A); // 0x84 Path table size
  blsiso_gen_number_731(of, 0x11); // 0x8C LE path table
  blsiso_gen_number_731(of, 0x00); // 0x90 LE optional path table
  blsiso_gen_number_732(of, 0x12); // 0x94 BE path table
  blsiso_gen_number_732(of, 0x00); // 0x98 BE optional path table
  blsiso_gen_record(of, 1, "", "", ROOTSECTOR, rootsize); // 0x9C Root directory entry
  blsiso_gen_string(of, name, 128, 1, ' '); // Volume set identifier
  blsiso_gen_string(of, author, 128, 1, ' '); // Publisher identifier
  blsiso_gen_string(of, "", 128, 1, ' '); // Data preparer identifier
  blsiso_gen_string(of, "Sega Mega CD bootable CD-ROM", 128, 1, ' '); // Data preparer identifier
  blsiso_gen_string(of, "", 128, 1, ' '); // Copyright identifier
  blsiso_gen_string(of, "", 128, 1, ' '); // Abstract file identifier
  blsiso_gen_string(of, "", 127, 1, ' '); // Bibliographic identifier
  blsiso_gen_date(of); // Volume creation date and time
  blsiso_gen_date(of); // Volume modification date and time
  blsiso_gen_null_date(of); // Volume expiration date and time
  blsiso_gen_null_date(of); // Volume effective date and time
  fwrite("\x01", 1, 2, of); // File structure version + reserved
  blsiso_gen_string(of, "", 512, 0, 0); // Application use
  blsiso_gen_string(of, "", 381, 0, 0); // Reserved

  return CDBLOCKSIZE;
}


static u32 blsiso_gen_pathtable(FILE *of, int msb)
{
  fwrite("\x01", 1, 2, of);
  blsiso_gen_number_73(of, ROOTSECTOR, msb);
  blsiso_gen_number_72(of, 1, msb);

  blsiso_gen_string(of, "", CDBLOCKSIZE - 0x08, 0, 0);

  return CDBLOCKSIZE;
}


static u32 blsiso_gen_rootdir(FILE *of, const char *data_dir, u32 iso_size)
{
  u32 outsize = 0;
  u32 rootsize = 0;
  u32 dataoffset = ISOHEADER + CDBLOCKSIZE * 3;

  if(iso_size) {
    u32 startoff = ftell(of);
    rootsize = blsiso_gen_rootdir(of, data_dir, 0);
    dataoffset += rootsize;
    fseek(of, startoff, SEEK_SET);
  }

  outsize += blsiso_gen_record(of, 1, "", "", ROOTSECTOR, rootsize);
  outsize += blsiso_gen_record(of, 1, "\x01", "", ROOTSECTOR, rootsize);
  outsize += blsiso_gen_record(of, 0, "SELF.ISO", ";1", 0, iso_size); // SELF.ISO containing the whole CD-ROM itself (!!!)

  if(data_dir) {
    DIR *d = opendir(data_dir);
    if(d) {
      struct dirent *de;
      for(de = readdir(d); de; de = readdir(d)) {
        char data[65536];
        snprintf(data, 65536, "%s/%s", data_dir, de->d_name);
        if(!is_file(data)) {
          continue;
        }
        FILE *f = fopen(data, "r");
        if(f) {
          fseek(f, 0, SEEK_END);
          u32 sz = ftell(f);
          outsize += blsiso_gen_record(of, 0, de->d_name, ";1", dataoffset / CDBLOCKSIZE, sz);
          dataoffset += sz + (CDBLOCKSIZE - (sz % CDBLOCKSIZE)) % CDBLOCKSIZE;
          fclose(f);
        }
      }
    } else {
      perror("Warning: Cannot open data directory.");
    }
  }

  u32 padding = (CDBLOCKSIZE - (outsize % CDBLOCKSIZE)) % CDBLOCKSIZE;
  outsize += blsiso_gen_string(of, "", padding, 0, 0);

  return outsize;
}


// Generate an ISO filesystem containing all files in data_dir plus SELF.ISO containing the image of the disk itself (of iso_size bytes).
// Returns the offset after the last file, excluding SELF.ISO.
u32 blsiso_gen(FILE *of, const char *name, const char *author, const char *copyright, const char *data_dir, u32 iso_size, int copy_content)
{
  set_current_date();

  printf("DATA DIR %p: [%s]\n", data_dir, data_dir ? data_dir : "(null)");

  // Count files in extra_dir
  u32 extra_files_count = 0; // TODO

  // Pad ISO to 32k
  fseek(of, 0, SEEK_END);
  while(ftell(of) < ISOHEADER) {
    fputc(0, of);
  }

  // Go to ISO header offset
  fseek(of, ISOHEADER, SEEK_SET);

  u32 imgsize = ISOHEADER; // Data starts at 16 sectors (32k)
  imgsize += blsiso_gen_primary_volume_descriptor(of, name, author, copyright, iso_size, CDBLOCKSIZE);
  imgsize += blsiso_gen_pathtable(of, 0);
  imgsize += blsiso_gen_pathtable(of, 1);
  u32 rootlen = blsiso_gen_rootdir(of, data_dir, iso_size);
  imgsize += rootlen;
  if(rootlen != CDBLOCKSIZE) {
    // Update root directory size
    u32 curpos = ftell(of);
    fseek(of, ISOHEADER, SEEK_SET);
    blsiso_gen_primary_volume_descriptor(of, name, author, copyright, iso_size, rootlen);
    fseek(of, curpos, SEEK_SET);
  }

  if(data_dir) {
    DIR *d = opendir(data_dir);
    if(d) {
      struct dirent *de;
      for(de = readdir(d); de; de = readdir(d)) {
        char data[65536];
        snprintf(data, 65536, "%s/%s", data_dir, de->d_name);
        if(!is_file(data)) {
          continue;
        }
        printf("Adding file %s to image\n", data);
        FILE *f = fopen(data, "r");
        if(f) {
          u32 sz = 0;
          if(copy_content) {
            while(!feof(f)) {
              u32 block = fread(data, 1, sizeof(data), f);
              sz += block;
              fwrite(data, 1, block, of);
            }
          } else {
            fseek(f, 0, SEEK_END);
            sz = ftell(f);
          }
          fclose(f);

          // Pad to next sector
          u32 psz = (CDBLOCKSIZE - (sz % CDBLOCKSIZE)) % CDBLOCKSIZE;
          if(copy_content) {
            blsiso_gen_string(of, "", psz, 0, 0);
          }
          imgsize += sz + psz;
        }
      }
    } else {
      perror("Warning: Cannot open data directory.");
    }
  }

  return imgsize;
}


/*
int main()
{
  set_current_date();
  FILE *of = fopen("out.iso", "w");
  u32 isosize = blsiso_gen(of, "MYGAME", "MYSELF", "COPYRIGHT", "content", 300 * CDBLOCKSIZE);
  while(ftell(of) < 300 * CDBLOCKSIZE) {
    fputc('P', of); // Padding
  }
  
  fclose(of);
  printf("isosize = %u\n", isosize);
  return 0;
}
*/
