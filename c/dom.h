
#include "resources.h"

// Decker document object model

#define FORMAT_VERSION 1
#define RTEXT_END      2147483647
#define ANTS           255
#define MODULE_QUOTA   (10*4096)
#define TRANS_QUOTA    ( 2*4096)
#define BRUSH_QUOTA    (    128)
#define LOOP_QUOTA     ( 1*4096)
#define ATTR_QUOTA     ( 1*4096)
#define FRAME_QUOTA    (10*4096)
#define CLAMP(a,x,b)   ((x)<(a)?(a): (x)>(b)?(b): (x))
#define itype(name)    int name##_is(lv*x){return x&&lii(x)&&!strcmp(x->a->sv,#name);}
#define init_field(dst,key,src) {lv*k=lmistr(key),*v=dget(src,k);if(v)iwrite(dst,k,v);}

typedef struct {int x,y;} pair;
typedef struct {double x,y;} fpair;
typedef struct {int x,y,w,h;} rect;
typedef struct {
	int brush, pattern;
	pair size; rect clip;
	lv *buffer, *font;
} cstate; cstate frame;

itype(image)itype(sound)itype(font)itype(button)itype(field)itype(slider)itype(grid)itype(canvas)itype(deck)itype(card)itype(patterns)itype(module)itype(array)
itype(prototype)itype(contraption)itype(proxy)
int widget_is(lv*x){return button_is(x)||field_is(x)||slider_is(x)||grid_is(x)||canvas_is(x)||contraption_is(x)||proxy_is(x);}

int sleep_frames=0, sleep_play=0;
lv* n_sleep(lv*self,lv*z){(void)self;z=l_first(z);if(matchr(z,lmistr("play"))){sleep_play=1;}else{sleep_frames=MAX(1,ln(z));}return z;}
lv* n_transition(lv*self,lv*z){lv*t=dget(self->b,lmistr("transit"));z=l_first(z);if(lion(z))dset(t,lmcstr(z->sv),z);return t;}

void go_notify(lv*deck,lv*args,int dest); // forward refs
void field_notify(lv*field);
lv*n_alert(lv*self,lv*z);
lv*n_open(lv*self,lv*z);
lv*n_save(lv*self,lv*z);
lv*n_array(lv*self,lv*z);
lv*n_image(lv*self,lv*z);
lv*n_sound(lv*self,lv*z);

lv* n_go(lv*self,lv*z){
	lv*data=self->b,*x=l_first(z),*r=NULL,*k=lmistr("card"),*cards=ifield(self,"cards");int i=ln(dget(data,k));
	lv*hist=dget(data,lmistr("history"));
	if(lin(x)){int n=ln(x);r=lmn(CLAMP(0,n,cards->c-1));} // by index
	else if(card_is(x)){EACH(z,cards)if(x==cards->lv[z]){r=lmn(z);break;}} // by value
	else{
		x=ls(x);
		if(hist->c>1&&!strcmp(x->sv,"Back")){
			ll_pop(hist);lv*c=ll_peek(hist);if(ln(c)>=0&&ln(c)<cards->c){go_notify(self,z,ln(c)),dset(data,k,c);return dget(data,k);}
		}
		else if(!strcmp(x->sv,"First"))r=lmn(0);
		else if(!strcmp(x->sv,"Last"))r=lmn(cards->c-1);
		else if(!strcmp(x->sv,"Prev"))r=lmn(mod(i-1,cards->c));
		else if(!strcmp(x->sv,"Next"))r=lmn(mod(i+1,cards->c));
		else{int ix=dgeti(cards,x);if(ix>=0)r=lmn(ix);} // by name
	}if(r){go_notify(self,z,ln(r)),dset(data,k,r);if(ln(r)!=i)ll_add(hist,r);}else{go_notify(self,z,-1);}return dget(data,k);
}

lv* interface_rtext(lv*self,lv*i,lv*x); // forward ref
lv* interface_bits(lv*self,lv*i,lv*x); // forward ref
lv* interface_pointer(lv*self,lv*i,lv*x); // forward ref
lv* interface_danger(lv*self,lv*i,lv*x); // forward ref
lv* interface_app(lv*self,lv*i,lv*x); // forward ref
lv* n_brush(lv*self,lv*z); // forward ref
lv* n_panic(lv*self,lv*z); // forward ref
#define MATH_PI 3.141592653589793
#define MATH_E  2.718281828459045
void constants(lv*env){
	dset(env,lmistr("sys"    ),lmi(interface_sys,    lmistr("system" ),NULL));
	dset(env,lmistr("app"    ),lmi(interface_app,    lmistr("app"    ),NULL));
	dset(env,lmistr("rtext"  ),lmi(interface_rtext,  lmistr("rtext"  ),NULL));
	dset(env,lmistr("bits"   ),lmi(interface_bits,   lmistr("bits"   ),NULL));
	dset(env,lmistr("pointer"),lmi(interface_pointer,lmistr("pointer"),NULL));
#ifdef DANGER_ZONE
	// potentially unsafe/nonportable scripting APIs
	dset(env,lmistr("danger" ),lmi(interface_danger, lmistr("danger" ),NULL));
#endif
	dset(env,lmistr("pi"),lmn(MATH_PI));
	dset(env,lmistr("e" ),lmn(MATH_E));
	lv*colors=lmd();
	dset(colors,lmistr("white"     ),lmn(32));
	dset(colors,lmistr("yellow"    ),lmn(33));
	dset(colors,lmistr("orange"    ),lmn(34));
	dset(colors,lmistr("red"       ),lmn(35));
	dset(colors,lmistr("magenta"   ),lmn(36));
	dset(colors,lmistr("purple"    ),lmn(37));
	dset(colors,lmistr("blue"      ),lmn(38));
	dset(colors,lmistr("cyan"      ),lmn(39));
	dset(colors,lmistr("green"     ),lmn(40));
	dset(colors,lmistr("darkgreen" ),lmn(41));
	dset(colors,lmistr("brown"     ),lmn(42));
	dset(colors,lmistr("tan"       ),lmn(43));
	dset(colors,lmistr("lightgray" ),lmn(44));
	dset(colors,lmistr("mediumgray"),lmn(45));
	dset(colors,lmistr("darkgray"  ),lmn(46));
	dset(colors,lmistr("black"     ),lmn(47));
	dset(env,lmistr("colors"),colors);
}
lv*n_play(lv*self,lv*z); // forward ref
lv*n_newdeck(lv*self,lv*z); // forward ref
void primitives(lv*env,lv*deck){
	dset(env,lmistr("show"      ),lmnat(n_show      ,NULL));
	dset(env,lmistr("print"     ),lmnat(n_print     ,NULL));
	dset(env,lmistr("panic"     ),lmnat(n_panic     ,NULL));
	dset(env,lmistr("play"      ),lmnat(n_play      ,deck));
	dset(env,lmistr("go"        ),lmnat(n_go        ,deck));
	dset(env,lmistr("transition"),lmnat(n_transition,deck));
	dset(env,lmistr("brush"     ),lmnat(n_brush     ,deck));
	dset(env,lmistr("sleep"     ),lmnat(n_sleep     ,NULL));
	dset(env,lmistr("eval"      ),lmnat(n_eval      ,NULL));
	dset(env,lmistr("random"    ),lmnat(n_random    ,NULL));
	dset(env,lmistr("array"     ),lmnat(n_array     ,NULL));
	dset(env,lmistr("image"     ),lmnat(n_image     ,NULL));
	dset(env,lmistr("sound"     ),lmnat(n_sound     ,NULL));
	dset(env,lmistr("newdeck"   ),lmnat(n_newdeck   ,NULL));
	dset(env,lmistr("readcsv"   ),lmnat(n_readcsv   ,NULL));
	dset(env,lmistr("writecsv"  ),lmnat(n_writecsv  ,NULL));
	dset(env,lmistr("readxml"   ),lmnat(n_readxml   ,NULL));
	dset(env,lmistr("writexml"  ),lmnat(n_writexml  ,NULL));
	dset(env,lmistr("alert"     ),lmnat(n_alert     ,NULL));
	dset(env,lmistr("read"      ),lmnat(n_open      ,NULL));
	dset(env,lmistr("write"     ),lmnat(n_save      ,NULL));
}

char* default_handlers=""
"on link x do"
"	go[x] "
"end\n"
"on navigate x do"
"	if x~\"right\" go[\"Next\"] end "
"	if x~\"left\"  go[\"Prev\"] end "
"end\n"
"on drag x do"
"	if !me.locked|me.draggable me.line[(pointer.prev-me.offset)/me.scale x] end "
"end\n"
"on order x do"
"	if !me.locked "
"		me.value:select orderby me.value[x] asc from me.value "
"	end "
"end\n"
"on changecell x do "
"	f:me.format[me.col] f:if count f f else \"s\" end "
"	me.cellvalue:(\"%%%l\" format f) parse x "
"	me.event[\"change\" me.value] "
"end\n"
"on loop x do"
"	x "
"end\n"
;

char* default_transitions=""
"transition[on SlideRight c a b t do  c.paste[a c.size*t,0   ] c.paste[b c.size*(t-1),0]      end]\n"
"transition[on SlideLeft  c a b t do  c.paste[a c.size*(-t),0] c.paste[b c.size*(1-t),0]      end]\n"
"transition[on SlideDown  c a b t do  c.paste[a c.size*0,t   ] c.paste[b c.size*0,t-1  ]      end]\n"
"transition[on SlideUp    c a b t do  c.paste[a c.size*0,-t  ] c.paste[b c.size*0,1-t  ]      end]\n"
"transition[on WipeRight  c a b t do  c.rect[0,0        c.size*t,1    ]          c.merge[a b] end]\n"
"transition[on WipeLeft   c a b t do  c.rect[0,0        c.size*(1-t),1]          c.merge[b a] end]\n"
"transition[on WipeDown   c a b t do  c.rect[0,0        c.size*1,t    ]          c.merge[a b] end]\n"
"transition[on WipeUp     c a b t do  c.rect[0,0        c.size*1,1-t  ]          c.merge[b a] end]\n"
"transition[on BoxIn      c a b t do  c.rect[c.size/2   c.size*t   \"center\"]   c.merge[a b] end]\n"
"transition[on BoxOut     c a b t do  c.rect[c.size/2   c.size*1-t \"center\"]   c.merge[b a] end]\n"
;

void ancestors_inner(lv*target,lv*found,lv**deck){
	if(deck_is(target)){*deck=target;return;}
	if(contraption_is(target)){*deck=ivalue(ivalue(target,"card"),"deck");}
	else if(card_is(target)||prototype_is(target)){*deck=ivalue(target,"deck");}
	else{ancestors_inner(ivalue(target,"card"),found,deck);}
	lv*t=contraption_is(target)?ifield(target,"def"):target;
	lv*s=ifield(t,"script"),*block=parse(s&&s->c?s->sv:"");
	if(perr()){block=parse("");}dset(found,target,block);
}
void ancestors_outer(lv*target,lv*found,lv**deck){
	if(deck_is(target)){*deck=target;}
	else{ancestors_outer(ivalue(target,widget_is(target)?"card":"deck"),found,deck);}
	lv*s=ifield(target,"script"),*block=parse(s&&s->c?s->sv:"");
	if(perr()){block=parse("");}dset(found,target,block);
}
void ancestors(lv*target,lv*found,lv**deck){
	lv*p=ivalue(target,"card");
	if(prototype_is(target)||prototype_is(p)||contraption_is(p)){ancestors_inner(target,found,deck);}
	else{ancestors_outer(target,found,deck);}
}
lv* parent_deck(lv*x){
	if(deck_is(x))return x;
	if(card_is(x)||prototype_is(x))return ivalue(x,"deck");
	return parent_deck(ivalue(x,"card"));
}
lv* event_invokev(lv*target,lv*name,lv*arg,lv*hunk,int nodiscard){
	lv*scopes=lmd();dset(scopes,ZERO,parse(default_handlers));
	lv*deck=NULL,*core=NULL;ancestors(target,scopes,&deck);
	for(int z=scopes->c-1;z>=0;z--){
		lv*t=scopes->kv[z],*b=lmblk();char*sname="!widget_scope";
		if(lin(t))sname="!default_handlers";
		if(deck_is(t)){
			lv*modules=ifield(t,"modules"),*cards=ifield(t,"cards");
			EACH(z,modules)blk_lit(b,ifield(modules->lv[z],"value")),blk_loc(b,modules->kv[z]),blk_op(b,DROP);
			EACH(z,cards  )blk_lit(b,       cards  ->lv[z]         ),blk_loc(b,cards  ->kv[z]),blk_op(b,DROP);
			sname="!deck_scope";
		}
		if(card_is(t)||prototype_is(t)||(contraption_is(t)&&target!=t)){
			blk_lit(b,t),blk_loc(b,lmistr("card")),blk_op(b,DROP);
			lv*widgets=ivalue(t,"widgets");
			EACH(z,widgets)blk_lit(b,widgets->lv[z]),blk_loc(b,widgets->kv[z]),blk_op(b,DROP);
			sname="!card_scope";
		}
		blk_cat(b,scopes->lv[z]),blk_op(b,DROP);
		if(!core&&hunk){
			str n=str_new();str_addz(&n,"!hunk");
			blk_lit(b,lmon(n,lml(0),blk_end(hunk))),blk_op(b,BIND),blk_op(b,DROP);
			name=lmistr("!hunk"),arg=lml(0);
		}else if(core){
			str n=str_new();str_addz(&n,sname);
			blk_lit(b,lmon(n,lml(0),blk_end(core))),blk_op(b,BIND),blk_op(b,DROP);
			name=lmistr(sname),arg=lml(0);
		}
		blk_get(b,name),blk_lit(b,arg),blk_op(b,CALL);if(!hunk&&!nodiscard)blk_op(b,DROP);core=b;
	}
	lv*b=lmblk();blk_lit(b,proxy_is(target)?ivalue(target,"card"):target),blk_loc(b,lmistr("me")),blk_op(b,DROP);
	blk_lit(b,deck                   ),blk_loc(b,lmistr("deck"    )),blk_op(b,DROP);
	blk_lit(b,ifield(deck,"patterns")),blk_loc(b,lmistr("patterns")),blk_op(b,DROP);
	blk_cat(b,core);return b;
}
lv* event_invoke(lv*target,lv*name,lv*arg,lv*hunk){return event_invokev(target,name,arg,hunk,0);}
int pending_popstate=0;
void fire_async(lv*target,lv*name,lv*arg,lv*hunk,int nest){
	lv*root=lmenv(NULL);primitives(root,parent_deck(target)),constants(root);
	lv*block=event_invoke(target,name,arg,hunk);
	if(nest)pushstate(root),pending_popstate=1;issue(root,block);
}
lv* n_event(lv*self,lv*x){
	lv*root=lmenv(NULL);primitives(root,parent_deck(self)),constants(root);
	lv*b=lmblk();blk_op(b,DROP),blk_cat(b,event_invokev(self,ls(l_first(x)),l_drop(ONE,x),NULL,1));
	issue(root,b);return LNIL;
}
void fire_event_async(lv*target,lv*name,lv*arg){fire_async(target,name,l_list(arg),NULL,1);}
void fire_hunk_async(lv*target,lv*hunk){fire_async(target,NULL,lml(0),hunk,1);}
int in_attr=0;
lv* fire_attr_sync(lv*target,char*prefix,lv*name,lv*arg){
	if(in_attr>=2)return LNIL;in_attr++;cstate bf=frame;
	lv*root=lmenv(NULL),*deck=ivalue(target,"deck");primitives(root,deck),constants(root);
	dset(root,lmistr("me"),target),dset(root,lmistr("card"),target);
	dset(root,lmistr("deck"),deck),dset(root,lmistr("patterns"),ifield(deck,"patterns"));
	lv*b=lmblk();lv*widgets=ivalue(target,"widgets");EACH(z,widgets)blk_lit(b,widgets->lv[z]),blk_loc(b,widgets->kv[z]),blk_op(b,DROP);
	lv*s=ifield(ivalue(target,"def"),"script"),*sb=parse(s&&s->c?s->sv:"");if(perr()){sb=parse("");}blk_cat(b,sb),blk_op(b,DROP);
	str n=str_new();str_addz(&n,prefix),str_addz(&n,name->sv);blk_get(b,lmstr(n)),blk_lit(b,arg?l_list(arg):lml(0)),blk_op(b,CALL);
	pushstate(root);issue(root,b);int q=ATTR_QUOTA;while(running()&&q>0)runop(),q--;lv*r=running()?LNIL:arg();popstate();frame=bf;return in_attr--,r;
}

lv*interface_widget(lv*self,lv*i,lv*x); // forward reference
int gcd(int x,int y){while(x!=y){if(x>y){x-=y;}else{y-=x;}}return x;}
int lcm(int x,int y){int r=gcd(x,y);return (x*y)/(r?r:1);}
int has_prefix(char*x,char*px){for(int z=0;px[z];z++)if(px[z]!=drom_tolower(x[z]))return 0;return 1;}
int has_suffix(char*x,char*sx){int a=strlen(x),b=strlen(sx);if(b>a)return 0;for(int z=0;z<b;z++)if(sx[z]!=drom_tolower(x[a-b+z]))return 0;return 1;}
int read2(char*src,int off){int a=0xFF&src[off],b=0xFF&src[off+1];return (a<<8)|b;}
lv* lmpair(pair x){return lml2(lmn(x.x),lmn(x.y));}
lv* lmfpair(fpair x){return lml2(lmn(x.x),lmn(x.y));}
lv* lmrect(rect x){lv*r=lml(4);r->lv[0]=lmn(x.x),r->lv[1]=lmn(x.y),r->lv[2]=lmn(x.w),r->lv[3]=lmn(x.h);return r;}
pair pair_add(pair a,pair b){return (pair){a.x+b.x,a.y+b.y};}
pair pair_sub(pair a,pair b){return (pair){a.x-b.x,a.y-b.y};}
pair pair_max(pair a,pair b){return (pair){MAX(a.x,b.x),MAX(a.y,b.y)};}
pair pair_min(pair a,pair b){return (pair){MIN(a.x,b.x),MIN(a.y,b.y)};}
rect rect_add(rect a,pair b){return (rect){a.x+b.x,a.y+b.y,a.w,a.h};}
rect rect_sub(rect a,pair b){return (rect){a.x-b.x,a.y-b.y,a.w,a.h};}
rect rect_pair(pair a,pair b){return (rect){a.x,a.y,b.x,b.y};}
rect rect_max(rect a,rect b){return (rect){MAX(a.x,b.x),MAX(a.y,b.y),MAX(a.w,b.w),MAX(a.h,b.h)};}
int rect_same(rect a,rect b){return a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h;}
rect inset(rect r,int n){return (rect){r.x+n,r.y+n,r.w-2*n,r.h-2*n};}
rect box_union    (rect a,rect b){int x=MIN(a.x,b.x),y=MIN(a.y,b.y);return(rect){x,y,MAX(a.x+a.w,b.x+b.w)-x,MAX(a.y+a.h,b.y+b.h)-y};}
rect box_intersect(rect a,rect b){int x=MAX(a.x,b.x),y=MAX(a.y,b.y);return(rect){x,y,MIN(a.x+a.w,b.x+b.w)-x,MIN(a.y+a.h,b.y+b.h)-y};}
rect box_center   (rect a,pair b){return (rect){a.x+(a.w-b.x)/2,ceil(a.y+(a.h-b.y)/2.0),b.x,b.y};}
pair box_midpoint (rect a){return (pair){a.x+a.w/2,a.y+a.h/2};}
int box_in(rect r,pair p){return p.x>=r.x&&p.y>=r.y&&p.x<r.x+r.w&&p.y<r.y+r.h;}
int box_overlap(rect a,rect b){return b.x+b.w>=a.x&&b.x<=a.x+a.w&&b.y+b.h>=a.y&&b.y<=a.y+a.h;}
pair getpair(lv*z){if(!z||!lil(z))return(pair){0,0};    return(pair){z->c>0?ln(z->lv[0]):0,z->c>1?ln(z->lv[1]):0};}
rect getrect(lv*z){if(!z||!lil(z))return(rect){0,0,0,0};return(rect){z->c>0?ln(z->lv[0]):0,z->c>1?ln(z->lv[1]):0,z->c>2?ln(z->lv[2]):0,z->c>3?ln(z->lv[3]):0};}
lv* unpack_str(lv*z,int index){return index>=z->c?lmistr(""): ls(z->lv[index]);}
lv* unpack_name(lv*z,int index){return index>=z->c?LNIL: ls(z->lv[index]);}
pair unpack_pair(lv*z,int index){return index>=z->c?(pair){0,0}: getpair(ll(z->lv[index]));}
pair pcast(fpair a){return (pair){a.x,a.y};}
fpair getfpair(lv*z){return (fpair){lil(z)&&z->c>0?ln(z->lv[0]):0, lil(z)&&z->c>1?ln(z->lv[1]):0};}
fpair unpack_fpair(lv*z,int index){return index>=z->c?(fpair){0,0}: getfpair(ll(z->lv[index]));}
lv* normalize_pair(lv*x){pair p=getpair(x);p.x=CLAMP(0,p.x,4096),p.y=CLAMP(0,p.y,4096);return lmpair(p);}
lv* normalize_enum(lv*x,char**v){int i=0;if(x)x=ls(x);while(x&&v[i]){if(!strcmp(v[i],x->sv))return lmistr(v[i]);i++;}return lmistr(v[0]);}
int ordinal_enum  (lv*x,char**v){int i=0;if(x)x=ls(x);while(x&&v[i]){if(!strcmp(v[i],x->sv))return i;i++;}return 0;}
lv* normalize_ints(lv*x,int n){if(!x)return lml(0);x=ll(x);lv*r=lml(MIN(x->c,n));EACH(z,r)r->lv[z]=lmn((int)ln(x->lv[z]));return r;}
lv* str_read(lv*x,char*d){return x?ls(x):lmistr(d);}
lv* bool_read(lv*x,int d){return lmbool(x?lb(x):d);}
char*anchor[]={"top_left","top_center","top_right","center_left","center","center_right","bottom_left","bottom_center","bottom_right",NULL};
rect unpack_anchor(rect r,lv*z,int index){
	int a=index<z->c?ordinal_enum(z->lv[index],anchor):0;
	if(a==1||a==4||a==7)r.x-=r.w/2; // x center
	if(a==2||a==5||a==8)r.x-=r.w;   // x right
	if(a==3||a==4||a==5)r.y-=r.h/2; // y center
	if(a==6||a==7||a==8)r.y-=r.h;   // y bottom
	return r;
}
lv* ukey(lv*dict,lv*name,char*root,lv*original){
	if(original&&matchr(name,original))return name;
	if(name&&lis(name)&&dget(dict,name)==NULL)return name;
	char n[64];int i=1;while(1){
		snprintf(n,sizeof(n),"%s%d",root,i++);
		int f=0;EACH(z,dict)if(!strcmp(n,ls(dict->kv[z])->sv)){f=1;break;}
		if(!f)return lmcstr(n);
	}
}
lv* uset(lv*dict,lv*name,char*root,lv*val){return dset(dict,ukey(dict,name,root,NULL),val),val;}
void reorder(lv*dict,int old,int n){
	n=CLAMP(0,n,dict->c-1); // move element at index OLD to position IX.
	lv*k=dict->kv[old],*v=dict->lv[old];
	if(n<old){for(int z=old;z>n;z--)dict->kv[z]=dict->kv[z-1],dict->lv[z]=dict->lv[z-1];}
	else     {for(int z=old;z<n;z++)dict->kv[z]=dict->kv[z+1],dict->lv[z]=dict->lv[z+1];}
	dict->kv[n]=k,dict->lv[n]=v;
}
int is_rooted(lv*self){
	if(card_is(self)||prototype_is(self))return ivalue(self,"dead")==NULL;
	if(widget_is(self))return is_rooted(ivalue(self,"card"))&&ivalue(self,"dead")==NULL;
	return 1;
}

// Data Blocks

char base64_enc[65]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
signed char base64_dec[128]={
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // control chars
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,                                                 // + and /
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,-1,                                              // digits
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,           // uppercase
	26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,    // lowercase
};
lv* base64_read(lv*x,int base){
	lv*r=lms(3*(x->c/4));r->c=0;for(int z=base;z<x->c;z+=4){
		int c[4]={0},o=0;for(;o<4&&z+o<x->c;o++){c[o]=base64_dec[(int)x->sv[z+o]];if(c[o]==-1)break;}
		if(o>=2)r->sv[r->c++]=(c[0]<<2)|(c[1]>>4);
		if(o>=3)r->sv[r->c++]=(c[1]<<4)|(c[2]>>2);
		if(o>=4)r->sv[r->c++]=(c[2]<<6)|(c[3]   );
	}return r;
}
lv* base64_write(str r,lv*x){
	for(int z=0;z<x->c;z+=3){
		str_addc(&r,base64_enc[              (((0xFF&x->sv[z  ])>>2)                                     )&0x3F]);
		str_addc(&r,base64_enc[              (((0xFF&x->sv[z  ])<<4)|((z+1>=x->c?0:(0xFF&x->sv[z+1]))>>4))&0x3F]);
		str_addc(&r,base64_enc[z+1>=x->c?64: (((0xFF&x->sv[z+1])<<2)|((z+2>=x->c?0:(0xFF&x->sv[z+2]))>>6))&0x3F]);
		str_addc(&r,base64_enc[z+2>=x->c?64: (                       (             (0xFF&x->sv[z+2]))    )&0x3F]);
	}return lmstr(r);
}
lv* data_read(char*type,char*f,lv*x){return !x||x->c<6||memcmp(x->sv,"%%",2)||memcmp(x->sv+2,type,3)?NULL: (*f=x->sv[5],base64_read(x,6));}
lv* data_write(char*type,char f,lv*x){str r=str_new();return str_addz(&r,"%%"),str_addz(&r,type),str_addc(&r,f),base64_write(r,x);}

// Image interface

lv*lmbuff(pair x){lv*r=lms(x.x*x.y);r->n=x.x;return r;}
pair buff_size(lv*x){if(x->n==0||x->c==0)return (pair){0,0};return(pair){x->n,x->c/x->n};}
rect find_occupied(lv*buff,int mask){
	pair s=buff_size(buff);rect d={s.x,s.y,0,0};for(int z=0;z<buff->c;z++){
		if(buff->sv[z]==mask)continue;int x=z%s.x, y=z/s.x; d.x=MIN(d.x,x), d.y=MIN(d.y,y), d.w=MAX(d.w,x), d.h=MAX(d.h,y);
	}d.w-=d.x,d.h-=d.y,d.w++,d.h++;return d;
}
lv* buffer_copy(lv*src,rect r){
	lv*c=lmbuff((pair){r.w,r.h});pair size=buff_size(src);rect clip={0,0,size.x,size.y};
	for(int y=0;y<r.h;y++)for(int x=0;x<r.w;x++)c->sv[x+r.w*y]=box_in(clip,(pair){r.x+x,r.y+y})?src->sv[(r.x+x)+size.x*(r.y+y)]:0;
	return c;
}
pair image_size(lv*x){return buff_size(x->b);}
lv* image_resize(lv*x,pair size){
	pair os=image_size(x);char*old=x->b->sv;size.x=MAX(0,size.x),size.y=MAX(0,size.y);if(os.x==size.x&&os.y==size.y)return x;
	x->b=lmbuff(size);for(int a=0;a<size.y;a++)for(int b=0;b<size.x;b++)x->b->sv[b+a*size.x]=a>=os.y||b>=os.x?0: old[b+a*os.x];return x;
}
void buffer_dither(lv*r){
	pair size=buff_size(r); int stride=2*size.x; int m[]={0,1,size.x-2,size.x-1,size.x,stride-1};
	float*e=calloc(stride,sizeof(float));int ei=0; for(int z=0;z<r->c;z++){
		float pix=((0xFF&(int)r->sv[z])/256.0)+e[ei], col=pix>.5?1:0, err=(pix-col)/8.0;
		e[ei]=0, ei=(ei+1)%stride; for(int x=0;x<6;x++)e[(ei+m[x])%stride]+=err; r->sv[z]=!col;
	}free(e);
}
void buffer_flip_h(lv*x){pair s=buff_size(x);for(int z=0;z<s.y;z++){int a=z*s.x,b=(z+1)*s.x-1;while(a<b){char t=x->sv[a];x->sv[a]=x->sv[b];x->sv[b]=t;a++;b--;}}}
void buffer_flip_v(lv*x){pair s=buff_size(x);for(int z=0;z<s.x;z++){int a=z,b=z+s.x*(s.y-1);while(a<b){char t=x->sv[a];x->sv[a]=x->sv[b];x->sv[b]=t;a+=s.x;b-=s.x;}}}
lv* buffer_transpose(lv*x){pair s=buff_size(x);lv*r=lmbuff((pair){s.y,s.x});for(int a=0;a<s.y;a++)for(int b=0;b<s.x;b++)r->sv[a+s.y*b]=x->sv[b+s.x*a];return r;}
lv* n_buffer_map(lv*self,lv*z){
	int m[256]={0};if(z->c>1){int f=ln(z->lv[1]);for(int z=0;z<256;z++)m[z]=f;}else{for(int z=0;z<256;z++)m[z]=z;}
	z=ld(l_first(z));EACH(i,z){int k=ln(z->kv[i]);if(k>=-128&&k<=255)m[0xFF&k]=0xFF&(int)ln(z->lv[i]);}
	EACH(z,self->b)self->b->sv[z]=m[0xFF&(int)self->b->sv[z]];return self;
}
lv* buffer_hist(lv*x,int sign){
	#define b_extend(u) ((u)|(0-((u)&0x80)))
	lv*r=lmd();double c[256]={0.0};EACH(z,x)c[0xFF&(int)(x->sv[z])]++;
	for(int z=0;z<256;z++)if(c[z]!=0)dset(r,lmn(sign?b_extend(z):z),lmn(c[z]));return r;
}
lv* buffer_bounds(lv*b){
	pair s=buff_size(b);rect d={s.x,s.y,0,0};
	for(int z=0;z<b->c;z++)if(b->sv[z]!=0){int x=z%s.x, y=z/s.x; d.x=MIN(d.x,x), d.y=MIN(d.y,y), d.w=MAX(d.w,x), d.h=MAX(d.h,y);}
	pair rp={d.x,d.y},rs={MIN(s.x,d.w-d.x+1),MIN(s.y,d.h-d.y+1)};
	lv*r=lmd();dset(r,lmistr("pos"),lmpair(rp)),dset(r,lmistr("size"),lmpair(rs));return r;
}
int is_empty(lv*x){pair s=image_size(x);return s.x==0&&s.y==0;}
int is_blank(lv*x){if(!image_is(x))return 0;EACH(z,x->b)if(x->b->sv[z])return 0;return 1;}
void buff_merge_op(lv*target,lv*src,char op){
	pair ts=buff_size(target),bs=buff_size(src);char*t=target->sv,*b=src->sv;if(bs.x==0||bs.y==0)return;
	#define op_kernel for(int y=0,i=0;y<ts.y;y++)for(int x=0;x<ts.x;x++,i++)
	#define op_index  (0xFF&b[(x%bs.x)+(y%bs.y)*bs.x])
	if(op=='+'){op_kernel t[i]=0xFF&((0xFF&t[i])+op_index)             ;}
	if(op=='-'){op_kernel t[i]=0xFF&((0xFF&t[i])-op_index)             ;}
	if(op=='*'){op_kernel t[i]=0xFF&((0xFF&t[i])*op_index)             ;}
	if(op=='&'){op_kernel {int c=op_index;t[i]=0xFF&MIN((0xFF&t[i]),c);}}
	if(op=='|'){op_kernel {int c=op_index;t[i]=0xFF&MAX((0xFF&t[i]),c);}}
	if(op=='<'){op_kernel t[i]=(0xFF&t[i]) <op_index                   ;}
	if(op=='>'){op_kernel t[i]=(0xFF&t[i]) >op_index                   ;}
	if(op=='='){op_kernel t[i]=(0xFF&t[i])==op_index                   ;}
}
lv* n_image_merge(lv*self,lv*z){
	if(lis(l_first(z))){if(z->c>=2&&image_is(z->lv[1]))buff_merge_op(self->b,z->lv[1]->b,ls(l_first(z))->sv[0]);return self;}
	if(lil(l_first(z)))z=l_first(z);pair s=image_size(self);char*pp=self->b->sv;int v[256]={0};pair ss[256]={{0}};char*b[256]={0};
	for(int p=0;p<z->c&&p<256;p++)if(image_is(z->lv[p])&&!is_empty(z->lv[p]))v[p]=1,ss[p]=image_size(z->lv[p]),b[p]=z->lv[p]->b->sv;
	for(int y=0,i=0;y<s.y;y++)for(int x=0;x<s.x;x++,i++){int p=0xFF&pp[i],c=v[p]?b[p][(x%ss[p].x)+(y%ss[p].y)*ss[p].x]:0;pp[i]=c;}
	return self;
}
lv* n_image_transform(lv*self,lv*z){
	z=ls(l_first(z));
	if     (!strcmp("horiz" ,z->sv))buffer_flip_h(self->b);
	else if(!strcmp("vert"  ,z->sv))buffer_flip_v(self->b);
	else if(!strcmp("flip"  ,z->sv))self->b=buffer_transpose(self->b);
	else if(!strcmp("left"  ,z->sv))buffer_flip_h(self->b),self->b=buffer_transpose(self->b);
	else if(!strcmp("right" ,z->sv))self->b=buffer_transpose(self->b),buffer_flip_h(self->b);
	else if(!strcmp("dither",z->sv))buffer_dither(self->b);
	return self;
}
void buffer_shear_x(lv*b,lv*t,float shear){
	pair s=buff_size(b);
	for(int y=0;y<s.y;y++){int o=(y-(s.y/2))*shear;for(int x=0;x<s.x;x++)t->sv[x+y*s.x]=b->sv[mod(x+o,s.x)+y*s.x];}
	memcpy(b->sv,t->sv,b->c);
}
void buffer_shear_y(lv*b,lv*t,float shear){
	pair s=buff_size(b);
	for(int x=0;x<s.x;x++){int o=(x-(s.x/2))*shear;for(int y=0;y<s.y;y++)t->sv[x+y*s.x]=b->sv[x+mod(y+o,s.y)*s.x];}
	memcpy(b->sv,t->sv,b->c);
}
lv* n_image_rotate(lv*self,lv*z){
	float n=-fmod(ln(l_first(z)),2*MATH_PI);
	if(fabs(n)>MATH_PI/2&&fabs(n)<MATH_PI*3/2.0){buffer_flip_v(self->b),buffer_flip_h(self->b),n+=(n<0?1:-1)*MATH_PI;}
	lv*tmp=lmbuff(buff_size(self->b));
	buffer_shear_x(self->b,tmp,-tan(n/2));
	buffer_shear_y(self->b,tmp, sin(n  ));
	buffer_shear_x(self->b,tmp,-tan(n/2));
	return self;
}
void buffer_trans(lv*b,pair o){
	pair s=buff_size(b);lv*t=lmbuff(s);
	for(int y=0,z=0;y<s.y;y++)for(int x=0;x<s.x;x++,z++){pair i={x-o.x,y-o.y};t->sv[z]=(i.x<0||i.x>=s.x||i.y<0||i.y>=s.y)?0: b->sv[i.x+i.y*s.x];}
	memcpy(b->sv,t->sv,b->c);
}
void buffer_wrap(lv*b,pair o){
	pair s=buff_size(b);lv*t=lmbuff(s);
	for(int y=0,z=0;y<s.y;y++)for(int x=0;x<s.x;x++,z++)t->sv[z]=b->sv[mod(x-o.x,s.x)+mod(y-o.y,s.y)*s.x];
	memcpy(b->sv,t->sv,b->c);
}
lv* n_image_translate(lv*self,lv*z){
	pair o=getpair(l_first(z));
	if(o.x==0&&o.y==0){}else if(z->c>=2&&lb(z->lv[1])){buffer_wrap(self->b,o);}else{buffer_trans(self->b,o);}return self;
}
lv* image_make(lv*buffer);lv* image_read(lv*x); // forward refs
lv* n_image(lv*self,lv*z){if(lis(l_first(z)))return image_read(l_first(z));return image_make(lmbuff(unpack_pair(z,0)));(void)self;}
lv* image_empty(void){return image_make(lmbuff((pair){0,0}));}
lv* buffer_clone(lv*x){pair size=buff_size(x);return buffer_copy(x,(rect){0,0,size.x,size.y});}
lv* image_clone(lv*x){pair size=image_size(x);return image_make(buffer_copy(x->b,(rect){0,0,size.x,size.y}));}
lv* unpack_image(lv*z,int i){return (i>=z->c||!image_is(z->lv[i]))?image_empty():z->lv[i];}
void buffer_paste(rect r,rect cl,lv*src,lv*dst,int opaque){
	pair ss=buff_size(src),ds=buff_size(dst);
	for(int y=0;y<ss.y;y++)for(int x=0;x<ss.x;x++)if(box_in(cl,(pair){r.x+x,r.y+y})&&(opaque||src->sv[x+ss.x*y]))dst->sv[r.x+x+ds.x*(r.y+y)]=src->sv[x+ss.x*y];
}

