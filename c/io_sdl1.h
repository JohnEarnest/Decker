// compatibility shim for SDL 1.2

// many features have reduced or missing functionality:
// - Decker does not check the application base path for start.deck
// - Decker cannot open URI as by go["http://..."]
// - Decker cannot interact with the system clipboard; it instead uses an internal buffer
// - Decker cannot record audio
// - Decker does not automatically engage touch mode
// - Decker does not support file drag-and-drop
// - Decker does not support partial window opacity (tracing mode)
// - Decker does not support fullscreen (might work on some platforms?)

#include <SDL.h>
#define NO_TRACING
#define NO_FULLSCREEN   // try commenting this out, if you dare...
#ifndef SYNC_DUTY
#define SYNC_DUTY 2     // how many logical frames execute per rendered frame?
#endif

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
#define KEY_r            SDLK_r
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
#define KEYM_LGUI        KMOD_LMETA
#define KEYM_RGUI        KMOD_RMETA

// global interpreter lock

SDL_mutex*gil=NULL;
void interpreter_lock(void){SDL_LockMutex(gil);}
void interpreter_unlock(void){SDL_UnlockMutex(gil);}

// resources

void base_path(char*path){snprintf(path,PATH_MAX,"%s","");} // stubbed
void open_url(char*x){(void)x;} // stubbed

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

#define SFX_INPUT_FORMAT AUDIO_S8
#define SFX_OUTPUT_FORMAT AUDIO_S16SYS
#define SFX_CHANNELS 1
int nosound=0;
SDL_AudioSpec audio;

void sfx_pump(void*user,Uint8*stream,int len);
void record_pump(void* userdata,Uint8* stream,int len);

