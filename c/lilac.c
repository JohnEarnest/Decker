// Lilac: a graphical runtime for Lil
#include "lil.h"
#include "dom.h"
#include <SDL.h>

// State

lv*env=NULL;
lv*canv=NULL;
lv*sounds=NULL;
lv*fonts=NULL;
lv*patterns=NULL;

SDL_Window  *win;
SDL_Renderer*ren;
SDL_Texture *gfx;
SDL_Joystick*joy=NULL;

int windowed=1, toggle_fullscreen=0;
int dir_up=0,dir_dn=0,dir_lf=0,dir_rt=0, key_action=0,key_cancel=0;

// Event Mailbox

int pending_click  =0; pair arg_click  ={-1,-1};
int pending_drag   =0; pair arg_drag   ={-1,-1};
int pending_release=0; pair arg_release={-1,-1};
int pending_show   =0;
int pending_tick   =0;

// Helpers

#define serr(x) {if(x==NULL)printf("SDL error: %s\n",SDL_GetError()),exit(1);}
void base_dir(char*dest,int n,char*src,char*sub){snprintf(dest,n,"%s",src);int e=strlen(dest)-1;while(e>0&&dest[e]!='/')e--;dest[e]='\0';strcat(dest,sub);}
lv* key_from_filename(char*name){return l_drop(lmn(-4),l_last(l_split(lmistr("/"),lmcstr(name))));}

// Images

lv* img_read_images(char*sourcefile){
	lv*r=lmd();if(!sourcefile)return r;
	char base[4096]={0};base_dir(base,sizeof(base),sourcefile,"/images/");
	directory_fetch(base,filter_image);for(int z=0;z<directory_count;z++){
		char path[4096];snprintf(path,sizeof(path),"%s/%s",base,directory[z].name);
		lv*i=n_readgif(NULL,l_list(lmcstr(path)));pair s=image_size(i);
		if(s.x!=0&&s.y!=0)dset(r,key_from_filename(path),i);
	}return r;
}

// Audio

#define SFX_FORMAT   AUDIO_S8
#define SFX_CHANNELS 1
#define SFX_SLOTS    4

typedef struct {lv*clip; Uint32 sample; float volume;}clip_state;
clip_state audio_slots[SFX_SLOTS]={{0}};
float master_volume=1.0;
SDL_AudioSpec audio;
lv*playing=NULL;

void sfx_pump(void*user,Uint8*stream,int len){
	(void)user;for(int z=0;z<len;z++){
		float samples=0;for(int z=0;z<SFX_SLOTS;z++){
			if(audio_slots[z].clip==NULL)continue;
			int8_t*data=(int8_t*)audio_slots[z].clip->sv; // stored as uint, but samples are signed(!)
			int b=data[audio_slots[z].sample];
			samples+=(b/128.0)*audio_slots[z].volume;
			audio_slots[z].sample++;
			if(audio_slots[z].sample>=((Uint32)audio_slots[z].clip->c))audio_slots[z].clip=NULL;
		}
		stream[z]=0xFF&(int)((master_volume*(samples/SFX_SLOTS))*120);
	}
}
int sfx_any(){for(int z=0;z<SFX_SLOTS;z++)if(audio_slots[z].clip!=NULL)return 1;return 0;}
void sfx_init(){
	audio.freq=SFX_RATE,audio.format=SFX_FORMAT,audio.channels=SFX_CHANNELS,audio.samples=(SFX_RATE/10),audio.callback=sfx_pump;
	SDL_OpenAudio(&audio,NULL),SDL_PauseAudio(0);
}
lv* sfx_readwav(char*name){
	Uint8* raw; Uint32 length; SDL_AudioSpec spec; SDL_AudioCVT cvt;
	if(SDL_LoadWAV(name,&spec,&raw,&length)==NULL)return NULL;
	if(SDL_BuildAudioCVT(&cvt, spec.format,spec.channels,spec.freq, SFX_FORMAT,SFX_CHANNELS,SFX_RATE)){
		cvt.len=length;
		cvt.buf=malloc(cvt.len * cvt.len_mult);
		memcpy(cvt.buf,raw,length);
		SDL_FreeWAV(raw);
		SDL_ConvertAudio(&cvt);
		raw=cvt.buf, length=cvt.len_cvt;
	}lv*r=lmv(1);r->c=length;r->sv=(char*)raw;return sound_make(r);
}
void sfx_read_sounds(char*sourcefile){
	if(!sourcefile)return;char base[4096]={0};base_dir(base,sizeof(base),sourcefile,"/sounds/");
	directory_fetch(base,filter_sound);for(int z=0;z<directory_count;z++){
		char path[4096];snprintf(path,sizeof(path),"%s/%s",base,directory[z].name);
		lv*sound=sfx_readwav(path);if(sound)dset(sounds,key_from_filename(path),sound);
	}
}
lv* n_play(lv*self,lv*z){
	(void)self;lv*x=l_first(z),*sfx=sound_is(x)?x: dget(sounds,ls(x));if(!sfx)return x;
	int max_sample=0;int avail=-1;for(int z=0;z<SFX_SLOTS;z++){
		if(!audio_slots[z].clip){avail=z;break;}
		if(audio_slots[z].sample>audio_slots[max_sample].sample)max_sample=z;
	}audio_slots[avail!=-1?avail:max_sample]=(clip_state){sfx->b,0,1.0};
	return x;
}