#define EPSILON      0.000001
#define lerp_scale() fpair sc={MAX(EPSILON,(r.w*1.0)/(s.x==0?EPSILON:s.x)),MAX(EPSILON,(r.h*1.0)/(s.y==0?EPSILON:s.y))};

void buffer_paste_scaled(rect r,rect cl,lv*src,lv*dst,int opaque){
	if(r.w==0||r.h==0)return;pair s=buff_size(src),ds=buff_size(dst);lerp_scale();
	if(r.w==s.x&&r.h==s.y){buffer_paste(r,cl,src,dst,opaque);return;}
	for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
		int sx=b/sc.x, sy=a/sc.y, c=src->sv[sx+sy*s.x];
		if((opaque||c!=0)&&box_in(cl,(pair){r.x+b,r.y+a}))dst->sv[r.x+b+ds.x*(r.y+a)]=c;
	}
}
lv* n_image_copy(lv*self,lv*z){
	rect v=rect_pair((pair){0,0},image_size(self));
	if(z->c>=1){pair a=unpack_pair(z,0),b=unpack_pair(z,1);if(b.x<0){a.x+=1+b.x;b.x*=-1;}if(b.y<0){a.y+=1+b.y;b.y*=-1;}v=rect_pair(a,b);}
	return image_make(buffer_copy(self->b,unpack_anchor(v,z,2)));
}
lv* n_image_paste(lv*self,lv*z){
	lv*c=unpack_image(z,0),*pos=z->c>=2?ll(z->lv[1]):lml(0);int solid=z->c>=3?!lb(z->lv[2]):1;rect cl=rect_pair((pair){0,0},image_size(self));
	if(c==self)c=image_clone(c);
	if(pos->c<=2){buffer_paste(rect_pair(getpair(pos),image_size(c)),cl,c->b,self->b,solid);}
	else{buffer_paste_scaled(getrect(pos),cl,c->b,self->b,solid);}return self;
}
lv* n_image_scale(lv*self,lv*z){
	lv*o=image_clone(self);int a=z->c>1&&lb(z->lv[1]);z=l_first(z);fpair n=lin(z)?(fpair){ln(z),ln(z)}: getfpair(z);pair s=image_size(self);
	pair r={MAX(0,n.x*(a?1:s.x)),MAX(0,n.y*(a?1:s.y))};
	image_resize(self,r);buffer_paste_scaled(rect_pair((pair){0,0},r),rect_pair((pair){0,0},r),o->b,self->b,1);return self;
}
lv* n_image_outline(lv*self,lv*z){
	int p=ln(l_first(z));if(p<1||p>47)return self;pair s=image_size(self);lv*target=self->b, *t=buffer_clone(target);
	for(int a=0,i=0;a<s.y;a++)for(int b=0;b<s.x;b++,i++){
		if(t->sv[i])continue;int n=0;
		if(b>0    )n|=t->sv[(b-1)+(a  )*s.x];
		if(b<s.x-1)n|=t->sv[(b+1)+(a  )*s.x];
		if(a>0    )n|=t->sv[(b  )+(a-1)*s.x];
		if(a<s.y-1)n|=t->sv[(b  )+(a+1)*s.x];
		if(n)target->sv[i]=p;
	}return self;
}
lv* image_write(lv*x){
	x=image_is(x)?x:image_empty();pair s=image_size(x);str t=str_new();char f;int colors=0;for(int z=0;z<s.x*s.y;z++)if(x->b->sv[z]>1)colors=1;
	str_addraw(&t,(s.x>>8)&0xFF),str_addraw(&t,s.x&0xFF);str_addraw(&t,(s.y>>8)&0xFF),str_addraw(&t,s.y&0xFF);
	str l=str_new();for(int z=0;z<4;z++)str_addraw(&l,t.sv[z]);
	for(int z=0;z<x->b->c;){int c=0,p=x->b->sv[z];while(c<255&&z<x->b->c&&x->b->sv[z]==p)c++,z++;str_addraw(&l,p),str_addraw(&l,c);}
	if(!colors&&(l.c>4+s.x*s.y/8)){
		f='0';int stride=8*ceil(s.x/8.0);for(int a=0;a<s.y;a++)for(int b=0;b<stride;b+=8)
		{int v=0;for(int i=0;i<8;i++)v=(v<<1)|(b+i>=s.x?0: x->b->sv[b+i+a*s.x]?1:0);str_addraw(&t,v);}
	}else if(l.c>4+s.x*s.y){f='1';free(l.sv);for(int z=0;z<s.x*s.y;z++)str_addraw(&t,x->b->sv[z]);}else{f='2';free(t.sv);t=l;}
	lv*ts=lmv(1);ts->c=t.c;ts->sv=t.sv;return data_write("IMG",f,ts);
}
lv* interface_image(lv*self,lv*i,lv*x){
	pair s=image_size(self);
	if(i&&lil(i)){ // read/write single pixels
		pair p=getpair(i);int ib=p.x>=0&&p.y>=0&&p.x<s.x&&p.y<s.y;
		if(x){if(ib)self->b->sv[p.x+p.y*s.x]=0xFF&(int)ln(x);return x;}
		return ib?lmn(0xFF&(self->b->sv[p.x+p.y*s.x])):LNIL;
	}
	ikey("pixels"){ // read/write all pixels
		if(x){lv*t=l_raze(ll(x));EACH(z,t){if(z>=self->b->c)break;self->b->sv[z]=0xFF&(int)ln(t->lv[z]);}return x;}
		lv*r=lml(s.y);for(int y=0;y<s.y;y++){GEN(t,s.x)lmn(0xFF&(self->b->sv[z+y*s.x]));r->lv[y]=t;}return r;
	}
	ikey("size"     ){if(x){image_resize(self,getpair(x));return x;}return lmpair(s);}
	ikey("map"      )return lmnat(n_buffer_map,self);
	ikey("merge"    )return lmnat(n_image_merge,self);
	ikey("transform")return lmnat(n_image_transform,self);
	ikey("rotate"   )return lmnat(n_image_rotate,self);
	ikey("translate")return lmnat(n_image_translate,self);
	ikey("scale"    )return lmnat(n_image_scale,self);
	ikey("outline"  )return lmnat(n_image_outline,self);
	ikey("copy"     )return lmnat(n_image_copy,self);
	ikey("paste"    )return lmnat(n_image_paste,self);
	ikey("encoded"  )return image_write(self);
	ikey("hist"     )return buffer_hist(self->b,0);
	ikey("bounds"   )return buffer_bounds(self->b);
	return x?x:LNIL;
}
lv* image_make(lv*buffer){return lmi(interface_image,lmistr("image"),buffer);}
lv* image_read(lv*x){
	char f=0;lv*data=data_read("IMG",&f,x);if(!data||data->c<4)return image_empty();
	int w=read2(data->sv,0),h=read2(data->sv,2);lv*r=lmbuff((pair){w,h});
	if(f=='0'&&data->c-4>=w*h/8){int s=ceil(w/8.0),o=0;for(int a=0;a<h;a++)for(int b=0;b<w;b++)r->sv[o++]=data->sv[4+(b/8)+a*s]&(1<<(7-(b%8)))?1:0;}
	if(f=='1'&&data->c-4>=w*h)memcpy(r->sv,data->sv+4,w*h);
	if(f=='2'){int i=4,o=0;while(i+2<=data->c){int p=data->sv[i++],c=0xFF&data->sv[i++];while(c&&o+1<=r->c)c--,r->sv[o++]=p;}}
	return image_make(r);
}
void buffer_overlay(lv*dst,lv*src,int mask,pair offset){
	pair ss=buff_size(src),sd=buff_size(dst);rect d=rect_pair((pair){0,0},sd);for(int b=0;b<ss.y;b++)for(int a=0;a<ss.x;a++){
		int c=src->sv[a+b*ss.x];pair p=(pair){a+offset.x,b+offset.y};if(c!=mask&&box_in(d,p))dst->sv[p.x+p.y*sd.x]=c;
	}
}
lv* buffer_mask(lv*src,lv*mask){lv*r=buffer_clone(src);EACH(z,mask)if(!mask->sv[z])r->sv[z]=0;return r;}

// Font interface

char*FONT_BLOCK_MENU=
	"%%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"
	"D/CAAAfwBjAF0AXQB7AHcAdwB/AHcAfwAAAAAA";
char*FONT_BLOCK_BODY=
	"%%FNT1CAoBIAEAAAAAAAAAAAAAIQEAgICAgIAAgAAAIgMAoKAAAAAAAAAAIwUAAFD4UPhQAAAAJAUgcKigcCiocCAAJQg"
	"Af5KUbhkpRgAAJgcwSFAgVIiUYgAAJwEAgIAAAAAAAAAAKAMgQICAgICAQCAAKQOAQCAgICAgQIAAKgUAUCD4IFAAAAAA"
	"KwUAACAg+CAgAAAALAIAAAAAAAAAQECALQQAAAAA8AAAAAAALgEAAAAAAAAAgAAALwQQECAgQECAgAAAMAUAcIiIiIiIc"
	"AAAMQUAIGAgICAgIAAAMgUAcIgIECBA+AAAMwUA+BAgcAiIcAAANAUAEDBQkPgQEAAANQUA+IDwCAiIcAAANgUAMECA8I"
	"iIcAAANwUA+AgQECAgIAAAOAUAcIiIcIiIcAAAOQUAcIiIeAgQYAAAOgIAAABAAAAAQAAAOwMAAAAgAAAAICBAPAQAABA"
	"gQCAQAAAAPQUAAAD4APgAAAAAPgQAAEAgECBAAAAAPwUAMEgIECAAIAAAQAcAOESaqqqcQDgAQQUAICBQUPiIiAAAQgUA"
	"8IiI8IiI8AAAQwUAcIiAgICIcAAARAUA4JCIiIiQ4AAARQQA8ICA4ICA8AAARgQA8ICA4ICAgAAARwUAcIiAmIiIcAAAS"
	"AUAiIiI+IiIiAAASQIAQEBAQEBAQAAASgUACAgICIiIcAAASwUAiJCgwKCQiAAATAQAgICAgICA8AAATQcAgsaqkoKCgg"
	"AATgUAyMioqJiYiAAATwUAcIiIiIiIcAAAUAUA8IiI8ICAgAAAUQUAcIiIiIiocBAAUgUA8IiI8KCQiAAAUwUAcIiAcAi"
	"IcAAAVAUA+CAgICAgIAAAVQUAiIiIiIiIcAAAVgUAiIiIUFAgIAAAVwcAgoJUVCgoKAAAWAUAiIhQIFCIiAAAWQUAiIhQ"
	"ICAgIAAAWgQA8BAgQICA8AAAWwNgQEBAQEBAQGAAXASAgEBAICAQEAAAXQNgICAgICAgIGAAXgMAQKAAAAAAAAAAXwYAA"
	"AAAAAAA/AAAYAIAgEAAAAAAAAAAYQQAAABgEHCQcAAAYgQAgIDgkJCQ4AAAYwQAAABgkICQYAAAZAQAEBBwkJCQcAAAZQ"
	"QAAABgkPCAYAAAZgQAMEDgQEBAQAAAZwQAAABwkJCQcBBgaAQAgIDgkJCQkAAAaQIAQADAQEBAQAAAagMAIABgICAgICD"
	"AawQAgICQoMCgkAAAbAIAwEBAQEBAQAAAbQcAAADskpKSkgAAbgQAAADgkJCQkAAAbwQAAABgkJCQYAAAcAQAAADgkJCQ"
	"4ICAcQQAAABwkJCQcBAQcgQAAACwwICAgAAAcwQAAABwgGAQ4AAAdAMAQEDgQEBAIAAAdQQAAACQkJCQcAAAdgUAAACIU"
	"FAgIAAAdwcAAACCVFQoKAAAeAUAAACIUCBQiAAAeQQAAACQkJCQcBBgegQAAADwIECA8AAAewMgQEBAgEBAQCAAfAGAgI"
	"CAgICAgIAAfQOAQEBAIEBAQIAAfgUAaLAAAAAAAAAAfwUAAAAAAAAAqAAAgAVAIAAgUFD4iAAAgQUQIAAgUFD4iAAAggU"
	"gUAAgUFD4iAAAgwUoUAAgUFD4iAAAhAVQACAgUFD4iAAAhQUgUCAgUFD4iAAAhgYAPDBQUPiQnAAAhwUAcIiAgICIcCBg"
	"iARAIADwgOCA8AAAiQQgQADwgOCA8AAAigQgUADwgOCA8AAAiwRQAPCA4ICA8AAAjAKAQABAQEBAQAAAjQMgQABAQEBAQ"
	"AAAjgNAoABAQEBAQAAAjwOgAEBAQEBAQAAAkAUA4JCI6IiQ4AAAkQUoUADIqKiYmAAAkgVAIHCIiIiIcAAAkwUQIHCIiI"
	"iIcAAAlAUgUHCIiIiIcAAAlQUoUHCIiIiIcAAAlgVQAHCIiIiIcAAAlwcAOkRMVGREuAAAmAVAIIiIiIiIcAAAmQUQIIi"
	"IiIiIcAAAmgUgUACIiIiIcAAAmwVQAIiIiIiIcAAAnAUQIIiIUCAgIAAAnQUAgPCIiPCAgAAAngUAcIiwiIiIsAAAnwRA"
	"IABgEHCQcAAAoAQgQABgEHCQcAAAoQQgUABgEHCQcAAAogRQoABgEHCQcAAAowQAUABgEHCQcAAApAQgUCBgEHCQcAAAp"
	"QcAAAB8En6QfAAApgQAAABgkICQYCBgpwRAIABgkPCAYAAAqAQgQABgkPCAYAAAqQQgUABgkPCAYAAAqgQAUABgkPCAYA"
	"AAqwKAQADAQEBAQAAArAMgQADAQEBAQAAArQNAoADAQEBAQAAArgMAoADAQEBAQAAArwUAaBAoeIiIcAAAsARQoADgkJC"
	"QkAAAsQRAIABgkJCQYAAAsgQgQABgkJCQYAAAswQgUABgkJCQYAAAtARQoABgkJCQYAAAtQQAUABgkJCQYAAAtgYAAAA0"
	"SHhIsAAAtwRAIACQkJCQcAAAuAQgQACQkJCQcAAAuQQgUACQkJCQcAAAugQAUACQkJCQcAAAuwQgQACQkJCQcBBgvAUAA"
	"ICwyIjIsIAAvQQAUACQkJCQcBBgvgVwICBQUPiIiAAAvwQAcABgEHCQcAAAwAVQcCAgUFD4iAAAwQRQcABgEHCQcAAAwg"
	"UAICBQUPiIiBAIwwQAAABgEHCQcCAQxAUQIHCIgICIcAAAxQQgQABgkICQYAAAxgRwAPCA4ICA8AAAxwQAcABgkPCAYAA"
	"AyAQA8ICA4ICA8CAQyQQAAABgkPCAYEAgygPgAEBAQEBAQAAAywMA4ADAQEBAQAAAzAIAAADAQEBAQAAAzQUAQEBQYMBA"
	"eAAAzgUAYCgwYKAgIAAAzwUQIMjIqKiYmAAA0AQgQADgkJCQkAAA0QVwAHCIiIiIcAAA0gQAcABgkJCQYAAA0wVIkHCIi"
	"IiIcAAA1AVIkABgkJCQYAAA1QcAfpCQnJCQfgAA1gYAAAB4pLygeAAA1wUQIHiAcAiIcAAA2AQgQABwgGAQ4AAA2QVQIH"
	"iAcAiIcAAA2gRQIABwgGAQ4AAA2wVwAIiIiIiIcAAA3AQAcACQkJCQcAAA3QVIkACIiIiIcAAA3gVIkACQkJCQcAAA3wV"
	"QAIiIUCAgIAAA4AQgQPAQIECA8AAA4QQgQADwIECA8AAA4gQgAPAQIECA8AAA4wQAIADwIECA8AAA5ARQIPAQIECA8AAA"
	"5QRQIADwIECA8AAA5gUAcIiAcAiIcCBA5wQAAABwgGAQ4CBA6AUA+CAgICAgACAg6QMAQEDgQEBAIEBA6gUA+IiQsIiIs"
	"AAA6wEAAIAAgICAgIAA7AUAABAAECBASDAA7QUAAChQoFAoAAAA7gUAAKBQKFCgAAAA7wYAOETwQPBEOAAA8AUwSEgwAA"
	"AAAAAA/wYA/My05Pzs/AAA";
char*FONT_BLOCK_MONO=
	"%%FNT1BQsBIAUAAAAAAAAAAAAAACEFAAAgICAgIAAgAAAiBQAAUFBQAAAAAAAAIwUAAFD4UPhQAAAAACQFACBwqKBwKKh"
	"wIAAlBQAASKhQIFCokAAAJgUAAGCQoECokGgAACcFACAgIAAAAAAAAAAoBQAQICBAQEAgIBAAKQUAIBAQCAgIEBAgACoF"
	"AAAgqHCoIAAAAAArBQAAACAg+CAgAAAALAUAAAAAAAAAYGAgQC0FAAAAAAD4AAAAAAAuBQAAAAAAAAAwMAAALwUICBAQI"
	"CBAQICAADAFAABwiJioyIhwAAAxBQAAIGAgICAgIAAAMgUAAHCICBAgQPgAADMFAABwiAgwCIhwAAA0BQAAEDBQkPgQEA"
	"AANQUAAPiA8AgIiHAAADYFAABwgPCIiIhwAAA3BQAA+AgIECAgIAAAOAUAAHCIiHCIiHAAADkFAABwiIiIeAhwAAA6BQA"
	"AADAwAAAwMAAAOwUAAABgYAAAYGAgQDwFAAAIECBAIBAIAAA9BQAAAAD4APgAAAAAPgUAAEAgEAgQIEAAAD8FAABwiAgQ"
	"IAAgAABABQBwiIio6LCAiHAAQQUAAHCIiPiIiIgAAEIFAADwiIjwiIjwAABDBQAAcIiAgICIcAAARAUAAPCIiIiIiPAAA"
	"EUFAAD4gIDwgID4AABGBQAA+ICA8ICAgAAARwUAAHCIgJiIiHAAAEgFAACIiIj4iIiIAABJBQAAICAgICAgIAAASgUAAA"
	"gICAiIiHAAAEsFAACIkKDAoJCIAABMBQAAgICAgICA+AAATQUAAIjYqIiIiIgAAE4FAACIyKiYiIiIAABPBQAAcIiIiIi"
	"IcAAAUAUAAPCIiPCAgIAAAFEFAABwiIiIiIhwCABSBQAA8IiI8IiIiAAAUwUAAHCIgHAIiHAAAFQFAAD4ICAgICAgAABV"
	"BQAAiIiIiIiIcAAAVgUAAIiIiFBQICAAAFcFAACIiIiIqNiIAABYBQAAiFAgICBQiAAAWQUAAIiIiFAgICAAAFoFAAD4C"
	"BAgQID4AABbBQAwICAgICAgIDAAXAWAgEBAICAQEAgIAF0FADAQEBAQEBAQMABeBQAgUIgAAAAAAAAAXwUAAAAAAAAAAP"
	"gAAGAFAEAgEAAAAAAAAABhBQAAAAB4iIiYaAAAYgUAAICA8IiIiPAAAGMFAAAAAHCIgIB4AABkBQAACAh4iIiIeAAAZQU"
	"AAAAAcIj4gHgAAGYFAAAYIHAgICAgAABnBQAAAAB4iIiIeAhwaAUAAICA8IiIiIgAAGkFAAAgACAgICAgAABqBQAAIAAg"
	"ICAgICDAawUAAICAkKDgkIgAAGwFAAAgICAgICAwAABtBQAAAADwqKioqAAAbgUAAAAAsMiIiIgAAG8FAAAAAHCIiIhwA"
	"ABwBQAAAADwiIiI8ICAcQUAAAAAeIiIiHgICHIFAAAAALDIgICAAABzBQAAAAB4gHAI8AAAdAUAACAgeCAgIBgAAHUFAA"
	"AAAIiIiJhoAAB2BQAAAACIiFBQIAAAdwUAAAAAqKioqFAAAHgFAAAAAIhQIFCIAAB5BQAAAACIiIiIeAhwegUAAAAA+BA"
	"gQPgAAHsFABggICDAICAgGAB8BSAgICAgICAgICAgfQUAwCAgIBggICDAAH4FAABosAAAAAAAAAB/BQAAAAAAAAAAqAAA"
	"gAVAIHCIiPiIiIgAAIEFECBwiIj4iIiIAACCBSBQcIiI+IiIiAAAgwUoUHCIiPiIiIgAAIQFUABwiIj4iIiIAACFBSBQI"
	"FCI+IiIiAAAhgUAADhQUPiQkJgAAIcFAABwiICAgIhwIGCIBUAg+ICA8ICA+AAAiQUQIPiAgPCAgPgAAIoFIFD4gIDwgI"
	"D4AACLBVAA+ICA8ICA+AAAjAVAIAAgICAgICAAAI0FECAAICAgICAgAACOBSBQACAgICAgIAAAjwVQACAgICAgICAAAJA"
	"FAADwiIjoiIjwAACRBShQiMiomIiIiAAAkgVAIHCIiIiIiHAAAJMFECBwiIiIiIhwAACUBSBQcIiIiIiIcAAAlQUoUHCI"
	"iIiIiHAAAJYFUABwiIiIiIhwAACXBQAAaJCoqKhIsAAAmAVAIIiIiIiIiHAAAJkFECCIiIiIiIhwAACaBSBQAIiIiIiIc"
	"AAAmwVQAIiIiIiIiHAAAJwFECCIiIhQICAgAACdBQAAgPCIiPCAgAAAngUAAOCQoJCIiLAAAJ8FAEAgAHiIiJhoAACgBQ"
	"AQIAB4iIiYaAAAoQUAIFAAeIiImGgAAKIFAChQAHiIiJhoAACjBQAAUAB4iIiYaAAApAUAIFAgeIiImGgAAKUFAAAAAHC"
	"ouKBYAACmBQAAAABwiICAeCBgpwUAQCAAcIj4gHgAAKgFABAgAHCI+IB4AACpBQAgUABwiPiAeAAAqgUAAFAAcIj4gHgA"
	"AKsFAEAgACAgICAgAACsBQAQIAAgICAgIAAArQUAIFAAICAgICAAAK4FAABQAGAgICAgAACvBQBoECh4iIiIcAAAsAUAK"
	"FAAsMiIiIgAALEFAEAgAHCIiIhwAACyBQAQIABwiIiIcAAAswUAIFAAcIiIiHAAALQFAChQAHCIiIhwAAC1BQAAUABwiI"
	"iIcAAAtgUAAABokKioSLAAALcFAEAgAIiIiJhoAAC4BQAQIACIiIiYaAAAuQUAIFAAiIiImGgAALoFAABQAIiIiJhoAAC"
	"7BQAQIACIiIiIeAhwvAUAAICwyIjIsIAAAL0FAABQAIiIiIh4CHC+BXAAcIiI+IiIiAAAvwUAAHAAeIiImGgAAMAFUCBw"
	"iIj4iIiIAADBBQBQIAB4iIiYaAAAwgUAAHCIiPiIiIgQCMMFAAAAAHiIiJhoEAjEBRAgcIiAgICIcAAAxQUAABAgcIiAg"
	"HgAAMYFcAD4gIDwgID4AADHBQAAcABwiPiAeAAAyAUAAPiAgPCAgPAQCMkFAAAAAHCI+IBwEAjKBXAAICAgICAgIAAAyw"
	"UAAHAAYCAgICAAAMwFAAAAAGAgICAgAADNBQAAQFBgQMBAeAAAzgUAACAgKDBgoDAAAM8FECCIyKiYiIiIAADQBQAQIAC"
	"wyIiIiAAA0QVwAHCIiIiIiHAAANIFAABwAHCIiIhwAADTBUiQAHCIiIiIcAAA1AUASJAAcIiIiHAAANUFAAB4oKCwoKB4"
	"AADWBQAAAABQqLigWAAA1wUQIHCIgHAIiHAAANgFABAgAHiAcAjwAADZBVAgcIiAcAiIcAAA2gUAUCAAeIBwCPAAANsFc"
	"ACIiIiIiIhwAADcBQAAcACIiIiYaAAA3QVIkACIiIiIiHAAAN4FAEiQAIiIiJhoAADfBVAAiIiIUCAgIAAA4AUQIPgIEC"
	"BAgPgAAOEFABAgAPgQIED4AADiBSAA+AgQIECA+AAA4wUAACAA+BAgQPgAAOQFUCD4CBAgQID4AADlBQBQIAD4ECBA+AA"
	"A5gUAcIiAcAiIcAAgIOcFAAAAeIBwCPAAICDoBQAA+CAgICAgACAg6QUAACAgeCAgGAAgIOoFAAD4iJCgkIiwAADrBQAA"
	"ACAAICAgICAA7AUAAAAgACBAgIhwAO0FAAAAKFCgUCgAAADuBQAAAKBQKFCgAAAA7wUAADBI4EDgSDAAAPAFADBISDAAA"
	"AAAAAD/BQAA+IiIiIiI+AAA";


#define font_w(f)            ((f->b->sv)[0])                                        // max glyph width
#define font_h(f)            ((f->b->sv)[1])                                        // max glyph height
#define font_sw(f)           ((f->b->sv)[2])                                        // glyph spacing
#define font_gs(f)           (font_h(f)*(int)ceil(font_w(f)/8.0)+1)
#define font_gb(f,c)         (3+(0xFF&(c))*font_gs(f))
#define font_gw(f,c)         ((f->b->sv)[font_gb(f,c)])                            // actual glyph width
#define font_pp(f,c,x,y)     (f->b->sv)[font_gb(f,c)+1+y*((int)ceil(iw/8.0))+(x/8)]
#define font_bit(x,v)        (v<<(7-(x%8)))
#define font_gpix(f,c,x,y)   ((font_pp(f,c,x,y)&font_bit(x,1))?1:0)                             // get pixel
#define font_spix(f,c,x,y,v) font_pp(f,c,x,y)=((font_pp(f,c,x,y)&~font_bit(x,1))|font_bit(x,v)) // set pixel
#define font_each(f,c)       int iw=font_w(f),ih=font_h(f);for(int a=0;a<ih;a++)for(int b=0;b<iw;b++)

