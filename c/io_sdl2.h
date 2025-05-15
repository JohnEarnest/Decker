
#include <SDL.h>

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
#define KEY_LGUI         SDLK_LGUI
#define KEY_RGUI         SDLK_RGUI
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

#define KEYM_LSHIFT      KMOD_LSHIFT
#define KEYM_RSHIFT      KMOD_RSHIFT
#define KEYM_LCTRL       KMOD_LCTRL
#define KEYM_RCTRL       KMOD_RCTRL
#define KEYM_LALT        KMOD_LALT
#define KEYM_RALT        KMOD_RALT
#define KEYM_LGUI        KMOD_LGUI
#define KEYM_RGUI        KMOD_RGUI

// global interpreter lock

SDL_mutex*gil=NULL;
void interpreter_lock(void){SDL_LockMutex(gil);}
void interpreter_unlock(void){SDL_UnlockMutex(gil);}

// resources

void base_path(char*path){
	char*t=SDL_GetBasePath();
	if(t){snprintf(path,PATH_MAX,"%s",t);SDL_free(t);}else{snprintf(path,PATH_MAX,"");}
}
void open_url(char*x){
	(void)x;
	#if SDL_VERSION_ATLEAST(2,0,14)
		int e=SDL_OpenURL(x);
		if(e)printf("open url error: %s\n",SDL_GetError());
	#endif
}

// clipboard

lv* get_clip(void){char*t=SDL_GetClipboardText();lv*r=lmutf8(t);SDL_free(t);return r;}
void set_clip(lv*x){SDL_SetClipboardText(drom_to_utf8(x)->sv);}

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

int record_possible(void){return SDL_GetNumAudioDevices(1)>=1;}
int record_begin(void){
	SDL_AudioSpec spec;SDL_zero(spec);
	spec.freq=SFX_RATE,spec.format=SFX_FORMAT,spec.channels=SFX_CHANNELS,spec.samples=1024,spec.callback=record_pump;
	return SDL_OpenAudioDevice(NULL,1,&spec,NULL,0);
}
void record_pause(int device){SDL_PauseAudioDevice(device,0);}
void record_finish(int device){SDL_PauseAudioDevice(device,1);}

// input events

void event_quit(void);
void event_touch(void);
void event_key(int c,int m,int down,const char*name);
void event_scroll(pair s);
void event_pointer_move(pair raw,pair scaled);
void event_pointer_button(int primary,int middle,int down);
void event_file(char*p);
void field_input(char*text);

void process_events(pair disp,pair size,int scale){
	SDL_Event e;
	while(SDL_WaitEvent(&e)){
		if(e.type==SDL_QUIT     )event_quit();
		if(e.type==SDL_USEREVENT)break;
		if(e.type==SDL_TEXTINPUT)field_input(lmutf8(e.text.text)->sv);
		if(e.type==SDL_KEYDOWN  )event_key(e.key.keysym.sym,e.key.keysym.mod,1,SDL_GetKeyName(e.key.keysym.sym));
		if(e.type==SDL_KEYUP    )event_key(e.key.keysym.sym,e.key.keysym.mod,0,SDL_GetKeyName(e.key.keysym.sym));
		if(e.type==SDL_MOUSEMOTION){
			pair b={(disp.x-(size.x*scale))/2,(disp.y-(size.y*scale))/2};
			event_pointer_move((pair){e.motion.x,e.motion.y},(pair){(e.motion.x-b.x)/scale,(e.motion.y-b.y)/scale});
		}
		if(e.type==SDL_MOUSEWHEEL     )event_scroll((pair){e.wheel.x,e.wheel.y});
		if(e.type==SDL_MOUSEBUTTONDOWN)event_pointer_button(e.button.button==SDL_BUTTON_LEFT,e.button.button==SDL_BUTTON_MIDDLE,1);
		if(e.type==SDL_MOUSEBUTTONUP  )event_pointer_button(e.button.button==SDL_BUTTON_LEFT,e.button.button==SDL_BUTTON_MIDDLE,0);
		if(e.type==SDL_FINGERDOWN     )event_touch();
		if(e.type==SDL_DROPFILE       )event_file(e.drop.file),SDL_free(e.drop.file);
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

SDL_Renderer*ren;
SDL_Texture*gfx;
SDL_Texture*gtool;
#include <SDL_image.h>
lv* readimage(char*path,int grayscale){
	SDL_Surface*b=IMG_Load(path);if(b==NULL)return image_empty();lv*i=lmbuff((pair){b->w,b->h});
	SDL_Surface*c=SDL_ConvertSurface(b,SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888),0);
	for(int y=0;y<b->h;y++)for(int x=0;x<b->w;x++){
		Uint32 v=((Uint32*)c->pixels)[x+(y*c->pitch/4)];Uint8 cr,cg,cb,ca;
		SDL_GetRGBA(v,c->format,&cr,&cg,&cb,&ca),i->sv[x+y*b->w]=(ca!=0xFF)?(grayscale?0xFF:0x00):readcolor(cr,cg,cb,grayscale);
	}SDL_FreeSurface(c),SDL_FreeSurface(b);return image_make(i);
}
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
pair get_display_size(void){SDL_DisplayMode dis;SDL_GetDesktopDisplayMode(0,&dis);return (pair){dis.w,dis.h};}
int get_display_density(pair disp){
	(void)disp;
	#if SDL_VERSION_ATLEAST(2,26,0)
		pair disp_pixels={0,0};SDL_GetWindowSizeInPixels(win,&disp_pixels.x,&disp_pixels.y);
		if(disp_pixels.x>disp.x)return disp_pixels.x/disp.x;
	#endif
	return 1;
}
void window_set_title(char*x){SDL_SetWindowTitle(win,x);}
void window_set_opacity(float x){SDL_SetWindowOpacity(win,x);}
void window_set_cursor(int x){SDL_SetCursor(CURSORS[x]);}
void window_set_fullscreen(int full){SDL_SetWindowFullscreen(win,full?SDL_WINDOW_FULLSCREEN_DESKTOP:0);}
pair window_get_size(void){pair r={0,0};SDL_GetWindowSize(win,&r.x,&r.y);return r;}
void window_set_size(pair wsize,pair size,int scale){
	if(win){SDL_SetWindowSize(win,wsize.x,wsize.y),SDL_DestroyTexture(gfx);}
	else{
		win=SDL_CreateWindow("Decker",
			SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,size.x*scale,size.y*scale,
			SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI
		);
		ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);
	}
	framebuffer_alloc(size,scale);
	SDL_SetWindowPosition(win,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
}

// entrypoint

void tick(lv*env);
void sync(void);

void io_init(void){
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | (nosound?0:SDL_INIT_AUDIO));
	gil=SDL_CreateMutex();
	CURSORS[0]=SDL_GetDefaultCursor();
	CURSORS[1]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	CURSORS[2]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	CURSORS[3]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
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
}