// Standard Library

void go_notify(lv*deck,lv*args,int dest){(void)deck,(void)args,(void)dest;}
lv* n_print  (lv*self,lv*a){(void)self;return n_printf(a,1,stdout);}
lv* n_alert  (lv*self,lv*z){(void)self,(void)z;return ONE;}
lv* n_open   (lv*self,lv*z){(void)self,(void)z;return lmistr("");}
lv* n_save   (lv*self,lv*z){(void)self,(void)z;return NONE;}
lv* n_show(lv*self,lv*a){
	(void)self;str s=str_new();EACH(z,a){if(z)str_addc(&s,' ');show(&s,a->lv[z],a->c==1);}
	printf("%s\n",lmstr(s)->sv);return l_first(a);
}

void setSize(){
	pair s=getpair(ifield(canv,"size")),l=getpair(ifield(canv,"lsize"));
	if(gfx)SDL_DestroyTexture(gfx);
	gfx=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,l.x,l.y);
	SDL_SetWindowSize(win,s.x,s.y);
	SDL_SetWindowPosition(win,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
}
lv*n_app_exit (lv*self,lv*z){(void)self,(void)z;exit(0);return NONE;}
lv*interface_app(lv*self,lv*i,lv*x){
	if(x){
		ikey("title"){SDL_SetWindowTitle(win,ls(x)->sv);}
	}else{
		ikey("title")return lmcstr((char*)SDL_GetWindowTitle(win));
		ikey("exit" )return lmnat(n_app_exit,self);
	}return x?x:NONE;
}
lv*interface_lilac_canvas(lv*self,lv*i,lv*x){
	lv*r=interface_canvas(self,i,x);
	if(x&&self==canv){
		ikey("size" )setSize();
		ikey("lsize")setSize();
		ikey("scale")setSize();
	}return r;
}
lv*interface_keys(lv*self,lv*i,lv*x){
	(void)x;
	ikey("dir"){pair d={0,0};if(dir_lf)d.x--;if(dir_rt)d.x++;if(dir_up)d.y--;if(dir_dn)d.y++;return lmpair(d);}
	ikey("action")return lmn(key_action);
	ikey("cancel")return lmn(key_cancel);
	return x?x:NONE; (void)self;
}

// Runtime