pair font_textsize(lv*f,char*t){
	pair cursor={0,0},size={0,font_h(f)};
	for(int z=0;t[z];z++)if(t[z]!='\n'){cursor.x+=font_gw(f,t[z])+font_sw(f),size.x=MAX(size.x,cursor.x);}else{cursor.x=0,size.y+=font_h(f);}
	return size;
}
lv* normalize_font(lv*fonts,lv*x){
	if(x)EACH(z,fonts){if(fonts->lv[z]==x)return fonts->kv[z];if(matchr(fonts->kv[z],x))return x;}
	return lmistr("body");
}
lv*font_make(pair s); // forward ref
lv* n_font_textsize(lv*self,lv*z){return lmpair(font_textsize(self,ls(l_first(z))->sv));}
lv* interface_font(lv*self,lv*i,lv*x){
	if(lin(i)||(lis(i)&&i->c==1)){ // read/write glyphs
		int ix=lin(i)?ln(i): (0xFF&i->sv[0]);
		if(x){
			if(ix<0||ix>255)return x;if(!image_is(x))x=image_empty();pair s=image_size(x);
			font_each(self,ix){int col=(b>=s.x||a>=s.y)?0: x->b->sv[b+a*s.x]?1:0; font_spix(self,ix,b,a,col);}
			font_gw(self,ix)=MIN(s.x,font_w(self));return x;
		}
		if(ix<0||ix>255)return image_empty();
		pair s={font_gw(self,ix),font_h(self)};lv*r=lmbuff(s);
		font_each(self,ix)if(b<s.x)r->sv[b+a*s.x]=font_gpix(self,ix,b,a);
		return image_make(r);
	}
	if(x){
		ikey("space"){font_sw(self)=ln(x);return x;}
		ikey("size" ){
			lv*r=font_make(pair_max(getpair(x),(pair){1,1}));iwrite(r,lmistr("space"),ifield(self,"space"));
			for(int z=0;z<256;z++)iindex(r,z,iindex(self,z,NULL));self->b=r->b;return x;
		}
	}else{
		ikey("size"    )return lmpair((pair){font_w(self),font_h(self)});
		ikey("space"   )return lmn(font_sw(self));
		ikey("textsize")return lmnat(n_font_textsize,self);
		ikey("glyphs"){lv*r=lml(0);for(int z=0;z<256;z++)if(font_gw(self,z))ll_add(r,lmn(z));return r;}
	}return x?x:LNIL;
}
lv* font_make(pair s){
	s=pair_max(s,(pair){1,1});
	lv*r=lmi(interface_font,lmistr("font"),lms(3+256*(1+s.y*(int)ceil(s.x/8.0))));
	return font_w(r)=s.x,font_h(r)=s.y,font_sw(r)=1,r;
}
lv* font_read(lv*x){
	char f=0;lv*r=data_read("FNT",&f,x);lv*ri=NULL;
	if(r&&f=='0'){
		pair s=pair_max((pair){0xFF&(r->sv[0]),0xFF&(r->sv[1])},(pair){1,1});int gs=ceil(s.x/8.0)*s.y;
		ri=font_make(s);font_sw(ri)=0xFF&(r->sv[2]);
		for(int z=3,ci=32;(z<r->c)&&(ci<128);ci++,z+=gs){
			int gw=0xFF&r->sv[z++];if(z+gs>r->c)break;
			memcpy(ri->b->sv+font_gb(ri,ci)+1,r->sv+z,gs);font_gw(ri,ci)=gw;
		}
	}
	if(r&&f=='1'){
		pair s=pair_max((pair){0xFF&(r->sv[0]),0xFF&(r->sv[1])},(pair){1,1});int gs=ceil(s.x/8.0)*s.y;
		ri=font_make(s);font_sw(ri)=0xFF&(r->sv[2]);
		for(int z=3;z+1<r->c;z+=gs){
			int ci=0xFF&r->sv[z++], gw=0xFF&r->sv[z++];if(z+gs>r->c)break;
			memcpy(ri->b->sv+font_gb(ri,ci)+1,r->sv+z,gs);font_gw(ri,ci)=gw;
		}
	}
	return ri;
}
lv* font_write(lv*x){
	pair s={font_w(x),font_h(x)};int gs=ceil(s.x/8.0)*s.y;
	str r=str_new();str_addraw(&r,s.x),str_addraw(&r,s.y),str_addraw(&r,font_sw(x));
	int dense=1;for(int z=0;z<256;z++)dense&=((font_gw(x,z)!=0)==(z>=32&&z<=127));
	if(dense){
		for(int ci=32;ci<128;ci++){
			str_addraw(&r,font_gw(x,ci));
			for(int z=0;z<gs;z++)str_addraw(&r,(x->b->sv)[font_gb(x,ci)+z+1]);
		};lv*p=lmv(1);p->c=r.c;p->sv=r.sv;return data_write("FNT",'0',p);
	}
	else{
		for(int ci=0;ci<256;ci++)if(font_gw(x,ci)){
			str_addraw(&r,ci),str_addraw(&r,font_gw(x,ci));
			for(int z=0;z<gs;z++)str_addraw(&r,(x->b->sv)[font_gb(x,ci)+z+1]);
		};lv*p=lmv(1);p->c=r.c;p->sv=r.sv;return data_write("FNT",'1',p);
	}
}

// Patterns interface

unsigned int COLORS[]={
	0xFFFFFFFF,0xFFFFFF00,0xFFFF6500,0xFFDC0000,0xFFFF0097,0xFF360097,0xFF0000CA,0xFF0097FF,
	0xFF00A800,0xFF006500,0xFF653600,0xFF976536,0xFFB9B9B9,0xFF868686,0xFF454545,0xFF000000,
};
unsigned int DEFAULT_COLORS[]={
	0xFFFFFFFF,0xFFFFFF00,0xFFFF6500,0xFFDC0000,0xFFFF0097,0xFF360097,0xFF0000CA,0xFF0097FF,
	0xFF00A800,0xFF006500,0xFF653600,0xFF976536,0xFFB9B9B9,0xFF868686,0xFF454545,0xFF000000,
};
char*DEFAULT_ANIMS="[[13,9,5,1,5,9],[4,4,8,14,14,8],[18,18,20,19,19,20],[0,0,0,0,1,1,1,1]]";
char*DEFAULT_PATTERNS=
	"%%IMG0AAgA4AAAAAAAAAAA//////////+AgID/CAgI/yBAgMEiHAgQgAAIAIAACAD/d//d/3f/3XEiF49HInT4iFAgAgW"
	"IiIiIACIAiAAiAHfdd9133XfdQIAACAQCACABAQOESDAMAogiiCKIIogiqlWqVapVqlWABEAIASACEMAMjbEwAxvYqgCq"
	"AKoAqgD/Vf9V/1X/Vf8A/wD/AP8AqqqqqqqqqqpEiBEiRIgRIt27d+7du3fuQIABAgQIECC/f/79+/fv3wgAqgAIAIgAj"
	"493mPj4d4mqAIgUIkGIALCwsL8Av7+w";

typedef struct {char pats[28][8][8];unsigned int colors[16];int anim_counts[4];char anims[4][256];} palette;
#define anim_count(pal,p)    ((palette*)pal)->anim_counts[p]
#define anim_frame(pal,p,f)  ((palette*)pal)->anims[p][f]
#define pal_pat(pal,p,x,y)   ((palette*)pal)->pats[p][y%8][x%8]
#define pal_col_get(pal,c)   (0xFF000000|((palette*)pal)->colors[c])
#define pal_col_set(pal,c,x) ((palette*)pal)->colors[c]=(0xFFFFFF&(x))

char* patterns_pal(lv*patterns){return patterns->b->b->sv;} // patterns -> image -> buffer -> pixels
lv* interface_patterns(lv*self,lv*i,lv*x){
	char*pal=patterns_pal(self);
	lv*r=NULL;int t=i&&ln(i)?ln(i):0;
	if(x){
		if(t>= 2&&t<=27&&image_is(x)){for(int a=0;a<8;a++)for(int b=0;b<8;b++)pal_pat(pal,t,b,a)=lb(iwrite(x,lmpair((pair){b,a}),NULL));}
		if(t>=28&&t<=31){r=ll(x);int c=anim_count(pal,t-28)=MIN(256,r->c);for(int z=0;z<c;z++){int f=CLAMP(0,ln(r->lv[z]),47);anim_frame(pal,t-28,z)=f>=28&&f<=31?0:f;}}
		if(t>=32&&t<=47){int n=ln(x);pal_col_set(pal,t-32,n);r=x;}
	}
	else{
		if(t== 0       ){r=image_make(lmbuff((pair){8,8}));}
		if(t>= 1&&t<=27){r=image_make(lmbuff((pair){8,8}));for(int a=0;a<8;a++)for(int b=0;b<8;b++)r->b->sv[b+(a*8)]=pal_pat(pal,t,b,a);}
		if(t>=28&&t<=31){r=lml(anim_count(pal,t-28));for(int z=0;z<r->c;z++)r->lv[z]=lmn(anim_frame(pal,t-28,z));}
		if(t>=32&&t<=47){r=lmn(0xFFFFFF&pal_col_get(pal,t-32));}
	}return r?r:x?x:LNIL;
}

lv* anims_write(char*pal){lv*r=lml(4);for(int ai=0;ai<4;ai++){GEN(a,anim_count(pal,ai))lmn(anim_frame(pal,ai,z));r->lv[ai]=a;}return r;}
void anims_read(char*pal,lv*f){
	if(!f||!lil(f))f=l_parse(lmistr("%j"),lmistr(DEFAULT_ANIMS));
	for(int ai=0;ai<4&&ai<f->c;ai++){
		lv*a=f->lv[ai];if(lil(a)){anim_count(pal,ai)=a->c;for(int z=0;z<256&&z<a->c;z++)anim_frame(pal,ai,z)=CLAMP(0,ln(a->lv[z]),48);}
	}
}
void pick_palette(lv*deck){char*pal=patterns_pal(ifield(deck,"patterns"));for(int z=0;z<16;z++)COLORS[z]=pal_col_get(pal,z);}
lv* patterns_write(lv*x){
	char*pal=patterns_pal(x);int c=0;for(int z=0;z<16;z++)if(pal_col_get(pal,z)!=DEFAULT_COLORS[z]){c=1;break;}
	lv*b=lmbuff((pair){8,(28*8)+(6*c)});for(int z=0;z<28*8*8;z++)b->sv[z]=pal[z];
	if(c){char*o=b->sv+(28*8*8);for(int z=0;z<16;z++,o+=3){int v=pal_col_get(pal,z);o[0]=0xFF&(v>>16);o[1]=0xFF&(v>>8);o[2]=0xFF&(v);}}
	return image_write(image_make(b));
}
lv* patterns_read(lv*x){
	lv*d=dget(x,lmistr("patterns"));
	lv*i=image_read(d?ls(d):lmistr(DEFAULT_PATTERNS));pair s=image_size(i);i=image_resize(i,(pair){8,(28*8)+6});
	lv*b=lmbuff((pair){8,((int)sizeof(palette))/8});char*pal=b->sv;for(int z=0;z<28*8*8;z++)pal[z]=i->b->sv[z];
	if(s.y<=(28*8)){for(int z=0;z<16;z++)pal_col_set(pal,z,DEFAULT_COLORS[z]);}
	else{char*o=i->b->sv+(28*8*8);for(int z=0;z<16;z++,o+=3)pal_col_set(pal,z,((0xFF&o[0])<<16)|((0xFF&o[1])<<8)|(0xFF&o[2]));}
	lv*r=lmi(interface_patterns,lmistr("patterns"),image_make(b));anims_read(patterns_pal(r),dget(x,lmistr("animations")));return r;
}

#define anim_ants(x,y)                (((x+y+(frame_count/2))/3)%2?15:0)
#define get_pattern(pal,pix,x,y)      (pix<2?(pix?1:0): pix>31?(pix==32?0:1): pix>27?0: pal_pat(pal,pix,x,y)&1)
#define get_anim(pal,pix,frame)       (pix<28||pix>31?pix: anim_frame(pal,pix-28,(frame/4)%MAX(1,anim_count(pal,pix-28))))
#define get_color(pal,pix,frame,x,y)  (pix==ANTS?anim_ants(x,y):            pix>47?0: pix>31?pix-32: draw_pattern(pal,pix,x,y)?15:0)
#define get_colort(pal,pix,frame,x,y) (pix==ANTS?anim_ants(x,y): pix==0?16: pix>47?0: pix>31?pix-32: draw_pattern(pal,pix,x,y)?15:0)
int draw_pattern(char*pal,int pix,int x,int y){return get_pattern(pal,pix,x,y);}
int anim_pattern(char*pal,int pix,int frame){return get_anim(pal,pix,frame);}
int draw_color      (char*pal,int pix,int frame,int x,int y){pix=anim_pattern(pal,pix,frame);return get_color (pal,pix,frame,x,y);}
int draw_color_trans(char*pal,int pix,int frame,int x,int y){pix=anim_pattern(pal,pix,frame);return get_colort(pal,pix,frame,x,y);}
void draw_frame(char*pal,lv*buffer,int*p,int pitch,int frame,int mask){
	pair size=buff_size(buffer);for(int y=0;y<size.y;y++)for(int x=0;x<size.x;x++){
		int stride=pitch/sizeof(int), pix=0xFF&buffer->sv[x+y*size.x], ci=13;
		if(pix||!mask){pix=anim_pattern(pal,pix,frame),ci=get_color(pal,pix,frame,x,y);}
		p[x+y*stride]=COLORS[ci];
	}
}

// Rendering

#define PIX(a,b)    frame.buffer->sv[(a)+((b)*frame.size.x)]
#define inclip(x,y) box_in(frame.clip,(pair){x,y})

char BRUSHES[]={
	0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00, 0x00,0x00,0x10,0x38,0x10,0x00,0x00,0x00,
	0x00,0x00,0x18,0x3C,0x3C,0x18,0x00,0x00, 0x00,0x38,0x7C,0x7C,0x7C,0x38,0x00,0x00,
	0x38,0x7C,0xFE,0xFE,0xFE,0x7C,0x38,0x00, 0x10,0x00,0x41,0x08,0x80,0x11,0x00,0x22,
	0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00, 0x00,0x00,0x38,0x38,0x38,0x00,0x00,0x00,
	0x00,0x00,0x3C,0x3C,0x3C,0x3C,0x00,0x00, 0x00,0x7C,0x7C,0x7C,0x7C,0x7C,0x00,0x00,
	0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0x00, 0x20,0x0A,0x80,0x24,0x01,0x48,0x02,0x51,
	0x00,0x00,0x10,0x10,0x10,0x00,0x00,0x00, 0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,
	0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00, 0x00,0x00,0x08,0x10,0x20,0x00,0x00,0x00,
	0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x00, 0x02,0x00,0x08,0x00,0x20,0x00,0x80,0x00,
	0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0xAA,0x00,0x00,0x00,0x00, 0x00,0x00,0x20,0x10,0x08,0x00,0x00,0x00,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x00, 0x80,0x00,0x20,0x00,0x08,0x00,0x02,0x00,
};
void draw_line_simple(rect r,int brush,int pattern){
	int dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1;while(1){
		for(int b=0;b<8;b++)for(int a=0;a<8;a++)if(((BRUSHES[(brush*8)+b]>>(7-a))&1)&&inclip(r.x+a-3,r.y+b-3))PIX(r.x+a-3,r.y+b-3)=pattern;
		if(r.x==r.w&&r.y==r.h)break;int e2=err*2;if(e2>=dy)err+=dy,r.x+=sx;if(e2<=dx)err+=dx,r.y+=sy;
	}
}
void draw_line_custom(rect r,lv*mask,int pattern){
	int dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1;pair ms=buff_size(mask),mc={ms.x/2,ms.y/2};while(1){
		for(int b=0;b<ms.y;b++)for(int a=0;a<ms.x;a++){
			int mp=mask->sv[a+b*ms.x];
			if(mp&&inclip(r.x+a-mc.x,r.y+b-mc.y))PIX(r.x+a-mc.x,r.y+b-mc.y)=mp==1?pattern: mp==47?1: mp;
		}
		if(r.x==r.w&&r.y==r.h)break;int e2=err*2;if(e2>=dy)err+=dy,r.x+=sx;if(e2<=dx)err+=dx,r.y+=sy;
	}
}
void draw_line_function(rect r,lv*func,int pattern){
	lv*a=lml2(lmpair((pair){r.w-r.x,r.h-r.y}),ONE),*p=lmblk(),*e=lmenv(NULL);blk_lit(p,func),blk_lit(p,a),blk_op(p,CALL),pushstate(e);
	int dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1;while(!do_panic){
		state.e->c=1,state.t->c=0,state.p->c=0,state.pcs.c=0;issue(e,p);int quota=BRUSH_QUOTA;while(quota&&running())runop(),quota--;lv*v=running()?ZERO:arg();
		if(image_is(v)){
			lv*mask=v->b;pair ms=buff_size(mask),mc={ms.x/2,ms.y/2};
			for(int b=0;b<ms.y;b++)for(int a=0;a<ms.x;a++){
				int mp=mask->sv[a+b*ms.x];
				if(mp&&inclip(r.x+a-mc.x,r.y+b-mc.y))PIX(r.x+a-mc.x,r.y+b-mc.y)=mp==1?pattern: mp==47?1: mp;
			}
		}if(r.x==r.w&&r.y==r.h)break;int e2=err*2;if(e2>=dy)err+=dy,r.x+=sx;if(e2<=dx)err+=dx,r.y+=sy;a->lv[1]=ZERO;
	}popstate();
}
void draw_line(rect r,int brush,int pattern,lv*deck){
	if(brush>=0&&brush<=23){draw_line_simple(r,brush,pattern);return;}
	lv*b=dget(deck->b,lmistr("brushes"));if(brush<0||brush-24>=b->c)return;lv*f=b->lv[brush-24];
	if(image_is(f)){draw_line_custom(r,f->b,pattern);}else if(lion(f)){draw_line_function(r,f,pattern);}
}
lv* n_brush(lv*self,lv*z){
	lv*b=dget(self->b,lmistr("brushes")),*bt=dget(self->b,lmistr("brusht")),*f=l_first(z);
	if(lion(f)){
		lv*k=lmcstr(f->sv),*v=lmbuff((pair){64,32});cstate t=frame;frame=(cstate){0,0,(pair){64,32},(rect){0,0,64,32},v,NULL},dset(b,k,f);
		draw_line((rect){16,16,32,16},24+dgeti(b,k),1,self);
		draw_line((rect){32,16,40,16},24+dgeti(b,k),1,self);
		draw_line((rect){40,16,44,16},24+dgeti(b,k),1,self);
		draw_line((rect){44,16,48,16},24+dgeti(b,k),1,self);
		frame=t,dset(bt,k,image_make(v));
	}
	if(lis(f)&&z->c>1&&image_is(z->lv[1])){dset(b,f,z->lv[1]),dset(bt,f,z->lv[1]);}
	return b;
}

void draw_pix(int x,int y,int pattern){if(inclip(x,y))PIX(x,y)=pattern;}
void draw_hline(int x0,int x1,int y,int pattern){
	if(y<frame.clip.y||y>=frame.clip.y+frame.clip.h)return;
	x0=MAX(frame.clip.x,x0),x1=MIN(frame.clip.x+frame.clip.w,x1);for(int z=x0;z<x1;z++)PIX(z,y)=pattern;
}
void draw_vline(int x,int y0,int y1,int pattern){
	if(x<frame.clip.x||x>=frame.clip.x+frame.clip.w)return;
	y0=MAX(frame.clip.y,y0),y1=MIN(frame.clip.y+frame.clip.h,y1);for(int z=y0;z<y1;z++)PIX(x,z)=pattern;
}
void draw_rect(rect r,int pattern   ){r=box_intersect(r,frame.clip);for(int a=r.y;a<r.y+r.h;a++)for(int b=r.x;b<r.x+r.w;b++)PIX(b,a)=pattern;}
void draw_invert_raw(char*pal,rect r){r=box_intersect(r,frame.clip);for(int a=r.y;a<r.y+r.h;a++)for(int b=r.x;b<r.x+r.w;b++)PIX(b,a)=1^draw_pattern(pal,0xFF&PIX(b,a),b,a);}
void draw_icon(pair pos,lv*i,int pattern){
	pair s=image_size(i);
	for(int a=0;a<s.y;a++)for(int b=0;b<s.x;b++)if(i->b->sv[b+(a*s.x)]&&inclip(pos.x+b,pos.y+a))PIX(pos.x+b,pos.y+a)=pattern;
}
void draw_iconc(rect r,lv*i,int pattern){rect s=box_center(r,image_size(i));draw_icon((pair){s.x,s.y},i,pattern);}
void draw_box(rect r,int brush,int pattern){
	if(r.w==0||r.h==0||!box_overlap(r,(rect){0,0,frame.size.x,frame.size.y}))return;
	if(r.y               >=0)draw_line_simple((rect){r.x      ,r.y      ,r.x+r.w-1,r.y      },brush,pattern);
	if(r.y+r.h<=frame.size.y)draw_line_simple((rect){r.x      ,r.y+r.h-1,r.x+r.w-1,r.y+r.h-1},brush,pattern);
	if(r.x               >=0)draw_line_simple((rect){r.x      ,r.y      ,r.x      ,r.y+r.h-1},brush,pattern);
	if(r.x+r.w<=frame.size.x)draw_line_simple((rect){r.x+r.w-1,r.y      ,r.x+r.w-1,r.y+r.h-1},brush,pattern);
}
void draw_box_fancy(rect r,int brush,int pattern,lv*deck){
	if(r.w==0||r.h==0||!box_overlap(r,(rect){0,0,frame.size.x,frame.size.y}))return;
	if(r.y               >=0)draw_line((rect){r.x      ,r.y      ,r.x+r.w-1,r.y      },brush,pattern,deck);
	if(r.y+r.h<=frame.size.y)draw_line((rect){r.x      ,r.y+r.h-1,r.x+r.w-1,r.y+r.h-1},brush,pattern,deck);
	if(r.x               >=0)draw_line((rect){r.x      ,r.y      ,r.x      ,r.y+r.h-1},brush,pattern,deck);
	if(r.x+r.w<=frame.size.x)draw_line((rect){r.x+r.w-1,r.y      ,r.x+r.w-1,r.y+r.h-1},brush,pattern,deck);
}
void draw_shadow(rect r,int fcol,int bcol,int solid){
	if(solid)draw_rect(box_intersect(r,frame.clip),bcol);draw_box(r,0,fcol),draw_hline(r.x+3,r.x+r.w,r.y+r.h,fcol),draw_vline(r.x+r.w,r.y+2,r.y+r.h+1,fcol);
}
#define grower(n,t) {if(!n)n=calloc(n##_size=16,sizeof(t));if(n##_count==n##_size)n=realloc(n,sizeof(t)*(n##_size+=16));}
pair*fringe ;int fringe_count=0,fringe_size=0;
char*visited;int visited_size=0;
void fringe_push(pair x){grower(fringe,pair);fringe[fringe_count++]=x;}
pair fringe_pop(void){return fringe[--fringe_count];}
void draw_fill(pair r,int pattern,char*src){
	if(!inclip(r.x,r.y))return;
	int sz=frame.size.x*frame.size.y;
	if(!visited)visited=malloc(visited_size=sz);
	if(visited&&visited_size<sz)visited=realloc(visited,visited_size=sz);
	memset(visited,0,sz);
	pair offsets[]={{-1,0},{0,-1},{1,0},{0,1}};
	src=src!=NULL?src:frame.buffer->sv;
	#define SPIX(a,b) src[(a)+((b)*frame.size.x)]
	int source=SPIX(r.x,r.y); fringe_count=0; fringe_push(r);
	while(fringe_count){
		pair here=fringe_pop();
		if(PIX(here.x,here.y)==pattern)continue;
		PIX(here.x,here.y)=pattern;
		for(int z=0;z<4;z++){
			pair there={here.x+offsets[z].x,here.y+offsets[z].y};int ti=there.x+there.y*frame.size.x;
			if(inclip(there.x,there.y)&&!visited[ti]&&SPIX(there.x,there.y)==source)fringe_push(there),visited[ti]=1;
		}
	}
}
void draw_scaled(rect r,lv*buff,int opaque){
	if(r.w==0||r.h==0)return;pair s=buff_size(buff);lerp_scale()
	if(r.w==s.x&&r.h==s.y){buffer_paste(r,frame.clip,buff,frame.buffer,opaque);return;}
	for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
		int sx=b/sc.x, sy=a/sc.y, c=buff->sv[sx+sy*s.x];
		if((opaque||c!=0)&&inclip(r.x+b,r.y+a))PIX(r.x+b,r.y+a)=c;
	}
}
void draw_invert_scaled(char*pal,rect r,lv*buff){
	if(r.w==0||r.h==0)return;pair s=buff_size(buff);lerp_scale();
	for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
		int sx=b/sc.x, sy=a/sc.y, dx=r.x+b, dy=r.y+a, v=buff->sv[sx+sy*s.x];
		int c=draw_pattern(pal,v,dx,dy);
		if(inclip(dx,dy))PIX(dx,dy)=c^draw_pattern(pal,PIX(dx,dy),dx,dy);
	}
}
void draw_fat(rect r,lv*buff,char*pal,int frame_count,int mask,float scale,pair offset){
	pair s=buff_size(buff);for(int y=0;y<ceil(r.h/scale);y++)for(int x=0;x<ceil(r.w/scale);x++){
		if(offset.x+x>=s.x||offset.y+y>=s.y||offset.x+x<0||offset.y+y<0)continue;
		int v=buff->sv[(offset.x+x)+(offset.y+y)*s.x];if(v==mask)continue;
		int c=anim_pattern(pal,v,frame_count),p=draw_pattern(pal,c,offset.x+x,offset.y+y);
		draw_rect((rect){r.x+x*scale,r.y+y*scale,scale,scale},c>=32?c: c==0?0: p?1:32);
	}
}
void draw_fat_scaled(rect r,lv*buff,int opaque,char*pal,int frame_count,int scale,pair offset){
	if(r.w==0||r.h==0)return;pair s=buff_size(buff);lerp_scale();
	for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
		int sx=b/sc.x, sy=a/sc.y, dx=r.x+b, dy=r.y+a, v=buff->sv[sx+sy*s.x];
		int c=anim_pattern(pal,v,frame_count),p=draw_pattern(pal,c,dx,dy);
		if(opaque||v!=0)draw_rect(rect_add((rect){dx*scale,dy*scale,scale,scale},offset),c>=32?c: p?1:0);
	}
}
float*dither_err=NULL;int dither_err_size=0;float dither_threshold=0.5;
void draw_dithered(rect r,lv*buff,int opaque,lv*mask){
	if(r.w==0||r.h==0)return;pair s=buff_size(buff);int stride=2*r.w, m[]={0,1,r.w-2,r.w-1,r.w,stride-1};
	if(!dither_err)dither_err=calloc(stride,sizeof(float)),dither_err_size=stride;
	if(dither_err_size<stride)dither_err=realloc(dither_err,sizeof(float)*stride),dither_err_size=stride;
	for(int z=0;z<dither_err_size;z++)dither_err[z]=0.0;
	for(int ei=0,a=0;a<r.h;a++)for(int b=0;b<r.w;b++){
		int sx=((b*1.0)/r.w)*s.x, sy=((a*1.0)/r.h)*s.y, src=0xFF&buff->sv[sx+sy*s.x], ms=mask?mask->sv[sx+sy*s.x]:1;
		float pix=(src/256.0)+dither_err[ei], col=pix>dither_threshold?1:0, err=(pix-col)/8.0;
		dither_err[ei]=0, ei=(ei+1)%stride; for(int z=0;z<6;z++)dither_err[(ei+m[z])%stride]+=err;
		int c=!col;if(ms&&(opaque||c!=0)&&inclip(r.x+b,r.y+a))PIX(r.x+b,r.y+a)=c;
	}
}
fpair*poly;int poly_count=0,poly_size=0;
void poly_push(fpair x){grower(poly,fpair);poly[poly_count++]=x;}
rect poly_bounds(void){
	rect d={frame.clip.x+frame.clip.w,frame.clip.y+frame.clip.h,frame.clip.x,frame.clip.y};
	for(int z=0;z<poly_count;z++){fpair p=poly[z];d.x=MIN(d.x,p.x),d.y=MIN(d.y,p.y),d.w=MAX(d.w,p.x),d.h=MAX(d.h,p.y);}
	d.w-=d.x,d.h-=d.y,d.w++,d.h++;return d;
}
int poly_in(fpair pos){
	int r=0;for(int i=0,j=poly_count-1;i<poly_count;i++){
		if(pos.x==poly[i].x&&pos.y==poly[i].y)return 1;
		if(((poly[i].y>=pos.y)!=(poly[j].y>=pos.y))&&(pos.x<=(poly[j].x-poly[i].x)*(pos.y-poly[i].y)/(poly[j].y-poly[i].y)+poly[i].x))r^=1;
		j=i;
	}return r;
}
void draw_poly(int pattern){
	rect r=box_intersect(frame.clip,poly_bounds());for(int a=0;a<r.h;a++)for(int b=0;b<r.w;b++)if(poly_in((fpair){b+r.x,a+r.y}))PIX(b+r.x,a+r.y)=pattern;
}
void draw_text(rect pos,char*text,lv*f,int pattern){
	rect cursor=pos;for(int z=0;text[z];z++){
		char c=text[z];
		if(c!='\n'){
			font_each(f,c)if(font_gpix(f,c,b,a)&&inclip(cursor.x+b,cursor.y+a))PIX(cursor.x+b,cursor.y+a)=pattern;
			cursor.x+=font_gw(f,c)+font_sw(f);
		}else{cursor.x=pos.x, cursor.y+=font_h(f);}
	}
}
void draw_text_outlined(rect pos,char*text,lv*f){
	draw_text((rect){pos.x-1,pos.y-1,0,0},text,f,32);
	draw_text((rect){pos.x-1,pos.y  ,0,0},text,f,32);
	draw_text((rect){pos.x-1,pos.y+1,0,0},text,f,32);
	draw_text((rect){pos.x  ,pos.y-1,0,0},text,f,32);
	draw_text((rect){pos.x  ,pos.y+1,0,0},text,f,32);
	draw_text((rect){pos.x+1,pos.y-1,0,0},text,f,32);
	draw_text((rect){pos.x+1,pos.y  ,0,0},text,f,32);
	draw_text((rect){pos.x+1,pos.y+1,0,0},text,f,32);
	draw_text(pos,text,f,1);
}
void draw_wrapped(rect o,rect sr,lv*dst,lv*src,rect clip,int opaque,char*pal){
	pair ss=buff_size(src),ds=buff_size(dst);sr=box_intersect(sr,rect_pair((pair){0,0},ss));
	rect r=box_intersect(o,clip);pair d={r.x-o.x,r.y-o.y};if(r.w<=0||r.h<=0||sr.w<=0||sr.h<=0)return;
	if(!pal){for(int y=0;y<r.h;y++)for(int x=0;x<r.w;x++){int c=src->sv[(sr.x+((x+d.x)%sr.w))+(sr.y+((y+d.y)%sr.h))*ss.x];if(opaque||c)dst->sv[(r.x+x)+(r.y+y)*ds.x]=c;}return;}
	for(int y=0;y<r.h;y++)for(int x=0;x<r.w;x++){
		int dx=r.x+x,dy=r.y+y, c=draw_pattern(pal,src->sv[(sr.x+((x+d.x)%sr.w))+(sr.y+((y+d.y)%sr.h))*ss.x],dx,dy), di=(r.x+x)+(r.y+y)*ds.x;
		dst->sv[di]=c^draw_pattern(pal,dst->sv[di],dx,dy);
	}
}
void draw_9seg(rect r,lv*dst,lv*src,rect m,rect clip,int opaque,char*inv){
	pair o={r.x,r.y}, s=buff_size(src);if(s.x<1||s.y<1)return;
	draw_wrapped(rect_add((rect){0      ,0      ,m.x          ,m.y          },o),(rect){0      ,0      ,m.x          ,m.y          },dst,src,clip,opaque,inv); // NW
	draw_wrapped(rect_add((rect){0      ,r.h-m.h,m.x          ,m.h          },o),(rect){0      ,s.y-m.h,m.x          ,m.h          },dst,src,clip,opaque,inv); // SW
	draw_wrapped(rect_add((rect){r.w-m.w,0      ,m.w          ,m.y          },o),(rect){s.x-m.w,0      ,m.w          ,m.y          },dst,src,clip,opaque,inv); // NE
	draw_wrapped(rect_add((rect){r.w-m.w,r.h-m.h,m.w          ,m.h          },o),(rect){s.x-m.w,s.y-m.h,m.w          ,m.h          },dst,src,clip,opaque,inv); // SE
	draw_wrapped(rect_add((rect){0      ,m.y    ,m.x          ,r.h-(m.y+m.h)},o),(rect){0      ,m.y    ,m.x          ,s.y-(m.y+m.h)},dst,src,clip,opaque,inv); // W
	draw_wrapped(rect_add((rect){m.x    ,0      ,r.w-(m.x+m.w),m.y          },o),(rect){m.x    ,0      ,s.x-(m.x+m.w),m.y          },dst,src,clip,opaque,inv); // N
	draw_wrapped(rect_add((rect){r.w-m.w,m.y    ,m.w          ,r.h-(m.y+m.h)},o),(rect){s.x-m.w,m.y    ,m.w          ,s.y-(m.y+m.h)},dst,src,clip,opaque,inv); // E
	draw_wrapped(rect_add((rect){m.x    ,r.h-m.h,r.w-(m.x+m.w),m.h          },o),(rect){m.x    ,s.y-m.h,s.x-(m.x+m.w),m.h          },dst,src,clip,opaque,inv); // S
	draw_wrapped(rect_add((rect){m.x    ,m.y    ,r.w-(m.x+m.w),r.h-(m.y+m.h)},o),(rect){m.x    ,m.y    ,s.x-(m.x+m.w),s.y-(m.y+m.h)},dst,src,clip,opaque,inv); // C
}

