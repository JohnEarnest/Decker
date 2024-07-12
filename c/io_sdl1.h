
// compatibility shim for SDL 1.2

// many features have reduced or missing functionality:
// - Decker does not check the application base path for start.deck
// - Decker cannot open URI as by go["http://..."]
// - Decker cannot interact with the system clipboard; it instead uses an internal buffer
// - Decker cannot record audio
// - Decker does not automatically upscale in windowed mode
// - Decker does not support mouse-wheel scrolling
// - Decker does not automatically engage touch mode
// - Decker does not support file drag-and-drop
// - Decker does not support partial window opacity (tracing mode)
// - Decker does not display pointer/ibeam/resize cursors (TODO: this one's fixable!)
// - Decker only supports reading GIF images

#include <SDL.h>
//#include "SDL_thread.h"

SDL_Cursor*CURSORS[4];

// keyboard keys

#define KEY_UP           SDLK_UP
#define KEY_DOWN         SDLK_DOWN
#define KEY_LEFT         SDLK_LEFT
#define KEY_RIGHT        SDLK_RIGHT
#define KEY_PAGEUP       SDLK_PAGEUP
#define KEY_PAGEDOWN     SDLK_PAGEDOWN
#define KEY_HOME         SDLK_HOME
#define KEY_END          SDLK_END
#define KEY_SPACE        SDLK_SPACE
#define KEY_BACKSPACE    SDLK_BACKSPACE
#define KEY_DELETE       SDLK_DELETE
#define KEY_ESCAPE       SDLK_ESCAPE
#define KEY_RETURN       SDLK_RETURN
#define KEY_TAB          SDLK_TAB
#define KEY_CAPSLOCK     SDLK_CAPSLOCK
#define KEY_LSHIFT       SDLK_LSHIFT
#define KEY_RSHIFT       SDLK_RSHIFT
#define KEY_LCTRL        SDLK_LCTRL
#define KEY_RCTRL        SDLK_RCTRL
#define KEY_LGUI         SDLK_LMETA
#define KEY_RGUI         SDLK_RMETA
#define KEY_LEFTBRACKET  SDLK_LEFTBRACKET
#define KEY_RIGHTBRACKET SDLK_RIGHTBRACKET
#define KEY_0            SDLK_0
#define KEY_1            SDLK_1
#define KEY_2            SDLK_2
#define KEY_3            SDLK_3
#define KEY_9            SDLK_9
#define KEY_l            SDLK_l
#define KEY_j            SDLK_j
#define KEY_k            SDLK_k
#define KEY_m            SDLK_m
#define KEY_t            SDLK_t
#define KEY_u            SDLK_u
#define KEY_y            SDLK_y
#define KEY_F1           SDLK_F1
#define KEY_F2           SDLK_F2
#define KEY_F3           SDLK_F3
#define KEY_F4           SDLK_F4
#define KEY_F5           SDLK_F5
#define KEY_F6           SDLK_F6
#define KEY_F7           SDLK_F7
#define KEY_F8           SDLK_F8
#define KEY_F9           SDLK_F9
#define KEY_F10          SDLK_F10
#define KEY_F11          SDLK_F11
#define KEY_F12          SDLK_F12

// global interpreter lock

SDL_mutex*gil=NULL;
void interpreter_lock(void){SDL_LockMutex(gil);}
void interpreter_unlock(void){SDL_UnlockMutex(gil);}

// resources

void base_path(char*path){path[0]="\0";} // stubbed
void open_url(char*x){(void)x;} // stubbed
lv* readimage(char*path,int grayscale){return n_readgif(NULL,lml2(lmcstr(path),grayscale?lmistr("gray"):NONE));}

// clipboard

char*clip_data=NULL;
int clip_size=0;
lv* get_clip(void){return clip_data?lmcstr(clip_data): lmistr("");}
void set_clip(lv*x){
	x=ls(x);
	if(!clip_data    ){clip_size=x->c,clip_data=malloc(x->c);}
	if(clip_size<x->c){clip_size=x->c,clip_data=realloc(clip_data,x->c);}
	memcpy(clip_data,x->sv,clip_size);
}

// audio

#define SFX_FORMAT AUDIO_S8
#define SFX_CHANNELS 1
int nosound=0;
SDL_AudioSpec audio;

void sfx_pump(void*user,Uint8*stream,int len);
void record_pump(void* userdata,Uint8* stream,int len);