lv*n_readwav(lv*self,lv*a){
	(void)self;char*name=ls(l_first(a))->sv;
	Uint8* raw; Uint32 length; SDL_AudioSpec spec; SDL_AudioCVT cvt;
	if(SDL_LoadWAV(name,&spec,&raw,&length)==NULL)return sound_make(lms(0));
	if(SDL_BuildAudioCVT(&cvt, spec.format,spec.channels,spec.freq, SFX_INPUT_FORMAT,SFX_CHANNELS,SFX_RATE)){
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
void event_pointer_button(int primary,int middle,int down);
void event_file(char*p);
void field_input(char*text);

void process_events(pair disp,pair size,int scale){
	SDL_Event e;
	int can_tick=0;
	while(1){
		while(SDL_PollEvent(&e)){
			if(e.type==SDL_QUIT     )event_quit();
			if(e.type==SDL_USEREVENT)can_tick=1;
			if(e.type==SDL_KEYDOWN  ){
				char u=e.key.keysym.unicode, c=drom_from_codepoint(u);
				if(u>=32&&c)field_input((char[2]){c,'\0'});
				event_key(e.key.keysym.sym,e.key.keysym.mod,1,SDL_GetKeyName(e.key.keysym.sym));
			}
			if(e.type==SDL_KEYUP    )event_key(e.key.keysym.sym,e.key.keysym.mod,0,SDL_GetKeyName(e.key.keysym.sym));
			if(e.type==SDL_MOUSEMOTION){
				pair b={(disp.x-(size.x*scale))/2,(disp.y-(size.y*scale))/2};
				event_pointer_move((pair){e.motion.x,e.motion.y},(pair){(e.motion.x-b.x)/scale,(e.motion.y-b.y)/scale});
			}
			if(e.type==SDL_MOUSEBUTTONUP){event_pointer_button(e.button.button==SDL_BUTTON_LEFT,e.button.button==SDL_BUTTON_MIDDLE,0);}
			if(e.type==SDL_MOUSEBUTTONDOWN){
				if     (e.button.button==SDL_BUTTON_WHEELUP  ){event_scroll((pair){0, 1});}
				else if(e.button.button==SDL_BUTTON_WHEELDOWN){event_scroll((pair){0,-1});}
				else                                          {event_pointer_button(e.button.button==SDL_BUTTON_LEFT,e.button.button==SDL_BUTTON_MIDDLE,1);}
			}
		}
		if(can_tick)return;
		SDL_WaitEvent(NULL);
	}
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

void surface_blit(SDL_Surface*src,SDL_Surface*dst,pair pos,int scale){
	if(scale<=1){
		SDL_Rect dstrect={pos.x,pos.y,src->w,src->h};
		SDL_BlitSurface(src,NULL,dst,&dstrect);
		return;
	}
	SDL_LockSurface(src);SDL_LockSurface(dst);
	int*a=(int*)src->pixels,*b=(int*)dst->pixels;
	int w=src->w,h=src->h,ai=0,bi=pos.x+(pos.y*dst->w);
	for(int y=0;y<h;y++){
		int row=bi;
		for(int x=0;x<w;x++){for(int z=0;z<scale;z++)b[bi++]=a[ai];ai++;}
		bi+=dst->w-(w*scale);
		for(int z=0;z<scale-1;z++){memcpy(b+bi,b+row,w*scale*sizeof(int));bi+=dst->w;}
	}
	SDL_UnlockSurface(dst);SDL_UnlockSurface(src);
}

SDL_Surface*vid;
SDL_Surface*gfx;
SDL_Surface*gtool;
#include <SDL_image.h>
lv* readimage(char*path,int grayscale){
	SDL_Surface*b=IMG_Load(path);if(b==NULL)return image_empty();lv*i=lmbuff((pair){b->w,b->h});
	SDL_Surface*c=SDL_DisplayFormat(b);
	for(int y=0;y<b->h;y++)for(int x=0;x<b->w;x++){
		Uint32 v=((Uint32*)c->pixels)[x+(y*c->pitch/4)];Uint8 cr,cg,cb,ca;
		SDL_GetRGBA(v,c->format,&cr,&cg,&cb,&ca),i->sv[x+y*b->w]=(ca!=0xFF)?(grayscale?0xFF:0x00):readcolor(cr,cg,cb,grayscale);
	}SDL_FreeSurface(c),SDL_FreeSurface(b);return image_make(i);
}
void framebuffer_alloc(pair size,int minscale){
	(void)minscale;
	SDL_FreeSurface(gfx);
	gfx=SDL_CreateRGBSurface(0,size.x,size.y,32,0,0,0,0);
}

int sync_parity=0;
int framebuffer_flip(pair disp,pair size,int scale,int mask,int frame,char*pal,lv*buffer){
	sync_parity=(sync_parity+1)%SYNC_DUTY;
	if(sync_parity)return 0;
	SDL_LockSurface(gfx);
	draw_frame(pal,buffer,gfx->pixels,gfx->pitch,frame,mask);frame_count++;
	SDL_UnlockSurface(gfx);
	pair pos={(disp.x-scale*size.x)/2,(disp.y-scale*size.y)/2};
	surface_blit(gfx,vid,pos,scale);
	return 1;
}
void toolbar_flip(lv*buffer,int frame,char*pal,rect dest){
	if(sync_parity)return;
	if(!gtool){pair s=buff_size(buffer);gtool=SDL_CreateRGBSurface(0,s.x,s.y,32,0,0,0,0);}
	SDL_LockSurface(gtool);
	draw_frame(pal,buffer,gtool->pixels,gtool->pitch,frame,0);
	SDL_UnlockSurface(gtool);
	int scale=1; // TODO!
	surface_blit(gtool,vid,(pair){dest.x,dest.y},scale);
}
void finish_flip(void){SDL_Flip(vid);}

// windows

pair get_display_size(void){return (pair){1280,768};} // stubbed
int get_display_density(pair disp){(void)disp;return 1;} // stubbed
void window_set_title(char*x){SDL_WM_SetCaption(x,NULL);}
void window_set_opacity(float x){(void)x;} // stubbed
void window_set_cursor(int x){SDL_SetCursor(CURSORS[x]);}
void window_set_fullscreen(int full){vid=SDL_SetVideoMode(vid->w,vid->h,32,SDL_DOUBLEBUF | (full?SDL_FULLSCREEN:0));}
pair window_get_size(void){return (pair){vid->w,vid->h};}
void window_set_size(pair wsize,pair size,int scale){
	vid=SDL_SetVideoMode(size.x*scale,size.y*scale,32,SDL_DOUBLEBUF);
	window_set_title("Decker");
	framebuffer_alloc(size,scale);
	(void)wsize;
}

// entrypoint

void tick(lv*env);
void sync(void);

SDL_Cursor* makeCursor(pair offset,lv*image){
	pair s=image_size(image);
	Uint8*data=malloc((s.x/8)*s.y);
	Uint8*mask=malloc((s.y/8)*s.y);
	// see: https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlcreatecursor.html
	for(int y=0,i=0;y<s.y;y++)for(int bx=0;bx<s.x;bx+=8,i++){
		int d=0,m=0;for(int x=0;x<8;x++){
			int c=image->b->sv[x+bx+(s.x*y)];
			d=(d*2)+(c==1?1:0);
			m=(m*2)+(c>0 ?1:0);
		}
		data[i]=d,mask[i]=m;
	}
	return SDL_CreateCursor(data,mask,s.x,s.y,offset.x,offset.y);
}
void io_init(void){
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | (nosound?0:SDL_INIT_AUDIO));
	gil=SDL_CreateMutex();
	CURSORS[0]=makeCursor((pair){0,0},image_read(lmistr("%%IMG2ABAAESABAA8gAgAOIAEBASABAA0gAQECIAEADCABAQMgAQALIAEBBCABAAogAQEFIAEACSABAQYgAQAIIAEBByABAAcgAQEIIAEABiABAQUgBQAFIAEBAiABAQIgAQAJIAEBASABAAEgAQECIAEACCACAAIgAQECIAEACCABAAQgAQECIAEADCABAQIgAQANIAIACA==")));
	CURSORS[1]=makeCursor((pair){7,0},image_read(lmistr("%%IMG2ABAAEAAGAQMADAEBIAIBAgALAQEgAgECAAsBASACAQIACwEBIAIBAgALAQEgAgECAAcBAwABAQEgAgEFAAMBASACAQMgAgEBIAEBASABAQMAAgEBIAIBAiACAQEgAQEBIAEBASABAQIAAgEBIAIBASAGAQEgAQECAAIBASACAQEgCAECAAMBASAKAQIABAEBIAkBAgAEAQEgCAECAAYBASAHAQIABgEBIAcBAgAB")));
	CURSORS[2]=makeCursor((pair){3,4},image_read(lmistr("%%IMG0AAgAEMYoEBAQEBAQEBAQEBAQKMY=")));
	CURSORS[3]=makeCursor((pair){7,7},image_read(lmistr("%%IMG2ABAAEAAHIAIADSABAQIgAQALIAEBBCABAAkgAQEGIAEAByAEAQIgBAAFIAEBASABAAEgAQECIAEAASABAQEgAQADIAEBAiADAQIgAwECIAEAASABAQ4gAgEOIAEAASABAQIgAwECIAMBAiABAAMgAQEBIAEAASABAQIgAQABIAEBASABAAUgBAECIAQAByABAQYgAQAJIAEBBCABAAsgAQECIAEADSACAAc=")));
	if(!nosound){
		audio.freq=SFX_RATE;
		audio.format=SFX_OUTPUT_FORMAT;
		audio.channels=SFX_CHANNELS;
		audio.samples=(SFX_RATE/10);
		audio.callback=sfx_pump;
		SDL_OpenAudio(&audio,NULL),SDL_PauseAudio(0);
	}
	SDL_AddTimer((1000/60),tick_pump,NULL);
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
}
void io_run(lv*env){
	while(!should_exit){tick(env);sync();}
}