typedef struct {pair pos;char c;} glyph;
glyph*glyphs;int glyph_count=0,glyph_size=0;
void glyph_push(pair pos,char c){
	if(!glyphs)glyphs=calloc(glyph_size=16,sizeof(glyph));
	if(glyph_count==glyph_size)glyphs=realloc(glyphs,sizeof(glyph)*(glyph_size+=16));
	glyphs[glyph_count++]=(glyph){pos,c};
}
void draw_text_fit(rect r,char*text,lv*f,int pattern){
	#define ELLIPSIS (95+32)
	int x=0,y=0,fh=font_h(f),ew=font_gw(f,ELLIPSIS); glyph_count=0;
	for(int z=0;text[z]&&(y+fh)<=r.h;z++){
		char c=text[z];
		if(c=='\n'){x=0;y+=fh;}
		else if(x+font_gw(f,c)>=(r.w-ew)){glyph_push((pair){x,y},ELLIPSIS);while(text[z]&&text[z]!='\n')z++;x=0;if(text[z]){y+=fh;}else{z--;}}
		else{glyph_push((pair){x,y},c),x+=font_gw(f,c)+font_sw(f);}
	}
	int yo=ceil((r.h-(y+fh))/2.0);for(int z=0;z<glyph_count;z++){
		glyph g=glyphs[z]; g.pos.x+=r.x,g.pos.y+=yo+r.y;
		font_each(f,g.c)if(font_gpix(f,g.c,b,a)&&inclip(g.pos.x+b,g.pos.y+a))PIX(g.pos.x+b,g.pos.y+a)=pattern;
	}
}
void draw_textr(rect r,char*text,lv*font,int pattern){
	pair size=font_textsize(font,text);
	if(size.x<r.w){draw_text((rect){r.x+r.w-size.x,r.y+ceil((r.h-size.y)/2.0),size.x,r.h},text,font,pattern);}else{draw_text_fit(r,text,font,pattern);}
}
void draw_textc(rect r,char*text,lv*font,int pattern){
	pair size=font_textsize(font,text);
	if(pattern==-1){draw_text_outlined(box_center(r,size),text,font);}
	else if(size.x<r.w){draw_text(box_center(r,size),text,font,pattern);}else{draw_text_fit(r,text,font,pattern);}
}
rect draw_modalbox(pair s){
	int menu=16; rect r=box_center((rect){0,menu,frame.size.x,frame.size.y-menu},s), o=inset(r,-5);
	draw_rect(inset(o,-5),32),draw_box(inset(o,-5),0,1),draw_box(inset(o,-2),0,1),draw_box(inset(o,-1),0,1);
	return r;
}

enum field_align{align_left,align_center,align_right};
typedef struct {rect pos;int line;char c;lv*font,*arg;} glyph_box;
typedef struct {rect pos;pair range;} line_box;
glyph_box*layout;int layout_count=0,layout_size=0;
line_box*lines;int lines_count=0,lines_size=0;
void layout_push(rect pos,int line,char c,lv*font,lv*arg){grower(layout,glyph_box);layout[layout_count++]=(glyph_box){pos,line,c,font,arg};}
void lines_push (rect pos,pair range                    ){grower(lines ,line_box );lines [lines_count++]=(line_box){pos,range};}
pair layout_plaintext(char*text,lv*font,int align,pair max){
	#define lnl() cursor=(pair){0,cursor.y+1}
	layout_count=lines_count=0; pair cursor={0,0}; int fh=font_h(font),fs=font_sw(font);
	for(int z=0;text[z]!='\0';z++){
		int a=z,w=text[z]=='\n'?0:(font_gw(font,text[z])+fs);
		if(!strchr("\n ",text[z]))while(text[z+1]&&!strchr("\n ",text[z+1]))w+=font_gw(font,text[++z])+fs; // find word
		if(cursor.x+w>=max.x&&cursor.x>0)lnl(); // word won't fit this line
		for(int i=a;i<=z;i++){                  // append word to line
			char c=text[i]; pair size={c=='\n'?0:font_gw(font,c)+fs,fh};
			if(c==' '&&cursor.x==0&&layout_count>0&&!strchr("\n ",layout[layout_count-1].c))size.x=0; // squish lead space after a soft-wrap
			if(cursor.x+size.x>=max.x)lnl(); // hard-break overlong words
			layout_push((rect){cursor.x,cursor.y,size.x,size.y},cursor.y,c,font,LNIL);
			if(c=='\n'){lnl();}else{cursor.x+=size.x;}
			if(cursor.y>=(max.y/fh)){
				layout_count=MAX(1,layout_count-3);
				layout[layout_count-1].c=ELLIPSIS;layout[layout_count-1].pos.w=font_gw(font,ELLIPSIS)+fs;
				z=strlen(text)-1;break;
			}
		}
	}
	int y=0;for(int i=0,line=0;i<layout_count;i++,line++){
		int a=i;while(i<(layout_count-1)&&(layout[i+1].pos.y==line))i++;        // find bounds of line
		int h=0;for(int z=a;z<=i;z++)h=MAX(h,layout[z].pos.h);                  // find height of line
		int w=(a&&a==i)?0:(layout[i].pos.x+layout[i].pos.w);                    // find width of line
		int x=align==align_center?(max.x-w)/2: align==align_right?(max.x-w): 0; // justify
		lines_push((rect){x,y,w,h},(pair){a,i});
		for(int z=a;z<=i;z++){rect*g=&layout[z].pos;g->y=y+((h-g->h)/2);g->x+=x;}if(i<layout_count-1)y+=h;
	}return (pair){max.x,y+fh};
}
pair layout_richtext(lv*deck,lv*table,lv*font,int align,int width){
	layout_count=lines_count=0; pair cursor={0,0};
	lv*texts=dget(table,lmistr("text")),*fonts=dget(table,lmistr("font")),*args=dget(table,lmistr("arg")),*dfonts=ifield(deck,"fonts");
	int fh=0;for(int chunk=0;chunk<table->n;chunk++){
		lv*f=dget(dfonts,fonts->lv[chunk]);if(!f)f=font; fh=font_h(f); int fs=font_sw(f);
		if(image_is(args->lv[chunk])){
			pair size=image_size(args->lv[chunk]);if(cursor.x+size.x>=width&&cursor.x>0)lnl(); // image won't fit this line
			layout_push((rect){cursor.x,cursor.y,size.x,size.y},cursor.y,'i',f,args->lv[chunk]);cursor.x+=size.x;continue;
		}
		lv*t=texts->lv[chunk];for(int z=0;z<t->c;z++){
			int a=z,w=t->sv[z]=='\n'?0:(font_gw(f,t->sv[z])+fs);
			if(!strchr("\n ",t->sv[z]))while(z+1<t->c&&!strchr("\n ",t->sv[z+1]))w+=font_gw(f,t->sv[++z])+fs; // find word
			if(cursor.x+w>=width&&cursor.x>0)lnl(); // word won't fit this line
			for(int i=a;i<=z;i++){                  // append word to line
				char c=t->sv[i]; pair size={c=='\n'?0:font_gw(f,c)+fs,fh};
				if(c==' '&&cursor.x==0&&layout_count>0&&!strchr("\n ",layout[layout_count-1].c))size.x=0; // squish lead space after a soft-wrap
				if(cursor.x+size.x>=width)lnl(); // hard-break overlong words
				layout_push((rect){cursor.x,cursor.y,size.x,size.y},cursor.y,c,f,args->lv[chunk]);
				if(c=='\n'){lnl();}else{cursor.x+=size.x;}
			}
		}
	}
	int y=0;for(int i=0,line=0;i<layout_count;i++,line++){
		int a=i;while(i<(layout_count-1)&&(layout[i+1].pos.y==line))i++;        // find bounds of line
		int h=0;for(int z=a;z<=i;z++)h=MAX(h,layout[z].pos.h);                  // find height of line
		int w=(a&&a==i)?0:(layout[i].pos.x+layout[i].pos.w);                    // find width of line
		int x=align==align_center?(width-w)/2: align==align_right?(width-w): 0; // justify
		lines_push((rect){x,y,w,h},(pair){a,i});
		for(int z=a;z<=i;z++){rect*g=&layout[z].pos;g->y=y+((h-g->h)/2);g->x+=x;}y+=h;
	}return (pair){width,y};
}
void draw_text_wrap(rect r,int pattern){
	rect oc=frame.clip;frame.clip=r;for(int z=0;z<layout_count;z++){
		glyph_box g=layout[z];if(g.pos.w<1)continue; // skip squashed spaces/newlines
		g.pos.x+=r.x, g.pos.y+=r.y;
		font_each(g.font,g.c)if(font_gpix(g.font,g.c,b,a)&&inclip(g.pos.x+b,g.pos.y+a))PIX(g.pos.x+b,g.pos.y+a)=pattern;
	}frame.clip=oc;
}
void draw_text_rich(rect r,int pattern,int opaque){
	rect oc=frame.clip;frame.clip=r;for(int z=0;z<layout_count;z++){
		glyph_box g=layout[z];if(g.pos.w<1)continue; // skip squashed spaces/newlines
		if(g.pos.y+g.pos.h<0||g.pos.y>r.h)continue; g.pos.x+=r.x, g.pos.y+=r.y; // coarse clip
		if(lis(g.arg)&&g.arg->c){draw_hline(g.pos.x,g.pos.x+g.pos.w,g.pos.y+g.pos.h-1,19);}
		if(image_is(g.arg)){buffer_paste(g.pos,frame.clip,g.arg->b,frame.buffer,opaque);}
		else{font_each(g.font,g.c)if(font_gpix(g.font,g.c,b,a)&&inclip(g.pos.x+b,g.pos.y+a))PIX(g.pos.x+b,g.pos.y+a)=pattern;}
	}frame.clip=oc;
}

// Sound interface

#define SFX_RATE 8000
void sound_resize(lv*self,int n){
	lv*data=self->b;int o=data->c;n=CLAMP(0,n,10*SFX_RATE); // cap (programmatic) size at 10 seconds.
	data->sv=realloc(data->sv,n),data->c=n;if(o<n)memset(data->sv+o,0,n-o);
}
lv* sound_write(lv*x){return data_write("SND",'0',x->b);}
lv* interface_sound(lv*self,lv*i,lv*x){
	lv*data=self->b;
	if(i&&lin(i)){ // read/write single samples
		int n=ln(i),ib=n>=0&&n<data->c;
		if(x){if(ib)data->sv[n]=0xFF&(int)ln(x);return x;}
		else{return ib?lmn((signed char)data->sv[n]):LNIL;}
	}
	if(i&&lil(i)){ // read/write ranges
		pair n=getpair(i);n.y=MAX(0,n.y);if(x){
			lv*s=ll(x),*r=lms(CLAMP(0,(data->c-n.y)+s->c,10*SFX_RATE));
			for(int z=0;z<n.x;z++)r->sv[z]=z>=data->c?0:data->sv[z]; // before splice
			EACH(z,s)if(n.x+z>=0&&n.x+z<r->c)r->sv[n.x+z]=0xFF&(int)ln(s->lv[z]); // splice
			for(int z=0;z<(data->c)-(n.x+n.y);z++)if(n.x+s->c+z>=0&&n.x+s->c+z<r->c)r->sv[n.x+s->c+z]=n.x+n.y+z>=data->c?0:data->sv[n.x+n.y+z]; // after splice
			self->b=r;return x;
		}else{GEN(r,n.y)((z+n.x<0||z+n.x>=data->c)?LNIL:lmn((signed char)data->sv[z+n.x]));return r;}
	}
	ikey("size"){if(x){sound_resize(self,ln(x));return x;}return lmn(data->c);}
	ikey("duration")return lmn(data->c/(1.0*SFX_RATE));
	ikey("encoded")return sound_write(self);
	ikey("hist")return buffer_hist(self->b,1);
	ikey("map")return lmnat(n_buffer_map,self);
	return x?x:LNIL;
}
lv* sound_make(lv*buffer){buffer->c=MIN(buffer->c,10*SFX_RATE);return lmi(interface_sound,lmistr("sound"),buffer);}
lv* sound_read(lv*x){char f=0;lv*r=data_read("SND",&f,x);return r&&f=='0'?sound_make(r):NULL;}
lv* n_sound(lv*self,lv*z){
	(void)self;z=l_first(z);if(lis(z))return sound_read(z);if(!lil(z)){int n=ln(z);return sound_make(lms(CLAMP(0,n,10*SFX_RATE)));}
	lv*r=sound_make(lms(CLAMP(0,z->c,10*SFX_RATE)));EACH(i,r->b)r->b->sv[i]=0xFF&(int)ln(z->lv[i]);return r;
}

// Array interface

char*casts[]   ={"u8","i8","u16b","u16l","i16b","i16l","u32b","u32l","i32b","i32l","char",NULL};
int cast_size[]={   1,   1,     2,     2,     2,     2,     4,     4,     4,     4,     1,   1};
typedef struct {int here,base,size,cast;lv*data;} array;
#define numf(n)   ln(dget(x->b,lmistr(n)))
#define numv(n,v) dset(d,lmistr(n),lmn(v))
array array_at(array a,int here){return (array){0,a.base+here,a.size-here,a.cast,a.data};}
array unpack_array(lv*x){return (array){numf("here"),numf("base"),numf("size"),numf("cast"),dget(x->b,lmistr("data"))};}
lv* interface_array(lv*self,lv*i,lv*x); // forward ref
lv* array_make(int size,int cast,int base,lv*buffer){
	lv*d=lmd();numv("here",0),numv("base",base),numv("size",size*cast_size[cast]),numv("cast",cast);
	dset(d,lmistr("data"),buffer?buffer:lms(size*cast_size[cast]));return lmi(interface_array,lmistr("array"),d);
}
void array_resize(lv*self,int bytes){
	if(dget(self->b,lmistr("slice")))return;
	bytes=MAX(0,bytes);array a=unpack_array(self);int old=a.data->c;a.data->sv=realloc(a.data->sv,bytes),a.data->c=bytes;
	if(bytes>old)memset(a.data->sv+old,0,bytes-old);dset(self->b,lmistr("size"),lmn(bytes));
}
#define array_offset(src) int offset=ln(lil(src)?l_first(src):src),len=lil(src)?MAX(0,ln(l_last(src))):-1;
#define array_data(r)     int step=cast_size[a.cast];if(index<0||index>=(a.size/step))return r;\
	                      int ix=a.base+step*index;unsigned char*d=(unsigned char*)a.data->sv+ix;if(ix<0||ix+step>a.data->c)return r;
double array_get_raw(array a,int index){
	array_data(0)
	#define ur(i,s) (((unsigned long long)d[i])<<s)
	if(a.cast==0)return ur(0,0);
	if(a.cast==1)return (signed char )(ur(0,0));
	if(a.cast==2)return               (ur(0,8)|ur(1,0));
	if(a.cast==3)return               (ur(1,8)|ur(0,0));
	if(a.cast==4)return (signed short)(ur(0,8)|ur(1,0));
	if(a.cast==5)return (signed short)(ur(1,8)|ur(0,0));
	if(a.cast==6)return               (ur(0,24)|ur(1,16)|ur(2,8)|ur(3,0));
	if(a.cast==7)return               (ur(3,24)|ur(2,16)|ur(1,8)|ur(0,0));
	if(a.cast==8)return (signed int  )(ur(0,24)|ur(1,16)|ur(2,8)|ur(3,0));
	if(a.cast==9)return (signed int  )(ur(3,24)|ur(2,16)|ur(1,8)|ur(0,0));
	return 0x7F&ur(0,0);
}
void array_set_raw(array a,int index,long long int v){
	array_data();
	#define uw(i,s) d[i]=(v>>s)
	if     (a.cast==2||a.cast==4)uw(0,8),uw(1,0);
	else if(a.cast==3||a.cast==5)uw(1,8),uw(0,0);
	else if(a.cast==6||a.cast==8)uw(0,24),uw(1,16),uw(2,8),uw(3,0);
	else if(a.cast==7||a.cast==9)uw(3,24),uw(2,16),uw(1,8),uw(0,0);
	else uw(0,0);
}
lv* array_get(array a,int index,int len){
	if(a.cast==10&&len<0)len=1;
	if(a.cast==10){
		str r=str_new(),s=str_new();a.cast=0;str_provision(&r,len),str_provision(&s,len);
		for(int z=0;z<len;z++)str_addraw(&r,array_get_raw(a,index+z));
		utf8_to_drom(&s,r.sv,len);lmstr(r),a.cast=10;return lmstr(s);
	}
	if(len<0)return lmn(array_get_raw(a,index));GEN(r,len)lmn(array_get_raw(a,index+z));return r;
}
void array_set(array a,int index,int len,lv*v){
	if(len<0)len=1;
	if(array_is(v)){array b=unpack_array(v);for(int z=0;z<len;z++)array_set_raw(a,index+z,array_get_raw(b,z));}
	else if(lis(v)){for(int z=0;z<len;z++)array_set_raw(a,index+z,z>=v->c?0:   v->sv[z] );} // copy chars up to len
	else if(lil(v)){for(int z=0;z<len;z++)array_set_raw(a,index+z,z>=v->c?0:ln(v->lv[z]));} // copy numbers up to len
	else{double vv=ln(v);for(int z=0;z<len;z++)array_set_raw(a,index+z,vv);} // spread a number up to len
}
lv* n_array_slice(lv*self,lv*a){
	array src=unpack_array(self);lv*o=l_first(a);array_offset(o);int cast=a->c>1?ordinal_enum(a->lv[1],casts):src.cast, step=cast_size[cast];offset*=cast_size[src.cast];
	if(len<0)len=(src.size-offset)/step;lv*r=array_make(len,cast,offset,src.data);dset(r->b,lmistr("slice"),ONE);return r;
}
lv* n_array_copy(lv*self,lv*a){
	array src=unpack_array(self);lv*o=l_first(a);array_offset(o);int cast=a->c>1?ordinal_enum(a->lv[1],casts):src.cast, step=cast_size[cast];offset*=cast_size[src.cast];
	if(len<0)len=(src.size-offset)/step;lv*r=array_make(len,cast,0,NULL);array dst=unpack_array(r);
	src.base+=offset;for(int z=0;z<len;z++)array_set_raw(dst,z,array_get_raw(src,z));return r;
}
int struct_size(lv*shape){
	if( lis(shape))return cast_size[ordinal_enum(shape         ,casts)];
	if( lil(shape))return cast_size[ordinal_enum(l_first(shape),casts)]*MAX(0,ln(l_last(shape)));
	if(!lid(shape))return 0;
	int bit=0, r=0;EACH(z,shape){lv*type=shape->lv[z];if(!lin(type)&&bit)bit=0,r++;
		if(lin(type)){bit+=CLAMP(1,ln(type),31);r+=(bit/8),bit%=8;}else{r+=struct_size(type);}
	}return r;
}
lv* struct_read(array*a,lv*shape){
	if(lis(shape)){a->cast=ordinal_enum(shape,casts);lv*r=array_get(array_at(*a,a->here),0,-1);a->here+=cast_size[a->cast];return r;}
	if(lil(shape)){
		int n=MAX(0,ln(l_last(shape)));
		a->cast=ordinal_enum(l_first(shape),casts);lv*r=array_get(array_at(*a,a->here),0, n);a->here+=n*cast_size[a->cast];return r;
	}
	if(!lid(shape))return ZERO;int bit=0;lv*r=lmd();EACH(z,shape){
		lv*type=shape->lv[z],*v=ZERO;if(!lin(type)&&bit)bit=0,a->here++;
		if(lin(type)){
			int n=CLAMP(1,ln(type),31);long long t=0;a->cast=0;
			while(n>0){t=(t<<1)|(1&((int)array_get_raw(*a,a->here)>>(7-bit))),bit++,n--;if(bit==8)bit=0,a->here++;}v=lmn(t);
		}else{v=struct_read(a,type);}dset(r,shape->kv[z],v);
	}return r;
}
void struct_write(array*a,lv*shape,lv*value){
	if(lis(shape)||lil(shape)){
		int n=lis(shape)?1:MAX(0,ln(l_last(shape)));a->cast=ordinal_enum(lis(shape)?shape:l_first(shape),casts);
		array_set(array_at(*a,a->here),0,n,value),a->here+=n*cast_size[a->cast];return;
	}
	if(!lid(shape))return;int bit=0;EACH(z,shape){
		lv*type=shape->lv[z],*v=dgetv(value,shape->kv[z]);
		if(!lin(type)){if(bit)bit=0,a->here++;struct_write(a,type,v);continue;}
		int n=CLAMP(1,ln(type),31);a->cast=0;unsigned long long t=ln(v),m=(1<<n)-1;t&=m;for(int z=0;z<n;z++){
			int pos=1<<(7-bit),dst=(int)array_get_raw(*a,a->here)&~pos;
			array_set_raw(*a,a->here,t&(1<<(n-1-z))?dst|pos:dst),bit++;if(bit==8)bit=0,a->here++;
		}
	}
}
lv* n_array_struct(lv*self,lv*z){
	array a=unpack_array(self);lv*shape=l_first(z),*value=z->c>1?z->lv[1]:NULL;int size=struct_size(shape);
	if(value&&a.here+size>=a.size){array_resize(self,a.here+size),a=unpack_array(self);}
	lv*r=value?(struct_write(&a,shape,value),value):struct_read(&a,shape);dset(self->b,lmistr("here"),lmn(a.here));return r;
}
lv* n_array_cat(lv*self,lv*z){
	array a=unpack_array(self);EACH(i,z){lv*v=z->lv[i];
		lv*s=lin(v)     ?     lmistr(casts[a.cast])             :
		     lil(v)     ?lml2(lmistr(casts[a.cast]),l_count(v)) :
		     array_is(v)?lml2(ifield(v,"cast"),ifield(v,"size")):
		     (v=ls(v),   lml2(lmistr("char")       ,l_count(v)));
		n_array_struct(self,lml2(s,v));
	}return self;
}
lv* array_write(lv*x){
	if(!array_is(x))return lms(0);array a=unpack_array(x);int f=a.cast+'0';a.cast=0;
	lv*r=lms(a.size);for(int z=0;z<a.size;z++)r->sv[z]=0xFF&(int)array_get_raw(a,z);return data_write("DAT",f,r);
}
lv* array_read(lv*x){char f=0;lv*data=data_read("DAT",&f,x);return data?array_make(data->c,CLAMP(0,(f-'0'),10),0,data):array_make(0,0,0,NULL);}
lv* n_array(lv*self,lv*a){
	(void)self;if(lis(l_first(a)))return array_read(l_first(a));
	int size=ln(l_first(a)),cast=a->c>1?ordinal_enum(a->lv[1],casts):0;return array_make(size,cast,0,NULL);
}
lv* interface_array(lv*self,lv*i,lv*x){
	array a=unpack_array(self);
	if(!lis(i)){array_offset(i);if(x){array_set(a,offset,len,x);return x;}else{return array_get(a,offset,len);}}
	if(x){
		ikey("size"){array_resize(self,ln(x)*cast_size[a.cast]);return x;}
		ikey("cast"){dset(self->b,lmistr("cast"),lmn(ordinal_enum(x,casts)));return x;}
		ikey("here"){dset(self->b,lmistr("here"),lmn(MAX(0,ln(x))));return x;}
	}else{
		ikey("encoded")return array_write(self);
		ikey("cast"   )return lmistr(casts[a.cast]);
		ikey("size"   )return lmn(a.size/cast_size[a.cast]);
		ikey("here"   )return lmn(a.here);
		ikey("struct" )return lmnat(n_array_struct,self);
		ikey("slice"  )return lmnat(n_array_slice,self);
		ikey("copy"   )return lmnat(n_array_copy,self);
		ikey("cat"    )return lmnat(n_array_cat,self);
	}return x?x:LNIL;
}

// Bits interface

#define lbits(x) (0xFFFFFFFF&((long long unsigned int)ln(x)))
lv* conformb(lv*z,lv*(f(lv*,lv*))){if(z->c<2)z=l_first(z);lv*r=l_first(z);for(int i=1;i<z->c;i++)r=conform(r,z->lv[i],f);return r;}
lv* a_bits_and(lv*x,lv*y){return lmn(lbits(x)&lbits(y));}lv* n_bits_and(lv*self,lv*z){(void)self;return conformb(z,a_bits_and);}
lv* a_bits_or (lv*x,lv*y){return lmn(lbits(x)|lbits(y));}lv* n_bits_or (lv*self,lv*z){(void)self;return conformb(z,a_bits_or );}
lv* a_bits_xor(lv*x,lv*y){return lmn(lbits(x)^lbits(y));}lv* n_bits_xor(lv*self,lv*z){(void)self;return conformb(z,a_bits_xor);}
lv* interface_bits(lv*self,lv*i,lv*x){
	ikey("and")return lmnat(n_bits_and,self);
	ikey("or" )return lmnat(n_bits_or ,self);
	ikey("xor")return lmnat(n_bits_xor,self);
	return x?x:LNIL;
}

// Pointer interface

pair pointer={0,0}, pointer_start={0,0}, pointer_prev={0,0}, pointer_end={0,0}; int pointer_held=0, pointer_down=0, pointer_up=0;
lv* interface_pointer(lv*self,lv*i,lv*x){
	ikey("held" )return lmn(pointer_held);
	ikey("down" )return lmn(pointer_down);
	ikey("up"   )return lmn(pointer_up);
	ikey("pos"  )return lmpair(pointer);
	ikey("start")return lmpair(pointer_start);
	ikey("prev" )return lmpair(pointer_prev);
	ikey("end"  )return lmpair(pointer_end);
	return x?x:LNIL;(void)self;
}

// Common widget fields
enum show_style{show_solid,show_invert,show_transparent,show_none};
char*widget_shows[]={"solid","invert","transparent","none",NULL};
typedef struct {rect size;int show,locked;} widget;
widget unpack_widget(lv*x){
	return (widget){
		rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size"))),
		ordinal_enum(ifield(x,"show"),widget_shows),
		lb(ifield(x,"locked")),
	};
}
lv* value_inherit(lv*self,lv*key){
	lv*card=dget(self->b,lmistr("card")),*r=dget(self->b,key);if(!contraption_is(card))return r;
	lv*p=dget(ifield(ifield(card,"def"),"widgets"),ifield(self,"name"));if(!p)return r;
	lv*v=iwrite(p,key,NULL);if(r&&v&&matchr(r,v))self->b=l_drop(key,self->b);return r?r:v;
}

// Canvas interface

