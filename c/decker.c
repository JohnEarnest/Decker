// Decker
#include "lil.h"
#include "dom.h"

int should_exit=0;

#include <SDL_version.h>
#if SDL_VERSION_ATLEAST(2,0,0)
#include "io_sdl2.h"
#else
#include "io_sdl1.h"
#endif

void save_deck(lv*path); // forward refs
void load_deck(lv*deck);
void quit(void);

// Assets

lv*CHECK,*LOCK,*ANIM,*ZOOM,*CHECKS[4],*CORNERS[4],*RADIOS[4],*ICONS[8],*GESTURES[4],*HANDLES[2],*ACCENTS[10];
lv*FONT_BODY,*FONT_MENU,*FONT_MONO,*TOOLS,*ARROWS,*TOOLB,*PLAYING,*ATTRS;
enum mini_icons {icon_dir,icon_doc,icon_sound,icon_font,icon_app,icon_lil,icon_pat,icon_chek,icon_none};
enum cursor_styles {cursor_default,cursor_point,cursor_ibeam,cursor_drag};
int uicursor=0, enable_touch=0, set_touch=0;
int set_tracing=0, tracing=0, toolbar_scroll=0, toolbars_enable=0;
#define PROFILE_HIST_SZ 200
int profiler=0, profiler_ix=0, profiler_hist[PROFILE_HIST_SZ]={0};

char*TOOL_ICONS=
	"%%IMG0ABAAwAMABIAEgASABIAEgGTwlKxMqiQKJAIQAggCCAQEBAQEAAAAAAAAAAA//EACgAGAAYABgAFAAz/+H/wAAAAAAA"
	"AAAAAA8+eAAYABAAAAAIABgAGAAQAAAACAAYAB588AAADwAIgBCAGQAnACIAQgBEAIQAiAEIARAB4AHAAYABAAAAAAAAP4HA"
	"YgAUABgAGAAoAcuOBnAFQAOAAIAAgAEAAAAAAAAADAADAADAADAADAADAADAADAAAAAAAAAAAAAAcACIAJgArADLAInBCOIU"
	"dAh4APgBdAJyBHEIcJBgYEAAAAADgcVjar69VVqqvVVaqrVVZqrDVYGrAHwAAAAAAAAAAA//+AAYABgAGAAYABgAGAAYABgA"
	"H//wAAAAAAAAAAAAD//6qr1VWqq9VVqqvVVaqr1VWqq///AAAAAAAAAAAAAAAAB+AYGCAEQAKAAYABgAFAAiAEGBgH4AAAAA"
	"AAAAAAAAAH4B1YKqxVVqqr1VWqq1VWKqwdWAfgAAAAAA==";
char*ARROW_ICONS=
	"%%IMG0AAwAYAAABgAJARCCIERAL/DxEIEQgRCBH4AAAAABH4EQgRCBEI/w9EAiIEEQgAkABgAAAAAABgAPAR+DP8d/7//xH4"
	"EfgR+BH4AAAAABH4EfgR+BH4//93/jP8EfgA8ABgAAAAIABgAKARPiICRAJEAiICET4AoABgACAAQABgAFB3yEQERAJEAkQE"
	"d8gAUABgAEAAIABgAOAR/jP+d/53/jP+Ef4A4ABgACAAQABgAHB3+Hf8d/53/nf8d/gAcABgAEAA==";

char*pangram="How razorback jumping-frogs can level six piqued gymnasts.";

// State

lv*env,*deck=NULL,*doc_hist; int doc_hist_cursor=0;
cstate context;
int audio_playing=0,windowed=1,toggle_fullscreen=0;
int autosave=0, noscale=0, dirty=0, dirty_timer=0; char document_path[PATH_MAX]={0};
#define AUTOSAVE_DELAY (10*60)
lv* deck_get(lv*text){interpreter_lock();lv*r=deck_read(text);interpreter_unlock();return r;}
void mark_dirty(void){dirty=1,dirty_timer=AUTOSAVE_DELAY;}
void set_path(char*path){
	snprintf(document_path,PATH_MAX,"%s",path);
	char t[4096];snprintf(t,sizeof(t),"Decker - %s",path);
	window_set_title(strlen(path)?t:"Decker");
}

typedef struct {
	int pending_drag;
	int pending_halt;
	int pending_view, next_view, overshoot;
	int pending_loop;
	lv* target_click;    fpair arg_click;
	lv* target_drag;     fpair arg_drag, lastdrag;
	lv* target_release;  fpair arg_release;
	lv* target_order;    lv*  arg_order;
	lv* target_run;      lv*  arg_run;
	lv* target_link;     lv*  arg_link;
	lv* target_ccell;    lv*  arg_ccell;
	lv* target_change;   lv*  arg_change;
	lv* target_navigate; lv*  arg_navigate;
} message_state; message_state msg={0};

#define FIELD_CURSOR_DUTY  20
#define FIELD_CHANGE_DELAY 15
typedef struct {
	int active, count;
	int scrolls,thumbid,thumbo;
	int col_drag,col_num,col_orig;
	int ingrid;  grid  g; lv*gt; grid_val*gv;  grid_val  gv_slot;
	int infield; field f; lv*ft; field_val*fv; field_val fv_slot;
	pair cursor; int cursor_timer;
	lv* hist; int hist_cursor;
	int field_dirty, change_timer;
	int pending_grid_edit;
} widget_state; widget_state wid={0};

#define DOUBLE_CLICK_DELAY 20
enum dirs{dir_none,dir_left,dir_right,dir_up,dir_down};
typedef struct {
	int mu,md, clicktime, down_modal, down_uimode, down_caps;
	int click, rdown, rup, mdown; pair pos, rawpos, opos;
	int dclick, clicklast;
	int drag;  pair dpos, rawdpos, odpos;
	int tab, shift, alt, action, dir, exit, eval;
	int shortcuts[256];
	int scroll; int hidemenu;
} event_state; event_state ev={0};
int over(rect r){return box_in(r,ev.pos);}
int dover(rect r){return box_in(r,ev.dpos);}

enum tools{tool_select,tool_pencil,tool_lasso,tool_line,tool_fill,tool_poly,tool_rect,tool_fillrect,tool_ellipse,tool_fillellipse};
typedef struct {
	int tool, brush, pattern, fill, erasing;
	int show_widgets, show_anim, trans, trans_mask, under, color, fatbits; pair offset;
	int show_grid, snap; pair grid_size;
	rect sel_here, sel_start; lv*limbo; int limbo_dither;
	lv* scratch, *mask, *omask;
	int pickfill, zoom, lasso_dirty;
} draw_state;
draw_state ddr={tool_pencil,0,1,0,0, 1,1,0,0,0,0,0,{0}, 0,0,{16,16}, {0},{0},NULL,0, NULL,NULL,NULL, 0, 4, 0};
draw_state dr ={tool_pencil,0,1,0,0, 1,1,0,0,0,0,0,{0}, 0,0,{16,16}, {0},{0},NULL,0, NULL,NULL,NULL, 0, 4, 0};
int bg_pat(void){return dr.trans_mask&&dr.pattern==0?32:dr.pattern;}
int bg_fill(void){return dr.trans_mask&&dr.fill==0?32:dr.fill;}
int bg_has_sel(void){return dr.tool==tool_select&&(dr.sel_here.w>0||dr.sel_here.h>0);}
int bg_has_lasso(void){return dr.tool==tool_lasso&&dr.mask!=NULL;}
pair snap(pair p){return !dr.snap?p:(pair){dr.grid_size.x*((p.x+dr.grid_size.x/2)/dr.grid_size.x),dr.grid_size.y*((p.y+dr.grid_size.y/2)/dr.grid_size.y)};}
pair snap_delta(pair p){pair a=snap(p);return (pair){a.x-p.x,a.y-p.y};}
rect snapp(rect r){pair a=snap((pair){r.x,r.y});                        return (rect){a.x,a.y,r.w,r.h};} // position only
rect snapr(rect r){pair a=snap((pair){r.x,r.y}),b=snap((pair){r.w,r.h});return (rect){a.x,a.y,b.x,b.y};} // position + dimensions
cstate draw_buffer(lv*buff){pair size=buff_size(buff);return (cstate){0,0,size,(rect){0,0,size.x,size.y},buff,FONT_BODY};}
void draw_invert(char*pal,rect r){
	for(int a=r.y;a<r.y+r.h;a++)for(int b=r.x;b<r.x+r.w;b++)if(inclip(b,a)){int p=draw_pattern(pal,PIX(b,a),b,a);PIX(b,a)=p==0||p==32?1:32;}
}

enum uimodes{mode_interact,mode_draw,mode_object,mode_script};
int uimode=mode_interact;lv*ui_container=NULL;
void setuimode(int mode);// forward ref
void con_set(lv*x){
	if(x!=ui_container)setuimode(uimode),msg.next_view=1;
	if(x!=ui_container&&prototype_is(ui_container))n_prototype_update(ui_container,LNIL);ui_container=x;
}
lv* con(void){return ui_container?ui_container:ifield(deck,"card");}
lv* con_wids(void){return ivalue(con(),"widgets");}
lv* con_image(void){return ifield(con(),"image");}
pair con_size(void){return getpair(ifield(con(),"size"));}
rect con_dim(void){return rect_pair((pair){0,0},con_size());}
#define FAT (dr.zoom)
rect con_clip(void){
	pair size=con_size();
	if(dr.fatbits)return box_center(frame.clip,pair_min(frame.size,(pair){size.x*FAT,size.y*FAT}));
	return box_intersect(frame.clip,box_center(frame.clip,size));
}
pair con_offset(void){rect r=con_clip();return (pair){r.x,r.y};}
pair con_to_screen  (pair a){if(dr.fatbits){a=pair_sub(a,dr.offset);a.x*=FAT,a.y*=FAT                  ;}return pair_add(a,con_offset());}
rect con_to_screenr (rect a){if(dr.fatbits){a=rect_sub(a,dr.offset);a.x*=FAT,a.y*=FAT,a.w*=FAT,a.h*=FAT;}return rect_add(a,con_offset());}
fpair con_to_screenf(fpair a){pair r=con_to_screen((pair){a.x,a.y});return (fpair){r.x,r.y};}
pair screen_to_con (pair a){a=pair_sub(a,con_offset());return dr.fatbits?pair_add((pair){a.x/FAT,a.y/FAT},dr.offset):a;}
rect screen_to_conr(rect a){return rect_pair(screen_to_con((pair){a.x,a.y}),dr.fatbits?(pair){a.w/FAT,a.h/FAT}:(pair){a.w,a.h});}
rect con_view_dim(void){pair a=screen_to_con((pair){0,0}), b=screen_to_con(frame.size);return rect_pair(a,pair_sub(b,a));}
void clamp_fatbits(void){
	pair v=(pair){frame.size.x/FAT,frame.size.y/FAT}, c=con_size(), o=pair_max(pair_sub(c,v),(pair){0,0});
	dr.offset=(pair){CLAMP(0,dr.offset.x,o.x),CLAMP(0,dr.offset.y,o.y)};
}
void center_fatbits(pair p){
	pair v=(pair){frame.size.x/FAT,frame.size.y/FAT};
	dr.offset=(pair){p.x-v.x/2,p.y-v.y/2};clamp_fatbits();
}
event_state ev_to_con(event_state e){
	e.pos =screen_to_con(e.opos =e.pos);
	e.dpos=screen_to_con(e.odpos=e.dpos);
	pointer_prev=screen_to_con(pointer_prev);
	return e;
}
event_state con_to_ev(event_state e){
	e.pos =e.opos;
	e.dpos=e.odpos;
	pointer_prev=con_to_screen(pointer_prev);
	return e;
}

void tracking(void){
	lv*c=con();
	if(prototype_is(c)){
		lv*defs=ifield(deck,"contraptions");int i=dgeti(defs,ifield(c,"name"));
		if(ev.dir==dir_left )con_set(defs->lv[(i+(defs->c-1))%defs->c]);
		if(ev.dir==dir_right)con_set(defs->lv[(i+1          )%defs->c]);
	}
	if(card_is(c)){
		if(ev.dir==dir_left )n_go(deck,l_list(lmistr("Prev")));
		if(ev.dir==dir_right)n_go(deck,l_list(lmistr("Next")));
	}
}

#define LISTEN_LINES 30
typedef struct {
	lv*hist, *vars;
	int scroll;
} listen_state; listen_state li={0};

typedef struct {
	lv* sel; int show_bounds, show_names, show_cursor, show_margins, show_guides;
	int move, move_first;
	int resize, resize_first, handle; pair prev;
	rect orig;
} obj_state; obj_state ob={NULL,1,0,0,0,1, 0,0,0,0,-1,{0,0},{0,0,0,0}};

typedef struct {
	lv* target, *others, *next; field_val f;
	int prev_mode, xray;
	char status[4096];
} script_state; script_state sc={NULL,NULL,NULL,{0},0,0,{0}};
void script_save(lv*x){
	lv*k=lmistr("script");mark_dirty();
	if(sc.target)iwrite(sc.target,k,x);
	if(sc.others&&sc.others->c)EACH(z,sc.others)iwrite(sc.others->lv[z],k,x);
}

enum recorder_mode{record_stopped,record_playing,record_recording};
typedef struct {
	lv* target; int input;
	int mode, head; pair sel;
	lv* hist; int hist_cursor;
} recording_state; recording_state au={0};

typedef struct {char*name; int enabled;                      rect t,b;} menu_head;
typedef struct {char*name; int enabled,check; char shortcut; rect t,b;} menu_entry;
typedef struct {
	menu_head  heads[32]; int head_count;
	menu_entry items[32]; int item_count;
	int x, active, stick, lw; rect sz;
} menu_state; menu_state menu={{{0}},0,{{0}},0, -1,-1,-1,0, {0,0,0,0}};
rect menu_label(void){return (rect){menu.x,1,context.size.x-menu.x-2,1+font_h(FONT_MENU)};}

enum modal_type{
	modal_none,modal_about,modal_query,modal_listen,modal_link,modal_gridcell,modal_open,modal_save,
	modal_alert,modal_confirm,modal_input,modal_url,
	modal_alert_lil,modal_confirm_lil,modal_input_lil,modal_choose_lil,modal_open_lil,modal_save_lil,
	modal_import_image,modal_export_image,
	modal_import_script,modal_export_script,
	modal_import_sound,
	modal_import_deck,
	modal_open_deck,modal_save_deck,modal_save_locked,modal_confirm_quit,modal_confirm_new,modal_confirm_script,modal_multiscript,
	modal_brush,modal_pattern,modal_fill,
	modal_deck_props,modal_card_props,modal_button_props,modal_field_props,modal_slider_props,modal_canvas_props,modal_grid_props,modal_contraption_props,
	modal_cards,modal_sounds,modal_recording,modal_fonts,modal_contraptions,modal_resources,modal_orderwids,
	modal_action,modal_pick_card,modal_pick_contraption,modal_trans,modal_grid,modal_prototype_props,modal_prototype_attrs,
};
typedef struct {
	int type, subtype, filter, in_modal, edit_json;
	widget_state old_wid;
	grid_val grid,grid2;field_val text,name,form0,form1,form2;
	char*desc;char path[PATH_MAX],path_suffix[PATH_MAX];
	lv*message,*verb;pair cell;
	int from_listener, from_action, from_keycaps;
	int act_go, act_card, act_gomode, act_trans, act_transno, act_sound;
	int time_curr, time_end; lv*carda, *cardb, *trans, *canvas;
	rect pending_grid_cell;
} modal_state; modal_state ms={0};
typedef struct {modal_state ms;widget_state wid;} modal_context;
modal_context ms_stack[8]={{{0},{0}}};int ms_index=0;
void modal_enter(int type);void modal_exit(int value);void field_stylespan(lv*font,lv*arg); // forward refs
void modal_push(int type){
	if(ms.type!=modal_none){
		ms_stack[ms_index]=(modal_context){ms,wid};
		#define swizzle_slot(x,slot) if(x==&slot)ms_stack[ms_index].x=&ms_stack[ms_index].slot;
		swizzle_slot(wid       .gv,wid       .gv_slot)
		swizzle_slot(wid       .fv,wid       .fv_slot)
		swizzle_slot(ms.old_wid.gv,ms.old_wid.gv_slot)
		swizzle_slot(ms.old_wid.fv,ms.old_wid.fv_slot)
		ms_index++;
	}modal_enter(type);
}
void modal_pop(int value){
	lv*l=(ms.type==modal_link&&value)?rtext_all(ms.text.table):NULL;
	modal_exit(value);if(ms_index>0){
		ms_index--,ms=ms_stack[ms_index].ms,wid=ms_stack[ms_index].wid;
		#define unswizzle_slot(x,slot) if(x==&ms_stack[ms_index].slot)x=&slot;
		unswizzle_slot(wid       .gv,wid       .gv_slot)
		unswizzle_slot(wid       .fv,wid       .fv_slot)
		unswizzle_slot(ms.old_wid.gv,ms.old_wid.gv_slot)
		unswizzle_slot(ms.old_wid.fv,ms.old_wid.fv_slot)
	}
	if(l){pair c=wid.cursor;field_stylespan(lmistr(""),l);wid.cursor=c;}
}
int no_menu(void){return menu.active==-1&&menu.stick==-1;}
int in_layer(void){return no_menu()&&(ms.type?ms.in_modal:1)&&((!running()&&!msg.overshoot)||ms.type!=modal_none);}
int in_widgets(void){return ms.type!=modal_none?ms.in_modal:1;}

typedef struct {int shift,alt,comb,lock,on;char*heading;} keycaps_state;keycaps_state kc={0};
int keydown[4096]={0},keyup[4096]={0};
void keycaps_force_enter(void){kc.shift=0,kc.alt=0,kc.comb=0,kc.lock=0,kc.on=1,ev.mu=ev.md=0;}
void keycaps_enter(void){if(!enable_touch||kc.on)return;keycaps_force_enter();}

// App Interface

lv*n_appsave(lv*self,lv*z){if(strlen(document_path)){save_deck(lmcstr(document_path));}else{modal_enter(modal_save_deck);}return LNIL;(void)self;(void)z;}
lv*n_appexit(lv*self,lv*z){
	#ifndef __ANDROID__
		should_exit=1;
	#endif
	return LNIL;(void)self;(void)z;
}

lv* print_array(lv*arr,FILE*out){array a=unpack_array(arr);for(int z=0;z<a.size;z++)fputc(0xFF&(int)array_get_raw(a,z),out);return arr;}
lv*n_appprint(lv*self,lv*a){(void)self;return a->c==1&&array_is(a->lv[0])?print_array(l_first(a),stdout):n_printf(a,1,stdout);}
lv*n_appshow(lv*self,lv*a){(void)self;str s=str_new();EACH(z,a){if(z)str_addc(&s,' ');show(&s,a->lv[z],a->c==1);}printf("%s\n",drom_to_utf8(lmstr(s))->sv);return l_first(a);}
lv* draw_widget(lv*w);lv* draw_con(lv*card,int active); // forward refs
lv*n_apprender(lv*self,lv*a){(void)self;a=l_first(a);return widget_is(a)?draw_widget(a): card_is(a)?draw_con(a,1): image_empty();}
lv*interface_app(lv*self,lv*i,lv*x){
	if(x&&lis(i)){
		ikey("fullscreen"){toggle_fullscreen=windowed!=!lb(x);return x;}
	}else if(lis(i)){
		ikey("fullscreen")return lmn(!windowed);
		ikey("playing"   )return lmn(audio_playing);
		ikey("save"      )return lmnat(n_appsave,NULL);
		ikey("exit"      )return lmnat(n_appexit,NULL);
		ikey("show"      )return lmnat(n_appshow,NULL);
		ikey("print"     )return lmnat(n_appprint,NULL);
		ikey("render"    )return lmnat(n_apprender,NULL);
	}return x?x:LNIL;(void)self;
}

// Menus

char clip_stash[16]={0};
int has_clip(char*type){return strlen(clip_stash)>=strlen(type)&&memcmp(clip_stash,type,strlen(type))==0;}
int menus_off(void){return lb(ifield(deck,"locked"));}
int menus_hidden(void){return uimode==mode_draw&&ev.hidemenu&&ms.type==modal_none;}
void menus_clear(void){menu.active=-1,menu.stick=-1;}
void menu_setup(void){
	menu.x=10,menu.head_count=0,menu.sz=(rect){0,0,0,0},menu.active=-1;
	if(0==(frame_count%8)){snprintf(clip_stash,sizeof(clip_stash),"%s",get_clip()->sv);}
}
void menu_bar(char*name,int enabled){
	if(menus_off())enabled=0;
	rect t=rect_pair((pair){menu.x,2},font_textsize(FONT_MENU,name)), b={t.x-5,0,t.w+10,t.h+3}; int i=menu.head_count;
	menu.heads[menu.head_count++]=(menu_head){name,enabled,t,b}; menu.x=b.x+b.w+5; if(menus_hidden())return;
	if(ev.click&&enabled&&over(b)){ev.mu=0;if(menu.stick==-1)menu.stick=i;}
	if(menu.stick!=-1&&enabled&&over(b))menu.stick=i,menu.lw=0;
	if(menu.stick==-1){
		if(ev.drag&&enabled&&over(b)&&ev.dpos.y<b.h)ev.dpos=ev.pos,menu.lw=0;
		if((ev.drag||ev.mu)&&enabled&&dover(b))menu.active=i;
	}if(i==menu.active||i==menu.stick)menu.sz=(rect){b.x,b.h,MAX(b.w,menu.lw),0},menu.item_count=0;
	if(ev.md&&over(b)&&enabled)ev.md=0;
}
int shortcut_w(char c){if(!c)return 0;char tn[8];snprintf(tn,8,"^%c",c);pair s=font_textsize(FONT_MENU,tn);return 10+s.x;}
int menu_check(char*name,int enabled,int check,char shortcut){
	if(!menu.heads[menu.head_count-1].enabled)return 0;
	int sc=enabled&&shortcut&&ev.shortcuts[0xFF&shortcut]; if(sc)ev.shortcuts[0xFF&shortcut]=0;
	if(menu.head_count-1!=menu.active&&menu.head_count-1!=menu.stick)return sc;
	rect t=name?rect_pair((pair){menu.sz.x+5+8,menu.sz.y+menu.sz.h+2},font_textsize(FONT_MENU,name)): (rect){menu.sz.x,menu.sz.y+menu.sz.h+2,1,1};
	if(shortcut)t.w+=shortcut_w(shortcut);
	rect b={menu.sz.x,menu.sz.y+menu.sz.h,MAX(menu.sz.w,t.w+10+8),t.h+4};
	menu.items[menu.item_count++]=(menu_entry){name,enabled,check,shortcut,t,b}, menu.sz=box_union(menu.sz,b);
	return sc||(enabled&&ev.mu&&over(b));
}
int menu_item(char*name,int enabled,char shortcut){return menu_check(name,enabled,-1,shortcut);}
void menu_separator(void){menu_check(NULL,0,0,'\0');}
void menu_finish(void){
	if(menus_off()||menus_hidden())return;
	rect b={0,0,context.size.x,3+font_h(FONT_MENU)};
	draw_rect(b,32),draw_hline(0,b.w,b.h,1); char*pal=patterns_pal(ifield(deck,"patterns"));
	for(int i=0;i<menu.head_count;i++){
		menu_head x=menu.heads[i];int a=x.enabled&&(over(x.b)||i==menu.stick||i==menu.active);
		if(ev.drag&&!dover(b))a=0;
		draw_text(x.t,x.name,FONT_MENU,x.enabled?1:13);if(a)draw_invert(pal,x.b);
	}
	if(menu.sz.w){
		draw_shadow(menu.sz,1,32,1);
		menu.lw=0;int sw=0;for(int i=0;i<menu.item_count;i++){menu.lw=MAX(menu.lw,menu.items[i].b.w);sw=MAX(sw,shortcut_w(menu.items[i].shortcut));}
		for(int i=0;i<menu.item_count;i++){
			menu_entry x=menu.items[i]; int o=over(x.b)&&x.name&&x.enabled;
			if(x.name){draw_text(x.t,x.name,FONT_MENU,x.enabled?1:13);}else{draw_rect((rect){x.t.x+2,x.t.y,menu.sz.w-5,1},19);}
			if(x.check==1)draw_icon((pair){menu.sz.x+2,x.t.y+3},CHECK,x.enabled?1:13);
			if(x.shortcut){char tn[8];snprintf(tn,8,"^%c",x.shortcut);draw_text((rect){menu.sz.x+menu.sz.w-3-sw+10,x.t.y,0,0},tn,FONT_MENU,x.enabled?1:13);}
			if(o)draw_invert(pal,inset(x.b,1));
		}if(ev.mu)menu.stick=-1;
	}
	if(!windowed)for(int x=0;x<=1;x++)for(int y=0;y<=1;y++)draw_icon((pair){x*(context.size.x-5),y*(context.size.y-5)},CORNERS[x+y*2],1);
}

// Widgets

int field_notify_disable=0;
void field_change(void){
	if(!wid.field_dirty||!wid.ft)return;
	field_notify_disable=1;iwrite(wid.ft,lmistr("value"),wid.fv->table),mark_dirty();field_notify_disable=0;
	msg.target_change=wid.ft, msg.arg_change=rtext_all(wid.fv->table);
}
void grid_exit(void){
	wid.ingrid=0,wid.gv=NULL,wid.gt=NULL;
	if(wid.hist)wid.hist->c=0,wid.hist_cursor=0;
}
void field_exit(void){
	field_change();kc.on=0;
	wid.infield=0,wid.fv=NULL,wid.ft=NULL,wid.cursor=(pair){0,0};
	if(wid.hist)wid.hist->c=0,wid.hist_cursor=0;
	wid.field_dirty=0,wid.change_timer=0;
}
void bg_end_selection(void);void bg_end_lasso(void); // forward-ref
void setuimode(int mode){
	n_play(deck,lml2(LNIL,lmistr("loop")));
	grid_exit(),field_exit(),bg_end_selection(),bg_end_lasso(),ob.sel->c=0,wid.active=-1;poly_count=0;sc.others=NULL;
	msg.next_view   =(uimode!=mode)&&mode==mode_interact;
	msg.pending_loop=(uimode!=mode)&&mode==mode_interact;
	uimode=mode;if(mode!=mode_interact)msg.pending_halt=1;
	if(mode!=mode_draw&&!prototype_is(con()))dr.fatbits=0;
	if(mode==mode_interact)dr.fatbits=0;
}
void settool(int tool){setuimode(mode_draw);dr.tool=tool;}
void setscript(lv*x){
	if(uimode!=mode_script)sc.prev_mode=uimode;setuimode(mode_script);
	sc.target=lil(x)?x->lv[0]:x, sc.others=lil(x)?l_drop(ONE,x):lml(0);
	sc.status[0]='\0';lv*v=ifield(sc.target,"script");int p=0;
	if(!v->c)v=card_is  (sc.target)?(p=1,lmistr("on view do\n \nend")):
		       button_is(sc.target)?(p=1,lmistr("on click do\n \nend")):
		       grid_is  (sc.target)?(p=1,lmistr("on click row do\n \nend")):
		       field_is (sc.target)?(p=1,lmistr("on change val do\n \nend")):
		       slider_is(sc.target)?(p=1,lmistr("on change val do\n \nend")):
		       canvas_is(sc.target)?(p=1,lmistr("on click pos do\n \nend\n\non drag pos do\n \nend\n\non release pos do\n \nend")):v;
	if(p&&widget_is(sc.target)&&!contraption_is(sc.target)&&lb(ifield(sc.target,"animated"))){v=l_fuse(lmistr(""),lml2(v,lmistr("\n\non view do\n \nend")));}
	if(!v->c&&contraption_is(sc.target)){lv*t=ifield(ifield(sc.target,"def"),"template");if(t->c)p=1,v=t;}
	if(p)snprintf(sc.status,sizeof(sc.status),"No existing script; populated a template.");
	sc.f=(field_val){rtext_cast(v),0};wid.active=0;
}
void finish_script(void){if(sc.next){setscript(sc.next),sc.next=NULL;}else{setuimode(sc.prev_mode);}}
void widget_setup(void){
	if(ev.mu||wid.active==-1)wid.col_drag=0;
	if(wid.active>=wid.count)wid.active=0;
	if((uimode==mode_interact||ms.type!=modal_none)&&ev.tab&&wid.count&&!(wid.infield&&wid.f.style==field_code)&&!kc.on){
		if(wid.ingrid)grid_exit();
		if(wid.infield)field_exit();
		wid.active+=ev.shift?-1:1;
		if(wid.active<0)wid.active+=wid.count;wid.cursor=(pair){0,0};
	}
	if(uimode!=mode_interact&&uimode!=mode_script&&ms.type==modal_none){
		if(wid.ingrid ||wid.gv!=NULL)grid_exit();
		if(wid.infield||wid.fv!=NULL)field_exit();
		wid.active=-1;
	}
	wid.count=0,wid.scrolls=0;wid.ingrid=0;wid.infield=0;
}
void draw_boxr(rect r,int fcol,int bcol,int background){
	draw_hline(r.x+2,r.x+r.w-2,r.y,fcol),draw_hline(r.x+2,r.x+r.w-2,r.y+r.h-1,fcol),draw_vline(r.x,r.y+2,r.y+r.h-2,fcol),draw_vline(r.x+r.w-1,r.y+2,r.y+r.h-2,fcol);
	draw_pix(r.x+1,r.y+1,fcol),draw_pix(r.x+r.w-2,r.y+1,fcol),draw_pix(r.x+1,r.y+r.h-2,fcol),draw_pix(r.x+r.w-2,r.y+r.h-2,fcol);
	if(background)draw_hline(r.x+2,r.x+r.w-2,r.y+1,bcol),draw_hline(r.x+2,r.x+r.w-2,r.y+r.h-2,bcol),draw_rect((rect){r.x+1,r.y+2,r.w-2,r.h-4},bcol);
}
void draw_boxinv(char*pal,rect r){
	draw_invert(pal,(rect){r.x,r.y,1,r.h})    ,draw_invert(pal,(rect){r.x+r.w-1,r.y,1,r.h});
	draw_invert(pal,(rect){r.x+1,r.y,r.w-2,1}),draw_invert(pal,(rect){r.x+1,r.y+r.h-1,r.w-2,1});
}
rect scrollbar(rect r,int n,int line,int page,int*scroll,int visible,int inverted){
	#define addscroll(x) *scroll=CLAMP(0,*scroll+x,n)
	addscroll(0);int sz=image_size(ARROWS->lv[0]).x+2, fcol=!in_layer()?13:inverted?32:1, bcol=inverted?1:32;
	rect b={r.x+r.w-sz-2,r.y,sz+2,r.h}, rr={r.x+1,r.y,visible?r.w-b.w-1:r.w-2,r.h-2};
	int dragging_thumb=(wid.thumbid==wid.scrolls++)&&dover(b);
	if(visible){
		draw_box(b,0,fcol);
		{
			rect bb={b.x,b.y,sz+2,sz+2}; int a=n>0&&over(bb)&&!dragging_thumb, o=a&&(ev.mu||ev.drag)&&dover(r);
			draw_box(bb,0,fcol);draw_icon((pair){bb.x+2,bb.y+2},ARROWS->lv[0+(o?2:0)],fcol);if(o&&ev.md)addscroll(line*-1);if(a&&!ev.drag)uicursor=cursor_point;
		}
		{
			rect bb={b.x,b.y+b.h-sz-2,sz+2,sz+2}; int a=n>0&&over(bb)&&!dragging_thumb, o=a&&(ev.mu||ev.drag)&&dover(r);
			draw_box(bb,0,fcol);draw_icon((pair){bb.x+2,bb.y+2},ARROWS->lv[1+(o?2:0)],fcol);if(o&&ev.md)addscroll(line);if(a&&!ev.drag)uicursor=cursor_point;
		}
		if(n<=0||!in_layer())return rr;
		rect s={b.x+1,b.y+sz+2,b.w-2,b.h-2*(sz+2)};draw_rect(s,inverted?9:12);
		int thumb_height=MAX(16,s.h/(1+n)), thumb_y=(s.h-thumb_height)*(*scroll/(float)n); rect thumb={s.x,s.y+thumb_y,s.w,thumb_height};
		if(in_layer()&&ev.md&&over(thumb)){wid.thumbid=wid.scrolls-1,wid.thumbo=ev.dpos.y-thumb.y,dragging_thumb=1;}
		if(in_layer()&&ev.drag&&dragging_thumb){
			float capped=MAX(s.y,MIN(s.y+s.h-thumb.h,ev.pos.y-wid.thumbo))-s.y;
			thumb.y=capped+s.y,*scroll=MAX(0,MIN((capped/(s.h-thumb_height))*n,n)); uicursor=cursor_drag;
		}
		if(in_layer()&&ev.mu&&dragging_thumb)wid.thumbid=-1;
		draw_rect(thumb,bcol),draw_box(thumb,0,fcol);
		char*pal=patterns_pal(ifield(deck,"patterns"));
		if(thumb_y>0){
			rect bb={s.x,s.y,s.w,thumb_y}; if(!dragging_thumb&&over(bb))uicursor=cursor_point;
			if(!dragging_thumb&&ev.mu&&over(bb))draw_invert(pal,bb),addscroll(page*-1);
		}
		if(thumb_y+thumb.h<s.h){
			rect bb={s.x,s.y+thumb_y+thumb.h,s.w,s.h-(thumb_y+thumb.h)}; if(!dragging_thumb&&over(bb))uicursor=cursor_point;
			if(!dragging_thumb&&ev.mu&&over(bb))draw_invert(pal,bb),addscroll(page);
		}
	}
	if(in_layer()&&over(box_union(r,b))&&ev.scroll)addscroll(ev.scroll*line);
	return rr;
}
int widget_button(lv*target,button x,int value){
	int l=x.locked||!in_layer(); if(!x.font)x.font=FONT_MENU; rect b=x.size; char*pal=patterns_pal(ifield(deck,"patterns"));
	int fcol=l?13:x.show==show_invert?32:1, bcol=x.show==show_invert?1:32, scol=x.show==show_invert?32:1;
	int sel=!l&&x.show!=show_none&&x.style!=button_invisible&&wid.active==wid.count;
	int sh=0,shh=0;if(!l&&uimode==mode_interact&&!wid.fv&&!ev.shift&&x.show!=show_none&&x.shortcut){
		if(keyup[(int)x.shortcut]){shh=1;}else if(keydown[(int)x.shortcut]){sh=1;}
	}
	int a=!l&&dover(b)&&over(b), cs=(sel&&ev.action), cl=cs||sh||((ev.md||ev.drag)&&a), cr=cs||shh|(ev.mu&&a);
	if(!l&&over(b)&&!ev.drag&&x.show!=show_none)uicursor=cursor_point;
	if(x.show==show_none)return 0; rect ar=inset(b,2);
	if(x.style==button_round){
		draw_boxr(b,fcol,bcol,x.show!=show_transparent);
		draw_textc(inset(b,3),x.text,x.font,fcol);
		if(sel)draw_box(ar,0,13);
		if(cl)draw_invert(pal,ar);
	}
	if(x.style==button_rect){
		if(cl){b=(rect){b.x+1,b.y+1,b.w-1,b.h-1},ar=(rect){ar.x+1,ar.y+1,ar.w-1,ar.h-1};if(x.show!=show_transparent)draw_rect(b,bcol);draw_box(b,0,fcol);}
		else  {b=(rect){b.x  ,b.y  ,b.w-1,b.h-1},ar=(rect){ar.x  ,ar.y  ,ar.w-1,ar.h-1};draw_shadow(b,fcol,bcol,x.show!=show_transparent);}
		draw_textc(inset(b,3),x.text,x.font,fcol);if(sel)draw_box(ar,0,13);
	}
	if(x.style==button_check||x.style==button_radio){
		if(x.show!=show_transparent)draw_rect(b,bcol);
		pair ts=font_textsize(x.font,x.text), cdim=image_size(x.style==button_check?CHECKS[0]:RADIOS[0]); int bh=MAX(ts.y,cdim.y);
		rect br={b.x,b.y+((b.h-bh)/2),b.w,bh}, to=box_intersect(b,(rect){br.x+cdim.x,br.y+(br.h-ts.y)/2,b.w-cdim.x,ts.y});
		draw_rect((rect){br.x+1,br.y+1,cdim.x-4,cdim.y-3},bcol);
		if(x.style==button_check){draw_icon((pair){br.x,br.y},CHECKS[(value^(cl||cr))+2*x.locked],scol);}
		else{pair p={br.x,br.y};draw_icon(p,RADIOS[3],bcol),draw_icon(p,RADIOS[cl||cr?1:0],fcol);if(value)draw_icon(p,RADIOS[2],fcol);}
		draw_text_fit(to,x.text,x.font,fcol);ar=to;
		if(sel)draw_box((rect){to.x-2,to.y-1,to.w+2,to.h+2},0,13);
		if(cl)draw_invert(pal,ar);
	}
	if(x.style==button_invisible){
		draw_textc(inset(b,3),x.text,x.font,fcol);
		if(cl&&x.show!=show_transparent)draw_invert(pal,ar);
	}
	if(target&&cr)msg.target_click=target;
	if(!x.locked&&in_widgets())wid.count++;
	return cr;
}
void widget_slider(lv*target,slider x){
	int l=x.locked||!in_layer(); if(!x.font)x.font=FONT_MENU; rect b=x.size; char*pal=patterns_pal(ifield(deck,"patterns"));
	int fcol=l?13:x.show==show_invert?32:1, bcol=x.show==show_invert?1:32, bpat=x.show==show_invert?9:12;
	int sel=!l&&x.show!=show_none&&wid.active==wid.count; float ov=x.value;
	if(x.show==show_none)return;
	lv*t=(x.style==slider_bar||x.style==slider_compact)?l_format(x.format,lmn(x.value)):lmistr("");
	rect oc=frame.clip; frame.clip=box_intersect(b,frame.clip);
	if(x.style==slider_horiz||x.style==slider_vert){
		if(x.show!=show_transparent)draw_rect(b,bpat);draw_box(b,0,sel?13:fcol);
		#define hv_btn(bb,dir,ba) {\
			int a=!l&&over(bb), o=a&&(ev.mu||ev.drag)&&dover(bb);\
			if(o&&ev.md)x.value+=(dir*x.step); if(a)uicursor=cursor_point;\
			draw_rect(bb,bcol),draw_box(bb,0,fcol),draw_icon((pair){bb.x+(bb.w-12)/2,bb.y+(bb.h-12)/2},ARROWS->lv[ba+(o?2:0)],fcol);}
		#define hv_tsz(axis) int drag=(wid.thumbid==wid.scrolls++)&&dover(b),\
		                     n=(x.max-x.min)/x.step, ts=MAX(MIN(axis,16),axis/(1+n)), tp=((x.value-x.min)/MAX(1,(x.max-x.min)))*(axis-ts);
		#define hv_tmv(av,as) if(!l){\
		    if(ev.md&&over(thumb))wid.thumbid=wid.scrolls-1,wid.thumbo=ev.dpos.av-thumb.av,drag=1;\
		    if(ev.drag&&drag){\
				float capped=MAX(b.av,MIN(b.av+b.as-thumb.as,ev.pos.av-wid.thumbo))-b.av;\
				thumb.av=capped+b.av,uicursor=cursor_drag,x.value=x.min+capped*((x.max-x.min)/(b.as-ts));\
			}if(ev.mu&&drag)wid.thumbid=-1;}
		#define hv_gap(bb,dir) {if(!l&&!drag&&over(bb)){uicursor=cursor_point;if(ev.mu)draw_invert(pal,bb),x.value+=(dir*10*x.step);}}
	}
	if(x.style==slider_horiz){
		rect lf={b.x,b.y,16,b.h},rt={b.x+b.w-16,b.y,16,b.h};hv_btn(lf,-1,4);hv_btn(rt,1,5);b=(rect){b.x+16,b.y+1,b.w-32,b.h-2};
		hv_tsz(b.w);rect thumb={b.x+tp,b.y,ts,b.h};hv_tmv(x,w);draw_rect(thumb,bcol),draw_box(thumb,0,fcol);
		rect g0={b.x,b.y,thumb.x-b.x,b.h},g1={thumb.x+thumb.w,b.y,b.w-(thumb.x+thumb.w-b.x),b.h};hv_gap(g0,-1);hv_gap(g1,1);
	}
	if(x.style==slider_vert){
		rect up={b.x,b.y,b.w,16},dn={b.x,b.y+b.h-16,b.w,16};hv_btn(up,-1,0);hv_btn(dn,1,1);b=(rect){b.x+1,b.y+16,b.w-2,b.h-32};
		hv_tsz(b.h);rect thumb={b.x,b.y+tp,b.w,ts};hv_tmv(y,h);draw_rect(thumb,bcol),draw_box(thumb,0,fcol);
		rect g0={b.x,b.y,b.w,thumb.y-b.y},g1={b.x,thumb.y+thumb.h,b.w,b.h-(thumb.y+thumb.h-b.y)};hv_gap(g0,-1);hv_gap(g1,1);
	}
	if(x.style==slider_bar){
		fcol=x.locked?(x.show==show_invert?32:1):fcol;
		if(x.show!=show_transparent)draw_rect(b,bcol);draw_box(b,0,sel?13:fcol);
		b=inset(b,2);draw_textc(b,t->sv,x.font,fcol);if(!l&&over(b))uicursor=cursor_point;
		float f=(x.max==x.min)?0:((x.value-x.min)/(x.max-x.min))*b.w;draw_invert(pal,(rect){b.x,b.y,f,b.h});
		if(!l&&dover(b)&&(ev.md||ev.drag)){x.value=x.min+(ev.pos.x-b.x)*((x.max-x.min)/b.w);uicursor=cursor_drag;}
	}
	if(x.style==slider_compact){
		if(x.show==show_transparent){draw_rect((rect){b.x+1,b.y+1,13,b.h-2},bcol),draw_rect((rect){b.x+b.w-14,b.y+1,13,b.h-2},bcol);}
		draw_boxr(b,fcol,bcol,x.show!=show_transparent),draw_textc((rect){b.x+14,b.y,b.w-28,b.h},t->sv,x.font,fcol);
		#define comp_btn(xo,dir,ba,li,en) {\
			rect bb={b.x+xo,b.y,14,b.h}; int a=en&&!l&&over(bb), o=a&&(ev.mu||ev.drag)&&dover(bb);\
			if(o&&ev.md)x.value+=(dir*x.step); if(a)uicursor=cursor_point;\
			draw_icon((pair){bb.x+1,b.y+(b.h-12)/2},ARROWS->lv[ba+(o?2:0)],en?fcol:13);draw_vline(bb.x+li,b.y+1,b.y+b.h-1,sel?13:fcol);}
		comp_btn(0,-1,4,13,x.min!=x.value);comp_btn(b.w-14,1,5,0,x.max!=x.value);
	}
	if(sel&&ev.dir==dir_up)x.value-=x.step;if(sel&&ev.dir==dir_down)x.value+=x.step;
	if(in_layer()&&over(frame.clip)&&ev.scroll)x.value+=x.step*ev.scroll; x.value=slider_normalize((fpair){x.min,x.max},x.step,x.value);
	if(target&&fabs(ov-x.value)>(x.step/2)){msg.target_change=target,msg.arg_change=lmn(x.value);iwrite(target,lmistr("value"),msg.arg_change),mark_dirty();}
	frame.clip=oc;
	if(!x.locked&&in_widgets())wid.count++;
}
void widget_canvas(lv*target,canvas x,lv*image){
	if(x.show==show_none)return; rect b=x.size;
	char*pal=patterns_pal(ifield(deck,"patterns"));
	if(x.show==show_solid){if(image){draw_scaled(b,image->b,1);}else{draw_rect(b,0);}}
	if(image&&x.show==show_transparent)draw_scaled(b,image->b,0);
	if(image&&x.show==show_invert)draw_invert_scaled(pal,b,image->b);
	if(x.border){if(x.show==show_invert){draw_boxinv(pal,b);}else{draw_box(b,0,1);}}
	if(!target||!in_layer())return;
	if(x.draggable){
		int sel=ob.sel->c&&ob.sel->lv[0]==target;
		if     (ev.md&&dover(b))msg.target_click  =target,msg.arg_click  =(fpair){b.x,b.y},ob.sel=l_list(target),ob.prev=(pair){ev.pos.x-b.x,ev.pos.y-b.y};
		else if(ev.mu     &&sel)msg.target_release=target,msg.arg_release=(fpair){ev.dpos.x-ob.prev.x,ev.dpos.y-ob.prev.y},ob.sel->c=0;
		else if(ev.drag   &&sel)msg.target_drag   =target,msg.arg_drag   =(fpair){ev.dpos.x-ob.prev.x,ev.dpos.y-ob.prev.y};
	}else if(dover(b)){
		fpair p={(ev.pos.x-b.x)/x.scale,(ev.pos.y-b.y)/x.scale}; int dp=p.x!=msg.lastdrag.x||p.y!=msg.lastdrag.y, im=over(b);
		if     (ev.md          )msg.target_click  =target,msg.arg_click  =p,msg.lastdrag=p;
		else if(ev.mu          )msg.target_release=target,msg.arg_release=p;
		else if(ev.drag&&dp&&im)msg.target_drag   =target,msg.arg_drag   =p,msg.lastdrag=p;
	}
}
str cf;void grid_edit_cell(pair cell,lv*v);
rect grid_cell(grid x,grid_val*value,rect bb,pair pos){
	lv*fnt=x.font?x.font:FONT_MONO;
	int hwt=0;for(int z=0;z<x.widths[0];z++)hwt+=x.widths[1+z];
	#define cw(n) (n>=x.widths[0]?((bb.w-hwt)/(nc-x.widths[0])):x.widths[1+n])
	int nc=value->table->c, rh=font_h(fnt)+(x.lines?5:3),cx=0; for(int z=0;z<MIN(nc,pos.x);z++)cx+=cw(z);
	return box_intersect((rect){bb.x+cx,bb.y+rh*pos.y+1,pos.x==nc-1?bb.w-cx:cw(pos.x),rh-1},bb);
}
rect grid_hcell(grid x,int column,rect cell){
	cell=inset(cell,x.lines?1:0);if(column&&x.lines)cell.x+=1,cell.w-=1;return cell;
}
int widget_grid(lv*target,grid x,grid_val*value){
	if(x.show==show_none)return 0; lv*hfnt=FONT_BODY; int hsize=x.headers?font_h(hfnt)+5:0; if(x.size.h<=(50+hsize)||x.size.w<16)x.scrollbar=0;
	lv*fnt=x.font?x.font:FONT_MONO; int os=value->scroll, or=value->row, oc=value->col, files=x.headers==2; if(files||x.size.h<=hsize)x.headers=0;
	int nr=value->table->n, nc=value->table->c, rh=font_h(fnt)+(x.lines?5:3), fcol=!in_layer()?13:x.show==show_invert?32:1, bcol=x.show==show_invert?1:32;
	int fk=strlen(x.format); rect b=x.size;
	int sel=in_layer()&&x.show!=show_none&&wid.active==wid.count;
	if(in_layer()&&dover(b)&&(ev.md||ev.mu||ev.drag)){if(!sel&&wid.gv)grid_exit(); wid.active=wid.count,sel=1;}
	if(sel&&in_layer()&&!over(b)&&ev.md){sel=0;wid.active=-1;grid_exit();}
	if(sel){if(wid.fv)field_exit();wid.ingrid=1,wid.g=x,wid.gv=value,wid.gt=target;}
	if(x.show!=show_transparent)draw_rect(b,bcol);
	rect bh={b.x,b.y,b.w,x.headers?font_h(hfnt)+5:0}; int nrd=MIN(nr,((b.h-bh.h+1)/rh)), scrollmax=nr-nrd;
	rect bb=scrollbar((rect){b.x,b.y+(x.headers?bh.h-1:0),b.w,b.h-(x.headers?bh.h-1:0)},scrollmax,1,nrd,&value->scroll,x.scrollbar,x.show==show_invert);
	draw_box(x.lines?b:(rect){b.x,bb.y,b.w,b.h-(bb.y-b.y)},0,sel?13:fcol);
	int hwt=0;for(int z=0;z<x.widths[0];z++)hwt+=x.widths[1+z];
	char*pal=patterns_pal(ifield(deck,"patterns"));
	if(x.lines)draw_rect(bh,fcol);if(nc<=0)draw_textc(inset(bb,1),"(no data)",hfnt,fcol);
	if(!cf.sv)cf=str_new();
	#define rowh(n) inset((rect){bb.x+1,bb.y+rh*n+2,bb.w-2,rh-3},x.lines?0:-1)
	#define rowb(n) (rect){bb.x,bb.y+rh*n,bb.w,rh}
	int clicked=0,rsel=0,hrow=-1,hcol=-1;
	for(int y=0;y<nrd;y++){
		int ra=in_layer()&&over(bb)&&over(rowb(y));rect cbox={0};
		if(ra&&x.bycell){for(int z=0;z<nc;z++){rect cell=grid_cell(x,value,bb,(pair){z,y});if(over(cell))hcol=z,cbox=grid_hcell(x,z,cell);}}
		if(ra&&(ev.md||ev.drag)){rsel=1,hrow=y+value->scroll;draw_rect(x.bycell?cbox:rowh(y),fcol);}
		if(ra&&ev.mu)clicked=1,value->row=y+value->scroll,value->col=hcol;
		if(ra&&!ev.drag)uicursor=cursor_point;
	}int rr=value->row-value->scroll;
	if(!rsel&&rr>=0&&rr<nrd&&(x.bycell?value->col>=0&&value->col<nc:1)){
		hrow=value->row,hcol=value->col;
		if(x.bycell&&value->col>=0){draw_rect(grid_hcell(x,value->col,grid_cell(x,value,bb,(pair){value->col,rr})),fcol);}
		else{draw_rect(rowh(rr),fcol);}
	}
	for(int z=0,cols=0,cx=0;z<nc&&cx+cw(cols)<=bb.w;z++,cols++){
		rect hs=(rect){bh.x+4+cx,bh.y+1,cw(cols)-5,bh.h-2};
		if(hs.w<=0)continue; // suppressed column
		if(x.headers){
			int oa=target&&in_layer()&&over(hs)&&((ev.drag||ev.mu)?dover(hs):1)&&!wid.col_drag;
			int dp=oa&&(ev.md||ev.drag);if(dp)draw_rect(hs,x.lines?bcol:fcol);
			draw_textc(hs,value->table->kv[z]->sv,hfnt,x.lines^dp?bcol:fcol);
			if(oa&&!ev.drag)uicursor=cursor_point;
			if(oa&&ev.mu){msg.target_order=target,msg.arg_order=value->table->kv[z];}
		}
		if(cols&&x.lines)draw_invert(pal,(rect){hs.x-3,b.y+1,1,b.h-2});cx+=cw(cols);
		for(int y=0;y<nrd;y++){
			int ccol=y+value->scroll==hrow&&(x.bycell?cols==hcol :1)?bcol:fcol;
			rect cell={hs.x-3,bb.y+rh*y+1,hs.w+5,rh-1}; lv*v=value->table->lv[z]->lv[y+value->scroll];
			cf.c=0;format_type_simple(&cf,v,z>=fk?'s':x.format[z]=='L'?'s':x.format[z]);str_term(&cf);
			rect ib=box_center(cell,image_size(ICONS[0]));pair ip={ib.x,ib.y};
			rect oc=frame.clip; frame.clip=box_intersect(cell,frame.clip);
			if     (z<fk&&x.format[z]=='I'){int i=MAX(0,MIN(8,ln(v)));if(i<8)draw_icon(ip,ICONS[i],ccol);}
			else if(z<fk&&x.format[z]=='B'){if(lb(v))draw_icon(ip,ICONS[icon_chek],ccol);}
			else{
				rect r={hs.x+1,bb.y+rh*y,hs.w-2,rh}; // right-align numeric columns:
				if(z<fk&&strchr("fcCihH",x.format[z])){draw_textr(r,cf.sv,fnt,ccol);}else{draw_text_fit(r,cf.sv,fnt,ccol);}
			}
			frame.clip=oc;
			if(!x.locked&&sel&& ((ev.dclick&&over(cell)) || (ev.action&&x.bycell&&z==value->col&&y+value->scroll==value->row))){
				char f=z<fk?x.format[z]:'s';pair tc=(pair){z,y+value->scroll};
				if     (f=='I'||f=='L'){} // no editing allowed
				else if(f=='B'||f=='b'){grid_edit_cell(tc,lmn(!lb(v)));} // toggle
				else{
					wid.pending_grid_edit=1,ms.pending_grid_cell=grid_cell(x,value,bb,(pair){tc.x,tc.y-value->scroll});
					ms.pending_grid_cell.y-=1,ms.pending_grid_cell.w+=1,ms.pending_grid_cell.h+=2;
					ms.cell=tc;ms.text=(field_val){rtext_cast(lmcstr(cf.sv)),0};
				}
			}
		}
	}
	if(x.lines)for(int y=1;y<nrd;y++)draw_hline(bb.x,bb.x+bb.w,bb.y+rh*y,fcol);
	if(!x.locked&&in_layer()&&target)for(int z=0,cx=bh.x;z<nc;cx+=cw(z),z++){
		rect h={cx+cw(z)-1,bh.y,5,bh.h};if(h.x+h.w>b.x+b.w)break;
		if(over(h))draw_vline(h.x+2,h.y,h.y+h.h,13);
		if(ev.md&&dover(h)){wid.col_drag=1,wid.col_num=z,wid.col_orig=cw(z);}
		if(sel&&wid.col_drag&&wid.col_num==z&&ev.drag){
			int s=MIN(MAX(10,wid.col_orig+(ev.pos.x-ev.dpos.x)),bb.w-10),i=z;uicursor=cursor_drag;
			GEN(r,MAX(x.widths[0],i+1))lmn(i==z?s:cw(z));iwrite(target,lmistr("widths"),r),mark_dirty();
		}
	}
	if(target&&os!=value->scroll)iwrite(target,lmistr("scroll"),lmn(value->scroll)),mark_dirty();
	if(target&&or!=value->row)iwrite(target,lmistr("row"),lmn(value->row)),mark_dirty();
	if(target&&oc!=value->col)iwrite(target,lmistr("col"),lmn(value->col)),mark_dirty();
	if(target&&clicked)msg.target_click=target,msg.arg_click=(fpair){0,value->row};
	if(in_widgets())wid.count++;
	return files?((clicked&&ev.dclick)||(sel&&ev.action)): clicked;
}
lv* grid_format(void){return strlen(wid.g.format)?lmcstr(wid.g.format):l_take(lmn(wid.gv->table->c),lmistr("s"));}
void grid_apply(lv*v){
	wid.gv->table=v,wid.gv->row=-1;if(!wid.gt)return;
	iwrite(wid.gt,lmistr("value"),v),iwrite(wid.gt,lmistr("row"),lmn(-1)),mark_dirty();
	msg.target_change=wid.gt, msg.arg_change=v;
}
void grid_undo(void){lv*x=wid.hist->lv[--(wid.hist_cursor)];grid_apply(x->lv[0]);}
void grid_redo(void){lv*x=wid.hist->lv[(wid.hist_cursor)++];grid_apply(x->lv[1]);}
void grid_edit(lv*v){wid.hist->c=wid.hist_cursor,ll_add(wid.hist,lml2(wid.gv->table,v)),grid_redo();}
void grid_deleterow(void){grid_edit(l_drop(l_list(lmn(wid.gv->row)),wid.gv->table));}
void grid_insertrow(void){
	lv*f=grid_format(),*x=wid.gv->table,*r=lmt();int s=wid.gv->row+1;EACH(z,x){
		lv*c=lml(x->n+1);dset(r,x->kv[z],c);EACH(i,c)c->lv[i]=(i==s)?(strchr("sluro",f->sv[z])?lmistr(""):ZERO): x->lv[z]->lv[i-(i>=s?1:0)];
	}grid_edit(torect(r));iwrite(wid.gt,lmistr("col"),ZERO),iwrite(wid.gt,lmistr("row"),lmn(s));
	int os=wid.gv->scroll,ns=grid_scrollto(x,wid.g,os,s);if(os!=ns){wid.gv->scroll=ns,iwrite(wid.gt,lmistr("scroll"),lmn(ns));}
}
void grid_edit_cell(pair cell,lv*v){
	wid.gv->col=cell.x,iwrite(wid.gt,lmistr("col"),lmn(cell.x));
	wid.gv->row=cell.y,iwrite(wid.gt,lmistr("row"),lmn(cell.y));
	msg.target_ccell=wid.gt,msg.arg_ccell=ls(v);
}
void grid_keys(int code){
	lv*fnt=wid.g.font?wid.g.font:FONT_MONO, *hfnt=FONT_BODY;
	int m=0, nr=wid.gv->table->n, nc=wid.gv->table->c, r=wid.gv->row, c=wid.gv->col;
	int rh=font_h(fnt)+5, bh=wid.g.headers?font_h(hfnt)+5:0, nrd=MIN(nr,((wid.g.size.h-bh+1)/rh));
	if(code==KEY_UP      ){m=1;if(r==-1){r=0;}else{r-=1;}}
	if(code==KEY_DOWN    ){m=1;if(r==-1){r=0;}else{r+=1;}}
	if(code==KEY_LEFT    ){m=1;if(c==-1){c=0;}else{c-=1;}}
	if(code==KEY_RIGHT   ){m=1;if(c==-1){c=0;}else{c+=1;}}
	if(code==KEY_PAGEUP  ){m=1;if(r==-1)r=0;r-=nrd;}
	if(code==KEY_PAGEDOWN){m=1;if(r==-1)r=0;r+=nrd;}
	if(code==KEY_HOME    ){m=1;r=0;}
	if(code==KEY_END     ){m=1;r=nr-1;}
	if(!wid.g.locked&&(code==KEY_BACKSPACE||code==KEY_DELETE))grid_deleterow();
	if(!m)return;if(ms.type==modal_prototype_attrs)ms.text.table=ms.name.table=NULL;
	wid.gv->row=r=MAX(0,MIN(r,nr-1)),wid.gv->col=c=MAX(0,MIN(c,nc-1));if(wid.gt){
		iwrite(wid.gt,lmistr("row"),lmn(r)),iwrite(wid.gt,lmistr("col"),lmn(c)),mark_dirty();
		msg.target_click=wid.gt,msg.arg_click=(fpair){0,r};
	}
	int os=wid.gv->scroll;if(r-os<0)wid.gv->scroll=r;if(r-os>=nrd)wid.gv->scroll=r-(nrd-1);
	if(wid.gt&&os!=wid.gv->scroll)iwrite(wid.gt,lmistr("scroll"),lmn(wid.gv->scroll)),mark_dirty();
}
int layout_index(pair p){
	for(int z=0;z<lines_count;z++){
		rect l=lines[z].pos  ;if(p.y>l.y+l.h)continue;
		pair r=lines[z].range;if(p.y<l.y    )return r.x;
		for(int i=r.x;i<=r.y;i++){rect g=layout[i].pos;if(p.x<g.x+(g.w/2))return i;}
		return z==lines_count-1?layout_count: r.y;
	}return layout_count;
}
glyph_box layout_last(lv*font){return layout_count>0?layout[layout_count-1]:(glyph_box){{0,0,1,font_h(font)},0,'\0',font,NULL};}
rect layout_cursor(int index,lv*font,field f){
	int bw=f.size.w-5-(f.scrollbar?image_size(ARROWS->lv[0]).x+3:0), bx=f.align==align_center?bw/2: f.align==align_right?bw: 0;
	rect r=layout_count>0?layout[MIN(index,layout_count-1)].pos:(rect){bx,0,1,font_h(font)};
	if(index>=layout_count){glyph_box last=layout_last(font);if(last.c=='\n'){r.x=0;r.y+=r.h;}else{r.x+=r.w-1;}}
	r.w=1;return r;
}
void widget_field(lv*target,field x,field_val*value){
	if(x.show==show_none)return; if(x.size.h<=50||x.size.w<16)x.scrollbar=0;
	int l=!in_layer(); lv*fnt=x.font?x.font: x.style==field_code?FONT_MONO: FONT_BODY; rect b=x.size;
	int fcol=(l&&!x.locked)?13:x.show==show_invert?32:1, bcol=x.show==show_invert?1:32, os=value->scroll;
	if(x.show!=show_transparent)draw_rect(box_intersect(b,frame.clip),bcol); if(x.border)draw_box(b,0,fcol);
	rect bi=inset(b,2);if(x.scrollbar)bi.w-=image_size(ARROWS->lv[0]).x+3;
	if(!l&&!x.locked&&over(bi)&&(ev.drag?dover(bi):1))uicursor=cursor_ibeam;
	layout_richtext(deck,value->table,fnt,x.align,bi.w);
	glyph_box last=layout_last(fnt);int eol=last.c!='\n'?0: last.pos.h;
	scrollbar(b,MAX(0,(last.pos.y+last.pos.h+eol)-bi.h),10,bi.h,&value->scroll,x.scrollbar,x.show==show_invert);
	int sel=!x.locked&&!l&&wid.active==wid.count;
	// find active link (if any)
	lv*alink=NULL;if(x.locked&&!sel&&in_layer()&&x.locked&&(ev.md||ev.drag))for(int z=0;z<layout_count;z++){
		glyph_box g=layout[z] ;if(g.pos.w<1)continue; // skip squashed spaces/newlines
		g.pos.y-=value->scroll;if(g.pos.y+g.pos.h<0||g.pos.y>bi.h)continue; g.pos.x+=bi.x, g.pos.y+=bi.y; // coarse clip
		if(lis(g.arg)&&g.arg->c&&dover(g.pos)&&over(g.pos)){alink=g.arg;break;}
	}
	if(!x.locked&&!l&&dover(bi)&&(ev.md||ev.mu||ev.drag)){
		int i=layout_index((pair){ev.pos.x-bi.x,ev.pos.y-bi.y+value->scroll});
		if(ev.md&&!ev.shift){wid.cursor.x=wid.cursor.y=i;}else{wid.cursor.y=i;}
		if(ev.dclick){ // double-click to select a word or whitespace span:
			int a,w=layout_count&&strchr("\n ",layout[MIN(wid.cursor.y,layout_count-1)].c)?1:0;
			a=wid.cursor.y;while(a>=0&&a<layout_count&&(w^!strchr("\n ",layout[a].c)))a--;wid.cursor.x=a+1;
			a=wid.cursor.y;while(      a<layout_count&&(w^!strchr("\n ",layout[a].c)))a++;wid.cursor.y=a;
		}
		rect c=layout_cursor(wid.cursor.y,fnt,x);c.y-=value->scroll;int ch=MIN(bi.h,c.h);
		if(c.y<0)value->scroll-=4;if(c.y+ch>bi.h)value->scroll+=CLAMP(1,(c.y+ch)-bi.h,4); // drag to scroll!
		if(!sel&&wid.fv&&!kc.on)field_exit(); wid.active=wid.count,sel=1;
	}
	if(sel&&in_layer()&&!over(b)&&ev.md&&!kc.on){sel=0;wid.active=-1;field_exit();}
	if(sel){if(wid.gv)grid_exit();wid.infield=1,wid.f=x,wid.fv=value,wid.ft=target;keycaps_enter();}
	// render
	rect bc=box_intersect(frame.clip,bi); rect oc=frame.clip;frame.clip=bc; char*pal=patterns_pal(ifield(deck,"patterns"));
	for(int z=0;z<layout_count;z++){
		glyph_box g=layout[z] ;if(g.pos.w<1)continue; // skip squashed spaces/newlines
		g.pos.y-=value->scroll;if(g.pos.y+g.pos.h<0||g.pos.y>bc.h)continue; g.pos.x+=bi.x, g.pos.y+=bi.y; // coarse clip
		if(lis(g.arg)&&g.arg->c){
			draw_hline(g.pos.x,g.pos.x+g.pos.w,g.pos.y+g.pos.h-1,alink==g.arg?fcol:19);
			int a=x.locked&&in_layer()&&over(g.pos)&&target;if(a&&!ev.drag)uicursor=cursor_point;
			if(a&&ev.mu&&dover(g.pos)){msg.target_link=target,msg.arg_link=g.arg;}
		}
		int csel=sel&&wid.cursor.x!=wid.cursor.y&&z>=MIN(wid.cursor.x,wid.cursor.y)&&z<MAX(wid.cursor.x,wid.cursor.y);
		if(csel)draw_rect(box_intersect(g.pos,frame.clip),fcol);
		if(image_is(g.arg)){buffer_paste(g.pos,frame.clip,g.arg->b,frame.buffer,x.show!=show_transparent);if(csel)draw_invert(pal,g.pos);}
		else{font_each(g.font,g.c)if(font_gpix(g.font,g.c,b,a)&&inclip(g.pos.x+b,g.pos.y+a))PIX(g.pos.x+b,g.pos.y+a)=csel?bcol:fcol;}
	}
	if(sel&&wid.cursor_timer<FIELD_CURSOR_DUTY){
		rect c=layout_cursor(wid.cursor.y,fnt,x);c.y-=value->scroll;c.y+=bi.y,c.x+=bi.x;
		draw_invert(pal,box_intersect(c,frame.clip));
	}
	if(target&&os!=value->scroll)iwrite(target,lmistr("scroll"),lmn(value->scroll)),mark_dirty();
	frame.clip=oc;
	if(!x.locked&&in_widgets())wid.count++;
}
void field_showcursor(void){
	rect b=wid.f.size, bi=inset(b,2);if(wid.f.scrollbar)bi.w-=image_size(ARROWS->lv[0]).x+3;
	lv*fnt=wid.f.font?wid.f.font: wid.f.style==field_code?FONT_MONO: FONT_BODY;
	layout_richtext(deck,wid.fv->table,fnt,wid.f.align,bi.w); int os=wid.fv->scroll;
	rect c=layout_cursor(wid.cursor.y,fnt,wid.f);c.y-=wid.fv->scroll;int ch=MIN(bi.h,c.h);
	if(c.y<0){wid.fv->scroll+=c.y;}if(c.y+ch>=bi.h){wid.fv->scroll+=((c.y+ch)-bi.h);}
	if(wid.ft&&os!=wid.fv->scroll)iwrite(wid.ft,lmistr("scroll"),lmn(wid.fv->scroll)),mark_dirty();
}
void field_apply(lv*v,pair c){
	wid.fv->table=v,wid.cursor=c;if(wid.cursor.x<0)wid.cursor.x=0;if(wid.cursor.y<0)wid.cursor.y=0;
	field_showcursor();wid.field_dirty=1,wid.change_timer=FIELD_CHANGE_DELAY;
}
void field_undo(void){lv*x=wid.hist->lv[--(wid.hist_cursor)];field_apply(x->lv[0],getpair(x->lv[1]));}
void field_redo(void){lv*x=wid.hist->lv[(wid.hist_cursor)++];field_apply(x->lv[2],getpair(x->lv[3]));}
void field_edit(lv*font,lv*arg,char*text,pair pos){
	wid.hist->c=wid.hist_cursor; lv*o=lml(4);ll_add(wid.hist,o); pair c={0,0};
	o->lv[0]=wid.fv->table                                   ,o->lv[1]=lmpair(wid.cursor); // before
	o->lv[2]=rtext_splice(wid.fv->table,font,arg,text,pos,&c),o->lv[3]=lmpair(c         ); // after
	field_redo();
}
void field_editr(lv*rtext,pair pos){
	wid.hist->c=wid.hist_cursor; lv*o=lml(4);ll_add(wid.hist,o); pair c={0,0};
	o->lv[0]=wid.fv->table                                   ,o->lv[1]=lmpair(wid.cursor); // before
	o->lv[2]=rtext_splicer(wid.fv->table,rtext,       pos,&c),o->lv[3]=lmpair(c         ); // after
	field_redo();
}
pair field_position(int cursor){
	if(layout_count<1)return (pair){1,1};cursor=MAX(0,MIN(cursor,layout_count+1));
	int e=cursor>=layout_count?1:0, i=cursor-e, l=layout[i].line, c=i-lines[l].range.x;
	return (pair){l+1,c+1+e};
}
pair field_sel_lines(void){
	int a=MIN(wid.cursor.x,wid.cursor.y),b=MAX(wid.cursor.x,wid.cursor.y);field_showcursor();
	while(a             &&layout[a-1].c!='\n')a--;
	while(b<layout_count&&layout[b]  .c!='\n')b++;
	return (pair){a,b};
}
void field_comment(void){
	pair p=field_sel_lines();int ac=1,z=p.x;while(z<p.y){
		while(z<p.y&&layout[z].c==' ')z++;
		if   (z<p.y&&layout[z].c!='#')ac=0;
		while(z<p.y&&layout[z].c!='\n')z++;z++;
	}
	str r=str_new();z=p.x;while(z<p.y){
		while(z<p.y&&layout[z].c==' ')str_addc(&r,' '),z++;
		if(ac){if(layout[z].c=='#'){z++;if(z<p.y&&layout[z].c==' ')z++;}}else{str_addz(&r,"# ");}
		while(z<p.y&&layout[z].c!='\n')str_addc(&r,layout[z++].c);
		if(z<p.y&&layout[z].c=='\n')str_addc(&r,'\n'),z++;
	}field_edit(lmistr(""),lmistr(""),lmstr(r)->sv,p);wid.cursor=(pair){p.x,wid.cursor.y};
}
void field_indent(int add){
	pair p=field_sel_lines();str r=str_new();int z=p.x;while(z<p.y){
		if(add){str_addc(&r,' ');}else{if(layout[z].c==' ')z++;}
		while(z<p.y&&layout[z].c==' ')str_addc(&r,' '),z++;
		while(z<p.y&&layout[z].c!='\n')str_addc(&r,layout[z++].c);
		if(z<p.y&&layout[z].c=='\n')str_addc(&r,'\n'),z++;
	}field_edit(lmistr(""),lmistr(""),lmstr(r)->sv,p);wid.cursor=(pair){p.x,wid.cursor.y};
}
void field_stylespan(lv*font,lv*arg){
	field_edit(font,arg,rtext_string(wid.fv->table,wid.cursor)->sv,wid.cursor);
}
void field_input(char*text){
	if(!wid.infield)return;
	if(!strcmp(text,"\n")){
		if(ms.type==modal_save)ev.action=1;
		if(ev.shift||ms.type==modal_save)return;
	}
	str t=str_new();str_addz(&t,text);str_term(&t);
	field_edit(rtext_font(wid.fv->table,wid.cursor.y),lmistr(""),t.sv,wid.cursor);free(t.sv);
}
void field_keys(int code,int shift){
	if(code==KEY_RETURN&&ms.type==modal_gridcell){modal_exit(1);ev.action=0;return;}
	rect b=wid.f.size, bi=inset(b,2);if(wid.f.scrollbar)bi.w-=image_size(ARROWS->lv[0]).x+3;
	lv*fnt=wid.f.font?wid.f.font: wid.f.style==field_code?FONT_MONO: FONT_BODY; layout_richtext(deck,wid.fv->table,fnt,wid.f.align,bi.w);
	int m=0, s=wid.cursor.x!=wid.cursor.y;
	int l=wid.cursor.y>=layout_count?lines_count-1:layout[wid.cursor.y].line; rect c=layout_cursor(wid.cursor.y,fnt,wid.f);
	if(code==KEY_LEFT     ){m=1;if(s&&!shift){wid.cursor.x=wid.cursor.y=MIN(wid.cursor.x,wid.cursor.y);}else{wid.cursor.y--;}}
	if(code==KEY_RIGHT    ){m=1;if(s&&!shift){wid.cursor.x=wid.cursor.y=MAX(wid.cursor.x,wid.cursor.y);}else{wid.cursor.y++;}}
	if(code==KEY_UP       ){m=1;if(l>=0)wid.cursor.y=layout_index((pair){c.x-1,lines[l].pos.y               -1   });}
	if(code==KEY_DOWN     ){m=1;if(l>=0)wid.cursor.y=layout_index((pair){c.x-1,lines[l].pos.y+lines[l].pos.h+1   });}
	if(code==KEY_PAGEUP   ){m=1;if(l>=0)wid.cursor.y=layout_index((pair){c.x-1,lines[l].pos.y               -bi.h});}
	if(code==KEY_PAGEDOWN ){m=1;if(l>=0)wid.cursor.y=layout_index((pair){c.x-1,lines[l].pos.y+lines[l].pos.h+bi.h});}
	if(code==KEY_HOME     ){m=1;if(ev.alt){wid.cursor.y=0           ;}else if(l>=0)wid.cursor.y=lines[l].range.x;}
	if(code==KEY_END      ){m=1;if(ev.alt){wid.cursor.y=layout_count;}else if(l>=0)wid.cursor.y=lines[l].range.y+(l==lines_count-1?1:0);}
	if(code==KEY_BACKSPACE){field_edit(lmistr(""),lmistr(""),"",s?wid.cursor:(pair){wid.cursor.y-1,wid.cursor.y});}
	if(code==KEY_DELETE   ){field_edit(lmistr(""),lmistr(""),"",s?wid.cursor:(pair){wid.cursor.y,wid.cursor.y+1});}
	if(code==KEY_RETURN){
		if(shift&&wid.ft){field_change();msg.target_run=wid.ft,msg.arg_run=rtext_string(wid.fv->table,s?wid.cursor:(pair){0,RTEXT_END});}
		else{
			int i=0;if(wid.f.style==field_code){pair s=field_sel_lines();while(s.x<layout_count&&layout[s.x].c==' ')i++,s.x++;}
			char tmp[4096];snprintf(tmp,sizeof(tmp),"%-*s",i+1,"\n");field_input(tmp);
		}
	}
	if(code==KEY_TAB&&wid.f.style==field_code){if(!shift&&!s){field_input(" ");}else{field_indent(!shift);}}
	wid.cursor.y=MAX(0,MIN(wid.cursor.y,layout_count)); if(!m)return;
	wid.cursor_timer=0; if(!shift)wid.cursor.x=wid.cursor.y; field_showcursor();
}
void widget_contraption(lv*x);// forward def
void handle_widgets(lv*x,pair offset){
	EACH(z,x){
		lv*w=x->lv[z];
		if(button_is(w)){
			int v=lb(ifield(w,"value")); button p=unpack_button(w);p.size=rect_add(p.size,offset);
			if(widget_button(w,p,v)&&p.style==button_check)iwrite(w,lmistr("value"),lmn(!v)),mark_dirty();
		}
		if(slider_is(w)){slider p=unpack_slider(w);p.size=rect_add(p.size,offset),widget_slider(w,p);}
		if(canvas_is(w)){lv* v=container_image(w,0);canvas p=unpack_canvas(w);p.size=rect_add(p.size,offset);widget_canvas(w,p,v);}
		if(grid_is(w)){grid_val v={0};grid p=unpack_grid(w,&v);p.size=rect_add(p.size,offset);widget_grid(w,p,&v);if(wid.gt==w)wid.gv_slot=v,wid.gv=&wid.gv_slot;}
		if(field_is(w)){
			if(wid.ft==w){widget_field(w,wid.f,wid.fv);}
			else{
				field_val v={0};field p=unpack_field(w,&v);p.size=rect_add(p.size,offset);widget_field(w,p,&v);
				if(wid.ft==w)wid.fv_slot=v,wid.fv=&wid.fv_slot;
			}
		}
		if(contraption_is(w))widget_contraption(w);
	}
}
void widget_contraption(lv*x){
	int show=ordinal_enum(ifield(x,"show"),widget_shows);if(show==show_none)return;
	rect b=rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size")));lv*image=ifield(x,"image");
	rect oc=frame.clip;frame.clip=box_intersect(frame.clip,b);char*pal=patterns_pal(ifield(deck,"patterns"));
	draw_9seg(b,frame.buffer,image->b,getrect(ifield(ifield(x,"def"),"margin")),frame.clip,show==show_solid,show==show_invert?pal:NULL);
	handle_widgets(ivalue(x,"widgets"),(pair){b.x,b.y});frame.clip=oc;
}

int  ui_button  (rect r,char*label,int enable          ){return widget_button(NULL,(button){label,r,FONT_MENU,button_round,show_solid,!enable,0},0);}
int  ui_toggle  (rect r,char*label,int inv,int enable  ){return widget_button(NULL,(button){label,r,FONT_MENU,button_round,inv?show_invert:show_solid,!enable,0},0);}
int  ui_radio   (rect r,char*label,int enable,int value){return widget_button(NULL,(button){label,r,FONT_BODY,button_radio,show_solid,!enable,0},value);}
int  ui_checkbox(rect r,char*label,int enable,int value){return widget_button(NULL,(button){label,r,FONT_BODY,button_check,show_solid,!enable,0},value);}
void ui_field   (rect r,           field_val*value){widget_field(NULL,(field){r,FONT_BODY,show_solid,0,1     ,field_plain,align_left,0},value);}
void ui_dfield  (rect r,int enable,field_val*value){widget_field(NULL,(field){r,FONT_BODY,show_solid,0,1     ,field_plain,align_left,!enable},value);}
void ui_textedit(rect r,int border,field_val*value){widget_field(NULL,(field){r,FONT_BODY,show_solid,1,border,field_plain,align_left,0},value);}
void ui_codeedit(rect r,int border,field_val*value){widget_field(NULL,(field){r,FONT_MONO,show_transparent,1,border,field_code ,align_left,running()},value);}
void ui_richedit(rect r,int border,field_val*value){widget_field(NULL,(field){r,FONT_BODY,show_solid,1,border,field_rich ,align_left,0},value);}
int  ui_table   (rect r,int w0,int w1,int w2,char*fmt,grid_val*value){return widget_grid(NULL,(grid){r,FONT_BODY,{w0,w1,w2,0,0},fmt,2,1,0,0,show_solid,1},value);}
int  ui_list    (rect r,                              grid_val*value){return widget_grid(NULL,(grid){r,FONT_BODY,{0 },"" ,0,1,0,0,show_solid,1},value);}

typedef struct {int type,bval;char label[1024];field_val value;} attr_item; attr_item* attrs=NULL;int attrs_count=0,attrs_size=0,attrs_scroll=0;
int attr_heights[]={0,16,20,20,80,80}; // nil,bool,number,string,code,rtext
void attr_push(attr_item x){grower(attrs,attr_item);attrs[attrs_count++]=x;}
void widget_attributes(rect b){
	draw_box(b,0,1);
	int h=5,lw=0;for(int z=0;z<attrs_count;z++){h+=attr_heights[attrs[z].type]+5;if(attrs[z].type!=attr_bool)lw=MAX(lw,font_textsize(FONT_MENU,attrs[z].label).x);}
	rect bi=scrollbar(b,MAX(0,h-b.h),10,b.h,&attrs_scroll,1,0);bi.y+=1;rect oc=frame.clip;frame.clip=bi;lw=MIN(lw+10,bi.w*.6);
	pair bp=ev.pos,bd=ev.dpos;int bs=ev.scroll;ev.scroll=0;if(!over(bi))ev.pos=(pair){-1,-1};if(!dover(bi))ev.dpos=(pair){-1,-1};
	int y=5;for(int z=0;z<attrs_count;z++){
		rect lb={bi.x+5,bi.y+y-attrs_scroll,lw,attr_heights[attrs[z].type]}, wb={lb.x+lb.w+5,lb.y,(bi.w-15)-lb.w,lb.h};y+=lb.h+5;
		if(attrs[z].type==attr_bool){lb.w=bi.w-10;if(ui_checkbox(lb,attrs[z].label,1,attrs[z].bval))attrs[z].bval^=1;continue;}
		else{draw_text_fit(lb,attrs[z].label,FONT_MENU,1);}
		if(attrs[z].type==attr_number)ui_field(wb,&attrs[z].value);
		if(attrs[z].type==attr_string)ui_field(wb,&attrs[z].value);
		if(attrs[z].type==attr_code  )ui_codeedit(wb,1,&attrs[z].value);
		if(attrs[z].type==attr_rich  )ui_richedit(wb,1,&attrs[z].value);
	}frame.clip=oc,ev.pos=bp,ev.dpos=bd,ev.scroll=bs;
}

lv* draw_lil(pair size,int align,int bare,lv*x){
	int GAP=50, w=size.x-GAP, xo=align==align_right?GAP: align==align_left?0: GAP/2;
	lv*r=lmbuff(size);cstate t=frame;frame=draw_buffer(r);
	if(lit(x)){
		int hh=x->c?3+font_h(FONT_BODY):3, ch=font_h(FONT_MONO), fh=font_h(FONT_BODY), rows=MIN((size.y-(hh+fh))/ch,x->n);
		lv* f=l_take(ZERO,x);int cw[256];
		for(int c=0;c<x->c&&c<256;c++){
			int dr=rows<x->n?rows-1:rows;cw[c]=0;for(int r=0;r<dr;r++){
				str t=str_new();show(&t,x->lv[c]->lv[r],0);lv*s=lmstr(t);ll_add(f->lv[c],s);
				int tw=font_textsize(FONT_MONO,s->sv).x+10;cw[c]=MAX(cw[c],tw);
			}if(rows<x->n)ll_add(f->lv[c],lmistr(" \x7f"));
			int tw=font_textsize(FONT_BODY,x->kv[c]->sv).x+10;cw[c]=MIN(100,MAX(cw[c],tw));
		}torect(f);
		int cols=0,tw=0,ve=0;for(int c=0;c<x->c&&c<256;c++){if(tw+cw[c]>=w){ve=1;break;};cols++;if(c+1<=x->c&&c+1<=256)tw+=cw[c];}
		xo=align==align_right?size.x-tw: align==align_left?0: (size.x-tw)/2;
		int cx=xo;for(int c=0;c<cols;c++){
			if(c)draw_vline(cx,0,hh+ch*rows+(rows==0),1);draw_text_fit((rect){cx+2,0,cw[c]-4,hh},x->kv[c]->sv,FONT_BODY,1);
			for(int r=0;r<rows;r++)draw_text_fit((rect){cx+2,hh+ch*r,cw[c]-4,ch},f->lv[c]->lv[r]->sv,FONT_MONO,1);
			if(c+1<=cols)cx+=cw[c];
		}draw_hline(xo,cx,hh-1,1);int bh=hh+ch*rows+1+(rows==0);if(cx==xo)cx+=MIN(25,w);
		char desc[4096];snprintf(desc,sizeof(desc),"(%d column%s, %d row%s.)",x->c,x->c==1?"":"s",x->n,x->n==1?"":"s");
		pair ds=font_textsize(FONT_BODY,desc);draw_text_fit((rect){xo+tw-ds.x,bh,w,fh},desc,FONT_BODY,1);
		draw_box((rect){xo,0,cx-xo,bh},0,1);if(ve)draw_vline(cx-1,0,bh,13);
	}
	else if(image_is(x)){
		pair s=image_size(x);char desc[4096];snprintf(desc,sizeof(desc),"(%d x %d)",s.x,s.y);pair ds=font_textsize(FONT_BODY,desc);
		int mh=MAX(1,MIN(s.y,size.y-font_h(FONT_BODY))), mw=MAX(1,MIN(s.x,w)); float scale=s.x==0&&s.y==0?1:MIN(mw/(s.x*1.0),mh/(s.y*1.0));
		int iw=scale*s.x, ih=scale*s.y; rect b={(xo-2)+(w-MAX(iw,ds.x)),0,iw+2,ih+2};
		draw_scaled(inset(b,1),x->b,1),draw_box(b,0,1),draw_text((rect){b.x,b.y+b.h,ds.x,ds.y},desc,FONT_BODY,1);
	}
	else{
		char*c=NULL;if(bare){c=ls(x)->sv;}else{str t=str_new();show(&t,x,0);c=lmstr(t)->sv;}
		layout_plaintext(c,FONT_MONO,align,(pair){w,size.y});draw_text_wrap((rect){xo,0,w,size.y},1);
	}
	for(int y=size.y-1;y>0;y--){int f=0;for(int x=0;x<size.x;x++)if(r->sv[x+(y*size.x)]){f=1;break;}if(f){break;}else{r->c-=size.x;}}
	return frame=t,r;
}

// The Listener

#define LISTEN_W (context.size.x-22)
#define LISTEN_H 100
void listen_show_image(lv*x,lv*v){
	frame=context;while(li.hist->c>=LISTEN_LINES)ll_unshift(li.hist);
	ll_add(li.hist,lml2(x,v));
	li.scroll=RTEXT_END;
}
void listen_show(int align,int bare,lv*x){
	listen_show_image(draw_lil((pair){LISTEN_W-18,LISTEN_H-5},align,bare,x),x);
}
lv* n_show(lv*self,lv*a){
	(void)self;if(a->c<2){listen_show(align_right,0,l_first(a));}
	else{str s=str_new();EACH(z,a){if(z)str_addc(&s,' ');show(&s,a->lv[z],0);}listen_show(align_right,1,lmstr(s));}
	return l_first(a);
}
lv* n_print(lv*self,lv*a){
	(void)self;if(a->c<2){listen_show(align_right,1,ls(l_first(a)));}
	else{a=l_format(ls(l_first(a)),l_drop(ONE,a));listen_show(align_right,1,a);}return a;
}
lv* n_pre_listen(lv*self,lv*a){
	(void)self;EACH(z,li.vars)if(!dget(ev(),li.vars->kv[z]))dset(ev(),li.vars->kv[z],li.vars->lv[z]);
	if(ob.sel->c&&uimode==mode_object)dset(ev(),lmistr("selected"),l_drop(ZERO,ob.sel));
	return a;
}
lv* n_post_listen(lv*self,lv*a){
	(void)self;EACH(z,ev())dset(li.vars,ev()->kv[z],ev()->lv[z]);
	dset(li.vars,lmistr("_"),a);listen_show(align_right,0,a);return a;
}
lv* n_post_query(lv*self,lv*a){
	(void)self;ms.grid=(grid_val){lt(a),0,-1,-1};return a;
}
void listener_eval(void){
	lv*str=rtext_all(ms.text.table);if(str->c<1)return;
	lv*prog=parse(str->sv);if(perr()){char e[4096];snprintf(e,sizeof(e),"error: %s",par.error);listen_show(align_right,1,lmcstr(e));return;}
	ms.text=(field_val){rtext_cast(lmistr("")),0};listen_show(align_left,1,str);lv*b=lmblk();
	lv*target=uimode==mode_script?sc.target: ob.sel->c==1?ob.sel->lv[0]: con();
	blk_lit(b,lmnat(n_pre_listen ,NULL)),blk_lit(b,LNIL),blk_op(b,CALL),blk_op(b,DROP);blk_cat(b,prog);
	blk_lit(b,lmnat(n_post_listen,NULL)),blk_op (b,SWAP),blk_op(b,CALL),blk_op(b,DROP);fire_hunk_async(target,b);
}
void listener(rect r){
	int th=0;EACH(z,li.hist)th+=buff_size(li.hist->lv[z]->lv[0]).y+5;int h=MIN(th,LISTEN_H);
	rect esize={r.x+(r.w-LISTEN_W)/2,r.y+r.h-49,LISTEN_W,50};
	rect tsize={esize.x,esize.y-(h?h+5:0),esize.w,h};
	rect bsize={esize.x-5,esize.y-5-(h?tsize.h+5:0),esize.w+10,(h?tsize.h+5:0)+esize.h+20};
	char*pal=patterns_pal(ifield(deck,"patterns"));
	draw_shadow(bsize,1,32,1),ui_codeedit(esize,1,&ms.text);
	if(h){
		rect b=scrollbar(tsize,MAX(0,th-LISTEN_H),10,tsize.h,&li.scroll,h>=LISTEN_H,0);
		int cy=0;EACH(z,li.hist){
			lv*l=li.hist->lv[z]->lv[0],*t=li.hist->lv[z]->lv[1]; pair s=buff_size(l); rect lb={b.x,b.y+cy-li.scroll,s.x,s.y};
			buffer_paste(lb,b,l,frame.buffer,0);cy+=s.y+5;
			lb=box_intersect(b,lb);int v=over(lb), a=v&&dover(lb);
			if(v)uicursor=cursor_point,draw_box(inset(lb,-1),0,13); if(a&&(ev.md||ev.drag))draw_invert(pal,lb);
			if(a&&ev.mu){
				if(image_is(t)){t=l_format(lmistr("image[\"%s\"]"),ifield(t,"encoded"));}
				else if(!lis(t)){str s=str_new();show(&s,t,0);t=lmstr(s);}
				ms.text=(field_val){rtext_cast(t),0};
			}
		}
	}
}
lv* n_panic(lv*self,lv*z){
	do_panic=1,halt();for(int z=gc.ss-1;z>=0;z--){gc.st[z].t->c=0;}
	modal_enter(modal_listen);
	pair s={(512-22)-18,16};rect b=rect_pair((pair){0,0},s);lv*r=lmbuff(s);cstate t=frame;frame=draw_buffer(r);
	draw_box(b,0,35),draw_textc(inset(b,2),"PANIC",FONT_MONO,35),frame=t,listen_show_image(r,z);
	n_show(self,z),dset(li.vars,lmistr("_"),l_first(z));return LNIL;
}

// Audio Editor

void sound_apply(lv*v){int len=v->b->c;au.target->b->c=len;memcpy(au.target->b->sv,v->b->sv,len);mark_dirty();}
void sound_undo(void){lv*x=au.hist->lv[--(au.hist_cursor)];sound_apply(x->lv[0]);}
void sound_redo(void){lv*x=au.hist->lv[(au.hist_cursor)++];sound_apply(x->lv[1]);}
lv* sound_slice(pair range){lv*r=lms(10*SFX_RATE);r->c=0;for(int z=range.x;z<range.y;z++)r->sv[r->c++]=au.target->b->sv[z];return sound_make(r);}
lv* sound_selected(void){return sound_slice(au.sel.x==au.sel.y?(pair){0,au.target->b->c}:au.sel);}
void sound_edit(lv*v){au.hist->c=au.hist_cursor,ll_add(au.hist,lml2(sound_slice((pair){0,au.target->b->c}),v)),sound_redo();}
void sound_delete(void){
	int len=au.target->b->c; pair sel=au.sel.x==au.sel.y?(pair){0,len}:au.sel;
	lv*r=lms(len-(sel.y-sel.x));
	memcpy(r->sv      ,au.target->b->sv      ,sel.x    );
	memcpy(r->sv+sel.x,au.target->b->sv+sel.y,len-sel.y);
	sound_edit(sound_make(r));au.head=au.sel.y=au.sel.x;
}
void sound_finish(void){
	record_finish(au.input);
	au.head=(au.sel.x!=au.sel.y)?au.sel.x:0;
	sound_undo(),sound_redo();
}
void record_pump(void* userdata,Uint8* stream,int len){
	(void)userdata;
	int h=au.head, end=(au.sel.x!=au.sel.y)?au.sel.y:(10*SFX_RATE); lv*edit=au.hist->lv[au.hist_cursor-1]->lv[1];
	char*b=au.target->b->sv, *c=edit->b->sv;
	for(int z=0;z<len&&h<end;z++){b[h]=c[h]=stream[z];h++;}
	au.head=h;if(h>au.target->b->c)au.target->b->c=edit->b->c=h;
	if(h>=end){sound_finish();au.mode=record_stopped;}
}
lv* sounds_enumerate(void){
	lv*r=lmt(),*i=lml(0),*n=lml(0),*b=lml(0),*s=lml(0),*sounds=ifield(deck,"sounds");
	dset(r,lmistr("icon"),i),dset(r,lmistr("name"),n),dset(r,lmistr("bytes"),b),dset(r,lmistr("secs"),s);
	EACH(z,sounds){
		ll_add(i,lmn(icon_sound)),ll_add(n,sounds->kv[z]);lv*sn=sounds->lv[z];
		ll_add(b,l_format(lmistr("%.2fkb"),lmn(ln(ifield(sn,"size"))/1000.0*1.33)));
		ll_add(s,l_format(lmistr("%.2fs" ),ifield(sn,"duration")));
	}return torect(r);
}
lv* contraptions_enumerate(void){
	lv*r=lmt(),*i=lml(0),*n=lml(0),*defs=ifield(deck,"contraptions");dset(r,lmistr("icon"),i),dset(r,lmistr("name"),n);
	EACH(z,defs){ll_add(i,lmn(icon_app)),ll_add(n,defs->kv[z]);}
	return torect(r);
}
lv*n_readfile(lv*self,lv*a){
	lv*name=ls(l_first(a)),*hint=a->c>1?ls(a->lv[1]):lms(0);
	if(!strcmp(hint->sv,"array"))return readbin(name);
	if(has_suffix(name->sv,".png" ))return readimage(name->sv,!strcmp(hint->sv,"gray"));
	if(has_suffix(name->sv,".bmp" ))return readimage(name->sv,!strcmp(hint->sv,"gray"));
	if(has_suffix(name->sv,".jpg" ))return readimage(name->sv,!strcmp(hint->sv,"gray"));
	if(has_suffix(name->sv,".jpeg"))return readimage(name->sv,!strcmp(hint->sv,"gray"));
	if(has_suffix(name->sv,".gif" ))return n_readgif(self,a);
	if(has_suffix(name->sv,".wav" ))return n_readwav(self,a);
	if(has_suffix(name->sv,".deck"))return n_readdeck(self,a);
	return n_read(self,a);
}

// Modal Helpers

lv* res_enumerate(lv*source){
	lv*r=lmt(),*i=lml(0),*n=lml(0),*v=lml(0);dset(r,lmistr("icon"),i),dset(r,lmistr("name"),n),dset(r,lmistr("value"),v);
	lv*pat=ifield(source,"patterns");
	char*pal=patterns_pal(pat),*pv=patterns_write(pat)->sv;lv*pa=anims_write(pal),*da=l_parse(lmistr("%j"),lmistr(DEFAULT_ANIMS));
	if(!matchr(pa,da)||strcmp(pv,DEFAULT_PATTERNS)){ll_add(i,lmn(icon_pat)),ll_add(n,lmistr("patterns")),ll_add(v,pat);}
	lv*fonts=ifield(source,"fonts" );fonts=l_drop(lmistr("body"),fonts),fonts=l_drop(lmistr("menu"),fonts),fonts=l_drop(lmistr("mono"),fonts);
	EACH(z,fonts )ll_add(i,lmn(icon_font )),ll_add(n,fonts ->kv[z]),ll_add(v,fonts ->lv[z]);
	lv*sounds=ifield(source,"sounds");EACH(z,sounds)ll_add(i,lmn(icon_sound)),ll_add(n,sounds->kv[z]),ll_add(v,sounds->lv[z]);
	lv*modules=ifield(source,"modules");EACH(z,modules)ll_add(i,lmn(icon_lil)),ll_add(n,modules->kv[z]),ll_add(v,modules->lv[z]);
	lv*defs=ifield(source,"contraptions");EACH(z,defs)ll_add(i,lmn(icon_app)),ll_add(n,defs->kv[z]),ll_add(v,defs->lv[z]);
	return torect(r);
}
void draw_thumbnail(lv*card,rect r){
	lv*back=ifield(card,"image");r=inset(r,1);draw_rect(r,0);if(!is_empty(back))draw_scaled(r,back->b,1);
	lv*wids=ifield(card,"widgets");pair s=getpair(ifield(card,"size"));float xr=r.w*(1.0/s.x), yr=r.h*(1.0/s.y);
	EACH(z,wids){
		widget w=unpack_widget(wids->lv[z]);
		draw_box(box_intersect((rect){r.x+w.size.x*xr,r.y+w.size.y*yr,w.size.w*xr,w.size.h*yr},r),0,w.show==show_invert?0:1);
	}
}
lv* draw_widget(lv*w){
	if(canvas_is(w))return n_canvas_copy(w,lml(0));
	pair rsize=getpair(ifield(w,"size"));lv*buff=lmbuff(rsize),*r=image_make(buff);int im=ms.in_modal,it=ms.type;ms.in_modal=1;
	cstate t=frame;frame=draw_buffer(buff);event_state eb=ev;ev=(event_state){0};ms.type=modal_about;menus_clear(); // !!!
	if     (button_is     (w)){                button p=unpack_button(w)  ;p.size.x=0,p.size.y=0;widget_button(w,p,lb(ifield(w,"value")));}
	else if(slider_is     (w)){                slider p=unpack_slider(w)  ;p.size.x=0,p.size.y=0;widget_slider(w,p);}
	else if(grid_is       (w)){grid_val  v={0};grid   p=unpack_grid (w,&v);p.size.x=0,p.size.y=0;widget_grid (w,p,&v);}
	else if(field_is      (w)){field_val v={0};field  p=unpack_field(w,&v);p.size.x=0,p.size.y=0;widget_field(w,p,&v);}
	else if(contraption_is(w)){pair o=getpair(ifield(w,"pos"));iwrite(w,lmistr("pos"),lmpair((pair){0,0}));widget_contraption(w);iwrite(w,lmistr("pos"),lmpair(o));}
	return ev=eb,frame=t,ms.in_modal=im,ms.type=it,r;
}
lv* draw_con(lv*card,int active){
	int im=ms.in_modal,it=ms.type;ms.in_modal=active;
	pair rsize=getpair(ifield(card,"size"));lv*buff=lmbuff(rsize),*r=image_make(buff);
	cstate t=frame;frame=draw_buffer(buff);event_state eb=ev;ev=(event_state){0};if(active){ms.type=modal_about,menus_clear();}
	lv*back=ifield(card,"image");pair bsize=image_size(back);
	if(bsize.x!=0&&bsize.y!=0)buffer_paste(rect_pair((pair){0,0},bsize),frame.clip,back->b,frame.buffer,1);
	lv*wids=ivalue(card,"widgets");if(uimode!=mode_draw||dr.show_widgets)EACH(z,wids){
		lv*w=wids->lv[z];
		if     (button_is     (w)){widget_button(w,unpack_button(w),lb(ifield(w,"value")));}
		else if(slider_is     (w)){widget_slider(w,unpack_slider(w));}
		else if(canvas_is     (w)){widget_canvas(w,unpack_canvas(w),container_image(w,0));}
		else if(grid_is       (w)){grid_val  v={0};grid  p=unpack_grid (w,&v);widget_grid (w,p,&v);}
		else if(field_is      (w)){field_val v={0};field p=unpack_field(w,&v);widget_field(w,p,&v);}
		else if(contraption_is(w)){widget_contraption(w);}
	}return ev=eb,frame=t,ms.in_modal=im,ms.type=it,r;
}
rect modal_rtext(pair extra){
	pair size={200,100};
	if(lit(ms.message)){
		pair t=layout_richtext(deck,ms.message,FONT_BODY,align_center,size.x);
		rect b=draw_modalbox((pair){t.x+extra.x,t.y+extra.y});draw_text_rich((rect){b.x,b.y,b.w,t.y},1,1);return b;
	}
	else{
		pair t=layout_plaintext(ms.message->sv,FONT_BODY,align_center,size);
		rect b=draw_modalbox((pair){t.x+extra.x,t.y+extra.y});draw_text_wrap((rect){b.x,b.y,b.w,t.y},1);return b;
	}
}
char title_buffer[4096];
char* title_caps(char*name,char*suffix){
	unsigned int i=0,j=0,w=1;
	while(i+1<sizeof(title_buffer)&&name  [i]){char c=name[i];if(w)c=drom_toupper(c);w=isspace(c);title_buffer[i++]=c;}
	while(i+1<sizeof(title_buffer)&&suffix[j])title_buffer[i++]=suffix[j++];return title_buffer[i]='\0',title_buffer;
}
void do_transition(float tween,int errors){
	cstate f=frame;n_canvas_clear(ms.canvas,lml(0));
	lv*a=lml(4);a->lv[0]=ms.canvas,a->lv[1]=ms.carda,a->lv[2]=ms.cardb,a->lv[3]=lmn(tween);
	lv*p=lmblk();blk_lit(p,ms.trans),blk_lit(p,a),blk_op(p,CALL);lv*e=lmenv(NULL);
	interpreter_lock(),pushstate(e),issue(e,p);int quota=TRANS_QUOTA;while(quota&&running())runop(),quota--;
	if(running()&&errors){
		char e[4096];snprintf(e,sizeof(e),"warning: transition %s exceeded quota and was halted.",ms.trans->sv);
		listen_show(align_right,1,lmcstr(e));ms.time_curr=ms.time_end;
	}
	popstate(),interpreter_unlock();frame=f;sleep_play=0,sleep_frames=0;
}

// Modal Dialogues

void modal_enter(int type){
	ev.md=ev.mu=ev.dclick=0; // discard inputs from this frame!
	menus_clear(); // modals may have their own menus; clear selection as a sanity-check.
	if(ms.type==modal_trans)return;
	ms.from_listener=ms.type==modal_listen;
	ms.from_keycaps=kc.on;
	ms.type=ms.subtype=type;
	ms.old_wid=wid;
	if(wid.gv==&wid.gv_slot)ms.old_wid.gv=&ms.old_wid.gv_slot;
	if(wid.fv==&wid.fv_slot)ms.old_wid.fv=&ms.old_wid.fv_slot;
	wid=(widget_state){0};wid.hist=lml(0);
	if(enable_touch){wid.active=type==modal_link||type==modal_gridcell||type==modal_listen?0:-1;}
	if(type==modal_query){
		ms.grid=(grid_val){ms.old_wid.gv->table,0,-1,-1};
		ms.text=(field_val){rtext_cast(lmistr("select from me.value")),0};
	}
	if(type==modal_listen){
		if(uimode==mode_script){
			lv*text=rtext_all(sc.f.table);parse(text->sv);
			if(perr()){listen_show(align_right,1,lmcstr("note: this script contains an error.\nexecuting under last saved version!"));}
			else{script_save(text);}
		}
		ms.text=(field_val){rtext_cast(lmistr("")),0};li.scroll=RTEXT_END;
	}
	if(type==modal_recording){
		au.head=0,au.sel=(pair){0,0},au.mode=record_stopped,au.hist->c=0,au.hist_cursor=0;
		lv*sounds=ifield(deck,"sounds");EACH(z,sounds)if(sounds->lv[z]==au.target)ms.name=(field_val){rtext_cast(sounds->kv[z]),0};
		if(!ms.name.table)ms.name.table=rtext_cast(lmistr("unknown sound"));
		int s=ln(ifield(au.target,"size"));sound_resize(au.target,10*SFX_RATE);au.target->b->c=s; // force pre-alloc to 10secs
		if(!nosound&&au.input==0&&record_possible()){
			int id=record_begin();
			if(id>0)au.input=id;
		}
	}
	if(type==modal_cards){ms.grid=(grid_val){NULL,0,-1,-1};}
	if(type==modal_orderwids){ms.grid=(grid_val){NULL,0,-1,-99};}
	if(type==modal_sounds){ms.grid=(grid_val){sounds_enumerate(),0,-1,-1};}
	if(type==modal_contraptions||type==modal_pick_contraption){ms.grid=(grid_val){contraptions_enumerate(),0,-1,-1};}
	if(type==modal_fonts){
		lv*r=lmt(),*i=lml(0),*n=lml(0),*fonts=ifield(deck,"fonts");dset(r,lmistr("icon"),i),dset(r,lmistr("name"),n);
		int fi=-1;EACH(z,fonts){
			if(uimode==mode_object){int f=1;EACH(o,ob.sel)if(!matchr(ifield(ob.sel->lv[o],"font"),fonts->lv[z])){f=0;break;}if(f)fi=z;}
			else if(ms.old_wid.ft){if(matchr(ifield(ms.old_wid.ft,"font"),fonts->lv[z]))fi=z;}
			ll_add(i,lmn(icon_font)),ll_add(n,fonts->kv[z]);
		}ms.grid=(grid_val){torect(r),-99,fi,-1};
	}
	if(type==modal_resources){
		ms.message=NULL;
		ms.grid =(grid_val){lmt(),0,-1,-1};
		ms.grid2=(grid_val){res_enumerate(deck),0,-1,-1};
	}
	if(type==modal_link){
		lv*t=ms.old_wid.fv->table;int i=rtext_get(t,ms.old_wid.cursor.y);
		lv*ol=dget(t,lmistr("arg"))->lv[i<0?0:i];
		ms.text=(field_val){rtext_cast(ol),0};if(ol->c)ms.old_wid.cursor=rtext_getr(t,ms.old_wid.cursor.y);
	}
	if(type==modal_grid){
		ms.name=(field_val){rtext_cast(lmn(dr.grid_size.x)),0};
		ms.text=(field_val){rtext_cast(lmn(dr.grid_size.y)),0};
	}
	if(type==modal_deck_props){
		ms.name=(field_val){rtext_cast(ifield(deck,"name")),0};
		ms.text=(field_val){rtext_cast(ifield(deck,"author")),0};
	}
	if(type==modal_button_props){
		lv*w=ob.sel->lv[0];
		ms.name=(field_val){rtext_cast(ifield(w,"name")),0};
		ms.text=(field_val){rtext_cast(ifield(w,"text")),0};
		ms.form0=(field_val){rtext_cast(ifield(w,"shortcut")),0};
	}
	if(type==modal_field_props){
		lv*w=ob.sel->lv[0];
		ms.name=(field_val){rtext_cast(ifield(w,"name")),0};
		ms.text=(field_val){ifield(w,"value"),0};
	}
	if(type==modal_grid_props){
		lv*w=ob.sel->lv[0];
		ms.name=(field_val){rtext_cast(ifield(w,"name")),0};
		ms.text=(field_val){rtext_cast(ifield(w,"format")),0};
		str r=str_new();fjson(&r,l_cols(ifield(w,"value")));ms.edit_json=1;
		ms.form0=(field_val){rtext_cast(lmstr(r)),0};
	}
	if(type==modal_slider_props){
		lv*w=ob.sel->lv[0];
		ms.name =(field_val){rtext_cast(ifield(w,"name"    ))       ,0};
		ms.text =(field_val){rtext_cast(ifield(w,"format"  ))       ,0};
		ms.form0=(field_val){rtext_cast(ifield(w,"interval")->lv[0]),0};
		ms.form1=(field_val){rtext_cast(ifield(w,"interval")->lv[1]),0};
		ms.form2=(field_val){rtext_cast(ifield(w,"step"    ))       ,0};
	}
	if(type==modal_canvas_props){
		lv*w=ob.sel->lv[0];
		ms.name=(field_val){rtext_cast(ifield(w,"name")),0};
		ms.text=(field_val){rtext_cast(ifield(w,"scale")),0};
	}
	if(type==modal_contraption_props){
		lv*w=ob.sel->lv[0];
		ms.name=(field_val){rtext_cast(ifield(w,"name")),0};
		attrs_count=attrs_scroll=0;lv*a=ifield(ifield(w,"def"),"attributes");
		for(int z=0;z<a->n;z++){
			attr_item i={0};lv*v=iwrite(w,a->lv[0]->lv[z],NULL);
			i.type=ordinal_enum(a->lv[2]->lv[z],attribute_types);
			snprintf(i.label,sizeof(i.label),"%s",a->lv[1]->lv[z]->sv);
			if     (i.type==attr_bool){i.bval=lb(v);}
			else if(i.type==attr_rich){i.value=(field_val){rtext_cast(v),0};}
			else {i.value=(field_val){rtext_cast(ls(v)),0};}
			attr_push(i);
		}
	}
	if(type==modal_prototype_props){
		lv*c=con();
		ms.name =(field_val){rtext_cast(ifield(c,"name")),0};
		ms.form0=(field_val){rtext_cast(ifield(c,"description")),0};
		ms.form1=(field_val){rtext_cast(ifield(c,"template")),0};
		ms.form2=(field_val){rtext_cast(ifield(c,"version")),0};
	}
	if(type==modal_prototype_attrs){
		lv*a=ifield(con(),"attributes");
		ms.grid=(grid_val){l_take(l_count(a),a),0,-1,-1};
		ms.name=(field_val){rtext_cast(lmistr("")),0};
		ms.text=(field_val){rtext_cast(lmistr("")),0};
	}
	if(type==modal_pick_card){
		ms.act_card=ln(ifield(ifield(deck,"card"),"index"));
		ms.carda=l_first(ob.sel);
	}
	if(type==modal_action){
		sc.target=ob.sel->lv[0],sc.others=lml(0);
		ms.act_go=1,ms.act_gomode=5,ms.act_trans=0,ms.act_sound=0;
		ms.verb   =lmistr(""); // card name
		ms.message=lmistr(""); // sound name
		ms.grid=(grid_val){l_table(l_keys(dget(deck->b,lmistr("transit")))),0,0,-1};
		pair ps={17,13};
		ms.canvas=free_canvas(deck);dset(ms.canvas->b,lmistr("size"),lmpair(ps));
		ms.carda=image_read(lmcstr("%%IMG0ABEADQAAAAAAAACAAAFAAAIgAAIgAAQQAAfwAAgIAAgIAAgIAAAAAAAAAA=="));
		ms.cardb=image_read(lmcstr("%%IMG0ABEADf//gP//gPA/gPffgPffgPAPgPf3gPf3gPf3gPf3gPAPgP//gP//gA=="));
		// parse action script, if any:
		lv*scr=ifield(sc.target,"script");
		lv*p0=l_parse(lmcstr("on click do\n  play[%q]\nend%m"             ),scr); // sound, no go
		lv*p1=l_parse(lmcstr("on click do\n  play[%q]\n  go[%q %q]\nend%m"),scr); // sound, go + trans
		lv*p2=l_parse(lmcstr("on click do\n  play[%q]\n  go[%q]\nend%m"   ),scr); // sound, just go
		lv*p3=l_parse(lmcstr("on click do\n  go[%q %q]\nend%m"            ),scr); // no sound, go, trans
		lv*p4=l_parse(lmcstr("on click do\n  go[%q]\nend%m"               ),scr); // no sound, just go
		lv*fs=lb(l_last(p0))||lb(l_last(p1))||lb(l_last(p2))?ls(l_first(p0)): NULL;
		lv*fg=lb(l_last(p1))||lb(l_last(p2))?ls(p1->lv[1]): lb(l_last(p3))||lb(l_last(p4))?ls(l_first(p3)): NULL;
		lv*ft=lb(l_last(p1))?ls(p1->lv[2]): lb(l_last(p3))?ls(p3->lv[1]): NULL;
		char*fk[]={"First","Prev","Next","Last","Back"};
		if(fs!=NULL||fg!=NULL||ft!=NULL){
			if(fs!=NULL){ms.act_sound=1,ms.message=fs;}
			if(ft!=NULL){ms.grid.scroll=-99;for(int r=0;r<ms.grid.table->n;r++)if(matchr(ft,ms.grid.table->lv[0]->lv[r]))ms.act_trans=1,ms.grid.row=r;}
			ms.act_go=fg!=NULL;if(fg!=NULL){for(int z=0;z<5;z++)if(!strcmp(fk[z],fg->sv))ms.act_gomode=z;if(ms.act_gomode==5)ms.verb=fg;}
		}
	}
	if(type==modal_card_props){ms.name=(field_val){rtext_cast(ifield(ifield(deck,"card"),"name")),0};}
	if(type==modal_link||type==modal_gridcell||type==modal_query){wid.cursor=(pair){0,RTEXT_END};}
	if(type==modal_alert_lil     ){ms.type=type=modal_alert;}
	if(type==modal_confirm_lil   ){ms.type=type=modal_confirm;}
	if(type==modal_input_lil     ){ms.type=type=modal_input;}
	if(type==modal_confirm_quit  ){ms.type=type=modal_confirm;}
	if(type==modal_confirm_new   ){ms.type=type=modal_confirm;}
	if(type==modal_confirm_script){ms.type=type=modal_confirm;}
	if(type==modal_multiscript   ){ms.type=type=modal_confirm;}
	if(type==modal_import_script ){ms.type=type=modal_open, ms.filter=filter_code, ms.desc="Open any .lil or .txt file.";}
	if(type==modal_export_script ){ms.type=type=modal_save, ms.filter=filter_code, ms.desc="Save script as a .lil file.";}
	if(type==modal_import_sound  ){ms.type=type=modal_open, ms.filter=filter_sound,ms.desc="Open a .wav sound file.";}
	if(type==modal_import_image  ){ms.type=type=modal_open, ms.filter=filter_image,ms.desc="Open an image file.";}
	if(type==modal_export_image  ){ms.type=type=modal_save, ms.filter=filter_gif  ,ms.desc="Save a .gif image file.";}
	if(type==modal_open_deck     ){ms.type=type=modal_open, ms.filter=filter_deck, ms.desc="Open any .html or .deck.";}
	if(type==modal_import_deck   ){ms.type=type=modal_open, ms.filter=filter_deck ,ms.desc="Open any .html or .deck.";}
	if(type==modal_save_deck     ){ms.type=type=modal_save, ms.filter=filter_deck, ms.desc="Save a .deck or .html file.";}
	if(type==modal_save_locked   ){ms.type=type=modal_save, ms.filter=filter_deck, ms.desc="Save locked deck.";}
	if(type==modal_open_lil      ){ms.type=type=modal_open, ms.filter=filter_none, ms.desc="Open any file.";}
	if(type==modal_save_lil      ){ms.type=type=modal_save, ms.filter=filter_none, ms.desc="Save a text file.";}
	if(type==modal_open||type==modal_save){ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};}
	if(type==modal_save||type==modal_input){ms.text=(field_val){rtext_cast(lmistr("")),0};}
}
void bg_paste(lv*b,int fit);void proto_prop(lv*target,char*key,lv*value);void proto_size(lv*target,pair size,rect margin); // forward refs
void import_image(char*path){
	lv*i=readimage(path,0),*m=NULL;if(is_empty(i))return;
	int color=0,c[256]={0};EACH(z,i->b)c[0xFF&(i->b->sv[z])]++;
	int tw=c[0],ow=c[32];c[32]=0,c[47]=0;for(int z=2;z<256;z++)if(c[z]){color=1;break;}
	if(color&&tw){EACH(z,i->b)i->b->sv[z]=i->b->sv[z]!=0;m=i->b;}
	if(color){i=readimage(path,!dr.color);}else if(ow&&!tw){EACH(z,i->b)i->b->sv[z]=i->b->sv[z]!=32;}
	setuimode(mode_draw),bg_paste(i->b,1);if(color&&!dr.color)dr.limbo_dither=1,dither_threshold=0.5;dr.fatbits=0;dr.omask=m;
}
lv* table_decode(lv*text,lv*format){return ms.edit_json?l_table(l_parse(lmistr("%j"),text)): n_readcsv(NULL,format->c?lml2(text,format):l_list(text));}
lv* modal_open_path(void){
	int n=ms.grid.table->n,r=ms.grid.row;
	char t[PATH_MAX]={0};directory_cat(t,ms.path,r<n&&r>=0?ms.grid.table->lv[1]->lv[r]->sv:"");return lmcstr(t);
}
lv* modal_save_path(char*suffix){
	char t[PATH_MAX]={0};directory_cat(t,ms.path,rtext_all(ms.text.table)->sv);
	size_t sn=strlen(suffix)+1;if(!has_suffix(t,suffix)&&strlen(t)<PATH_MAX-sn)snprintf(t+strlen(t),sn,"%s",suffix);return lmcstr(t);
}
void modal_save_replace(int type,char*name,lv*path){
	modal_enter(modal_confirm);ms.subtype=type;
	char m[2*PATH_MAX];snprintf(m,sizeof(m),"The %s \"%s\" already exists. Replace it?",name,path->sv);
	ms.message=lmcstr(m);ms.verb=lmistr("Replace");
}
void ob_edit_prop(char*key,lv*value);void ob_create(lv*props);void validate_modules(void); // forward refs
void modal_exit(int value){
	wid=ms.old_wid;
	if(wid.gv==&ms.old_wid.gv_slot)wid.gv=&wid.gv_slot;
	if(wid.fv==&ms.old_wid.fv_slot)wid.fv=&wid.fv_slot;
	if(ms.subtype==modal_import_script&&value){
		field_exit(),sc.f=(field_val){rtext_cast(n_read(NULL,l_list(modal_open_path()))),0};
	}
	if(ms.subtype==modal_import_sound){
		if(value){sound_edit(n_readwav(NULL,l_list(modal_open_path())));au.sel=(pair){0,0},au.head=0;}
		modal_enter(modal_recording);return;
	}
	if(ms.subtype==modal_import_image&&value){import_image(modal_open_path()->sv);}
	if(ms.subtype==modal_open_deck&&value){
		lv*path=modal_open_path();
		load_deck(deck_get(n_read(NULL,l_list(path))));
		set_path(path->sv);
	}
	if(ms.subtype==modal_import_deck){
		lv*path=modal_open_path();modal_enter(modal_resources);
		if(value){ms.message=deck_get(n_read(NULL,l_list(path))),ms.grid=(grid_val){res_enumerate(ms.message),0,-1,-1};}
		return;
	}
	if(ms.type==modal_gridcell&&value){
		grid_edit_cell(ms.cell,ls(rtext_all(ms.text.table)));
	}
	if(ms.type==modal_card_props){
		lv*card=ifield(deck,"card");
		iwrite(card,lmistr("name"),rtext_all(ms.name.table));mark_dirty();
	}
	if(ms.type==modal_slider_props){
		lv*f=ob.sel->lv[0];
		iwrite(f,lmistr("interval"),lml2(rtext_all(ms.form0.table),rtext_all(ms.form1.table)));
		iwrite(f,lmistr("step"),rtext_all(ms.form2.table));mark_dirty();
	}
	if(ms.type==modal_grid_props){
		lv*g=ob.sel->lv[0];
		lv*t=table_decode(rtext_all(ms.form0.table),rtext_all(ms.text.table));
		if(!matchr(t,ifield(g,"value")))ob_edit_prop("value",t);
	}
	if(ms.type==modal_contraption_props){
		lv*w=ob.sel->lv[0],*a=ifield(ifield(w,"def"),"attributes");;
		for(int z=0;z<a->n;z++){
			int t=attrs[z].type;lv*v=lmbool(attrs[z].bval);
			if(t==attr_number)v=lmn(ln(rtext_all(attrs[z].value.table)));
			if(t==attr_string)v=rtext_all(attrs[z].value.table);
			if(t==attr_code  )v=rtext_all(attrs[z].value.table);
			if(t==attr_rich  )v=attrs[z].value.table;
			iwrite(w,a->lv[0]->lv[z],v);
		}attrs_count=0;
	}
	if(ms.type==modal_action&&value){
		str r=str_new();str_addz(&r,"on click do\n");
		if(ms.act_sound){str_addz(&r,"  play[");show(&r,ms.message,0);str_addz(&r,"]\n");}
		if(ms.act_go){
			str_addz(&r,"  go[");
			if     (ms.act_gomode==0)str_addz(&r,"\"First\"");
			else if(ms.act_gomode==1)str_addz(&r,"\"Prev\"");
			else if(ms.act_gomode==2)str_addz(&r,"\"Next\"");
			else if(ms.act_gomode==3)str_addz(&r,"\"Last\"");
			else if(ms.act_gomode==4)str_addz(&r,"\"Back\"");
			else{show(&r,ms.verb,0);}
			if(ms.act_trans){str_addc(&r,' ');show(&r,ms.grid.table->lv[0]->lv[ms.grid.row],0);}
			str_addz(&r,"]\n");
		}str_addz(&r,"end");script_save(lmstr(r));
	}
	if(ms.type==modal_recording){
		lv*name=rtext_all(ms.name.table);rename_sound(deck,au.target,name);mark_dirty();
		au.mode=record_stopped;modal_enter(modal_sounds);ms.grid.row=dgeti(ifield(deck,"sounds"),name);return;
	}
	if(ms.type==modal_confirm&&ms.subtype==modal_export_script&&!value){modal_enter(ms.subtype);return;}
	if(ms.type==modal_confirm&&ms.subtype==modal_save_deck    &&!value){modal_enter(ms.subtype);return;}
	if(ms.type==modal_confirm&&ms.subtype==modal_save_locked  &&!value){modal_enter(ms.subtype);return;}
	if(ms.type==modal_confirm&&ms.subtype==modal_save_lil     &&!value){modal_enter(ms.subtype);return;}
	if(ms.subtype==modal_export_script&&value){
		lv*path=modal_save_path(".lil");
		if(directory_exists(path)&&ms.type!=modal_confirm){modal_save_replace(modal_export_script,"Lil script",path);return;}
		n_write(NULL,lml2(path,rtext_all(sc.f.table)));
	}
	if(ms.subtype==modal_export_image&&value){
		lv*path=modal_save_path(".gif");
		if(directory_exists(path)&&ms.type!=modal_confirm){modal_save_replace(modal_export_image,"GIF file",path);return;}
		if(bg_has_sel()){rect s=dr.sel_here;bg_end_selection();dr.sel_here=s;}
		lv*i=draw_con(con(),1);pair off={0,0};char*pal=patterns_pal(ifield(deck,"patterns"));
		if(bg_has_sel()||bg_has_lasso()){rect r=box_intersect(dr.sel_here,con_dim());i=image_make(buffer_copy(i->b,r));off=(pair){r.x,r.y};}
		pair s=image_size(i);int f=1;for(int z=0;z<4&&dr.show_anim;z++){int c=anim_count(pal,z);if(c){f=lcm(f,c);}}
		int a=0;int bg=dr.trans?0:32;lv*r=lml(f);EACH(z,r){
			lv*frame=image_make(buffer_copy(i->b,rect_pair((pair){0,0},s)));r->lv[z]=frame;
			for(int y=0;y<s.y;y++)for(int x=0;x<s.x;x++){
				int v=frame->b->sv[x+(s.x*y)];if(v>=28&&v<=31)a=1;
				int c=anim_pattern(pal,v,z*4),p=draw_pattern(pal,c,x+off.x,y+off.y);
				frame->b->sv[x+(s.x*y)]=c>=32?c: p?1:bg;
			}
			if(bg_has_lasso())r->lv[z]->b=buffer_mask(r->lv[z]->b,dr.mask);
		}if(!a)r->c=1;lv*rd=lmd();dset(rd,lmistr("frames"),r),dset(rd,lmistr("delays"),l_take(lmn(r->c),lmn(10))),n_writegif(NULL,lml2(path,rd));
	}
	if((ms.subtype==modal_save_deck||ms.subtype==modal_save_locked)&&value){
		lv*path=modal_save_path("");
		if(directory_exists(path)&&ms.type!=modal_confirm){modal_save_replace(ms.subtype,"deck",path);return;}
		if(ms.subtype==modal_save_locked)iwrite(deck,lmistr("locked"),ONE);
		save_deck(path);
		if(ms.subtype==modal_save_locked)iwrite(deck,lmistr("locked"),ZERO);
	}
	if(ms.subtype==modal_open_lil&&value){
		lv*hint=ms.verb,*name=modal_open_path();lv*type=arg();ret(array_is(type)?readbin(name):n_readfile(NULL,lml2(name,hint)));
	}
	if(ms.subtype==modal_save_lil){
		if(ms.type==modal_save&&!value){arg();ret(ZERO);ms.type=modal_none;return;}
		lv*path=modal_save_path(ms.filter==filter_sound?".wav":ms.filter==filter_gif?".gif": ms.path_suffix);
		if(directory_exists(path)&&ms.type!=modal_confirm){modal_save_replace(modal_save_lil,"file",path);return;}
		lv*value=arg();ret(!value?ZERO:
			(image_is(value)||lil(value)||lid(value))?n_writegif(NULL,lml2(path,value)):
			sound_is(value)?n_writewav(NULL,lml2(path,value)):
			deck_is(value)?n_writedeck(NULL,lml2(path,value)):
			array_is(value)?writebin(path,value):
			n_write(NULL,lml2(path,ls(value)))
		);
	}
	if(ms.subtype==modal_confirm_quit&&value)should_exit=1;
	if(ms.subtype==modal_confirm_new&&value)load_deck(deck_get(lmistr(""))),set_path("");
	if(ms.subtype==modal_confirm_script&&value)finish_script();
	if(ms.subtype==modal_multiscript&&value)setscript(l_drop(ZERO,ob.sel));
	if(ms.subtype==modal_alert_lil  ){arg();ret(ONE);}
	if(ms.subtype==modal_confirm_lil){arg();ret(lmn(value));}
	if(ms.subtype==modal_input_lil  ){arg();ret(rtext_all(ms.text.table));}
	if(ms.subtype==modal_choose_lil ){arg();ret(ms.verb->lv[ms.grid.row]);}
	ms.type=modal_none;
	if(ms.from_listener)modal_enter(modal_listen);
	if(enable_touch&&ms.from_keycaps)kc.on=1;
	if(ms.type==modal_none&&uimode==mode_interact)msg.next_view=1;
}
void object_properties(lv*x);void object_select(lv*x); // forward refs
void ob_move_up(void);void ob_move_dn(void);void ob_order(void);

void modals(void){
	ms.in_modal=1;
	char*pal=patterns_pal(ifield(deck,"patterns"));
	if(ms.type==modal_about){
		rect b=draw_modalbox((pair){150,90});char v[256];
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit)modal_exit(0);
		snprintf(v,sizeof(v),"Decker v%s",VERSION);draw_text(b,v,FONT_MENU,1);b.y+=15;
		draw_text(b,"by John Earnest",FONT_BODY,1);b.y+=12;
		draw_text(b,"beyondloom.com/decker",FONT_BODY,1);b.y+=20;
		snprintf(v,sizeof(v),"built on %s, %s",__DATE__,__TIME__);draw_text(b,v,FONT_BODY,1);
	}
	else if(ms.type==modal_listen){
		if(!kc.on){listener((rect){0,0,frame.size.x,frame.size.y});if(ev.eval)listener_eval();}
		if(ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_cards){
		rect b=draw_modalbox((pair){210,frame.size.y-46});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Cards",FONT_MENU,1);
		rect gsize={b.x,b.y+15,b.w,b.h-20-20};
		lv*cards=ifield(deck,"cards"),*curr=ifield(deck,"card");
		int slot=30, m=0, props=0, gutter=-1, ch=slot*cards->c; draw_box(gsize,0,1);
		rect bb=scrollbar(gsize,MAX(0,ch-(gsize.h-2)),10,gsize.h,&ms.grid.scroll,ch>=gsize.h,0); bb.y++;
		rect oc=frame.clip;frame.clip=bb;char temp[4096];EACH(z,cards){
			rect c={bb.x,bb.y+(z*slot)-ms.grid.scroll,bb.w,slot}; lv*card=cards->lv[z];
			if(c.y>bb.y+bb.h||c.y+c.h<bb.y)continue; rect cb=box_intersect(c,bb); // coarse clip
			rect p={c.x+2,c.y+1,40,28}, t={p.x+p.w+5,p.y,bb.w-(2+p.w+5+5),font_h(FONT_MENU)}, s={t.x,t.y+t.h+2,t.w,font_h(FONT_BODY)};
			snprintf(temp,sizeof(temp),"%d widget%s",ifield(card,"widgets")->c,ifield(card,"widgets")->c==1?"":"s");
			if(ev.md&&dover(cb)){m=1,n_go(deck,l_list(card));curr=card;ms.grid.row=z;}if(ev.dclick&&over(cb))props=1;
			int col=ev.drag&&ms.grid.row==z?13:1;
			draw_text_fit(t,ifield(card,"name")->sv,FONT_MENU,col),draw_text_fit(s,temp,FONT_BODY,col);draw_box(p,0,col);
			if(card==curr&&col==1)draw_invert(pal,c);draw_thumbnail(card,p);
			if((ev.drag||ev.mu)&&ms.grid.row!=-1){
				{rect g={c.x,c.y-3    ,c.w,7};if(over(g)){draw_hline(c.x,c.x+c.w,c.y      ,13);gutter=z  ;}}
				{rect g={c.x,c.y-3+c.h,c.w,7};if(over(g)){draw_hline(c.x,c.x+c.w,c.y+c.h-1,13);gutter=z+1;}}
			}
		}frame.clip=oc;
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit||ev.action)modal_exit(0);
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"New",1)){
			lv*c=n_deck_add(deck,l_list(lmistr("card")));
			int n=ln(ifield(curr,"index"));iwrite(c,lmistr("index"),lmn(n+1));
			m=1,n_go(deck,l_list(c));
		}c.x+=65;
		if(ev.mu){
			if(ms.grid.row!=-1&&gutter!=-1){
				lv*s=cards->lv[ms.grid.row];int oi=ln(ifield(s,"index"));
				iwrite(s,lmistr("index"),lmn(gutter>oi?gutter-1:gutter));m=1,n_go(deck,l_list(s));
			}ms.grid.row=-1;
		}
		else if(ev.drag&&ms.grid.row!=-1){rect r={ev.pos.x-5,ev.pos.y-5,10,10};draw_rect(r,0);draw_box(r,0,1);uicursor=cursor_drag;}
		else if(ev.dir==dir_up  &&ev.shift){iwrite(curr,lmistr("index"),lmn(ln(ifield(curr,"index"))-1));m=1,n_go(deck,l_list(curr));}
		else if(ev.dir==dir_down&&ev.shift){iwrite(curr,lmistr("index"),lmn(ln(ifield(curr,"index"))+1));m=1,n_go(deck,l_list(curr));}
		else if(ev.dir==dir_left ||ev.dir==dir_up  ){m=1,n_go(deck,l_list(lmistr("Prev")));}
		else if(ev.dir==dir_right||ev.dir==dir_down){m=1,n_go(deck,l_list(lmistr("Next")));}
		if(m){
			curr=ifield(deck,"card");int y=(ln(ifield(curr,"index"))*slot)-ms.grid.scroll;
			if(y<0){ms.grid.scroll+=y;}if(y+slot>=bb.h){ms.grid.scroll+=(y+slot)-bb.h;}
		}if(props)modal_enter(modal_card_props);
	}
	else if(ms.type==modal_orderwids){
		rect b=draw_modalbox((pair){210,frame.size.y-46});lv*def=con(),*wids=ifield(def,"widgets");
		draw_textc((rect){b.x,b.y-5,b.w,20},"Widget Order",FONT_MENU,1);
		rect gsize={b.x,b.y+15,b.w,b.h-20-20};
		int slot=16, m=ms.grid.col==-99?-1:0, props=0, gutter=-1, ch=slot*wids->c;  draw_box(gsize,0,1);
		rect bb=scrollbar(gsize,MAX(0,ch-(gsize.h-2)),10,gsize.h,&ms.grid.scroll,ch>=gsize.h,0); bb.y++;
		rect oc=frame.clip;frame.clip=bb;EACH(z,wids){
			rect c={bb.x,bb.y+(z*slot)-ms.grid.scroll,bb.w,slot}; lv*wid=wids->lv[z];
			if(c.y>bb.y+bb.h||c.y+c.h<bb.y)continue; rect cb=box_intersect(c,bb); // coarse clip
			if(ev.dclick&&over(cb))props=1;
			if(ev.md&&dover(cb)){
				if(ev.shift){if(lb(l_in(wid,ob.sel))){ob.sel=l_drop(wid,ob.sel);props=0;}else{ll_add(ob.sel,wid);}}
				else{object_select(wid);ms.grid.row=z;}
			}
			int col=ev.drag&&ms.grid.row==z?13:1;
			draw_text_fit((rect){c.x+3,c.y+2,bb.w-6,font_h(FONT_BODY)},ifield(wid,"name")->sv,FONT_BODY,col);
			if(ifield(wid,"script")->c){
				rect i={c.x+c.w-13,c.y+2,12,12};
				draw_rect((rect){i.x+2,i.y+1,i.w-2,i.h-2},0),draw_icon((pair){i.x,i.y},ICONS[icon_lil],1);
			}
			if(lb(l_in(wid,ob.sel)))draw_invert(pal,c);
			if((ev.drag||ev.mu)&&ms.grid.row!=-1){
				{rect g={c.x,c.y-3    ,c.w,7};if(over(g)){draw_hline(c.x,c.x+c.w,c.y      ,13);gutter=z  ;}}
				{rect g={c.x,c.y-3+c.h,c.w,7};if(over(g)){draw_hline(c.x,c.x+c.w,c.y+c.h-1,13);gutter=z+1;}}
			}
		}frame.clip=oc;
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit)modal_exit(0);
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,80,20},"Properties...",ob.sel->c==1)||props)if(ob.sel->c)object_properties(l_first(ob.sel));
		if(ev.mu){
			if(ms.grid.row!=-1&&gutter!=-1){
				lv*s=wids->lv[ms.grid.row];int oi=ln(ifield(s,"index"));
				iwrite(s,lmistr("index"),lmn(gutter>oi?gutter-1:gutter));m=1;object_select(s);
			}ms.grid.row=-1;
		}
		else if(ev.drag&&ms.grid.row!=-1){rect r={ev.pos.x-5,ev.pos.y-5,10,10};draw_rect(r,0);draw_box(r,0,1);uicursor=cursor_drag;}
		else if(ev.shift &&ev.dir==dir_up  ){ob_move_dn();m=-1;}
		else if(ev.shift &&ev.dir==dir_down){ob_move_up();m= 1;}
		else if(ob.sel->c&&ev.dir==dir_up  ){ob_order();object_select(wids->lv[mod(ln(ifield(l_first(ob.sel),"index"))-1,wids->c)]);m=-1;}
		else if(ob.sel->c&&ev.dir==dir_down){ob_order();object_select(wids->lv[mod(ln(ifield(l_last (ob.sel),"index"))+1,wids->c)]);m= 1;}
		if(m!=0&&ob.sel->c){
			ms.grid.col=-1;ob_order();lv*target=m==-1?l_first(ob.sel):l_last(ob.sel);
			int y=(ln(ifield(target,"index"))*slot)-ms.grid.scroll;
			if(y<0){ms.grid.scroll+=y;}if(y+slot>=bb.h){ms.grid.scroll+=(y+slot)-bb.h;}
		}
	}
	else if(ms.type==modal_sounds){
		rect b=draw_modalbox((pair){250,frame.size.y-16-30});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Sounds",FONT_MENU,1);
		rect gsize={b.x,b.y+15,b.w,b.h-16-(2*25)};
		lv*s=ms.grid.row>=0?dget(ifield(deck,"sounds"),ms.grid.table->lv[1]->lv[ms.grid.row]):NULL;
		if(ui_table(gsize,2,16,130,"Isss",&ms.grid))n_play(deck,l_list(s));
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit){
			if(ms.from_action){
				if(ms.grid.row>=0)ms.message=ifield(deck,"sounds")->kv[ms.grid.row];
				ms.grid=(grid_val){l_table(l_keys(dget(deck->b,lmistr("transit")))),0,ms.act_transno,-1},ms.type=modal_action;ms.from_action=0;
			}else{modal_exit(1);}
		}
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y   ,60,20},"Edit...",s!=NULL)){au.target=s;modal_enter(modal_recording);}
		if(ui_button((rect){c.x,c.y-25,60,20},"New...",1)){au.target=n_deck_add(deck,l_list(lmistr("sound")));mark_dirty();modal_enter(modal_recording);}
		c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Delete",s!=NULL)){n_deck_remove(deck,l_list(s));mark_dirty();ms.grid=(grid_val){sounds_enumerate(),0,-1,-1};}
	}
	else if(ms.type==modal_contraptions){
		rect b=draw_modalbox((pair){250,230});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Contraption Prototypes",FONT_MENU,1);
		rect gsize={b.x,b.y+15,b.w,b.h-20-50-25-25};
		int choose=ui_table(gsize,1,16,0,"Is",&ms.grid);
		rect psize={b.x,gsize.y+gsize.h+5,b.w,50};draw_box(psize,0,1);psize=inset(psize,2);
		if(ms.grid.row>=0){
			char*desc=ifield(ifield(deck,"contraptions")->lv[ms.grid.row],"description")->sv;
			layout_plaintext(desc,FONT_BODY,align_left,(pair){psize.w,psize.h});draw_text_wrap(psize,1);
		}
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit){modal_exit(0);}
		if(ui_button((rect){b.x,b.y+b.h-45,60,20},"New...",1)){modal_exit(1);con_set(n_deck_add(deck,l_list(lmistr("contraption"))));mark_dirty();}
		if(ui_button((rect){b.x,b.y+b.h-20,60,20},"Edit...",ms.grid.row>=0)||choose){modal_exit(2);con_set(ifield(deck,"contraptions")->lv[ms.grid.row]);}
		if(ui_button((rect){b.x+65,b.y+b.h-45,60,20},"Clone",ms.grid.row>=0)){
			lv*s=ifield(deck,"contraptions")->lv[ms.grid.row];
			n_deck_add(deck,lml2(s,l_format(lmistr("%s_clone"),ifield(s,"name"))));
			ms.grid=(grid_val){contraptions_enumerate(),0,-1,-1};
		}
		if(ui_button((rect){b.x+65,b.y+b.h-20,60,20},"Delete",ms.grid.row>=0)){
			n_deck_remove(deck,l_list(ifield(deck,"contraptions")->lv[ms.grid.row]));
			ms.grid=(grid_val){contraptions_enumerate(),0,-1,-1};
		}
	}
	else if(ms.type==modal_recording){
		rect b=draw_modalbox((pair){frame.size.x-50,130}); int samples=MAX(1,ln(ifield(au.target,"size")));
		draw_textc((rect){b.x,b.y-5,b.w,20},"Audio Editor",FONT_MENU,1);
		au.head=CLAMP(0,au.head,samples), au.sel.x=CLAMP(0,au.sel.x,samples), au.sel.y=CLAMP(0,au.sel.y,samples);
		rect gsize={b.x,b.y+15,b.w,64};
		rect lsize={b.x,gsize.y+gsize.h+2,b.w/2,font_h(FONT_BODY)};
		#define sndpos(x) ((x)*((gsize.w*1.0)/samples))
		#define possnd(x) MAX(0,MIN(samples-1,((((x)*1.0)/gsize.w)*samples)))
		if((ev.mu||ev.drag)&&dover(gsize)){
			int x=possnd(ev.dpos.x-gsize.x), y=possnd(ev.pos.x-gsize.x); field_exit();
			if(ev.mu)au.head=y;if(ev.drag)au.sel=(pair){MIN(x,y),MAX(x,y)},au.head=au.sel.x;
		}
		pair sel=au.sel;int sc=sel.y-sel.x;
		rect oc=frame.clip;frame.clip=gsize;for(int z=0;z<gsize.w;z++){
			int v=1, base=possnd(z); for(int d=-1;d<=1;d++){if(base+d<0||base+d>=samples)continue;int s=au.target->b->sv[base+d];if(abs(s)>abs(v))v=s;}
			float vp=(v/127.0)*64;draw_vline(gsize.x+z,gsize.y+32,gsize.y+32+vp,1),draw_vline(gsize.x+z,gsize.y+32-vp,gsize.y+32,1);
		}frame.clip=oc;
		if(sc){draw_invert(pal,(rect){gsize.x+sndpos(sel.x),gsize.y,sndpos(sel.y)-sndpos(sel.x),gsize.h});}
		draw_invert(pal,(rect){gsize.x+sndpos(au.head)-1,gsize.y,3,gsize.h}),draw_box(gsize,0,1);
		char t[4096]={0};
		if(sc==0){snprintf(t,sizeof(t),"%.2fkb, %.2fs"         ,samples/1000.0*1.33,ln(ifield(au.target,"duration")));}
		else     {snprintf(t,sizeof(t),"%.2fkb, %.2fs selected",sc     /1000.0*1.33,sc/(1.0*SFX_RATE)               );}
		draw_text_fit(lsize,t,FONT_BODY,1);
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit)modal_exit(0);
		pair c={b.x,b.y+b.h-20};
		if(ui_toggle((rect){c.x,c.y,60,20},"Play",au.mode==record_playing,!nosound)){
			if(au.mode==record_recording)sound_finish();
			au.head=sc?au.sel.x:0; au.mode=(au.mode!=record_playing)?record_playing:record_stopped;
		};c.x+=65;
		if(ui_toggle((rect){c.x,c.y,60,20},"Record",au.mode==record_recording,au.input!=0)){
			if(au.mode==record_recording)sound_finish();
			au.mode=(au.mode!=record_recording)?record_recording:record_stopped;
			if(au.mode==record_recording){sound_edit(sound_slice((pair){0,au.target->b->c}));if(sc)au.head=au.sel.x;record_pause(au.input);}
		};c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Crop",sc&&au.mode==record_stopped)){sound_edit(sound_slice(au.sel));au.sel=(pair){0,0},au.head=0;}
		draw_text((rect){b.x+(b.w/2),gsize.y+gsize.h+9,37,20},"Name",FONT_MENU,1);
		ui_field((rect){b.x+(b.w/2)+37,gsize.y+gsize.h+5,(b.w/2)-37,20},&ms.name);
	}
	else if(ms.type==modal_fonts){
		rect b=draw_modalbox((pair){170,170});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Fonts",FONT_MENU,1);
		rect gsize={b.x,b.y+15,b.w,b.h-20-50-25};
		if(ms.grid.scroll==-99){ms.grid.scroll=grid_scrollto(ms.grid.table,(grid){gsize,FONT_BODY,{0},"",0,1,0,0,show_solid,1},-1,ms.grid.row);}
		int choose=ui_table(gsize,1,16,0,"Is",&ms.grid);
		rect psize={b.x,gsize.y+gsize.h+5,b.w,50};draw_box(psize,0,1);psize=inset(psize,2);
		if(ms.grid.row>=0){layout_plaintext(pangram,ifield(deck,"fonts")->lv[ms.grid.row],align_left,(pair){psize.w,psize.h});draw_text_wrap(psize,1);}
		pair c={b.x+b.w-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"OK",ms.grid.row>=0)||choose){
			lv*nf=ms.grid.table->lv[1]->lv[ms.grid.row];int nested=ms_index>0;modal_pop(1);
			if(uimode==mode_object&&!nested){ob_edit_prop("font",nf);}
			else if(wid.fv&&wid.cursor.x!=wid.cursor.y){pair c=wid.cursor;field_stylespan(nf,lmistr(""));wid.cursor=c;mark_dirty();}
			else if(wid.ft){iwrite(wid.ft,lmistr("font"),nf),wid.f=unpack_field(ms.old_wid.ft,&ms.old_wid.fv_slot),mark_dirty();}
		};c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_pop(0);
	}
	else if(ms.type==modal_resources){
		rect b=draw_modalbox((pair){380,190});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Font/Deck Accessory Mover",FONT_MENU,1);
		rect lgrid={b.x            ,b.y+15 ,120    ,b.h-(15+15+5+20)};
		rect rgrid={b.x+b.w-lgrid.w,lgrid.y,lgrid.w,lgrid.h         };
		if(ui_button((rect){rgrid.x+(rgrid.w-80)/2,b.y+b.h-20,80,20},"OK",1)||ev.exit)modal_exit(0);
		ui_table(lgrid,2,16,lgrid.w-38,"Is",&ms.grid );if(ms.grid .row>-1)ms.grid2.row=-1;
		ui_table(rgrid,2,16,rgrid.w-38,"Is",&ms.grid2);if(ms.grid2.row>-1)ms.grid .row=-1;
		draw_vline(lgrid.x+lgrid.w,lgrid.y+lgrid.h+5,b.y+b.h,18);
		draw_vline(rgrid.x        ,rgrid.y+rgrid.h+5,b.y+b.h,18);
		draw_textc((rect){lgrid.x,lgrid.y+lgrid.h+3,lgrid.w,15},ms.message?(ifield(ms.message,"name")->sv):"(Choose a Deck)",FONT_BODY,1);
		draw_textc((rect){rgrid.x,rgrid.y+rgrid.h+3,rgrid.w,15},ifield(deck,"name")->sv,FONT_BODY,1);
		rect cb={lgrid.x+lgrid.w+5,lgrid.y+5,b.w-(lgrid.w+5+5+rgrid.w),20};
		#define rvalue(g,k) dget(ms.g.table,lmistr(k))->lv[ms.g.row]
		lv*sel=(ms.grid.table&&ms.grid.row>-1)?rvalue(grid,"value"): ms.grid2.row>-1?rvalue(grid2,"value"): NULL;
		char*copy_message=">> Copy >>";int can_copy=1;
		if(ms.grid.row>-1&&sel&&(module_is(sel)||prototype_is(sel))){
			lv*name=ifield(sel,"name");
			float sver=ln(ifield(sel,"version")),dver=sver;can_copy=0;
			if(module_is   (sel)){lv*v=dget(ifield(deck,"modules"     ),name);if(v){dver=ln(ifield(v,"version"));}else{can_copy=1;}}
			if(prototype_is(sel)){lv*v=dget(ifield(deck,"contraptions"),name);if(v){dver=ln(ifield(v,"version"));}else{can_copy=1;}}
			if(sver>dver)can_copy=1,copy_message=">> Upgrade >>";
			if(sver<dver)can_copy=1,copy_message=">> Downgrade >>";
		}
		if(ui_button(cb,copy_message,can_copy&&ms.grid.row>-1)){
			if(patterns_is(sel)){lv*dst=ifield(deck,"patterns");for(int z=2;z<=47;z++)iindex(dst,z,iindex(sel,z,NULL));}
			else if(module_is(sel)||prototype_is(sel)){n_deck_add(deck,l_list(sel));}
			else{n_deck_add(deck,lml2(sel,rvalue(grid,"name")));}
			ms.grid2=(grid_val){res_enumerate(deck),0,-1,-1},mark_dirty();
			if(module_is(sel))validate_modules();
		}cb.y+=25;
		if(ui_button(cb,"Remove",ms.grid2.row>-1)){
			n_deck_remove(deck,l_list(rvalue(grid2,"value")));ms.grid2=(grid_val){res_enumerate(deck),0,-1,-1},mark_dirty();sel=NULL;
		}cb.y+=25;
		rect pre={cb.x,cb.y,cb.w,b.h-(cb.y-b.y)};
		if(sel&&font_is(sel)){
			draw_textc((rect){pre.x,pre.y+pre.h-18,pre.w,18},l_format(lmistr("%i glyphs"),l_count(ifield(sel,"glyphs")))->sv,FONT_BODY,1);pre.h-=20;
			layout_plaintext(pangram,sel,align_center,(pair){pre.w,pre.h}),draw_text_wrap(pre,1);
		}
		if(sel&&(module_is(sel)||prototype_is(sel))){
			draw_textc((rect){pre.x,pre.y+pre.h-18,pre.w,18},l_format(lmistr("version %f"),ifield(sel,"version"))->sv,FONT_BODY,1);pre.h-=20;
			layout_plaintext(ifield(sel,"description")->sv,FONT_BODY,align_center,(pair){pre.w,pre.h}),draw_text_wrap(pre,1);
		}
		if(sel&&sound_is(sel)){if(ui_button(cb,"Play",1))n_play(deck,l_list(sel));}
		if(sel&&patterns_is(sel)){
			rect c=frame.clip;frame.clip=pre;char*pal=patterns_pal(sel);for(int z=0;z<32;z++)for(int y=0;y<16;y++)for(int x=0;x<16;x++){
				int bx=3+x+pre.x+16*(z%(pre.w/16)), by=y+pre.y+16*(z/(pre.w/16));if(inclip(bx,by))PIX(bx,by)=32+draw_color(pal,z,frame_count,x,y);
			}frame.clip=c;
		}
		if(ui_button((rect){lgrid.x+(lgrid.w-80)/2,b.y+b.h-20,80,20},"Choose...",1))modal_enter(modal_import_deck);
	}
	else if(ms.type==modal_query){
		rect b=draw_modalbox((pair){frame.size.x-30,frame.size.y-16-30}); char desc[256];
		if(ms.grid.table){lv*t=ms.grid.table;snprintf(desc,sizeof(desc),"%d column%s, %d row%s.",t->c,t->c==1?"":"s",t->n,t->n==1?"":"s");}
		else{snprintf(desc,sizeof(desc),"Executing Query.");}
		parse(rtext_all(ms.text.table)->sv);int compiles=!perr();
		pair dsize=font_textsize(FONT_BODY,desc);draw_text(b,desc,FONT_BODY,1);
		pair msize=font_textsize(FONT_BODY,compiles?" ":par.error);
		rect gsize={b.x,b.y+dsize.y,b.w,(b.h-(2*5)-dsize.y-20)/2};
		rect esize={b.x,gsize.y+gsize.h+5,b.w,gsize.h-msize.y};
		ui_codeedit(esize,1,&ms.text);
		if(!compiles)draw_text_fit((rect){b.x,esize.y+esize.h,b.w,msize.y},par.error,FONT_BODY,1);
		pair c={b.x+b.w-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Run",compiles)||ev.eval){
			lv*str=rtext_all(ms.text.table),*prog=parse(str->sv);if(!perr()){
				blk_lit(prog,lmnat(n_post_query,NULL)),blk_op(prog,SWAP),blk_op(prog,CALL);
				fire_hunk_async(ms.old_wid.gt,prog);ms.grid=(grid_val){NULL,0,-1,-1};
			}
		};c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Apply",ms.grid.table!=NULL&&ms.grid.table!=ms.old_wid.gv->table&&!ms.old_wid.g.locked)){
			lv*t=ms.grid.table;modal_exit(0),grid_edit(t);
			listen_show(align_right,1,lmcstr("applied table query:")),listen_show(align_left,1,rtext_all(ms.text.table));
		}c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Close",1)||ev.exit)modal_exit(0);
		if(ms.grid.table){widget_grid(NULL,(grid){gsize,FONT_MONO,{0},"",1,1,1,0,show_solid,1},&ms.grid);}
		else{draw_box(gsize,0,1);draw_textc(gsize,"Working...",FONT_BODY,1);}
	}
	else if(ms.type==modal_url){
		rect b=draw_modalbox((pair){200,90});
		draw_textc((rect){b.x,b.y,b.w,20},"Do you wish to open this URL?",FONT_BODY,1);
		ui_textedit((rect){b.x,b.y+20+5,b.w,40},1,&ms.text);
		pair c={b.x+b.w-(b.w-(2*60+5))/2-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Open",1)||ev.action){open_url(rtext_all(ms.text.table)->sv);modal_exit(0);};c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_link){
		rect b=draw_modalbox((pair){230,70});
		draw_textc((rect){b.x,b.y,b.w,20},kc.heading="Enter a link string for\nthe selected text span:",FONT_BODY,1);
		ui_field((rect){b.x,b.y+20+5,b.w,20},&ms.text);
		pair c={b.x+b.w-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"OK",1))modal_pop(1);c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_pop(0);
		if(ui_button((rect){b.x,c.y,60,20},"Card...",1))modal_push(modal_pick_card);
	}
	else if(ms.type==modal_gridcell){
		rect c=ms.pending_grid_cell;
		draw_rect(inset(c,-2),32),draw_box(inset(c,-2),0,1);
		ui_field(c,&ms.text);
		if(ev.click&&!over(c))modal_exit(1);
		if(ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_open){
		rect b=draw_modalbox((pair){300,frame.size.y-16-30});
		rect gsize={b.x,b.y+25,b.w,b.h-(25+25)};
		rect dsize={b.x+16,b.y,b.w-(16+5+60+5+60),20};
		draw_text_fit((rect){b.x,b.y+b.h-20,b.w-60+5+60,20},ms.desc,FONT_BODY,1);
		draw_icon((pair){b.x,b.y+3},ICONS[icon_dir],1);
		draw_box(dsize,0,13);draw_text_fit(inset(dsize,3),directory_last(ms.path),FONT_BODY,1);
		int choose=ui_table(gsize,1,16,0,"Is",&ms.grid);
		pair c={b.x+b.w-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Open",ms.grid.table->n)||choose){
			if(0==ln(ms.grid.table->lv[0]->lv[ms.grid.row])){
				directory_child(ms.path,ms.grid.table->lv[1]->lv[ms.grid.row]->sv);
				ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
			}else{modal_exit(1);}
		};c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_exit(0);
		if(ui_button((rect){b.x+b.w-125,b.y,60,20},"Home",!directory_is_home(ms.path))){
			directory_home(ms.path);ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
		}
		if(ui_button((rect){b.x+b.w-60,b.y,60,20},"Parent",directory_has_parent(ms.path))){
			directory_parent(ms.path);ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
		}
	}
	else if(ms.type==modal_save){
		rect b=draw_modalbox((pair){300,frame.size.y-16-30});
		rect gsize={b.x,b.y+25,b.w,b.h-(25+25+25)};
		rect dsize={b.x+16,b.y,b.w-(16+5+60+5+60),20};
		rect fsize={b.x+45,gsize.y+gsize.h+5,b.w-45,20};
		int ename=ms.text.table->n==0||dget(ms.text.table,lmistr("text"))->lv[0]->c==0;
		int vname=1;if(!ename){lv*t=dget(ms.text.table,lmistr("text"))->lv[0];EACH(z,t)if(strchr("\n\\/:*?\"<>|#%$",t->sv[z])){vname=0;break;}}
		draw_text_fit((rect){b.x,b.y+b.h-20,b.w-60+5+60,20},vname||ename?ms.desc:"Filenames may not include:\n\\/:*?\"<>|#%$",FONT_BODY,1);
		draw_icon((pair){b.x,b.y+3},ICONS[icon_dir],1);
		draw_box(dsize,0,13);draw_text_fit(inset(dsize,3),directory_last(ms.path),FONT_BODY,1);
		draw_text_fit((rect){b.x,fsize.y,45,20},"Filename",FONT_BODY,1);
		ui_field(fsize,&ms.text);
		if(ui_table(gsize,1,16,0,"Is",&ms.grid)){
			lv*name=ms.grid.table->lv[1]->lv[ms.grid.row];
			if(0==ln(ms.grid.table->lv[0]->lv[ms.grid.row])){
				directory_child(ms.path,name->sv);
				ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
			}else{ms.text=(field_val){rtext_cast(name),0};}
		}
		pair c={b.x+b.w-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Save",vname&&!ename)||(wid.infield&&ev.action))modal_exit(1);c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_exit(0);
		if(ui_button((rect){b.x+b.w-125,b.y,60,20},"Home",!directory_is_home(ms.path))){
			directory_home(ms.path);ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
		}
		if(ui_button((rect){b.x+b.w-60,b.y,60,20},"Parent",directory_has_parent(ms.path))){
			directory_parent(ms.path);ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
		}
	}
	else if(ms.type==modal_alert){
		rect b=modal_rtext((pair){0,5+20});
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_confirm){
		rect b=modal_rtext((pair){0,5+20});char*v=ms.verb?ls(ms.verb)->sv:"OK";
		pair vs=font_textsize(FONT_MENU,v);vs.x=MIN(MAX(60,vs.x+10),200-65); pair c={b.x+b.w-vs.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,vs.x,20},v,1))modal_exit(1);c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_input){
		rect b=modal_rtext((pair){0,5+20+5+20});
		ui_field((rect){b.x,b.y+b.h-(20+5+20),b.w,20},&ms.text);
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",1)||ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_choose_lil){
		rect b=modal_rtext((pair){0,5+80+5+20});
		int choose=ui_table((rect){b.x,b.y+b.h-(20+5+80),b.w,80},0,0,0,"s",&ms.grid);
		if(ui_button((rect){b.x+b.w-60,b.y+b.h-20,60,20},"OK",ms.grid.row>=0)||choose)modal_exit(1);
	}
	else if(ms.type==modal_brush){
		pair grid={6,4};int ss=25, gs=ss+4, m=5, lh=font_h(FONT_BODY);
		lv*br=dget(deck->b,lmistr("brushes"));grid.y+=ceil(br->c/(1.0*grid.x));
		rect b=draw_modalbox((pair){m+(grid.x*gs)+m,m+(grid.y*gs)+lh+m});
		char*lab=dr.brush>=(6*4)?br->kv[dr.brush-(6*4)]->sv:"Choose a brush shape.";
		draw_textc((rect){b.x,b.y+b.h-lh,b.w,lh},lab,FONT_BODY,1);
		for(int z=0;z<grid.x*grid.y;z++){
			rect s={b.x+m+2+gs*(z%grid.x),b.y+m+2+gs*(z/grid.x),ss,ss};
			pair c={s.x+s.w/2,s.y+s.h/2}; draw_line((rect){c.x,c.y,c.x,c.y},z,1,deck);
			if(z==dr.brush)draw_box(inset(s,-2),0,1);
			int a=dover(s)&&over(s), cs=(z==dr.brush&&ev.action), cl=cs||((ev.md||ev.drag)&&a), cr=cs||(ev.mu&&a);
			if(cl)draw_invert(pal,inset(s,-1)); if(cr){dr.brush=z;modal_exit(z);break;}
		}
		if(ev.exit||(ev.mu&&!dover(b)&&!over(b)))modal_exit(-1),ev.mu=0;
		if(ev.dir==dir_left )dr.brush=((dr.brush/grid.x)*grid.x)+((dr.brush+grid.x-1)%grid.x);
		if(ev.dir==dir_right)dr.brush=((dr.brush/grid.x)*grid.x)+((dr.brush+       1)%grid.x);
		if(ev.dir==dir_up   )dr.brush=(dr.brush+(grid.x*(grid.y-1)))%(grid.x*grid.y);
		if(ev.dir==dir_down )dr.brush=(dr.brush+grid.x             )%(grid.x*grid.y);
		dr.brush=CLAMP(0,dr.brush,(6*4)+br->c-1);
	}
	else if(ms.type==modal_pattern||ms.type==modal_fill){
		pair grid={8,dr.color?2:4};int ss=25, gs=ss+4, m=5, lh=font_h(FONT_BODY); int*v=ms.type==modal_pattern?&dr.pattern:&dr.fill;
		rect b=draw_modalbox((pair){m+(grid.x*gs)+m,m+(grid.y*gs)+lh+m});
		char*label=dr.color?(ms.type==modal_fill?"Choose a fill color."  :"Choose a stroke color."  ):
		                    (ms.type==modal_fill?"Choose a fill pattern.":"Choose a stroke pattern.");
		draw_textc((rect){b.x,b.y+b.h-lh,b.w,lh},label,FONT_BODY,1);
		for(int z=0;z<grid.x*grid.y;z++){
			rect s={b.x+m+2+gs*(z%grid.x),b.y+m+2+gs*(z/grid.x),ss,ss};
			int ci=!dr.color?z: z<2?z: 31+z;
			draw_rect(s,ci==0?32:ci); if(ci==*v)draw_box(inset(s,-2),0,1);
			int a=dover(s)&&over(s), cs=(ci==*v&&ev.action), cl=cs||((ev.md||ev.drag)&&a), cr=cs||(ev.mu&&a);
			if(cl)draw_invert(pal,inset(s,-1)); if(cr){*v=ci;modal_exit(ci);break;}
		}
		if(ev.exit||(ev.mu&&!dover(b)&&!over(b)))modal_exit(-1),ev.mu=0;
		if(ev.dir&&dr.color&&*v>=2)*v=*v-31;
		if(ev.dir==dir_left )*v=((*v/grid.x)*grid.x)+((*v+grid.x-1)%grid.x);
		if(ev.dir==dir_right)*v=((*v/grid.x)*grid.x)+((*v+       1)%grid.x);
		if(ev.dir==dir_up   )*v=(*v+(grid.x*(grid.y-1)))%(grid.x*grid.y);
		if(ev.dir==dir_down )*v=(*v+grid.x             )%(grid.x*grid.y);
		if(ev.dir&&dr.color&&*v>=2)*v=*v+31;
	}
	else if(ms.type==modal_grid){
		rect b=draw_modalbox((pair){120,160});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Grid Size",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Width" ,FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,42,20},"Height",FONT_MENU,1);
		ui_field((rect){b.x+42,b.y+20,b.w-42,18},&ms.name);
		ui_field((rect){b.x+42,b.y+40,b.w-42,18},&ms.text);
		dr.grid_size.x=ln(rtext_all(ms.name.table));dr.grid_size.x=MAX(1,dr.grid_size.x);
		dr.grid_size.y=ln(rtext_all(ms.text.table));dr.grid_size.y=MAX(1,dr.grid_size.y);
		pair zc={b.x,b.y+70};draw_hline(b.x,b.x+b.w,zc.y-5,13);
		draw_textc((rect){zc.x,zc.y,b.w,20},"FatBits Scale",FONT_MENU,1);zc.y+=20;
		if(ui_radio((rect){zc.x,zc.y,b.w,16},"2x",1,dr.zoom==2)){dr.zoom=2;}zc.y+=16;
		if(ui_radio((rect){zc.x,zc.y,b.w,16},"4x",1,dr.zoom==4)){dr.zoom=4;}zc.y+=16;
		if(ui_radio((rect){zc.x,zc.y,b.w,16},"8x",1,dr.zoom==8)){dr.zoom=8;}zc.y+=16;
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_deck_props){
		rect b=draw_modalbox((pair){220,100});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Deck Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Name"  ,FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,42,20},"Author",FONT_MENU,1);
		ui_field((rect){b.x+42,b.y+20,b.w-42,18},&ms.name);
		ui_field((rect){b.x+42,b.y+40,b.w-42,18},&ms.text);
		iwrite(deck,lmistr("name"  ),rtext_all(ms.name.table));
		iwrite(deck,lmistr("author"),rtext_all(ms.text.table));mark_dirty();
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))setscript(deck),modal_exit(0);c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Protect...",1))modal_enter(modal_save_locked);
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_card_props){
		rect b=draw_modalbox((pair){220,100});lv*card=ifield(deck,"card");
		draw_textc((rect){b.x,b.y-5,b.w,20},"Card Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Name",FONT_MENU,1);
		ui_field((rect){b.x+42,b.y+20,b.w-42,18},&ms.name);
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))setscript(card),modal_exit(0);
	}
	else if(ms.type==modal_button_props){
		rect b=draw_modalbox((pair){220,170});lv*button=ob.sel->lv[0];
		draw_textc((rect){b.x,b.y-5,b.w,20},"Button Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Name",FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,42,20},"Text",FONT_MENU,1);
		ui_field((rect){b.x+42,b.y+20,b.w-42,18},&ms.name);
		ui_field((rect){b.x+42,b.y+40,b.w-42,18},&ms.text);
		iwrite(button,lmistr("name"),rtext_all(ms.name.table));
		iwrite(button,lmistr("text"),rtext_all(ms.text.table));
		draw_text((rect){b.x+b.w/2,b.y+72,54,20},"Shortcut",FONT_MENU,1);
		lv*s=normalize_shortcut(rtext_all(ms.form0.table));ms.form0.table=rtext_cast(s);
		if(wid.fv==&ms.form0)wid.cursor=(pair){CLAMP(0,wid.cursor.x,s->c),CLAMP(0,wid.cursor.y,s->c)};
		ui_field((rect){b.x+b.w/2+54,b.y+70,b.w/2-54,18},&ms.form0);
		iwrite(button,lmistr("shortcut"),rtext_all(ms.form0.table));
		mark_dirty();
		int style=ordinal_enum(ifield(button,"style"),button_styles);pair sb={b.x,b.y+70};
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Round"    ,1,style==button_round    )){iwrite(button,lmistr("style"),lmistr("round"    )),mark_dirty();}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Rectangle",1,style==button_rect     )){iwrite(button,lmistr("style"),lmistr("rect"     )),mark_dirty();}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Checkbox" ,1,style==button_check    )){iwrite(button,lmistr("style"),lmistr("check"    )),mark_dirty();}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Invisible",1,style==button_invisible)){iwrite(button,lmistr("style"),lmistr("invisible")),mark_dirty();}sb.y+=16;
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))setscript(button),modal_exit(0); c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Action...",card_is(con())))modal_enter(modal_action);
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_field_props){
		rect b=draw_modalbox((pair){260,200+60});lv*f=ob.sel->lv[0];field p=unpack_field(f,NULL);
		draw_textc((rect){b.x,b.y-5,b.w,20},"Field Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Name",FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,42,60},"Text",FONT_MENU,1);
		ui_field((rect){b.x+42,b.y+20,b.w-42,18},&ms.name);
		int style=ordinal_enum(ifield(f,"style"),field_styles);
		widget_field(NULL,(field){{b.x+42,b.y+40,b.w-42,88},p.font,show_solid,1,1,style,p.align,0},&ms.text);
		iwrite(f,lmistr("name"),rtext_all(ms.name.table));
		iwrite(f,lmistr("value"),ms.text.table);mark_dirty();
		int border=lb(ifield(f,"border")), scrollbar=lb(ifield(f,"scrollbar")); pair cb={b.x,b.y+80+60};
		if(ui_checkbox((rect){cb.x,cb.y,b.w,16},"Border"   ,1,border   )){border   ^=1;iwrite(f,lmistr("border"   ),lmn(border   )),mark_dirty();}cb.y+=16;
		if(ui_checkbox((rect){cb.x,cb.y,b.w,16},"Scrollbar",1,scrollbar)){scrollbar^=1;iwrite(f,lmistr("scrollbar"),lmn(scrollbar)),mark_dirty();}cb.y+=16;
		pair sb={b.x,cb.y+10};int cp=0;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Rich Text" ,1,style==field_rich )){iwrite(f,lmistr("style"),lmistr("rich" )),mark_dirty()     ;}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Plain Text",1,style==field_plain)){iwrite(f,lmistr("style"),lmistr("plain")),mark_dirty(),cp=1;}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Code"      ,1,style==field_code )){iwrite(f,lmistr("style"),lmistr("code" )),mark_dirty(),cp=1;}sb.y+=16;
		if(cp&&!rtext_is_plain(ms.text.table))ms.text.table=rtext_cast(rtext_all(ms.text.table));
		int align=ordinal_enum(ifield(f,"align"),field_aligns);pair ab={b.x+(b.w/2),cb.y+10};
		if(ui_radio((rect){ab.x,ab.y,b.w/2,16},"Align Left" ,1,align==align_left  )){iwrite(f,lmistr("align"),lmistr("left"  )),mark_dirty();}ab.y+=16;
		if(ui_radio((rect){ab.x,ab.y,b.w/2,16},"Center"     ,1,align==align_center)){iwrite(f,lmistr("align"),lmistr("center")),mark_dirty();}ab.y+=16;
		if(ui_radio((rect){ab.x,ab.y,b.w/2,16},"Align Right",1,align==align_right )){iwrite(f,lmistr("align"),lmistr("right" )),mark_dirty();}ab.y+=16;
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))setscript(f),modal_exit(0);
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_slider_props){
		rect b=draw_modalbox((pair){220,170});lv*f=ob.sel->lv[0];
		draw_textc((rect){b.x,b.y-5,b.w,20},"Slider Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Name"  ,FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,42,20},"Format",FONT_MENU,1);
		ui_field((rect){b.x+50,b.y+20,b.w-50,18},&ms.name);
		ui_field((rect){b.x+50,b.y+40,b.w-50,18},&ms.text);
		iwrite(f,lmistr("name"  ),rtext_all(ms.name.table));
		iwrite(f,lmistr("format"),rtext_all(ms.text.table));mark_dirty();
		int style=ordinal_enum(ifield(f,"style"),slider_styles);pair sb={b.x,b.y+70};
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Horizontal",1,style==slider_horiz  )){iwrite(f,lmistr("style"),lmistr("horiz"  )),mark_dirty();}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Vertical"  ,1,style==slider_vert   )){iwrite(f,lmistr("style"),lmistr("vert"   )),mark_dirty();}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Bar"       ,1,style==slider_bar    )){iwrite(f,lmistr("style"),lmistr("bar"    )),mark_dirty();}sb.y+=16;
		if(ui_radio((rect){sb.x,sb.y,b.w/2,16},"Compact"   ,1,style==slider_compact)){iwrite(f,lmistr("style"),lmistr("compact")),mark_dirty();}sb.y+=16;
		pair ib={b.x+b.w/2,b.y+70};
		draw_text((rect){ib.x+5,ib.y+2,40,20},"Min" ,FONT_MENU,1);ui_field((rect){ib.x+40,ib.y,b.w/2-40,18},&ms.form0);ib.y+=20;
		draw_text((rect){ib.x+5,ib.y+2,40,20},"Max" ,FONT_MENU,1);ui_field((rect){ib.x+40,ib.y,b.w/2-40,18},&ms.form1);ib.y+=20;
		draw_text((rect){ib.x+5,ib.y+2,40,20},"Step",FONT_MENU,1);ui_field((rect){ib.x+40,ib.y,b.w/2-40,18},&ms.form2);ib.y+=20;
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))setscript(f),modal_exit(0);
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_canvas_props){
		rect b=draw_modalbox((pair){220,141});lv*canvas=ob.sel->lv[0];
		draw_textc((rect){b.x,b.y-5,b.w,20},"Canvas Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,42,20},"Name" ,FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,42,20},"Scale",FONT_MENU,1);
		ui_field((rect){b.x+42,b.y+20,b.w-42,18},&ms.name);
		ui_field((rect){b.x+42,b.y+40,b.w-42,18},&ms.text);
		iwrite(canvas,lmistr("name" ),rtext_all(ms.name.table));
		iwrite(canvas,lmistr("scale"),rtext_all(ms.text.table));mark_dirty();
		int border=lb(ifield(canvas,"border")),draggable=lb(ifield(canvas,"draggable"));pair cb={b.x,b.y+50+20};
		if(ui_checkbox((rect){cb.x,cb.y,b.w,16},"Border"   ,1,border   )){border   ^=1;iwrite(canvas,lmistr("border"   ),lmn(border   )),mark_dirty();}cb.y+=16;
		if(ui_checkbox((rect){cb.x,cb.y,b.w,16},"Draggable",1,draggable)){draggable^=1;iwrite(canvas,lmistr("draggable"),lmn(draggable)),mark_dirty();}
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))setscript(canvas),modal_exit(0);
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_grid_props){
		rect b=draw_modalbox((pair){280,216+70});lv*grid=ob.sel->lv[0];
		draw_textc((rect){b.x,b.y-5,b.w,20},"Grid Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,47,20},"Name"  ,FONT_MENU,1);
		draw_text((rect){b.x,b.y+42,47,20},"Format",FONT_MENU,1);
		draw_text((rect){b.x,b.y+62,47,20},"Value" ,FONT_MENU,1);
		ui_field   ((rect){b.x+47,b.y+20,b.w-47,18},  &ms.name);
		ui_field   ((rect){b.x+47,b.y+40,b.w-47,18},  &ms.text);
		ui_codeedit((rect){b.x+47,b.y+60,b.w-47,118},1,&ms.form0);
		lv*etext=rtext_all(ms.form0.table),*format=rtext_all(ms.text.table),*eval=table_decode(etext,format);
		char desc[4096];snprintf(desc,sizeof(desc),"%d column%s, %d row%s.",eval->c,eval->c==1?"":"s",eval->n,eval->n==1?"":"s");
		draw_text((rect){b.x+47,b.y+120+60,b.w-47,18},desc,FONT_BODY,1);
		iwrite(grid,lmistr("name"  ),rtext_all(ms.name.table));
		iwrite(grid,lmistr("format"),format);mark_dirty();
		int headers=lb(ifield(grid,"headers")), scrollbar=lb(ifield(grid,"scrollbar")), lines=lb(ifield(grid,"lines")), bycell=lb(ifield(grid,"bycell"));
		pair cb={b.x,b.y+130+70};
		if(ui_checkbox((rect){cb.x,cb.y,b.w/2,16},"Column Headers",1,headers  )){headers  ^=1;iwrite(grid,lmistr("headers"  ),lmn(headers  )),mark_dirty();}cb.y+=16;
		if(ui_checkbox((rect){cb.x,cb.y,b.w/2,16},"Scrollbar"     ,1,scrollbar)){scrollbar^=1;iwrite(grid,lmistr("scrollbar"),lmn(scrollbar)),mark_dirty();}cb.y+=16;
		if(ui_checkbox((rect){cb.x,cb.y,b.w/2,16},"Grid Lines"    ,1,lines    )){lines    ^=1;iwrite(grid,lmistr("lines"    ),lmn(lines    )),mark_dirty();}cb.y+=16;
		if(ui_checkbox((rect){cb.x,cb.y,b.w/2,16},"Select by Cell",1,bycell   )){bycell   ^=1;iwrite(grid,lmistr("bycell"   ),lmn(bycell   )),mark_dirty();}
		pair eb={b.x+(b.w/2),b.y+130+70};
		if(ui_radio((rect){eb.x,eb.y,b.w/2,16},"Edit as JSON",1,ms.edit_json==1)){
			str r=str_new();fjson(&r,l_cols(eval));ms.form0=(field_val){rtext_cast(lmstr(r)),0};ms.edit_json=1;
		}eb.y+=16;
		if(ui_radio((rect){eb.x,eb.y,b.w/2,16},"Edit as CSV",1,ms.edit_json==0)){
			ms.form0=(field_val){rtext_cast(n_writecsv(NULL,format->c?lml2(eval,format):l_list(eval))),0};ms.edit_json=0;
		}
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))modal_exit(0),setscript(grid);c.x+=65;
		lv*w=ifield(grid,"widths");
		if(ui_button((rect){c.x,c.y,90,20},"Reset Widths",w->c))iwrite(grid,lmistr("widths"),lml(0)),mark_dirty();
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_contraption_props){
		rect b=draw_modalbox((pair){240,240});lv*contraption=ob.sel->lv[0];
		draw_textc((rect){b.x,b.y-5,b.w,20},title_caps(ifield(ifield(contraption,"def"),"name")->sv," Properties"),FONT_MENU,1);
		draw_text((rect){b.x,b.y+22,47,20},"Name",FONT_MENU,1);
		ui_field ((rect){b.x+47  ,b.y+20  ,b.w-47,18},&ms.name);
		iwrite(contraption,lmistr("name"),rtext_all(ms.name.table)),mark_dirty();
		widget_attributes((rect){b.x,b.y+42,b.w,b.h-42-25});
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script..."   ,1))modal_exit(0),setscript(contraption);c.x+=65;
		if(ui_button((rect){c.x,c.y,80,20},"Prototype...",1))modal_exit(1),con_set(ifield(contraption,"def"));
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(1);
	}
	else if(ms.type==modal_prototype_props){
		rect b=draw_modalbox((pair){220,230});lv*def=con();int lw=67;
		draw_textc((rect){b.x,b.y-5,b.w,20},"Prototype Properties",FONT_MENU,1);
		draw_text((rect){b.x,b.y+ 22,lw,20},"Name",FONT_MENU,1);
		draw_text((rect){b.x,b.y+ 42,lw,20},"Version",FONT_MENU,1);
		draw_text((rect){b.x,b.y+ 62,lw,20},"Description",FONT_MENU,1);
		draw_text((rect){b.x,b.y+122,lw,20},"Template\nScript",FONT_MENU,1);
		ui_field   ((rect){b.x+lw,b.y+20 ,b.w-lw,18},&ms.name);
		ui_field   ((rect){b.x+lw,b.y+40 ,b.w-lw,18},&ms.form2);
		ui_textedit((rect){b.x+lw,b.y+60 ,b.w-lw,58},1,&ms.form0);
		ui_codeedit((rect){b.x+lw,b.y+120,b.w-lw,78},1,&ms.form1);
		iwrite(def,lmistr("name"       ),rtext_all(ms.name .table));
		iwrite(def,lmistr("version"    ),rtext_all(ms.form2.table));
		iwrite(def,lmistr("description"),rtext_all(ms.form0.table));
		iwrite(def,lmistr("template"   ),rtext_all(ms.form1.table));mark_dirty();
		pair c={b.x,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Script...",1))modal_exit(1),setscript(def);c.x+=65;
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_prototype_attrs){
		rect b=draw_modalbox((pair){220,200});lv*def=con();int lw=42;
		draw_textc((rect){b.x,b.y-5,b.w,20},title_caps(ifield(def,"name")->sv," Attributes"),FONT_MENU,1);
		rect gsize={b.x,b.y+20,80,b.h-(20+5+20)};
		int before=ms.grid.row;ui_table(gsize,1,gsize.w-18,0,"s",&ms.grid);
		if(before!=ms.grid.row||ms.name.table==NULL||ms.text.table==NULL){
			ms.name=(field_val){rtext_cast(ms.grid.row>=0?ms.grid.table->lv[0]->lv[ms.grid.row]:lmistr("")),0};
			ms.text=(field_val){rtext_cast(ms.grid.row>=0?ms.grid.table->lv[1]->lv[ms.grid.row]:lmistr("")),0};
		}
		int sel=ms.grid.row>=0;
		draw_text((rect){gsize.x+gsize.w+10,b.y+20+2,lw,20},"Name" ,FONT_MENU,1);
		draw_text((rect){gsize.x+gsize.w+10,b.y+40+2,lw,20},"Label",FONT_MENU,1);
		draw_text((rect){gsize.x+gsize.w+10,b.y+60+2,lw,20},"Type" ,FONT_MENU,1);
		ui_dfield((rect){gsize.x+gsize.w+5+lw,b.y+20,b.w-(lw+5+gsize.w),18},sel,&ms.name);
		ui_dfield((rect){gsize.x+gsize.w+5+lw,b.y+40,b.w-(lw+5+gsize.w),18},sel,&ms.text);
		if(sel){
			ms.grid.table->lv[0]->lv[ms.grid.row]=rtext_all(ms.name.table);
			ms.grid.table->lv[1]->lv[ms.grid.row]=rtext_all(ms.text.table);
		}
		pair cr={gsize.x+gsize.w+5+lw,b.y+62}, c={b.x,b.y+b.h-20};
		char*attr_labels[]={"","Boolean","Number","String","Code","Rich Text",NULL};
		char*t=sel?ms.grid.table->lv[2]->lv[ms.grid.row]->sv:"";
		for(int z=1;attr_labels[z];z++,cr.y+=16)if(ui_radio((rect){cr.x,cr.y,b.w-(lw+5+gsize.w),16},attr_labels[z],sel,!strcmp(t,attribute_types[z]))){
			ms.grid.table->lv[2]->lv[ms.grid.row]=lmistr(attribute_types[z]);
		}
		if(ui_button((rect){c.x,c.y,60,20},"Add",1)){
			ll_add(ms.grid.table->lv[0],lmistr("untitled"));
			ll_add(ms.grid.table->lv[1],lmistr(""));
			ll_add(ms.grid.table->lv[2],lmistr("bool"));
			torect(ms.grid.table);ms.grid.row=ms.grid.table->n-1;ms.name.table=NULL,ms.text.table=NULL;
		};c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Remove",sel)){
			ms.grid.table=l_drop(l_list(lmn(ms.grid.row)),ms.grid.table);
			ms.grid.row=-1,ms.name.table=NULL,ms.text.table=NULL;
		}
		if(ui_button((rect){b.x+b.w-60,c.y,60,20},"OK",1)||ev.exit){iwrite(def,lmistr("attributes"),ms.grid.table),mark_dirty(),modal_exit(1);}
	}
	else if(ms.type==modal_action){
		rect b=draw_modalbox((pair){220,190});
		draw_textc((rect){b.x,b.y-5,b.w,20},"Button Action",FONT_MENU,1);
		pair c={b.x+b.w-60,b.y+b.h-20};
		int ready=(ms.act_go||ms.act_sound)&&(ms.act_go?(ms.act_gomode!=5||ms.verb->c):1)&&(ms.act_sound?(ms.message->c):1);
		if(ui_button((rect){c.x,c.y,60,20},"OK",ready))modal_exit(1);c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_exit(0);
		if(ui_checkbox((rect){b.x,b.y+20,b.w/2,16},"Go to Card",1,ms.act_go))ms.act_go^=1; pair cr={b.x,b.y+36};
		if(ui_radio((rect){cr.x+5,cr.y,80,16},"First"   ,ms.act_go,ms.act_gomode==0))ms.act_gomode=0;cr.y+=16;
		if(ui_radio((rect){cr.x+5,cr.y,80,16},"Previous",ms.act_go,ms.act_gomode==1))ms.act_gomode=1;cr.y+=16;
		if(ui_radio((rect){cr.x+5,cr.y,80,16},"Next"    ,ms.act_go,ms.act_gomode==2))ms.act_gomode=2;cr.y+=16;
		if(ui_radio((rect){cr.x+5,cr.y,80,16},"Last"    ,ms.act_go,ms.act_gomode==3))ms.act_gomode=3;cr.y+=16;
		if(ui_radio((rect){cr.x+5,cr.y,80,16},"Back"    ,ms.act_go,ms.act_gomode==4))ms.act_gomode=4;cr.y+=16;
		if(ui_radio((rect){cr.x+5,cr.y,45,16},"Pick:"   ,ms.act_go,ms.act_gomode==5))ms.act_gomode=5;
		if(ms.act_go&&ms.act_gomode==5){
			rect l={cr.x+5+45,cr.y,b.w-5-45-5-60,16};
			draw_hline(l.x,l.x+l.w,l.y+l.h,13),draw_text_fit(inset(l,1),ms.verb->sv,FONT_BODY,1);
			if(ui_button((rect){b.x+b.w-60,cr.y,60,20},"Choose...",ms.act_go&&ms.act_gomode==5))modal_push(modal_pick_card);
		}cr.y+=26;
		if(ui_checkbox((rect){cr.x,cr.y,80,16},"Play a Sound",1,ms.act_sound))ms.act_sound^=1;
		if(ms.act_go){
			if(ui_checkbox((rect){b.x+b.w/2,b.y+20,b.w/2-19,16},"With Transition",1,ms.act_trans))ms.act_trans^=1;
			if(ms.act_trans){
				rect gd={b.x+b.w/2,b.y+36,b.w/2,70},pv={b.x+b.w-17,b.y+20,17,13};
				if(ms.grid.scroll==-99){ms.grid.scroll=grid_scrollto(ms.grid.table,(grid){gd,FONT_BODY,{0},"",0,1,0,0,show_solid,1},-1,ms.grid.row);}
				ui_list(gd,&ms.grid);ms.trans=dget(dget(deck->b,lmistr("transit")),ms.grid.table->lv[0]->lv[ms.grid.row]);
				do_transition((frame_count%60)/60.0,0),buffer_paste(pv,frame.clip,container_image(ms.canvas,1)->b,frame.buffer,1),draw_box(pv,0,1);
			}
		}
		if(ms.act_sound){
			rect l={cr.x+5+75,cr.y,b.w-5-75-5-60,16};
			draw_hline(l.x,l.x+l.w,l.y+l.h,13),draw_text_fit(inset(l,1),ms.message->sv,FONT_BODY,1);
			if(ui_button((rect){b.x+b.w-60,cr.y,60,20},"Choose...",ms.act_sound)){
				ms.act_transno=ms.grid.row,ms.grid=(grid_val){sounds_enumerate(),0,-1,-1},ms.from_action=1,ms.type=modal_sounds;
			}
		}
	}
	else if(ms.type==modal_pick_card){
		rect b=draw_modalbox((pair){220,45});
		draw_textc((rect){b.x,b.y,b.w,16},"Pick a card- any card.",FONT_BODY,1);
		pair c={b.x+(b.w-60-5-60-5-60)/2,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Previous",1)||ev.dir==dir_left){n_go(deck,l_list(lmistr("Prev")));}c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Choose",1)){
			lv*name=ifield(ifield(deck,"card"),"name");
			n_go(deck,l_list(lmn(ms.act_card)));
			if(lb(ms.carda))ob.sel=l_list(ms.carda);
			modal_pop(0);
			if(ms.type==modal_action)ms.verb=name;
			if(ms.type==modal_link)ms.text=(field_val){rtext_cast(name),0};
		}c.x+=65;
		if(ui_button((rect){c.x,c.y,60,20},"Next",1)||ev.dir==dir_right){n_go(deck,l_list(lmistr("Next")));}
	}
	else if(ms.type==modal_pick_contraption){
		rect b=draw_modalbox((pair){250,230});
		draw_textc((rect){b.x,b.y-5,b.w,20},"New Contraption",FONT_MENU,1);
		rect gsize={b.x,b.y+15,b.w,b.h-20-50-25};
		int choose=ui_table(gsize,1,16,0,"Is",&ms.grid);
		rect psize={b.x,gsize.y+gsize.h+5,b.w,50};draw_box(psize,0,1);psize=inset(psize,2);
		if(ms.grid.row>=0){
			char*desc=ifield(ifield(deck,"contraptions")->lv[ms.grid.row],"description")->sv;
			layout_plaintext(desc,FONT_BODY,align_left,(pair){psize.w,psize.h});draw_text_wrap(psize,1);
		}
		pair c={b.x+b.w-60,b.y+b.h-20};
		if(ui_button((rect){c.x,c.y,60,20},"Create",ms.grid.row>=0)||choose){
			lv*p=lmd();dset(p,lmistr("type"),lmistr("contraption"));dset(p,lmistr("def"),ms.grid.table->lv[1]->lv[ms.grid.row]);
			ob_create(l_list(p));modal_exit(1);
		};c.x-=65;
		if(ui_button((rect){c.x,c.y,60,20},"Cancel",1)||ev.exit)modal_exit(0);
	}
	else if(ms.type==modal_trans){
		do_transition((ms.time_curr*1.0)/ms.time_end,1);
		lv*i=container_image(ms.canvas,1);buffer_paste(rect_pair((pair){0,0},frame.size),frame.clip,i->b,frame.buffer,1);
		ms.time_curr++;if(ms.time_curr>ms.time_end||do_panic)modal_exit(0);
	}
	ms.in_modal=0;
}

lv*plain_or_rich(lv*z){z=l_first(z);return lit(z)?rtext_cast(z):ls(z);}
lv*n_alert(lv*self,lv*z){
	(void)self;char*type=z->c>1?ls(z->lv[1])->sv:"none";
	if(!strcmp(type,"bool")){modal_enter(modal_confirm_lil);ms.verb=z->c>2?ls(z->lv[2]):NULL;}
	else if(!strcmp(type,"string")){modal_enter(modal_input_lil);if(z->c>2)ms.text.table=rtext_cast(z->lv[2]);}
	else if(!strcmp(type,"choose")){
		modal_enter(modal_choose_lil);
		ms.verb=z->c<=2?ld(LNIL): lil(z->lv[2])?l_dict(z->lv[2],z->lv[2]): ld(z->lv[2]);if(ms.verb->c<1)ms.verb=ld(l_list(ZERO));
		ms.grid=(grid_val){lt(l_keys(ms.verb)), 0, z->c<=3?-1:dgeti(ms.verb,z->lv[3]),-1};
	}
	else{modal_enter(modal_alert_lil);}
	ms.message=plain_or_rich(z);return LNIL;
}
lv*n_open(lv*self,lv*z){
	(void)self,(void)z;modal_enter(modal_open_lil);
	char*type=ls(l_first(z))->sv;lv*r=lms(0),*hint=z->c>1?z->lv[1]:ZERO;
	if(!strcmp(type,"array"))ms.desc="Open a binary file.",r=array_make(0,0,0,r);
	if(!strcmp(type,"image"))ms.filter=filter_image,ms.desc="Open an image file.",r=image_empty();
	if(!strcmp(type,"sound"))ms.filter=filter_sound,ms.desc="Open a .wav sound file.",r=sound_make(lms(0));
	if(!strcmp(type,"text" ))ms.filter=filter_data ,ms.desc="Open any .csv or .txt file.";
	if(!strcmp(type,"deck" ))ms.filter=filter_deck ,ms.desc="Open .deck or .html file.",r=deck_read(lmistr(""));
	if(image_is(r)&&(matchr(hint,lmistr("frames"))||matchr(hint,lmistr("gray_frames"))))r=empty_frames();
	ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1},ms.verb=hint;
	return r;
}
lv*n_save(lv*self,lv*z){
	(void)self;modal_enter(modal_save_lil);lv*value=l_first(z);ms.path_suffix[0]=0;
	if(array_is(value)){ms.desc="Save a binary file.";if(z->c>1)snprintf(ms.path_suffix,sizeof(ms.path_suffix),"%s",ls(z->lv[1])->sv);}
	if(sound_is(value))ms.filter=filter_sound,ms.desc="Save a .wav sound file.";
	if(image_is(value)||lid(value))ms.filter=filter_gif,ms.desc="Save a .gif image file.";
	if(deck_is(value))ms.filter=filter_deck,ms.desc="Save a .deck or .html file.";
	if(lil(value)){EACH(z,value)if(image_is(value->lv[z]))ms.filter=filter_gif,ms.desc="Save a .gif image file.";}
	ms.grid=(grid_val){directory_enumerate(ms.path,ms.filter,0),0,0,-1};
	return value;
}
void close_script(lv*next){
	sc.next=next;field_exit();lv*text=rtext_all(sc.f.table);parse(text->sv);
	if(!perr()){script_save(text),finish_script();return;}
	char t[4096];snprintf(t,sizeof(t),"The current script contains errors:\n\n%s\n\nDo you wish to discard your changes?",par.error);
	modal_enter(modal_confirm_script);ms.message=lmcstr(t),ms.verb=lmcstr("Discard");
}
void go_notify(lv*target_deck,lv*args,int dest){
	if(target_deck!=deck)return;
	if(args->c&&lis(args->lv[0])){
		char*s=l_first(args)->sv;char*p[]={"http://","https://","ftp://","gopher://","gemini://",NULL};
		int f=0;for(int z=0;p[z];z++)if(has_prefix(s,p[z])){f=1;break;}
		if(f){modal_enter(modal_url);ms.text=(field_val){rtext_cast(l_first(args)),0};return;}
	}
	lv*tfun=args->c<2?NULL: lion(args->lv[1])?args->lv[1]: dget(dget(target_deck->b,lmistr("transit")),args->lv[1]);
	int moved=dest!=ln(ifield(ifield(target_deck,"card"),"index"));
	if(moved){con_set(NULL);if(uimode==mode_script)close_script(NULL);}
	if(dest>=0&&tfun!=NULL&&ms.type!=modal_trans){
		modal_enter(modal_trans);ms.time_curr=0,ms.time_end=args->c<3?30: MAX(1,ln(args->lv[2]));
		ms.trans=tfun, ms.canvas=free_canvas(target_deck);
		ms.carda=draw_con(ifield(target_deck,"card"),0), ms.cardb=draw_con(ifield(target_deck,"cards")->lv[dest],0);
	}
	if(moved&&uimode==mode_interact)msg.pending_loop=1;
	if(moved||args->c>1){
		grid_exit(),field_exit(),bg_end_selection(),bg_end_lasso(),ob.sel->c=0,wid.active=ms.type==modal_listen?0:-1;mark_dirty();
	}
	if(uimode==mode_interact)msg.next_view=1;
}
void field_notify(lv*field){
	if(field_notify_disable||!wid.infield||wid.ft!=field)return;
	lv*v=ifield(field,"value");if(rtext_len(v))return;
	wid.fv->table=v;
	wid.cursor=(pair){0,0},wid.field_dirty=0;
	wid.hist=lml(0);wid.hist_cursor=0;
	if(enable_touch){field_exit();wid.active=-1;}
}
void validate_modules(void){
	lv*modules=ifield(deck,"modules");EACH(z,modules){
		lv*err=ifield(modules->lv[z],"error");if(!err->c)continue;
		modal_enter(modal_alert);ms.message=rtext_cast(l_format(lmcstr("The module \"%s\" failed to initialize: %s"),lml2(modules->kv[z],err)));
		break;
	}
}
void script_editor(rect r){
	int mh=3+font_h(FONT_MENU);
	lv*overw=NULL;if(sc.xray){
		lv*wids=con_wids();int ow=0;EACHR(z,wids){
			lv*wid=wids->lv[z];rect size=con_to_screenr(unpack_widget(wid).size);
			int o=ev.alt&&over(size)&&!ow, col=o?(overw=wid,13):44;ow|=o;
			draw_textc(size,ifield(wid,"name")->sv,FONT_BODY,o?-1:col),draw_box(size,0,col);
			if(ifield(wid,"script")->c)draw_icon((pair){size.x-1,size.y},ICONS[icon_lil],o?1:col);
			if(ev.alt&&ev.mu&&over(size)&&dover(size)){close_script(wid);ev.md=ev.mu=0;break;}
		}if(ev.alt&&ev.mu)close_script(con()),ev.md=ev.mu=0;
	}
	ui_codeedit((rect){r.x,r.y,r.w,r.h-mh},0,&sc.f),draw_hline(r.x,r.x+r.w,r.y+r.h-mh-1,1);
	if(overw){uicursor=cursor_point;draw_textc(con_to_screenr(unpack_widget(overw).size),ifield(overw,"name")->sv,FONT_BODY,-1);}
	if(strlen(sc.status)){draw_text_fit((rect){r.x+3,r.y+r.h-mh+3,r.w,mh-6},sc.status,FONT_BODY,1);}
	else{
		char stat[4096]={0};
		if(in_layer()&&wid.infield){
			int a=MIN(wid.cursor.x,wid.cursor.y), b=MAX(wid.cursor.x,wid.cursor.y);if(a!=b&&a<layout_count){
				a=MAX(0,MIN(a,layout_count)), b=MAX(0,MIN(b,layout_count));
				pair ap=field_position(a),bp=field_position(b);int l=(bp.x-ap.x)+1, c=b-a;
				snprintf(stat,sizeof(stat),"%d line%s, %d character%s selected",l,l==1?"":"s",c,c==1?"":"s");
			}else{pair p=field_position(MIN(a,b));snprintf(stat,sizeof(stat),"Line %d, Column %d",p.x,p.y);}
		}
		pair l=font_textsize(FONT_BODY,stat);draw_text((rect){3,r.y+r.h-mh+3,l.x,l.y},stat,FONT_BODY,1);
		if(sc.others->c){snprintf(stat,sizeof(stat),"script of %s  '%s' and %d more",sc.target->a->sv,ifield(sc.target,"name")->sv,sc.others->c);}
		else{snprintf(stat,sizeof(stat),"script of %s  '%s'",sc.target->a->sv,ifield(sc.target,"name")->sv);}
		pair t=layout_plaintext(stat,FONT_BODY,align_right,(pair){r.x+r.w-6-20-l.x,font_h(FONT_BODY)});
		draw_text_wrap((rect){3+l.x+20,r.y+r.h-mh+3,t.x,t.y},1);
	}if(in_layer()&&ev.exit)close_script(NULL),ev.exit=0;
}

// Keycaps

typedef struct {int v;char*l;float w;} keycap;
typedef struct {int c;keycap caps[16];} keyrow;
#define K(x,l) {(x-' ')+KEY_SPACE,l,1.0}
#define KBL       {0      ,"",1.0}
#define KCOMB(x)  {COMB_##x,"",1.0}
#define KS(x,l,w) {x,l,w}
#define KROWS 5
#define KEY_OK  -1
#define KEY_RUN -2
#define KEY_ALT -3
#define COMB_MACRON -10
#define COMB_ACUTE  -11
#define COMB_UMLAUT -12
#define COMB_CIRCUM -13
#define COMB_GRAVE  -14
#define COMB_HUNGRA -15
#define COMB_OGONEK -16
#define COMB_CARON  -17
#define COMB_TILDE  -18
#define COMB_COMMA  -19

keyrow LCAPS[KROWS]={
	{14,{K('`',"`"),K('1',"1"),K('2',"2"),K('3',"3"),K('4',"4"),K('5',"5"),K('6',"6"),K('7',"7"),K('8',"8"),K('9',"9"),K('0',"0"),K('-',"-"),K('=',"="),KS(KEY_BACKSPACE,"delete",1.5)}},
	{14,{KS(KEY_TAB,"tab",1.5),K('q',"q"),K('w',"w"),K('e',"e"),K('r',"r"),K('t',"t"),K('y',"y"),K('u',"u"),K('i',"i"),K('o',"o"),K('p',"p"),K('[',"["),K(']',"]"),K('\\',"\\")}},
	{13,{KS(KEY_CAPSLOCK,"capslock",2),K('a',"a"),K('s',"s"),K('d',"d"),K('f',"f"),K('g',"g"),K('h',"h"),K('j',"j"),K('k',"k"),K('l',"l"),K(';',";"),K('\'',"'"),KS(KEY_RETURN,"return",2)}},
	{12,{KS(KEY_LSHIFT,"shift",2.5),K('z',"z"),K('x',"x"),K('c',"c"),K('v',"v"),K('b',"b"),K('n',"n"),K('m',"m"),K(',',","),K('.',"."),K('/',"/"),KS(KEY_RSHIFT,"shift",2.5)}},
	{9,{KS(KEY_LEFT,"",1),KS(KEY_DOWN,"",1),KS(KEY_UP,"",1),KS(KEY_RIGHT,"",1),KS(KEY_ALT,"alt",1),KS(KEY_SPACE," ",5),KS(KEY_ALT,"alt",1),KS(KEY_RUN,"",2),KS(KEY_OK,"OK",2)}},
};
keyrow UCAPS[KROWS]={
	{14,{K('~',"~"),K('!',"!"),K('@',"@"),K('#',"#"),K('$',"$"),K('%',"%"),K('^',"^"),K('&',"&"),K('*',"*"),K('(',"("),K(')',")"),K('_',"_"),K('+',"+"),KS(KEY_BACKSPACE,"delete",1.5)}},
	{14,{KS(KEY_TAB,"tab",1.5),K('Q',"Q"),K('W',"W"),K('E',"E"),K('R',"R"),K('T',"T"),K('Y',"Y"),K('U',"U"),K('I',"I"),K('O',"O"),K('P',"P"),K('{',"{"),K('}',"}"),K('|',"|")}},
	{13,{KS(KEY_CAPSLOCK,"capslock",2.0),K('A',"A"),K('S',"S"),K('D',"D"),K('F',"F"),K('G',"G"),K('H',"H"),K('J',"J"),K('K',"K"),K('L',"L"),K(':',":"),K('"',"\""),KS(KEY_RETURN,"return",2)}},
	{12,{KS(KEY_LSHIFT,"shift",2.5),K('Z',"Z"),K('X',"X"),K('C',"C"),K('V',"V"),K('B',"B"),K('N',"N"),K('M',"M"),K('<',"<"),K('>',">"),K('?',"?"),KS(KEY_RSHIFT,"shift",2.5)}},
	{9,{KS(KEY_LEFT,"",1),KS(KEY_DOWN,"",1),KS(KEY_UP,"",1),KS(KEY_RIGHT,"",1),KS(KEY_ALT,"alt",1),KS(KEY_SPACE," ",5),KS(KEY_ALT,"alt",1),KS(KEY_RUN,"",2),KS(KEY_OK,"OK",2)}},
};
keyrow ALCAPS[KROWS]={
	{14,{KBL,{0xeb,"\xeb",1},KBL,KBL,{0xef,"\xef",1},KBL,KBL,KBL,{0xf0,"\xf0",1},KBL,KBL,KCOMB(MACRON),{0xff,"\xff",1},KS(0,"",1.5)}},
	{14,{KS(0,"",1.5),{0xd6,"\xd6",1},{0xc1,"\xc1",1},KCOMB(ACUTE),KBL,{0xbc,"\xbc",1},KBL,KCOMB(UMLAUT),KCOMB(CIRCUM),{0xb6,"\xb6",1},KBL,{0xed,"\xed",1},{0xee,"\xee",1},KBL}},
	{13,{KS(KEY_CAPSLOCK,"capslock",2),{0xa4,"\xa4",1},{0x9e,"\x9e",1},{0xaf,"\xaf",1},KBL,KCOMB(GRAVE),KCOMB(HUNGRA),KBL,KCOMB(OGONEK),{0xce,"\xce",1},{0x7f,"\x7f",1},{0xa5,"\xa5",1},KS(0,"",2)}},
	{12,{KS(KEY_LSHIFT,"shift",2.5),KBL,KBL,{0xa6,"\xa6",1},KCOMB(CARON),{0xcc,"\xcc",1},KCOMB(TILDE),KBL,KCOMB(COMMA),{0xe3,"\xe3",1},{0xec,"\xec",1},KS(KEY_RSHIFT,"shift",2.5)}},
	{9,{KBL,KBL,KBL,KBL,KS(KEY_ALT,"alt",1),KS(KEY_SPACE," ",5),KS(KEY_ALT,"alt",1),KS(0,"",2),KS(0,"",2)}},
};
keyrow AUCAPS[KROWS]={
	{14,{KBL,{0xeb,"\xeb",1},KBL,KBL,{0xef,"\xef",1},KBL,KBL,KBL,{0xf0,"\xf0",1},KBL,KBL,KCOMB(MACRON),{0xff,"\xff",1},KS(0,"",1.5)}},
	{14,{KS(0,"",1.5),{0xd5,"\xd5",1},{0xc0,"\xc0",1},KCOMB(ACUTE),KBL,{0x9d,"\x9d",1},KBL,KCOMB(UMLAUT),KCOMB(CIRCUM),{0x97,"\x97",1},KBL,{0xed,"\xed",1},{0xee,"\xee",1},KBL}},
	{13,{KS(KEY_CAPSLOCK,"capslock",2),{0x85,"\x85",1},{0xea,"\xea",1},{0x90,"\x90",1},KBL,KCOMB(GRAVE),KCOMB(HUNGRA),KBL,KCOMB(OGONEK),{0xcd,"\xcd",1},{0x7f,"\x7f",1},{0x86,"\x86",1},KS(0,"",2)}},
	{12,{KS(KEY_LSHIFT,"shift",2.5),KBL,KBL,{0x87,"\x87",1},KCOMB(CARON),{0xcc,"\xcc",1},KCOMB(TILDE),KBL,KCOMB(COMMA),{0xe2,"\xe2",1},{0xec,"\xec",1},KS(KEY_RSHIFT,"shift",2.5)}},
	{9,{KBL,KBL,KBL,KBL,KS(KEY_ALT,"alt",1),KS(KEY_SPACE," ",5),KS(KEY_ALT,"alt",1),KS(0,"",2),KS(0,"",2)}},
};
int ACCENT[10][26]={
	// a   b   c   d   e   f   g   h   i   j   k   l   m   n   o   p   q   r   s   t   u   v   w   x   y   z
	{191,  0,  0,  0,199,  0,  0,  0,203,  0,  0,  0,  0,  0,210,  0,  0,  0,  0,  0,220,  0,  0,  0,  0,  0}, // macrons
	{160,  0,197,  0,168,  0,  0,  0,172,  0,  0,  0,  0,208,178,  0,  0,  0,216,  0,184,  0,  0,  0,187,225}, // acutes
	{163,  0,  0,  0,170,  0,  0,  0,174,  0,  0,  0,  0,  0,181,  0,  0,  0,  0,  0,186,  0,  0,  0,189,  0}, // umlauts
	{161,  0,  0,  0,169,  0,  0,  0,173,  0,  0,  0,  0,  0,179,  0,  0,  0,  0,  0,185,  0,  0,  0,  0,  0}, // circumflexes
	{159,  0,  0,  0,167,  0,  0,  0,171,  0,  0,  0,  0,  0,177,  0,  0,  0,  0,  0,183,  0,  0,  0,  0,  0}, // graves
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,212,  0,  0,  0,  0,  0,222,  0,  0,  0,  0,  0}, // hungarumlauts
	{195,  0,  0,  0,201,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // ogoneks
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,218,  0,  0,  0,  0,  0,  0,229}, // carons
	{162,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,176,180,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // tildes
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,231,233,  0,  0,  0,  0,  0,  0}, // commas
};
lv* isolate_accent(int id,lv*f){
	int i=-1;for(int z=0;z<26;z++)if(ACCENT[id][z]){i=z;break;}
	lv* a=iindex(f,(i+97         ),NULL); // simple char
	lv* b=iindex(f,(ACCENT[id][i]),NULL); // accented exemplar
	n_image_merge(b,lml2(lmistr("-"),a)); // pixels added for the accent
	return b;
}
void soft_keyboard(rect r,int*exit,int*eval){
	int y=r.y, kh=r.h/KROWS, sh=ev.shift^kc.lock^kc.shift, combiner=kc.comb;char*pal=patterns_pal(ifield(deck,"patterns"));
	for(int row=0;row<KROWS;row++){
		float w=0;for(int z=0;z<LCAPS[row].c;z++)w+=LCAPS[row].caps[z].w;
		for(int z=0,x=0;z<LCAPS[row].c;z++){
			keycap k=(kc.alt?(sh?AUCAPS:ALCAPS): (sh?UCAPS:LCAPS))[row].caps[z];
			rect b={r.x+x+1,y,z==LCAPS[row].c-1?(r.w-x):(k.w*(r.w/w)),kh+1};x+=b.w-1;
			draw_box(b,0,1);
			int e=k.v==KEY_RUN&&wid.f.style==field_code&&uimode==mode_interact;
			int letter=(k.v>='a'&&k.v<='z')?k.v-'a': (k.v>='A'&&k.v<='Z')?k.v-'A': -1;
			int accent=(-combiner)-10, accented=(letter!=-1&&accent>=0&&accent<=9)?ACCENT[accent][letter]: 0;
			char temp[4]={0};if(accented)snprintf(temp,4,"%c",sh?drom_toupper(accented): accented);
			int special=k.v==KEY_LSHIFT||k.v==KEY_RSHIFT||k.v==KEY_CAPSLOCK;
			if(combiner&&!special){if(accented)draw_textc(b,temp,FONT_MENU,1);}
			else if(k.v==KEY_LEFT )draw_iconc(b,ARROWS->lv[4],1);
			else if(k.v==KEY_DOWN )draw_iconc(b,ARROWS->lv[1],1);
			else if(k.v==KEY_UP   )draw_iconc(b,ARROWS->lv[0],1);
			else if(k.v==KEY_RIGHT)draw_iconc(b,ARROWS->lv[5],1);
			else                    draw_textc(b,e?"run":k.l,FONT_MENU,1);
			int kd=k.v>0&&k.v<4096&&keydown[k.v];b=inset(b,2);
			int a=dover(b)&&over(inset(b,-4))&&ev.down_modal==ms.type&&ev.down_uimode==uimode&&ev.down_caps==1;if(k.v&&a&&(ev.md||ev.drag))kd=1;
			if(combiner&&!special){
				if(ev.mu&&a&&accented){field_input(temp);kc.shift=0,kc.alt=0,kc.comb=0;}
				kd=kd&&accented&&a;
			}
			else if(k.v==KEY_OK ){draw_box(b,0,13);if(ev.mu&&a)*exit=1;}
			else if(k.v==KEY_ALT){if(ev.mu&&a)kc.alt^=1;if(kc.alt)kd=1;}
			else if(e){if(ev.mu&a)*eval=1;}
			else if(k.v==KEY_LSHIFT||k.v==KEY_RSHIFT){if(ev.mu&&a)kc.shift^=1;if(kc.shift||ev.shift)kd=1;}
			else if(k.v==KEY_CAPSLOCK){if(ev.mu&&a)kc.lock^=1;if(kc.lock)kd=1;}
			else if(k.v<=COMB_MACRON){
				int acc=(-k.v)-10;
				draw_box(b,0,1);draw_iconc(b,ACCENTS[acc],1);
				if(ev.mu&&a)kc.comb=k.v,kc.alt=0;
			}
			else if(ev.mu&&a&&k.v){
				if((k.v>=' '&&k.v<=255)||k.v=='\n'){char t[4]={0};snprintf(t,4,"%c",k.v);field_input(t);}
				else{field_keys(k.v,sh);}
				kc.shift=0,kc.alt=0;
			}
			if(kd)draw_invert(pal,b);
		}y+=kh;
	}
}
void keycaps(void){
	if(!wid.fv)kc.on=0;if(!kc.on)return;
	memset(frame.buffer->sv,0,frame.buffer->c);
	int mh=3+font_h(FONT_MENU);
	rect r={0,mh,frame.size.x+1,(frame.size.y/2)-mh};
	if(kc.heading){pair s=font_textsize(FONT_MENU,kc.heading);rect h={r.x,r.y,r.w,s.y+5};draw_textc(h,kc.heading,FONT_MENU,1);r.h-=h.h,r.y+=h.h;}
	if(ms.type)ms.in_modal=1;
	if(uimode==mode_script){script_editor(r);}
	else if(ms.type==modal_listen){rect c=frame.clip;frame.clip=(rect){r.x,r.y,r.w,r.h+6};wid.count=0,listener(r);frame.clip=c;}
	else{wid.count=wid.active;widget_field(wid.ft,(field){inset(r,5),wid.f.font,show_solid,1,1,wid.f.style,wid.f.align,0},wid.fv),ms.in_modal=0;}
	int exit=0, eval=0;
	soft_keyboard(inset((rect){r.x,r.y+r.h+1,r.w-2,frame.size.y-(r.y+r.h)},5),&exit,&eval);
	if(ms.type==modal_listen&&(eval||ev.eval))listener_eval();
	if(ms.type!=modal_listen&&(eval||ev.eval))field_keys(KEY_RETURN,1);
	if(exit||ev.exit){
		field_exit();wid.active=-1;
		if(uimode==mode_script)close_script(NULL);
		if(ms.type==modal_gridcell)modal_exit(1);
		if(ms.type==modal_link    )modal_pop(1);
		if(ms.type==modal_listen  )modal_exit(0);
	}
}

// General Purpose History

enum edit_types{edit_bg_block,edit_ob_props,edit_ob_create,edit_ob_destroy,edit_proto_props};
void edit_target(lv*r){
	lv*c=con();
	if(card_is     (c))dset(r,lmistr("card"),ifield(c,"index"));
	if(prototype_is(c))dset(r,lmistr("def" ),ifield(c,"name"));
}
void apply(int fwd,lv*x){
	lv*container=NULL,*defn=dget(x,lmistr("def")),*card=dget(x,lmistr("card"));
	if(defn){con_set(container=dget(ifield(deck,"contraptions"),ls(defn)));}
	else if(card){
		lv*c=ifield(deck,"card");
		int cn=ln(ifield(c,"index")), tn=ln(card);
		if(cn!=tn)n_go(deck,l_list(lmn(tn))),c=ifield(deck,"card");
		con_set(NULL),container=c;
	}
	if(!container)return;
	int t=ln(dget(x,lmistr("type")));
	lv* wids=con_wids();
	if(t==edit_ob_create){fwd=!fwd,t=edit_ob_destroy;}
	if(t==edit_bg_block){
		if(uimode!=mode_draw)setuimode(mode_draw);
		rect r=getrect(dget(x,lmistr("pos")));
		lv*  p=dget(x,lmistr(fwd?"after":"before"));
		lv* bg=container_image(container,1);pair s=image_size(bg), cs=getpair(ifield(container,"size"));
		if(s.x!=cs.x||s.y!=cs.y){bg=image_resize(bg,cs),iwrite(container,lmistr("image"),bg),s=cs;}
		rect c=getrect(dget(x,lmistr("clr_pos")));
		lv*cb=dget(x,lmistr("clr_before"));
		lv*ca=dget(x,lmistr("clr_after"));
		rect clip={0,0,s.x,s.y};
		if(fwd&&ca)buffer_paste(c,clip,ca,bg->b,1);
		buffer_paste(r,clip,p,bg->b,1);
		if(!fwd&&cb)buffer_paste(c,clip,cb,bg->b,1);
	}
	else if(t==edit_ob_props){
		if(uimode!=mode_object)setuimode(mode_object);
		lv* p=dget(x,lmistr(fwd?"after":"before"));
		EACH(z,p){lv*d=p->lv[z],*w=dget(wids,p->kv[z]);if(w)EACH(i,d)iwrite(w,d->kv[i],d->lv[i]);}
	}
	else if(t==edit_ob_destroy){
		if(uimode!=mode_object)setuimode(mode_object);
		lv* props=dget(x,lmistr("props"));
		ob.sel->c=0;
		if(fwd){
			lv*w=lml(0);EACH(z,props){lv*wid=dget(wids,dget(props->lv[z],lmistr("name")));if(wid)ll_add(w,wid);}
			dset(x,lmistr("props"),con_copy_raw(container,w)); EACH(z,w)n_con_remove(container,l_list(w->lv[z]));
		}
		else{
			lv*w=con_paste_raw(container,props);EACH(z,w){
				dset(props->lv[z],lmistr("name"),ifield(w->lv[z],"name"));ll_add(ob.sel,w->lv[z]);
				if(dget(props->lv[z],lmistr("pos")))continue;
				rect c=box_center(con_dim(),getpair(ifield(w->lv[z],"size")));
				iwrite(w->lv[z],lmistr("pos"),lmpair((pair){c.x,c.y}));
			}
		}
	}
	else if(t==edit_proto_props){
		lv*k=dget(x,lmistr("keys")),*v=dget(x,lmistr(fwd?"after":"before"));
		EACH(z,k)iwrite(container,k->lv[z],v->lv[z]);
	}
	mark_dirty();
}
int  has_undo(void){return doc_hist_cursor>0;}
int  has_redo(void){return doc_hist_cursor<doc_hist->c;}
void undo(void){lv*x=doc_hist->lv[--(doc_hist_cursor)];apply(0,x);}
void redo(void){lv*x=doc_hist->lv[(doc_hist_cursor)++];apply(1,x);}
void edit(lv*x){doc_hist->c=doc_hist_cursor;ll_add(doc_hist,x),redo();}

// Draw Mode

rect normalize_rect(rect r){if(r.w<0)r.w*=-1,r.x-=r.w;if(r.h<0)r.h*=-1,r.y-=r.h;return r;}
#define BG_MASK 100
void bg_scratch_clear(void){pair s=buff_size(dr.scratch);memset(dr.scratch->sv,BG_MASK,s.x*s.y);}
void bg_scratch_under(void){if(!dr.scratch||!dr.under)return;lv*back=con_image()->b;EACH(z,back)if(back->sv[z]==1)dr.scratch->sv[z]=BG_MASK;}
void bg_scratch(void){
	pair c=con_size();if(!dr.scratch)dr.scratch=lmbuff(c);
	pair s=buff_size(dr.scratch);if(s.x!=c.x||s.y!=c.y)dr.scratch=lmbuff(c);
	bg_scratch_clear();
}
void bg_edit(void){
	rect d=find_occupied(dr.scratch,BG_MASK);
	lv*back=con_image()->b,*r=lmd();
	lv*after=buffer_copy(back,d);buffer_overlay(after,buffer_copy(dr.scratch,d),BG_MASK,(pair){0,0});
	dset(r,lmistr("type"  ),lmn(edit_bg_block));
	dset(r,lmistr("pos"   ),lmrect(d));
	dset(r,lmistr("before"),buffer_copy(back,d));
	dset(r,lmistr("after" ),after);
	edit_target(r);
	edit(r);
}
void draw_limbo(rect clip,int scale){
	clip=normalize_rect(clip);if(!dr.limbo){/*nothing*/}
	else if(scale&&dr.limbo_dither){draw_rect      (clip,21);}
	else if(scale                 ){draw_fat_scaled(screen_to_conr(clip),dr.limbo,!dr.trans,patterns_pal(ifield(deck,"patterns")),frame_count,FAT,con_to_screen((pair){0,0}));}
	else if(       dr.limbo_dither){draw_dithered  (clip,dr.limbo,!dr.trans,dr.omask);}
	else                           {draw_scaled    (clip,dr.limbo,!dr.trans);}
}
lv* bg_scaled_limbo(void){
	rect d=dr.sel_here;
	lv*back=con_image()->b;
	lv*r=dr.trans||dr.omask?buffer_copy(back,d):lmbuff((pair){d.w,d.h});
	cstate t=frame;frame=draw_buffer(r);
	if(dr.trans){rect c=dr.sel_start;draw_rect((rect){c.x-d.x,c.y-d.y,c.w,c.h},dr.fill);}
	draw_limbo(frame.clip,0);
	frame=t;return r;
}
void bg_edit_sel(void){
	if(!dr.limbo)return;
	rect d=dr.sel_here;
	lv*back=con_image()->b,*r=lmd();
	edit_target(r);
	dset(r,lmistr("type"  ),lmn(edit_bg_block));
	dset(r,lmistr("pos"   ),lmrect(d));
	dset(r,lmistr("before"),buffer_copy(back,d));
	dset(r,lmistr("after" ),bg_scaled_limbo());
	dr.limbo=NULL,dr.limbo_dither=0;
	if(dr.sel_start.w>0||dr.sel_start.h>0){
		lv*after=lmbuff((pair){dr.sel_start.w,dr.sel_start.h});
		memset(after->sv,dr.fill,after->c);
		dset(r,lmistr("clr_pos"   ),lmrect(dr.sel_start));
		dset(r,lmistr("clr_before"),buffer_copy(back,dr.sel_start));
		dset(r,lmistr("clr_after" ),after);
		dr.sel_start=(rect){0,0,0,0};
	}
	edit(r);
}
lv* bg_copy_selection(rect s){lv*bg=container_image(con(),1)->b;return buffer_copy(bg,s);}
void bg_scoop_selection(void){if(dr.limbo)return;dr.sel_start=dr.sel_here;dr.limbo=bg_copy_selection(dr.sel_start),dr.limbo_dither=0;}
void bg_draw_lasso(rect r,rect o,int show_ants,int fill){
	if(dr.omask){
		pair s=buff_size(dr.omask);
		for(int a=0;a<s.y;a++)for(int b=0;b<s.x;b++)if(inclip(b+o.x,a+o.y)&&dr.omask->sv[b+a*s.x])PIX(b+o.x,a+o.y)=fill;
	}
	if(dr.mask){
		pair s=buff_size(dr.mask),ls=buff_size(dr.limbo);
		for(int a=0;a<s.y;a++)for(int b=0;b<s.x;b++)if(inclip(b+r.x,a+r.y)&&dr.mask->sv[b+a*s.x]){
			int c=show_ants&&ANTS==(0xFF&dr.mask->sv[b+a*s.x]),p=c?ANTS:dr.limbo->sv[b+a*ls.x];if(p||!dr.trans)PIX(b+r.x,a+r.y)=p;
		}
	}
}
void bg_lasso_preview(void){
	if(!bg_has_lasso())return;
	pair d={ev.pos.x-ev.dpos.x,ev.pos.y-ev.dpos.y};
	rect dh={dr.sel_here.x+d.x,dr.sel_here.y+d.y,dr.sel_here.w,dr.sel_here.h};
	pair dd={ev.pos.x-dh.x,ev.pos.y-dh.y}; int insel=dr.mask!=NULL&&box_in(dh,ev.pos)&&dr.mask->sv[dd.x+dd.y*dh.w];
	rect r=ev.drag&&insel?dh:dr.sel_here;pair origin=con_to_screen((pair){0,0});
	if(dr.fatbits){
		rect o=dr.sel_start;char*pal=patterns_pal(ifield(deck,"patterns"));
		pair os=buff_size(dr.omask);
		for(int a=0;a<os.y;a++)for(int b=0;b<os.x;b++){
			if(!dr.omask->sv[b+a*os.x])continue;
			draw_rect(rect_add((rect){(b+o.x)*FAT,(a+o.y)*FAT,FAT,FAT},origin),dr.fill);
		}
		for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
			if(!dr.mask->sv[b+a*r.w])continue;
			int v=dr.limbo->sv[b+a*r.w],c=anim_pattern(pal,v,frame_count),pat=draw_pattern(pal,c,r.x+b,r.y+a);
			if(c||!dr.trans)draw_rect(rect_add((rect){(b+r.x)*FAT,(a+r.y)*FAT,FAT,FAT},origin),c>=32?c: c==0?c: pat?1:32);
		}
		for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
			if((0xFF&dr.mask->sv[b+a*r.w])!=ANTS)continue;
			pair p=pair_add((pair){(b+r.x)*FAT,(a+r.y)*FAT},origin);
			if(b<=    0||!dr.mask->sv[(b-1)+a*r.w])draw_vline(p.x      ,p.y,p.y+FAT-1,ANTS);
			if(b>=r.w-1||!dr.mask->sv[(b+1)+a*r.w])draw_vline(p.x+FAT-1,p.y,p.y+FAT-1,ANTS);
			if(a<=    0||!dr.mask->sv[b+(a-1)*r.w])draw_hline(p.x,p.x+FAT-1,p.y      ,ANTS);
			if(a>=r.h-1||!dr.mask->sv[b+(a+1)*r.w])draw_hline(p.x,p.x+FAT-1,p.y+FAT-1,ANTS);
		}
	}else{bg_draw_lasso(con_to_screenr(r),con_to_screenr(dr.sel_start),1,dr.fill);}
}
void bg_cancel(void){bg_scratch();poly_count=0;}
void bg_tools(void){
	if     (!dr.fatbits&&ev.mu&&ev.alt){dr.fatbits=1;center_fatbits(ev.pos);bg_cancel();return;}
	else if( dr.fatbits&&ev.mu&&ev.alt){dr.fatbits=0;bg_cancel();return;}if(ev.alt)return;
	if(ev.md)pointer_prev=ev.pos;
	if(!dover(con_view_dim()))ev.md=ev.mu=ev.drag=0;
	if(dr.tool==tool_pencil||dr.tool==tool_line||dr.tool==tool_rect||dr.tool==tool_fillrect||dr.tool==tool_ellipse||dr.tool==tool_fillellipse){
		int clear=0;if(!dr.scratch)bg_scratch();
		if(ev.mdown){dr.pattern=ln(iwrite(con_image(),lmpair(ev.pos),NULL));} // pipette
		else if(ev.md){bg_scratch();dr.erasing=ev.rdown||ev.shift;}
		else if(ev.mu||ev.drag){
			cstate t=frame;frame=draw_buffer(dr.scratch);
			if(dr.tool==tool_pencil||dr.erasing){
				draw_line((rect){pointer_prev.x,pointer_prev.y,ev.pos.x,ev.pos.y},dr.brush,dr.erasing?0:bg_pat(),deck);
			}
			else if(dr.tool==tool_line){
				pair b=snap(ev.dpos),t=snap(ev.pos);if(ev.shift){ // snap to isometric angles
					pair d=(pair){t.x-b.x,t.y-b.y};t=b;
					if     (abs(d.x)*4<abs(d.y)){t.y+=d.y;}
					else if(abs(d.y)*4<abs(d.x)){t.x+=d.x;}
					else if(abs(d.x)*2<abs(d.y)){t.x+=SIGN(d.x)*abs(d.y)/2;t.y+=d.y;}
					else if(abs(d.y)*2<abs(d.x)){t.y+=SIGN(d.y)*abs(d.x)/2;t.x+=d.x;}
					else {t.x+=SIGN(d.x)*abs(MAX(d.x,d.y));t.y+=SIGN(d.y)*abs(MAX(d.x,d.y));}
				}
				bg_scratch_clear();
				draw_line((rect){b.x,b.y,t.x,t.y},dr.brush,bg_pat(),deck);
			}
			else if(dr.tool==tool_rect||dr.tool==tool_fillrect){
				pair b=snap(ev.dpos),a=snap(ev.pos);
				pair t={a.x-b.x,a.y-b.y};if(ev.shift){t.x=t.y=MAX(t.x,t.y);} // snap to square
				bg_scratch_clear();rect r=normalize_rect((rect){b.x,b.y,t.x,t.y});r.w++,r.h++;
				if(dr.tool==tool_fillrect)draw_rect(r,bg_fill());draw_box_fancy(r,dr.brush,bg_pat(),deck);
			}
			else if(dr.tool==tool_ellipse||dr.tool==tool_fillellipse){
				pair b=snap(ev.dpos),a=snap(ev.pos);
				pair t={a.x-b.x,a.y-b.y};if(ev.shift){t.x=t.y=MAX(t.x,t.y);} // snap to circle
				bg_scratch_clear();rect r=normalize_rect((rect){b.x+1,b.y+1,t.x,t.y});r.w--,r.h--;
				#define circ_r(a) (fpair){(int)(c.x+(0.5+r.w/2.0)*cos(a)),(int)(c.y+(0.5+r.h/2.0)*sin(a))}
				#define circ(a)   circ_r(((2*3.141592653589793)/divs)*(a))
				#define divs      100
				poly_count=0;fpair c={r.x+(r.w/2.0),r.y+(r.h/2.0)};for(int z=0;z<=divs;z++)poly_push(circ(z));
				if(dr.tool==tool_fillellipse)draw_poly(bg_fill());
				for(int z=0;z<poly_count-1;z++)draw_line(rect_pair(pcast(poly[z]),pcast(poly[z+1])),dr.brush,bg_pat(),deck);
			}
			bg_scratch_under(),frame=t;if(ev.mu)bg_edit(),clear=1;
		}
		if(dr.scratch){
			if(dr.fatbits){draw_fat(con_clip(),dr.scratch,patterns_pal(ifield(deck,"patterns")),frame_count,BG_MASK,FAT,dr.offset);}
			else{buffer_overlay(frame.buffer,dr.scratch,BG_MASK,con_offset());}
		}
		if(clear)bg_scratch_clear();
	}
	if(dr.tool==tool_lasso){
		pair d={ev.pos.x-ev.dpos.x,ev.pos.y-ev.dpos.y};
		rect dh={dr.sel_here.x+d.x,dr.sel_here.y+d.y,dr.sel_here.w,dr.sel_here.h};
		pair dd={ev.pos.x-dh.x,ev.pos.y-dh.y}; int insel=dr.mask!=NULL&&over(dh)&&dr.mask->sv[dd.x+dd.y*dh.w];
		if(ev.md&&!insel){bg_lasso_preview();bg_end_lasso(),poly_push((fpair){ev.dpos.x,ev.dpos.y});}
		else if(ev.drag&&!insel&&poly_count>0){fpair l=poly[poly_count-1];if(ev.pos.x!=l.x||ev.pos.y!=l.y)poly_push((fpair){ev.pos.x,ev.pos.y});}
		else if(ev.mu&&insel){dr.sel_here.x+=d.x,dr.sel_here.y+=d.y;}
		else if(ev.mu){
			rect r=poly_bounds();if(r.w>1&&r.h>1){
				dr.mask=lmbuff((pair){r.w,r.h});
				cstate t=frame;frame=draw_buffer(dr.mask);
				for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++)if(poly_in((fpair){b+r.x,a+r.y}))PIX(b,a)=1;
				for(int z=0;z<poly_count;z++)draw_line_simple((rect){poly[z].x-r.x,poly[z].y-r.y,poly[(z+1)%poly_count].x-r.x,poly[(z+1)%poly_count].y-r.y},0,ANTS);
				frame=t;dr.sel_here=dr.sel_start=r;
				dr.omask=buffer_clone(dr.mask);EACH(z,dr.omask)dr.omask->sv[z]=dr.omask->sv[z]!=0;
				bg_scoop_selection();
			}poly_count=0;
		}
		if(dr.mask&&dr.limbo){
			if(ev.dir==dir_left ){ev.dir=dir_none;dr.sel_here.x--;}
			if(ev.dir==dir_up   ){ev.dir=dir_none;dr.sel_here.y--;}
			if(ev.dir==dir_right){ev.dir=dir_none;dr.sel_here.x++;}
			if(ev.dir==dir_down ){ev.dir=dir_none;dr.sel_here.y++;}
			if(ev.exit){dr.limbo=dr.mask=dr.omask=NULL;bg_end_lasso();}
		}
	}
	if(dr.tool==tool_poly){
		if(ev.md){poly_count=0;poly_push((fpair){ev.dpos.x,ev.dpos.y});}
		else if(ev.drag&&poly_count>0){fpair l=poly[poly_count-1];if(ev.pos.x!=l.x||ev.pos.y!=l.y)poly_push((fpair){ev.pos.x,ev.pos.y});}
		else if(ev.mu){
			poly_push((fpair){ev.dpos.x,ev.dpos.y});
			bg_scratch();cstate t=frame;frame=draw_buffer(dr.scratch);
			draw_poly(bg_fill());for(int z=0;z<poly_count-1;z++)draw_line(rect_pair(pcast(poly[z]),pcast(poly[z+1])),dr.brush,bg_pat(),deck);
			bg_scratch_under();frame=t;bg_edit();bg_scratch_clear();poly_count=0;
		}
	}
	if(dr.tool==tool_lasso||dr.tool==tool_poly){
		pair o=(pair){dr.sel_here.x-dr.sel_start.x,dr.sel_here.y-dr.sel_start.y};
		int br=dr.tool==tool_lasso?0:dr.brush, pat=dr.tool==tool_lasso?ANTS:bg_pat();
		for(int z=0;z<poly_count-1;z++){
			fpair a=con_to_screenf(poly[z]),b=con_to_screenf(poly[z+1]);
			draw_line((rect){a.x+o.x,a.y+o.y,b.x+o.x,b.y+o.y},br,pat,deck);
		}
	}
	if(dr.tool==tool_fill&&ev.mu){
		bg_scratch();
		cstate t=frame;frame=draw_buffer(dr.scratch);
		lv*bg=container_image(con(),1);
		draw_fill(ev.pos,ev.rup?0:bg_pat(),bg->b->sv);
		frame=t;bg_edit();bg_scratch_clear();
	}
	if(!bg_has_sel()&&!bg_has_lasso()){
		if(dr.fatbits){
			if(ev.dir==dir_left )dr.offset.x-=ev.shift?dr.grid_size.x:1;
			if(ev.dir==dir_right)dr.offset.x+=ev.shift?dr.grid_size.x:1;
			if(ev.dir==dir_up   )dr.offset.y-=ev.shift?dr.grid_size.y:1;
			if(ev.dir==dir_down )dr.offset.y+=ev.shift?dr.grid_size.y:1;
			clamp_fatbits();if(ev.exit)dr.fatbits=0,ev.exit=0;
		}else{tracking();}
	}
}
void bg_end_lasso(void){
	if(uimode!=mode_draw||dr.tool!=tool_lasso)return;
	int data=dr.mask&&dr.limbo, diffrect=!rect_same(dr.sel_here,dr.sel_start), diffmask=dr.omask==NULL||(dr.mask&&dr.omask->c!=dr.mask->c);
	if(dr.omask&&!diffmask)for(int z=0;data&&z<dr.mask->c;z++)if((dr.mask->sv[z]>0)!=(dr.omask->sv[z]>0)){diffmask=1;break;}
	if(data&&(diffrect||diffmask||dr.lasso_dirty)){
		bg_scratch();cstate t=frame;frame=draw_buffer(dr.scratch);
		bg_draw_lasso(dr.sel_here,dr.sel_start,0,dr.fill);frame=t;bg_edit();bg_scratch_clear();
	}poly_count=0,dr.mask=NULL,dr.omask=NULL,dr.limbo=NULL,dr.lasso_dirty=0,dr.sel_here=dr.sel_start=(rect){0};
}
void bg_end_selection(void){
	if(uimode!=mode_draw||dr.tool!=tool_select)return;
	if(dr.sel_here.w<=0&&dr.sel_here.h<=0)return;
	bg_edit_sel();
	dr.sel_start=dr.sel_here=(rect){ev.dpos.x,ev.dpos.y,0,0};
}
void bg_delete_selection(void){
	if(bg_has_lasso()){
		dr.mask=NULL;
		bg_scratch();cstate t=frame;frame=draw_buffer(dr.scratch);
		bg_draw_lasso(dr.sel_here,dr.sel_start,0,dr.fill);frame=t;bg_edit();bg_scratch_clear();
		bg_end_lasso();return;
	}
	if(!bg_has_sel())return;
	if(dr.limbo!=NULL&&dr.sel_start.w<=0&&dr.sel_start.h<=0){dr.sel_here=(rect){0,0,0,0};dr.limbo=NULL,dr.limbo_dither=0;return;}
	if(dr.sel_start.w<=0&&dr.sel_start.h<=0){dr.sel_start=dr.sel_here;}
	dr.sel_here=(rect){0,0,0,0};dr.limbo=lmbuff((pair){1,1}),dr.limbo_dither=0;bg_edit_sel();
	dr.sel_start=dr.sel_here=(rect){ev.dpos.x,ev.dpos.y,0,0};
}
void bg_paste(lv*b,int fit){
	rect clip=con_dim();pair s=buff_size(b);fpair f={clip.w*.75,clip.h*.75};
	if(fit&&(s.x>f.x||s.y>f.y)){float scale=MIN(f.x/s.x,f.y/s.y);s=(pair){s.x*scale,s.y*scale};}if(!s.x)return;
	if(bg_has_sel()){bg_scoop_selection();dr.limbo=b,dr.limbo_dither=0;}
	else{settool(tool_select);dr.sel_start=(rect){0,0,0,0};dr.sel_here=box_center(con_view_dim(),s);dr.limbo=b,dr.limbo_dither=0;}
}
rect keep_ratio(rect r,pair s){
	if(!ev.shift||s.x==0||s.y==0)return r;
	float scale=MAX(r.w/(s.x*1.0),r.h/(s.y*1.0));return (rect){r.x,r.y,scale*s.x,scale*s.y};
}
int handle_size(void){return enable_touch?10:5;}
void draw_handles(rect r){
	char*pal=patterns_pal(ifield(deck,"patterns"));
	int h=handle_size();
	int x0=r.x+1-h, x2=r.x+r.w-1, x1=(x2-x0)/2+x0;
	int y0=r.y+1-h, y2=r.y+r.h-1, y1=(y2-y0)/2+y0;
	draw_invert(pal,box_intersect((rect){x0,y0,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x0,y2,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x2,y0,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x2,y2,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x2,y1,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x1,y2,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x1,y0,h,h},frame.clip));
	draw_invert(pal,box_intersect((rect){x0,y1,h,h},frame.clip));
}
int in_handle(rect r){
	int h=handle_size();
	int x0=r.x+1-h, x2=r.x+r.w-1, x1=(x2-x0)/2+x0;
	int y0=r.y+1-h, y2=r.y+r.h-1, y1=(y2-y0)/2+y0;
	if(over((rect){x0,y0,h,h}))return 4;
	if(over((rect){x0,y2,h,h}))return 6;
	if(over((rect){x2,y0,h,h}))return 2;
	if(over((rect){x2,y2,h,h}))return 0;
	if(over((rect){x2,y1,h,h}))return 1;
	if(over((rect){x1,y0,h,h}))return 3;
	if(over((rect){x0,y1,h,h}))return 5;
	if(over((rect){x1,y2,h,h}))return 7;
	return -1;
}
rect bg_select(void){
	if(uimode!=mode_draw||dr.tool!=tool_select)return (rect){0,0,0,0};
	rect s=dr.sel_here; int has_sel=s.w>0||s.h>0, in_sel=has_sel&&dover(s);
	int ax=MIN(ev.dpos.x,ev.pos.x), bx=MAX(ev.dpos.x,ev.pos.x);
	int ay=MIN(ev.dpos.y,ev.pos.y), by=MAX(ev.dpos.y,ev.pos.y);
	int h=handle_size();
	int x0=s.x+1-h, x2=s.x+s.w-1, x1=(x2-x0)/2+x0;
	int y0=s.y+1-h, y2=s.y+s.h-1, y1=(y2-y0)/2+y0;
	int dx=ev.pos.x-ev.dpos.x;
	int dy=ev.pos.y-ev.dpos.y;
	pair sz=dr.limbo?buff_size(dr.limbo):(pair){dr.sel_start.w,dr.sel_start.h};
	#define handle(rw,rh,ox,oy,ow,oh) (has_sel&&(ev.mu||ev.drag)&&dover((rect){rw,rh,h,h}))\
	        {s=normalize_rect(keep_ratio((rect){s.x+ox,s.y+oy,s.w+ow,s.h+oh},sz));if(ev.mu)dr.sel_here=snapr(s);bg_scoop_selection();uicursor=cursor_drag;}
	if(in_layer()){
		if      handle(x2,y2,  0, 0, dx, dy) // se
		else if handle(x0,y2, dx, 0,-dx, dy) // sw
		else if handle(x2,y0,  0,dy, dx,-dy) // ne
		else if handle(x0,y0, dx,dy,-dx,-dy) // nw
		else if handle(x2,y1,  0, 0, dx,  0) // e
		else if handle(x1,y0,  0,dy,  0,-dy) // n
		else if handle(x0,y1, dx, 0,-dx,  0) // w
		else if handle(x1,y2,  0, 0,  0, dy) // s
		else if(ev.md&&in_sel){
			// begin move selection
			bg_scoop_selection();
		}
		else if((ev.mu||ev.drag)&&in_sel){
			// move selection
			s.x+=dx, s.y+=dy;
			if(ev.mu)dr.sel_here=snapp(s); // finish
		}
		else if(ev.md&&!in_sel){
			// begin create selection
			if(has_sel){draw_limbo(con_to_screenr(s),dr.fatbits),bg_end_selection(),has_sel=0;}s=dr.sel_here;
		}
		else if(ev.mu||ev.drag){
			// size selection
			s=snapr((rect){ax,ay,bx-ax,by-ay});
			if(ev.mu)dr.sel_here=s; // finish
		}
	}
	if(has_sel){draw_limbo(con_to_screenr(s),dr.fatbits);}
	if(in_layer()){
		int nudge=0;
		if(has_sel&&ev.dir==dir_left ){ev.dir=dir_none;bg_scoop_selection();dr.sel_here.x-=ev.shift?dr.grid_size.x:1;nudge=1;}
		if(has_sel&&ev.dir==dir_up   ){ev.dir=dir_none;bg_scoop_selection();dr.sel_here.y-=ev.shift?dr.grid_size.y:1;nudge=1;}
		if(has_sel&&ev.dir==dir_right){ev.dir=dir_none;bg_scoop_selection();dr.sel_here.x+=ev.shift?dr.grid_size.x:1;nudge=1;}
		if(has_sel&&ev.dir==dir_down ){ev.dir=dir_none;bg_scoop_selection();dr.sel_here.y+=ev.shift?dr.grid_size.y:1;nudge=1;}
		if(nudge&&ev.shift)dr.sel_here=snapp(dr.sel_here);
		if(ev.exit){dr.limbo=NULL,dr.limbo_dither=0;bg_end_selection();}
	}return s;
}
void bg_mask_set(pair p,int v){dr.mask->sv[p.x+p.y*dr.sel_here.w]=v;}
int bg_mask_get(pair p){return (p.x<0||p.y<0||p.x>=dr.sel_here.w||p.y>=dr.sel_here.h)?0:dr.mask->sv[p.x+p.y*dr.sel_here.w];}
void bg_box_to_lasso(void){
	rect r=dr.sel_here;
	if(dr.tool==tool_select){
		dr.tool=tool_lasso;bg_scoop_selection();rect s=dr.sel_start;
		lv*l=dr.limbo;dr.limbo=lmbuff((pair){r.w,r.h});cstate t=frame;frame=draw_buffer(dr.limbo);
		if(dr.limbo_dither){draw_dithered(frame.clip,l,1,dr.omask),dr.limbo_dither=0;}else{draw_scaled(frame.clip,l,1);}frame=t;
		dr.mask=lmbuff((pair){r.w,r.h}),memset(dr.mask->sv,1,r.w*r.h);
		if(s.w>0&&s.h>0){dr.omask=lmbuff((pair){s.w,s.h}),memset(dr.omask->sv,1,s.w*s.h);}else{dr.omask=NULL;}
	}
}
void bg_regenerate_lasso_outline(void){
	rect r=dr.sel_here;
	for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++)if(bg_mask_get((pair){b,a})){
		int n=bg_mask_get((pair){b-1,a})&&bg_mask_get((pair){b,a-1})&&bg_mask_get((pair){b+1,a})&&bg_mask_get((pair){b,a+1});
		if(!n)bg_mask_set((pair){b,a},ANTS);
	}
}
void bg_tighten(void){
	rect r=dr.sel_here;
	bg_box_to_lasso();
	int changed=1,background=dr.fill;while(changed){changed=0; // erode the mask, iterating to a fixed point
		for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++)if(bg_mask_get((pair){b,a})&&dr.limbo->sv[b+a*r.w]==background){
			int n=bg_mask_get((pair){b-1,a})&&bg_mask_get((pair){b,a-1})&&bg_mask_get((pair){b+1,a})&&bg_mask_get((pair){b,a+1});
			if(!n)bg_mask_set((pair){b,a},0),changed=1;
		}
	}
	bg_regenerate_lasso_outline();
	rect d=find_occupied(dr.mask,0);if(d.w<1||d.h<1||(d.w==r.w&&d.h==r.h))return; // trim excess?
	dr.limbo=buffer_copy(dr.limbo,d);if(dr.mask)dr.mask=buffer_copy(dr.mask,d);
	dr.sel_here .x+=d.x,dr.sel_here .y+=d.y,dr.sel_here .w=d.w,dr.sel_here .h=d.h;
}
void bg_outline(void){
	rect r=dr.sel_here;
	bg_box_to_lasso();
	EACH(z,dr.mask)if((0xFF&dr.mask->sv[z])==ANTS)dr.mask->sv[z]=1; // strip the ANTS outline
	lv*l=buffer_clone(dr.limbo);for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
		int i=b+a*r.w, n=0;if(dr.limbo->sv[i]||!dr.mask->sv[i])continue;
		if(b>0    ){int i=(b-1)+(a  )*r.w; n|=dr.limbo->sv[i]&&dr.mask->sv[i];}
		if(b<r.w-1){int i=(b+1)+(a  )*r.w; n|=dr.limbo->sv[i]&&dr.mask->sv[i];}
		if(a>0    ){int i=(b  )+(a-1)*r.w; n|=dr.limbo->sv[i]&&dr.mask->sv[i];}
		if(a<r.h-1){int i=(b  )+(a+1)*r.w; n|=dr.limbo->sv[i]&&dr.mask->sv[i];}
		if(n)l->sv[i]=bg_pat();
	}dr.limbo=l,dr.lasso_dirty=1;bg_regenerate_lasso_outline();
}

// Object Edit Mode

int ob_by_index(const void*av,const void*bv){int a=ln(ifield((*((lv**)av)),"index")),b=ln(ifield((*((lv**)bv)),"index"));return a-b;}
void ob_order(void){qsort(ob.sel->lv,ob.sel->c,sizeof(lv*),ob_by_index);}
void ob_move_up(void){if(ob.sel->c<1)return;ob_order();EACHR(z,ob.sel){lv*o=ob.sel->lv[z];iwrite(o,lmistr("index"),lmn(ln(ifield(o,"index"))+1));}}
void ob_move_dn(void){if(ob.sel->c<1)return;ob_order();EACH( z,ob.sel){lv*o=ob.sel->lv[z];iwrite(o,lmistr("index"),lmn(ln(ifield(o,"index"))-1));}}

void ob_edit_prop(char*key,lv*value){
	lv*before=lmd(),*after=lmd();EACH(z,ob.sel){
		lv*w=ob.sel->lv[z],*n=ifield(w,"name");
		lv*bp=lmd();dset(bp,lmistr(key),ifield(w,key));
		lv*ap=lmd();dset(ap,lmistr(key),value);
		dset(before,n,bp),dset(after,n,ap);
	}lv*r=lmd();
	dset(r,lmistr("type"  ),lmn(edit_ob_props));
	dset(r,lmistr("before"),before);
	dset(r,lmistr("after" ),after);
	edit_target(r);
	edit(r);
}
void ob_create(lv*props){
	lv*r=lmd();
	dset(r,lmistr("type" ),lmn(edit_ob_create));
	dset(r,lmistr("props"),props);
	edit_target(r);
	edit(r);
}
void ob_destroy(void){
	if(ob.sel->c<1)return;lv*r=lmd();
	lv*props=lml(0);EACH(z,ob.sel){lv*p=lmd();dset(p,lmistr("name"),ifield(ob.sel->lv[z],"name"));ll_add(props,p);}
	dset(r,lmistr("type" ),lmn(edit_ob_destroy));
	dset(r,lmistr("props"),props);
	edit_target(r);
	edit(r);ob.sel->c=0;
}
int can_coalesce(int move){
	if(has_redo()||doc_hist->c==0)return 0; // must be at the leading edge, must be a preceding operation
	lv*prev=doc_hist->lv[doc_hist_cursor-1];
	if(ln(dget(prev,lmistr("type")))!=edit_ob_props)return 0; // preceding op must be the same type
	lv*c=con(); // preceding op must target widgets in the same container(!)
	{lv*v=dget(prev,lmistr("def" ));if(v){if(!prototype_is(c)||!matchr(ifield(c,"name" ),v))return 0;}}
	{lv*v=dget(prev,lmistr("card"));if(v){if(!card_is(c)     ||!matchr(ifield(c,"index"),v))return 0;}}
	lv*after=dget(prev,lmistr("after"));
	if(after->c!=ob.sel->c)return 0; // item count must match
	EACH(z,ob.sel){
		if(!matchr(ifield(ob.sel->lv[z],"name"),after->kv[z]))return 0; // item names must match (including order!)
		if(after->lv[z]->c!=(move?1:2))return 0;
		if(       !matchr(after->lv[z]->kv[0],lmistr("pos" )))return 0;
		if(!move&&!matchr(after->lv[z]->kv[1],lmistr("size")))return 0;
	}return 1;
}
void ob_move(pair delta,int coalesce){
	if(ob.sel->c<1)return;lv*key=lmistr("pos");
	if(coalesce&&can_coalesce(1)){
		lv*r=doc_hist->lv[doc_hist_cursor-1],*after=dget(r,lmistr("after"));EACH(z,after){
			pair a=getpair(dget(after->lv[z],key));dset(after->lv[z],key,lmpair((pair){a.x+delta.x,a.y+delta.y}));
		}undo(),redo();
	}else{
		lv*before=lmd(),*after=lmd();EACH(z,ob.sel){
			lv*w=ob.sel->lv[z],*n=ifield(w,"name");
			lv*f=ifield(w,"pos");pair fv=getpair(f);
			lv*bp=lmd();dset(bp,key,f);
			lv*ap=lmd();dset(ap,key,lmpair((pair){fv.x+delta.x,fv.y+delta.y}));
			dset(before,n,bp),dset(after,n,ap);
		}lv*r=lmd();
		dset(r,lmistr("type"  ),lmn(edit_ob_props));
		dset(r,lmistr("before"),before);
		dset(r,lmistr("after" ),after);
		edit_target(r);
		edit(r);
	}
}
void ob_resize(rect size,int coalesce){
	if(ob.sel->c!=1)return;lv*w=ob.sel->lv[0];
	if(coalesce&&can_coalesce(0)){
		lv*r=doc_hist->lv[doc_hist_cursor-1],*after=dget(r,lmistr("after"))->lv[0];
		dset(after,lmistr("pos" ),lmpair((pair){size.x,size.y}));
		dset(after,lmistr("size"),lmpair((pair){size.w,size.h}));undo(),redo();
	}
	else{
		lv*before=lmd(),*after=lmd(),*n=ifield(w,"name"),*r=lmd();
		lv*bp=lmd();dset(bp,lmistr("pos"),ifield(w,"pos")              ),dset(bp,lmistr("size"),ifield(w,"size")             );
		lv*ap=lmd();dset(ap,lmistr("pos"),lmpair((pair){size.x,size.y})),dset(ap,lmistr("size"),lmpair((pair){size.w,size.h}));
		dset(before,n,bp),dset(after,n,ap);
		dset(r,lmistr("type"  ),lmn(edit_ob_props));
		dset(r,lmistr("before"),before);
		dset(r,lmistr("after" ),after );
		edit_target(r);
		edit(r);
	}
}
void proto_prop(lv*target,char*key,lv*value){
	lv*r=lmd();
	dset(r,lmistr("type"  ),lmn(edit_proto_props));
	dset(r,lmistr("keys"  ),l_list(lmcstr(key)));
	dset(r,lmistr("before"),l_list(ifield(target,key)));
	dset(r,lmistr("after" ),l_list(value));
	edit_target(r);
	edit(r);
}
void proto_size(lv*target,pair size,rect margin){
	lv*r=lmd(),*o=image_resize(ifield(target,"image"),getpair(ifield(target,"size"))),*n=image_make(lmbuff(size));
	rect c=rect_pair((pair){0,0},size);draw_9seg(c,n->b,o->b,getrect(ifield(target,"margin")),c,1,NULL);
	dset(r,lmistr("type"  ),lmn(edit_proto_props));
	dset(r,lmistr("keys"  ),lml3(lmistr("size"),lmistr("image"),lmistr("margin")));
	dset(r,lmistr("before"),lml3(ifield(target,"size"),image_clone(o),ifield(target,"margin")));
	dset(r,lmistr("after" ),lml3(lmpair(size),n,lmrect(margin)));
	edit_target(r);
	edit(r);
}
void object_select(lv*x){ob.sel->c=0;ll_add(ob.sel,x);}
void object_properties(lv*x){
	object_select(x);
	if(button_is     (x))modal_enter(modal_button_props     );
	if(field_is      (x))modal_enter(modal_field_props      );
	if(slider_is     (x))modal_enter(modal_slider_props     );
	if(canvas_is     (x))modal_enter(modal_canvas_props     );
	if(grid_is       (x))modal_enter(modal_grid_props       );
	if(contraption_is(x))modal_enter(modal_contraption_props);
}
int is_resizable(void){return ob.sel->c==1&&(contraption_is(ob.sel->lv[0])?lb(ifield(ifield(ob.sel->lv[0],"def"),"resizable")):1);}
void object_editor(void){
	char*pal=patterns_pal(ifield(deck,"patterns"));
	lv*wids=con_wids();EACH(z,wids){ // forward pass for rendering
		lv*wid=wids->lv[z];widget w=unpack_widget(wid);w.size=con_to_screenr(w.size);
		int sel=0;EACH(z,ob.sel)if(ob.sel->lv[z]==wid){sel=1;break;}
		if(sel){draw_box(inset(w.size,-1),0,ANTS);}else if(ob.show_bounds){draw_boxinv(pal,inset(w.size,-1));}
		if(sel&&is_resizable()){draw_handles(w.size);}
		if(ob.show_bounds){
			if(lb(ifield(wid,"volatile"))){
				draw_line((rect){w.size.x,w.size.y,w.size.x+w.size.w,w.size.y+w.size.h},0,1,deck);
				draw_line((rect){w.size.x+w.size.w,w.size.y,w.size.x,w.size.y+w.size.h},0,1,deck);
			}
			rect badge={w.size.x+w.size.w-10,w.size.y,10,10};
			if(w.locked                  )draw_rect(badge,1),draw_icon((pair){badge.x+1,badge.y+1},LOCK,32),badge.y+=10;
			if(lb(ifield(wid,"animated")))draw_rect(badge,1),draw_icon((pair){badge.x+1,badge.y+1},ANIM,32);
		}
	}
	if(!in_layer())return;
	if(ob.sel->c>0){
		int nudge=0;
		if(ev.dir==dir_left )ob_move((pair){-1*(ev.shift?dr.grid_size.x:1), 0},1),nudge=1;
		if(ev.dir==dir_right)ob_move((pair){ 1*(ev.shift?dr.grid_size.x:1), 0},1),nudge=1;
		if(ev.dir==dir_up   )ob_move((pair){ 0,-1*(ev.shift?dr.grid_size.y:1)},1),nudge=1;
		if(ev.dir==dir_down )ob_move((pair){ 0, 1*(ev.shift?dr.grid_size.y:1)},1),nudge=1;
		if(nudge&&ev.shift){
			pair p={RTEXT_END,RTEXT_END};EACH(z,ob.sel){p=pair_min(p,getpair(ifield(ob.sel->lv[z],"pos")));}
			if(ob.sel->c)ob_move(snap_delta(p),1);
		}
	}
	int ish=is_resizable()?in_handle(unpack_widget(ob.sel->lv[0]).size):-1;
	int isw=0;EACH(w,wids){rect wid=unpack_widget(wids->lv[w]).size;EACH(z,ob.sel)if(ob.sel->lv[z]==wids->lv[w]&&over(wid))isw=1;}
	pair a=ev.pos,b=ob.prev;int dragged=a.x!=b.x||a.y!=b.y;
	rect sr=normalize_rect((rect){ev.dpos.x,ev.dpos.y,ev.pos.x-ev.dpos.x,ev.pos.y-ev.dpos.y});int box=sr.w>1||sr.h>1;
	if(isw&&ev.dclick){
		for(int z=wids->c-1;z>=0;z--){
			lv*wid=wids->lv[z];widget w=unpack_widget(wid);
			if(over(w.size)){object_properties(wid);break;}
		}
	}
	else if(isw&&ev.mu&&!box){
		for(int z=ob.sel->c-1;z>=0;z--){
			lv*wid=ob.sel->lv[z];widget w=unpack_widget(wid);
			if(over(w.size)){if(ev.shift){ob.sel=l_drop(wid,ob.sel);}else{ob.sel->c=0;ll_add(ob.sel,wid);}break;}
		}
	}
	else if(ish!=-1&&ev.md){ob.resize=1,ob.resize_first=1,ob.handle=ish,ob.prev=ev.pos;ob.orig=unpack_widget(ob.sel->lv[0]).size;}
	else if(isw    &&ev.md){ob.move  =1,ob.move_first  =1,ob.prev=ev.pos;}
	else if(ob.resize&&(!ev.drag||!ob.sel->c)){ob.resize=0,ob.resize_first=0;}
	else if(ob.move&&(!ev.drag||!ob.sel->c)){
		pair p={RTEXT_END,RTEXT_END};EACH(z,ob.sel){p=pair_min(p,getpair(ifield(ob.sel->lv[z],"pos")));}
		if(ob.sel->c)ob_move(snap_delta(p),!ob.move_first);
		ob.move=0,ob.move_first=0;
	}
	else if(ob.resize){
		pair delta={ev.pos.x-ev.dpos.x,ev.pos.y-ev.dpos.y};rect r=ob.orig;
		if(ob.handle==0){r.w+=delta.x, r.h+=delta.y;}
		if(ob.handle==2){r.w+=delta.x, r.h-=delta.y; r.y+=delta.y;}
		if(ob.handle==6){r.w-=delta.x, r.h+=delta.y; r.x+=delta.x;}
		if(ob.handle==4){r.w-=delta.x, r.h-=delta.y; r.x+=delta.x; r.y+=delta.y;}
		if(ob.handle==1){r.w+=delta.x;}
		if(ob.handle==3){r.h-=delta.y, r.y+=delta.y;}
		if(ob.handle==5){r.w-=delta.x, r.x+=delta.x;}
		if(ob.handle==7){r.h+=delta.y;}
		r=snapr(r), r.w=MAX(8,r.w), r.h=MAX(8,r.h);
		if(dragged){ob_resize(r,!ob.resize_first);ob.resize_first=0,ob.prev=ev.pos;}
	}
	else if(ob.move){
		pair delta=(pair){a.x-b.x,a.y-b.y};
		if(dragged){ob_move(delta,!ob.move_first);ob.move_first=0,ob.prev=ev.pos;}
	}else{
		if(box&&ev.drag)draw_box(con_to_screenr(sr),0,ANTS); if(box&&(ev.drag||ev.mu))ob.sel->c=0;
		int f=0;for(int z=wids->c-1;z>=0;z--){ // backward pass for selection priority(!)
			lv*wid=wids->lv[z];widget w=unpack_widget(wid);
			int sel=0;EACH(z,ob.sel)if(ob.sel->lv[z]==wid){sel=1;break;}
			rect overlap=box_intersect(w.size,sr);
			int insel=box?overlap.w>=1&&overlap.h>=1: over(w.size)&&dover(w.size), c=ev.mu&insel;
			if(c)f=1; if(!c)continue; if(box){ll_add(ob.sel,wid);continue;}
			if(!sel){if(!ev.shift)ob.sel->c=0;ll_add(ob.sel,wid);break;}
		}if(ev.mu&&!f)ob.sel->c=0;
	}
}
int prototype_size_editor(void){
	lv*def=con();if(uimode==mode_interact||!prototype_is(def))return 0;int r=0;
	rect m=getrect(ifield(def,"margin")),om=m,drag={0,0,0,0};pair s=getpair(ifield(def,"size"));
	pair delta=pair_sub(screen_to_con(ev.pos),screen_to_con(ev.dpos));int resize=0;
	rect view=con_clip();if(view.x!=0&&view.y!=0&&view.w!=frame.size.x&&view.h!=frame.size.y&&uimode!=mode_draw){
		// resize handle
		char l[4096]={0};rect sh=rect_pair(pair_add(con_to_screen(s),(pair){1,1}),(pair){8,8});
		if(over(sh))r=1;
		if((ev.drag||ev.mu)&&dover(sh)&&(delta.x!=0||delta.y!=0)){
			s=snap(pair_max((pair){1,1},pair_add(s,delta))),sh=rect_pair(pair_add(con_to_screen(s),(pair){1,1}),(pair){8,8});
			draw_box(con_to_screenr(rect_pair((pair){0,0},s)),0,ANTS),r=1;if(ev.mu)resize=1;
		}
		draw_rect(sh,32),draw_box(sh,0,1);if(over(sh))uicursor=cursor_drag;
		snprintf(l,sizeof(l),"%d x %d",s.x,s.y),draw_text((rect){sh.x+10,sh.y+10,100,20},l,FONT_MONO,1);
		// margin handles
		#define h_margin(i,v,o)     rect h=rect_pair(pair_sub(con_to_screen(v),o),image_size(i))
		#define m_margin(i,v,o,a,d) {h_margin(i,v,o);if(over(h)||dover(h))r=1;if((ev.drag||ev.mu)&&dover(h))m.d+=a,drag.d=1;}
		#define i_margin(h,i)       {buffer_paste(h,frame.clip,i->b,frame.buffer,0);if(over(h))uicursor=cursor_drag;}
		#define l_label(h,v)        {snprintf(l,sizeof(l),"%d",v);pair t=font_textsize(FONT_MONO,l);draw_text((rect){h.x-2-t.x    ,h.y-3  ,60,20},l,FONT_MONO,1);}
		#define t_label(h,v)        {snprintf(l,sizeof(l),"%d",v);pair t=font_textsize(FONT_MONO,l);draw_text((rect){h.x+3-(t.x/2),h.y-t.y,60,20},l,FONT_MONO,1);}
		#define l_margin(i,v,o,d)   {h_margin(i,v,o);if(drag.d)l_label(h,m.d);i_margin(h,i)}
		#define t_margin(i,v,o,d)   {h_margin(i,v,o);if(drag.d)t_label(h,m.d);i_margin(h,i)}
		if(ob.show_margins){
			m_margin(HANDLES[0],((pair){m.x    ,0      }),((pair){3,11}), delta.x,x);
			m_margin(HANDLES[1],((pair){0      ,m.y    }),((pair){11,3}), delta.y,y);
			m_margin(HANDLES[0],((pair){s.x-m.w,0      }),((pair){3,11}),-delta.x,w);if(drag.x&&drag.w)m.w+=delta.x,drag.w=0;
			m_margin(HANDLES[1],((pair){0      ,s.y-m.h}),((pair){11,3}),-delta.y,h);if(drag.y&&drag.h)m.h+=delta.y,drag.h=0;
			m=getrect(normalize_margin(lmrect(m),getpair(ifield(def,"size"))));
			t_margin(HANDLES[0],((pair){m.x    ,0      }),((pair){3,11}),x);
			l_margin(HANDLES[1],((pair){0      ,m.y    }),((pair){11,3}),y);
			t_margin(HANDLES[0],((pair){s.x-m.w,0      }),((pair){3,11}),w);
			l_margin(HANDLES[1],((pair){0      ,s.y-m.h}),((pair){11,3}),h);
		}
	}
	if(ob.show_margins){
		rect m0=con_to_screenr((rect){m.x,0,1,s.y    });draw_vline(m0.x,m0.y,m0.y+m0.h,ANTS);
		rect m1=con_to_screenr((rect){0,m.y,s.x,1    });draw_hline(m1.x,m1.x+m1.w,m1.y,ANTS);
		rect m2=con_to_screenr((rect){s.x-m.w,0,1,s.y});draw_vline(m2.x,m2.y,m2.y+m2.h,ANTS);
		rect m3=con_to_screenr((rect){0,s.y-m.h,s.x,1});draw_hline(m3.x,m3.x+m3.w,m3.y,ANTS);
	}
	int move_margin=m.x!=om.x||m.y!=om.y||m.w!=om.w||m.h!=om.h;
	if(resize)proto_size(def,s,m);
	else if(ev.mu&&move_margin)proto_prop(def,"margin",lmrect(m));
	return r||resize||move_margin;
}

// Audio

#define SFX_SLOTS 4
typedef struct {lv*clip; Uint32 sample; float volume;}clip_state;
clip_state audio_loop={0};lv*orig_loop=NULL;
clip_state audio_slots[SFX_SLOTS]={{0}};
float master_volume=1.0;

void sfx_install(lv*sfx,clip_state*target){
	lv*c=lms(sfx->b->c);memcpy(c->sv,sfx->b->sv,c->c); // clone the buffer so it can't be rewritten during playback (!)
	target->clip=c,target->sample=0,target->volume=1.0;
}
void sfx_doloop(int clear){
	interpreter_lock();
	lv*a=orig_loop?orig_loop:LNIL,*b=lmblk(),*r=LNIL;
	blk_get(b,lmistr("loop")),blk_lit(b,l_list(a)),blk_op(b,CALL);
	int pp=pending_popstate;fire_hunk_async(ifield(deck,"card"),b);
	int quota=LOOP_QUOTA;while(quota>0&&running()){runop(),quota--;}
	if(!running())r=arg();popstate();pending_popstate=pp;
	if(clear)n_play(deck,lml2(LNIL,lmistr("loop")));
	n_play(deck,lml2(r,lmistr("loop"))),msg.pending_loop=0;
	lv*loop=audio_loop.clip;if(loop&&audio_loop.sample>=((Uint32)loop->c))audio_loop.sample=0;
	interpreter_unlock();
}
void sfx_pump(void*user,Uint8*stream,int len){
	if(msg.pending_loop)sfx_doloop(0);
	(void)user;int play=0;for(int z=0;z<len;z++){
		float samples=0;for(int z=0;z<SFX_SLOTS;z++){
			if(audio_slots[z].clip==NULL)continue;play=1;
			int8_t*data=(int8_t*)audio_slots[z].clip->sv; // stored as uint, but samples are signed(!)
			int b=data[audio_slots[z].sample];
			samples+=(b/128.0)*audio_slots[z].volume;
			audio_slots[z].sample++;
			if(audio_slots[z].sample>=((Uint32)audio_slots[z].clip->c))audio_slots[z].clip=NULL;
		}
		lv*loop=audio_loop.clip;
		if(loop&&audio_loop.sample>=((Uint32)loop->c)){sfx_doloop(1),loop=audio_loop.clip;}
		if(loop){
			int s=audio_loop.sample;
			int8_t*data=(int8_t*)loop->sv;int b=data[s++];samples+=(b/128.0)*audio_loop.volume;
			audio_loop.sample=s;
		}
		if(au.mode==record_playing){
			int len=au.target->b->c, sel=au.sel.x!=au.sel.y, start=sel?au.sel.x:0, end=sel?au.sel.y:len;
			if(au.head>=end){au.head=start;au.mode=record_stopped;}
			else{int8_t*data=(int8_t*)au.target->b->sv;int b=data[au.head++];samples+=(b/128.0)*1.0;}
		}
		stream[z]=0xFF&(int)((master_volume*(samples/(SFX_SLOTS+1)))*120);
	}audio_playing=play;
}
int sfx_any(void){if(nosound)return 0;for(int z=0;z<SFX_SLOTS;z++)if(audio_slots[z].clip!=NULL)return 1;return 0;}
lv* n_play(lv*deck,lv*z){
	if(z->c>1&&matchr(z->lv[1],lmistr("loop"))){
		lv*x=l_first(z);if(lis(x))x=dget(ifield(deck,"sounds"),x);
		if(orig_loop&&orig_loop==x){} // don't re-trigger!
		else if(sound_is(x)&&ln(ifield(x,"size"))>0){sfx_install(x,&audio_loop),orig_loop=x;} // play
		else{audio_loop.clip=orig_loop=NULL;} // stop the loop
		return LNIL;
	}
	lv*x=l_first(z),*sfx=sound_is(x)?x: dget(ifield(deck,"sounds"),ls(x));if(!sfx)return LNIL;
	if(!sound_is(sfx)||ln(ifield(sfx,"size"))<1)return LNIL;
	int max_sample=0;int avail=-1;for(int z=0;z<SFX_SLOTS;z++){
		if(!audio_slots[z].clip){avail=z;break;}
		if(audio_slots[z].sample>audio_slots[max_sample].sample)max_sample=z;
	}
	sfx_install(sfx,&audio_slots[avail!=-1?avail:max_sample]),audio_playing=1;
	return LNIL;
}

// Toolbars

int toolbtn(pair pos,pair dn,rect b,int icon,int active){
	rect i=box_center(b,(pair){16,16});
	draw_box(b,0,1);if(active)draw_rect(b,1);draw_icon((pair){i.x,i.y},TOOLS->lv[icon],active?0:1);
	if(box_in(b,pos))uicursor=cursor_point;return box_in(b,pos)&&box_in(b,dn)&&ev.mu;
}
int modebtn(pair pos,pair dn,rect b,char*text,int active){
	draw_box(b,0,1);if(active)draw_rect(inset(b,2),1);draw_textc(b,text,FONT_BODY,active?0:1);
	if(box_in(b,pos))uicursor=cursor_point;return box_in(b,pos)&&box_in(b,dn)&&ev.mu;
}
int scrollbtn(pair pos,pair dn,rect b,int icon){
	rect i=box_center(b,(pair){12,12});int active=box_in(b,dn)&&(ev.mu||ev.drag);
	draw_box(b,0,1);if(active)draw_rect(b,1);draw_icon((pair){i.x,i.y},ARROWS->lv[icon],active?0:1);
	if(box_in(b,pos))uicursor=cursor_point;return box_in(b,pos)&&active&&ev.mu;
}
void brushbtn(pair pos,pair dn,rect b,int brush){
	pair i={b.x+(b.w/2),b.y+(b.h/2)};
	draw_box(b,0,1),draw_line((rect){i.x,i.y,i.x,i.y},brush,1,deck);
	if(dr.brush==brush)draw_box(inset(b,2),0,1);
	if(!box_in(b,pos))return;uicursor=cursor_point;if(!ev.mu||!box_in(b,dn))return;setuimode(mode_draw);
	if(dr.tool==tool_select||dr.tool==tool_lasso||dr.tool==tool_fill)settool(tool_pencil);
	dr.brush=brush;
}
void cbrushbtn(pair pos,pair dn,rect b,int brush,lv*bt){
	lv*icon=bt->lv[brush-24];rect oc=frame.clip;frame.clip=b;
	rect p=box_center(b,image_size(icon));buffer_paste(p,b,icon->b,frame.buffer,1);frame.clip=oc;draw_box(b,0,1);
	if(dr.brush==brush)draw_box(inset(b,2),0,1);
	if(!box_in(b,pos))return;uicursor=cursor_point;if(!ev.mu||!box_in(b,dn))return;setuimode(mode_draw);
	if(dr.tool==tool_select||dr.tool==tool_lasso||dr.tool==tool_fill)settool(tool_pencil);
	dr.brush=brush;
}
void palbtn(pair pos,pair dn,rect b,int pattern){
	if((dr.pickfill?dr.fill:dr.pattern)==pattern){draw_rect(inset(b,3),pattern),draw_box(inset(b,3),0,1);}else{draw_rect(b,pattern);}
	if(box_in(b,pos)){uicursor=cursor_point;if(ev.mu&&box_in(b,dn)){if(dr.pickfill){dr.fill=pattern;}else{dr.pattern=pattern;}}}draw_box(b,0,1);
}

// 9 + 6
#define tcellw 22
#define tcellh 19
#define tgap   1
void ltoolbar(pair pos,pair dn){
	lv*bs=dget(deck->b,lmistr("brushes")),*bt=dget(deck->b,lmistr("brusht"));
	toolbar_scroll=CLAMP(0,toolbar_scroll,bs->c);int th=bs->c?17:tcellh;
	pair size=buff_size(TOOLB);frame=draw_buffer(TOOLB);
	memset(frame.buffer->sv,0,frame.buffer->c),draw_box((rect){0,0,size.x,size.y},0,1),draw_rect((rect){0,6*tcellh,size.x,tgap},1);
	if(toolbtn(pos,dn,(rect){0     ,0,tcellw+1,tcellh+1},0,uimode==mode_interact))setuimode(mode_interact),ev.mu=ev.md=0;
	if(toolbtn(pos,dn,(rect){tcellw,0,tcellw+1,tcellh+1},1,uimode==mode_object  ))setuimode(mode_object  ),ev.mu=ev.md=0;
	for(int z=0;z<10;z++){if(toolbtn(pos,dn,(rect){(z%2)*tcellw,(1+(z/2))*tcellh,tcellw+1,tcellh+1},z+2,uimode==mode_draw&&dr.tool==z))settool(z),ev.mu=ev.md=0;}
	int cy=(6*tcellh)+tgap;int brow=0;for(int z=0;z<12-toolbar_scroll;z++){
		brushbtn(pos,dn,(rect){0     ,cy,tcellw+1,th+1},z   +toolbar_scroll);
		brushbtn(pos,dn,(rect){tcellw,cy,tcellw+1,th+1},z+12+toolbar_scroll);
		cy+=th,brow++;
	}
	if(bs->c){
		for(int bi=0;brow<12;bi++,cy+=th,brow++)cbrushbtn(pos,dn,(rect){0,cy,size.x,th+1},24+bi+MAX(toolbar_scroll-12,0),bt);
		draw_rect((rect){0,cy+1,size.x,tgap},1);cy+=tgap;
		if(toolbar_scroll>0    )if(scrollbtn(pos,dn,(rect){0     ,cy,tcellw+1,size.y-cy},0))toolbar_scroll--;
		if(toolbar_scroll<bs->c)if(scrollbtn(pos,dn,(rect){tcellw,cy,tcellw+1,size.y-cy},1))toolbar_scroll++;
	}
}
void rtoolbar(pair pos,pair dn){
	int pp[]={0,1,4,5,8,9,16,17,12,13,18,19,20,21,22,23,24,25,26,27,2,6,3,7,10,11,14,15,28,29,30,31}; // pleasing visual ramps
	pair size=buff_size(TOOLB);frame=draw_buffer(TOOLB);
	memset(frame.buffer->sv,0,frame.buffer->c),draw_box((rect){0,0,size.x,size.y},0,1),draw_rect((rect){0,16*tcellh,size.x,tgap},1);
	if(modebtn(pos,dn,(rect){0,0     ,tcellw*2+1,tcellh+1},"Stroke",dr.pickfill==0))dr.pickfill=0;
	if(modebtn(pos,dn,(rect){0,tcellh,tcellw*2+1,tcellh+1},"Fill"  ,dr.pickfill==1))dr.pickfill=1;
	if(dr.color){for(int z=0;z<16 ;z++)palbtn(pos,dn,(rect){0,(2*tcellh)+z*tcellh,2*tcellw+1,tcellh+1},(z>=2?31:0)+z);}
	else        {for(int z=0;z<4*8;z++)palbtn(pos,dn,(rect){(z%2)*tcellw,(2*tcellh)+(z/2)*tcellh+(z>=28?tgap:0),tcellw+1,tcellh+1},pp[z]);}
}

// Input and Events

void event_quit(void){
	if(lb(ifield(deck,"locked")))should_exit=1;
	ev.shortcuts['q']=1;
}
void event_touch(void){
	if(!set_touch)enable_touch=1;
}
void event_key(int c,int m,int down,const char*name){
	int cmd=m&(KEYM_LCTRL|KEYM_RCTRL|KEYM_LGUI|KEYM_RGUI);
	if(down){
		if(c>0&&c<4096)keydown[c]=1;
		if(c==KEY_LSHIFT||c==KEY_RSHIFT)ev.shift=1;
		if(c==KEY_UP   )ev.dir=dir_up;
		if(c==KEY_DOWN )ev.dir=dir_down;
		if(c==KEY_LEFT )ev.dir=dir_left;
		if(c==KEY_RIGHT)ev.dir=dir_right;
		if(cmd){
			ev.alt=1;
			if(strlen(name)==1){char i=ev.shift?drom_toupper(name[0]):drom_tolower(name[0]);ev.shortcuts[0xFF&i]=1;}
			if(c==KEY_y&&!ev.shift)ev.shortcuts['Z']=1; // alias for 'redo'
		}
		else if(uimode==mode_draw&&ms.type==modal_none){
			if(c==KEY_1)modal_enter(modal_pattern);
			if(c==KEY_2)modal_enter(modal_fill);
			if(c==KEY_3)modal_enter(modal_brush);
			if(c==KEY_BACKSPACE||c==KEY_DELETE)bg_delete_selection();
		}
		else if(uimode==mode_object&&ms.type==modal_none){
			if(c==KEY_BACKSPACE||c==KEY_DELETE)ob_destroy();
			if(c==KEY_LEFTBRACKET)ob_move_dn();
			if(c==KEY_RIGHTBRACKET)ob_move_up();
		}
		else if(ms.type==modal_recording&&!wid.infield&&au.mode==record_stopped){
			if(c==KEY_BACKSPACE||c==KEY_DELETE)sound_delete();
		}
		if(c==KEY_RETURN)ev.action=1;
		if     (wid.ingrid )grid_keys(c);
		else if(wid.infield)field_keys(c,m&(KEYM_LSHIFT|KEYM_RSHIFT));
		if(uimode==mode_script)sc.status[0]='\0';
		if(c==KEY_SPACE&&!wid.infield)ev.action=1;
		if(c==KEY_TAB   )ev.tab=1;
		if(c==KEY_l&&ms.type==modal_none&&!wid.ingrid&&!wid.infield&&ev.shift)ev.shortcuts['l']=1;
		if(c==KEY_j&&ms.type==modal_none&&!cmd&&dr.limbo_dither&&dither_threshold>-2.0)dither_threshold-=.1;
		if(c==KEY_k&&ms.type==modal_none&&!cmd&&dr.limbo_dither&&dither_threshold< 2.0)dither_threshold+=.1;
		if(ev.alt&&keydown['u']&&keydown['l']&&keydown['d']&&deck&&lb(ifield(deck,"locked"))){
			iwrite(deck,lmistr("locked"),ZERO);ev.alt=0,keydown['u']=0,keydown['l']=0,keydown['d']=0;
		}
	}
	else{
		if(c>0&&c<4096)keydown[c]=0,keyup[c]=1;
		if(c==KEY_LCTRL||c==KEY_RCTRL||c==KEY_LGUI||c==KEY_RGUI)ev.alt=0;
		if(c==KEY_RETURN&&ev.shift)ev.eval=1;
		if(c==KEY_LSHIFT||c==KEY_RSHIFT)ev.shift=0;
		if(c==KEY_m&&uimode==mode_draw&&in_layer())ev.hidemenu^=1;
		if(c==KEY_t&&uimode==mode_draw&&in_layer())dr.trans^=1;
		if(c==KEY_u&&uimode==mode_draw&&in_layer())dr.under^=1;
		if(c==KEY_y&&uimode==mode_draw&&in_layer()&&!cmd)set_tracing=!tracing;
		if(c==KEY_ESCAPE)ev.exit=1;
		if(!wid.infield&&!wid.ingrid&&ms.type==modal_none&&uimode==mode_interact&&card_is(con())){
			if(c==KEY_UP   )msg.target_navigate=ifield(deck,"card"),msg.arg_navigate=lmistr("up");
			if(c==KEY_DOWN )msg.target_navigate=ifield(deck,"card"),msg.arg_navigate=lmistr("down");
			if(c==KEY_LEFT )msg.target_navigate=ifield(deck,"card"),msg.arg_navigate=lmistr("left");
			if(c==KEY_RIGHT)msg.target_navigate=ifield(deck,"card"),msg.arg_navigate=lmistr("right");
		}
		if((uimode==mode_interact||uimode==mode_object||uimode==mode_draw)&&ms.type==modal_none&&!kc.on&&deck&&!lb(ifield(deck,"locked"))){
			if(c==KEY_F1)setuimode(mode_interact);
			if(c==KEY_F2)setuimode(mode_object);
			int f[]={KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_F11,KEY_F12};
			for(int z=0;z<10;z++)if(c==f[z])settool(z);
		}
		if(uimode==mode_draw&&ms.type==modal_none){
			int brush_count=24+dget(deck->b,lmistr("brushes"))->c;
			if(c==KEY_9)dr.brush=MAX(            0,dr.brush-1);
			if(c==KEY_0)dr.brush=MIN(brush_count-1,dr.brush+1);
		}
	}
}
void event_scroll(pair s){
	ev.scroll=s.y<0?1:s.y>0?-1:0;
}
void event_pointer_move(pair raw,pair scaled){
	if(!msg.pending_drag)pointer_prev=pointer;
	ev.rawpos=raw;pointer=ev.pos=scaled;
	if(pointer_held)msg.pending_drag=1;
}
void event_pointer_button(int primary,int middle,int down){
	if(middle){if(down)ev.mdown=1;return;}
	if(down){
		ev.rawdpos=ev.rawpos;
		pointer_held=ev.drag=1;pointer_start=ev.dpos=pointer;ev.md=1;ev.clicktime=12;
		ev.down_modal=ms.type,ev.down_uimode=uimode,ev.down_caps=kc.on;
		if(!primary)ev.rdown=1;
	}
	else{
		pointer_held=ev.drag=0;pointer_end=pointer;ev.mu=1;
		if(ev.clicktime)ev.click =1;ev.clicktime=0;
		if(ev.clicklast)ev.dclick=1;ev.clicklast=DOUBLE_CLICK_DELAY;
		if(!primary)ev.rup=1;
	}
}
float color_dist(int a,int b){
	int dr=(0xFF&(a>>16))-(0xFF&(b>>16));
	int dg=(0xFF&(a>> 8))-(0xFF&(b>> 8));
	int db=(0xFF&(a    ))-(0xFF&(b    ));
	return (dr*dr)+(dg*dg)+(db*db);
}
void event_file(char*p){
	if(lb(ifield(deck,"locked")))return;
	if(has_suffix(p,".html")||has_suffix(p,".deck")){
		modal_enter(modal_resources);
		ms.message=deck_get(n_read(NULL,l_list(lmcstr(p))));
		ms.grid=(grid_val){res_enumerate(ms.message),0,-1,-1};
	}
	if(has_suffix(p,".gif"))import_image(p);
	if(has_suffix(p,".jpeg")||has_suffix(p,".jpg"))import_image(p);
	if(has_suffix(p,".png"))import_image(p);
	if(has_suffix(p,".bmp"))import_image(p);
	if(has_suffix(p,".wav")){
		au.target=n_deck_add(deck,l_list(lmistr("sound")));mark_dirty();modal_enter(modal_recording);
		sound_edit(n_readwav(NULL,l_list(lmutf8(p))));au.sel=(pair){0,0},au.head=0;
	}
	if(has_suffix(p,".csv")||has_suffix(p,".psv")){
		setuimode(mode_object);lv*a=lmd();
		lv* dat=n_read(NULL,l_list(lmcstr(p)));
		lv* sep=lmistr(has_suffix(p,".csv")?",": "|");
		lv* arg=lml(3);arg->lv[0]=dat,arg->lv[1]=LNIL,arg->lv[2]=sep;
		dset(a,lmistr("type"),lmistr("grid"));
		dset(a,lmistr("value"),l_cols(n_readcsv(NULL,arg)));
		ob_create(l_list(a));
	}
	if(has_suffix(p,".hex")){
		lv*pat=ifield(deck,"patterns");iwrite(pat,lmn(32),lmn(0xFFFFFF));iwrite(pat,lmn(47),ZERO);
		lv*t=l_parse(lmistr("%h"),l_split(lmistr("\n"),ls(n_read(NULL,l_list(lmcstr(p))))));
		if(t->c&&ln(t->lv[t->c-1])==0)t->c--; // trailing newline?
		if(t->c>14){
			int a=0,b=0;EACH(z,t){
				if(color_dist(ln(t->lv[z]),0x000000)<color_dist(ln(t->lv[a]),0x000000))a=z; // darkest  -> black
				if(color_dist(ln(t->lv[z]),0xFFFFFF)<color_dist(ln(t->lv[b]),0xFFFFFF))b=z; // lightest -> white
			}
			iwrite(pat,lmn(47),t->lv[a]),iwrite(pat,lmn(32),t->lv[b]);
			lv*f=lml(0);EACH(z,t)if(z!=a&&z!=b)ll_add(f,t->lv[z]);t=f;
		}
		for(int z=0;z<14&&z<t->c;z++)iwrite(pat,lmn(33+z),t->lv[z]);
	}
}

void sync(void){
	pair disp=window_get_size();
	pair size=buff_size(context.buffer);
	int scale=noscale?1: MIN(disp.x/size.x,disp.y/size.y);
	ev.mu=ev.md=ev.click=ev.dclick=ev.tab=ev.action=ev.dir=ev.exit=ev.eval=ev.scroll=ev.rdown=ev.mdown=ev.rup=0;
	if(ev.clicktime)ev.clicktime--;
	if(ev.clicklast)ev.clicklast--;
	if(ev.pos.x!=ev.dpos.x||ev.pos.y!=ev.dpos.y)ev.clicklast=0;
	wid.cursor_timer=(wid.cursor_timer+1)%(2*FIELD_CURSOR_DUTY);
	if(wid.change_timer){wid.change_timer--;if(wid.change_timer==0)field_change();}
	for(int z=0;z<256;z++)ev.shortcuts[z]=0;memset(keyup,0,sizeof(keyup));
	process_events(disp,size,scale);
	pointer_down=ev.md,pointer_up=ev.mu;
	if(toggle_fullscreen){toggle_fullscreen=0;windowed=!windowed;window_set_fullscreen(!windowed);}
	if(!windowed||uimode!=mode_draw)set_tracing=0;
	if(set_tracing!=tracing){tracing=set_tracing,window_set_opacity(tracing?0.7:1.0);}
	pair tsize=buff_size(TOOLB);
	int tscale=MIN((disp.x-scale*size.x)/(2*tsize.x),disp.y/tsize.y),tmscale=tscale;if(tscale&&noscale)tscale=tmscale=1;
	int dpi=get_display_density(disp);disp.x*=dpi,disp.y*=dpi,scale*=dpi,tscale*=dpi;
	pick_palette(deck);
	if(framebuffer_flip(disp,size,scale,dr.trans_mask&&uimode==mode_draw,dr.show_anim?frame_count:0,patterns_pal(ifield(deck,"patterns")),context.buffer)){
		char*pal=patterns_pal(ifield(deck,"patterns"));
		int showwings=!kc.on&&toolbars_enable&&tscale>0&&!(lb(ifield(deck,"locked")))&&ms.type==modal_none&&uimode!=mode_script;
		if(showwings){
			rect dst={0,(disp.y-tscale*tsize.y)/2,tscale*tsize.x,tscale*tsize.y};
			ltoolbar(
				(pair){(ev.rawpos .x-dst.x/dpi)/tmscale,(ev.rawpos .y-dst.y/dpi)/tmscale},
				(pair){(ev.rawdpos.x-dst.x/dpi)/tmscale,(ev.rawdpos.y-dst.y/dpi)/tmscale}
			);
			toolbar_flip(TOOLB,dr.show_anim?frame_count:0,pal,dst);
		}
		if(showwings){
			rect dst={disp.x-tscale*tsize.x,(disp.y-tscale*tsize.y)/2,tscale*tsize.x,tscale*tsize.y};
			rtoolbar(
				(pair){(ev.rawpos .x-dst.x/dpi)/tmscale,(ev.rawpos .y-dst.y/dpi)/tmscale},
				(pair){(ev.rawdpos.x-dst.x/dpi)/tmscale,(ev.rawdpos.y-dst.y/dpi)/tmscale}
			);
			int animate=box_in((rect){dst.x,dst.y,dst.w,dst.h},(pair){ev.rawpos.x*dpi,ev.rawpos.y*dpi})&&dr.show_anim?frame_count:0;
			toolbar_flip(TOOLB,animate,pal,dst);
		}
		finish_flip();
	}
	window_set_cursor(uicursor);
	if(do_panic)setuimode(mode_object);
	do_panic=0;
}

// Runtime

lv*viewed=NULL;
lv* find_animated(void){
	lv*r=lmd();if(uimode!=mode_interact)return r;
	lv*wids=con_wids();EACH(z,wids){
		lv*wid=wids->lv[z];
		if(lb(ifield(wid,"animated"))&&dget(viewed,wid)==NULL)dset(r,wid,ZERO);
		if(!contraption_is(wid))continue;
		lv*cwids=ivalue(wid,"widgets");EACH(c,cwids){
			lv*cwid=cwids->lv[c];
			if(lb(ifield(cwid,"animated"))&&dget(viewed,cwid)==NULL)dset(r,cwid,ONE);
		}
	}return r;
}
void fire_animate(lv*targets){
	lv*name=lmistr("view"),*root=lmenv(NULL);primitives(root,deck),constants(root);
	lv*block=lmblk();EACH(z,targets){
		lv*w=targets->kv[z];
		blk_cat(block,event_invoke(w,name,LNIL,NULL)),dset(viewed,w,ONE);
	}pushstate(root),pending_popstate=1,issue(root,block);
}
void fire_view(lv*target){
	lv*name=lmistr("view"),*root=lmenv(NULL);primitives(root,deck),constants(root);
	lv*block=event_invoke(target,name,LNIL,NULL);
	lv*wids=ifield(target,"widgets");EACH(z,wids){
		lv*w=wids->lv[z];if(!contraption_is(w)||dget(viewed,w)!=NULL)continue;
		blk_cat(block,event_invoke(ivalue(w,"viewproxy"),name,LNIL,NULL)),dset(viewed,w,ONE);
	}pushstate(root),pending_popstate=1,issue(root,block);
}
int interpret(void){
	viewed->c=0;
	if(msg.overshoot&&!running()&&!msg.pending_view&&!msg.next_view)msg.overshoot=0;
	if(msg.pending_halt){if(running())halt();sleep_frames=0,sleep_play=0,msg.pending_view=0,msg.next_view=0;}
	if(sleep_play&&sfx_any())return 0;sleep_play=0;
	if(sleep_frames){sleep_frames--;return 0;}
	int quota=FRAME_QUOTA;
	while(1){
		#define nomodal (ms.type==modal_none||ms.type==modal_query||ms.type==modal_listen)
		while(nomodal&&running()&&sleep_frames==0&&sleep_play==0&&quota>0&&!should_exit){runop();quota--;mark_dirty();}frame=context;
		if(should_exit)break;
		if(quota<=0&&running())msg.overshoot=1;
		if(!nomodal||quota<=0||sleep_frames||sleep_play){if(sleep_frames)sleep_frames--;break;}
		if(!running()&&pending_popstate){popstate();pending_popstate=0;}
		lv*a=find_animated();
		if(msg.pending_halt||pending_popstate){/*suppress other new events until this one finishes*/}
		else if(msg.pending_view){fire_view(con()),msg.pending_view=0;}
		else if(msg.target_click){
			lv*arg=grid_is(msg.target_click)?lmn(msg.arg_click.y): canvas_is(msg.target_click)?lmfpair(msg.arg_click): LNIL;
			fire_event_async(msg.target_click,lmistr("click"),arg);msg.target_click=NULL;
		}
		else if(msg.target_drag    ){fire_event_async(msg.target_drag    ,lmistr("drag"      ),lmfpair(msg.arg_drag   ));msg.target_drag    =NULL;}
		else if(msg.target_release ){fire_event_async(msg.target_release ,lmistr("release"   ),lmfpair(msg.arg_release));msg.target_release =NULL;}
		else if(msg.target_run     ){fire_event_async(msg.target_run     ,lmistr("run"       ),msg.arg_run             );msg.target_run     =NULL;}
		else if(msg.target_link    ){fire_event_async(msg.target_link    ,lmistr("link"      ),msg.arg_link            );msg.target_link    =NULL;}
		else if(msg.target_order   ){fire_event_async(msg.target_order   ,lmistr("order"     ),msg.arg_order           );msg.target_order   =NULL;}
		else if(msg.target_ccell   ){fire_event_async(msg.target_ccell   ,lmistr("changecell"),msg.arg_ccell           );msg.target_ccell   =NULL;}
		else if(msg.target_change  ){fire_event_async(msg.target_change  ,lmistr("change"    ),msg.arg_change          );msg.target_change  =NULL;}
		else if(msg.target_navigate){fire_event_async(msg.target_navigate,lmistr("navigate"  ),msg.arg_navigate        );msg.target_navigate=NULL;}
		else if(a->c               ){fire_animate(a);}
		if(!running())break; // not running, and no remaining events to process, so we're done for this frame
	}
	if(msg.next_view&&ms.type!=modal_listen)msg.pending_view=1,msg.next_view=0; // no more than one view[] event per frame!
	return FRAME_QUOTA-quota;
}
void paste_any(void){
	if(has_clip("%%IMG")){if(menu_item("Paste Image",1,'v')){
		lv*b=image_read(get_clip())->b;setuimode(mode_draw);bg_paste(b,0);
	}}
	else if(has_clip("%%WGT")){if(menu_item("Paste Widgets",1,'v')){
		lv*t=get_clip();int f=1,i=6,n=t->c-i;lv*v=pjson(t->sv,&i,&f,&n);
		lv*defs=dget(v,lmistr("d")),*wids=dget(v,lmistr("w"));wids=wids?ll(wids):lml(0);
		merge_fonts(deck,dget(v,lmistr("f"))),merge_prototypes(deck,defs?ld(defs):lmd(),wids),ob_create(wids);
	}}
	else if(has_clip("%%CRD")){if(menu_item("Paste Card",1,'v')){
		lv*c=n_deck_paste(deck,l_list(get_clip()));con_set(NULL);
		lv*card=ifield(deck,"card");int n=ln(ifield(card,"index"));iwrite(c,lmistr("index"),lmn(n+1));n_go(deck,l_list(c));
	}}
	else{menu_item("Paste",0,'v');}
}
void gestures(void){
	if(!enable_touch||!card_is(con()))return;lv*wids=con_wids();
	if(!in_layer()||uimode!=mode_interact||(!ev.drag&&!ev.mu))return;                              // must be in the right state of mind
	if(ev.drag&&ob.sel->c&&lb(ifield(ob.sel->lv[0],"draggable")))return;                           // must not be dragging a canvas
	int outside=1;EACH(z,wids)outside&=!dover(unpack_widget(wids->lv[z]).size);if(!outside)return; // must touch grass
	int dx=ev.pos.x-ev.dpos.x,dy=ev.pos.y-ev.dpos.y;if(sqrt(dx*dx+dy*dy)<50)return;                // must be emphatic
	if(abs(dx)<2*abs(dy)&&abs(dy)<2*abs(dx))return;                                                // must be highly directional
	int dir=abs(dx)>2*abs(dy)?(dx<0?0:1):(dy<0?2:3);char*dnames[]={"left","right","up","down"};	   // 0:left, 1:right, 2:up, 3:down
	buffer_paste((rect){ev.pos.x-8,ev.pos.y-8,16,16},frame.clip,GESTURES[dir]->b,frame.buffer,0);
	if(ev.mu)msg.target_navigate=con(),msg.arg_navigate=lmistr(dnames[dir]);
}
void text_edit_menu(void){
	int selection=wid.fv!=NULL&&wid.cursor.x!=wid.cursor.y;
	int rich=wid.fv!=NULL&&wid.f.style==field_rich;
	if(menu_item("Undo",wid.hist_cursor>0          ,'z'))field_undo();
	if(menu_item("Redo",wid.hist_cursor<wid.hist->c,'Z'))field_redo();
	menu_separator();
	if(menu_item("Cut",selection,'x')){set_clip(rtext_string(wid.fv->table,wid.cursor));field_keys(KEY_DELETE,0);}
	if(menu_item("Copy",selection,'c')){
		lv*s=rtext_span(wid.fv->table,wid.cursor),*i=rtext_is_image(s);
		set_clip((i?image_write(i):rtext_all(s)));
	}
	if(rich&&menu_item("Copy Rich Text",selection,'r'))set_clip(rtext_encode(rtext_span(wid.fv->table,wid.cursor)));
	if(has_clip("%%IMG")&&rich&&menu_item("Paste Inline Image",wid.fv!=NULL,'v')){field_edit(lmistr(""),image_read(get_clip()),"i",wid.cursor);}
	else if(has_clip("%%RTX")&&rich&&menu_item("Paste Rich Text",wid.fv!=NULL,'v')){field_editr(rtext_decode(get_clip()),wid.cursor);}
	else if((!has_clip("%%RTX")||!rich)&&menu_item("Paste",wid.fv!=NULL&&strlen(clip_stash),'v')){
		field_input(has_clip("%%RTX")?rtext_all(rtext_decode(get_clip()))->sv:get_clip()->sv);
	}
	if(menu_item("Clear",wid.fv!=NULL,0)){wid.cursor=(pair){0,RTEXT_END};field_keys(KEY_DELETE,0);}
	if(!enable_touch&&!kc.on){
		menu_separator();
		if(menu_item("Keycaps...",wid.fv!=NULL,'k'))keycaps_force_enter();
	}
	menu_separator();
	if(menu_item("Select All",wid.fv!=NULL,'a')){wid.cursor=(pair){0,RTEXT_END};}
}
void resize_window(lv*deck); // forward ref
void all_menus(void){
	int blocked=running()||msg.overshoot;
	int canlisten=!blocked&&(ms.type==modal_listen||ms.type==modal_none);
	menu_bar("Decker",canlisten&&!kc.on);
	if(menu_item("About...",1,'\0'))modal_enter(modal_about);
	if(menu_check("Listener",canlisten,ms.type==modal_listen,'l')){if(ms.type!=modal_listen){modal_enter(modal_listen);}else{modal_exit(0);}}
	menu_separator();
	#ifndef NO_FULLSCREEN
		if(menu_check("Fullscreen",1,!windowed,'f' ))toggle_fullscreen=1;
	#endif
	if(menu_check("Touch Input"    ,1                    ,enable_touch   ,'\0')){enable_touch^=1,set_touch=1;if(!enable_touch)kc.on=0;}
	if(menu_check("Script Profiler",1                    ,profiler       ,'\0'))profiler^=1;
	if(menu_check("Toolbars"       ,1                    ,toolbars_enable,'\0'))toolbars_enable^=1,resize_window(deck);
	if(menu_check("Auto-Save"      ,strlen(document_path),autosave       ,'\0'))autosave^=1;
	#ifndef __ANDROID__
		menu_separator();
		if(menu_item("Quit",ms.type==modal_none&&uimode!=mode_script,'q'))quit();
	#endif
	if(blocked){
		menu_bar("Script",1);
		if(menu_item("Stop",1,'\0')){
			msg.pending_halt=1;
			if(ms.type!=modal_query&&ms.type!=modal_listen){if(ms.type!=modal_none){modal_exit(0);}setuimode(mode_object);}
		}
		menu_bar("Edit",(ms.type==modal_input||ms.type==modal_save)&&wid.fv);
		text_edit_menu();
		return;
	}
	menu_bar("File",(ms.type==modal_none||ms.type==modal_recording)&&(!kc.on||uimode==mode_script));
	if(uimode==mode_script){
		if(menu_item("Close Script",1,'\0'))close_script(NULL);
		if(menu_item("Save Script",1,'s')){
			lv*text=rtext_all(sc.f.table);parse(text->sv);
			if(perr()){snprintf(sc.status,sizeof(sc.status),"Error: %s",par.error);wid.cursor=(pair){par.i,par.i};}
			else{script_save(text);snprintf(sc.status,sizeof(sc.status),"Saved.");}
		}
		menu_separator();
		if(menu_item("Import Script..."  ,1,'\0'))modal_enter(modal_import_script);
		if(menu_item("Export Script..."  ,1,'\0'))modal_enter(modal_export_script);
		menu_separator();
		if(menu_item("Go to Deck",!deck_is(sc.target)           ,'\0'))close_script(deck);
		lv*container=con();
		if(prototype_is(container)){if(menu_item("Go to Prototype",sc.target!=container,'\0'))close_script(container);}
		else                       {if(menu_item("Go to Card"     ,sc.target!=container,'\0'))close_script(container);}
		if(menu_check("X-Ray Specs",!kc.on,sc.xray,'r'))sc.xray^=1;
	}
	else if(ms.type==modal_recording){
		if(menu_item("Import Sound...",1,'\0'))modal_enter(modal_import_sound);
		menu_separator();
		if(menu_item("Close Sound",1,'\0'))modal_exit(0);
	}
	else{
		if(menu_item("New Deck...",1,'\0')){
			if(dirty&&autosave&&strlen(document_path)){save_deck(lmcstr(document_path));dirty=0;}
			if(dirty){
				modal_enter(modal_confirm_new);
				ms.message=lmcstr("The current deck has unsaved changes.\nAre you sure you want to discard it?");
				ms.verb=lmcstr("Discard");
			}else{load_deck(deck_get(lmistr("")));set_path("");}
		}
		if(menu_item("New Card",1,'\0')){
			lv*c=n_deck_add(deck,l_list(lmistr("card")));int n=ln(ifield(ifield(deck,"card"),"index"));
			iwrite(c,lmistr("index"),lmn(n+1)),n_go(deck,l_list(c));
		}
		menu_separator();
		if(menu_item("Open...",1,'o'))modal_enter(modal_open_deck);
		if(menu_item("Save",dirty&&strlen(document_path),'s'))save_deck(lmcstr(document_path));
		if(menu_item("Save As...",1,'\0'))modal_enter(modal_save_deck);
		menu_separator();
		if(menu_item("Import Image..."   ,1,'\0'))modal_enter(modal_import_image);
		if(menu_item("Export Image..."   ,1,'\0'))modal_enter(modal_export_image);
		menu_separator();
		if(menu_item("Purge Volatiles",1,'\0')){n_deck_purge(deck,LNIL);msg.next_view=1;}
		menu_separator();
		if(menu_item("Cards..."     ,1,'C' ))modal_enter(modal_cards);
		if(menu_item("Sounds..."    ,1,'S' ))modal_enter(modal_sounds);
		if(menu_item("Prototypes...",1,'T' ))modal_enter(modal_contraptions);
		if(menu_item("Resources..." ,1,'\0'))modal_enter(modal_resources);
		if(menu_item("Properties...",1,'\0'))modal_enter(modal_deck_props);
	}
	if(ms.type==modal_none||wid.gv||wid.fv){
		menu_bar("Edit",wid.gv||wid.fv||(ms.type==modal_none&&uimode==mode_interact)||uimode==mode_draw||uimode==mode_object);
		if(wid.gv){
			int mutable=!wid.g.locked&&ms.type==modal_none;
			if(menu_item("Undo",wid.hist_cursor>0          ,'z'))grid_undo();
			if(menu_item("Redo",wid.hist_cursor<wid.hist->c,'Z'))grid_redo();
			menu_separator();
			if(menu_item("Copy Table",1,'c')){
				set_clip(n_writecsv(NULL,lml2(wid.gv->table,grid_format())));
			}
			if(menu_item("Paste Table",mutable&&strlen(clip_stash),'v')){
				grid_edit(n_readcsv(NULL,lml2(get_clip(),lmcstr(wid.g.format))));
			}
			menu_separator();
			if(menu_item("Delete Row",mutable&&wid.gv->row!=-1,'\0'))grid_deleterow();
			if(menu_item("Add Row",mutable,'\0'))grid_insertrow();
			if(menu_item("Query...",ms.type==modal_none,'u'))modal_enter(modal_query);
		}
		if(wid.fv)text_edit_menu();
		if(ms.type==modal_none&&uimode==mode_interact){
			if(menu_item("Undo",has_undo(),'z'))undo();
			if(menu_item("Redo",has_redo(),'Z'))redo();
			menu_separator();
			paste_any();
		}
		if(ms.type==modal_none&&uimode==mode_draw){
			int sel=bg_has_sel()||bg_has_lasso();
			if(menu_item("Undo",(!sel)&&has_undo(),'z'))undo();
			if(menu_item("Redo",(!sel)&&has_redo(),'Z'))redo();
			menu_separator();
			if(menu_item("Cut Image",sel,'x')){
				lv*i=bg_has_lasso()?buffer_mask(dr.limbo,dr.mask): dr.limbo?bg_scaled_limbo():bg_copy_selection(dr.sel_here);
				bg_scoop_selection(),set_clip(image_write(image_make(i)));bg_delete_selection();
			}
			if(menu_item("Copy Image",sel,'c' )){
				lv*i=bg_has_lasso()?buffer_mask(dr.limbo,dr.mask): dr.limbo?bg_scaled_limbo():bg_copy_selection(dr.sel_here);
				set_clip(image_write(image_make(i)));
			}
			paste_any();
			if(menu_item("Clear",1,'\0')){int t=dr.tool;if(!sel){settool(tool_select),dr.sel_here=con_dim();}bg_delete_selection();settool(t);}
			menu_separator();
			if(menu_item("Select All",1,'a')){settool(tool_select),dr.sel_here=con_dim();}
			if(menu_item("Tight Selection",sel,'g'))bg_tighten();
			if(menu_item("Add Outline",sel,'\0'))bg_outline();
			if(menu_item("Resize to Original",sel&&dr.tool==tool_select,'\0')){
				bg_scoop_selection();pair s=buff_size(dr.limbo);dr.sel_here.w=s.x,dr.sel_here.h=s.y;
			}
			lv*c=con();
			if(card_is     (c))if(menu_item("Resize to Card"     ,sel&&dr.tool==tool_select,'\0')){bg_scoop_selection(),dr.sel_here=con_dim();}
			if(prototype_is(c))if(menu_item("Resize to Prototype",sel&&dr.tool==tool_select,'\0')){bg_scoop_selection(),dr.sel_here=con_dim();}
			menu_separator();
			if(menu_item("Invert",sel&&!dr.limbo_dither,'i')){
				if(bg_has_sel())bg_scoop_selection();
				pair s=buff_size(dr.limbo);char*pal=patterns_pal(ifield(deck,"patterns"));
				EACH(z,dr.limbo)dr.limbo->sv[z]=1^draw_pattern(pal,dr.limbo->sv[z],(z%s.x),(z/s.x));
			}
			if(menu_item("Flip Horizontal",sel,'\0')){
				if(bg_has_sel())bg_scoop_selection();
				buffer_flip_h(dr.limbo);if(dr.mask)buffer_flip_h(dr.mask);if(dr.omask&&dr.limbo_dither)buffer_flip_h(dr.omask);
			}
			if(menu_item("Flip Vertical",sel,'\0')){
				if(bg_has_sel())bg_scoop_selection();
				buffer_flip_v(dr.limbo);if(dr.mask)buffer_flip_v(dr.mask);if(dr.omask&&dr.limbo_dither)buffer_flip_v(dr.omask);
			}
			if(menu_item("Rotate Left",sel,',')){
				pair s={dr.sel_here.w,dr.sel_here.h};
				if(bg_has_sel())bg_scoop_selection();
				buffer_flip_h(dr.limbo),dr.limbo=buffer_transpose(dr.limbo);
				if(dr.mask)buffer_flip_h(dr.mask),dr.mask=buffer_transpose(dr.mask);
				if(dr.omask&&dr.limbo_dither)buffer_flip_h(dr.omask),dr.omask=buffer_transpose(dr.omask);
				dr.sel_here.w=s.y,dr.sel_here.h=s.x;
			}
			if(menu_item("Rotate Right",sel,'.')){
				pair s={dr.sel_here.w,dr.sel_here.h};
				if(bg_has_sel())bg_scoop_selection();
				dr.limbo=buffer_transpose(dr.limbo),buffer_flip_h(dr.limbo);
				if(dr.mask)dr.mask=buffer_transpose(dr.mask),buffer_flip_h(dr.mask);
				if(dr.omask&&dr.limbo_dither)dr.omask=buffer_transpose(dr.omask),buffer_flip_h(dr.omask);
				dr.sel_here.w=s.y,dr.sel_here.h=s.x;
			}
			if(dr.limbo_dither&&sel){
				menu_separator();
				if(menu_item("Lighten Image",dither_threshold>-2.0,'j'))dither_threshold-=.1;
				if(menu_item("Darken  Image",dither_threshold< 2.0,'k'))dither_threshold+=.1;
			}
		}
		if(ms.type==modal_none&&uimode==mode_object){
			if(menu_item("Undo",has_undo(),'z'))undo();
			if(menu_item("Redo",has_redo(),'Z'))redo();
			menu_separator();
			if(menu_item("Cut Widgets" ,ob.sel->c,'x' )){ob_order();set_clip(n_con_copy(con(),l_list(ob.sel)));ob_destroy();}
			if(menu_item("Copy Widgets",ob.sel->c,'c' )){ob_order();set_clip(n_con_copy(con(),l_list(ob.sel)));}
			if(menu_item("Copy Image",ob.sel->c==1,'\0')){set_clip(image_write(draw_widget(ob.sel->lv[0])));frame=context;}
			paste_any();
			menu_separator();
			if(menu_item("Paste as new Canvas",has_clip("%%IMG"),'\0')){
				lv*p=lmd();dset(p,lmistr("type"),lmistr("canvas")),dset(p,lmistr("locked"),ONE),dset(p,lmistr("border"),ZERO);
				dset(p,lmistr("image"),get_clip()),ob_create(l_list(p));frame=context;
			}
			if(menu_item("Paste into Canvas",has_clip("%%IMG")&&ob.sel->c==1&&canvas_is(ob.sel->lv[0]),'\0')){
				lv*i=image_read(get_clip());
				lv*c=ob.sel->lv[0];iwrite(c,lmistr("size"),ifield(i,"size")),dset(c->b,lmistr("image"),i);
			}
			menu_separator();
			if(menu_item("Select All",1,'a')){lv*wids=con_wids();ob.sel->c=0;EACH(z,wids)ll_add(ob.sel,wids->lv[z]);}
			if(menu_item("Move to Front",ob.sel->c,'\0')){ob_order();EACH(z,ob.sel){iwrite(ob.sel->lv[z],lmistr("index"),lmn(RTEXT_END));}mark_dirty();}
			if(menu_item("Move Up"      ,ob.sel->c,'\0')){ob_move_up();}
			if(menu_item("Move Down"    ,ob.sel->c,'\0')){ob_move_dn();}
			if(menu_item("Move to Back" ,ob.sel->c,'\0')){ob_order();EACHR(z,ob.sel){iwrite(ob.sel->lv[z],lmistr("index"),ZERO);};mark_dirty();}
		}
		if(wid.fv){
			int selection=wid.fv!=NULL&&wid.cursor.x!=wid.cursor.y;
			menu_bar("Text",selection&&wid.f.style!=field_plain);
			if(wid.f.style==field_rich){
				if(menu_item("Heading"    ,selection,'\0'))field_stylespan(lmistr("menu"),lmistr(""));
				if(menu_item("Body"       ,selection,'\0'))field_stylespan(lmistr(""    ),lmistr(""));
				if(menu_item("Fixed Width",selection,'\0'))field_stylespan(lmistr("mono"),lmistr(""));
				if(menu_item("Link..."    ,selection,'\0'))modal_push(modal_link);
			}
			else if(wid.f.style==field_code){
				if(menu_item("Indent"        ,1,'\0'))field_indent(1);
				if(menu_item("Unindent"      ,1,'\0'))field_indent(0);
				if(menu_item("Toggle Comment",1,'/' ))field_comment();
			}
			if(wid.f.style!=field_code){
				if(menu_item("Font...",wid.f.style!=field_plain,'\0'))modal_push(modal_fonts);
			}
		}
	}
	if(ms.type==modal_recording&&!wid.fv){
		menu_bar("Edit",au.mode==record_stopped);
		if(menu_item("Undo",au.hist_cursor>0         ,'z'))sound_undo();
		if(menu_item("Redo",au.hist_cursor<au.hist->c,'Z'))sound_redo();
		menu_separator();
		if(menu_item("Cut Sound"  ,1,'x' )){set_clip(sound_write(sound_selected()));sound_delete();}
		if(menu_item("Copy Sound" ,1,'c' )){set_clip(sound_write(sound_selected()));}
		if(menu_item("Paste Sound",has_clip("%%SND"),'v')){
			lv*s=sound_read(get_clip());
			lv*r=sound_slice((pair){0,au.sel.x});int i=au.sel.x; au.head=i;
			for(int z=0       ;z<s->b->c        &&i<(10*SFX_RATE);z++)r->b->sv[i++]=s->b->sv[z];int a=i;
			for(int z=au.sel.y;z<au.target->b->c&&i<(10*SFX_RATE);z++)r->b->sv[i++]=au.target->b->sv[z];
			r->b->c=i;au.sel.y=a;sound_edit(r);
		}
		if(menu_item("Clear",1,'\0'))sound_delete();
		menu_separator();
		if(menu_item("Select All",1,'a')){au.head=0;au.sel=(pair){0,au.target->b->c-1};}
	}
	if((uimode==mode_interact||uimode==mode_draw||uimode==mode_object)&&card_is(con())){
		menu_bar("Card",ms.type==modal_none&&!kc.on);
		lv*card=con();
		if(menu_item("Go to First"   ,1,'\0'))n_go(deck,l_list(lmistr("First")));
		if(menu_item("Go to Previous",1,'\0'))n_go(deck,l_list(lmistr("Prev")));
		if(menu_item("Go to Next"    ,1,'\0'))n_go(deck,l_list(lmistr("Next")));
		if(menu_item("Go to Last"    ,1,'\0'))n_go(deck,l_list(lmistr("Last")));
		if(menu_item("Go Back",dget(deck->b,lmistr("history"))->c>1,'\0'))n_go(deck,l_list(lmistr("Back")));
		menu_separator();
		if(menu_item("Cut Card" ,1,'\0')){set_clip(ls(n_deck_copy(deck,l_list(card))));n_deck_remove(deck,l_list(card));mark_dirty();}
		if(menu_item("Copy Card",1,'\0')){set_clip(ls(n_deck_copy(deck,l_list(card))));}
		menu_separator();
		if(menu_item("Script..."    ,1,'e'))setscript(card);
		if(menu_item("Properties...",1,'\0'))modal_enter(modal_card_props);
	}
	if((uimode==mode_interact||uimode==mode_draw||uimode==mode_object)&&prototype_is(con())){
		menu_bar("Prototype",ms.type==modal_none&&!kc.on);
		lv*def=con(),*defs=ifield(deck,"contraptions");
		if(menu_item("Close",1,'\0'))con_set(NULL);
		if(menu_item("Go to Previous",defs->c>1,'\0')){ev.dir=dir_left ;tracking();ev.dir=dir_none;}
		if(menu_item("Go to Next"    ,defs->c>1,'\0')){ev.dir=dir_right;tracking();ev.dir=dir_none;}
		menu_separator();
		if(menu_item("Script..."    ,1,'e'))setscript(def);
		if(menu_item("Properties...",1,'\0'))modal_enter(modal_prototype_props);
		if(menu_item("Attributes...",1,'\0'))modal_enter(modal_prototype_attrs);
		if(menu_check("Show Margins",1,ob.show_margins,0))ob.show_margins^=1;
		menu_separator();
		int r=lb(ifield(def,"resizable"));
		if(menu_check("Resizable",1,r,'\0')){r^=1,iwrite(def,lmistr("resizable"),lmn(r)),mark_dirty();}
	}
	if(uimode==mode_interact||uimode==mode_draw||uimode==mode_object){
		menu_bar("Tool",ms.type==modal_none&&!kc.on);
		if(menu_check("Interact",1,uimode==mode_interact,0))setuimode(mode_interact);
		if(menu_check("Widgets" ,1,uimode==mode_object  ,0))setuimode(mode_object);
		menu_separator();
		if(menu_check("Select"     ,1,uimode==mode_draw&&dr.tool==tool_select     ,0))settool(tool_select     );
		if(menu_check("Lasso"      ,1,uimode==mode_draw&&dr.tool==tool_lasso      ,0))settool(tool_lasso      );
		if(menu_check("Pencil"     ,1,uimode==mode_draw&&dr.tool==tool_pencil     ,0))settool(tool_pencil     );
		if(menu_check("Line"       ,1,uimode==mode_draw&&dr.tool==tool_line       ,0))settool(tool_line       );
		if(menu_check("Flood"      ,1,uimode==mode_draw&&dr.tool==tool_fill       ,0))settool(tool_fill       );
		if(menu_check("Box"        ,1,uimode==mode_draw&&dr.tool==tool_rect       ,0))settool(tool_rect       );
		if(menu_check("Filled Box" ,1,uimode==mode_draw&&dr.tool==tool_fillrect   ,0))settool(tool_fillrect   );
		if(menu_check("Oval"       ,1,uimode==mode_draw&&dr.tool==tool_ellipse    ,0))settool(tool_ellipse    );
		if(menu_check("Filled Oval",1,uimode==mode_draw&&dr.tool==tool_fillellipse,0))settool(tool_fillellipse);
		if(menu_check("Polygon"    ,1,uimode==mode_draw&&dr.tool==tool_poly       ,0))settool(tool_poly       );
	}
	if(uimode==mode_draw||uimode==mode_object){
		menu_bar("View",ms.type==modal_none&&!kc.on);
		if(menu_check("Show Widgets"         ,1,dr.show_widgets,'\0'))dr.show_widgets^=1;
		if(menu_check("Show Widget Bounds"   ,1,ob.show_bounds ,'\0'))ob.show_bounds ^=1;
		if(menu_check("Show Widget Names"    ,1,ob.show_names  ,'\0'))ob.show_names  ^=1;
		if(menu_check("Show Cursor Info"     ,1,ob.show_cursor ,'\0'))ob.show_cursor ^=1;
		if(menu_check("Show Alignment Guides",1,ob.show_guides ,'\0'))ob.show_guides ^=1;
		menu_separator();
		if(menu_check("Show Grid Overlay",1,dr.show_grid,0  ))dr.show_grid^=1;
		if(menu_check("Snap to Grid"     ,1,dr.snap     ,'p'))dr.snap     ^=1;
		if(menu_item("Grid and Scale..." ,1,'\0'))modal_enter(modal_grid);
		menu_separator();
		if(menu_check("Show Animation"   ,1,dr.show_anim   ,0))dr.show_anim   ^=1;
		if(menu_check("Transparency Mask",1,dr.trans_mask  ,0))dr.trans_mask  ^=1;
		if(menu_check("Fat Bits"         ,1,dr.fatbits     ,0)){
			if(ms.type==modal_none&&uimode!=mode_draw)setuimode(mode_draw);
			dr.fatbits^=1;if(dr.fatbits){center_fatbits(box_midpoint(bg_has_sel()||bg_has_lasso()?dr.sel_here:con_dim()));}
		}
	}
	if(uimode==mode_draw){
		menu_bar("Style",ms.type==modal_none&&!kc.on);
		if(menu_item("Stroke...",1,'\0'))modal_enter(modal_pattern);
		if(menu_item("Fill..."  ,1,'\0'))modal_enter(modal_fill);
		if(menu_item("Brush..." ,1,'\0'))modal_enter(modal_brush);
		menu_separator();
		if(menu_check("Color"       ,1,dr.color,0))dr.color^=1;
		if(menu_check("Transparency",1,dr.trans,0))dr.trans^=1;
		if(menu_check("Underpaint"  ,1,dr.under,0))dr.under^=1;
		#ifndef NO_TRACING
		if(menu_check("Tracing Mode",windowed,tracing,0))set_tracing=!tracing;
		#endif
	}
	if(uimode==mode_object){
		menu_bar("Widgets",ms.type==modal_none);
		if(menu_item("New Button..."     ,1,'\0')){lv*p=lmd();dset(p,lmistr("type"),lmistr("button"));ob_create(l_list(p));}
		if(menu_item("New Field..."      ,1,'\0')){lv*p=lmd();dset(p,lmistr("type"),lmistr("field" ));ob_create(l_list(p));}
		if(menu_item("New Slider..."     ,1,'\0')){lv*p=lmd();dset(p,lmistr("type"),lmistr("slider"));ob_create(l_list(p));}
		if(menu_item("New Canvas..."     ,1,'\0')){lv*p=lmd();dset(p,lmistr("type"),lmistr("canvas"));ob_create(l_list(p));}
		if(menu_item("New Grid..."       ,1,'\0')){lv*p=lmd();dset(p,lmistr("type"),lmistr("grid"  ));ob_create(l_list(p));}
		if(card_is(con())&&menu_item("New Contraption...",1,'\0'))modal_enter(modal_pick_contraption);
		menu_separator();
		if(menu_item("Order..."   ,ifield(con(),"widgets")->c,'O'))modal_enter(modal_orderwids);
		menu_separator();
		int al=1,aa=1,av=1,as=1,at=1,ai=1,an=1;EACH(z,ob.sel){
			widget w=unpack_widget(ob.sel->lv[z]);al&=w.locked;
			as&=w.show==show_solid, at&=w.show==show_transparent, ai&=w.show==show_invert, an&=w.show==show_none;
			aa&=lb(ifield(ob.sel->lv[z],"animated"));
			av&=lb(ifield(ob.sel->lv[z],"volatile"));
		}
		if(menu_check("Locked"          ,ob.sel->c,ob.sel->c&&al,'\0'))ob_edit_prop("locked"  ,lmn(!al));
		if(menu_check("Animated"        ,ob.sel->c,ob.sel->c&&aa,'\0'))ob_edit_prop("animated",lmn(!aa));
		if(menu_check("Volatile"        ,ob.sel->c,ob.sel->c&&av,'\0'))ob_edit_prop("volatile",lmn(!av));
		menu_separator();
		if(menu_check("Show Solid"      ,ob.sel->c,ob.sel->c&&as,'\0'))ob_edit_prop("show",lmistr("solid"));
		if(menu_check("Show Transparent",ob.sel->c,ob.sel->c&&at,'\0'))ob_edit_prop("show",lmistr("transparent"));
		if(menu_check("Show Inverted"   ,ob.sel->c,ob.sel->c&&ai,'\0'))ob_edit_prop("show",lmistr("invert"));
		if(menu_check("Show None"       ,ob.sel->c,ob.sel->c&&an,'\0'))ob_edit_prop("show",lmistr("none"));
		menu_separator();
		if(menu_item("Font..."      ,ob.sel->c   ,'\0'))modal_enter(modal_fonts);
		if(menu_item("Script..."    ,ob.sel->c   ,'r')){
			int m=1;for(int z=1;z<ob.sel->c;z++)if(!matchr(ifield(ob.sel->lv[0],"script"),ifield(ob.sel->lv[z],"script"))){m=0;break;}
			if(m){setscript(l_drop(ZERO,ob.sel));}else{
				modal_enter(modal_multiscript);
				ms.message=lmcstr("Not all of the selected widgets\nhave the same script.\nEdit them all together anyway?");
				ms.verb=lmcstr("Edit");
			}
		}
		if(menu_item("Properties...",ob.sel->c==1,'\0')||(ob.sel->c==1&&ev.action&&ms.type==modal_none))object_properties(ob.sel->lv[0]);
	}
	if(ms.type==modal_listen){
		menu_bar("Listener",1);
		if(menu_item("Clear History",1,'\0')){li.hist->c=0,li.scroll=0;}
		if(menu_item("Clear Locals" ,1,'\0')){li.vars->c=0;}
		menu_separator();
		if(menu_item("Show Locals"  ,1,'\0')){listen_show(align_right,0,li.vars);}
		menu_separator();
		if(menu_item("Evaluate"     ,rtext_len(ms.text.table),'\0'))listener_eval();
	}
	menu_bar("Help",1);
	if(menu_item("Decker Website..."  ,1,'\0'))n_go(deck,l_list(lmcstr("http://beyondloom.com/decker/index.html"          )));
	if(menu_item("Decker Community...",1,'\0'))n_go(deck,l_list(lmcstr("https://internet-janitor.itch.io/decker/community")));
	if(menu_item("Decker Reference...",1,'\0'))n_go(deck,l_list(lmcstr("http://beyondloom.com/decker/decker.html"         )));
	if(menu_item("Lil Reference..."   ,1,'\0'))n_go(deck,l_list(lmcstr("http://beyondloom.com/decker/lil.html"            )));
}

void main_view(void){
	if(in_layer()&&uimode==mode_object&&ob.sel->c==0)tracking();
	lv*back=con_image();pair bsize=image_size(back);char*pal=patterns_pal(ifield(deck,"patterns"));
	if(ms.type!=modal_trans){
		rect cl=con_clip();pair s=con_size();
		if(bsize.x!=s.x||bsize.y!=s.y)image_resize(back,s),mark_dirty();
		if(dr.fatbits){memset(frame.buffer->sv,46,frame.buffer->c),draw_rect(cl,dr.trans_mask?45:32);draw_fat(cl,back->b,pal,frame_count,0,FAT,dr.offset);}
		else if(s.x==frame.size.x&&s.y==frame.size.y){memcpy(frame.buffer->sv,back->b->sv,frame.buffer->c);}
		else{memset(frame.buffer->sv,46,frame.buffer->c),draw_rect(cl,dr.trans_mask?45:32);buffer_paste(cl,frame.clip,back->b,frame.buffer,0);}
	}
	ev=ev_to_con(ev);
	if(uimode==mode_draw&&in_layer())bg_tools();
	if(dr.tool==tool_select&&(dr.sel_start.w>0||dr.sel_start.h>0))draw_rect(con_to_screenr(dr.sel_start),dr.fill);
	bg_lasso_preview();rect livesel=bg_select();
	ev=con_to_ev(ev);
	if(((uimode==mode_object||uimode==mode_draw)&&dr.show_grid)||ms.type==modal_grid){
		rect c=con_dim();
		if(dr.fatbits){
			for(int x=dr.grid_size.x;x<c.w;x+=dr.grid_size.x){rect r=con_to_screenr((rect){x,0,1,c.h});r.w=1;draw_rect(r,44);}
			for(int y=dr.grid_size.y;y<c.h;y+=dr.grid_size.y){rect r=con_to_screenr((rect){0,y,c.w,1});r.h=1;draw_rect(r,44);}
		}
		else{
			for(int x=dr.grid_size.x;x<c.w;x+=dr.grid_size.x)for(int y=dr.grid_size.y;y<c.h;y+=dr.grid_size.y){
				pair r=con_to_screen((pair){x,y});draw_rect((rect){r.x,r.y,1,1},44);
			}
		}
	}
	lv*wids=con_wids();
	if(uimode==mode_object&&ob.show_guides&&ob.sel->c>0){
		rect b={0};EACH(z,ob.sel){rect s=con_to_screenr(unpack_widget(ob.sel->lv[z]).size);b=z==0?s:box_union(b,s);}
		EACH(z,wids){
			int f=0;EACH(w,ob.sel)if(wids->lv[z]==ob.sel->lv[w]){f=1;break;}if(f)continue;
			rect a=con_to_screenr(unpack_widget(wids->lv[z]).size),u=box_union(a,b);
			if(b.y    ==a.y    )draw_hline(u.x,u.x+u.w,u.y    ,13); // top-top
			if(b.y+b.h==a.y+a.h)draw_hline(u.x,u.x+u.w,u.y+u.h,13); // bottom-bottom
			if(b.x    ==a.x    )draw_vline(u.x    ,u.y,u.y+u.h,13); // left-left
			if(b.x+b.w==a.x+a.w)draw_vline(u.x+u.w,u.y,u.y+u.h,13); // right-right
			if(b.y    ==a.y+a.h)draw_hline(u.x,u.x+u.w,b.y    ,13); // top-bottom
			if(b.y+b.h==a.y    )draw_hline(u.x,u.x+u.w,a.y    ,13); // bottom-top
			if(b.x    ==a.x+a.w)draw_vline(b.x,u.y,u.y+u.h    ,13); // left-right
			if(b.x+b.w==a.x    )draw_vline(a.x,u.y,u.y+u.h    ,13); // right-left
		}
	}
	event_state eb=ev;if(uimode!=mode_interact)ev=(event_state){0};
	if(uimode==mode_interact||(dr.show_widgets&&!dr.fatbits)){handle_widgets(wids,con_offset());}
	else if(dr.show_widgets&&dr.fatbits)EACH(z,wids){draw_boxinv(pal,con_to_screenr(unpack_widget(wids->lv[z]).size));}
	ev=eb;
	int resizing=prototype_size_editor();
	if(uimode==mode_draw){if(bg_has_sel())draw_handles(con_to_screenr(livesel));draw_box(con_to_screenr(livesel),0,ANTS);}
	if(wid.pending_grid_edit){wid.pending_grid_edit=0;modal_enter(modal_gridcell);}
	if(uimode==mode_object&&!resizing){ev=ev_to_con(ev),object_editor(),ev=con_to_ev(ev);}
	if((uimode==mode_object&&ob.show_names)||(uimode==mode_draw&&dr.show_widgets&&dr.fatbits)||ms.type==modal_listen){
		EACH(z,wids){
			lv*wid=wids->lv[z];rect size=con_to_screenr(unpack_widget(wid).size);
			lv*n=ifield(wid,"name");pair s=font_textsize(FONT_BODY,n->sv);
			draw_text_outlined((rect){size.x,size.y-s.y,s.x,s.y},n->sv,FONT_BODY);
		}
	}
	if(ob.show_cursor&&ms.type==modal_none&&(uimode==mode_draw||uimode==mode_object)){
		char t[4096];ev=ev_to_con(ev);
		if(uimode==mode_draw&&bg_has_sel()){rect r=livesel;snprintf(t,sizeof(t),"(%3d,%3d,%3d,%3d)",r.x,r.y,r.w,r.h);}
		else if(uimode==mode_object&&ev.drag&&is_resizable()){
			rect w=unpack_widget(ob.sel->lv[0]).size;snprintf(t,sizeof(t),"(%3d,%3d,%3d,%3d)",w.x,w.y,w.w,w.h);
		}
		else if(ev.drag){pair a=ev.dpos,b=ev.pos;snprintf(t,sizeof(t),"(%3d,%3d,%3d,%3d)",b.x,b.y,b.x-a.x,b.y-a.y);}
		else{pair c=ev.pos;snprintf(t,sizeof(t),"(%3d,%3d)",c.x,c.y);}
		pair s=font_textsize(FONT_BODY,t);draw_text_outlined(rect_sub(con_to_screenr(rect_pair(ev.pos,s)),(pair){0,s.y}),t,FONT_BODY);
		ev=con_to_ev(ev);
	}
	if(in_layer()&&ev.exit&&!dr.fatbits&&!card_is(con()))con_set(NULL),ev.exit=0;
}

lv* wid_track(widget_state*w){
	#define mtrack(v) if(v)dset(r,lmistr(#v),v);
	lv*r=lmd();
	mtrack(w->gt)
	mtrack(w->ft)
	mtrack(w->hist)
	mtrack(w->fv_slot.table)
	mtrack(w->gv_slot.table)
	return r;
}
lv* modal_track(modal_state*m){
	lv*r=lmd();
	mtrack(m->grid.table)
	mtrack(m->grid2.table)
	mtrack(m->name.table)
	mtrack(m->text.table)
	mtrack(m->form0.table)
	mtrack(m->form1.table)
	mtrack(m->form2.table)
	mtrack(m->message)
	mtrack(m->verb)
	mtrack(m->carda)
	mtrack(m->cardb)
	mtrack(m->trans)
	mtrack(m->canvas)
	mtrack(m->old_wid.gt)
	mtrack(m->old_wid.ft)
	mtrack(m->old_wid.hist)
	mtrack(m->old_wid.fv_slot.table)
	mtrack(m->old_wid.gv_slot.table)
	return r;
}
void tick(lv*env){
	interpreter_lock();
	msg.pending_drag=0,msg.pending_halt=0;
	if(dirty&&dirty_timer>0&&!running()&&autosave&&strlen(document_path)){
		dirty_timer--;
		if(dirty_timer==0)dirty=0,save_deck(lmcstr(document_path));
	}
	frame=context,uicursor=0;
	menu_setup(),all_menus(),widget_setup();
	memset(frame.buffer->sv,0,frame.buffer->c);
	event_state ev_stash=ev;kc.heading=NULL;if(kc.on){ev=(event_state){0};}
	if(uimode==mode_script){int mh=3+font_h(FONT_MENU);if(!kc.on)script_editor((rect){0,mh,frame.size.x+1,frame.size.y-mh});}else{main_view();}
	modals(),gestures();
	if(uimode==mode_interact&&ev.drag&&ob.sel->c&&lb(ifield(ob.sel->lv[0],"draggable"))){
		lv*c=ivalue(ob.sel->lv[0],"card");pair off=(contraption_is(c)||prototype_is(c))?getpair(ifield(c,"pos")):(pair){0,0};
		iwrite(ob.sel->lv[0],lmistr("pos"),lmpair(pair_sub((pair){ev.pos.x-ob.prev.x,ev.pos.y-ob.prev.y},off))),mark_dirty();
	}
	if(kc.on){ev=ev_stash;keycaps();}
	if(uimode==mode_script&&enable_touch&&ms.type==modal_none)wid.active=0;
	menu_finish();
	if(uimode==mode_draw&&dr.fatbits&&!ev.hidemenu)draw_icon((pair){frame.size.x-14,2},ZOOM,1);
	double used=interpret();
	if(uimode==mode_interact&&profiler){
		rect r={frame.size.x-60,2,50,12};char*pal=patterns_pal(ifield(deck,"patterns"));
		char t[64];snprintf(t,sizeof(t),"%.02f%%",100*used/FRAME_QUOTA),draw_text(inset(r,2),t,FONT_BODY,1);draw_box(r,0,1);
		for(int z=0;z<r.w-2;z++){
			int v=0;for(int i=0;i<4;i++)v=MAX(v,profiler_hist[(profiler_ix+(4*z)+i)%PROFILE_HIST_SZ]);
			draw_invert(pal,(rect){r.x+1+z,r.y+r.h-v,1,v-1});
		}profiler_hist[profiler_ix]=(r.h-2)*(1.0*used)/FRAME_QUOTA;profiler_ix=(profiler_ix+1)%PROFILE_HIST_SZ;
	}
	if((uimode==mode_object||(uimode==mode_draw&&!dr.fatbits))&&!ev.hidemenu){
		rect b=menu_label();
		draw_textr(b,ls(ifield(con(),"name"))->sv,FONT_BODY,1);
		if(ms.type==modal_none){
			if(over(b))uicursor=cursor_point;
			if(ev.mu&&over(b)&&dover(b))modal_enter(card_is(con())?modal_card_props:modal_prototype_props);
		}
	}
	if(ui_container&&ivalue(ui_container,"dead"))ui_container=NULL;
	#define track(v) dset(env,lmistr(#v),v?v:LNIL);
	track(msg.target_click)
	track(msg.target_drag)
	track(msg.target_release)
	track(msg.target_run)
	track(msg.target_link)
	track(msg.target_ccell)
	track(msg.target_order)
	track(msg.target_change)
	track(msg.target_navigate)
	track(msg.arg_run)
	track(msg.arg_link)
	track(msg.arg_ccell)
	track(msg.arg_order)
	track(msg.arg_change)
	track(msg.arg_navigate)
	track(dr.scratch)
	track(dr.mask)
	track(dr.omask)
	track(dr.limbo)
	track(sc.target)
	track(sc.others)
	track(sc.next)
	track(sc.f.table)
	track(ob.sel)
	dset(env,lmistr("wid"),wid_track(&wid));
	dset(env,lmistr("ms"),modal_track(&ms));
	if(ms_index){lv*r=lml(0);for(int z=0;z<ms_index;z++)ll_add(r,modal_track(&ms_stack[z].ms)),ll_add(r,wid_track(&ms_stack[z].wid));dset(env,lmistr("ms-stack"),r);}
	EACH(z,PLAYING)PLAYING->lv[z]=audio_slots[z].clip?audio_slots[z].clip:LNIL;
	ATTRS->c=0;for(int z=0;z<attrs_count;z++)if(attrs[z].value.table)ll_add(ATTRS,attrs[z].value.table);
	track(audio_loop.clip)
	track(orig_loop)
	lv_collect();
	interpreter_unlock();
}

void quit(void){
	if(ms.type!=modal_none)return;
	if(!dirty||lb(ifield(deck,"locked"))){should_exit=1;return;}
	if(autosave&&strlen(document_path)){save_deck(lmcstr(document_path));should_exit=1;return;}
	modal_enter(modal_confirm_quit);
	ms.message=lmcstr("The current deck has unsaved changes.\nAre you sure you want to quit?");
	ms.verb=lmcstr("Quit");
}
void save_deck(lv*path){
	field_change();dirty=0;int html=0;
	if(has_suffix(path->sv,".html")){html=1;}
	else if(!has_suffix(path->sv,".deck")){str n=str_new();str_addz(&n,path->sv),str_addz(&n,".deck");path=lmstr(n);}
	if(!strcmp(ifield(deck,"name")->sv,""))iwrite(deck,lmistr("name"),lmcstr(directory_last(path->sv)));
	n_write(NULL,lml2(path,deck_write(deck,html)));
	set_path(path->sv);
}
void resize_window(lv*deck){
	lv*card=ifield(deck,"card");
	pair size=getpair(ifield(card,"size")), dis=get_display_size();
	context=draw_buffer(lmbuff(size));
	dset(env,lmistr("buff"),context.buffer);
	int minscale=noscale?1: (size.x*2<=dis.x&&size.y*2<=dis.y)?2:1;
	pair wsize={(size.x+(toolbars_enable?4+2*buff_size(TOOLB).x:0))*minscale,size.y*minscale+(toolbars_enable?4:0)};
	window_set_size(wsize,size,minscale);
}
void load_deck(lv*d){
	dirty=0; wid.active=-1; dr=ddr; con_set(NULL);
	dset(env,lmistr("deck"),deck=d);
	dset(env,lmistr("hist" ),wid.hist=lml(0));
	dset(env,lmistr("ahist"),au.hist=lml(0));
	dset(env,lmistr("doc_hist"),doc_hist=lml(0)); doc_hist_cursor=0;
	lv*fonts=ifield(deck,"fonts");
	FONT_BODY=dget(fonts,lmistr("body"));
	FONT_MENU=dget(fonts,lmistr("menu"));
	FONT_MONO=dget(fonts,lmistr("mono"));
	resize_window(deck);
	time_t now;time(&now);seed=0xFFFFFFFF&now;
	validate_modules();
	setuimode(mode_interact);n_play(deck,lml2(LNIL,lmistr("loop")));msg.next_view=1;
}
int main(int argc,char**argv){
	char*file=NULL,*startcard=NULL;int ul=0;
	for(int z=1;z<argc;z++){
		if(!strcmp("--no-sound"   ,argv[z])){nosound=1;continue;}
		if(!strcmp("--no-scale"   ,argv[z])){noscale=1;continue;}
		if(!strcmp("--no-touch"   ,argv[z])){set_touch=1;continue;}
		if(!strcmp("--fullscreen" ,argv[z])){toggle_fullscreen=1;continue;}
		if(!strcmp("--unlock"     ,argv[z])){ul=1;continue;}
		if(!strcmp("--card"       ,argv[z])){if(z<argc-1)startcard=argv[++z];continue;}
		file=argv[z],set_path(argv[z]);
	}
	init_interns();
	if(file){directory_normalize(ms.path,file),directory_parent(ms.path);}else{directory_home(ms.path);}
	env=lmenv(NULL);init(env);
	{lv*i=image_read(lmcstr(TOOL_ICONS ));TOOLS =lml(12);EACH(z,TOOLS )TOOLS ->lv[z]=image_make(buffer_copy(i->b,(rect){0,z*16,16,16}));}
	{lv*i=image_read(lmcstr(ARROW_ICONS));ARROWS=lml( 8);EACH(z,ARROWS)ARROWS->lv[z]=image_make(buffer_copy(i->b,(rect){0,z*12,12,12}));}
	dset(env,lmistr("check"    ),CHECK     =image_read(lmistr("%%IMG0AAkABwCAAcGDY8Y2bBw4CBAA")));
	dset(env,lmistr("check on" ),CHECKS[1] =image_read(lmistr("%%IMG0AA8ADQAAf/BgMFBQSJBFEEIQRRBIkFBQYDB/8AAA")));
	dset(env,lmistr("check off"),CHECKS[0] =image_read(lmistr("%%IMG0AA8ADQAAf/BAEEAQQBBAEEAQQBBAEEAQQBB/8AAA")));
	dset(env,lmistr("checkdon" ),CHECKS[3] =image_read(lmistr("%%IMG0AA8ADQAAVVAgIFBQCIBFEAIARRAIgFBQICBVUAAA")));
	dset(env,lmistr("checkdoff"),CHECKS[2] =image_read(lmistr("%%IMG0AA8ADQAAVVAAAEAQAABAEAAAQBAAAEAQAABVUAAA")));
	dset(env,lmistr("locked"   ),LOCK      =image_read(lmistr("%%IMG0AAgACDhERP7+/v4A")));
	dset(env,lmistr("anim"     ),ANIM      =image_read(lmistr("%%IMG0AAgACBAoKER8goIA")));
	dset(env,lmistr("zoom"     ),ZOOM      =image_read(lmistr("%%IMG0AAwADB4AIQBMgIxAv0C/QIxATIAhwB7gAHAAMA==")));
	dset(env,lmistr("corner nw"),CORNERS[0]=image_read(lmistr("%%IMG0AAUABf/mxISA")));
	dset(env,lmistr("corner ne"),CORNERS[1]=image_read(lmistr("%%IMG0AAUABfk4GAgI")));
	dset(env,lmistr("corner sw"),CORNERS[2]=image_read(lmistr("%%IMG0AAUABYSGx+f4")));
	dset(env,lmistr("corner se"),CORNERS[3]=image_read(lmistr("%%IMG0AAUABQgIGT/4")));
	dset(env,lmistr("radio nrm"),RADIOS [0]=image_read(lmistr("%%IMG0ABAADgAAB4AYYCAQIBBACEAIQAhACCAQIBAYYAeAAAA=")));
	dset(env,lmistr("radio act"),RADIOS [1]=image_read(lmistr("%%IMG0ABAADgAAB4Af4DhwMDBgGGAYYBhgGDAwOHAf4AeAAAA=")));
	dset(env,lmistr("radio dot"),RADIOS [2]=image_read(lmistr("%%IMG0ABAADgAAAAAAAAAAB4APwA/AD8APwAeAAAAAAAAAAAA=")));
	dset(env,lmistr("radio msk"),RADIOS [3]=image_read(lmistr("%%IMG0ABAADgAAB4Af4D/wP/B/+H/4f/h/+D/wP/Af4AeAAAA=")));
	dset(env,lmistr("icon dir" ),ICONS  [0]=image_read(lmistr("%%IMG0AAwADAAAAAM8BEPkQBRAFEAUQBRAFEAXf/AAAA==")));
	dset(env,lmistr("icon file"),ICONS  [1]=image_read(lmistr("%%IMG0AAwADAABHwERgRFBEeEQIRAhECEQIRAhH+AAAA==")));
	dset(env,lmistr("icon snd" ),ICONS  [2]=image_read(lmistr("%%IMG0AAwADAAAAgAGQA4jPqM+oz6gDiAGQAIAAAAAAA==")));
	dset(env,lmistr("icon fnt" ),ICONS  [3]=image_read(lmistr("%%IMG0AAwADAABH4EfwAHBH8M/wznDOcM5wz/hH+AAAA==")));
	dset(env,lmistr("icon app" ),ICONS  [4]=image_read(lmistr("%%IMG0AAwADAAAAgAFAAiBEEIjpERSLiEScAnwBWACAA==")));
	dset(env,lmistr("icon lil" ),ICONS  [5]=image_read(lmistr("%%IMG0AAwADAAAAAEf4RShF6EUoRShF6EQoR/gAAAAAA==")));
	dset(env,lmistr("icon pat" ),ICONS  [6]=image_read(lmistr("%%IMG0AAwADAABFUIqoRVCKqEVQiqhFUIqoRVCKqAAAA==")));
	dset(env,lmistr("icon chek"),ICONS  [7]=image_read(lmistr("%%IMG0AAwADAAAAAAAIABgAMRBhmMDNgEcAAgAAAAAAA==")));
	dset(env,lmistr("gest lf"  ),GESTURES[0]=image_read(lmistr("%%IMG2ABAAEAAaIAQACiADAQIgAQAIIAMBBCABAAYgAwEGIAEABCADAQggBgEMIAIBDiAEAQwgAQACIAMBCCADAAQgAwEGIAEACCADAQQgAQAKIAMBAiABAAwgBAAi")));
	dset(env,lmistr("gest rt"  ),GESTURES[1]=image_read(lmistr("%%IMG2ABAAEAASIAQADCABAQIgAwAKIAEBBCADAAggAQEGIAMABCADAQggAwACIAEBDCAEAQ4gAgEMIAYBCCADAAQgAQEGIAMABiABAQQgAwAIIAEBAiADAAogBAAq")));
	dset(env,lmistr("gest up"  ),GESTURES[2]=image_read(lmistr("%%IMG2ABAAEAAHIAMADSABAQEgAQAMIAIBASACAAsgAQEDIAEACiACAQMgAgAJIAEBBSABAAggAgEFIAIAByABAQcgAQAGIAIBByACAAUgAQEJIAEABCACAQkgAgADIAEBCyABAAMgAQELIAEAAyAFAQMgBQAHIAEBAyABAAsgBQAF")));
	dset(env,lmistr("gest dn"  ),GESTURES[3]=image_read(lmistr("%%IMG2ABAAEAAFIAUACyABAQMgAQAHIAUBAyAFAAMgAQELIAEAAyABAQsgAQADIAIBCSACAAQgAQEJIAEABSACAQcgAgAGIAEBByABAAcgAgEFIAIACCABAQUgAQAJIAIBAyACAAogAQEDIAEACyACAQEgAgAMIAEBASABAA0gAwAH")));
	dset(env,lmistr("handlet"  ),HANDLES[0]=image_read(lmistr("%%IMG2AAcACgABAQUAAQEBIAUBAiAFAQIgBQECIAUBAiAFAQIgBQEBAAEBASADAQEAAwEBIAEBAQAFAQEAAw==")));
	dset(env,lmistr("handlel"  ),HANDLES[1]=image_read(lmistr("%%IMG2AAoABwABAQYAAwEBIAYBAQACAQEgBwEBAAEBASAIAQIgBwEBAAEBASAGAQEAAwEGAAM=")));
	dset(env,lmistr("tools"    ),TOOLS);
	dset(env,lmistr("arrows"   ),ARROWS);
	dset(env,lmistr("ltools"   ),TOOLB     =lmbuff((pair){tcellw*2+1,tcellh*18+tgap+1}));
	dset(env,lmistr("playing"  ),PLAYING   =l_take(lmn(SFX_SLOTS),LNIL));
	dset(env,lmistr("attribs"  ),ATTRS     =lml(0));
	dset(env,lmistr("li hist"  ),li.hist   =lml(0));
	dset(env,lmistr("li vars"  ),li.vars   =lmd());
	dset(env,lmistr("viewed"   ),viewed    =lmd());
	ob.sel=lml(0);

	FONT_BODY=font_read(lmistr(FONT_BLOCK_BODY));
	FONT_MENU=font_read(lmistr(FONT_BLOCK_MENU));
	FONT_MONO=font_read(lmistr(FONT_BLOCK_MONO));
	dset(env,lmistr("acc m" ),ACCENTS[0]=isolate_accent(0,FONT_MENU));
	dset(env,lmistr("acc a" ),ACCENTS[1]=isolate_accent(1,FONT_MENU));
	dset(env,lmistr("acc u" ),ACCENTS[2]=isolate_accent(2,FONT_MENU));
	dset(env,lmistr("acc ci"),ACCENTS[3]=isolate_accent(3,FONT_MENU));
	dset(env,lmistr("acc g" ),ACCENTS[4]=isolate_accent(4,FONT_MENU));
	dset(env,lmistr("acc h" ),ACCENTS[5]=image_read(lmistr("%%IMG0AAYADQBEiAAAAAAAAAAAAAA=")));
	dset(env,lmistr("acc o" ),ACCENTS[6]=isolate_accent(6,FONT_MENU));
	dset(env,lmistr("acc ca"),ACCENTS[7]=isolate_accent(7,FONT_MENU));
	dset(env,lmistr("acc t" ),ACCENTS[8]=isolate_accent(8,FONT_MENU));
	dset(env,lmistr("acc co"),ACCENTS[9]=isolate_accent(9,FONT_MENU));

	context=draw_buffer(lmbuff((pair){512,342}));
	dset(env,lmistr("buff"),context.buffer);

	io_init();

	if(file){load_deck(deck_get(n_read(NULL,l_list(lmcstr(file))))),set_path(file);}
	char base[PATH_MAX];base_path(base);
	struct stat buffer;if(!deck&&base[0]){
		char p[PATH_MAX];snprintf(p,sizeof(p),"%s/start.deck",base);
		if(stat(p,&buffer)==0)load_deck(deck_get(n_read(NULL,l_list(lmcstr(p))))),set_path(p);
	}
	if(!deck){str doc=str_new();str_add(&doc,(char*)examples_decks_tour_deck,examples_decks_tour_deck_len);load_deck(deck_get(lmstr(doc)));}
	if(startcard){iwrite(deck,lmistr("card"),lmcstr(startcard));}
	if(ul){iwrite(deck,lmistr("locked"),ZERO);}
	io_run(env);
	return 0;
}