lv*n_readwav(lv*self,lv*a){
	(void)self;char*name=ls(l_first(a))->sv;
	Uint8* raw; Uint32 length; SDL_AudioSpec spec; SDL_AudioCVT cvt;
	if(SDL_LoadWAV(name,&spec,&raw,&length)==NULL)return sound_make(lms(0));
	if(SDL_BuildAudioCVT(&cvt, spec.format,spec.channels,spec.freq, SFX_FORMAT,SFX_CHANNELS,SFX_RATE)){
		cvt.len=length,cvt.buf=malloc(cvt.len * cvt.len_mult);
		memcpy(cvt.buf,raw,length),SDL_FreeWAV(raw),SDL_ConvertAudio(&cvt);
		raw=cvt.buf, length=cvt.len_cvt;
	}lv*r=lmv(1);r->c=MIN(length,10*SFX_RATE);r->sv=(char*)raw;return sound_make(r);
}

int record_possible(void){return 0;} // stubbed
int record_begin(void){return -1;} // stubbed
void record_pause(int device){(void)device;} // stubbed
void record_finish(int device){(void)device;} // stubbed

// input events

void event_quit(void);
void event_touch(void);
void event_key(int c,int m,int down,const char*name);
void event_scroll(pair s);
void event_pointer_move(pair raw,pair scaled);
void event_pointer_button(int primary,int down);
void event_file(char*p);
void field_input(char*text);

void process_events(pair disp,pair size,int scale){
	SDL_Event e;
	while(SDL_PollEvent(&e)){
		if(e.type==SDL_QUIT     )event_quit();
		if(e.type==SDL_USEREVENT)break;
		if(e.type==SDL_KEYDOWN  ){
			int u=e.key.keysym.unicode;
			if(u=='\t'||u=='\n'||(u>=32&&u<=126)){char t[2]={0};t[0]=0xFF&u;field_input(t);}
			event_key(e.key.keysym.sym,e.key.keysym.mod,1,SDL_GetKeyName(e.key.keysym.sym));
		}
		if(e.type==SDL_KEYUP    )event_key(e.key.keysym.sym,e.key.keysym.mod,0,SDL_GetKeyName(e.key.keysym.sym));
		if(e.type==SDL_MOUSEMOTION){
			pair b={(disp.x-(size.x*scale))/2,(disp.y-(size.y*scale))/2};
			event_pointer_move((pair){e.motion.x,e.motion.y},(pair){(e.motion.x-b.x)/scale,(e.motion.y-b.y)/scale});
		}
		if(e.type==SDL_MOUSEBUTTONDOWN)event_pointer_button(e.button.button==SDL_BUTTON_LEFT,1);
		if(e.type==SDL_MOUSEBUTTONUP  )event_pointer_button(e.button.button==SDL_BUTTON_LEFT,0);
		//if(e.type==SDL_MOUSEWHEEL     )event_scroll((pair){e.wheel.x,e.wheel.y});
		//if(e.type==SDL_FINGERDOWN     )event_touch();
		//if(e.type==SDL_DROPFILE       )event_file(e.drop.file),SDL_free(e.drop.file);
	}
	SDL_FlushEvent(SDL_USEREVENT);
}

Uint32 tick_pump(Uint32 interval,void*param){
	SDL_Event e; SDL_UserEvent u;
	u.type=e.type=SDL_USEREVENT;
	u.data1=param;
	e.user=u;
	SDL_PushEvent(&e);
	return interval;
}

// rendering

SDL_Surface*surf;

// TODO!

