void loader_main();

void display_splash()
{
  // Display splash.png
}

void main()
{
  display_splash();
  BLS_LOAD_BINARY_LOADER_C();
  loader_main();
}
