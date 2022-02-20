#include <vga.h>
#include <nvboard.h>
#include <macro.h>
#include <assert.h>
#include <configs.h>

// http://tinyvga.com/vga-timing/800x600@60Hz
VGA_MODE vga_mod_accepted[NR_VGA_MODE] = {
  [VGA_MODE_640_480] = {
    .h_frontporch = 96,
    .h_active = 144,
    .h_backporch = 784,
    .h_total = 800,
    .v_frontporch = 2,
    .v_active = 35,
    .v_backporch = 515,
    .v_total = 525,
  },
  [VGA_MODE_800_600] = {
    .h_frontporch = 128,
    .h_active = 216,
    .h_backporch = 1016,
    .h_total = 1056,
    .v_frontporch = 4,
    .v_active = 27,
    .v_backporch = 627,
    .v_total = 628,
  },
};

#ifdef MODE_NEMU
VGA::VGA(SDL_Renderer *rend, int cnt, int init_val, int it, int ct, uint32_t* vmem, int MODE800x600):
    Component(rend, cnt, init_val, it, ct),
    vga_screen_width(MODE800x600? 800: 400), vga_screen_height(MODE800x600? 600:300),
    vga_pre_clk(0), vga_pre_hsync(0), vga_pre_vsync(0),
    vga_pos(0), vga_vaddr(0), vga_haddr(0) {
  SDL_Texture *temp_texture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STATIC, vga_screen_width, vga_screen_height);
  set_texture(temp_texture, 0);
  pixels = vmem;
}
#else
VGA::VGA(SDL_Renderer *rend, int cnt, int init_val, int it, int ct):
    Component(rend, cnt, init_val, it, ct), 
    vga_screen_width(VGA_DEFAULT_WIDTH), vga_screen_height(VGA_DEFAULT_HEIGHT),
    vga_pre_clk(0), vga_pre_hsync(0), vga_pre_vsync(0),
    vga_pos(0), vga_vaddr(0), vga_haddr(0) {
  SDL_Texture *temp_texture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STATIC, vga_screen_width, vga_screen_height);
  set_texture(temp_texture, 0);
  pixels = new uint32_t[vga_screen_width * vga_screen_height];
  memset(pixels, 0, vga_screen_width * vga_screen_height * sizeof(uint32_t));
}
#endif

VGA::~VGA() {
  SDL_DestroyTexture(get_texture(0));
#ifndef MODE_NEMU
  delete []pixels;
#endif
}

void VGA::update_gui() {
#ifndef MODE_NEMU
  static int frames = 0;
  frames ++;
  printf("%d frames\n", frames);
#endif
  SDL_Texture *temp_texture = get_texture(0);
  SDL_Renderer *temp_renderer = get_renderer();
  SDL_Rect *temp_rect = get_rect(0);
  SDL_UpdateTexture(temp_texture, NULL, pixels, vga_screen_width * sizeof(uint32_t));
  //SDL_RenderClear(temp_renderer);
  SDL_RenderCopy(temp_renderer, temp_texture, NULL, temp_rect);
}

#ifdef MODE_NEMU
void VGA::update_state() {
  update_gui();
}
#else

void VGA::update_state() {
  int vga_clk = output_map[output_pin::VGA_CLK];
  int vga_vsync = output_map[output_pin::VGA_VSYNC];
  int vga_hsync = output_map[output_pin::VGA_HSYNC];
  int vga_blank_n = output_map[output_pin::VGA_BLANK_N];
  if(!VGA_POS_EDGE(clk)){
    vga_pre_clk = vga_clk;
    return;
  }
  if(vga_blank_n) {
    int vga_r = (output_map[output_pin::VGA_R7] << 7) | 
                (output_map[output_pin::VGA_R6] << 6) | 
                (output_map[output_pin::VGA_R5] << 5) | 
                (output_map[output_pin::VGA_R4] << 4) |
                (output_map[output_pin::VGA_R3] << 3) | 
                (output_map[output_pin::VGA_R2] << 2) | 
                (output_map[output_pin::VGA_R1] << 1) | 
                 output_map[output_pin::VGA_R0];
    int vga_g = (output_map[output_pin::VGA_G7] << 7) | 
                (output_map[output_pin::VGA_G6] << 6) | 
                (output_map[output_pin::VGA_G5] << 5) | 
                (output_map[output_pin::VGA_G4] << 4) |
                (output_map[output_pin::VGA_G3] << 3) | 
                (output_map[output_pin::VGA_G2] << 2) | 
                (output_map[output_pin::VGA_G1] << 1) | 
                 output_map[output_pin::VGA_G0];
    int vga_b = (output_map[output_pin::VGA_B7] << 7) | 
                (output_map[output_pin::VGA_B6] << 6) | 
                (output_map[output_pin::VGA_B5] << 5) | 
                (output_map[output_pin::VGA_B4] << 4) |
                (output_map[output_pin::VGA_B3] << 3) | 
                (output_map[output_pin::VGA_B2] << 2) | 
                (output_map[output_pin::VGA_B1] << 1) | 
                 output_map[output_pin::VGA_B0];
    uint32_t vga_rgb = (vga_r << 16) | (vga_g << 8) | (vga_b);
    assert(vga_pos < vga_screen_width * vga_screen_height);
    pixels[vga_pos] = vga_rgb;
    vga_pos ++;
  }
  if(VGA_NEG_EDGE(vsync)) {
    vga_pos = 0;
    update_gui();
  }
  vga_pre_vsync = vga_vsync;
  vga_pre_clk = vga_clk;
}
#endif