SDL_Renderer*ren;
SDL_Texture*gfx;
SDL_Texture*gtool;
#ifdef LOSPEC
// a set of customizations intended to make Decker more portable
// and more performant on extremely limited devices such as the OLPC XO-4.
int parity=0;
int*sgfx=NULL;
void framebuffer_alloc(pair size,int minscale){
	gfx=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,size.x*minscale,size.y*minscale);
	sgfx=calloc(size.x*size.y,sizeof(int));
}
int framebuffer_flip(pair disp,pair size,int scale,int mask,int frame,char*pal,lv*buffer){
	parity^=1;if(!parity)return 0;
	int mask=dr.trans_mask&&uimode==mode_draw;
	draw_frame(pal,buffer,sgfx,size.x*4,frame,mask);frame_count++;
	int*p, pitch;
	SDL_LockTexture(gfx,NULL,(void**)&p,&pitch);
	int stride=pitch/sizeof(int);
	// SDL's X11 software renderer is outrageously slow at texture upscaling on the OLPC, so we'll do it by hand:
	if(scale==1)for(int y=0,i=0,o=0;y<size.y;y++,o+=stride)for(int x=0;x<size.x;x++,i++     )p[o+x]=sgfx[i];
	if(scale==2)for(int y=0,i=0,o=0;y<size.y;y++,o+=stride)for(int x=0;x<size.x;x++,i++,o+=2)p[o]=p[o+1]=p[o+stride]=p[o+stride+1]=sgfx[i];
	SDL_UnlockTexture(gfx);
	SDL_Rect dst={(disp.x-scale*size.x)/2,(disp.y-scale*size.y)/2,scale*size.x,scale*size.y};
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren,gfx,NULL,&dst);
	return 1;
}
#else
void framebuffer_alloc(pair size,int minscale){
	(void)minscale;
	gfx=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,size.x,size.y);
}
int framebuffer_flip(pair disp,pair size,int scale,int mask,int frame,char*pal,lv*buffer){
	int* p, pitch;
	SDL_LockTexture(gfx,NULL,(void**)&p,&pitch);
	draw_frame(pal,buffer,p,pitch,frame,mask);frame_count++;
	SDL_UnlockTexture(gfx);
	SDL_Rect src={0,0,size.x,size.y};
	SDL_Rect dst={(disp.x-scale*size.x)/2,(disp.y-scale*size.y)/2,scale*size.x,scale*size.y};
	SDL_SetRenderDrawColor(ren,0x00,0x00,0x00,0xFF);
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren,gfx,&src,&dst);
	return 1;
}
#endif

void toolbar_flip(lv*buffer,int frame,char*pal,rect dest){
	pair tsize=buff_size(buffer);
	SDL_Rect src={0,0,tsize.x,tsize.y},dst={dest.x,dest.y,dest.w,dest.h};
	if(!gtool){pair s=buff_size(buffer);gtool=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,s.x,s.y);}
	int*p, pitch;
	SDL_LockTexture(gtool,NULL,(void**)&p,&pitch);
	draw_frame(pal,buffer,p,pitch,frame,0);
	SDL_UnlockTexture(gtool);
	SDL_RenderCopy(ren,gtool,&src,&dst);
}
void finish_flip(void){SDL_RenderPresent(ren);}

// windows

SDL_Window*win;
pair get_display_size(void){return (pair){800,600};} // stubbed
int get_display_density(pair disp){return 1;} // stubbed
void window_set_title(char*x){SDL_WM_SetCaption(x,NULL);}
void window_set_opacity(float x){(void)x;} // stubbed
void window_set_cursor(int x){
	// TODO!
	//SDL_SetCursor(CURSORS[x]);
	(void)x;
}
int is_fullscreen=0;
void window_set_fullscreen(int full){
	if(full==is_fullscreen)return;
	SDL_WM_ToggleFullScreen(surf);
	is_fullscreen=!is_fullscreen;
}
pair window_size={0,0};
pair window_get_size(void){return window_size;}
void window_set_size(pair wsize,pair size,int scale){
	window_size=wsize;
	surf=SDL_SetVideoMode(wsize.x,wsize.h,0,SDL_DOUBLEBUF);
	window_set_title("Decker");
	framebuffer_alloc(size,scale);
}

// entrypoint

void tick(lv*env);
void sync(void);

void io_init(void){
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | (nosound?0:SDL_INIT_AUDIO));
	SDL_EnableUNICODE(1);
	gil=SDL_CreateMutex();
	// CURSORS[0]=SDL_GetDefaultCursor();
	// CURSORS[1]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	// CURSORS[2]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	// CURSORS[3]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	if(!nosound){
		audio.freq=SFX_RATE;
		audio.format=SFX_FORMAT;
		audio.channels=SFX_CHANNELS;
		audio.samples=(SFX_RATE/10);
		audio.callback=sfx_pump;
		SDL_OpenAudio(&audio,NULL),SDL_PauseAudio(0);
	}
	SDL_AddTimer((1000/60),tick_pump,NULL);
}
void io_run(lv*env){
	while(!should_exit){tick(env);sync();}
	SDL_Quit();
}