typedef struct {rect size;float scale;int border,draggable,show,locked;} canvas;
canvas unpack_canvas(lv*x){
	return (canvas){
		rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size"))),
		ln(ifield(x,"scale")),
		lb(ifield(x,"border")),
		lb(ifield(x,"draggable")),
		ordinal_enum(ifield(x,"show"),widget_shows),
		lb(ifield(x,"locked")),
	};
}
lv* n_canvas_clip(lv*self,lv*z); // forward ref
void canvas_size(lv*self,pair size){
	lv*i=dget(self->b,lmistr("image"));if(!i)return;
	float scale=ln(ifield(self,"scale"));
	image_resize(i,(pair){ceil(size.x/scale),ceil(size.y/scale)});
	n_canvas_clip(self,lml(0));
}
lv* container_image(lv*self,int build){
	pair size=getpair(ifield(self,"size"));lv*image=dget(self->b,lmistr("image"));
	if(!image&&build){
		lv*i=ivalue(self,"image");float scale=!canvas_is(self)?1.0:ln(ifield(self,"scale"));
		image=i?image_clone(i):image_make(lmbuff((pair){ceil(size.x/scale),ceil(size.y/scale)}));
		dset(self->b,lmistr("image"),image);
	}return image;
}
void pick_canvas(lv*self){
	lv*image=container_image(self,1),*clip=ivalue(self,"clip");pair size=image_size(image);
	frame=(cstate){
		ln(ifield(self,"brush")),
		ln(ifield(self,"pattern")),
		size,
		clip?getrect(clip):(rect){0,0,size.x,size.y},
		image->b,
		ifield(self,"font"),
	};
}
rect unpack_rect(lv*z){
	rect v=rect_pair((pair){0,0},frame.size);
	if(z->c>=1){pair a=unpack_pair(z,0),b=unpack_pair(z,1);if(b.x<0){a.x+=1+b.x;b.x*=-1;}if(b.y<0){a.y+=1+b.y;b.y*=-1;}v=rect_pair(a,b);}
	return unpack_anchor(v,z,2);
}
lv* n_canvas_invert(lv*self,lv*z){
	lv*card=dget(self->b,lmistr("card")),*deck=dget(card->b,lmistr("deck")),*patterns=dget(deck->b,lmistr("patterns"));
	pick_canvas(self);draw_invert_raw(patterns_pal(patterns),box_intersect(unpack_rect(z),frame.clip));return self;
}
lv* n_canvas_box(lv*self,lv*z){
	lv*card=dget(self->b,lmistr("card")),*deck=dget(card->b,lmistr("deck"));
	pick_canvas(self);draw_box_fancy(unpack_rect(z),frame.brush,frame.pattern,deck);return self;
}
lv* n_canvas_rect (lv*self,lv*z){pick_canvas(self);draw_rect(box_intersect(unpack_rect(z),frame.clip),frame.pattern);return self;}
lv* n_canvas_clear(lv*self,lv*z){pick_canvas(self);draw_rect(box_intersect(unpack_rect(z),frame.clip),0);return self;}
lv* n_canvas_fill(lv*self,lv*z){pick_canvas(self);draw_fill(unpack_pair(z,0),frame.pattern,NULL);return self;}
lv* n_canvas_copy(lv*self,lv*z){return n_image_copy(container_image(self,1),z);}
lv* n_canvas_paste(lv*self,lv*z){
	pick_canvas(self);lv*c=unpack_image(z,0),*pos=z->c>=2?ll(z->lv[1]):lml(0);int solid=z->c>=3?!lb(z->lv[2]):1;
	if(pos->c<=2){buffer_paste(rect_pair(getpair(pos),image_size(c)),frame.clip,c->b,frame.buffer,solid);}
	else{draw_scaled(getrect(pos),c->b,solid);}return self;
}
lv* n_canvas_clip(lv*self,lv*z){
	cstate f=frame;pick_canvas(self);rect w=rect_pair((pair){0,0},frame.size);
	frame.clip=(!lil(z)||z->c<1)?w:box_intersect(w,unpack_rect(z));
	dset(self->b,lmistr("clip"),lmrect(frame.clip));frame=f;return self;
}
lv* rtext_cast(lv*x); // forward ref
lv* n_canvas_text(lv*self,lv*z){
	pick_canvas(self);lv*t=lit(l_first(z))?rtext_cast(l_first(z)):ls(l_first(z)),*deck=dget(dget(self->b,lmistr("card"))->b,lmistr("deck"));
	if(z->c>=2&&lil(z->lv[1])&&z->lv[1]->c>=4) {
		rect r=getrect(z->lv[1]);int a=2<z->c?ordinal_enum(z->lv[2],anchor):0, align=(a==0||a==3||a==6)?align_left:(a==2||a==5||a==8)?align_right:align_center;
		#define valign int x=align==align_left?0:align==align_right?r.w-s.x:(r.w-s.x)/2, y=(a==0||a==1||a==2)?0:(a==6||a==7||a==8)?r.h-s.y:(r.h-s.y)/2;
		#define rbox   box_intersect((rect){r.x+x,r.y+y,s.x,s.y},frame.clip)
		if(lit(t)){pair s=layout_richtext (deck,t,frame.font,align,r.w)           ;valign;draw_text_rich(rbox,frame.pattern,0);}
		else      {pair s=layout_plaintext(t->sv,frame.font,align,(pair){r.w,r.h});valign;draw_text_wrap(rbox,frame.pattern  );}
	}else{
		if(lit(t))return n_canvas_text(self,lml2(t,lmrect(rect_pair(unpack_pair(z,1),(pair){RTEXT_END/1000,RTEXT_END}))));
		rect pos=unpack_anchor(rect_pair(unpack_pair(z,1),font_textsize(frame.font,ls(l_first(z))->sv)),z,2);
		draw_text(pos,ls(l_first(z))->sv,frame.font,frame.pattern);
	}return self;
}
lv* n_canvas_textsize(lv*self,lv*z){
	lv*t=rtext_cast(l_first(z)),*f=ifield(self,"font");int w=z->c>1?ln(z->lv[1]):RTEXT_END;
	lv*deck=dget(dget(self->b,lmistr("card"))->b,lmistr("deck"));pair s=layout_richtext(deck,t,f,align_left,w);
	if(z->c<=1){s.x=0;for(int z=0;z<lines_count;z++){rect p=lines[z].pos;s.x=MAX(s.x,p.x+p.w);}}return lmpair(s);
}
void unpack_poly(lv*z){
	poly_count=0;EACH(i,z){
		lv*a=z->lv[i];int f=lil(a);if(f)EACH(j,a)if(lin(a->lv[j])){f=0;break;}
		if(f){EACH(j,a)poly_push(getfpair(a->lv[j]));}else{poly_push(getfpair(a));}
	}if(poly_count==1)poly_push(poly[0]);
}
lv* n_canvas_poly(lv*self,lv*z){pick_canvas(self);unpack_poly(z);draw_poly(frame.pattern);return self;}
lv* n_canvas_line(lv*self,lv*z){
	pick_canvas(self);unpack_poly(z);lv*deck=dget(dget(self->b,lmistr("card"))->b,lmistr("deck"));
	for(int z=0;z<poly_count-1;z++)draw_line(rect_pair(pcast(poly[z]),pcast(poly[z+1])),frame.brush,frame.pattern,deck);return self;
}
lv* n_canvas_merge(lv*self,lv*z){
	pick_canvas(self);
	if(lis(l_first(z))){if(z->c>=2&&image_is(z->lv[1]))buff_merge_op(frame.buffer,z->lv[1]->b,ls(l_first(z))->sv[0]);return self;}
	if(lil(l_first(z)))z=l_first(z);int v[256]={0};pair s[256]={{0}};char*b[256]={0}; // valid src? size, buffer
	for(int p=0;p<z->c&&p<256;p++)if(image_is(z->lv[p])&&!is_empty(z->lv[p]))v[p]=1,s[p]=image_size(z->lv[p]),b[p]=z->lv[p]->b->sv;
	for(int y=0;y<frame.size.y;y++)for(int x=0;x<frame.size.x;x++)if(inclip(x,y)){int p=0xFF&PIX(x,y),c=v[p]?b[p][(x%s[p].x)+(y%s[p].y)*s[p].x]:0;PIX(x,y)=c;}
	return self;
}
lv* normalize_margin(lv*x,pair s){
	rect m=getrect(x);
	return lmrect(rect_max((rect){MIN(m.x,s.x),MIN(m.y,s.y),MIN(m.w,s.x-m.x),MIN(m.h,s.y-m.y)},(rect){0,0,0,0}));
}
lv* n_canvas_segment(lv*self,lv*z){
	pick_canvas(self);if(z->c<1||!image_is(z->lv[0]))return self;lv*i=z->lv[0]->b;
	rect r=getrect(z->c>1?z->lv[1]:NULL),m=getrect(normalize_margin(z->c>2?z->lv[2]:NULL,buff_size(i)));
	r.w=MAX(r.w,m.x+m.w),r.h=MAX(r.h,m.y+m.h);draw_9seg(r,frame.buffer,i,m,frame.clip,0,NULL);return self;
}
lv* interface_canvas(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	lv*data=self->b;lv*card=dget(data,lmistr("card")),*deck=dget(card->b,lmistr("deck")),*fonts=dget(deck->b,lmistr("fonts"));
	if(x){
		ikey("brush"  ){int n=MAX(0,ln(x));if(lis(x)){int v=dgeti(dget(deck->b,lmistr("brushes")),x);if(v!=-1)n=24+v;}dset(data,i,lmn(n));return x;}
		ikey("pattern"){int n=CLAMP(0,ln(x),255);dset(data,i,lmn(n));return x;}
		ikey("font"   ){dset(data,i,normalize_font(fonts,x));return x;}
		if(!lis(i)    ){return interface_image(container_image(self,1),i,x);}
		if(dget(data,lmistr("free")))return x;
		ikey("border"   ){dset(data,i,lmn(lb(x)));return x;}
		ikey("draggable"){dset(data,i,lmn(lb(x)));return x;}
		ikey("lsize"    ){float s=ln(ifield(self,"scale"));pair d=getpair(x);i=lmistr("size");x=lmpair((pair){d.x*s,d.y*s});} // falls through!
		ikey("size"     ){canvas_size(self,getpair(x));}// falls through to widget.size!
		ikey("scale"    ){dset(data,i,lmn(MAX(0.1,ln(x))));canvas_size(self,getpair(ifield(self,"size")));return x;}
	}else{
		if(!lis(i)      ){lv*img=container_image(self,0);return img?interface_image(img,i,x):LNIL;}
		ikey("border"   ){lv*r=dget(data,i);return r?r:ONE;}
		ikey("draggable"){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("brush"    ){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("pattern"  ){lv*r=dget(data,i);return r?r:ONE;}
		ikey("size"     ){lv*r=dget(data,i);return r?r:lmpair((pair){100,100});}
		ikey("scale"    ){lv*r=dget(data,i);return r?r:lmn(1.0);}
		ikey("lsize"    ){pair s=getpair(ifield(self,"size"));float z=ln(ifield(self,"scale"));return lmpair((pair){ceil(s.x/z),ceil(s.y/z)});}
		ikey("clear"    )return lmnat(n_canvas_clear, self);
		ikey("clip"     )return lmnat(n_canvas_clip,  self);
		ikey("rect"     )return lmnat(n_canvas_rect,  self);
		ikey("poly"     )return lmnat(n_canvas_poly,  self);
		ikey("invert"   )return lmnat(n_canvas_invert,self);
		ikey("box"      )return lmnat(n_canvas_box,   self);
		ikey("line"     )return lmnat(n_canvas_line,  self);
		ikey("fill"     )return lmnat(n_canvas_fill,  self);
		ikey("merge"    )return lmnat(n_canvas_merge, self);
		ikey("text"     )return lmnat(n_canvas_text,  self);
		ikey("copy"     )return lmnat(n_canvas_copy,  self);
		ikey("paste"    )return lmnat(n_canvas_paste, self);
		ikey("segment"  )return lmnat(n_canvas_segment,self);
		ikey("textsize" )return lmnat(n_canvas_textsize,self);
	}return interface_widget(self,i,x);
}
lv* canvas_read(lv*x,lv*r){
	x=ld(x);lv*ri=lmi(interface_canvas,lmistr("canvas"),r);
	{lv*k=lmistr("image"),*v=dget(x,k);if(v){lv*i=image_read(v);dset(r,k,i);iwrite(ri,lmistr("size"),lmpair(image_size(i)));}}
	{lv*k=lmistr("clip" ),*v=dget(x,k);if(v)n_canvas_clip(ri,l_list(x));}
	{lv*k=lmistr("size" ),*v=dget(x,k);if(v)dset(ri->b,k,lmpair(getpair(v)));}
	{lv*k=lmistr("scale"),*v=dget(x,k);if(v)dset(ri->b,k,lmn(MAX(0.1,ln(v))));}
	init_field(ri,"border"   ,x);
	init_field(ri,"draggable",x);
	init_field(ri,"brush"    ,x);
	init_field(ri,"pattern"  ,x);
	init_field(ri,"font"     ,x);
	return ri;
}
lv* canvas_write(lv*x){
	lv*data=x->b,*r=lmd();dset(r,lmistr("type"),lmistr("canvas"));pair lsize=getpair(ifield(x,"lsize"));
	{lv*k=lmistr("border"   ),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("image"    ),*v=dget(data,k);if(v&&!is_blank(v)&&!lb(ifield(x,"volatile")))dset(r,k,image_write(v));}
	{lv*k=lmistr("draggable"),*v=dget(data,k);if(v&&ln(v)!=0)dset(r,k,v);}
	{lv*k=lmistr("brush"    ),*v=dget(data,k);if(v&&ln(v)!=0)dset(r,k,v);}
	{lv*k=lmistr("pattern"  ),*v=dget(data,k);if(v&&ln(v)!=1)dset(r,k,v);}
	{lv*k=lmistr("scale"    ),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("clip"     ),*v=dget(data,k);if(v&&!matchr(v,lmrect((rect){0,0,lsize.x,lsize.y})))dset(r,k,v);}
	return r;
}

// Button interface

enum button_style{button_round,button_rect,button_check,button_invisible,button_radio};
typedef struct {char*text;rect size;lv*font;int style,show,locked;char shortcut;} button;
char*button_styles[]={"round","rect","check","invisible",NULL};
button unpack_button(lv*x){
	return (button){
		ifield(x,"text")->sv,
		rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size"))),
		ifield(x,"font"),
		ordinal_enum(ifield(x,"style"),button_styles),
		ordinal_enum(ifield(x,"show"),widget_shows),
		lb(ifield(x,"locked")),
		ifield(x,"shortcut")->sv[0],
	};
}
lv* normalize_shortcut(lv*x){char c=drom_tolower(ls(x)->sv[0]);if((c>='0'&&c<='9')||(c>='a'&&c<='z')||c==' '){lv*r=lms(1);r->sv[0]=c;return r;}return lms(0);}
lv* interface_button(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	if(x){
		ikey("value"   ){dset(self->b,i,lmn(lb(x)));return x;}
		ikey("text"    ){dset(self->b,i,ls(x));return x;}
		ikey("style"   ){dset(self->b,i,normalize_enum(x,button_styles));return x;}
		ikey("shortcut"){dset(self->b,i,normalize_shortcut(x));return x;}
	}else{
		ikey("value"   ){lv*r=value_inherit(self,i);return r?r:ZERO;}
		ikey("text"    ){lv*r=dget(self->b,i);return r?r:lmistr("");}
		ikey("style"   ){lv*r=dget(self->b,i);return r?r:lmistr(button_styles[0]);}
		ikey("size"    ){lv*r=dget(self->b,i);return r?r:lmpair((pair){60,20});}
		ikey("shortcut"){lv*r=dget(self->b,i);return r?r:lmistr("");}
	}return interface_widget(self,i,x);
}
lv* button_read(lv*x,lv*r){
	x=ld(x),r=lmi(interface_button,lmistr("button"),r);
	init_field(r,"text"    ,x);
	init_field(r,"style"   ,x);
	init_field(r,"value"   ,x);
	init_field(r,"shortcut",x);
	return r;
}
lv* button_write(lv*x){
	lv*data=x->b,*r=lmd();dset(r,lmistr("type"),lmistr("button"));
	{lv*k=lmistr("text" ),*v=dget(data,k);if(v&&v->c)dset(r,k,v);}
	{lv*k=lmistr("style"),*v=dget(data,k);if(v&&strcmp(button_styles[0],v->sv))dset(r,k,v);}
	{lv*k=lmistr("value"),*v=dget(data,k);if(v&&!lb(ifield(x,"volatile")))dset(r,k,v);}
	{lv*k=lmistr("shortcut"),*v=dget(data,k);if(v&&v->c)dset(r,k,v);}
	return r;
}

// Rich Text Manipulation

int rtext_len(lv*table){lv*t=dget(table,lmistr("text"));int r=0;EACH(z,t)r+=t->lv[z]->c;return r;}
int rtext_get(lv*table,int x){lv*t=dget(table,lmistr("text"));int i=0;EACH(z,t){i+=t->lv[z]->c;if(i>=x)return z;}return -1;}
pair rtext_getr(lv*table,int x){lv*t=dget(table,lmistr("text"));int i=0;EACH(z,t){int c=t->lv[z]->c;if(i+c>=x)return (pair){i,i+c};i+=c;}return(pair){x,x};}
lv* rtext_font(lv*table,int x){int i=rtext_get(table,x);return i<0?lmistr(""):dget(table,lmistr("font"))->lv[i];}
int rtext_is_plain(lv*x){
	if(!lit(x)||!dget(x,lmistr("text"))||!dget(x,lmistr("font"))||!dget(x,lmistr("arg"))||x->n>1)return 0;if(x->n==0)return 1;
	lv*f=dget(x,lmistr("font"))->lv[0],*a=dget(x,lmistr("arg"))->lv[0];return !strcmp(f->sv,"")&&!image_is(a)&&!strcmp(ls(a)->sv,"");
}
lv* rtext_is_image(lv*x){
	lv*r=NULL,*t=dget(x,lmistr("text")),*a=dget(x,lmistr("arg")); // look for at least one image, and other spans must be only whitespace.
	for(int z=0;z<x->n;z++){if(image_is(a->lv[z])){if(!r)r=a->lv[z];}else if((int)strspn(t->lv[z]->sv," \n")!=t->lv[z]->c){return NULL;}}return r;
}
int rtext_append(lv*table,lv*text,lv*font,lv*arg){
	if(image_is(arg)){if(text->c>1)text=lmistr("i");if(text->c<1)return 0;}if(!text->c)return 0; // NOTE: this routine modifies <table> in place!
	lv*t=dget(table,lmistr("text")),*f=dget(table,lmistr("font")),*a=dget(table,lmistr("arg"));
	if(t->c&&matchr(font,l_last(f))&&!image_is(arg)&&matchr(arg,l_last(a))){str u=str_new();str_addl(&u,t->lv[t->c-1]),str_addl(&u,text),t->lv[t->c-1]=lmstr(u);}
	else{ll_add(t,text),ll_add(f,font),ll_add(a,arg);}torect(table);return text->c;
}
void rtext_appendr(lv*table,lv*suffix){
	lv*t=dget(suffix,lmistr("text")),*f=dget(suffix,lmistr("font")),*a=dget(suffix,lmistr("arg"));
	EACH(z,t)rtext_append(table,t->lv[z],f->lv[z],a->lv[z]);
}
lv* rtext_string(lv*table,pair cursor){
	str r=str_new(); int i=0,a=MIN(cursor.x,cursor.y),b=MAX(cursor.x,cursor.y); lv*t=dget(table,lmistr("text"));
	EACH(z,t){lv*s=t->lv[z];for(int z=0;z<s->c;z++,i++)if(i>=a&&i<b)str_addc(&r,s->sv[z]);}
	return lmstr(r);
}
lv* rtext_all(lv*table){return rtext_string(table,(pair){0,RTEXT_END});}
lv* rtext_span(lv*table,pair cursor){
	lv*r=l_take(ZERO,table); int i=0,c=0, a=MIN(cursor.x,cursor.y),b=MAX(cursor.x,cursor.y);
	lv*t=dget(table,lmistr("text")),*f=dget(table,lmistr("font")),*g=dget(table,lmistr("arg"));
	while(c<t->c&&(i+t->lv[c]->c)<a)i+=t->lv[c++]->c; // skip whole preceding chunks
	if(c<t->c&&i<=a){ // copy lead-in
		str rr=str_new();for(int z=0;z<t->lv[c]->c;z++,i++)if(i>=a&&i<b)str_addc(&rr,t->lv[c]->sv[z]);
		rtext_append(r,lmstr(rr),f->lv[c],g->lv[c]);c++;
	}
	while(c<t->c&&(i+t->lv[c]->c)<b)i+=rtext_append(r,t->lv[c],f->lv[c],g->lv[c]),c++; // copy whole chunks
	if(c<t->c&&i<b){ // copy lead-out
		str rr=str_new();for(int z=0;z<t->lv[c]->c;z++,i++)if(i>=a&&i<b)str_addc(&rr,t->lv[c]->sv[z]);
		rtext_append(r,lmstr(rr),f->lv[c],g->lv[c]);
	}return r;
}
lv* n_rtext_make(lv*self,lv*z){
	(void)self;lv*r=lmt();
	lv*a=z->c<3?lmistr(""): image_is(z->lv[2])?z->lv[2]: ls(z->lv[2]);
	lv*f=z->c<2?lmistr(""): ls(z->lv[1]);
	lv*t=image_is(a)?lmistr("i"): z->c<1?lmistr(""): ls(z->lv[0]);
	dset(r,lmistr("text"),l_list(t)),dset(r,lmistr("font"),l_list(f)),dset(r,lmistr("arg"),l_list(a));
	return torect(r);
}
lv* rtext_cast(lv*x){
	if(!x)x=lmistr("");
	if(image_is(x))return n_rtext_make(NULL,lml3(lmistr(""),lmistr(""),x));
	if(lid(x))x=l_table(x);if(!lit(x))return n_rtext_make(NULL,l_list(ls(x)));
	lv*t=lmistr("text"),*f=lmistr("font"),*a=lmistr("arg"),*tv=dget(x,t),*fv=dget(x,f),*av=dget(x,a);
	if(x->c==3&&tv&&fv&&av){int v=1;EACH(z,tv){if(!lis(tv->lv[z])||!lis(fv->lv[z])||(!lis(av->lv[z])&&!image_is(av->lv[z])))v=0;break;}if(v)return x;}lv*r=lmt();
	{lv*v=dget(x,t);dset(r,t,v?v:l_list(lmistr("")));}
	{lv*v=dget(x,f);dset(r,f,v?v:l_list(lmistr("")));}
	{lv*v=dget(x,a);dset(r,a,v?v:l_list(lmistr("")));}
	torect(r);for(int z=0;z<r->n;z++){
		int i=image_is(r->lv[2]->lv[z]);
		r->lv[0]->lv[z]=i?lmistr("i"):ls(r->lv[0]->lv[z]);
		r->lv[1]->lv[z]=ls(r->lv[1]->lv[z]);
		r->lv[2]->lv[z]=i?r->lv[2]->lv[z]:ls(r->lv[2]->lv[z]);
	}return torect(r);
}
lv* rtext_splice(lv*table,lv*font,lv*arg,char*text,pair cursor,pair*endcursor){
	int a=MIN(cursor.x,cursor.y),b=MAX(cursor.x,cursor.y); lv*r=rtext_cast(NULL),*t=lmcstr(text);
	rtext_appendr(r,rtext_span(table,(pair){0,a}));
	rtext_append (r,t,font,arg);
	rtext_appendr(r,rtext_span(table,(pair){b,RTEXT_END}));
	endcursor->x=endcursor->y=a+t->c; return r;
}
lv* rtext_splicer(lv*table,lv*insert,pair cursor,pair*endcursor){
	int a=MIN(cursor.x,cursor.y),b=MAX(cursor.x,cursor.y); lv*r=rtext_cast(NULL);
	rtext_appendr(r,rtext_span(table,(pair){0,a}));
	rtext_appendr(r,insert);
	rtext_appendr(r,rtext_span(table,(pair){b,RTEXT_END}));
	endcursor->x=endcursor->y=a+rtext_len(insert); return r;
}
lv* rtext_read(lv*x){
	if(lis(x))return x; x=ld(x);
	lv*a=dget(x,lmistr("arg"));if(a){MAP(n,a)has_prefix(ls(a->lv[z])->sv,"%%img")?image_read(ls(a->lv[z])):ls(a->lv[z]);dset(x,lmistr("arg"),n);}
	return rtext_cast(x);
}
lv* rtext_write(lv*x){
	x=l_cols(x);
	lv*a=dget(x,lmistr("arg"));if(a){MAP(n,a)image_is(a->lv[z])?image_write(a->lv[z]):a->lv[z];dset(x,lmistr("arg"),n);}
	return x;
}
lv* rtext_encode(lv*x){str r=str_new();str_addz(&r,"%%RTX0");fjson(&r,rtext_write(x));return lmstr(r);}
lv* rtext_decode(lv*x){int f=1,i=6,n=x->c-i;lv*v=rtext_read(pjson(x->sv,&i,&f,&n));return v;}
lv*n_rtext_split(lv*self,lv*z){
	(void)self;if(z->c<2)return lml(0);lv*d=ls(z->lv[0]),*v=rtext_cast(z->lv[1]),*t=rtext_string(v,(pair){0,RTEXT_END}),*r=lml(0);
	if(d->c<1)return r;int n=0;EACH(z,t){
		int m=1;EACH(w,d)if(d->sv[w]!=t->sv[z+w]){m=0;break;}if(!m)continue;
		ll_add(r,rtext_span(v,(pair){n,z})),z+=d->c-1,n=z+1;
	}if(n<=t->c)ll_add(r,rtext_span(v,(pair){n,t->c}));return r;
}
lv*n_rtext_len   (lv*self,lv*z){(void)self;return lmn(rtext_len(rtext_cast(l_first(z))));}
lv*n_rtext_get   (lv*self,lv*z){(void)self;return lmn(rtext_get(rtext_cast(l_first(z)),z->c<2?0:ln(z->lv[1])));}
lv*n_rtext_string(lv*self,lv*z){(void)self;return rtext_string(rtext_cast(l_first(z)),z->c<2?(pair){0,RTEXT_END}:unpack_pair(z,1));}
lv*n_rtext_span  (lv*self,lv*z){(void)self;return rtext_span  (rtext_cast(l_first(z)),z->c<2?(pair){0,RTEXT_END}:unpack_pair(z,1));}
lv*n_rtext_cat   (lv*self,lv*z){(void)self;lv*r=l_take(ZERO,rtext_cast(lmt()));EACH(i,z)rtext_appendr(r,rtext_cast(z->lv[i]));return r;}
lv*n_rtext_replace(lv*self,lv*z){
	if(z->c<3)return l_first(z);lv*t=rtext_cast(z->lv[0]),*k=z->lv[1],*v=z->lv[2],*r=lml(0),*text=rtext_string(t,(pair){0,RTEXT_END});
	if(!lil(k))k=l_list(k);if(!lil(v))v=l_list(v);int nocase=z->c>=4&&lb(z->lv[3]);
	k=l_take(lmn(MAX(k->c,v->c)),k);EACH(z,k)k->lv[z]=ls(k->lv[z]);
	v=l_take(lmn(MAX(k->c,v->c)),v);EACH(z,v)v->lv[z]=rtext_cast(v->lv[z]);
	pair c={0,0};while(c.y<text->c){
		int any=0;EACH(ki,k){
			lv*key=k->lv[ki],*val=v->lv[ki];if(key->c<1)break;int f=1;
			if(nocase){EACH(i,key)if(drom_tolower(text->sv[c.y+i])!=drom_tolower(key->sv[i])){f=0;break;}}
			else      {EACH(i,key)if(             text->sv[c.y+i] !=             key->sv[i] ){f=0;break;}}
			if(f){if(c.x!=c.y)ll_add(r,rtext_span(t,c));ll_add(r,val);c.x=c.y=(c.y+key->c),any=1;}
		}if(!any)c.y++;
	}if(c.x<text->c)ll_add(r,rtext_span(t,(pair){c.x,RTEXT_END}));return n_rtext_cat(self,r);
}
lv*n_rtext_find(lv*self,lv*z){
	(void)self;lv*r=lml(0);if(z->c<2)return r;int nocase=z->c>=3&&lb(z->lv[2]);
	lv*text=lit(z->lv[0])?rtext_all(rtext_cast(z->lv[0])): ls(z->lv[0]), *k=z->lv[1];
	if(!lil(k))k=l_list(k);EACH(z,k)k->lv[z]=ls(k->lv[z]);
	for(int x=0;x<text->c;){
		int any=0;EACH(ki,k){
			lv*key=k->lv[ki];int f=1;
			if(nocase){EACH(i,key)if(drom_tolower(text->sv[x+i])!=drom_tolower(key->sv[i])){f=0;break;}}
			else      {EACH(i,key)if(             text->sv[x+i] !=             key->sv[i] ){f=0;break;}}
			if(f){ll_add(r,lml2(lmn(x),lmn(x+key->c)));x+=MAX(1,key->c);any=1;break;}
		}if(!any)x++;
	}return r;
}
lv*n_rtext_index(lv*self,lv*z){
	(void)self;if(!z->c)return ZERO;int r=0;lv*t=rtext_all(rtext_cast(l_first(z)));pair g=z->c>1?getpair(z->lv[1]):(pair){0,0};
	while(r<t->c&&g.x>0)if(t->sv[r++]=='\n')g.x--; while(r<t->c&&g.y>0&&t->sv[r]!='\n'){g.y--,r++;} return lmn(r);
}
lv* rtext_read_images(lv*x){lv*r=lml(0),*a=dget(x,lmistr("arg"));if(a)EACH(z,a)if(image_is(a->lv[z]))ll_add(r,a->lv[z]);return r;}
lv* rtext_write_images(lv*x){return n_rtext_cat(NULL,ll(x));}
lv* interface_rtext(lv*self,lv*i,lv*x){
	ikey("end"    )return lmn(RTEXT_END);
	ikey("make"   )return lmnat(n_rtext_make   ,self);
	ikey("len"    )return lmnat(n_rtext_len    ,self);
	ikey("get"    )return lmnat(n_rtext_get    ,self);
	ikey("index"  )return lmnat(n_rtext_index  ,self);
	ikey("string" )return lmnat(n_rtext_string ,self);
	ikey("span"   )return lmnat(n_rtext_span   ,self);
	ikey("split"  )return lmnat(n_rtext_split  ,self);
	ikey("replace")return lmnat(n_rtext_replace,self);
	ikey("find"   )return lmnat(n_rtext_find   ,self);
	ikey("cat"    )return lmnat(n_rtext_cat    ,self);
	return x?x:LNIL;
}

// Field interface

enum field_types{text_body,text_heading,text_fixed,text_link};
enum field_style{field_rich,field_plain,field_code};
typedef struct {rect size;lv*font;int show,scrollbar,border,style,align,locked;} field;
typedef struct {lv*table;int scroll;} field_val;
char*field_styles[]={"rich","plain","code",NULL};
char*field_aligns[]={"left","center","right",NULL};
field unpack_field(lv*x,field_val*value){
	if(value){value->table=ifield(x,"value"),value->scroll=ln(ifield(x,"scroll"));}
	return (field){
		rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size"))),
		ifield(x,"font"),
		ordinal_enum(ifield(x,"show"),widget_shows),
		lb(ifield(x,"scrollbar")),
		lb(ifield(x,"border")),
		ordinal_enum(ifield(x,"style"),field_styles),
		ordinal_enum(ifield(x,"align"),field_aligns),
		lb(ifield(x,"locked")),
	};
}
lv* n_field_scrollto(lv*self,lv*x){
	field_val v;field f=unpack_field(self,&v);rect bi=inset(f.size,2);if(f.scrollbar)bi.w-=12+3; // arrow size+pad
	layout_richtext(ivalue(ivalue(self,"card"),"deck"),v.table,f.font,f.align,bi.w);
	int i=ln(l_first(x));rect c=layout[MIN(MAX(0,i),(layout_count-1))].pos;c.y-=v.scroll;int ch=MIN(bi.h,c.h),t=v.scroll;
	if(c.y<0){t+=c.y;}if(c.y+ch>=bi.h){t+=((c.y+ch)-bi.h);}if(t!=v.scroll)iwrite(self,lmistr("scroll"),lmn(t));
	return self;
}
lv* interface_field(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	lv*data=self->b;
	if(x){
		ikey("text"     ){dset(data,lmistr("value"),rtext_cast(ls(x)));field_notify(self);return x;}
		ikey("images"   ){dset(data,lmistr("value"),rtext_write_images(x));field_notify(self);return x;}
		ikey("data"     ){dset(data,lmistr("value"),rtext_cast(l_format(lmistr("%J"),l_list(x))));field_notify(self);return x;}
		ikey("scroll"   ){int n=MAX(0,ln(x));dset(data,i,lmn(n));return x;}
		ikey("value"    ){
			lv*style=ivalue(self,"style");int plain=style&&strcmp(ls(style)->sv,"rich");
			if(plain&&!rtext_is_plain(x)){x=rtext_all(rtext_cast(x));}
			dset(data,i,rtext_cast(x));field_notify(self);return x;
		}
		ikey("border"   ){dset(data,i,lmn(lb(x)));return x;}
		ikey("scrollbar"){dset(data,i,lmn(lb(x)));return x;}
		ikey("style"    ){dset(data,i,normalize_enum(x,field_styles));iwrite(self,lmistr("value"),dget(data,lmistr("value")));return x;}
		ikey("align"    ){dset(data,i,normalize_enum(x,field_aligns));return x;}
	}else{
		ikey("text"     ){lv*r=value_inherit(self,lmistr("value"));return r?rtext_all(r):lmistr("");}
		ikey("images"   ){lv*r=value_inherit(self,lmistr("value"));return r?rtext_read_images(r):lml(0);}
		ikey("data"     ){lv*r=value_inherit(self,lmistr("value"));return r?l_parse(lmistr("%J"),rtext_all(r)):LNIL;}
		ikey("value"    ){lv*r=value_inherit(self,i);return r?r:rtext_cast(NULL);}
		ikey("scroll"   ){lv*r=value_inherit(self,i);return r?r:ZERO;}
		ikey("scrollbar"){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("border"   ){lv*r=dget(data,i);return r?r:ONE;}
		ikey("style"    ){lv*r=dget(data,i);return r?r:lmistr(field_styles[0]);}
		ikey("align"    ){lv*r=dget(data,i);return r?r:lmistr(field_aligns[0]);}
		ikey("size"     ){lv*r=dget(data,i);return r?r:lmpair((pair){100,20});}
		ikey("font"     ){
			lv*card=ivalue(self,"card"),*deck=ivalue(card,"deck"),*fonts=ifield(deck,"fonts");
			lv*style=ivalue(self,"style"); int code=style&&!strcmp(ls(style)->sv,"code");
			lv*r=dget(data,i);return r?dget(fonts,r): code?dget(fonts,lmistr("mono")): fonts->lv[0];
		}
		ikey("scrollto" )return lmnat(n_field_scrollto,self);
	}return interface_widget(self,i,x);
}
lv* field_read(lv*x,lv*r){
	x=ld(x),r=lmi(interface_field,lmistr("field"),r);
	{lv*k=lmistr("value"),*v=dget(x,k);if(v)iwrite(r,k,rtext_read(v));}
	init_field(r,"border"   ,x);
	init_field(r,"scrollbar",x);
	init_field(r,"style"    ,x);
	init_field(r,"align"    ,x);
	init_field(r,"scroll"   ,x);
	return r;
}
lv* field_write(lv*x){
	lv*data=x->b,*r=lmd();dset(r,lmistr("type"),lmistr("field"));int vol=lb(ifield(x,"volatile"));
	{lv*k=lmistr("border"   ),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("scrollbar"),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("style"    ),*v=dget(data,k);if(v&&strcmp(field_styles[0],v->sv))dset(r,k,v);}
	{lv*k=lmistr("align"    ),*v=dget(data,k);if(v&&strcmp(field_aligns[0],v->sv))dset(r,k,v);}
	{lv*k=lmistr("scroll"   ),*v=dget(data,k);if(v&&ln(v)!=0&&!vol)dset(r,k,v);}
	{lv*k=lmistr("value"    ),*v=dget(data,k);if(v&&!vol){if(rtext_is_plain(v)){v=rtext_all(v);if(v->c)dset(r,k,v);}else{dset(r,k,rtext_write(v));}}}
	return r;
}

// Slider interface

enum slider_style{slider_horiz,slider_vert,slider_bar,slider_compact};
typedef struct {rect size;lv*font,*format;int show,style,locked;double min,max,step,value;} slider;
char*slider_styles[]={"horiz","vert","bar","compact",NULL};
double slider_normalize(fpair interval,double step,double n){double r=round(n/step)*step;return CLAMP(interval.x,r,interval.y);}
slider unpack_slider(lv*x){
	lv*interval=ifield(x,"interval");
	return (slider){
		rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size"))),
		ifield(x,"font"),
		ifield(x,"format"),
		ordinal_enum(ifield(x,"show"),widget_shows),
		ordinal_enum(ifield(x,"style"),slider_styles),
		lb(ifield(x,"locked")),
		ln(interval->lv[0]),
		ln(interval->lv[1]),
		ln(ifield(x,"step")),
		ln(ifield(x,"value")),
	};
}
lv* interface_slider(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	lv*data=self->b;
	if(x){
		ikey("value"    ){dset(data,i,lmn(slider_normalize(getfpair(ifield(self,"interval")),ln(ifield(self,"step")),ln(x))));return x;}
		ikey("step"     ){double n=ln(x);dset(data,i,lmn(n<=0?0.000001:n));iwrite(self,lmistr("value"),dget(data,lmistr("value")));return x;}
		ikey("format"   ){dset(data,i,ls(x));return x;}
		ikey("style"    ){dset(data,i,normalize_enum(x,slider_styles));return x;}
		ikey("interval" ){
			fpair v=getfpair(x);dset(data,i,lmfpair((fpair){MIN(v.x,v.y),MAX(v.x,v.y)}));
			iwrite(self,lmistr("value"),ifield(self,"value"));return x;
		}
	}else{
		ikey("value"    ){lv*r=value_inherit(self,i);pair v=getpair(ifield(self,"interval"));return r?r:lmn(CLAMP(v.x,0,v.y));}
		ikey("step"     ){lv*r=dget(data,i);return r?r:ONE;}
		ikey("interval" ){lv*r=dget(data,i);return r?r:lml2(ZERO,lmn(100));}
		ikey("format"   ){lv*r=dget(data,i);return r?r:lmistr("%f");}
		ikey("style"    ){lv*r=dget(data,i);return r?r:lmistr(slider_styles[0]);}
		ikey("size"     ){lv*r=dget(data,i);return r?r:lmpair((pair){100,25});}
	}return interface_widget(self,i,x);
}
lv* slider_read(lv*x,lv*r){
	x=ld(x),r=lmi(interface_slider,lmistr("slider"),r);
	init_field(r,"interval",x);
	init_field(r,"step"    ,x);
	init_field(r,"value"   ,x);
	init_field(r,"format"  ,x);
	init_field(r,"style"   ,x);
	return r;
}
lv* slider_write(lv*x){
	lv*data=x->b,*r=lmd();dset(r,lmistr("type"),lmistr("slider"));
	{lv*k=lmistr("interval"),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("value"   ),*v=dget(data,k);if(v&&ln(v)!=0&&!lb(ifield(x,"volatile")))dset(r,k,v);}
	{lv*k=lmistr("step"    ),*v=dget(data,k);if(v&&ln(v)!=1)dset(r,k,v);}
	{lv*k=lmistr("format"  ),*v=dget(data,k);if(v&&strcmp("%f",v->sv))dset(r,k,v);}
	{lv*k=lmistr("style"   ),*v=dget(data,k);if(v&&strcmp(slider_styles[0],v->sv))dset(r,k,v);}
	return r;
}

// Grid interface

typedef struct {rect size;lv*font;int widths[256];char*format;int headers,scrollbar,lines,bycell,show,locked;} grid;
typedef struct {lv*table;int scroll,row,col;} grid_val;
grid unpack_grid(lv*x,grid_val*value){
	value->table=ifield(x,"value"),value->scroll=ln(ifield(x,"scroll")),value->row=ln(ifield(x,"row")),value->col=ln(ifield(x,"col"));
	grid r={
		rect_pair(getpair(ifield(x,"pos")),getpair(ifield(x,"size"))),
		ifield(x,"font"),
		{0},
		ifield(x,"format")->sv,
		lb(ifield(x,"headers")),
		lb(ifield(x,"scrollbar")),
		lb(ifield(x,"lines")),
		lb(ifield(x,"bycell")),
		ordinal_enum(ifield(x,"show"),widget_shows),
		lb(ifield(x,"locked")),
	};
	lv*w=ifield(x,"widths");r.widths[0]=w->c;EACH(z,w)r.widths[1+z]=ln(w->lv[z]);
	return r;
}
int grid_scrollto(lv*t,grid g,int s,int r){
	int head=g.headers?10+5:0;         // default to body font height
	int row=g.font?font_h(g.font):11;  // default to mono font height
	int n=MIN(t->n,0|((g.size.h-head+1)/(row+5)));
	return (r-s<0)?r: (r-s>=n)?r-(n-1): s;
}
lv* n_grid_scrollto(lv*self,lv*x){
	grid_val v;grid g=unpack_grid(self,&v);
	int t=grid_scrollto(v.table,g,v.scroll,ln(l_first(x)));
	if(t!=v.scroll)iwrite(self,lmistr("scroll"),lmn(t));
	return self;
}
lv* interface_grid(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	lv*data=self->b;
	if(x){
		ikey("value"    ){dset(data,i,lt(x));return x;}
		ikey("scroll"   ){int n=MAX(0,ln(x));dset(data,i,lmn(n));return x;}
		ikey("row"      ){int n=MAX(-1,ln(x));dset(data,i,lmn(n));return x;}
		ikey("col"      ){if(!lin(x))return iwrite(self,lmistr("colname"),x);int n=MAX(-1,ln(x));dset(data,i,lmn(n));return x;}
		ikey("colname"  ){iwrite(self,lmistr("col"),lmn(dgeti(ifield(self,"value"),x)));return x;}
		ikey("cell"     ){iwrite(self,lmistr("col"),l_at(x,lmn(0))),iwrite(self,lmistr("row"),l_at(x,lmn(1)));return x;}
		ikey("scrollbar"){dset(data,i,lmn(lb(x)));return x;}
		ikey("headers"  ){dset(data,i,lmn(lb(x)));return x;}
		ikey("lines"    ){dset(data,i,lmn(lb(x)));return x;}
		ikey("bycell"   ){dset(data,i,lmn(lb(x)));return x;}
		ikey("widths"   ){dset(data,i,normalize_ints(x,255));return x;}
		ikey("format"   ){dset(data,i,ls(x));return x;}
		ikey("rowvalue" ){iwrite(self,lmistr("value"   ),amend(ifield(self,"value"   ),ifield(self,"row"    ),x));return x;}
		ikey("cellvalue"){iwrite(self,lmistr("rowvalue"),amend(ifield(self,"rowvalue"),ifield(self,"colname"),x));return x;}
	}else{
		ikey("value"    ){lv*r=value_inherit(self,i);return r?r:lmt();}
		ikey("scroll"   ){lv*r=value_inherit(self,i);return r?r:ZERO;}
		ikey("scrollbar"){lv*r=dget(data,i);return r?r:ONE;}
		ikey("headers"  ){lv*r=dget(data,i);return r?r:ONE;}
		ikey("lines"    ){lv*r=dget(data,i);return r?r:ONE;}
		ikey("bycell"   ){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("widths"   ){lv*r=dget(data,i);return r?r:lml(0);}
		ikey("format"   ){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("size"     ){lv*r=dget(data,i);return r?r:lmpair((pair){100,50});}
		ikey("cell"     ){return lml2(ifield(self,"col"),ifield(self,"row"));}
		ikey("row"      ){lv*r=value_inherit(self,i),*v=ifield(self,"value");if(!r)return lmn(-1); int n=ln(r); return lmn(CLAMP(-1,n,v->n-1));}
		ikey("col"      ){lv*c=value_inherit(self,i),*v=ifield(self,"value");if(!c)return lmn(-1); int n=ln(c); return lmn(CLAMP(-1,n,v->c-1));}
		ikey("colname"  ){int c=ln(ifield(self,"col"));lv*v=ifield(self,"value");return c<0||c>=v->c?LNIL: v->kv[c];}
		ikey("rowvalue" ){int r=ln(ifield(self,"row"))                         ;lv*v=ifield(self,"value");return r<0||     r>=v->n         ?lmd():l_at(v,lmn(r));}
		ikey("cellvalue"){int r=ln(ifield(self,"row")),c=ln(ifield(self,"col"));lv*v=ifield(self,"value");return r<0||c<0||r>=v->n||c>=v->c?LNIL:v->lv[c]->lv[r];}
		ikey("scrollto" )return lmnat(n_grid_scrollto,self);
	}return interface_widget(self,i,x);
}
lv* grid_read(lv*x,lv*r){
	x=ld(x),r=lmi(interface_grid,lmistr("grid"),r);
	init_field(r,"scrollbar",x);
	init_field(r,"headers"  ,x);
	init_field(r,"lines"    ,x);
	init_field(r,"bycell"   ,x);
	init_field(r,"widths"   ,x);
	init_field(r,"format"   ,x);
	init_field(r,"scroll"   ,x);
	init_field(r,"row"      ,x);
	init_field(r,"col"      ,x);
	{lv*k=lmistr("value"),*v=dget(x,k);if(v)iwrite(r,k,l_table(v));}
	return r;
}
lv* grid_write(lv*x){
	lv*data=x->b,*r=lmd();dset(r,lmistr("type"),lmistr("grid"));int vol=lb(ifield(x,"volatile"));
	{lv*k=lmistr("scrollbar"),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("headers"  ),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("lines"    ),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("bycell"   ),*v=dget(data,k);if(v)dset(r,k,v);}
	{lv*k=lmistr("widths"   ),*v=dget(data,k);if(v&&v->c)dset(r,k,v);}
	{lv*k=lmistr("format"   ),*v=dget(data,k);if(v&&v->c)dset(r,k,v);}
	{lv*k=lmistr("value"    ),*v=dget(data,k);if(v&&!vol)dset(r,k,l_cols(v));}
	{lv*k=lmistr("scroll"   ),*v=dget(data,k);if(v&&ln(v)!= 0&&!vol)dset(r,k,v);}
	{lv*k=lmistr("row"      ),*v=dget(data,k);if(v&&ln(v)!=-1&&!vol)dset(r,k,v);}
	{lv*k=lmistr("col"      ),*v=dget(data,k);if(v&&ln(v)!=-1&&!vol)dset(r,k,v);}
	return r;
}

// Contraption interface

pair corner_reflow(pair p,pair s,rect m,pair d){ // point, proto size, margins, dest size
	p.x=(p.x<m.x)?p.x: (p.x>s.x-m.w)?d.x-(s.x-p.x): s.x==0?0:round(((1.0f*p.x)/s.x)*d.x); // left | right  | stretch horiz
	p.y=(p.y<m.y)?p.y: (p.y>s.y-m.h)?d.y-(s.y-p.y): s.y==0?0:round(((1.0f*p.y)/s.y)*d.y); // top  | bottom | stretch vert
	return p;
}
void contraption_reflow(lv*c){
	lv*def=ifield(c,"def"),*swids=ifield(def,"widgets"),*dwids=ivalue(c,"widgets");
	rect m=getrect(ifield(def,"margin"));pair s=getpair(ifield(def,"size")), d=getpair(ifield(c,"size"));
	EACH(z,swids){
		lv*swid=swids->lv[z],*dwid=dget(dwids,dwids->kv[z]);if(!dwid)continue;
		pair a=getpair(ifield(swid,"pos")), b=pair_add(getpair(ifield(swid,"size")),a);
		a=corner_reflow(a,s,m,d),b=corner_reflow(b,s,m,d);
		iwrite(dwid,lmistr("pos"),lmpair(a)),iwrite(dwid,lmistr("size"),lmpair(pair_sub(b,a)));
	}
}
lv* interface_contraption(lv*self,lv*i,lv*x){
	lv*data=self->b;char*masks[]={
		"name","index","image","script","locked","animated","volatile","pos","show","font","toggle","event","offset","parent",
		NULL
	};
	lv*def=dget(data,lmistr("def"));
	if(!is_rooted(self))return LNIL;
	if(x){
		ikey("def"  )return x; // not mutable!
		ikey("image")return x; // not mutable!
		ikey("size" ){
			rect m=getrect(ifield(def,"margin"));
			dset(data,i,lmpair(pair_max((pair){m.x+m.w,m.y+m.h},getpair(normalize_pair(x)))));
			contraption_reflow(self);return x;
		}
		if(lis(i))for(int z=0;masks[z];z++)if(!strcmp(i->sv,masks[z]))return interface_widget(self,i,x);
		fire_attr_sync(self,"set_",ls(i),x);return x;
	}else{
		ikey("def"  )return dget(data,i);
		ikey("size" )return lb(ifield(def,"resizable"))?dget(data,i):ifield(def,"size");
		ikey("image")return ifield(dget(data,lmistr("def")),"image");
		if(lis(i))for(int z=0;masks[z];z++)if(!strcmp(i->sv,masks[z]))return interface_widget(self,i,NULL);
		return fire_attr_sync(self,"get_",ls(i),NULL);
	}
}
lv*widget_write(lv*x);lv*widget_read(lv*x,lv*card); // forward refs
lv*dict_delta(lv*a,lv*b){lv*r=lmd();EACH(z,b){lv*v=dget(a,b->kv[z]);if(!v||!matchr(v,b->lv[z]))dset(r,b->kv[z],b->lv[z]);}return r;}
lv* contraption_write(lv*x){
	lv*data=x->b,*def=ifield(x,"def"),*r=lmd();
	dset(r,lmistr("type"),lmistr("contraption")),dset(r,lmistr("def"),ifield(def,"name"));
	lv*o=ifield(def,"widgets"),*w=dget(data,lmistr("widgets")),*wids=lmd();EACH(z,w){
		lv*wid=widget_write(w->lv[z]),*n=dget(wid,lmistr("name"));
		lv*src=dget(o,n);if(src)wid=dict_delta(widget_write(src),wid);
		wid=l_drop(lmistr("name"),wid);dset(wids,n,wid);
	}dset(r,lmistr("widgets"),wids);return r;
}
lv* contraption_read(lv*x,lv*r){
	x=ld(x);lv*card=dget(r,lmistr("card")),*deck=dget(card->b,lmistr("deck")),*defs=ifield(deck,"contraptions");
	lv*dname=dget(x,lmistr("def")),*def=dname?dget(defs,dname):NULL;if(!def)return NULL;dset(r,lmistr("def"),def);
	lv*widgets=lmd(),*ri=lmi(interface_contraption,lmistr("contraption"),r);dset(r,lmistr("widgets"),widgets),dset(r,lmistr("deck"),deck);
	lv*d=ifield(def,"widgets"),*w=dget(x,lmistr("widgets"));if(w){w=ld(w);}else{w=lmd();EACH(z,d)dset(w,d->kv[z],lmd());}
	EACH(z,d){
		lv*a=widget_write(d->lv[z]),*o=dget(w,d->kv[z]);if(o)a=l_comma(a,o);
		lv*i=widget_read(a,ri);if(lii(i))dset(widgets,ifield(i,"name"),i);
	}
	{lv*k=lmistr("size"),*v=dget(x,k);iwrite(ri,k,v?v:ifield(def,"size"));}
	{lv*p=lmd();dset(p,lmistr("card"),ri);dset(r,lmistr("viewproxy"),lmi(interface_widget,lmistr("proxy"),p));}
	return ri;
}

// Widget interface

lv* n_toggle(lv*wid,lv*x){
	lv*s=x->c<1?lmistr("solid"):x->lv[0], *v=x->c<2?ZERO:x->lv[1], *o=ifield(wid,"show"), *n=lmistr("none");
	lv*r=(x->c<2?matchr(o,n):(lb(v)&&!matchr(v,n)))?s:n;iwrite(wid,lmistr("show"),r);return r;
}
lv* widget_add(lv*card,lv*x){lv*r=widget_read(x,card);if(lii(r))dset(ivalue(card,"widgets"),ifield(r,"name"),r);return r;}
lv* interface_widget(lv*self,lv*i,lv*x){
	lv*data=self->b,*card=ivalue(self,"card"),*deck=ivalue(card,"deck"),*fonts=ifield(deck,"fonts");
	lv*widgets=ivalue(card,"widgets"),*name=ivalue(self,"name");
	if(x){
		ikey("name"  ){
			int ix=dgeti(widgets,name);lv*n=ukey(widgets,ls(x),ls(x)->sv,dget(data,i));
			widgets->kv[ix]=n;dset(widgets->lv[ix]->b,lmistr("name"),n);return x;
		}
		ikey("index"   ){reorder(widgets,dgeti(widgets,name),ln(x));return x;}
		ikey("font"    ){dset(data,i,normalize_font(fonts,x));return x;}
		ikey("script"  ){dset(data,i,ls(x));return x;}
		ikey("locked"  ){dset(data,i,lmn(lb(x)));return x;}
		ikey("animated"){dset(data,i,lmn(lb(x)));return x;}
		ikey("volatile"){dset(data,i,lmn(lb(x)));return x;}
		ikey("size"    ){dset(data,i,normalize_pair(x));return x;}
		ikey("pos"     ){dset(data,i,lmpair(getpair(x)));return x;}
		ikey("show"    ){dset(data,i,normalize_enum(x,widget_shows));return x;}
	}else{
		ikey("name"    )return dget(data,i);
		ikey("index"   )return lmn(dgeti(widgets,name));
		ikey("script"  ){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("locked"  ){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("animated"){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("volatile"){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("pos"     ){lv*r=dget(data,i);return r?r:lmpair((pair){0,0});}
		ikey("show"    ){lv*r=dget(data,i);return r?r:lmistr(widget_shows[0]);}
		ikey("font"    ){lv*r=dget(data,i);return r?dget(fonts,r): fonts->lv[button_is(self)?1:0];}
		ikey("toggle"  )return lmnat(n_toggle,self);
		ikey("event"   )return lmnat(n_event,self);
		ikey("parent"  )return card;
		ikey("offset"  ){
			pair c=getpair(ifield(card,"size")),p=getpair(ifield(self,"pos")),d=getpair(ivalue(deck,"size"));
			lv*con=card;while(contraption_is(con)){pair o=getpair(ifield(con,"pos"));p.x+=o.x,p.y+=o.y;con=ivalue(con,"card"),c=getpair(ifield(con,"size"));}
			rect b=box_center(rect_pair((pair){0,0},d),c);return lmpair((pair){p.x+b.x,p.y+b.y});
		}
	}return x?x:LNIL;
}
lv* widget_read(lv*x,lv*card){
	x=ld(x);lv*r=lmd();dset(r,lmistr("card"),card);
	lv*widgets=ivalue(card,"widgets"),*type=dget(x,lmistr("type"));if(!type)type=lmistr("button");
	lv*tname=!strcmp(type->sv,"contraption")?dget(x,lmistr("def")):type;if(!tname)tname=type;
	{lv*k=lmistr("name");dset(r,k,ukey(widgets,dget(x,k),ls(tname)->sv,NULL));}
	lv*ri=(type&&!strcmp(type->sv,"field"      ))?field_read (x,r):
	      (type&&!strcmp(type->sv,"grid"       ))?grid_read  (x,r):
	      (type&&!strcmp(type->sv,"canvas"     ))?canvas_read(x,r):
	      (type&&!strcmp(type->sv,"slider"     ))?slider_read(x,r):
	      (type&&!strcmp(type->sv,"contraption"))?contraption_read(x,r): button_read(x,r);
	if(!lii(ri))return NULL;
	init_field(ri,"size"    ,x);
	init_field(ri,"script"  ,x);
	init_field(ri,"font"    ,x);
	init_field(ri,"locked"  ,x);
	init_field(ri,"animated",x);
	init_field(ri,"volatile",x);
	init_field(ri,"pos"     ,x);
	init_field(ri,"show"    ,x);
	return ri;
}
void widget_purge(lv*x); // forward ref
lv* widget_write(lv*x){
	lv*data=x->b,*r=lmd();
	{lv*k=lmistr("name"    ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("type"    ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("size"    );dset(r,k,ifield(x,"size"));}
	{lv*k=lmistr("pos"     );dset(r,k,ifield(x,"pos" ));}
	{lv*k=lmistr("locked"  ),*v=dget(data,k);if(v&&lb(v))dset(r,k,v);}
	{lv*k=lmistr("animated"),*v=dget(data,k);if(v&&lb(v))dset(r,k,v);}
	{lv*k=lmistr("volatile"),*v=dget(data,k);if(v&&lb(v))dset(r,k,v);}
	{lv*k=lmistr("script"  ),*v=dget(data,k);if(v&&v->c)dset(r,k,v);}
	{lv*k=lmistr("font"    ),*v=dget(data,k);if(v&&strcmp(button_is(x)?"menu":"body",v->sv))dset(r,k,v);}
	{lv*k=lmistr("show"    ),*v=dget(data,k);if(v&&strcmp(widget_shows[0],v->sv))dset(r,k,v);}
	return l_comma(r,button_is(x)?button_write(x): field_is (x)?field_write (x):slider_is(x)?slider_write(x):
	                 grid_is  (x)?grid_write  (x): canvas_is(x)?canvas_write(x):contraption_is(x)?contraption_write(x): lmd());
}

// Keystore interface

lv* interface_keystore(lv*self,lv*i,lv*x){
	i=ls(i);ikey("keys")return l_keys(self->b);
	if(x){
		lv*f=lmistr("%J");x=l_parse(f,l_format(f,x));
		if(linil(x)){self->b=l_drop(i,self->b);}else{dset(self->b,i,x);}return x;
	}else{return dgetv(self->b,i);}
}
lv* keystore_make(lv*x){
	lv*r=lmd();if(x){x=ld(x);EACH(z,x)if(!linil(x->lv[z]))dset(r,ls(x->kv[z]),x->lv[z]);}
	return lmi(interface_keystore,lmistr("keystore"),r);
}

// Module interface

lv* interface_module(lv*self,lv*i,lv*x){
	lv*data=self->b,*deck=ivalue(self,"deck"),*modules=ivalue(deck,"modules");
	if(x){
		ikey("description"){dset(data,i,ls(x));return x;}
		ikey("version"    ){dset(data,i,lmn(ln(x)));return x;}
		ikey("name"){
			lv*name=ivalue(self,"name");
			if(ls(x)->c==0){return x;}lv*n=ukey(modules,ls(x),ls(x)->sv,dget(data,i));
			modules->kv[dgeti(modules,name)]=n;dset(data,i,n);return x;
		}
		ikey("script"){
			dset(data,i,ls(x)),dset(data,lmistr("error"),lmistr("")),dset(data,lmistr("value"),lmd());
			lv*prog=parse(ls(x)->sv);if(perr()){dset(data,lmistr("error"),lmcstr(par.error));return x;}
			lv*root=lmenv(NULL);primitives(root,deck),constants(root),dset(root,lmistr("data"),dget(data,lmistr("data")));
			pushstate(root),issue(root,prog);int q=MODULE_QUOTA;while(running()&&q>0)runop(),q--;
			if(running()){dset(data,lmistr("error"),lmcstr("initialization took too long."));}
			else{dset(data,lmistr("value"),ld(arg()));}popstate();
		}
	}else{
		ikey("name"       )return dget(data,i);
		ikey("data"       )return dget(data,i);
		ikey("script"     ){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("value"      ){lv*r=dget(data,i);return r?r:lmd();}
		ikey("description"){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("version"    ){lv*r=dget(data,i);return r?r:ZERO;}
		ikey("error"      ){lv*r=dget(data,i);return r?r:lmistr("");}
	}return x?x:LNIL;
}
lv* module_read(lv*x,lv*deck){
	x=ld(x);lv*r=lmd(),*modules=ivalue(deck,"modules");dset(r,lmistr("deck"),deck),dset(r,lmistr("value"),lmd());
	dset(r,lmistr("data"),keystore_make(dget(x,lmistr("data"))));
	{lv*k=lmistr("name");dset(r,k,ukey(modules,dget(x,k),"module",NULL));}
	lv*ri=lmi(interface_module,lmistr("module"),r);
	init_field(ri,"description",x);
	init_field(ri,"version",x);
	init_field(ri,"script",x);
	return ri;
}
lv* module_write(lv*x){
	lv*data=x->b,*r=lmd();
	{lv*k=lmistr("name"       ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("data"       ),*v=dget(data,k);dset(r,k,v->b);}
	{lv*k=lmistr("script"     ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("description"),*v=dget(data,k);if(v&&v->c)dset(r,k,v);}
	{lv*k=lmistr("version"    ),*v=dget(data,k);if(v&&ln(v))dset(r,k,v);}
	return r;
}

// Card interface

char*nav_dirs[]={"right","left","up","down",NULL};
lv* n_card_add(lv*self,lv*z){
	lv*t=l_first(z);
	if(prototype_is(self)&&(contraption_is(t)||!strcmp("contraption",ls(t)->sv)))return LNIL;
	if(lis(t)){
		if(!strcmp("contraption",t->sv)){
			lv*defs=ivalue(ivalue(self,"deck"),"contraptions"),*ct=z->c>1?ls(z->lv[1]):lmistr(""),*def=dget(defs,ct);if(!def)return LNIL;
			lv*a=lmd();dset(a,lmistr("type"),lmistr("contraption")),dset(a,lmistr("def"),ct);
			if(z->c>2)dset(a,lmistr("name"),unpack_str(z,2));return widget_add(self,a);
		}
		if(strcmp("button",t->sv)&&strcmp("field",t->sv)&&strcmp("slider",t->sv)&&strcmp("canvas",t->sv)&&strcmp("grid",t->sv))return LNIL;
		lv*a=lmd();dset(a,lmistr("type"),t);if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));return widget_add(self,a);
	}
	if(widget_is(t)){lv*a=widget_write(t);if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));return widget_add(self,a);}
	return LNIL;
}
lv* n_card_remove(lv*self,lv*z){
	z=l_first(z);
	if(lil(z)||lid(z)){int r=1;EACH(i,z)r&=lb(n_card_remove(self,z->lv[i]));return lmbool(r);}
	if(!widget_is(z)||lin(dkey(ivalue(self,"widgets"),z)))return ZERO;
	lv*name=ifield(z,"name"),*widgets=ivalue(self,"widgets"),*target=dget(widgets,name);
	dset(self->b,lmistr("widgets"),l_drop(name,widgets));
	dset(target->b,lmistr("dead"),ONE);return ONE;
}
void contraption_update(lv*def);lv* prototype_write(lv*prototype);lv* prototype_read(lv*x,lv*deck); // forward refs
lv* n_con_add(lv*self,lv*z){lv*r=n_card_add(self,z);if(widget_is(r)&&prototype_is(self))contraption_update(self);return r;}
lv* n_con_remove(lv*self,lv*z){lv*r=n_card_remove(self,z);if(lb(r)&&prototype_is(self))contraption_update(self);return r;}
lv* con_copy_raw(lv*container,lv*z){lv*r=lml(0);EACH(i,z){lv*w=z->lv[i];if(widget_is(w)&&dget(w->b,lmistr("card"))==container)ll_add(r,widget_write(w));}return r;}
lv* con_paste_raw(lv*container,lv*payload){lv*r=lml(0);EACH(z,payload){lv*a=ld(payload->lv[z]);ll_add(r,widget_add(container,a));}return r;}
void find_fonts(lv*deck,lv*target,lv*widgets){
	lv*defs=ivalue(deck,"fonts"),*fonts=lmd();EACH(z,widgets){
		lv*wid=widgets->lv[z];if(!widget_is(wid))continue;
		lv*f=ivalue(wid,"font");if(f)dset(fonts,f,dget(defs,f)); // directly on widgets
		if(contraption_is(wid)){ // inside contraption instances
			lv*dwids=ivalue(wid,"widgets");
			EACH(z,dwids){lv*f=ifield(dwids->lv[z],"font");dset(fonts,dkey(defs,f),f);}
		}
		if(field_is(wid)&&matchr(ifield(wid,"style"),lmistr("rich"))){ // inside rtext field values
			lv*v=ifield(wid,"value"),*f=dget(v,lmistr("font"));
			EACH(z,f){lv*n=f->lv[z];if(n->c&&!dget(fonts,n)&&dget(defs,n))dset(fonts,n,dget(defs,n));}
		}
	}
	fonts=l_drop(lml3(lmistr("mono"),lmistr("menu"),lmistr("body")),fonts);
	EACH(z,fonts)fonts->lv[z]=font_write(fonts->lv[z]);if(fonts->c)dset(target,lmistr("f"),fonts);
}
void merge_fonts(lv*deck,lv*f){
	if(!f)return;f=ld(f);lv*fonts=ivalue(deck,"fonts");
	EACH(z,f){lv*k=ls(f->kv[z]),*v=font_read(ls(f->lv[z]));if(font_is(v)&&!dget(fonts,k))dset(fonts,k,v);}
}
lv* n_con_copy(lv*card,lv*z){
	z=l_first(z);z=lil(z)?z: lid(z)?ll(z): l_list(z);
	lv*wids=con_copy_raw(card,z),*defs=lmd(),*v=lmd();dset(v,lmistr("w"),wids),dset(v,lmistr("d"),defs);
	lv*deck=ivalue(card,"deck"),*condefs=ifield(deck,"contraptions");find_fonts(deck,v,z);EACH(w,wids){
		lv*wid=wids->lv[w],*type=dget(wid,lmistr("type")),*def=dget(wid,lmistr("def"));
		if(!strcmp(type->sv,"contraption")&&dget(defs,def)==NULL)dset(defs,def,prototype_write(dget(condefs,def)));
	}str r=str_new();str_addz(&r,"%%WGT0");fjson(&r,v);return lmstr(r);
}
lv* n_deck_add(lv*self,lv*z);lv* deck_read(lv*x); // forward refs
void merge_prototypes(lv*deck,lv*defs,lv*uses){
	lv*condefs=ifield(deck,"contraptions");EACH(d,defs){
		lv*def=ld(defs->lv[d]),*name=dget(def,lmistr("name")),*desc=dget(def,lmistr("description"));
		lv*ver=dget(def,lmistr("version"));float version=ver?ln(ver):0.0;
		if(!lis(name))continue;if(!desc)desc=lmistr("");
		int f=0;EACH(c,condefs){
			lv*con=condefs->lv[c];if(matchr(name,ifield(con,"name"))&&matchr(desc,ifield(con,"description"))){
				if(ln(ifield(con,"version"))<version)n_deck_add(deck,prototype_read(def,deck_read(lmistr(""))));
				f=1;break;
			}
		}if(f)continue;
		lv*p=prototype_read(def,deck),*nn=ifield(p,"name");dset(condefs,nn,p);
		EACH(w,uses){
			lv*wid=ld(uses->lv[w]),*type=dget(wid,lmistr("type")),*def=dget(wid,lmistr("def"));
			if(lis(type)&&strcmp(type->sv,"contraption")&&lis(def)&&strcmp(def->sv,name->sv))dset(wid,lmistr("def"),nn);
		}
	}
}
lv* n_con_paste(lv*card,lv*z){
	z=l_first(z);if(!lis(z)||!has_prefix(z->sv,"%%wgt0"))return LNIL;
	int f=1,i=6,n=z->c-i;lv*v=ld(pjson(z->sv,&i,&f,&n)),*defs=dget(v,lmistr("d")),*wids=dget(v,lmistr("w"));wids=wids?ll(wids):lml(0);
	lv*deck=ivalue(card,"deck");merge_fonts(deck,dget(v,lmistr("f"))),merge_prototypes(deck,defs?ld(defs):lmd(),wids);
	return con_paste_raw(card,wids);
}
lv* interface_card(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	lv*data=self->b,*deck=ivalue(self,"deck"),*cards=ivalue(deck,"cards"),*name=ivalue(self,"name");
	if(x){
		ikey("name"){
			if(ls(x)->c==0){return x;}lv*n=ukey(cards,ls(x),ls(x)->sv,dget(data,i));
			cards->kv[dgeti(cards,name)]=n;dset(data,i,n);return x;
		}
		ikey("script"){dset(data,i,ls(x));return x;}
		ikey("image" ){dset(data,i,image_is(x)?x:image_empty());return x;}
		ikey("index" ){reorder(cards,dgeti(cards,name),ln(x));dset(deck->b,lmistr("history"),l_list(ifield(self,"index")));return x;}
	}else{
		ikey("name"    )return dget(data,i);
		ikey("size"    )return dget(deck->b,i);
		ikey("index"   )return lmn(dgeti(cards,name));
		ikey("script"  ){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("image"   )return dget(data,i);
		ikey("widgets" )return dget(data,i);
		ikey("parent"  )return deck;
		ikey("add"     )return lmnat(n_card_add,self);
		ikey("remove"  )return lmnat(n_card_remove,self);
		ikey("event"   )return lmnat(n_event,self);
		ikey("copy"    )return lmnat(n_con_copy,self);
		ikey("paste"   )return lmnat(n_con_paste,self);
	}return x?x:LNIL;
}
lv* card_write(lv*card){
	lv*data=card->b, *r=lmd();
	{lv*k=lmistr("name"  ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("script"),*v=dget(data,k);if(v&&lis(v)&&v->c)dset(r,k,v);}
	{lv*k=lmistr("image" ),*v=dget(data,k);if(v&&!is_blank(v))dset(r,k,image_write(v));}
	lv*w=dget(data,lmistr("widgets")),*wids=lmd();EACH(z,w){
		lv*wid=widget_write(w->lv[z]),*n=dget(wid,lmistr("name"));
		wid=l_drop(lmistr("name"),wid);if(wid->c)dset(wids,n,wid);
	}dset(r,lmistr("widgets"),wids);return r;
}
lv* card_read(lv*x,lv*deck){
	x=ld(x);lv*r=lmd(),*widgets=lmd(),*ri=lmi(interface_card,lmistr("card"),r),*cards=ivalue(deck,"cards");
	dset(r,lmistr("deck"),deck),dset(r,lmistr("widgets"),widgets);
	{lv*k=lmistr("name"  );lv*v=dget(x,k);dset(r,k,ukey(cards,v&&lis(v)&&v->c==0?NULL:v,"card",NULL));}
	{lv*k=lmistr("image" ),*v=dget(x,k);dset(r,k,v?image_read(v):image_make(lmbuff(getpair(dget(deck->b,lmistr("size"))))));}
	init_field(ri,"script",x);lv*w=dget(x,lmistr("widgets"));w=w?ll(w):lml(0);
	EACH(z,w){lv*n=dget(w->lv[z],lmistr("name"));if(n){lv*i=widget_read(w->lv[z],ri);if(lii(i))dset(widgets,ifield(i,"name"),i);}}
	return ri;
}

// Prototype interface

lv* contraption_strip(lv*x){
	lv*r=lmd(),*widgets=dget(x->b,lmistr("widgets"));EACH(z,widgets){
		lv*w=widgets->lv[z];
		lv*p=widget_write(w);
		if(button_is(w))p=l_take(lmistr("value"),p);
		if(slider_is(w))p=l_take(lmistr("value"),p);
		if(field_is (w))p=l_take(lml2(lmistr("value"),lmistr("scroll")),p);
		if(grid_is  (w))p=l_take(lml4(lmistr("value"),lmistr("scroll"),lmistr("row"),lmistr("col")),p);
		if(canvas_is(w))p=l_take(lmistr("image"),p);
		dset(r,ifield(w,"name"),p);
	}return r;
}
void contraption_update(lv*def){
	lv*deck=dget(def->b,lmistr("deck")),*cards=ifield(deck,"cards");EACH(c,cards){
		lv*card=cards->lv[c],*widgets=ifield(card,"widgets");EACH(w,widgets){
			lv*widget=widgets->lv[w];if(!contraption_is(widget)||ifield(widget,"def")!=def)continue;
			lv*d=widget_write(widget),*n=ifield(widget,"name");
			dset(d,lmistr("widgets"),contraption_strip(widget));
			widget->b=widget_read(d,card)->b;dset(widget->b,lmistr("name"),n);
		}
	}
}
lv* n_prototype_update(lv*self,lv*z){(void)z;contraption_update(self);return LNIL;}
char*attribute_types[]={"","bool","number","string","code","rich",NULL};
enum attribute_type{attr_nil,attr_bool,attr_number,attr_string,attr_code,attr_rich};
lv* normalize_attributes(lv*x){
	lv*r=lmt(),*n=lml(0),*l=lml(0),*t=lml(0),*nk=lmistr("name"),*lk=lmistr("label"),*tk=lmistr("type");dset(r,nk,n),dset(r,lk,l),dset(r,tk,t);
	if(lit(x)){
		lv*sn=dget(x,nk),*sl=dget(x,lk),*st=dget(x,tk);if(!sl)sl=sn;if(sn&&st)EACH(z,sn){
			if(!lis(sn->lv[z])||!sn->lv[z]->c)continue;
			lv*type=normalize_enum(st->lv[z],attribute_types);
			if(type->c)ll_add(n,sn->lv[z]),ll_add(l,ls(sl->lv[z])),ll_add(t,type);
		}
	}return torect(r),r;
}
lv* prototype_pos(lv*self){
	lv*data=self->b,*deck=dget(data,lmistr("deck"));
	pair cs=getpair(dget(deck->b,lmistr("size"))),ps=getpair(dget(data,lmistr("size")));
	rect c=box_center(rect_pair((pair){0,0},cs),ps);return lmpair((pair){c.x,c.y});
}
lv* interface_prototype(lv*self,lv*i,lv*x){
	if(!is_rooted(self))return LNIL;
	lv*data=self->b,*deck=dget(data,lmistr("deck")),*defs=ivalue(deck,"contraptions");
	if(x){
		ikey("name"){
			lv*o=dget(data,i),*n=ukey(defs,ls(x),ls(x)->sv,o);
			defs->kv[dgeti(defs,o)]=n;dset(data,i,n);return x;
		}
		ikey("description"){dset(data,i,ls(x));return x;}
		ikey("version"    ){dset(data,i,lmn(ln(x)));return x;}
		ikey("size"       ){dset(data,i,normalize_pair  (x                             )),contraption_update(self);return x;}
		ikey("margin"     ){dset(data,i,normalize_margin(x,getpair(ifield(self,"size")))),contraption_update(self);return x;}
		ikey("resizable"  ){dset(data,i,lmn(lb(x)))                                      ,contraption_update(self);return x;}
		ikey("image"      ){dset(data,i,image_is(x)?x:image_empty());return x;}
		ikey("script"     ){dset(data,i,ls(x));return x;}
		ikey("template"   ){dset(data,i,ls(x));return x;}
		ikey("attributes" ){dset(data,i,normalize_attributes(x));return x;}
	}else{
		ikey("name"       )return dget(data,i);
		ikey("description"){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("version"    ){lv*r=dget(data,i);return r?r:ZERO;      }
		ikey("script"     ){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("template"   ){lv*r=dget(data,i);return r?r:lmistr("");}
		ikey("font"       )return l_first(ifield(ivalue(self,"deck"),"fonts"));
		ikey("show"       )return lmistr("solid");
		ikey("parent"     )return ifield(deck,"card");
		ikey("size"       )return dget(data,i);
		ikey("margin"     )return dget(data,i);
		ikey("resizable"  )return dget(data,i);
		ikey("image"      )return dget(data,i);
		ikey("widgets"    )return dget(data,i);
		ikey("attributes" ){lv*r=dget(data,i);return r?r:normalize_attributes(LNIL);}
		ikey("offset"     )return prototype_pos(self); // (for compatibility with cards during editing)
		ikey("pos"        )return prototype_pos(self); // (for compatibility with cards during editing)
		ikey("add"        )return lmnat(n_con_add,self);
		ikey("remove"     )return lmnat(n_con_remove,self);
		ikey("update"     )return lmnat(n_prototype_update,self);
	}return x?x:LNIL;
}
lv* prototype_write(lv*prototype){
	lv*data=prototype->b, *r=lmd();
	{lv*k=lmistr("name"       ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("size"       ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("resizable"  ),*v=dget(data,k);if(lb(v))dset(r,k,v);}
	{lv*k=lmistr("margin"     ),*v=dget(data,k);dset(r,k,v);}
	{lv*k=lmistr("description"),*v=dget(data,k);if(v&&lis(v)&&v->c)dset(r,k,v);}
	{lv*k=lmistr("version"    ),*v=dget(data,k);if(v&&lin(v)&&ln(v))dset(r,k,v);}
	{lv*k=lmistr("script"     ),*v=dget(data,k);if(v&&lis(v)&&v->c)dset(r,k,v);}
	{lv*k=lmistr("template"   ),*v=dget(data,k);if(v&&lis(v)&&v->c)dset(r,k,v);}
	{lv*k=lmistr("image"      ),*v=dget(data,k);if(v&&!is_blank(v))dset(r,k,image_write(v));}
	{lv*k=lmistr("attributes" ),*v=dget(data,k);if(v)dset(r,k,l_cols(v));}
	lv*w=dget(data,lmistr("widgets")),*wids=lmd();EACH(z,w){
		lv*wid=widget_write(w->lv[z]),*n=dget(wid,lmistr("name"));
		wid=l_drop(lmistr("name"),wid);if(wid->c)dset(wids,n,wid);
	}dset(r,lmistr("widgets"),wids);return r;
}
lv* prototype_read(lv*x,lv*deck){
	x=ld(x);lv*r=lmd(),*widgets=lmd(),*ri=lmi(interface_prototype,lmistr("prototype"),r),*defs=ivalue(deck,"contraptions");
	dset(r,lmistr("deck"),deck),dset(r,lmistr("widgets"),widgets);
	{lv*k=lmistr("name"       ),*v=dget(x,k);dset(r,k,ukey(defs,v&&lis(v)&&v->c==0?NULL:v,"prototype",NULL));}
	{lv*k=lmistr("attributes" ),*v=dget(x,k);if(v)iwrite(ri,k,l_table(v));}
	{lv*k=lmistr("size"       ),*v=dget(x,k);dset(r,k,v?normalize_pair(v):lmpair((pair){100,100}));}
	{lv*k=lmistr("image"      ),*v=dget(x,k);dset(r,k,v?image_read(v):image_make(lmbuff(getpair(ifield(ri,"size")))));}
	{lv*k=lmistr("resizable"  ),*v=dget(x,k);dset(r,k,v?lmn(lb(v)):ZERO);}
	lv*w=dget(x,lmistr("widgets"));if(lid(w)){EACH(z,w)dset(w->lv[z],lmistr("name"),ls(w->kv[z]));}w=w?ll(w):lml(0);
	EACH(z,w){lv*n=dget(w->lv[z],lmistr("name"));if(n){lv*i=widget_read(w->lv[z],ri);if(lii(i))dset(widgets,ifield(i,"name"),i);}}
	init_field(ri,"description",x)
	init_field(ri,"version"    ,x)
	init_field(ri,"script"     ,x)
	init_field(ri,"template"   ,x)
	{lv*k=lmistr("margin"),*v=dget(x,k);dset(r,k,normalize_margin(v?v:LNIL,getpair(ifield(ri,"size"))));}
	return ri;
}

// Deck interface

void rename_sound(lv*deck,lv*sound,lv*name){
	lv*sounds=dget(deck->b,lmistr("sounds")),*oldname=dkey(sounds,sound);
	sounds->kv[dgeti(sounds,oldname)]=ukey(sounds,ls(name),ls(name)->sv,oldname);
}
lv* n_deck_copy(lv*deck,lv*z){
	(void)deck;z=l_first(z);if(!card_is(z))return LNIL;
	lv*defs=lmd(),*v=lmd();dset(v,lmistr("c"),card_write(z)),dset(v,lmistr("d"),defs);
	lv*wids=ifield(z,"widgets");find_fonts(deck,v,wids);EACH(w,wids){
		lv*wid=wids->lv[w];if(!contraption_is(wid))continue;
		lv*d=ifield(wid,"def"),*n=ifield(d,"name");if(dget(defs,n)==NULL)dset(defs,n,prototype_write(d));
	}str r=str_new();str_addz(&r,"%%CRD0");fjson(&r,v);return lmstr(r);
}
lv* deck_paste_named(lv*deck,lv*z,lv*name){
	z=l_first(z);if(!lis(z)||!has_prefix(z->sv,"%%crd0"))return LNIL;
	int f=1,i=6,n=z->c-i;lv*v=ld(pjson(z->sv,&i,&f,&n)),*payload=dget(v,lmistr("c")),*defs=dget(v,lmistr("d"));payload=payload?ld(payload):lmd();
	merge_fonts(deck,dget(v,lmistr("f")));
	lv*wids=dget(payload,lmistr("widgets"));if(wids&&lid(wids)){EACH(z,wids)dset(wids->lv[z],lmistr("name"),wids->kv[z]);}
	merge_prototypes(deck,defs?ld(defs):lmd(),wids?ll(wids):lml(0));
	lv*r=card_read(payload,deck);dset(dget(deck->b,lmistr("cards")),name?name:ifield(r,"name"),r);return r;
}
lv* n_deck_paste(lv*deck,lv*z){return deck_paste_named(deck,z,NULL);}
lv* n_deck_remove(lv*self,lv*z); // forward ref
lv* n_deck_add(lv*self,lv*z){
	lv*sounds=ivalue(self,"sounds"),*fonts=ivalue(self,"fonts"),*modules=ivalue(self,"modules"),*cards=ivalue(self,"cards"),*t=l_first(z);
	lv*defs=ivalue(self,"contraptions");
	if(card_is(t)){return deck_paste_named(self,l_list(n_deck_copy(self,t)),z->c>1?ls(z->lv[1]):NULL);}
	if(font_is(t)){lv*r=font_make((pair){font_w(t),font_h(t)});memcpy(r->b->sv,t->b->sv,r->b->c);return uset(fonts,unpack_str(z,1),"font",r);}
	if(sound_is(t)){lv*b=lms(t->b->c);memcpy(b->sv,t->b->sv,b->c);return uset(sounds,unpack_name(z,1),"sound",sound_make(b));}
	if(module_is(t)){
		if(z->c<2&&dget(modules,ifield(t,"name"))){ // replace
			n_deck_remove(self,dget(modules,ifield(t,"name")));modules=ivalue(self,"modules");
			lv*r=module_read(module_write(t),self);dset(modules,ifield(r,"name"),r);return r;
		}else{ // insert
			lv*a=module_write(t);if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));
			lv*r=module_read(a,self);dset(modules,ifield(r,"name"),r);return r;
		}
	}
	if(prototype_is(t)){
		if(z->c<2&&dget(defs,ifield(t,"name"))){ // replace
			lv*name=ifield(t,"name"),*r=dget(defs,name);
			r->b=prototype_read(prototype_write(t),self)->b;dset(r->b,lmistr("name"),name);
			contraption_update(r);return r;
		}else{ // insert
			lv*a=prototype_write(t);if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));
			lv*r=prototype_read(a,self);dset(defs,ifield(r,"name"),r);return r;
		}
	}
	if(lis(t)&&!strcmp("sound" ,t->sv))return uset(sounds,unpack_name(z,2),"sound",sound_make(lms(MAX(0,z->c>1?ln(z->lv[1]):0))));
	if(lis(t)&&!strcmp("font"  ,t->sv))return uset(fonts,unpack_name(z,2),"font",font_make(unpack_pair(z,1)));
	if(lis(t)&&!strcmp("module",t->sv)){
		lv*a=lmd();if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));
		lv*r=module_read(a,self);dset(modules,ifield(r,"name"),r);return r;
	}
	if(lis(t)&&!strcmp("contraption",t->sv)){
		lv*a=lmd();if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));
		lv*r=prototype_read(a,self);dset(defs,ifield(r,"name"),r);return r;
	}
	if(lis(t)&&!strcmp("card",t->sv)){
		lv*a=lmd();if(z->c>1)dset(a,lmistr("name"),unpack_str(z,1));
		lv*r=card_read(a,self);dset(cards,ifield(r,"name"),r);return r;
	}return LNIL;
}
void remove_font(lv*w,lv*t){
	EACH(z,w){
		lv*widget=w->lv[z];
		if(ifield(widget,"font")==t)dset(widget->b,lmistr("font"),lmistr("body"));
		if(contraption_is(w))remove_font(ivalue(w,"widgets"),t);
	}
}
lv* n_deck_remove(lv*self,lv*z){
	lv*data=self->b,*t=l_first(z),*cards=ivalue(self,"cards"),*defs=ivalue(self,"contraptions");
	if(patterns_is(t)){t->b=patterns_read(lmd())->b;return ONE;}
	if(widget_is(t)&&is_rooted(t))return n_card_remove(ivalue(t,"card"),t);
	if(module_is(t)){
		lv*k=lmistr("modules"),*m=dget(data,k);
		lv*n=dkey(m,t);if(lin(n))return ZERO; // this module isn't part of this deck
		dset(data,k,l_drop(n,m));return ONE;
	}
	if(sound_is(t)){
		lv*k=lmistr("sounds"),*s=dget(data,k);
		lv*n=dkey(s,t);if(lin(n))return ZERO; // this sound isn't part of this deck
		dset(data,k,l_drop(n,s));return ONE;
	}
	if(font_is(t)){
		lv*k=lmistr("fonts"),*fonts=dget(data,k);
		lv*n=dkey(fonts,t);if(lin(n))return ZERO; // this font isn't part of this deck
		if(!strcmp("body",n->sv)||!strcmp("menu",n->sv)||!strcmp("mono",n->sv))return ZERO; // cannot delete builtin fonts
		EACH(c,cards)remove_font(ifield(cards->lv[c],"widgets"),t);
		EACH(c,defs )remove_font(ifield(defs ->lv[c],"widgets"),t);
		dset(data,k,l_drop(n,fonts));return ONE;
	}
	if(prototype_is(t)){
		lv*k=lmistr("contraptions");
		lv*n=dkey(defs,t);if(lin(n))return ZERO; // this contraption isn't part of the deck
		lv*cards=ifield(self,"cards");EACH(c,cards){ // scrub instances from every card:
			lv*card=cards->lv[c],*widgets=ifield(card,"widgets");EACH(w,widgets){
				lv*widget=widgets->lv[w];if(contraption_is(widget)&&ifield(widget,"def")==t)n_card_remove(card,widget);
			}
		}dset(data,k,l_drop(n,defs));dset(t->b,lmistr("dead"),ONE);return ONE;
	}
	if(card_is(t)){
		if(cards->c<=1)return ZERO; // cannot delete the last card from a deck
		dset(data,lmistr("cards"),cards=l_drop(dkey(cards,t),cards));dset(t->b,lmistr("dead"),ONE);
		int n=ln(dget(data,lmistr("card")));if(n>=cards->c)dset(data,lmistr("card"),lmn(cards->c-1));
		dset(data,lmistr("history"),l_list(ifield(ifield(self,"card"),"index")));
		return ONE;
	}return ZERO;
}
void widget_purge(lv*x){
	if(!lb(ifield(x,"volatile")))return;
	if(button_is(x))iwrite(x,lmistr("value"),ZERO);
	if(slider_is(x))iwrite(x,lmistr("value"),ZERO);
	if(field_is (x))iwrite(x,lmistr("value"),lmistr("")),iwrite(x,lmistr("scroll"),ZERO);
	if(grid_is  (x))iwrite(x,lmistr("value"),lmt()     ),iwrite(x,lmistr("scroll"),ZERO);
	if(canvas_is(x)){cstate f=frame;n_canvas_clear(x,lml(0));frame=f;}
	if(contraption_is(x)){lv*widgets=dget(x->b,lmistr("widgets"));EACH(z,widgets)widget_purge(widgets->lv[z]);}
}
lv* n_deck_purge(lv*self,lv*z){
	lv*cards=ivalue(self,"cards");
	EACH(c,cards){lv*wids=ivalue(cards->lv[c],"widgets");EACH(w,wids)widget_purge(wids->lv[w]);}
	(void)z;return self;
}
lv* deck_write(lv*x,int html); // forward ref
lv* interface_deck(lv*self,lv*i,lv*x){
	lv*data=self->b,*cards=ivalue(self,"cards");
	if(x){
		ikey("locked"){dset(data,i,lmn(lb(x)));return x;}
		ikey("name"  ){dset(data,i,ls(x));return x;}
		ikey("author"){dset(data,i,ls(x));return x;}
		ikey("script"){dset(data,i,ls(x));return x;}
		ikey("card"  ){n_go(self,l_list(x));return x;}
	}else{
		ikey("version" )return dget(data,i);
		ikey("locked"  )return dget(data,i);
		ikey("name"    )return dget(data,i);
		ikey("author"  )return dget(data,i);
		ikey("script"  )return dget(data,i);
		ikey("patterns")return dget(data,i);
		ikey("sounds"  )return l_drop(ZERO,dget(data,i)); // expose a shallow copy
		ikey("fonts"   )return l_drop(ZERO,dget(data,i)); // expose a shallow copy
		ikey("cards"   )return dget(data,i);
		ikey("modules" )return dget(data,i);
		ikey("contraptions")return dget(data,i);
		ikey("add"     )return lmnat(n_deck_add,self);
		ikey("remove"  )return lmnat(n_deck_remove,self);
		ikey("event"   )return lmnat(n_event,self);
		ikey("card"    ){int n=ln(dget(data,lmistr("card")));return cards->lv[MIN(cards->c-1,n)];}
		ikey("copy"    )return lmnat(n_deck_copy,self);
		ikey("paste"   )return lmnat(n_deck_paste,self);
		ikey("purge"   )return lmnat(n_deck_purge,self);
		ikey("encoded" )return deck_write(self,0);
	}return x?x:LNIL;
}
lv* deck_read(lv*x){
	x=ls(x);lv*deck=lmd(),*fonts=lmd(),*sounds=lmd(),*scripts=lmd(),*cards=lmd(),*modules=lmd(),*defs=lmd();int i=0,m=0,md=0,lc=0;
	dset(fonts,lmistr("body"),font_read(lmistr(FONT_BLOCK_BODY)));
	dset(fonts,lmistr("menu"),font_read(lmistr(FONT_BLOCK_MENU)));
	dset(fonts,lmistr("mono"),font_read(lmistr(FONT_BLOCK_MONO)));
	#define dmatch(k) (has_prefix(x->sv+i,k)?(i+=strlen(k),1):0)
	#define dend      (!x->sv[i]||has_prefix(x->sv+i,"</script>"))
	#define descs(n)  {str_addc(&n,dmatch("{l}")?'{': dmatch("{r}")?'}': dmatch("{c}")?':': dmatch("{s}")?'/': x->sv[i++]);}
	#define did(n,e)  str _##n=str_new();while(!dend&&!dmatch(e))descs(_##n);lv*n=lmstr(_##n);
	dmatch("<meta charset=\"utf-8\">");
	dmatch("<body><script language=\"decker\">");while(!dend){
		if(x->sv[i]=='\n'){i++;}
		else if(x->sv[i]=='#'){while(!dend&&x->sv[i]!='\n')i++;}
		else if(dmatch("{deck}\n"   ))m=1;
		else if(dmatch("{fonts}\n"  ))m=2;
		else if(dmatch("{sounds}\n" ))m=3;
		else if(dmatch("{widgets}\n"))m=4;
		else if(dmatch("{card:")){
			did(k,"}");lv*v=lmd();
			dset(v,lmistr("name"),k),dset(v,lmistr("widgets"),lml(0));
			dset(cards,k,v);m=5,lc=0;
		}
		else if(dmatch("{script:")){did(k,"}\n");did(v,"\n{end}");dset(scripts,ls(k),v);}
		else if(m==6&&dmatch("{data}\n")){md=1;}
		else if(m==6&&dmatch("{script}\n")){did(v,"\n{end}");dset(l_last(modules),lmistr("script"),v);m=1;}
		else if(dmatch("{module:")){
			did(k,"}");lv*v=lmd();dset(v,lmistr("name"),k),dset(v,lmistr("script"),lmistr("")),dset(v,lmistr("data"),lmd());dset(modules,k,v);m=6,md=0;}
		else if(dmatch("{contraption:")){did(k,"}");lv*v=lmd();dset(v,lmistr("name"),k),dset(v,lmistr("widgets"),lml(0));dset(defs,k,v);m=7,lc=1;}
		else{
			did(k,":");int f=1,n=x->c-i;lv*v=plove(x->sv,&i,&f,&n);
			if(m==1)dset(deck,k,v);
			if(m==2){v=font_read (v);if(v)dset(fonts ,k,v);}
			if(m==3){v=sound_read(v);if(v)dset(sounds,k,v);}
			if(m==4&&!lc){v=ld(v);dset(v,lmistr("name"),k);if(cards->c)ll_add(dget(l_last(cards),lmistr("widgets")),v);}
			if(m==4&& lc){v=ld(v);dset(v,lmistr("name"),k);if(defs ->c)ll_add(dget(l_last(defs ),lmistr("widgets")),v);}
			if(m==5)dset(l_last(cards),k,v);
			if(m==6)dset(md?dget(l_last(modules),lmistr("data")):l_last(modules),k,v);
			if(m==7)dset(l_last(defs),k,v);
		}
	}
	#define dscript(x) {if(x){lv*i=dget(x,lmistr("script"));if(i){lv*v=dget(scripts,ls(i));if(v)dset(x,lmistr("script"),v);}}}
	dscript(deck);EACH(z,cards){dscript(cards->lv[z]);lv*wids=dget(cards->lv[z],lmistr("widgets"));EACH(w,wids)dscript(wids->lv[w]);}
	EACH(z,defs)               {dscript(defs ->lv[z]);lv*wids=dget(defs ->lv[z],lmistr("widgets"));EACH(w,wids)dscript(wids->lv[w]);}
	// assemble the interface:
	lv*r=lmd(),*ri=lmi(interface_deck,lmistr("deck"),r);
	if(cards->c==0){lv*a=lmd(),*n=lmistr("home");dset(a,lmistr("name"),n);dset(cards,n,a);}
	dset(r,lmistr("fonts"  ),fonts);
	dset(r,lmistr("sounds" ),sounds);
	{lv*k=lmistr("patterns");dset(r,k,patterns_read(deck));}
	{lv*k=lmistr("version" ),*f=dget(deck,k);dset(r,k,f?f:lmn(1));}
	{lv*k=lmistr("locked"  ),*f=dget(deck,k);dset(r,k,bool_read(f,0));}
	{lv*k=lmistr("size"    ),*f=dget(deck,k);dset(r,k,f?lmpair(pair_max(getpair(normalize_pair(f)),(pair){8,8})):lmpair((pair){512,342}));}
	{lv*k=lmistr("name"    ),*f=dget(deck,k);dset(r,k,str_read(f,""));}
	{lv*k=lmistr("author"  ),*f=dget(deck,k);dset(r,k,str_read(f,""));}
	{lv*k=lmistr("script"  ),*f=dget(deck,k);dset(r,k,str_read(f,""));}
	{lv*k=lmistr("card"    ),*f=dget(deck,k);int n=f?ln(f):0;dset(r,k,lmn(CLAMP(0,n,cards->c-1)));}
	dset(r,lmistr("brushes"),lmd()),dset(r,lmistr("brusht"),lmd());
	lv*trans=lmd();dset(r,lmistr("transit"),trans);lv*root=lmenv(NULL);constants(root);dset(root,lmistr("transition"),lmnat(n_transition,ri));
	pushstate(root),issue(root,parse(default_transitions));while(running())runop();arg();popstate();
	MAP(ddata,defs )defs ->lv[z];
	MAP(cdata,cards)cards->lv[z];
	MAP(mdata,modules)modules->lv[z];
	{lv*k=lmistr("contraptions");defs   =lmd();dset(r,k,defs   );EACH(z,ddata){lv*v=prototype_read(ddata->lv[z],ri);dset(defs   ,ifield(v,"name"),v);}}
	{lv*k=lmistr("cards"       );cards  =lmd();dset(r,k,cards  );EACH(z,cdata){lv*v=card_read     (cdata->lv[z],ri);dset(cards  ,ifield(v,"name"),v);}}
	dset(r,lmistr("history"),l_list(ifield(ifield(ri,"card"),"index")));
	{lv*k=lmistr("modules"     );modules=lmd();dset(r,k,modules);EACH(z,mdata){lv*v=module_read   (mdata->lv[z],ri);dset(modules,ifield(v,"name"),v);}}
	return ri;
}
void esc_write(str*r,int id,lv*x){
	char c='\0',lc=c;for(int z=0;x->sv[z];z++){
		lc=c,c=x->sv[z];
		if     (c=='{'         ){str_addz(r,"{l}");}
		else if(c=='}'         ){str_addz(r,"{r}");}
		else if(c==':'&&id     ){str_addz(r,"{c}");}
		else if(c=='/'&&lc=='<'){str_addz(r,"{s}");}
		else                    {str_addc(r,c    );}
	}
}

lv* script_ref(lv*scripts,lv*base,int*index,lv*x,char suff){
	EACH(z,scripts)if(matchr(scripts->lv[z],x))return scripts->kv[z];
	str k=str_new();if(base)str_addz(&k,base->sv),str_addc(&k,'.');
	char t[4096];snprintf(t,sizeof(t),"%d",*index);str_addz(&k,t);if(suff)str_addc(&k,suff);lv*key=lmstr(k);
	dset(scripts,key,x),(*index)++;return key;
}
void scripts_write(lv*scripts,str*r,int*index){
	while((*index)<scripts->c){
		str_addz(r,"\n{script:"),esc_write(r,1,scripts->kv[*index]),str_addz(r,"}\n");
		esc_write(r,0,scripts->lv[*index]);
		str_addz(r,"\n{end}\n"),(*index)++;
	}
}
lv* deck_write(lv*x,int html){
	#define write_dict(k,x,f) if(x->c)str_addz(&r,k);EACH(z,x)esc_write(&r,1,x->kv[z]),str_addc(&r,':'),flove(&r,f(x->lv[z])),str_addc(&r,'\n');
	#define write_line(k,p,f) {lv*v=dget(data,lmistr(k));(void)v;if(p)str_addz(&r,k),str_addc(&r,':'),fjson(&r,f),str_addc(&r,'\n');}
	if(!deck_is(x))return lmistr("");lv*data=x->b;str r=str_new();lv*scripts=lmd();int si=0,sci=0;
	if(html)str_addz(&r,"<meta charset=\"UTF-8\"><body><script language=\"decker\">\n");
	str_addz(&r,"{deck}\nversion:1\n");
	char*pal=patterns_pal(dget(data,lmistr("patterns")));lv*pa=anims_write(pal),*da=l_parse(lmistr("%j"),lmistr(DEFAULT_ANIMS));
	lv*f=dget(data,lmistr("fonts"));
	{lv*k=lmistr("body"),*tf=font_write(dget(f,k));if(!strcmp(tf->sv,FONT_BLOCK_BODY))f=l_drop(k,f);}
	{lv*k=lmistr("menu"),*tf=font_write(dget(f,k));if(!strcmp(tf->sv,FONT_BLOCK_MENU))f=l_drop(k,f);}
	{lv*k=lmistr("mono"),*tf=font_write(dget(f,k));if(!strcmp(tf->sv,FONT_BLOCK_MONO))f=l_drop(k,f);}
	lv*s=dget(data,lmistr("sounds"));
	write_line("card"      ,1                                             ,v                                )
	write_line("size"      ,1                                             ,v                                )
	write_line("locked"    ,lb(v)                                         ,v                                )
	write_line("script"    ,v->c                                          ,script_ref(scripts,NULL,&sci,v,0))
	write_line("name"      ,v->c                                          ,v                                )
	write_line("author"    ,v->c                                          ,v                                )
	write_line("patterns"  ,strcmp(patterns_write(v)->sv,DEFAULT_PATTERNS),patterns_write(v)                )
	write_line("animations",!matchr(pa,da)                                ,pa                               )
	scripts_write(scripts,&r,&si);
	write_dict("\n{fonts}\n" ,f,font_write )
	write_dict("\n{sounds}\n",s,sound_write)
	lv*c=dget(data,lmistr("cards"));EACH(z,c){
		lv*card=c->lv[z],*data=card_write(card),*wids=dget(data,lmistr("widgets"));lv*base=dget(data,lmistr("name"));sci=0;
		str_addz(&r,"\n{card:"),esc_write(&r,1,base);str_addz(&r,"}\n");
		write_line("image" ,v,v                              )
		write_line("script",v,script_ref(scripts,base,&sci,v,0))
		EACH(w,wids){lv*k=lmistr("script"),*v=dget(wids->lv[w],k);if(v)dset(wids->lv[w],k,script_ref(scripts,base,&sci,v,0));}
		write_dict("{widgets}\n",wids,)
		scripts_write(scripts,&r,&si);
	}
	lv*m=dget(data,lmistr("modules"));EACH(z,m){
		lv*module=m->lv[z],*data=module_write(module);
		str_addz(&r,"\n{module:"),esc_write(&r,1,dget(data,lmistr("name"))),str_addz(&r,"}\n");
		write_line("description",v,v)
		write_line("version"    ,v,v)
		lv*mdata=dget(data,lmistr("data"));write_dict("{data}\n",mdata,)
		str_addz(&r,"{script}\n"),esc_write(&r,0,ifield(module,"script")),str_addz(&r,"\n{end}\n");
	}
	lv*d=dget(data,lmistr("contraptions"));EACH(z,d){
		lv*def=d->lv[z],*data=prototype_write(def),*wids=dget(data,lmistr("widgets"));lv*base=dget(data,lmistr("name"));sci=0;
		str_addz(&r,"\n{contraption:"),esc_write(&r,1,base),str_addz(&r,"}\n");
		write_line("size"       ,1       ,v                                  )
		write_line("resizable"  ,v&&lb(v),v                                  )
		write_line("margin"     ,1       ,v                                  )
		write_line("description",v       ,v                                  )
		write_line("version"    ,v       ,v                                  )
		write_line("image"      ,v       ,v                                  )
		write_line("script"     ,v&&v->c ,script_ref(scripts,base,&sci,v,'p'))
		write_line("template"   ,v&&v->c ,v                                  )
		write_line("attributes" ,v&&v->c ,v                                  )
		EACH(w,wids){lv*k=lmistr("script"),*v=dget(wids->lv[w],k);if(v)dset(wids->lv[w],k,script_ref(scripts,base,&sci,v,'p'));}
		write_dict("{widgets}\n",wids,)
		scripts_write(scripts,&r,&si);
	}
	str_addz(&r,"\n");
	if(html){
		str_addz(&r,"</script>\n");
		str_addz(&r,lmiutf8(js_decker_html,js_decker_html_len));
		str_addz(&r,"<script>\n");
		str_addz(&r,"VERSION=\""),str_addz(&r,VERSION),str_addz(&r,"\"\n");
		#ifdef DANGER_ZONE
			char*df="DANGEROUS=1\n";
		#else
			char*df="DANGEROUS=0\n";
		#endif
		str_addz(&r,df);
		str_addz(&r,lmiutf8(js_lil_js,js_lil_js_len));
		str_addz(&r,lmiutf8(js_danger_js,js_danger_js_len));
		str_addz(&r,lmiutf8(js_decker_js,js_decker_js_len));
		str_addz(&r,"</script></body>\n");
	}
	return lmstr(r);
}
lv* free_canvas(lv*deck){ // make a drawing surface that isn't attached to the parent deck, but is aware of its resources:
	lv*d=deck_read(lmistr("{deck}\n{card:home}\n{widgets}\nc:{\"type\":\"canvas\"}"));
	lv*c=l_first(dget(d->b,lmistr("cards")));
	lv*r=l_first(dget(c->b,lmistr("widgets")));
	{lv*k=lmistr("size");    iwrite(r,k,dget(deck->b,k));}
	{lv*k=lmistr("fonts");   dset(d->b,k,dget(deck->b,k));}
	{lv*k=lmistr("patterns");dset(d->b,k,dget(deck->b,k));}
	dset(r->b,lmistr("free"),ONE);
	return r;
}

// Image IO

int readshort(int*i,char*d){int a=0xFF&d[*i],b=0xFF&d[1+*i];(*i)+=2;return (b<<8)|a;}
int readcolor(unsigned char cr,unsigned char cg,unsigned char cb,int grayscale){
	if(grayscale){
		// perceptually weighted gray: http://entropymine.com/imageworsener/grayscale/
		double rf=0.2126*pow(cr,2.2), gf=0.7152*pow(cg,2.2), bf=0.0722*pow(cb,2.2), gg=pow(rf+gf+bf,1/2.2);
		return CLAMP(0,(int)gg,255);
	}
	int ci=0;float cd=1e20;for(int c=0;c<16;c++){
		float dr=fabs(((COLORS[c]>>16)&0xFF)/256.0-cr/256.0),
			  dg=fabs(((COLORS[c]>> 8)&0xFF)/256.0-cg/256.0),
			  db=fabs(((COLORS[c]    )&0xFF)/256.0-cb/256.0),
			  diff=(dr*dr)+(dg*dg)+(db*db);
		if(diff<cd)ci=c,cd=diff;
	}if(ci==15)return 1;return ci+32;
}
void readcolors(int*i,char*d,int*pal,int packed,int grayscale){
	int c=1<<((packed&0x07)+1);for(int z=0;z<c;z++)pal[z]=readcolor(d[*i],d[1+(*i)],d[2+(*i)],grayscale),(*i)+=3;
}
lv* empty_frames(void){lv*r=lmd();dset(r,lmistr("frames"),lml(0)),dset(r,lmistr("delays"),lml(0));return r;}
lv* readgif(char*data,int size,int gray,int frames){
	if(memcmp(data,"GIF89a",6)&&memcmp(data,"GIF87a",6))return free(data),(frames?empty_frames():image_empty());
	lv*r_frames=lml(0),*r_delays=lml(0),*r_disposal=lml(0),*r_dict=lmd();dset(r_dict,lmistr("frames"),r_frames),dset(r_dict,lmistr("delays"),r_delays);
	int i=6, w=readshort(&i,data),h=readshort(&i,data),gpal[256]={0},lpal[256]={0};int packed=data[i++],back=data[i++],hastrans=0,trans=255,delay=0,dispose=0;i++;
	if(packed&0x80)readcolors(&i,data,gpal,packed,gray);
	lv*r=lmbuff((pair){w,h});
	while(i<size){
		unsigned char type=data[i++];
		if(type==0x3B)break; // end
		if(type==0x21){ // text, gce, comment, app...?
			if((0xFF&data[i++])==0xF9){
				i++;int packed=0xFF&data[i++];delay=readshort(&i,data);
				int tindex=0xFF&data[i++];i++;dispose=(packed>>2)&7;
				if(packed&1){hastrans=1,trans=tindex;}else{hastrans=0;}
			}else{while(1){unsigned char s=data[i++];if(!s)break;i+=s;}}
		}
		if(type==0x2C){ // image descriptor
			int xo=readshort(&i,data),yo=readshort(&i,data),iw=readshort(&i,data),ih=readshort(&i,data),packed=data[i++],local=packed&0x80;
			if(local){readcolors(&i,data,lpal,packed,gray);}else{memcpy(lpal,gpal,sizeof(lpal));}if(hastrans)lpal[trans]=gray?255:0;
			int min_code=0xFF&data[i++], si=0, di=0; unsigned char*src=calloc(iw*ih*2,1),*dst=calloc(iw*ih,1);
			while(1){unsigned char s=data[i++];if(!s)break;for(int z=0;z<s;z++)src[si++]=data[i++];}
			int prefix[4096]={0}, suffix[4096]={0}, code[4096]={0};
			int clear=1<<min_code, size=min_code+1, mask=(1<<size)-1, next=clear+2, old=-1;
			int first, i=0,b=0,d=0;
			for(int z=0;z<clear;z++)suffix[z]=z;
			while(i<si){
				while(b<size)d+=(0xFF&src[i++])<<b, b+=8;
				int t=d&mask; d>>=size, b-=size;
				if(t>next||t==clear+1)break;
				if(t==clear){size=min_code+1, mask=(1<<size)-1, next=clear+2, old=-1;}
				else if (old==-1) dst[di++]=suffix[old=first=t];
				else{
					int ci=0,tt=t;
					if   (t==next)code[ci++]=first,    t=old;
					while(t>clear)code[ci++]=suffix[t],t=prefix[t];
					dst[di++]=first=suffix[t];
					while(ci>0)dst[di++]=code[--ci];
					if(next<4096){prefix[next]=old, suffix[next++]=first;if((next&mask)==0&&next<4096)size++, mask+=next;}
					old=tt;
				}
			}
			for(int y=0;y<ih;y++)for(int x=0;x<iw;x++)if(xo+x>=0&&yo+y>=0&&xo+x<w&&yo+y<h&&(!hastrans||dst[x+y*iw]!=trans))r->sv[(xo+x)+(yo+y)*w]=lpal[0xFF&(int)dst[x+y*iw]];
			free(src),free(dst);ll_add(r_frames,image_make(buffer_clone(r))),ll_add(r_delays,lmn(delay)),ll_add(r_disposal,lmn(dispose));if(!frames)break;
			if(dispose==2){memset(r->sv,hastrans?0:lpal[back],r->c);} // dispose to background
			if(dispose==3){int i=r_frames->c-2;while(i&&ln(r_disposal->lv[i])>=2)i--;memcpy(r->sv,r_frames->lv[i]->b->sv,r->c);}// dispose to previous
		}
	}return free(data),(frames?r_dict: r_frames->c?r_frames->lv[0]: image_empty());
}
#define add_byte(x)  str_addraw(&r,(x)&0xFF)
#define add_short(x) str_addraw(&r,(x)&0xFF),str_addraw(&r,((x)>>8)&0xFF) // more like bug-endian, amirite?
#define add_long(x)  str_addraw(&r,(x)&0xFF),str_addraw(&r,((x)>>8)&0xFF),str_addraw(&r,((x)>>16)&0xFF),str_addraw(&r,((x)>>24)&0xFF)
#define add_byte_framed(b) {if(bo==r.c)add_byte(0);r.sv[bo]++;add_byte(b);if((0xFF&r.sv[bo])==0xFF)bo=r.c;}
#define add_bits(c) {b|=(c)<<nb;nb+=w;while(nb>=8){add_byte_framed(b&0xff);b>>=8,nb-=8;}}
#define inc_hi() {hi++;if(hi==ov){w++;ov<<=1;}if(hi==0xfff){unsigned int c=1<<lw;add_bits(c);w=lw+1;hi=c+1;ov=c<<1;memset(t, 0, sizeof(t));tc=1;}}
char* writegif(lv*frames,lv*delays,int*len){
	lv*patterns=patterns_read(lmd());str r=str_new();pair size={1,1};
	EACH(z,frames)size=pair_max(size,image_size(frames->lv[z]));
	str_addz(&r,"GIF89a");add_short(size.x),add_short(size.y);
	add_byte(0xF4);          // global colortable, 8-bits per channel, 32 colors
	add_byte(0),add_byte(0); // background color is 0, 1:1 pixel aspect ratio
	for(int z=0;z<16;z++)add_byte(COLORS[z]>>16),add_byte(COLORS[z]>>8),add_byte(COLORS[z]); // global colortable
	for(int z=0;z<16;z++)add_byte(0xFF),add_byte(0xFF),add_byte(0xFF); // padding entries
	add_short(0xFF21),add_byte(11),str_addz(&r,"NETSCAPE2.0"),add_byte(3),add_byte(1),add_short(0),add_byte(0); // NAB; loop gif forever
	str_provision(&r,r.size+frames->c*(20+(size.x*size.y*2)));
	EACH(frame,frames)if(image_is(frames->lv[frame])){
		add_byte(0x21),add_byte(0xF9),add_byte(4); // graphic control extension
		add_byte(9),add_short(((int)ln(delays->lv[frame]))),add_byte(16); // dispose to bg + has transparency, 100ths of a second delay, color 16 is transparent
		add_byte(0); // end GCE
		add_byte(0x2C); // image descriptor
		size=image_size(frames->lv[frame]);add_short(0),add_short(0),add_short(size.x),add_short(size.y); // window {x,y,width,height}
		add_byte(0),add_byte(5); // no local colortable, minimum LZW code size = 5
		unsigned int count=0; char*data=frames->lv[frame]->b->sv;
		unsigned int lw=5,w=1+lw,hi=(1<<lw)+1,ov=1<<(lw+1),sc=-1,b=0,nb=0,t[1<<14]={0},tm=(1<<14)-1,tc;int bo=r.c;
		for(int y=0;y<size.y;y++)for(int x=0;x<size.x;x++){
			int v=draw_color_trans(patterns_pal(patterns),data[y*size.x+x],count,x,y);
			unsigned int c=sc;if(c==(unsigned)-1){add_bits(1<<lw);sc=v;continue;} // first write sends clear code
			unsigned int k=(c<<8)|v,hash=((k>>12)^k)&tm,em=0;
			for(unsigned int h=hash;t[h];h=(h+1)&tm)if(k==t[h]>>12){em=1,sc=t[h]&0xfff;}
			if(!em){add_bits(c);sc=v;tc=0;inc_hi();if(!tc){while(t[hash])hash=(hash+1)&tm;t[hash]=(k<<12)|hi;}}
		}
		add_bits(sc);inc_hi();add_bits((1<<lw)+1);if(nb>0)add_byte_framed(b&0xff);
		add_byte(0),count++; // end of frame
	}add_byte(0x3B); // end of GIF
	return *len=r.c, r.sv;
}
char* writewav(lv*data,int*len){
	str r=str_new();
	str_addz(&r,"RIFF");
	add_long(4+24+(8+data->c)+(data->c%2));
	str_addz(&r,"WAVE");
	str_addz(&r,"fmt ");
	add_long(16);   // chunk size
	add_short(1);   // pcm data
	add_short(1);   // 1 channel
	add_long(8000); // 8khz
	add_long(8000); // 8000*(1 byte per sample)*(1 channel)
	add_short(1);   //      (1 byte per sample)*(1 channel)
	add_short(8);   // 8 bits per sample
	str_addz(&r,"data");
	add_long(data->c);
	str_provision(&r,r.size+data->c+1);
	for(int z=0;z<data->c;z++)str_addraw(&r,(128+data->sv[z]));
	if(data->c%2)str_addraw(&r,0);
	return *len=r.c, r.sv;
}

#ifndef __COSMOPOLITAN__
#include <sys/stat.h>
#endif
lv* n_readgif(lv*self,lv*a){
	(void)self;lv*name=drom_to_utf8(l_first(a));lv*hint=a->c>1?ls(a->lv[1]):lmistr("color");
	int gray=!strcmp("gray",hint->sv)||!strcmp("gray_frames",hint->sv);
	int frames=!strcmp("frames",hint->sv)||!strcmp("gray_frames",hint->sv);
	struct stat st;if(stat(name->sv,&st)||st.st_size<13)return frames?empty_frames():image_empty();
	char*data=calloc(st.st_size,1);FILE*f=fopen(name->sv,"rb");
	if(fread(data,1,st.st_size,f)!=(unsigned)st.st_size){fclose(f),free(data);return frames?empty_frames():image_empty();}
	fclose(f);return readgif(data,st.st_size,gray,frames);
}
char* n_writegif_raw(lv*a,int*len){
	if(a->c<2)return NULL;lv*i=lml(0),*d=lml(0);
	lv*si=lil(a->lv[1])?a->lv[1]: lid(a->lv[1])?dgetv(a->lv[1],lmistr("frames")): l_list(a->lv[1]); if(!lil(si))si=l_list(si);
	lv*sd=lid(a->lv[1])?dget(a->lv[1],lmistr("delays")) :lml(0);
	EACH(z,si)if(image_is(si->lv[z])&&!is_empty(si->lv[z]))ll_add(i,si->lv[z]),ll_add(d,lil(sd)?lmn(z>=sd->c?3: CLAMP(1,ln(sd->lv[z]),65535)): lmn(ln(sd)));
	if(i->c<1)return NULL;return writegif(i,d,len);
}
lv* n_writegif(lv*self,lv*a){
	(void)self;lv*name=drom_to_utf8(l_first(a));int len=0;char*data=n_writegif_raw(a,&len);if(!data)return ZERO;
	FILE*f=fopen(name->sv,"wb");if(f)fwrite(data,1,len,f),fclose(f);free(data);return f?ONE:ZERO;
}
lv* n_writewav(lv*self,lv*a){
	(void)self;lv*name=drom_to_utf8(l_first(a));if(a->c<2||!sound_is(a->lv[1]))return ZERO;
	int len=0;char*data=writewav(a->lv[1]->b,&len);
	FILE*f=fopen(name->sv,"wb");if(f)fwrite(data,1,len,f),fclose(f);free(data);return f?ONE:ZERO;
}
lv* readbin(lv*path){
	struct stat st;path=drom_to_utf8(path);if(stat(path->sv,&st))return array_make(0,0,0,lms(0));FILE*f=fopen(path->sv,"rb");
	lv*r=lms(st.st_size);if(fread(r->sv,1,r->c,f)!=(unsigned)r->c){fclose(f);return array_make(0,0,0,lms(0));}
	fclose(f);return array_make(st.st_size,0,0,r);
}
lv* n_read(lv*self,lv*a){
	(void)self;a=drom_to_utf8(l_first(a));struct stat st;if(stat(a->sv,&st)){return lms(0);}FILE*f=fopen(a->sv,"rb");if(!f)return lms(0);
	char head[]={0,0,0},ref[]={0xEF,0xBB,0xBF};if(fread(head,1,sizeof(head),f)!=sizeof(head)){fclose(f);return lms(0);}
	int bom=memcmp(head,ref,sizeof(head))==0; // UTF-8 BOM
	lv*r=lms(st.st_size-(bom?3:0));fseek(f,bom?3:0,SEEK_SET);if(fread(r->sv,1,r->c,f)!=(unsigned)r->c){fclose(f);return lms(0);}
	fclose(f);return lmutf8(r->sv);
}
lv* writebin(lv*path,lv*x){
	array a=unpack_array(x);FILE*f=fopen(drom_to_utf8(path)->sv,"wb");if(f)fwrite(a.data->sv,1,a.data->c,f),fclose(f);return f?ONE:ZERO;
}
lv* n_write(lv*self,lv*a){
	(void)self;lv*x=a->c>0?drom_to_utf8(a->lv[0]):lms(0),*y=a->c>1?drom_to_utf8(a->lv[1]):lms(0);
	FILE*f=fopen(x->sv,"w");if(f)fwrite(y->sv,1,y->c,f),fclose(f);return f?ONE:ZERO;
}
lv* n_newdeck(lv*self,lv*a){(void)self;a=l_first(a);return deck_read(lin(a)?lmistr(""): ls(a));}
lv* idecode(lv*x){
	return has_prefix(x->sv,"%%img")?image_read(x):
	       has_prefix(x->sv,"%%snd")?sound_read(x):
	       has_prefix(x->sv,"%%dat")?array_read(x):LNIL;
}

// Filesystem Traversal

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
typedef struct {int dir;char name[PATH_MAX];} dir_item;
dir_item*directory;int directory_count=0,directory_size=0;
enum file_filter{filter_none,filter_deck,filter_data,filter_code,filter_sound,filter_image,filter_gif};
void directory_push(int dir,char*name,int filter){
	if(name[0]=='.')return;
	if(!dir&&filter==filter_deck&&!has_suffix(name,".html")&&!has_suffix(name,".deck"))return;
	if(!dir&&filter==filter_data&&!has_suffix(name,".txt")&&!has_suffix(name,".csv"))return;
	if(!dir&&filter==filter_code&&!has_suffix(name,".txt")&&!has_suffix(name,".lil"))return;
	if(!dir&&filter==filter_sound&&!has_suffix(name,".wav"))return;
	if(!dir&&filter==filter_gif  &&!has_suffix(name,".gif"))return;
	if(!dir&&filter==filter_image&&!has_suffix(name,".gif")&&!has_suffix(name,".bmp")&&
		                           !has_suffix(name,".png")&&!has_suffix(name,".jpg")&&!has_suffix(name,".jpeg"))return;
	grower(directory,dir_item);
	directory[directory_count].dir=dir;snprintf(directory[directory_count++].name,PATH_MAX,"%s",name);
}

#ifdef _WIN32
#include <windows.h>
#define SEPARATOR "\\"
#define HOME      "USERPROFILE"
void directory_fetch(char*path,int filter){
	directory_count=0;
	char wildcard[PATH_MAX];
	if(!strlen(path)){
		int m=GetLogicalDrives();
		for(int z=0;z<26;z++)if(m&(1<<z)){snprintf(wildcard,PATH_MAX,"%c:\\",z+'A');directory_push(1,wildcard,filter);}
		return;
	}
	snprintf(wildcard,PATH_MAX,"%s%s*",path,SEPARATOR);
	WIN32_FIND_DATAA find;HANDLE d=FindFirstFileA(wildcard,&find);
	if(d==INVALID_HANDLE_VALUE)return;
	do{
		if(find.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)continue;
		directory_push(find.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY?1:0,find.cFileName,filter);
	}while(FindNextFileA(d,&find));FindClose(d);
}
#include "shlwapi.h"
void directory_normalize(char*x,const char*src){
	if(!strlen(src)){snprintf(x,PATH_MAX,"");return;} // retain identity for parent of drives
	_fullpath(x,src,PATH_MAX);
}
void directory_parent(char*x){
	if(strlen(x)<=3){snprintf(x,PATH_MAX,"");return;} // parent of a drive root
	char t[PATH_MAX];snprintf(t,PATH_MAX,"%s%s..",x,SEPARATOR);directory_normalize(x,t);
}
char** os_env_list(void){return *__p__environ();}
#else
#ifndef __COSMOPOLITAN__
#include <dirent.h>
#endif
#define SEPARATOR "/"
#define HOME      "HOME"
void directory_fetch(char*path,int filter){
	directory_count=0;
	DIR*d=opendir(path);if(d==NULL)return;
	struct dirent*find;char work_path[PATH_MAX];
	while((find=readdir(d))){
		snprintf(work_path,PATH_MAX,"%s%s%s",path,strlen(path)?SEPARATOR:"",find->d_name);
		DIR*test=opendir(work_path);directory_push(test!=NULL,find->d_name,filter);
		if(test!=NULL)closedir(test);
	}closedir(d);
}
void directory_normalize(char*x,const char*src){if(realpath(src,x)==NULL){x[0]='\0';}}
void directory_parent(char*x){char t[PATH_MAX];snprintf(t,PATH_MAX,"%s%s..",x,SEPARATOR);directory_normalize(x,t);}
char** os_env_list(void){extern char ** environ;return environ;}
#endif

void directory_home(char*path){
	#ifdef __ANDROID__
		directory_normalize(path,"/sdcard/");
	#else
		directory_normalize(path,getenv(HOME));
	#endif
}
int directory_is_home(char*path){char t[PATH_MAX];directory_home(t);return !strcmp(path,t);}
int directory_sort(const void*a,const void*b){
	const dir_item*ia=((dir_item*)a);
	const dir_item*ib=((dir_item*)b);
	if(ia->dir==1&&ib->dir!=1)return -1; // directories come before files
	if(ia->dir!=1&&ib->dir==1)return  1;
	return strcmp(ia->name,ib->name); // sort alphabetically asc by name
}
lv* directory_enumerate(char*root,int filter,int type){
	root=drom_to_utf8(lmcstr(root))->sv;
	directory_fetch(root,filter);
	qsort(directory,directory_count,sizeof(dir_item),directory_sort);
	lv*r=lmt(),*d=lml(directory_count),*n=lml(directory_count);
	dset(r,lmistr("icon"),d),dset(r,lmistr("name"),n);
	for(int z=0;z<directory_count;z++){
		d->lv[z]=lmn((directory[z].dir?0:1)^type);
		n->lv[z]=lmutf8(directory[z].name);
	}if(!type)return torect(r);
	lv*t=lml(directory_count);dset(r,lmistr("type"),t);
	for(int z=0;z<directory_count;z++){
		char*path=directory[z].name;
		int i=strlen(path);while(i>=0&&path[i]!='.')path[i]=tolower(path[i]),i--;
		t->lv[z]=lmutf8(!directory[z].dir&&path[i]=='.'?path+i:"");
	}return torect(r);
}
void directory_cat(char*x,char*a,char*b){if(strlen(a)==0){snprintf(x,PATH_MAX,"%s",b);}else{snprintf(x,PATH_MAX,"%s%s%s",a,SEPARATOR,b);}}
void directory_child(char*x,char*name){char t[PATH_MAX];directory_cat(t,x,name);snprintf(x,PATH_MAX,"%s",t);}
int directory_has_parent(char*x){return strcmp(x,SEPARATOR)!=0&&strlen(x)>0;}
char* directory_last(char*x){int r=strlen(x);while(r&&x[r-1]!=SEPARATOR[0])r--;return x+r;}
int directory_exists(lv*x){struct stat st;return stat(drom_to_utf8(x)->sv,&st)?0:1;}

// Danger : nonportable, potentially hazardous IO

lv*env_enumerate(void){
	char**env=os_env_list();lv*r=lmd();for(int z=0;env[z];z++){
		int i=0;while(env[z][i]&&env[z][i]!='=')i++;
		str k=str_new();utf8_to_drom(&k,env[z],i);dset(r,lmstr(k),lmutf8(env[z]+i+1));
	}return r;
}
lv*n_dir(lv*self,lv*a){
	(void)self;lv*r=directory_enumerate(ls(l_first(a))->sv,filter_none,1);
	r->kv[0]=lmistr("dir");return r;
}
#ifndef _WIN32
#ifndef __COSMOPOLITAN__
#include <sys/wait.h>
#endif
lv*n_shell(lv*self,lv*a){
	(void)self;lv*x=drom_to_utf8(l_first(a)),*r=lmd();FILE*child=popen(x->sv,"r");str o=str_new();
	while(1){int c=fgetc(child);if(feof(child))break;str_addraw(&o,c);}int e=pclose(child);lv*os=lmstr(o);
	return dset(r,lmistr("out"),lmutf8(os->sv)),dset(r,lmistr("exit"),lmn(WIFEXITED(e)?WEXITSTATUS(e): -1)),r;
}
#endif
lv*n_path(lv*self,lv*a){
	(void)self;char t[PATH_MAX]={0},t2[PATH_MAX]={0};
	if     (a->c==1){directory_normalize(t2,drom_to_utf8(a->lv[0])->sv);}
	else if(a->c==2&&!strcmp(drom_to_utf8(a->lv[1])->sv,"..")){directory_normalize(t2,drom_to_utf8(a->lv[0])->sv),directory_parent(t2);}
	else if(a->c==2&&!strcmp(drom_to_utf8(a->lv[0])->sv,""  )){directory_normalize(t2,drom_to_utf8(a->lv[1])->sv);}
	else if(a->c==2){directory_cat(t,drom_to_utf8(a->lv[0])->sv,drom_to_utf8(a->lv[1])->sv),directory_normalize(t2,t);}
	return lmutf8(t2);
}
lv*n_writedeck(lv*self,lv*a){
	lv*path=ls(l_first(a));int html=0;if(has_suffix(path->sv,".html")){html=1;}
	lv*v=deck_write(a->c<2?ZERO:a->lv[1],html);if(v->c<1)return ZERO;return n_write(self,lml2(path,v));
}
lv*n_writefile(lv*self,lv*a){
	lv*value=a->c>1?a->lv[1]:lms(0);
	if(array_is(value))return writebin(l_first(a),value);
	if(sound_is(value))return n_writewav(self,a);
	if(image_is(value)||lid(value))return n_writegif(self,a);
	if(deck_is(value))return n_writedeck(self,a);
	if(lil(value)){EACH(z,value)if(image_is(value->lv[z]))return n_writegif(self,a);}
	return n_write(self,a);
}
lv*n_readdeck(lv*self,lv*a){return deck_read(lin(l_first(a))?lmistr(""):n_read(self,a));}
lv*n_readfile(lv*self,lv*a);// forward ref
lv* interface_danger(lv*self,lv*i,lv*x){
	ikey("homepath"){char t[PATH_MAX];directory_home(t);return lmcstr(t);}
	ikey("env"     )return env_enumerate();
	ikey("dir"     )return lmnat(n_dir      ,self);
	ikey("path"    )return lmnat(n_path     ,self);
	ikey("write"   )return lmnat(n_writefile,self);
	ikey("read"    )return lmnat(n_readfile ,self);
#ifndef _WIN32
	ikey("shell"   )return lmnat(n_shell    ,self);
#endif
	return x?x:LNIL;
}
