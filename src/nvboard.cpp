#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <nvboard.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <assert.h>
#include <string>

using namespace std;

static SDL_Window *main_window = nullptr;
static SDL_Renderer *main_renderer = nullptr;
static map<void *, vector<input_pin >> input_pin_map;
static map<void *, vector<output_pin>> output_pin_map;
map<input_pin, int> input_map;
map<output_pin, int> output_map;
string nvboard_home;

int read_event();

static int read_event_flag = true;
static int render_flag = true;
static void alarm_sig_handler(int signum) {
    read_event_flag = true;
    render_flag = true;
}

static int nvboard_event_handler() {
    if(!read_event_flag) return 0;
    read_event_flag = false;
    int ev = read_event();
    if(ev != -1){
        update_components(main_renderer);
    } else {
      exit(0);
    }
    return ev;
}

static void nvboard_update_all_input() {
  for (auto p = input_pin_map.begin(); p != input_pin_map.end(); p ++) {
    void *ptr = p->first;
    vector<input_pin> &pins = p->second;
    uint32_t size = pins.size();
    assert(size <= 64);
    uint64_t val = 0;
    for (auto ppins = pins.begin(); ppins != pins.end(); ppins ++) {
      val <<= 1;
      val |= input_map[*(ppins)];
    }

    if (size <= 8) { *(uint8_t *)ptr = val; }
    else if (size <= 16) { *(uint16_t *)ptr = val; }
    else if (size <= 32) { *(uint32_t *)ptr = val; }
    else if (size <= 64) { *(uint64_t *)ptr = val; }
  }
  static int pre_clk;
  pre_clk = input_map[input_pin::PS2_CLK];
}

static void nvboard_update_all_output() {
  for (auto p = output_pin_map.begin(); p != output_pin_map.end(); p ++) {
    void *ptr = p->first;
    vector<output_pin> &pins = p->second;
    uint32_t size = pins.size();
    assert(size <= 64);
    uint64_t val = 0;
    if (size <= 8) { val = *(uint8_t *)ptr; }
    else if (size <= 16) { val = *(uint16_t *)ptr; }
    else if (size <= 32) { val = *(uint32_t *)ptr; }
    else if (size <= 64) { val = *(uint64_t *)ptr; }

    for (auto ppins = pins.rbegin(); ppins != pins.rend(); ppins ++) {
      output_map[*(ppins)] = val & 1;
      val >>= 1;
    }
  }
}

#ifdef MODE_NEMU
extern "C" {
  void nvboard_update() {

    if(render_flag) {
      update_components(main_renderer);
      SDL_RenderPresent(main_renderer);
      render_flag = false;
    }

    nvboard_event_handler();
  }

  uint8_t nvboard_get_key(bool* succ){
    extern KEYBOARD* kb;
    return kb->pop_key(succ);
  }

  void nvboard_init(uint32_t* vmem, int MODE800x600) {
    printf("nvboard v0.2\n");
    // init SDL and SDL_image
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    IMG_Init(IMG_INIT_PNG);

    main_window = SDL_CreateWindow("nvboard", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH * 2, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    main_renderer = SDL_CreateRenderer(main_window, -1,
    #ifdef VSYNC
        SDL_RENDERER_PRESENTVSYNC |
    #endif
    #ifdef HARDWARE_ACC
        SDL_RENDERER_ACCELERATED |
    #else
        SDL_RENDERER_SOFTWARE |
    #endif
        0
    );

    nvboard_home = getenv("NVBOARD_HOME");

    load_background(main_renderer);
    load_texture(main_renderer);
    init_components(main_renderer, vmem, MODE800x600);
    init_gui(main_renderer);

    update_components(main_renderer);
    struct sigaction s;
    memset(&s, 0, sizeof(s));
    s.sa_handler = alarm_sig_handler;
    int ret = sigaction(SIGVTALRM, &s, NULL);
    assert(ret == 0);

    struct itimerval it = {};
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 1000000 / 60;
    it.it_interval = it.it_value;
    ret = setitimer(ITIMER_VIRTUAL, &it, NULL);
    assert(ret == 0);
  }
  void nvboard_quit(){
      delete_components();
      SDL_DestroyWindow(main_window);
      SDL_DestroyRenderer(main_renderer);
      IMG_Quit();
      SDL_Quit();
  }
}
#else
void nvboard_update() {
  nvboard_update_all_input();
  nvboard_update_all_output();

  update_components(main_renderer);
  if(render_flag) {
    SDL_RenderPresent(main_renderer);
    render_flag = false;
  }

  nvboard_event_handler();
}

void nvboard_init() {
    printf("nvboard v0.2\n");
    // init SDL and SDL_image
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    IMG_Init(IMG_INIT_PNG);

    main_window = SDL_CreateWindow("nvboard", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH * 2, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    main_renderer = SDL_CreateRenderer(main_window, -1, 
    #ifdef VSYNC
        SDL_RENDERER_PRESENTVSYNC |
    #endif
    #ifdef HARDWARE_ACC
        SDL_RENDERER_ACCELERATED |
    #else
        SDL_RENDERER_SOFTWARE |
    #endif
        0
    );
    
    // To avoid the SDL bugs on hby's linux
    //usleep(200000);

    nvboard_home = getenv("NVBOARD_HOME");
    
    load_background(main_renderer);
    load_texture(main_renderer);
    init_components(main_renderer);
    init_gui(main_renderer);
    for (input_pin i = input_pin(0); i < input_pin::NR_INPUT_PINS; i = input_pin((int)i + 1)) {
      input_map[i] = 0;
    }
    for (output_pin i = output_pin(0); i < output_pin::NR_OUTPUT_PINS; i = output_pin((int)i + 1)) {
      output_map[i] = 0;
    }

    update_components(main_renderer);
    struct sigaction s;
    memset(&s, 0, sizeof(s));
    s.sa_handler = alarm_sig_handler;
    int ret = sigaction(SIGVTALRM, &s, NULL);
    assert(ret == 0);

    struct itimerval it = {};
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 1000000 / 60;
    it.it_interval = it.it_value;
    ret = setitimer(ITIMER_VIRTUAL, &it, NULL);
    assert(ret == 0);
}

void nvboard_quit(){
    delete_components();
    SDL_DestroyWindow(main_window);
    SDL_DestroyRenderer(main_renderer);
    IMG_Quit();
    SDL_Quit();
}
#endif

void nvboard_bind_pin(vector<output_pin> &pin, void *signal) {
  output_pin_map[signal] = pin;
}

void nvboard_bind_pin(vector<input_pin> &pin, void *signal) {
  input_pin_map[signal] = pin;
}

void nvboard_bind_pin(output_pin pin, void *signal) {
  vector<output_pin> p = {pin};
  nvboard_bind_pin(p, signal);
}

void nvboard_bind_pin(input_pin pin, void *signal) {
  vector<input_pin> p = {pin};
  nvboard_bind_pin(p, signal);
}
