#ifndef _VFPGA_VGA_H
#define _VFPGA_VGA_H

#include <component.h>
#include <constrs.h>
#include <SDL2/SDL.h>
#include <nvboard.h>

#define VGA_DEFAULT_WIDTH  640
#define VGA_DEFAULT_HEIGHT 480

enum { //VGA_MOD_ID
  VGA_MODE_640_480, VGA_MODE_800_600, NR_VGA_MODE
};

struct VGA_MODE{
  int h_frontporch, h_active, h_backporch, h_total;
  int v_frontporch, v_active, v_backporch, v_total;
};

class VGA : public Component{
private:
  int vga_screen_width, vga_screen_height;
  uint32_t *pixels;
  int vga_pos;
  int vga_pre_clk, vga_pre_vsync, vga_pre_hsync;
  int vga_vaddr, vga_haddr;
public:
#ifdef MODE_NEMU
  VGA(SDL_Renderer *rend, int cnt, int init_val, int it, int ct, uint32_t* vmem, int MODE800x600);
#else
  VGA(SDL_Renderer *rend, int cnt, int init_val, int it, int ct);
#endif
  ~VGA();

  virtual void update_gui();
  virtual void update_state();
};

#endif
