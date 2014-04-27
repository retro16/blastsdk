void common_level_init();

void load_level(int l)
{
  switch(l)
  {
    case 1:
      BLS_LOAD_BINARY_LEVEL1_C();
      level1_init();
      break;
    case 2:
      BLS_LOAD_BINARY_LEVEL2_C();
      level2_init();
      break;
  }
}

void loader_main()
{
  BLS_LOAD_BINARY_COMMON();
  common_init();
  load_level(1);
}
