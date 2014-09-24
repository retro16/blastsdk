#include "bls.h"

int main(int argc, char **argv)
{
  char file[1024] = "blsbuild.ini";

  if(argc >= 2) {
    const char *s = strrchr(argv[1], '/');

    if(!s) {
      // use file name as-is
      s = argv[1];
    } else {
      // chdir to the ini file
      int l = s - argv[1];
      strncpy(file, argv[1], l);
      file[l] = '\0';
      printf("Entering directory %s\n", file);
      chdir(file);
      ++s; // Skip separator
    }

    strncpy(file, s, 1024);
    file[1023] = '\0';
  }

  printf("Building %s\n", file);

  mkdir(TMPDIR, 0777);
  parse_ini(file);
  out_genimage();

  return 0;
}

// vim: ts=2 sw=2 sts=2 et
