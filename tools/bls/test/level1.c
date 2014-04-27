extern int common_variable;
void common_load_map(void *img, int imgtarget, void *map, int maptarget, void *pal);
void level1_init()
{
  common_load_map(0, 0, 0, 0, 0);
  ++common_variable;
}
