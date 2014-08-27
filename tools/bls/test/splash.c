void loader_main();

void display_splash()
{
  // Display splash.png
  BLS_LOAD_BINARY_SPLASH_PNG_IMG();
}

void main()
{
  display_splash();
  BLS_LOAD_BINARY_LOADER_C();
  loader_main();
}
