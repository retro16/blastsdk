#include <stdio.h>
#include <string.h>

void fputhex(int c, FILE *o)
{
  if(c >= 10) {
    fputc('a' + c - 10, o);
  } else {
    fputc('0' + c, o);
  }
}

char valid_symbol(char c)
{
  if(c >= '0' && c <= '9') {
    return c;
  }

  if(c >= 'a' && c <= 'z') {
    return c;
  }

  if(c >= 'A' && c <= 'Z') {
    return c;
  }

  return '_';
}

int main(int argc, char **argv)
{
  FILE *i;
  FILE *o;
  int c;
  int oc;
  int count = -1;
  char symname[256];
  char oname[256];

  if(argc != 2 && argc != 3) {
    return 1;
  }

  for(c = 0, oc = 0; argv[1][c] && c < 250; ++c, ++oc) {
    if(argv[1][c] == '/' || argv[1][c] == '\\') {
      oc = -1;
    } else {
      symname[oc] = valid_symbol(argv[1][c]);
    }
  }

  if(c <= 0) {
    printf("Invalid input file name.\n");
    return 1;
  }

  symname[oc] = '\0';

  if(argc == 3) {
    strcpy(oname, argv[2]);
  } else {
    memcpy(oname, argv[1], c);
    oname[c] = '.';
    oname[c+1] = 'c';
    oname[c+2] = '\0';
  }

  i = fopen(argv[1], "r");

  if(!i) {
    printf("Could not open input file.\n");
    return 1;
  }

  // Get input file size
  fseek(i, 0, SEEK_END);
  oc = ftell(i);
  fseek(i, 0, SEEK_SET),

        o = fopen(oname, "w");

  if(!o) {
    printf("Could not open output file.\n");
    return 1;
  }

  fputs("unsigned char ", o);
  fputs(symname, o);
  fputs("[", o);
  fprintf(o, "0x%x", oc);
  fputs("] = {\n ", o);

  while((c = fgetc(i)) != EOF) {
    if(count > 0) {
      if(count > 16) {
        count = 1;
        fputs("\n", o);
      }

      fputc(',', o);
    } else {
      count = 1;
    }

    ++count;
    fputs("0x", o);
    fputhex(c >> 4, o);
    fputhex(c & 0x0F, o);
  }

  fputs("\n};\n", o);
  return 0;
}