Uint32 tick_pump(Uint32 interval,void*param){
	SDL_Event e; SDL_UserEvent u;
	u.type=e.type=SDL_USEREVENT;
	u.data1=param;
	e.user=u;
	SDL_PushEvent(&e);
	return interval;
}
void dokeys(int code,int state){
	if(code==SDLK_ESCAPE&&state)exit(0);
	if(code==SDLK_UP   ||code==SDLK_w)dir_up=state;
	if(code==SDLK_DOWN ||code==SDLK_s)dir_dn=state;
	if(code==SDLK_LEFT ||code==SDLK_a)dir_lf=state;
	if(code==SDLK_RIGHT||code==SDLK_d)dir_rt=state;
	if(code==SDLK_SPACE||code==SDLK_RETURN||code==SDLK_e||code==SDLK_z)key_action=state;
	if(code==SDLK_q||code==SDLK_x)key_cancel=state;
	if(state==0&&code==SDLK_f)toggle_fullscreen=1;
}
void sync(){
	int dw, dh;
	SDL_GetWindowSize(win,&dw,&dh);
	lv* buffer=canvas_image(canv,1)->b;
	pair size=buff_size(buffer);
	int scale=MIN(dw/size.x,dh/size.y);
	SDL_Event e;
	while(SDL_WaitEvent(&e)){
		if(e.type==SDL_QUIT)exit(0);
		if(e.type==SDL_USEREVENT)break;
		if(e.type==SDL_KEYDOWN){dokeys(e.key.keysym.sym,1);}
		if(e.type==SDL_KEYUP  ){dokeys(e.key.keysym.sym,0);}
		if(e.type==SDL_MOUSEMOTION){
			int bw=(dw-(size.x*scale))/2, bh=(dh-(size.y*scale))/2;
			if(!pending_drag)pointer_prev=pointer;
			pointer=(pair){(e.motion.x-bw)/scale,(e.motion.y-bh)/scale};
			if(pointer_held)pending_drag=1,arg_drag=pointer;
		}
		if(e.type==SDL_MOUSEBUTTONUP  ){pointer_held=0;pending_release=1;arg_release=pointer;pointer_end  =pointer;}
		if(e.type==SDL_MOUSEBUTTONDOWN){pointer_held=1;pending_click  =1;arg_click  =pointer;pointer_start=pointer;}
		if(e.type==SDL_JOYDEVICEADDED  &&e.jdevice.which==0&&joy==NULL){joy=SDL_JoystickOpen(0);}
		if(e.type==SDL_JOYDEVICEREMOVED&&e.jdevice.which==0&&joy!=NULL){SDL_JoystickClose(joy);joy=NULL;}
		if(e.type==SDL_JOYBUTTONDOWN){if(e.jbutton.button%2)key_action=1;else key_cancel=1;}
		if(e.type==SDL_JOYBUTTONUP  ){if(e.jbutton.button%2)key_action=0;else key_cancel=0;}
		if(e.type==SDL_JOYAXISMOTION&&joy!=NULL){
			#define DEAD_ZONE ((int)(32768*0.2))
			pair axis={SDL_JoystickGetAxis(joy,0),SDL_JoystickGetAxis(joy,1)};
			dir_up=axis.y<-DEAD_ZONE, dir_dn=axis.y> DEAD_ZONE, dir_lf=axis.x<-DEAD_ZONE, dir_rt=axis.x> DEAD_ZONE;
		}
	}
	SDL_FlushEvent(SDL_USEREVENT);
	if(toggle_fullscreen){
		toggle_fullscreen=0;
		windowed=!windowed;
		SDL_SetWindowFullscreen(win,windowed?0:SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	int* p, pitch;
	SDL_LockTexture(gfx,NULL,(void**)&p,&pitch);
	draw_frame(patterns_pal(patterns),buffer,p,pitch,frame_count++,0);
	SDL_UnlockTexture(gfx);
	SDL_Rect src={0,0,size.x,size.y};
	SDL_Rect dst={(dw-scale*size.x)/2,(dh-scale*size.y)/2,scale*size.x,scale*size.y};
	SDL_SetRenderDrawColor(ren,0x00,0x00,0x00,0xFF);
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren,gfx,&src,&dst);
	SDL_RenderPresent(ren);
	pending_tick=1;
}

#include <unistd.h>
#include <sys/select.h>
void tick(lv*env){
	if(sleep_play&&sfx_any())return;sleep_play=0;
	if(sleep_frames){sleep_frames--;return;}
	int quota=10*4096;
	while(1){
		while(running()&&sleep_frames==0&&sleep_play==0&&quota>0){runop();quota--;}
		EACH(z,playing)playing->lv[z]=audio_slots[z].clip?audio_slots[z].clip:NONE;
		if(!running()){lv*a=arg();if(pending_show){pending_show=0;dset(env,lmistr("_"),a);debug_show(a);printf("  ");fflush(stdout);}lv_collect();}
		if(quota<=0||sleep_frames||sleep_play)break;
		else if(pending_click  ){pending_click  =0;lv*f=dget(env,lmistr("click"  ));runfunc(env,f,l_list(lmpair(arg_click  )));}
		else if(pending_drag   ){pending_drag   =0;lv*f=dget(env,lmistr("drag"   ));runfunc(env,f,l_list(lmpair(arg_drag   )));}
		else if(pending_release){pending_release=0;lv*f=dget(env,lmistr("release"));runfunc(env,f,l_list(lmpair(arg_release)));}
		else{
			fd_set rfds;struct timeval tv;FD_ZERO(&rfds);FD_SET(0, &rfds);tv.tv_sec=0;tv.tv_usec=5000;
			if(select(1, &rfds, NULL, NULL, &tv)){
				str line=str_new();char c;
				while(read(0,&c,1)>0){if(c=='\n')break;str_addc(&line,c);}
				lv*text=lmstr(line);lv*prog=parse(text->sv);
				if(perr()){for(int z=0;z<par.c+2;z++)printf(" ");printf("^\n%s\n",par.error);}
				else{runexpr(env,prog);pending_show=1;}
			}
			else if(pending_tick){pending_tick=0;runfunc(env,dget(env,lmistr("tick")),lml(0));}
		}
		if(!running())break;
	}
}

int main(int argc,char**argv){
	init_interns();
	env=lmenv(NULL);
	init(env);
	lv*deck=deck_read(lmistr("{card:home}\n{widgets}\ng:{\"type\":\"canvas\"}"));
	lv*card=l_first(dget(deck->b,lmistr("cards")));
	canv=l_first(dget(card->b,lmistr("widgets")));canv->f=(void*)interface_lilac_canvas;
	fonts=dget(deck->b,lmistr("fonts"));
	sounds=dget(deck->b,lmistr("sounds"));sfx_read_sounds(argc>=2?argv[1]:NULL);
	playing=l_take(lmn(SFX_SLOTS),NONE);dset(env,lmistr("!playing"),playing);
	patterns=dget(deck->b,lmistr("patterns"));
	dset(env,lmistr("deck"    ),deck);
	dset(env,lmistr("sounds"  ),sounds);
	dset(env,lmistr("fonts"   ),fonts);
	dset(env,lmistr("canvas"  ),canv);
	dset(env,lmistr("patterns"),patterns);
	dset(env,lmistr("images"  ),img_read_images(argc>=2?argv[1]:NULL));
	dset(env,lmistr("show"    ),lmnat(n_show,    NULL));
	dset(env,lmistr("print"   ),lmnat(n_print,   NULL));
	dset(env,lmistr("play"    ),lmnat(n_play,    NULL));
	dset(env,lmistr("sleep"   ),lmnat(n_sleep,   NULL));
	dset(env,lmistr("eval"    ),lmnat(n_eval,    NULL));
	dset(env,lmistr("random"  ),lmnat(n_random,  NULL));
	dset(env,lmistr("array"   ),lmnat(n_array,   NULL));
	dset(env,lmistr("image"   ),lmnat(n_image,   NULL));
	dset(env,lmistr("sound"   ),lmnat(n_sound,   NULL));
	dset(env,lmistr("readcsv" ),lmnat(n_readcsv, NULL));
	dset(env,lmistr("writecsv"),lmnat(n_writecsv,NULL));
	dset(env,lmistr("readxml" ),lmnat(n_readxml, NULL));
	dset(env,lmistr("writexml"),lmnat(n_writexml,NULL));
	dset(env,lmistr("app"     ),lmi(interface_app,    lmistr("app"     ),NULL));
	dset(env,lmistr("pointer" ),lmi(interface_pointer,lmistr("pointer" ),NULL));
	dset(env,lmistr("keys"    ),lmi(interface_keys,   lmistr("keys"    ),NULL));
	constants(env);
	if(argc>=2){
		struct stat st;if(stat(argv[1],&st)){fprintf(stderr,"unable to read %s.",argv[1]);exit(1);}
		char*text=malloc(st.st_size+1);FILE*f=fopen(argv[1],"r");
		fread(text,1,st.st_size,f),fclose(f),text[st.st_size]='\0';
		lv*prog=parse(text);
		if(perr()){fprintf(stderr,"cannot load %s: (%d:%d) %s\n",argv[1],par.r+1,par.c+1,par.error);exit(1);}
		else{runexpr(env,prog);}
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
	pair size=getpair(ifield(canv,"size"));
	win=SDL_CreateWindow(
		"Lilac",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		size.x,
		size.y,
		SDL_WINDOW_SHOWN
	);serr(win);
	// on new machines, the software renderer is fast enough.
	// on old machines, accelerated rendering can be extremely slow.
	ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);serr(ren);
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_AddTimer((1000/60),tick_pump,NULL);
	sfx_init();
	iwrite(canv,lmistr("size"),lmpair((pair){160,160}));
	printf("  ");fflush(stdout);
	while(1){tick(env);sync();}
}
