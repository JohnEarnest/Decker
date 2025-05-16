// Learning in Layers
#ifndef __COSMOPOLITAN__
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#endif

typedef struct{int c,size;char*sv;}str;
typedef struct lvs{int t,c,n,s,ns,g;double nv;char*sv;struct lvs**lv,**kv,*a,*b,*env;void*f;}lv;
typedef struct{int c,size,*iv;}idx;
typedef struct{lv*p,*t,*e;idx pcs;}pstate;pstate state={0}; // parameters, tasks, envs, index
typedef struct{int lo,hi,live,size,g,ss;lv**heap;long frees,allocs,depth;pstate st[4];}gc_state;gc_state gc={0};
typedef struct{char*name;void*func;}primitive;
int seed=0x12345;lv interned[1024]={{0}};unsigned int intern_count=383+1, do_panic=0;
#define intern_num {if(x==floor(x)&&x>=-128&&x<=255)return &interned[((int)x)+128];}
lv*n_show (lv*self,lv*a); // user-supplied function which displays raw to stdout.
lv*n_print(lv*self,lv*a); // user-supplied function which formats/displays to stdout.
lv*debug_show(lv*x);      // version of l_show() which _always_ goes to stdout, for internal use.
lv*idecode(lv*x);         // decode datablock representation of interfaces into an instance or 0.
lv nilvalue={-1,0,0,0,0,0,0.0,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
int linil(lv*x){return x==&nilvalue;}
#define LNIL         (&nilvalue)
#define NUM          512 // number parsing/formatting buffer size
#define ONE          lmn(1)
#define ZERO         lmn(0)
#define lmbool(x)    ((x)?ONE:ZERO)
#define NOSTR        (str){0,0,NULL}
#define MAX(a,b)     ((a)<(b)?(b):(a))
#define MIN(a,b)     ((a)<(b)?(a):(b))
#define SIGN(x)      (x>0?1:-1)
#define EACH(v,x)    for(int v=0;v<x->c;v++)
#define FIND(v,x,k)  for(int v=0;v<x->c;v++)if(matchr(x->kv[v],k))
#define SFIND(v,x,k) for(int v=0;v<x->c;v++)if(k==x->kv[z]->sv||!strcmp(x->kv[z]->sv,k))
#define EACHR(v,x)   for(int v=x->c-1;v>=0;v--)
#define GEN(v,n)     lv*v=lml(n);   for(int z=0;z<n   ;z++)v->lv[z]=
#define MAP(v,x)     lv*v=lml(x->c);for(int z=0;z<x->c;z++)v->lv[z]=
#define DMAP(v,x,f)  lv*v=lmd();    for(int z=0;z<x->c;z++)dset(v,x->kv[z],f);
#define TMAP(v,x,f)  lv*v=lmt();    for(int z=0;z<x->c;z++)dset(v,ls(x->kv[z]),f);
#define monad(n)     lv*n(lv*x)
#define dyad(n)      lv*n(lv*x,lv*y)
void idx_free(idx*x){free(x->iv);}
idx  idx_new(int n){int c=MAX(n,16);return(idx){n,c,calloc(c,sizeof(int))};}
int* idx_peek(idx*x){if(x->c<1)printf("peek empty idx stack!\n");return &x->iv[x->c-1];}
int  idx_pop (idx*x){if(x->c<1)printf("pop empty idx stack!\n");return x->iv[--(x->c)];}
void idx_push(idx*x,int n){if(x->size<x->c+1)x->iv=realloc(x->iv,(x->size*=2)*sizeof(int));x->iv[x->c++]=n;}
str str_new(void){return(str){0,32,calloc(32,1)};}
void str_provision(str*s,int size){if(s->size<size)s->sv=realloc(s->sv,s->size=size);}
void str_addraw(str*s,int x){if(s->c+1>=s->size)s->sv=realloc(s->sv,s->size*=2);s->sv[s->c++]=x;}
void str_term(str*s){str_addraw(s,'\0');}
void str_addc(str*s,char x){if(x!='\r')str_addraw(s,x=='\t'?' ':((0xFF&x)>=32)||x=='\n'?x:255);}
void str_add(str*s,char*x,int n){for(int z=0;z<n;z++)str_addc(s,x[z]);}
void str_addz(str*s,char*x){str_add(s,x,strlen(x));} // null-terminated c-string
void str_addl(str*s,lv*x){str_add(s,x->sv,x->c);}    // counted lil string
lv*  ll_peek(lv*x){return x->c?x->lv[x->c-1]:NULL;}
lv*  ll_pop(lv*x){return x->c?x->lv[--(x->c)]:NULL;}
lv*  ll_unshift(lv*x){lv*r=x->c?x->lv[0]:NULL;for(int z=0;z<x->c-1;z++)x->lv[z]=x->lv[z+1];x->c--;return r;}
void ll_add(lv*x,lv*y){if(x->s<x->c+1)x->lv=realloc(x->lv,(x->s*=2)*sizeof(lv*));x->lv[x->c++]=y;}
void ld_add(lv*d,lv*k,lv*x){
	if(d->c+1>d->s){
		d->s*=2;d->kv=realloc(d->kv,d->s*sizeof(lv*));d->lv=realloc(d->lv,d->s*sizeof(lv*));
	}d->kv[d->c]=k,d->lv[d->c]=x,d->c++;
}
void lv_walk(lv*x){
	if(x==NULL||x->g==gc.g){return;}x->g=gc.g;
	if(x->lv)EACH(z,x)lv_walk(x->lv[z]);if(x->kv)EACH(z,x)lv_walk(x->kv[z]);
	lv_walk(x->a),lv_walk(x->b),lv_walk(x->env);
}
void lv_free(lv*x){
	if(!x)return;
	if(x->lv)free(x->lv);if(x->kv)free(x->kv);if(x->sv&&!(x->t==1&&x->b))free(x->sv);free(x);gc.frees++,gc.live--;
}
void lv_grow(void){
	gc.heap=realloc(gc.heap,(gc.size*2)*sizeof(lv*));
	memset(gc.heap+gc.size,0,gc.size*sizeof(lv*));gc.size*=2;
}
void lv_collect(void){
	if(gc.live+(gc.size*0.1)<gc.size)return;gc.g++;
	for(int z=0;z<gc.ss;z++){lv_walk(gc.st[z].e),lv_walk(gc.st[z].p),lv_walk(gc.st[z].t);}
	lv_walk(state.e),lv_walk(state.p),lv_walk(state.t);
	for(int z=0;z<gc.hi;z++)if(gc.heap[z]){
		if(gc.heap[z]->g!=gc.g){lv_free(gc.heap[z]),gc.heap[z]=NULL,gc.lo=MIN(gc.lo,z);}
		else{gc.hi=MAX(gc.hi,z);}
	}
}
int lv_stash(lv*x){
	while(gc.lo<gc.size&&gc.heap[gc.lo]!=NULL)gc.lo++;
	return(gc.lo>=gc.size)?0: (gc.heap[gc.lo]=x,gc.hi=MAX(gc.hi,gc.lo),gc.lo++,1);
}
lv* lmv(int type){
	gc.allocs++,gc.live++;lv*r=calloc(1,sizeof(lv));r->t=type;
	if(gc.heap==NULL){gc.size=64,gc.heap=calloc(gc.size,sizeof(lv*));}
	if(lv_stash(r))return r;lv_grow();lv_stash(r);return r;
}
lv* lmvv(int t,int n){lv*r=lmv(t);r->lv=calloc(r->s=MAX(n,8),sizeof(lv*));r->c=n;return r;}
#define lm(n,c) int li##n(lv*x){return x&&x->t==c;} lv*lm##n
lm(n  ,0)(double x){intern_num;lv*r=lmv(0);r->c=1,r->nv=isfinite(x)?x:0;                 return r;}
lm(s  ,1)(int n)           {lv*r=lmv(1);r->c=n;r->sv=calloc(n+1,1);                      return r;}
lm(l  ,2)(int n)           {lv*r=lmvv(2,n);                                              return r;}
lm(d  ,3)(void)            {lv*r=lmvv(3,16);r->c=0,r->kv=calloc(16,sizeof(lv*));         return r;}
lm(t  ,4)(void)            {lv*r=lmvv(4,16);r->c=0,r->kv=calloc(16,sizeof(lv*));         return r;}
lm(on ,5)(str n,lv*r,lv*b) {r->t=5,r->sv=n.sv,r->b=b;                                    return r;}
lm(i  ,6)(lv*(*f)(lv*,lv*,lv*),lv*n,lv*s){lv*r=lmv(6);r->f=(void*)f,r->a=n,r->b=s;       return r;}
lm(blk,7)(void)            {lv*r=lmvv(7,0);r->sv=calloc(32,sizeof(char)),r->ns=32,r->n=0;return r;}
lm(env,8)(lv*p)            {lv*r=lmd();r->t=8;r->env=p;                                  return r;}
lm(nat,9)(lv*(*f)(lv*,lv*),lv*c){lv*r=lmv(9);r->f=(void*)f,r->a=c;                       return r;}
lv* lmstr(str x){lv*r=lmv(1);str_term(&x),r->c=strlen(x.sv);r->sv=x.sv;return r;}
lv* lmcstr(char*x){lv*r=lmv(1);r->c=strlen(x),r->sv=calloc(r->c+1,1),memcpy(r->sv,x,r->c);return r;}
lv* lmistr(char*x){
	for(unsigned int z=383+1;z<intern_count;z++)if(interned[z].sv==x)return &interned[z];
	lv*r=&interned[intern_count++];r->t=1,r->c=strlen(x),r->sv=x;
	if(intern_count>sizeof(interned))printf("warning: intern heap is full!\n");return r;
}
lv* lmslice(lv*x,int off){lv*r=lmv(1);r->c=MAX(0,x->c-off),r->b=x->b?x->b:x;r->sv=x->sv+MIN(MAX(0,off),x->c);return r;}
int     mod(int    x,int    y){x=y==0?0:x%y      ;if(x<0)x+=y;return x;}
double dmod(double x,double y){x=y==0?0:fmod(x,y);if(x<0)x+=y;return x;}
double rnum_len(char*x,int n,int*len){
	if(n==0)return 0;int i=0,sign=1;double r=0,p=10;while(isspace(x[i]))i++;
	if(x[0]=='-')sign=-1,i++;while(i<n&&isdigit(x[i]))r=r*10+(x[i]-'0'),i++;
	if(x[i]=='.')        i++;while(i<n&&isdigit(x[i]))r+=(x[i++]-'0')/p,p*=10;
	return (*len)=i,sign*r;
}
double rnum(char*x,int n){int i=0;return rnum_len(x,n,&i);}
void cswap(char*t,int a,int b){char v=t[a];t[a]=t[b],t[b]=v;}
void crev(char*t,int n){int i=0,j=n-1;while(i<j)cswap(t,i++,j--);}
void wnum(str*x,double y){
	if(y<0)y=-y,str_addc(x,'-');char t[NUM*2]={0};int n=0,s=0;
	t[n++]='0';double i=floor(y);y=round((y-floor(y))*1000000.0);if(y>=1000000)i++;
	while(i>0){t[n++]=fmod(i,10)+'0',i=i/10;}crev(t+1,n-1);while(t[s]=='0'&&t[s+1])s++;t[n++]='.';
	for(int z=0;z<6;z++){t[n+5-z]=fmod(y,10)+'0',y=y/10;}n+=5;while(n>0&&t[n]=='0')n--;if(t[n]=='.')n--;
	str_add(x,t+s,n-s+1);
}
monad(l_rows);monad(l_cols);monad(l_range);monad(l_list);monad(l_first);
dyad(l_dict);dyad(l_fuse);dyad(l_take);void dset(lv*d,lv*k,lv*x);
int    lb(lv*x){return linil(x)?0: lin(x)?x->nv!=0:lis(x)||lil(x)||lid(x)?x->c!=0:1;}
double ln(lv*x){return linil(x)?0: lin(x)?x->nv:lis(x)?rnum(x->sv,x->c):(lil(x)||lid(x))&&x->c?ln(x->lv[0]):0;}
lv* ls(lv*x){
	if(lin(x)){str n=str_new();wnum(&n,x->nv);return lmstr(n);}
	return lis(x)?x: lil(x)?l_fuse(lmistr(""),x): lms(0);
}
lv* ll(lv*x){
	if(lid(x)){lv*r=lml(x->c);EACH(z,x)r->lv[z]=x->lv[z];return r;}
	if(lis(x)){lv*r=lml(x->c);EACH(z,x)r->lv[z]=lms(1),r->lv[z]->sv[0]=x->sv[z];return r;}
	return linil(x)?lml(0): lil(x)?x: lit(x)?l_rows(x): l_list(x);
}
lv* ld(lv*x){return lid(x)?x: lit(x)?l_cols(x): lil(x)||lis(x)?l_dict(l_range(lmn(x->c)),ll(x)): lmd();}
lv* lt(lv*x){
	if(lit(x))return x;lv*r=lmt();if(linil(x))return r;if(lid(x)||lil(x)){
		if(lid(x)){lv*k=lml(x->c);EACH(z,x)k->lv[z]=x->kv[z];dset(r,lmistr("key"),k);}
		r->n=x->c; lv*v=lml(x->c);EACH(z,x)v->lv[z]=x->lv[z];dset(r,lmistr("value"),v);
	}else{x=ll(x);dset(r,lmistr("value"),x);r->n=x->c;}return r;
}
lv* lml2(lv*x,lv*y){lv*r=lml(2);r->lv[0]=x,r->lv[1]=y;return r;}
lv* lml3(lv*x,lv*y,lv*z){lv*r=lml(3);r->lv[0]=x,r->lv[1]=y,r->lv[2]=z;return r;}
lv* lml4(lv*x,lv*y,lv*z,lv*w){lv*r=lml(4);r->lv[0]=x,r->lv[1]=y,r->lv[2]=z,r->lv[3]=w;return r;}
int matchr(lv*x,lv*y){
	if(x==y)return 1;if(x->t!=y->t||x->n!=y->n||x->c!=y->c)return 0;
	if(lin(x))return x->nv==y->nv; if(lis(x))return !strcmp(x->sv,y->sv);
	if(lil(x)){EACH(z,x)if(!matchr(x->lv[z],y->lv[z]))return 0;return 1;}
	if(lid(x)||lit(x)){EACH(z,x)if(!matchr(x->lv[z],y->lv[z])||!matchr(x->kv[z],y->kv[z]))return 0;return 1;}
	return 0;
}
void dsetuq(lv*d,lv*k,lv*x){
	FIND(z,d,k){str s=str_new();str_addl(&s,k);str_addc(&s,'_');k=lmstr(s);break;}ld_add(d,k,x);
}
void dset(lv*d,lv*k,lv*x){FIND(z,d,k){d->lv[z]=x;return;}ld_add(d,k,x);}
lv* dget(lv*d,lv*k){FIND(z,d,k)return d->lv[z];return NULL;}
lv* dgetv(lv*d,lv*k){FIND(z,d,k)return d->lv[z];return LNIL;}
int dgeti(lv*d,lv*k){EACH(z,d)if(matchr(d->kv[z],k))return z;return -1;}
lv* dkey(lv*d,lv*v){EACH(z,d)if(matchr(d->lv[z],v))return d->kv[z];return LNIL;}
lv* amend(lv*x,lv*i,lv*y){
	if(lii(x))return ((lv*(*)(lv*,lv*,lv*))x->f)(x,i,y);
	if(lit(x)&&lin(i)){
		lv*rn=lmn(x->n), *r=l_take(rn,x);int ri=ln(i);if(!lid(y)){lv*t=lmd();EACH(z,x)dset(t,x->kv[z],y);y=t;}
		if(ri>=0&&ri<x->n)EACH(k,y){int ki=dgeti(r,ls(y->kv[k]));if(ki!=-1)r->lv[ki]=amend(r->lv[ki],lmn(ri),y->lv[k]);}return r;
	}
	if(lit(x)&&lis(i)){
		lv*rn=lmn(x->n), *r=l_take(rn,x), *c=lil(y)?l_take(lmn(MIN(y->c,x->n)),ll(y)): l_take(rn,l_list(y));
		while(c->c<x->n)ll_add(c,LNIL);dset(r,ls(i),c);return r;
	}
	if(!lis(x)&&!lil(x)&&!lid(x))return amend(lml(0),i,y);
	if((lil(x)||lis(x))&&(!lin(i)||(i->nv<0||i->nv>x->c)))return amend(ld(x),i,y);
	if(lil(x)){int n=ln(i);MAP(r,x)z==n?y:x->lv[z];if(n==x->c)ll_add(r,y);return r;}
	if(lid(x)){DMAP(r,x,x->lv[z])return dset(r,i,y),r;}
	if(lis(x)){
		str r=str_new();int n=ln(i);y=ls(y);
		str_add(&r,x->sv,n),str_addl(&r,y),str_addz(&r,x->sv+n+1);return lmstr(r);
	}return lml(0);
}
lv* l_ati(lv*x,lv*y){return lis(y)&&!strcmp(y->sv,"type")?x->a: ((lv*(*)(lv*,lv*,lv*))x->f)(x,y,NULL);}
lv* l_at(lv*x,lv*y){
	if(linil(x))return LNIL;
	if(lii(x))return l_ati(x,y);
	if(lit(x)&&lin(y))x=l_rows(x); if((lis(x)||lil(x))&&linil(y))y=ZERO; if((lis(x)||lil(x))&&!lin(y))x=ld(x);
	if(lis(x)){int n=ln(y);lv*r=lms(1);r->sv[0]=(n<0||n>=x->c)?(r->c=0,'\0'):x->sv[n];return r;}
	if(lil(x)){int n=ln(y);return n<0||n>=x->c?LNIL:x->lv[n];}
	return lid(x)||(lit(x)&&lis(y))?dgetv(x,y):LNIL;
}
lv* amendv(lv*x,lv*i,lv*y,int n,int*tla){
	if(lii(x)){*tla=0;}if(!*tla&&n+1<i->c)return amendv(l_at(x,l_first(i->lv[n])),i,y,n+1,tla);
	return (n+1<i->c)?amend(x,l_first(i->lv[n]),amendv(l_at(x,l_first(i->lv[n])),i,y,n+1,tla)):
	(n+1==i->c)?amend(x,l_first(i->lv[n]),y): y;
}
lv* perfuse(lv*x,lv*(f(lv*))){
	if(lid(x)){DMAP(r,x,perfuse(x->lv[z],f));return r;}
	if(lil(x)){MAP(r,x)perfuse(x->lv[z],f);return r;}return f(x);
}
lv* nlperfuse(lv*x,lv*(f(lv*))){if(!lil(x))return f(ll(x));int n=1;EACH(z,x)if(!lin(x->lv[z])){n=0;break;}if(n)return f(x);MAP(r,x)nlperfuse(x->lv[z],f);return r;}
lv* conform(lv*x,lv*y,lv*(f(lv*,lv*))){
	if(lid(x)&&lid(y)){ // union keys, zero-fill unmatched elements.
		DMAP(r,x,conform(x->lv[z],dgetv(y,x->kv[z]),f));
		EACH(z,y)if(!dget(x,y->kv[z]))dset(r,y->kv[z],conform(LNIL,y->lv[z],f));return r;
	}
	if( lid(x)&&!lid(y)){DMAP(r,x,conform(x->lv[z],y,f));return r;}
	if(!lid(x)&& lid(y)){DMAP(r,y,conform(x,y->lv[z],f));return r;}
	if( lil(x)&& lil(y)){MAP(r,x)conform(x->lv[z],y->c==0?LNIL:y->lv[z%y->c],f);return r;}
	if( lil(x)&&!lil(y)){MAP(r,x)conform(x->lv[z],y,f);return r;}
	if(!lil(x)&& lil(y)){MAP(r,y)conform(x,y->lv[z],f);return r;} return f(x,y);
}
lv* torect(lv*t){ // modifies t in-place to rectangularize columns!
	int n=0;EACH(z,t)n=MAX(n,lil(t->lv[z])?t->lv[z]->c:1);
	t->n=n; EACH(z,t)t->lv[z]=l_take(lmn(n),lil(t->lv[z])?t->lv[z]:l_list(t->lv[z]));return t;
}

// DeckRoman character encoding
char* DROM_INKEY[]={
	"…","À","À","Á","Á","Â","Â","Ã","Ã","Ä","Ä","Å","Å","Æ","Ç","Ç",
	"È","È","É","É","Ê","Ê","Ë","Ë","Ì","Ì","Í","Í","Î","Î","Ï","Ï",
	"Ð","Ñ","Ñ","Ò","Ò","Ó","Ó","Ô","Ô","Õ","Õ","Ö","Ö","Ø","Ù","Ù",
	"Ú","Ú","Û","Û","Ü","Ü","Ý","Ý","Þ","ß","à","à","á","á","â","â",
	"ã","ã","ä","ä","å","å","æ","ç","ç","è","è","é","é","ê","ê","ë",
	"ë","ì","ì","í","í","î","î","ï","ï","ð","ñ","ñ","ò","ò","ó","ó",
	"ô","ô","õ","õ","ö","ö","ø","ù","ù","ú","ú","û","û","ü","ü","ý",
	"ý","þ","ÿ","ÿ","Ā","Ā","ā","ā","Ă","Ă","ă","ă","Ą","Ą","ą","ą",
	"Ć","Ć","ć","ć","Ē","Ē","ē","ē","Ę","Ę","ę","ę","Ī","Ī","ī","ī",
	"ı","Ł","ł","Ń","Ń","ń","ń","Ō","Ō","ō","ō","Ő","Ő","ő","ő","Œ",
	"œ","Ś","Ś","ś","ś","Š","Š","š","š","Ū","Ū","ū","ū","Ű","Ű","ű",
	"ű","Ÿ","Ÿ","Ź","Ź","ź","ź","Ż","Ż","ż","ż","Ž","Ž","ž","ž","Ș",
	"Ș","ș","ș","Ț","Ț","ț","ț","ẞ","¡","¿","«","»","€","°","“","”",
	"‘","’", 0
};
int DROM_INPOINT[]={
	0x2026,0x00c0,0x0041,0x00c1,0x0041,0x00c2,0x0041,0x00c3,0x0041,0x00c4,0x0041,0x00c5,0x0041,0x00c6,0x00c7,0x0043,
	0x00c8,0x0045,0x00c9,0x0045,0x00ca,0x0045,0x00cb,0x0045,0x00cc,0x0049,0x00cd,0x0049,0x00ce,0x0049,0x00cf,0x0049,
	0x00d0,0x00d1,0x004e,0x00d2,0x004f,0x00d3,0x004f,0x00d4,0x004f,0x00d5,0x004f,0x00d6,0x004f,0x00d8,0x00d9,0x0055,
	0x00da,0x0055,0x00db,0x0055,0x00dc,0x0055,0x00dd,0x0059,0x00de,0x00df,0x00e0,0x0061,0x00e1,0x0061,0x00e2,0x0061,
	0x00e3,0x0061,0x00e4,0x0061,0x00e5,0x0061,0x00e6,0x00e7,0x0063,0x00e8,0x0065,0x00e9,0x0065,0x00ea,0x0065,0x00eb,
	0x0065,0x00ec,0x0069,0x00ed,0x0069,0x00ee,0x0069,0x00ef,0x0069,0x00f0,0x00f1,0x006e,0x00f2,0x006f,0x00f3,0x006f,
	0x00f4,0x006f,0x00f5,0x006f,0x00f6,0x006f,0x00f8,0x00f9,0x0075,0x00fa,0x0075,0x00fb,0x0075,0x00fc,0x0075,0x00fd,
	0x0079,0x00fe,0x00ff,0x0079,0x0100,0x0041,0x0101,0x0061,0x0102,0x0041,0x0103,0x0061,0x0104,0x0041,0x0105,0x0061,
	0x0106,0x0043,0x0107,0x0063,0x0112,0x0045,0x0113,0x0065,0x0118,0x0045,0x0119,0x0065,0x012a,0x0049,0x012b,0x0069,
	0x0131,0x0141,0x0142,0x0143,0x004e,0x0144,0x006e,0x014c,0x004f,0x014d,0x006f,0x0150,0x004f,0x0151,0x006f,0x0152,
	0x0153,0x015a,0x0053,0x015b,0x0073,0x0160,0x0053,0x0161,0x0073,0x016a,0x0055,0x016b,0x0075,0x0170,0x0055,0x0171,
	0x0075,0x0178,0x0059,0x0179,0x005a,0x017a,0x007a,0x017b,0x005a,0x017c,0x007a,0x017d,0x005a,0x017e,0x007a,0x0218,
	0x0053,0x0219,0x0073,0x021a,0x0054,0x021b,0x0074,0x1e9e,0x00a1,0x00bf,0x00ab,0x00bb,0x20ac,0x00b0,0x201c,0x201d,
	0x2018,0x2019, 0
};
int DROM_INVAL[]={
	127,128,128,129,129,130,130,131,131,132,132,133,133,134,135,135,
	136,136,137,137,138,138,139,139,140,140,141,141,142,142,143,143,
	144,145,145,146,146,147,147,148,148,149,149,150,150,151,152,152,
	153,153,154,154,155,155,156,156,157,158,159,159,160,160,161,161,
	162,162,163,163,164,164,165,166,166,167,167,168,168,169,169,170,
	170,171,171,172,172,173,173,174,174,175,176,176,177,177,178,178,
	179,179,180,180,181,181,182,183,183,184,184,185,185,186,186,187,
	187,188,189,189,190,190,191,191,192,192,193,193,194,194,195,195,
	196,196,197,197,198,198,199,199,200,200,201,201,202,202,203,203,
	204,205,206,207,207,208,208,209,209,210,210,211,211,212,212,213,
	214,215,215,216,216,217,217,218,218,219,219,220,220,221,221,222,
	222,223,223,224,224,225,225,226,226,227,227,228,228,229,229,230,
	230,231,231,232,232,233,233,234,235,236,237,238,239,240, 34, 34,
	 39, 39, 0
};
char* DROM_OUTPUT[256]={
	"�","�","�","�","�","�","�","�","�","�","\n","�","�","�","�","�",
	"�","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
	" ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/",
	"0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
	"@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
	"P","Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
	"`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o",
	"p","q","r","s","t","u","v","w","x","y","z","{","|","}","~","…",
	"À","Á","Â","Ã","Ä","Å","Æ","Ç","È","É","Ê","Ë","Ì","Í","Î","Ï",
	"Ð","Ñ","Ò","Ó","Ô","Õ","Ö","Ø","Ù","Ú","Û","Ü","Ý","Þ","ß","à",
	"á","â","ã","ä","å","æ","ç","è","é","ê","ë","ì","í","î","ï","ð",
	"ñ","ò","ó","ô","õ","ö","ø","ù","ú","û","ü","ý","þ","ÿ","Ā","ā",
	"Ă","ă","Ą","ą","Ć","ć","Ē","ē","Ę","ę","Ī","ī","ı","Ł","ł","Ń",
	"ń","Ō","ō","Ő","ő","Œ","œ","Ś","ś","Š","š","Ū","ū","Ű","ű","Ÿ",
	"Ź","ź","Ż","ż","Ž","ž","Ș","ș","Ț","ț","ẞ","¡","¿","«","»","€",
	"°","�","�","�","�","�","�","�","�","�","�","�","�","�","�","�",
};
int DROM_TOUPPER[256]={
	255,255,255,255,255,255,255,255,255,255, 10,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	 96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,234,128,
	129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,
	145,146,147,148,149,150,151,152,153,154,155,156,157,223,190,190,
	192,192,194,194,196,196,198,198,200,200,202,202, 73,205,205,207,
	207,209,209,211,211,213,213,215,215,217,217,219,219,221,221,223,
	224,224,226,226,228,228,230,230,232,232,234,235,236,237,238,239,
	240,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};
int DROM_TOLOWER[256]={
	255,255,255,255,255,255,255,255,255,255, 10,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95,
	 96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,
	175,176,177,178,179,180,181,182,183,184,185,186,187,188,158,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,184,185,186,187,188,189,191,191,
	193,193,195,195,197,197,199,199,201,201,203,203,204,206,206,208,
	208,210,210,212,212,214,214,216,216,218,218,220,220,222,222,189,
	225,225,227,227,229,229,231,231,233,233,158,235,236,237,238,239,
	240,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};
lv* drom_to_utf8(lv*x){
	str r=str_new();x=ls(x);
	EACH(z,x){char*o=DROM_OUTPUT[0xFF&x->sv[z]];while(*o){str_addraw(&r,*o);o++;};}
	str_term(&r);lv*p=lmv(1);p->c=r.c-1;p->sv=r.sv;return p;
}
void utf8_to_drom(str*s,char*x,int n){
	for(int z=0;z<n;z++){unsigned char c=x[z];
		if     (c=='\r')continue;             // suppress windows newlines
		else if(c=='\t')c=' ';                // resolve the tabs vs. spaces debate once and for all
		else if(c<' '&&c!='\n'){c=255;}       // unix newlines are the only control code we respect
		else if(c>'~'||((z+1<n)&&(0xFF&x[z+1])==0xCC)){ // possible combining accent
			int f=0;for(int i=0;DROM_INKEY[i];i++){
				int ln=strlen(DROM_INKEY[i]);
				if((z+ln<=n)&&!memcmp(x+z,DROM_INKEY[i],ln)){c=DROM_INVAL[i];z+=(ln-1);f=1;break;}
			}
			if((!f)&&(c>'~')){ // neither recognized nor in the plain ASCII range
				if     ((c&0xF0)==0xF0)z+=3; // skip 4-byte codepoints
				else if((c&0xE0)==0xE0)z+=2; // skip 3-byte codepoints
				else if((c&0xC0)==0xC0)z+=1; // skip 2-byte codepoints
				c=255;                       // any unrecognized codepoint becomes UNKNOWN
			}
		}
		str_addraw(s,c);
	}
}
lv* lmutf8(char*x){str r=str_new();utf8_to_drom(&r,x,strlen(x));return lmstr(r);}
char* lmiutf8(void*x,int n){str r=str_new();utf8_to_drom(&r,(char*)x,n);return lmstr(r)->sv;}
char drom_toupper(char c){return DROM_TOUPPER[0xFF&c];}
char drom_tolower(char c){return DROM_TOLOWER[0xFF&c];}
char drom_from_codepoint(int n){
	// if we return 0, the result should be understood as 'no character'!
	if(n=='\r')return 0;if(n=='\t')return ' ';if(n<' '&&n!='\n')return 0xFF;if(n<='~')return n;
	for(int i=0;DROM_INPOINT[i];i++)if(n==DROM_INPOINT[i])return DROM_INVAL[i];return 0xFF;
}

// Primitives
dyad(l_format);

#define vm(n,op,arg) monad(a_##n){return lmn(op(arg(x)));}monad(l_##n){return perfuse(x,a_##n);}
#define vd(n)        dyad(l_##n){return conform(x,y,a_##n);}
vm(not,!  ,lb) vm(negate,-  ,ln) vm(floor,floor,ln) vm(cos,cos,ln)
vm(sin,sin,ln) vm(tan   ,tan,ln) vm(exp  ,exp  ,ln) vm(ln ,log,ln) vm(sqrt,sqrt,ln)
monad(l_count){return lmn(lin(x)||lis(x)||lil(x)||lid(x)?x->c:lit(x)?x->n:0);}
monad(l_list ){lv*r=lml(1);r->lv[0]=x;return r;}
monad(l_first){
	if(linil(x))return LNIL;
	if(lit(x))return l_first(l_rows(x));
	if(linat(x))return lmistr("native");
	if(lion(x))return lmcstr(x->sv);
	if(lis(x))return !x->c?LNIL:l_at(x,ZERO);
	lv*l=ll(x);return!l->c?LNIL:l->lv[0];
}
monad(l_last ){
	if(linil(x))return LNIL;
	if(lit(x))return l_last(l_rows(x));
	if(lis(x))return !x->c?LNIL:l_at(x,lmn(x->c-1));
	lv*l=ll(x);return!l->c?LNIL:l->lv[l->c-1];
}
monad(l_typeof){
	if(linat(x))return lmistr("function");if(linil(x))return lmistr("nil");if(lii(x))return x->a;
	char*n[]={"number","string","list","dict","table","function","INTERNAL"};return lmistr(n[MIN(6,x->t)]);
}
monad(l_keys){if(lii(x))return lml(0);if(lion(x)){MAP(r,x)x->lv[z];return r;};x=ld(x);MAP(r,x)x->kv[z];return r;}
monad(l_range){if(!lin(x))return ll(x);int n=ln(x);if(n<0)n=0;GEN(r,n)lmn(z);return r;}
monad(l_rows){x=lt(x);lv*r=lml(x->n);for(int w=0;w<x->n;w++){DMAP(t,x,x->lv[z]->lv[w]);r->lv[w]=t;}return r;}
monad(l_cols){x=lt(x);DMAP(r,x,x->lv[z]);return r;}
monad(l_ltable){
	int c=1;EACH(z,x)c&=lid(x->lv[z]);if(c){ // list-of-dicts
		lv*t=lmt(),*ok=lml(0);t->n=x->c;
		EACH(r,x)EACH(c,x->lv[r]){lv*k=x->lv[r]->kv[c];dset(t,ls(k),NULL);if(t->c>ok->c)ll_add(ok,k);};EACH(z,t)t->lv[z]=lml(x->c);
		EACH(r,x)EACH(c,t){t->lv[c]->lv[r]=dgetv(x->lv[r],ok->lv[c]);}return t;
	}
	int m=0;c=1;EACH(z,x)c&=lil(x->lv[z]),m=MAX(m,x->lv[z]->c);if(c){ // list-of-lists
		lv*t=lmt();t->n=x->c;for(int c=0;c<m;c++)dset(t,l_format(lmistr("c%i"),lmn(c)),lml(0));
		EACH(z,x)for(int c=0;c<m;c++)ll_add(t->lv[c],c>=x->lv[z]->c?LNIL:x->lv[z]->lv[c]);return t;
	}return lt(x);
}
monad(l_table){if(lid(x)){TMAP(t,x,x->lv[z]);return torect(t);}return lil(x)?l_ltable(x):lt(x);}
monad(l_tflip){
	lv*r=lmt(),*k=NULL,*ks=lmistr("key");
	int ki=dgeti(x,ks);if(ki==-1)ki=0;lv*kl=lml(0);EACH(z,x)if(z!=ki)ll_add(kl,x->kv[z]);dset(r,ks,kl);
	if(x->c){k=x->lv[ki];EACH(zz,k){lv*c=lml(0);EACH(z,x)if(z!=ki)ll_add(c,x->lv[z]->lv[zz]);dset(r,ls(k->lv[zz]),c);}}
	return torect(r);
}
monad(l_flip){
	if(lit(x))return l_tflip(x);x=ll(x);lv*r=lml(0);int w=0;EACH(z,x)w=MAX(w,lil(x->lv[z])?x->lv[z]->c:1);
	for(int i=0;i<w;i++){MAP(c,x)!lil(x->lv[z])?x->lv[z]:i<x->lv[z]->c?x->lv[z]->lv[i]:LNIL;ll_add(r,c);}
	return r;
}
monad(a_mag    ){double s=0;EACH(z,x){double v=ln(x->lv[z]);s+=v*v;};return lmn(sqrt(s));}
monad(a_heading){double a=x->c>0?ln(x->lv[0]):0,b=x->c>1?ln(x->lv[1]):0;return lmn(atan2(b,a));}
monad(a_unit   ){double n=ln(x);lv*r=lml(2);r->lv[0]=lmn(cos(n)),r->lv[1]=lmn(sin(n));return r;}
monad(l_mag    ){return nlperfuse(x,a_mag    );}
monad(l_heading){return nlperfuse(x,a_heading);}
monad(l_unit   ){return perfuse  (x,a_unit   );}
dyad(a_add ){return lmn(ln(x)+ln(y));}vd(add)
dyad(a_sub ){return lmn(ln(x)-ln(y));}vd(sub)
dyad(a_mul ){return lmn(ln(x)*ln(y));}vd(mul)
dyad(a_div ){return lmn(ln(y)==0?0:ln(x)/ln(y));}vd(div)
dyad(a_mod ){return lmn(dmod(ln(y),ln(x)));}vd(mod)
dyad(a_pow ){return lmn(pow(ln(x),ln(y)));}vd(pow)
dyad(a_less){return lmn((linil(x)||lin(x))&&(linil(y)||lin(y))? ln(x) <ln(y): strcmp(ls(x)->sv,ls(y)->sv)<0);}vd(less)
dyad(a_more){return lmn((linil(x)||lin(x))&&(linil(y)||lin(y))? ln(x) >ln(y): strcmp(ls(x)->sv,ls(y)->sv)>0);}vd(more)
dyad(a_eq  ){return lmn(lii(x)||lii(y)?x==y: linil(x)||linil(y)?x==y: lin(x)&&lin(y)? ln(x)==ln(y): !strcmp(ls(x)->sv,ls(y)->sv));}vd(eq)
dyad(a_smin){lv*a=ls(x),*b=ls(y);return strcmp(a->sv,b->sv)<0?x:y;}
dyad(a_min ){if(lin(x)||lin(y)){double a=ln(x),b=ln(y);return lmn(a<b?a:b);}return a_smin(x,y);}vd(min)
dyad(a_smax){lv*a=ls(x),*b=ls(y);return strcmp(a->sv,b->sv)>0?x:y;}
dyad(a_max ){if(lin(x)||lin(y)){double a=ln(x),b=ln(y);return lmn(a>b?a:b);}return a_smax(x,y);}vd(max)
dyad(l_unless){return linil(y)?x:y;}
dyad(l_match){return lmbool(matchr(x,y));}
dyad(l_dict){x=ll(x);lv*r=lmd();y=ll(y);EACH(z,x)dset(r,x->lv[z],z>=y->c?LNIL:y->lv[z]);return r;}
dyad(l_split){
	x=ls(x),y=ls(y);if(x->c==0)return ll(y);lv*r=lml(0);int n=0;EACH(z,y){
		int m=1;EACH(w,x)if(x->sv[w]!=y->sv[z+w]){m=0;break;}if(!m)continue;
		str s=str_new();str_add(&s,y->sv+n,z-n);ll_add(r,lmstr(s));z+=x->c-1,n=z+1;
	}if(n<=y->c){str s=str_new();str_add(&s,y->sv+n,y->c-n);ll_add(r,lmstr(s));}return r;
}
dyad(l_fuse){
	str t=str_new();x=ls(x),y=ll(y);EACH(z,y){if(z)str_addl(&t,x);str_addl(&t,ls(y->lv[z]));}
	return lmstr(t);
}
dyad(l_ina){
	if(lil(y))EACH(z,y)if(matchr(y->lv[z],x))return ONE;
	return lis(y)?lmbool(strstr(y->sv,ls(x)->sv)): lmbool((lid(y)||lit(y))&&dget(y,x));
}
size_t hash_n=0;lv**hash_v=NULL;
#define hash_t long long unsigned int
hash_t hash_key(lv*x){
	if(lin(x)){union{double f;hash_t u;}fu={.f=ln(x)};return fu.u;}
	else if(lis(x)        ){hash_t r=1;EACH(z,x)r=(31*r)+(0xFF&x->sv[z]);return r;}
	else if(lil(x)        ){hash_t r=1;EACH(z,x)r=(31*r)+hash_key(x->lv[z]);return r;}
	else if(lid(x)||lit(x)){hash_t r=1;EACH(z,x)r=(31*((31*r)+hash_key(x->lv[z])))+hash_key(x->kv[z]);return r;}
	else{return (hash_t)((uintptr_t)x);}
}
void hash_add(lv*x){hash_t i=hash_key(x)%hash_n;while(hash_v[i])i=(i+1)%hash_n;hash_v[i]=x;}
int hash_in(lv*x){hash_t i=hash_key(x)%hash_n;while(hash_v[i]){if(matchr(hash_v[i],x))return 1;i=(i+1)%hash_n;}return 0;}
dyad(l_in){
	if(lil(x)&&(lil(y)||lid(y))&&y->c>32){
		hash_n=ceil(1.3*y->c),hash_v=calloc(hash_n,sizeof(lv*));
		if(lid(y)){EACH(z,y)hash_add(y->kv[z]);}else{EACH(z,y)hash_add(y->lv[z]);}
		MAP(r,x)lmn(hash_in(x->lv[z]));return free(hash_v),r;
	}
	if(lil(x)){MAP(r,x)l_ina(x->lv[z],y);return r;}return l_ina(x,y);
}
lv*filter(int in,lv*x,lv*y){
	x=(lis(x)||linil(x))?l_list(x):ll(x);int n=1;EACH(z,x)if(!lin(x->lv[z]))n=0;
	if(lid(y)){lv*r=lmd();EACH(z,y)if(in==lb(l_ina(y->kv[z],x)))dset(r,y->kv[z],y->lv[z]);return r;}
	if(!lit(y)){lv*r=lml(0);y=ll(y);EACH(z,y)if(in==lb(l_ina(y->lv[z],x)))ll_add(r,y->lv[z]);return r;}
	if(n&&in ){lv*r=l_take(ZERO,y);EACH(i,x){int z=ln(x->lv[i]);if(z>=0&&z<y->n)EACH(c,y)ll_add(r->lv[c],y->lv[c]->lv[z]);}return torect(r);}
	if(n&&!in){lv*r=l_take(ZERO,y);for(int z=0;z<y->n;z++)if(!lb(l_ina(lmn(z),x)))EACH(c,y)ll_add(r->lv[c],y->lv[c]->lv[z]);return torect(r);}
	lv*r=lmt();EACH(z,y)if(in==lb(l_ina(y->kv[z],x)))dset(r,y->kv[z],y->lv[z]);r->n=y->n;return r;
}
dyad(l_take){
	if(!lin(x))return filter(1,x,y);if(lil(y)&&ln(x)==y->c)return y;
	if(lis(y)&&ln(x)< 0&&abs((int)ln(x))<=y->c)return lmslice(y,y->c+ln(x));
	if(lis(y)&&ln(x)>=0&&         ln(x) <=y->c){lv*r=lms(ln(x));memcpy(r->sv,y->sv,r->c);return r;}
	if(lid(y)){lv*t=l_take(x,l_range(lmn(y->c))),*r=lmd();
		EACH(z,t){int i=t->lv[z]->nv;dset(r,y->kv[i],y->lv[i]);}return r;}
	if(lis(y))return l_fuse(lmistr(""),l_take(x,ll(y)));
	if(lit(y)){TMAP(r,y,l_take(x,y->lv[z]));r->n=fabs(ln(x));return r;}
	y=ll(y);int n=y->c,m=ln(x),s=m<0?mod(m,n):0;lv*r=lml(m<0?-m:m);
	EACH(z,r)r->lv[z]=n?y->lv[mod(z+s,n)]:LNIL;return r;
}
dyad(l_drop){
	if(!lin(x))return filter(0,x,y);
	if(lis(y)&&ln(x)< 0){lv*r=lms(MAX(0,y->c+ln(x)));memcpy(r->sv,y->sv,r->c);return r;}
	if(lis(y)&&ln(x)>=0)return lmslice(y,ln(x));
	if(lis(y))return l_fuse(lmistr(""),l_drop(x,ll(y)));
	if(lit(y)){TMAP(r,y,l_drop(x,y->lv[z]));return torect(r);}
	if(lid(y)){
		lv*t=l_drop(x,l_range(lmn(y->c))),*r=lmd();
		EACH(z,t){int i=t->lv[z]->nv;dset(r,y->kv[i],y->lv[i]);}return r;
	}
	int n=ln(x);y=ll(y);if(n>0){GEN(r,MAX(0,y->c-n))y->lv[n+z];return r;}
	GEN(r,MAX(0,y->c+n))y->lv[z];return r;
}
dyad(l_limit){int n=ln(x);return ln(l_count(y))<=n?y:l_take(lmn(n),y);}
dyad(l_tcomma){
	lv*r=lmt();
	EACH(i,x){MAP(c,x->lv[i])x->lv[i]->lv[z];if(!dget(y,x->kv[i]))for(int z=0;z<y->n;z++)ll_add(c,LNIL);dset(r,x->kv[i],c);}
	EACH(i,y){lv*c=dget(r,y->kv[i]);if(!c){GEN(nc,x->n)LNIL;dset(r,y->kv[i],nc);c=nc;}EACH(z,y->lv[i])ll_add(c,y->lv[i]->lv[z]);}
	return torect(r);
}
dyad(l_comma){
	if(lit(x)&&lit(y))return l_tcomma(x,y);
	if(lid(x)){y=ld(y);DMAP(r,x,x->lv[z]);EACH(z,y)dset(r,y->kv[z],y->lv[z]);return r;}
	if(lis(x)||linil(x))return l_comma(l_list(x),y);if(lis(y)||linil(y))return l_comma(x,l_list(y));
	x=ll(x),y=ll(y);GEN(r,x->c+y->c)z<x->c?x->lv[z]:y->lv[z-x->c];return r;
}
dyad(l_cross){
	if(lin(x))x=l_range(x);if(lin(y))y=l_range(y);if(!lit(x)||!lit(y))x=ll(x),y=ll(y);
	if(lil(x)&&lil(y)){
		lv*r=lml(x->c*y->c);
		EACH(w,r){lv*p=lml(2);p->lv[0]=x->lv[w%x->c];p->lv[1]=y->lv[w/x->c];r->lv[w]=p;}
		return r;
	}
	x=lt(x),y=lt(y);lv*r=lmt();r->n=x->n*y->n;
	EACH(c,x){GEN(t,r->n)x->lv[c]->lv[z%x->n];dset  (r,x->kv[c],t);}
	EACH(c,y){GEN(t,r->n)y->lv[c]->lv[z/x->n];dsetuq(r,y->kv[c],t);}return r;
}
dyad(l_join){
	if(!lit(x)||!lit(y)){
		x=lin(x)?l_range(x):ll(x),y=lin(y)?l_range(y):ll(y);
		MAP(r,x)l_comma(x->lv[z],y->c==0?LNIL:y->lv[z%y->c]);return r;
	}
	lv*ik=lml(0),*dk=lml(0);TMAP(r,x,lml(0));EACH(z,y){
		int i=-1;FIND(w,x,y->kv[z]){i=w;break;}
		if(i>=0){ll_add(ik,y->kv[z]);}else{ll_add(dk,lmn(z)),dset(r,y->kv[z],lml(0));}
	}
	#define join_key(t,r) lv*k;if(ik->c==1){k=dget(t,ik->lv[0])->lv[r];}else{k=lml(ik->c);EACH(z,ik)k->lv[z]=dget(t,ik->lv[z])->lv[r];}
	lv*km=lmd();for(int bi=0;bi<y->n;bi++){
		join_key(y,bi);lv*ix=dget(km,k);if(ix){ll_add(ix,lmn(bi));}else{dset(km,k,l_list(lmn(bi)));}
	}
	for(int ai=0;ai<x->n;ai++){
		join_key(x,ai);lv*ix=dget(km,k);if(ix)EACH(ii,ix){int bi=ln(ix->lv[ii]);
			EACH(z,x )ll_add(r->lv[z     ],x->lv[z                   ]->lv[ai]);
			EACH(z,dk)ll_add(r->lv[x->c+z],y->lv[(int)(dk->lv[z]->nv)]->lv[bi]);r->n++;
		}
	}return r;
}
monad(l_sum ){x=ll(x);lv*r=ZERO      ;for(int z=0;z<x->c;z++)r=l_add  (r,x->lv[z]);return r;}
monad(l_prod){x=ll(x);lv*r=ONE       ;for(int z=0;z<x->c;z++)r=l_mul  (r,x->lv[z]);return r;}
monad(l_amax){x=ll(x);lv*r=l_first(x);for(int z=1;z<x->c;z++)r=l_max  (r,x->lv[z]);return r;}
monad(l_amin){x=ll(x);lv*r=l_first(x);for(int z=1;z<x->c;z++)r=l_min  (r,x->lv[z]);return r;}
monad(l_raze){if(lit(x))return l_dict(x->c?x->lv[0]:lml(0), x->c>1?x->lv[1]:lml(0));
	          x=ll(x);lv*r=l_first(x);for(int z=1;z<x->c;z++)r=l_comma(r,x->lv[z]);return r;}

char esc(char e,int*i,char*t,int*n){
	if(e=='n')return '\n';if(strchr("\\\"/'",e))return e;if(e!='u'||*n<4)return ' ';
	char h[5]={0};memcpy(h,t+*i,4),(*i)+=4,(*n)-=4;return drom_from_codepoint(0xFFFF&strtol(h,NULL,16));
}
lv* pjson(char*t,int*i,int*f,int*n){
	#define jc()    (*n&&*f?t[*i]:0)
	#define jn()    (*n&&*f?(--*n,t[(*i)++]):0)
	#define jm(x)   jc()==x?(jn(),1):0
	#define js()    while(isspace(jc()))jn();
	#define jd()    while(isdigit(jc()))jn();
	#define jl(x,y) if((*n)>=(int)strlen(x)&&memcmp(t+*i,x,strlen(x))==0)return(*i)+=strlen(x),(*n)-=strlen(x),y;
	jl("null",LNIL);jl("false",ZERO);jl("true",ONE);
	if(jm('[')){lv*r=lml(0);while(jc()){js();if(jm(']'))break;ll_add(r,pjson(t,i,f,n));js();jm(',');}return r;}
	if(jm('{')){lv*r=lmd( );while(jc()){js();if(jm('}'))break;lv*k=pjson(t,i,f,n);js();jm(':');js();if(*f)dset(r,k,pjson(t,i,f,n));js();jm(',');}return r;}
	if(jm('"' )){str r=str_new();while(jc()&&!(jm('"' ))){char tmp=(jm('\\'))?esc(jn(),i,t,n):jn();if(tmp)str_addc(&r,tmp);}return lmstr(r);}
	if(jm('\'')){str r=str_new();while(jc()&&!(jm('\''))){char tmp=(jm('\\'))?esc(jn(),i,t,n):jn();if(tmp)str_addc(&r,tmp);}return lmstr(r);}
	int ns=*i;jm('-');jd();jm('.');jd();if(jm('e')||jm('E')){jm('-')||jm('+');jd();}if(*i<=ns){*f=0;return LNIL;}
	char tb[NUM];snprintf(tb,MIN(*i-ns+1,NUM),"%s",t+ns);return lmn(atof(tb));
}
lv* plove(char*t,int*i,int*f,int*n){
	if(jm('[')){lv*r=lml(0);while(jc()){js();if(jm(']'))break;ll_add(r,plove(t,i,f,n));js();jm(',');}return r;}
	if(jm('{')){lv*r=lmd( );while(jc()){js();if(jm('}'))break;lv*k=plove(t,i,f,n);js();jm(':');js();if(*f)dset(r,k,plove(t,i,f,n));js();jm(',');}return r;}
	if(jm('<')){lv*r=lmd( );while(jc()){js();if(jm('>'))break;lv*k=plove(t,i,f,n);js();jm(':');js();if(*f)dset(r,ls(k),ll(plove(t,i,f,n)));js();jm(',');}return l_table(r);}
	if(jm('%')){jm('%');str r=str_new();str_addz(&r,"%%");while(jc()&&(isalnum(jc())||strchr("+/=",jc())))str_addc(&r,jn());return idecode(lmstr(r));}
	return pjson(t,i,f,n);
}
int cumulative_month_days[12]={0,31,59,90,120,151,181,212,243,273,304,334};
int leap_year(int year){return year%400==0?1: year%100==0?0: year%4==0?1: 0;}
time_t parts_to_epoch(struct tm *p){
	return p->tm_sec+p->tm_min*60+p->tm_hour*3600+
	       (cumulative_month_days[p->tm_mon]+(p->tm_mon>=1&&leap_year(p->tm_year+1900)))*86400+
	       (p->tm_mday-1)*86400+(p->tm_year-70)*31536000+((p->tm_year-69)/4)*86400-((p->tm_year-1)/100)*86400+((p->tm_year+299)/400)*86400;
}
int format_has_names(lv*x){
	#define fc x->sv[f]
	int f=0;while(fc){
		if(fc!='%'){f++;continue;}f++;if(fc=='[')return 1;
		if(fc=='*')f++;if(fc=='-')f++;if(fc=='0')f++;while(isdigit(fc))f++;if(fc=='.')f++;
		int d=0;while(isdigit(fc))d=d*10+fc-'0',f++;if(!fc)break;char t=fc;f++;if(t=='r'||t=='o')while(d&&fc)d--,f++;
	}return 0;
}
char*ncc; // forward ref
dyad(l_parse){
	if(lil(y)){MAP(r,y)l_parse(x,y->lv[z]);return r;}
	#define hc y->sv[h]
	#define hn m&&hc&&(n?h-si<n:1)
	#define ulc(x) t=='l'?drom_tolower(x):t=='u'?drom_toupper(x):x
	x=ls(x),y=ls(y);int f=0,h=0,m=1,pi=0,named=format_has_names(x);lv*r=named?lmd():lml(0);while(fc){
		if(fc!='%'){if(m&&fc==hc){h++;}else{m=0;}f++;continue;}f++;
		str nk={0};if(fc=='['){f++;nk=str_new();while(fc&&fc!=']')str_addc(&nk,fc),f++;if(fc==']')f++;}
		int im=m,n=0,d=0,si=h,sk=fc=='*'&&(f++,1),lf=fc=='-'&&(f++,1);if(fc=='0')f++;
		while(isdigit(fc))n=n*10+fc-'0',f++;if(fc=='.')f++;
		while(isdigit(fc))d=d*10+fc-'0',f++;if(!fc)break;char t=fc;f++;
		if(!strchr("%mnzsluqarojJ",t))while(hn&&isspace(hc))h++;lv*v=NULL;
		if     (t=='%'){if(m&&t==hc){h++;}else{m=0;}}
		else if(t=='m')v=lmbool(m);
		else if(t=='n')v=lmn(h);
		else if(t=='z')v=lmbool(m&&h==y->c);
		else if(t=='i'){long long r=0,s=hc=='-'?(h++,-1):1;m&=!!isdigit(hc);while(hn&&isdigit(hc))r=r*10+hc-'0',h++;v=lmn(r*s);}
		else if(t=='h'||t=='H'){long long r=0;m&=!!isxdigit(hc);while(hn&&isxdigit(hc))r=r*16+(hc>'9'?drom_tolower(hc)-'a'+10:hc-'0'),h++;v=lmn(r);}
		else if(strchr("slu",t)){str r=str_new();while(hn&&(n?1:hc!=fc))str_addc(&r,ulc(hc)),h++;v=lmstr(r);}
		else if(t=='a'){v=lml(0);while(hn&&(n?1:hc!=fc))ll_add(v,lmn(0xFF&hc)),h++;}
		else if(t=='b'){v=lmbool(strchr("tTyYx1",hc));while(hn&&n?1:hc!=fc)h++;}
		else if(t=='j'){int f=1,c=n?n:y->c;v=m?pjson(y->sv,&h,&f,&c):LNIL;}
		else if(t=='J'){int f=1,c=n?n:y->c;v=m?plove(y->sv,&h,&f,&c):LNIL;}
		else if(t=='v'){str r=str_new();m&=!isdigit(hc);while(hn&&hc!='\n'&&ncc[hc-32]=='n')str_addc(&r,hc),h++;v=lmstr(r);}
		else if(t=='q'){
			str r=str_new();m&=hc=='"';if(m)h++;while(hn&&hc!='"'){
				if(hc=='\\'){h++;if(m&=!!strchr("\\\"n",hc)){str_addc(&r,hc=='n'?'\n':hc);}}else{str_addc(&r,hc);}h++;
			}if(m&=hc=='"')h++;v=lmstr(r);if(!m)v=LNIL;
		}
		else if(t=='r'||t=='o'){
			str r=str_new();d=MAX(1,d);
			int cc=f;for(int z=0;m&&z<d;z++){if(!fc){m=0;}else{f++;}}while(hn){
				int mc=0;for(int z=0;z<d;z++)if(hc==x->sv[cc+z])mc=1;
				if(mc==lf?1:0){if(n)m=0;break;}str_addc(&r,hc);h++;if(t=='o')break;
			}if(!m)r.c=0;v=lmstr(r);
		}
		else if(t=='f'||t=='c'||t=='C'){
			double r=0,p=10,s=hc=='-'?(h++,-1):1;if(t=='c'&&m&&hc=='$')h++;
			m&=!!isdigit(hc)||hc=='.';while(hn&&isdigit(hc))r=r*10+hc-'0',h++;
			if(hn&&hc=='.')h++;while(hn&&isdigit(hc))r+=(hc-'0')/p,p*=10,h++;v=lmn(r*s);
		}
		else if(t=='e'||t=='p'){
			struct tm tm={0};if(m){
				#define pf(f,o,i) tm.tm_##f=ln(e->lv[i])-o
				lv*e=l_parse(lmistr("%04i-%02i-%02iT%02i:%02i:%02iZ%n%m"),lmcstr(y->sv+h));
				pf(year,1900,0),pf(mon,1,1),pf(mday,0,2),pf(hour,0,3),pf(min,0,4),pf(sec,0,5);
				if(lb(e->lv[7])){h+=ln(e->lv[6]);}else{m=0;}
			}
			#define ps(x,f,o) dset(v,lmcstr(x),lmn(m?tm.tm_##f+o:0));
			if(t=='e'){v=lmn(m?parts_to_epoch(&tm):0);}
			else{v=lmd();ps("year",year,1900)ps("month",mon,1)ps("day",mday,0)ps("hour",hour,0)ps("minute",min,0)ps("second",sec,0)}
		}
		else{m=0;}while(n&&hc&&h-si<n)h++,m=0;
		if(!sk&&v){
			if     (!im&&!strchr("%mnz",t))v=LNIL;             // some previous pattern failed
			else if((h-si)==0&&strchr("fcCihHv",t))v=LNIL,m=0; // no characters consumed
			named?dset(r,nk.sv?lmstr(nk):lmn(pi),v):ll_add(r,v);pi++;
		}
	}return named?r: r->c==1?r->lv[0]:r;
}
void fjson(str*s,lv*x){
	if(lin(x)){wnum(s,x->nv);}
	else if(lit(x)){fjson(s,l_rows(x));}
	#define wrap(a,b,c) str_addc(s,a);EACH(z,x)c;str_addc(s,b);
	else if(lil(x)){wrap('[',']',{if(z)str_addc(s,',');fjson(s,x->lv[z]);})}
	else if(lid(x)){wrap('{','}',{if(z)str_addc(s,',');fjson(s,ls(x->kv[z]));str_addc(s,':');fjson(s,x->lv[z]);})}
	else if(lis(x)){
		str_addc(s,'"');int ct=0;EACH(z,x){
			char c=x->sv[z],e=0;if(c=='<'){ct=1;}else if(c=='/'&&ct){e=1;}else if(c!=' '&&c!='\n'){ct=0;}
			if(c=='\n'?(c='n',1):e||!!strchr("\"\\",c))str_addc(s,'\\');str_addc(s,c);
		}str_addc(s,'"');
	}else{str_addz(s,"null");}
}
void flove(str*s,lv*x){
	if(lin(x)||lis(x)){fjson(s,x);}
	else if(lil(x)){wrap('[',']',{if(z)str_addc(s,',');flove(s,x->lv[z]);})}
	else if(lid(x)){wrap('{','}',{if(z)str_addc(s,',');flove(s,x->kv[z]);str_addc(s,':');flove(s,x->lv[z]);})}
	else if(lit(x)){wrap('<','>',{if(z)str_addc(s,',');flove(s,x->kv[z]);str_addc(s,':');flove(s,x->lv[z]);})}
	else if(lii(x)){lv*t=((lv*(*)(lv*,lv*,lv*))x->f)(x,lmistr("encoded"),NULL);str_addl(s,linil(t)?lmistr("null"):ls(t));}
	else{str_addz(s,"null");}
}
void format_type(str*r,lv*a,char t,int n,int d,int lf,int pz,int*f,char*c){
	char o[NUM]={0},*op=o;
	if     (t=='%')snprintf(o,NUM,"%%");
	else if(t=='s'||t=='l'||t=='u'||t=='v'){op=ls(a)->sv;}
	else if(t=='r'||t=='o'){op=ls(a)->sv,lf=1;d=MAX(1,d);while(d&&c[*f])d--,(*f)++;d=n;}
	else if(t=='a'){str v=str_new();lv*l=ll(a);EACH(z,l)str_addc(&v,0xFF&((int)ln(l->lv[z])));op=lmstr(v)->sv;}
	else if(t=='b')snprintf(o,NUM,"%s",lb(a)?"true":"false");
	else if(t=='f'){if(d){snprintf(o,NUM,"%.*f",d,ln(a));}else{str v=str_new();wnum(&v,ln(a));op=lmstr(v)->sv;}}
	else if(t=='c'){double v=ln(a);snprintf(o,NUM,"%s$%.*f",v<0?"-":"",d?d:2,fabs(v));}
	else if(t=='C'){double v=ln(a);snprintf(o,NUM,"%s%.*f" ,v<0?"-":"",d?d:2,fabs(v));}
	else if(t=='i')snprintf(o,NUM,"%lld",(long long)ln(a));
	else if(t=='h')snprintf(o,NUM,"%llx",(long long)ln(a));
	else if(t=='H')snprintf(o,NUM,"%llX",(long long)ln(a));
	else if(t=='j'){str v=str_new();fjson(&v,a    );op=lmstr(v)->sv;}
	else if(t=='J'){str v=str_new();flove(&v,a    );op=lmstr(v)->sv;}
	else if(t=='q'){str v=str_new();fjson(&v,ls(a));op=lmstr(v)->sv;}
	else if(t=='e'){time_t v=ln(a);strftime(o,NUM,"%FT%TZ",gmtime(&v));}
	else if(t=='p'){
		lv*isodate=lmistr("%[year]04i-%[month]02i-%[day]02iT%[hour]02i:%[minute]02i:%[second]02iZ%n%m");
		snprintf(o,NUM,"%s",l_format(isodate,ld(a))->sv);
	}
	int vn=strlen(op); if(d&&strchr("fcC",t))d=0; if(d&&lf)vn=MIN(d,vn);
	if(n&&!lf)for(int z=0;z<n-vn;z++)str_addc(r,pz?'0':' ');
	for(int z=d&&!lf?MAX(0,vn-d):0;z<vn;z++)str_addc(r,ulc(op[z]));
	if(n&&lf)for(int z=0;z<n-vn;z++)str_addc(r,pz?'0':' ');
}
void format_type_simple(str*r,lv*value,char t){int f=0;format_type(r,value,t,0,0,0,0,&f,"");}
lv* format_rec(int i,lv*x,lv*y){
	if(i>=x->c)return y;
	int fuse=(x->c-i)%2?0:1,named=format_has_names(ls(x->lv[i+fuse]));lv*a=lit(y)?l_rows(y):ll(y);
	MAP(r,a)l_format(x->lv[i+fuse],format_rec(i+fuse+1,x,lit(y)&&!named?ll(a->lv[z]):a->lv[z]));
	return fuse?l_fuse(x->lv[i],r):r;
}
dyad(l_format){
	if(lil(x))return format_rec(0,x,y);
	str r=str_new();x=ls(x);int f=0,h=0,named=format_has_names(x);y=named?ld(y):lil(y)?y:l_list(y);while(fc){
		if(fc!='%'){str_addc(&r,fc),f++;continue;}f++;
		str nk={0};if(fc=='['){f++;nk=str_new();while(fc&&fc!=']')str_addc(&nk,fc),f++;if(fc==']')f++;}
		int n=0,d=0,sk=fc=='*'&&(f++,1),lf=fc=='-'&&(f++,1),pz=fc=='0'&&(f++,1);
		while(isdigit(fc))n=n*10+fc-'0',f++;if(fc=='.')f++;
		while(isdigit(fc))d=d*10+fc-'0',f++;if(!fc)break;char t=fc;f++;
		lv*an=named?dget(y,nk.sv?lmstr(nk):lmn(h)): NULL;
		lv*a=t=='%'?LNIL: named?(an?an:LNIL): (!sk&&h<y->c)?y->lv[h]: LNIL;
		format_type(&r,a,t,n,d,lf,pz,&f,x->sv);if(t!='%'&&!sk)h++;
	}return lmstr(r);
}
lv*like_test(lv*str,lv*pats){
	EACH(z,pats){
		lv*p=pats->lv[z];char*m=p->lv[0]->sv,*l=p->lv[1]->sv,*a=p->lv[2]->sv;int sc=p->lv[0]->c;
		if(!sc&&!str->c){return ONE;}else if(!sc)continue; // an empty pattern matches only the empty string
		memset(a,0,sc);a[0]=m[0]=='*';for(int ci=0;ci<str->c;ci++){
			char c=str->sv[ci];for(int si=sc-1;si>=0;si--){ // iterate backwards so we can update alive states in-place
				int prev=(si>0&&a[si-1])||(si==0&&ci==0)||(si>1&&m[si-1]=='*'&&a[si-2]);
				a[si]=m[si]=='*'?a[si]||prev: m[si]=='.'?prev: m[si]=='#'?isdigit(c)&&prev: c==l[si]&&prev;
			}
		}if(a[sc-1]||(sc>1&&m[sc-1]=='*'&&a[sc-2]))return ONE;
	}return ZERO;
}
dyad(l_like){
	if(!lil(y))y=l_list(y);lv*pats=lml(y->c);EACH(z,pats){
		lv*p=ls(y->lv[z]),*r=lml(3);r->lv[0]=lms(p->c),r->lv[1]=lms(p->c),r->lv[2]=lms(p->c);pats->lv[z]=r;// {mode,literal,alive?}
		char*m=r->lv[0]->sv,*l=r->lv[1]->sv;
		int s=0;EACH(i,p){
			char c=p->sv[i];m[s]=c=='`'&&i<p->c-1?(l[s]=p->sv[++i],'a'): strchr(".*#",c)?(l[s]='!',c): (l[s]=c,'a');
			while(p->sv[i]=='*'&&p->sv[i+1]=='*')i++;s++; // collapse sequential *s into one(!)
		}r->lv[0]->c=s;
	}if(lil(x)){MAP(r,x)like_test(ls(x->lv[z]),pats);return r;}else{return like_test(ls(x),pats);}
}
dyad(l_window){
	int n=ln(x);lv*r=lml(0);if(lis(y)){
		if(n>0){     for(int z=0;z    <y->c;z+=n){lv*t=lms(MIN(n,y->c-z));ll_add(r,t);memcpy(t->sv,y->sv+z,t->c);}}
		if(n<0){n=-n;for(int z=0;z+n-1<y->c;z++ ){lv*t=lms(    n        );ll_add(r,t);memcpy(t->sv,y->sv+z,t->c);}}
	}else{y=ll(y);
		if(n>0){     for(int z=0;z    <y->c;z+=n){lv*t=lml(0);ll_add(r,t);for(int i=0;i<n&&z+i<y->c;i++)ll_add(t,y->lv[z+i]);}}
		if(n<0){n=-n;for(int z=0;z+n-1<y->c;z++ ){lv*t=lml(0);ll_add(r,t);for(int i=0;i<n          ;i++)ll_add(t,y->lv[z+i]);}}
	}return r;
}
dyad(l_fill){
	if(lil(y)){MAP(r,y)l_fill(x,y->lv[z]);return r;}
	if(lid(y)){DMAP(r,y,l_fill(x,y->lv[z]));return r;}
	if(lit(y)){TMAP(r,y,l_fill(x,y->lv[z]));torect(r);return r;}
	return linil(y)?x: y;
}
lv* l_ins(lv*v,lv*n,lv*x){
	int rc=ceil((1.0*v->c)/n->c);lv*c=lml(n->c);
	EACH(z,c){c->lv[z]=lml(rc);for(int r=0;r<rc;r++){int x=(n->c*r)+z;c->lv[z]->lv[r]=x>=v->c?LNIL:v->lv[x];}}
	lv*r=l_table(l_dict(n,c));return lin(x)?r:l_comma(lt(x),r);
}
lv* l_tab(lv*t){
	t=lt(t);TMAP(r,t,t->lv[z]);torect(r);
	dset(r,lmistr("index" ),l_range(lmn(r->n))),dset(r,lmistr("gindex"),l_range(lmn(r->n))),dset(r,lmistr("group" ),l_take(lmn(r->n),ZERO));
	return r;
}
lv* merge(lv*vals,lv*keys,int widen,lv**ix){
	lv*i=lmistr("@index");
	if(!widen){*ix=lml(0);EACH(z,vals){lv*x=dget(vals->lv[z],i);EACH(z,x)ll_add(*ix,x->lv[z]);}}
	if(widen){lv*t=lml(0);EACH(z,vals)if(dget(vals->lv[z],i)->c)ll_add(t,vals->lv[z]);vals=t;}
	if(vals->c==0){lv*d=lmd();EACH(z,keys)dset(d,keys->lv[z],lml(0));ll_add(vals,d);}
	GEN(r,vals->c)l_table(widen?vals->lv[z]:l_drop(i,vals->lv[z]));r=l_raze(r);
	if(widen){*ix=dget(r,i);r=l_drop(i,r);}return r;
}
lv* disclose(lv*x){lv*t=lml(3);t->lv[0]=lmistr("index"),t->lv[1]=lmistr("gindex"),t->lv[2]=lmistr("group");return l_drop(t,x);}
lv* l_select(lv*orig,lv*vals,lv*keys){lv*ix=NULL,*r=merge(vals,keys,0,&ix);return keys->c>1?r:l_take(ix,disclose(orig));}
lv* l_extract(lv*orig,lv*vals,lv*keys){
	lv*r=l_cols(l_select(orig,vals,keys));
	return keys->c==1?(r->c?l_first(r):lml(0)): (r->c!=1||r->kv[0]->c)?r: l_first(r);
}
lv* l_update(lv*orig,lv*vals,lv*keys){
	orig=disclose(orig);lv*ix=NULL,*r=merge(vals,keys,1,&ix);EACH(c,r){
		if(r->lv[c]==ix)continue;lv*k=r->kv[c];
		int ci=dgeti(orig,k);GEN(col,orig->n)ci==-1?LNIL:orig->lv[ci]->lv[z];dset(orig,k,col);
		EACH(row,ix){col->lv[(int)ln(ix->lv[row])]=r->lv[c]->lv[row];}
	}return orig;
}
lv* l_where(lv*col,lv*tab){
	lv*w=l_take(lmn(tab->n),ll(col)),*p=lml(0);EACH(z,w)if(lb(w->lv[z]))ll_add(p,lmn(z));
	lv*r=l_take(p,tab);dset(r,lmistr("gindex"),l_range(lmn(r->n)));return r;
}
lv* l_by(lv*col,lv*tab){
	lv*b=l_take(lmn(tab->n),ll(col)),*u=lmd(),*gi=lmistr("gindex"),*gr=lmistr("group");
	EACH(row,b){
		int ki=dgeti(u,b->lv[row]);if(ki==-1){TMAP(nt,tab,lml(0));ki=u->c,dset(u,b->lv[row],nt);}
		lv*t=u->lv[ki];EACH(col,tab)ll_add(t->lv[col],tab->lv[col]->lv[row]);
		dget(t,gi)->lv[t->n]=lmn(t->n);dget(t,gr)->lv[t->n]=lmn(ki);t->n++;
	}return ll(u);
}
lv*order_vec=NULL;int order_dir=0; // this is gross. qsort() is badly designed, and qsort_r is unportable.
int lex_less(lv*a,lv*b);int lex_more(lv*a,lv*b);// forward refs
int lex_list(lv*x,lv*y,int a,int ix){
	if(x->c<ix&&y->c<ix)return 0;lv*xv=x->c>ix?x->lv[ix]:LNIL,*yv=y->c>ix?y->lv[ix]:LNIL;
	return lex_less(xv,yv)?a: lex_more(xv,yv)?!a: lex_list(x,y,a,ix+1);
}
int lex_less(lv*a,lv*b){return lil(a)&&lil(b)? lex_list(a,b,1,0): lb(l_less(a,b));}
int lex_more(lv*a,lv*b){return lil(a)&&lil(b)? lex_list(a,b,0,0): lb(l_more(a,b));}
int orderby(const void*av,const void*bv){
	int a=ln(*(lv**)av),b=ln(*(lv**)bv);
	if(lex_less(order_vec->lv[a],order_vec->lv[b]))return  order_dir;
	if(lex_more(order_vec->lv[a],order_vec->lv[b]))return -order_dir;
	return a-b; // produce a stable sort
}
lv* l_orderby(lv*col,lv*tab,lv*dir){
	order_vec=l_take(lmn(tab->n),ll(col)),order_dir=ln(dir);
	lv*p=l_range(lmn(order_vec->c));qsort(p->lv,p->c,sizeof(lv*),orderby);
	lv*r=l_take(p,tab);dset(r,lmistr("gindex"),l_range(lmn(r->n)));return r;
}

#define prim(n,f) {n,(void*)f}
primitive monads[]={
	prim("-",l_negate),prim("!",l_not),prim("floor",l_floor),prim("cos",l_cos),prim("sin",l_sin),
	prim("tan",l_tan),prim("exp",l_exp),prim("ln",l_ln),prim("sqrt",l_sqrt),
	prim("sum",l_sum),prim("prod",l_prod),prim("raze",l_raze),prim("max",l_amax),prim("min",l_amin),
	prim("count",l_count),prim("first",l_first),prim("last",l_last),prim("flip",l_flip),
	prim("range",l_range),prim("keys",l_keys),prim("list",l_list),prim("rows",l_rows),
	prim("cols",l_cols),prim("table",l_table),prim("typeof",l_typeof),prim("@tab",l_tab),
	prim("mag",l_mag),prim("heading",l_heading),prim("unit",l_unit),prim("",NULL)
};
primitive dyads[]={
	prim("+",l_add),prim("-",l_sub),prim("*",l_mul),prim("/",l_div),prim("%",l_mod),
	prim("^",l_pow),prim("<",l_less),prim(">",l_more),prim("=",l_eq),prim("&",l_min),
	prim("|",l_max),prim("~",l_match),prim("split",l_split),prim("fuse",l_fuse),
	prim("dict",l_dict),prim("take",l_take),prim("drop",l_drop),prim("in",l_in),
	prim(",",l_comma),prim("join",l_join),prim("cross",l_cross),prim("parse",l_parse),
	prim("format",l_format),prim("unless",l_unless),prim("limit",l_limit),
	prim("like",l_like),prim("window",l_window),prim("fill",l_fill),
	prim("@where",l_where),prim("@by",l_by),prim("",NULL)
};
primitive triads[]={
	prim("@sel",l_select),prim("@ext",l_extract),prim("@upd",l_update),prim("@ins",l_ins),
	prim("@orderby",l_orderby),prim("",NULL)
};

// Bytecode

int findop(char*n,primitive*p){if(n)for(int z=0;p[z].name[0];z++)if(!strcmp(n,p[z].name))return z;return -1;}
int tnames=0;lv* tempname(void){char t[64];snprintf(t,sizeof(t),"@t%d",tnames++);return lmcstr(t);}
enum opcodes {JUMP,JUMPF,LIT,DUP,DROP,SWAP,OVER,BUND,OP1,OP2,OP3,GET,SET,LOC,AMEND,TAIL,CALL,BIND,ITER,EACH,NEXT,COL,IPRE,IPOST,FIDX,FMAP};
int oplens[]={3   ,3    ,3  ,1  ,1   ,1   ,1   ,3   ,3  ,3  ,3  ,3  ,3  ,3  ,3    ,1   ,1   ,1   ,1   ,3   ,3   ,1  ,3   ,3    ,3   ,3   };
void blk_addb(lv*x,int n){
	if(x->ns<x->n+1)x->sv=realloc(x->sv,(x->ns*=2)*sizeof(int));x->sv[x->n++]=n;
	if(x->n>=65536||x->c>=65536)printf("TOO MUCH BYTECODE!\n"),exit(1);
}
int  blk_here(lv*x){return x->n;}
void blk_setb(lv*x,int i,int n){x->sv[i]=n&0xFF;}
int  blk_getb(lv*x,int i){return 0xFF&(x->sv[i]);}
void blk_adds(lv*x,int n){blk_addb(x,0xFF&(n>>8)),blk_addb(x,0xFF&n);}
void blk_sets(lv*x,int i,int n){blk_setb(x,i,n>>8),blk_setb(x,i+1,n);}
int  blk_gets(lv*x,int i){return 0xFFFF&(blk_getb(x,i)<<8|blk_getb(x,i+1));}
void blk_op  (lv*x,int o){blk_addb(x,o);if(o==COL)blk_addb(x,SWAP);}
int  blk_opa (lv*x,int o,int i){blk_addb(x,o),blk_adds(x,i);return blk_here(x)-2;}
void blk_imm (lv*x,int o,lv*k){int i=-1;EACH(z,x)if(matchr(x->lv[z],k))i=z;if(i==-1)i=x->c,ll_add(x,k);blk_opa(x,o,i);}
#define blk_op1(x,n) blk_opa(x,OP1,findop(n,monads))
#define blk_op2(x,n) blk_opa(x,OP2,findop(n,dyads ))
#define blk_op3(x,n) blk_opa(x,OP3,findop(n,triads))
#define blk_lit(x,v)    blk_imm(x,LIT,v)
#define blk_set(x,n)    blk_imm(x,SET,n)
#define blk_loc(x,n)    blk_imm(x,LOC,n)
#define blk_get(x,n)    blk_imm(x,GET,n)
lv*  blk_getimm(lv*x,int i){return x->lv[i];}
void blk_cat(lv*x,lv*y){
	int z=0,base=blk_here(x);while(z<blk_here(y)){
		int b=blk_getb(y,z);if(b==LIT||b==GET||b==SET||b==LOC||b==AMEND){blk_imm(x,b,blk_getimm(y,blk_gets(y,z+1)));}
		else if(b==JUMP||b==JUMPF||b==EACH||b==NEXT||b==FIDX){blk_opa(x,b,blk_gets(y,z+1)+base);}
		else{for(int i=0;i<oplens[b];i++)blk_addb(x,blk_getb(y,z+i));}z+=oplens[b];
	}
}
void blk_loop(lv*b,lv*names,lv*body){
	blk_op(b,ITER);int head=blk_here(b);blk_lit(b,names);int each=blk_opa(b,EACH,0);
	blk_cat(b,body),blk_opa(b,NEXT,head),blk_sets(b,each,blk_here(b));
}
lv* blk_end(lv*x){
	int z=0;while(z<blk_here(x)){
		int b=blk_getb(x,z);z+=oplens[b];if(b!=CALL)continue;
		int t=1,i=z;while(i<blk_here(x)){
			if(blk_getb(x,i)!=JUMP){t=0;break;}
			int a=blk_gets(x,i+1);if(a<=i){t=0;break;}i=a;
		}if(t)blk_setb(x,z-1,TAIL);
	}return x;
}

// Parser

typedef struct{int row,col,a,b;char type;double nv;}token;
typedef struct{int i,r,c,tl;char*text;token here,next;char error[1024];}parser;parser par;
#define init_tok(x,v) (x->type=v,x->row=par.r,x->col=par.c)
#define perr()        par.error[0]
//         ! "#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
char*tcc=" s\" sss ()ssssdsdddddddddd: sssn@nnnnnnnnnnnnnnnnnnnnnnnnnn[ ]sn nnnnnnnnnnnnnnnnnnnnnnnnnn s s";
char*ncc= "                nnnnnnnnnn     n nnnnnnnnnnnnnnnnnnnnnnnnnn    n nnnnnnnnnnnnnnnnnnnnnnnnnn    ";
char*mcc= "     xx x xxxx x          x xxx x                          x  x                             x x";
char tc(void){return (par.i>=par.tl)?' ':par.text[par.i];}
char ccc(void){char x=tc();return x>=32&&x<=126?tcc[x-32]:' ';}
char ccn(void){char x=tc();return x>=32&&x<=126?ncc[x-32]:' ';}
int  iw(void){char x=tc();return x==' '||x=='\t'||x=='\n'||x=='#';}
int mprev(void){if(par.i<1)return 0;char x=par.text[par.i-1];return x>=32&&x<=126?mcc[x-32]=='x':0;}
char nc(void){
	if(par.i>=par.tl)return' ';
	char x=par.text[par.i++];x=='\n'?(par.r++,par.c=0):(par.c++);return x;
}
void num(token*r,char x,int sign){
	if(x=='.'&&!isdigit(tc())){init_tok(r,'.');return;}
	int len=0;double v=sign*rnum_len(par.text+(par.i-1),NUM,&len);for(int z=0;z<len-1;z++)nc();
	init_tok(r,'d'),r->nv=v;
}
void tok(token*r){
	if(perr()){init_tok(r,'e');return;}int w=par.i==0||iw()||mprev();
	while(par.i<par.tl&&iw())if(tc()=='#')while(par.i<par.tl&&tc()!='\n')nc();else nc();
	if(par.i>=par.tl){init_tok(r,'e');return;}
	char cc=ccc(),x=nc();if(cc==' '){
		snprintf(par.error,sizeof(par.error),"Invalid character '%c'.",x);init_tok(r,'e');return;
	}
	if(cc=='"'){
		init_tok(r,'s'),r->a=par.i;
		while(!perr()&&par.i<par.tl&&(x=nc())!='"'){
			if(x!='\\')continue;x=nc();if(x=='\\'||x=='"'||x=='n')continue;
			snprintf(par.error,sizeof(par.error),"Invalid escape character '\\%c' in string.",x);break;
		}r->b=par.i-1;return;
	}
	if(x=='-'&&w&&ccc()=='d'){num(r,nc(),-1);return;}
	if(cc=='n'){init_tok(r,'n');r->a=par.i-1;while(ccn()=='n')nc();r->b=par.i;return;}
	if(cc=='s'){init_tok(r,'m');r->a=par.i-1,r->b=par.i;return;}
	if(cc=='d'){num(r,x,1);return;}
	init_tok(r,x);
}
str token_str(token*t){
	str r=str_new();
	if(t->a==0&&t->b==0){str_addc(&r,t->type);}else{str_add(&r,par.text+t->a,t->b-t->a);}
	return str_term(&r),r;
}
str literal_str(token*t){
	str r=str_new();for(int z=t->a;z<t->b;z++){
		if(par.text[z]=='\\'){
			char x=par.text[++z];str_addc(&r,x=='\\'?x: x=='"'?x: x=='n'?'\n':' ');
		}else{str_addc(&r,par.text[z]);}
	}return r;
}
token* next(void){
	if(par.next.type){par.here=par.next;par.next.type=0;}else{tok(&par.here);}
	return &par.here;
}
token* peek(void){if(!par.next.type)tok(&par.next);return &par.next;}
token peek2(void){parser t=par;next();token r=*peek();par=t;return r;}
int hasnext(void){return !perr()&&((par.next.type&&par.next.type!='e')||peek()->type!='e');}
int matchsp(char x){return perr()?0: peek()->type==x?(next(),1):0;}
int matchp(char*x){
	if(perr()||peek()->type!='n')return 0;int r=peek()->b-peek()->a;
	for(int z=0;z<r;z++)if(x[z]!=par.text[z+peek()->a])return 0;return x[r]?0:1;
}
int match(char*x){return matchp(x)?(next(),1):0;}
#define tsname(x) x=='e'?"the end of the script":x=='d'?"number":x=='n'?"name":x=='s'?"string":x=='m'?"symbol"
str expect(char t,int alloc){
	if(peek()->type==t)return alloc?token_str(next()):(next(),NOSTR);
	if(perr())return alloc?str_new():NOSTR;
	char                nn[2]={t,'\0'}, *sname=tsname(t):nn;
	char x=peek()->type, n[2]={x,'\0'}, *tname=tsname(x):n;
	snprintf(par.error,sizeof(par.error),"Expected %s, but found %s.",sname,tname);
	return alloc?str_new():NOSTR;
}
int ident(char*n){
	char*kws[]={
		"while","each","send","on","if","elseif","else","end","do","with","local",
		"select","extract","update","insert","into","from","where","by","orderby","asc","desc",
	};for(size_t z=0;z<(sizeof(kws)/sizeof(kws[0]));z++)if(strcmp(n,kws[z])==0)return 0;
	return findop(n,monads)<0&&findop(n,dyads)<0;
}
str name(char*n){
	str r=expect('n',1);if(perr()||!strcmp(n,"member"))return r;
	if(!ident(r.sv))snprintf(par.error,sizeof(par.error),"'%s' is a keyword, and cannot be used for a %s name.",r.sv,n);
	return r;
}
lv* names(char*end,char*type){lv*r=lml(0);while(!match(end)&&!perr())ll_add(r,lmstr(name(type)));return r;}
void expr(lv*b);lv*n_uplevel(lv*self,lv*a); // forward refs
lv* quote(void){lv*r=lmblk();expr(r);blk_end(r);return r;}
void iblock(lv*r){
	int c=0;while(hasnext()){
		if(match("end")){if(!c)blk_lit(r,LNIL);return;}if(c)blk_op(r,DROP);expr(r),c++;
	}if(!perr())snprintf(par.error,sizeof(par.error),"Expected 'end' for block.");
}
lv* block(void){lv*r=lmblk();iblock(r);return r;}
int parseclause(lv*b,int isupdate){
	if(match("where")){
		lv*ex=quote();int grouped=parseclause(b,isupdate);
		if(!grouped)                                {blk_lit(b,ex),blk_op(b,COL),blk_op2(b,"@where");}
		else{lv*n=tempname(),*l=lmblk();blk_get(l,n),blk_lit(l,ex),blk_op(l,COL),blk_op2(l,"@where");blk_loop(b,l_list(n),l);}
		return grouped;
	}
	if(match("orderby")){
		lv*ex=quote();int dir=1;if(match("asc"))dir=-1;else if(match("desc"))dir=1;
		else if(!perr())snprintf(par.error,sizeof(par.error),"Expected 'asc' or 'desc'.");int grouped=parseclause(b,isupdate);
		if(!grouped)                                {blk_lit(b,ex),blk_op(b,COL),blk_lit(b,lmn(dir)),blk_op3(b,"@orderby");}
		else{lv*n=tempname(),*l=lmblk();blk_get(l,n),blk_lit(l,ex),blk_op(l,COL),blk_lit(l,lmn(dir)),blk_op3(l,"@orderby");blk_loop(b,l_list(n),l);}
		return grouped;
	}
	if(match("by")){
		lv*ex=quote();int grouped=parseclause(b,isupdate);if(grouped)blk_op1(b,"raze");
		blk_lit(b,ex),blk_op(b,COL),blk_op2(b,"@by");return 1;
	}
	if(!match("from")&&!perr())snprintf(par.error,sizeof(par.error),"Expected 'from'.");
	expr(b),blk_op1(b,"@tab"),blk_op(b,DUP);return 0;
}
void parsequery(lv*b,char*op,int dcol){
	lv*cols=lmd();while(!perr()&&!matchp("from")&&!matchp("where")&&!matchp("by")&&!matchp("orderby")){
		str x=str_new();int set=peek2().type==':', lit=peek()->type=='s';
		lv* name=lit?(set?lmstr(literal_str(peek())):lmistr("")):lmstr(token_str(peek()));
		int get=ident(name->sv), unique=name->c&&dgeti(cols,name)==-1;if(set&&lit&&!unique)next(),next();
		if     (set&&unique)      {str_addz(&x,name->sv),next();next();}
		else if(get&&unique&&dcol){str_addz(&x,name->sv);}
		else if(dcol)             {str_addc(&x,'c'),wnum(&x,cols->c);}
		ld_add(cols,lmstr(x),quote());
	}
	int grouped=parseclause(b,!strcmp(op,"@upd"));lv*index=lmblk();blk_get(index,lmistr("index"));
	lv*keys=l_comma(l_keys(cols),lmistr("@index")),*n=tempname(),*l=lmblk();if(!grouped)blk_op1(b,"list");
	blk_lit(l,keys),blk_get(l,n);EACH(z,cols){blk_lit(l,cols->lv[z]),blk_op(l,COL);}
	blk_lit(l,index),blk_op(l,COL),blk_op(l,DROP),blk_opa(l,BUND,keys->c),blk_op2(l,"dict");
	blk_loop(b,l_list(n),l),blk_lit(b,keys),blk_op3(b,op);
}
lv* quotesub(void){int c=0;lv*r=lmblk();while(hasnext()&&!matchsp(']'))expr(r),c++;blk_opa(r,BUND,c);return r;}
lv* quotedot(void){lv*r=lmblk();blk_lit(r,l_list(lmstr(name("member"))));return r;}
void parseindex(lv*b,lv*name){
	lv*i=lml(0);while(!perr()&&strchr("[.",peek()->type)){
		if(matchsp('['))ll_add(i,quotesub());
		if(matchsp('.')){
			if(strchr("[.",peek()->type)){
				lv*vn=tempname();
				EACH(z,i){blk_cat(b,i->lv[z]),blk_op(b,CALL);}
				lv*l=lmblk();blk_get(l,vn),parseindex(l,NULL),blk_loop(b,l_list(vn),l);return;
			}else{ll_add(i,quotedot());}
		}
	}
	if(matchsp(':')){
		EACH(z,i)blk_cat(b,i->lv[z]);blk_opa(b,BUND,i->c);blk_op(b,OVER);
		for(int z=0;z<i->c-1;z++)blk_opa(b,IPRE,z),blk_opa(b,IPOST,z);expr(b);blk_imm(b,AMEND,name?name:ZERO);
	}else{EACH(z,i)blk_cat(b,i->lv[z]),blk_op(b,CALL);}
}
void term(lv*b){
	if(peek()->type=='d'){blk_lit(b,lmn(next()->nv));return;}
	if(peek()->type=='s'){blk_lit(b,lmstr(literal_str(next())));return;}
	if(match("if")){
		int fin[4096]={0},fi=0,c=0,e=0,next=-1;expr(b);next=blk_opa(b,JUMPF,0);while(hasnext()){
			if(fi>=4096){snprintf(par.error,sizeof(par.error),"Too many elseif clauses.");return;}
			if(match("elseif")){
				if(e){snprintf(par.error,sizeof(par.error),"Expected 'end'.");return;}
				if(!c)blk_lit(b,LNIL);c=0;fin[fi++]=blk_opa(b,JUMP,0);blk_sets(b,next,blk_here(b));expr(b);next=blk_opa(b,JUMPF,0);continue;
			}
			if(match("else")){
				if(e){snprintf(par.error,sizeof(par.error),"Expected 'end'.");return;}
				if(!c)blk_lit(b,LNIL);c=0,e=1;fin[fi++]=blk_opa(b,JUMP,0);blk_sets(b,next,blk_here(b)),next=-1;continue;
			}
			if(match("end")){
				if(!c)blk_lit(b,LNIL);c=0;if(!e)fin[fi++]=blk_opa(b,JUMP,0);if(next!=-1)blk_sets(b,next,blk_here(b));if(!e)blk_lit(b,LNIL);
				for(int z=0;z<fi;z++)blk_sets(b,fin[z],blk_here(b));return;
			}
			if(c)blk_op(b,DROP);expr(b),c++;
		}
	}
	if(match("while")){
		blk_lit(b,LNIL);int head=blk_here(b);expr(b);int cond=blk_opa(b,JUMPF,0);
		blk_op(b,DROP);iblock(b);blk_opa(b,JUMP,head);blk_sets(b,cond,blk_here(b));return;
	}
	if(match("each")){lv*n=names("in","variable");expr(b),blk_loop(b,n,block());return;}
	if(match("on")){
		str n=name("function");int var=matchsp('.')&&matchsp('.')&&matchsp('.');lv*a=names("do","argument");
		if(!perr()&&var&&a->c!=1){snprintf(par.error,sizeof(par.error),"Variadic functions must take exactly one named argument.");return;}
		if(var&&a->c==1)a=l_list(l_format(lmistr("...%s"),l_first(a)));
		blk_lit(b,lmon(n,a,blk_end(block())));blk_op(b,BIND);return;
	}
	if(match("send")){
		blk_lit(b,lmnat(n_uplevel,NULL)),blk_lit(b,lmstr(name("function"))),blk_op(b,CALL);
		expect('[',0),blk_cat(b,quotesub()),blk_op(b,CALL);return;
	}
	if(match("local")){str n=name("variable");expect(':',0),expr(b),blk_loc(b,lmstr(n));return;}
	if(match("select" )){parsequery(b,"@sel",1);return;}
	if(match("extract")){parsequery(b,"@ext",0);return;}
	if(match("update" )){parsequery(b,"@upd",1);return;}
	if(match("insert")){
		lv*n=lml(0);while(!perr()&&!match("with")){ll_add(n,lmstr(peek()->type=='s'?literal_str(next()):name("column")));}
		int v=0,i=0;while(!perr()){if(match("into")){i=1;break;}if(match("end")){i=0;break;}expr(b),v++;}
		if(n->c==0&&v==0&&i==0){blk_lit(b,lmt());return;}
		if(n->c<1)ll_add(n,lmistr("value"));blk_opa(b,BUND,v),blk_lit(b,n);if(i){expr(b);}else{blk_lit(b,ZERO);}blk_op3(b,"@ins");return;
	}
	if(matchsp('(')){if(matchsp(')')){blk_lit(b,lml(0));return;}expr(b);expect(')',0);return;}
	str s=token_str(peek());
	if(findop(s.sv,monads)>=0&&strchr("mn",peek()->type)){
		next();if(matchsp('@')){
			int depth=0;while(matchsp('@'))depth++;
			expr(b);lv*l=lmblk();blk_opa(l,FMAP,findop(s.sv,monads));
			while(depth-->0){lv*t=tempname(),*m=lmblk(),*n=lmblk();blk_get(n,t),blk_cat(n,l),blk_loop(m,l_list(t),n);l=m;}
			blk_cat(b,l);
		}else{expr(b),blk_op1(b,s.sv);}free(s.sv);return;
	}
	free(s.sv);lv* n=lmstr(name("variable"));
	if(matchsp(':')){expr(b),blk_set(b,n);return;}
	blk_get(b,n);parseindex(b,n);
}
void expr(lv*b){
	term(b);if(strchr("[.",peek()->type)){parseindex(b,NULL);}
	if(matchsp('@')){
		int depth=0;while(matchsp('@'))depth++;
		lv*func=tempname();blk_set(b,func);blk_op(b,DROP);expr(b);
		lv*l=lmblk();blk_get(l,func),blk_op(l,SWAP);int fidx=blk_opa(l,FIDX,0);
		lv*ll=lmblk();blk_get(ll,func);blk_get(ll,lmistr("v"));blk_opa(ll,BUND,1);blk_op(ll,CALL);
		blk_loop(l,l_list(lmistr("v")),ll);blk_sets(l,fidx,blk_here(l));
		while(depth-->0){lv*t=tempname(),*m=lmblk(),*n=lmblk();blk_get(n,t),blk_cat(n,l),blk_loop(m,l_list(t),n);l=m;}
		blk_cat(b,l);return;
	}
	str s=token_str(peek());if(findop(s.sv,dyads)>=0&&strchr("mn",peek()->type)){next(),expr(b),blk_op2(b,s.sv);}free(s.sv);
}
lv* parse(char*text){
	par=(parser){0,0,0,strlen(text),text,{0},{0},"\0"};
	lv*b=lmblk();if(hasnext())expr(b);while(hasnext())blk_op(b,DROP),expr(b);
	if(blk_here(b)==0)blk_lit(b,LNIL);return b;
}

// Interpreter

void env_local(lv*e,lv*n,lv*x){SFIND(z,e,n->sv){e->lv[z]=x;return;}ld_add(e,n,x);}
lv* env_getr(lv*e,lv*n){SFIND(z,e,n->sv)return e->lv[z];return e->env?env_getr(e->env,n): NULL;}
void env_setr(lv*e,lv*n,lv*x){SFIND(z,e,n->sv){e->lv[z]=x;return;}if(e->env)env_setr(e->env,n,x);}
lv* env_get(lv*e,lv*n){lv*r=env_getr(e,n);return r?r:LNIL;}
void env_set(lv*e,lv*n,lv*x){lv*r=env_getr(e,n);r?env_setr(e,n,x):env_local(e,n,x);}
lv* env_bind(lv*e,lv*k,lv*v){lv*r=lmenv(e);EACH(z,k)env_local(r,k->lv[z],z<v->c?v->lv[z]:LNIL);return r;}
#define running()      (state.t->c)
#define ev()           (state.e->lv[state.e->c-1])
#define issue(env,blk) ll_add(state.e,env),ll_add(state.t,blk),idx_push(&state.pcs,0)
#define descope        ll_pop(state.t),ll_pop(state.e),idx_pop(&state.pcs)
#define ret(x)         ll_add(state.p,x)
#define arg()          ll_pop(state.p)
#define getblock()     ll_peek(state.t)
#define getpc()        idx_peek(&state.pcs)

void docall(lv*f,lv*a,int tail){
	if(linat(f)){ret(((lv*(*)(lv*,lv*))f->f)(f->a,a));return;}
	if(!lion(f)){ret(l_at(f,l_first(a)));return;}
	if(tail){descope;}
	issue(f->c==1&&f->lv[0]->sv[0]=='.'?env_bind(f->env,l_list(lmcstr(f->lv[0]->sv+3)),l_list(a)) :env_bind(f->env,f,a),f->b);
	gc.depth=MAX(gc.depth,state.e->c);
}
void runop(void){
	lv*b=getblock();
	int*pc=getpc(),op=blk_getb(b,*pc),imm=(oplens[op]==3?blk_gets(b,1+*pc):0);(*pc)+=oplens[op];
	switch(op){
		case DROP:arg();break;
		case DUP:{lv*a=arg();ret(a),ret(a);break;}
		case SWAP:{lv*a=arg(),*b=arg();ret(a),ret(b);break;}
		case OVER:{lv*a=arg(),*b=arg();ret(b),ret(a),ret(b);break;}
		case JUMP:*pc=imm;break;
		case JUMPF:if(!lb(arg()))*pc=imm;break;
		case LIT:ret(blk_getimm(b,imm));break;
		case GET:{ret(env_get(ev(),blk_getimm(b,imm)));break;}
		case SET:{lv*v=arg();env_set(ev(),blk_getimm(b,imm),v);ret(v);break;}
		case LOC:{lv*v=arg();env_local(ev(),blk_getimm(b,imm),v);ret(v);break;}
		case BUND:{lv*r=lml(imm);EACHR(z,r)r->lv[z]=arg();ret(r);break;}
		case OP1:{                      ret(((lv*(*)(lv*        ))monads[imm].func)(arg()    ));break;}
		case OP2:{           lv*y=arg();ret(((lv*(*)(lv*,lv*    ))dyads [imm].func)(arg(),y  ));break;}
		case OP3:{lv*z=arg();lv*y=arg();ret(((lv*(*)(lv*,lv*,lv*))triads[imm].func)(arg(),y,z));break;}
		case IPRE:{lv*s=arg(),*i=arg();ret(i);docall(s,i->lv[imm],0);if(lion(s)||lii(s)||linat(s)){for(int z=0;z<=imm;z++)i->lv[z]=NULL;}break;}
		case IPOST:{lv*s=arg(),*i=arg(),*r=arg();ret(i->lv[imm]?r:s),ret(i),ret(s);break;}
		case AMEND:{
			lv*v=arg(),*r=arg(),*i=arg(),*ro=arg(),*n=blk_getimm(b,imm);
			int t=1;if(i->c&&!i->lv[0]){lv*ni=lml(0);EACH(z,i)if(i->lv[z])ll_add(ni,i->lv[z]);i=ni,t=0;}
			r=amendv(ro,i,v,0,&t);if(t&&!lin(n))env_set(ev(),n,r);ret(r);break;
		}
		case CALL:case TAIL:{lv*a=arg(),*f=arg();docall(f,a,op==TAIL);break;}
		case BIND:{
			lv*f=arg();str n=str_new();str_addz(&n,f->sv);MAP(a,f)f->lv[z];
			lv*r=lmon(n,a,f->b);r->env=ev(),env_local(ev(),lmcstr(r->sv),r),ret(r);break;
		}
		case ITER:{lv*x=arg();ret(lil(x)?x:ld(x));ret(lid(x)?lmd():lml(0));break;}
		case FIDX:{lv*x=arg(),*f=arg();if((lid(f)||lil(f)||lis(f))&&lil(x)){MAP(r,x)l_at(f,x->lv[z]);ret(r);*pc=imm;}else{ret(x);}break;}
		case FMAP:{
			lv*x=arg();lv*(*f)(lv*)=(lv*(*)(lv*))monads[imm].func;
			if(lid(x)){DMAP(r,x,f(x->lv[z]));ret(r);}else{x=ll(x);MAP(r,x)f(x->lv[z]);ret(r);}break;
		}
		case EACH:{
			lv*n=arg(),*r=arg(),*s=arg();if(r->c==s->c){*pc=imm,ret(r);break;}
			int z=r->c;lv*v=lml(3);v->lv[0]=s->lv[z],v->lv[1]=lid(s)?s->kv[z]:lmn(z),v->lv[2]=lmn(z);
			ll_add(state.e,env_bind(ev(),n,v)),ret(s),ret(r);break;
		}
		case NEXT:{
			lv*v=arg(),*r=arg(),*s=arg();ll_pop(state.e);
			if(lid(r)){ld_add(r,s->kv[r->c],v);}else{ll_add(r,v);}ret(s),ret(r),*pc=imm;break;
		}
		case COL:{
			lv*ex=arg(),*t=arg();ret(t);
			GEN(n,t->c)t->kv[z];GEN(v,t->c)t->lv[z];ll_add(n,lmistr("column")),ll_add(v,t);
			issue(env_bind(ev(),n,v),ex);break;
		}
	}while(running()&&*getpc()>=blk_here(getblock()))descope;
}
lv*n_uplevel(lv*self,lv*a){
	(void)self;int i=2;lv*e=ev(),*r=NULL,*name=ls(a);
	while(e&&i){r=NULL;SFIND(z,e,name->sv)r=e->lv[z];if(r)i--;e=e->env;}return r?r:LNIL;
}
lv*n_feval(lv*self,lv*a){
	(void)self;lv*r=a->lv[0],*x=a->lv[1];dset(r,lmistr("value"),x);
	lv*b=dget(r,lmistr("vars"));EACH(z,ev())dset(b,lmcstr(ev()->kv[z]->sv),ev()->lv[z]);return r;
}
lv*n_eval(lv*self,lv*a){
	(void)self;lv*y=a->c>1?ld(a->lv[1]):lmd(),*r=lmd();DMAP(yy,y,y->lv[z]);
	dset(r,lmistr("value"),LNIL),dset(r,lmistr("vars"),yy);
	lv* prog=parse(ls(l_first(a))->sv);
	if(perr()){dset(r,lmistr("error"),lmcstr(par.error)),dset(r,lmistr("errorpos"),lml2(lmn(par.r),lmn(par.c)));return r;}
	GEN(k,yy->c)yy->kv[z];GEN(v,yy->c)yy->lv[z];
	blk_opa(prog,BUND,2),blk_lit(prog,lmnat(n_feval,NULL)),blk_op(prog,SWAP),blk_op(prog,CALL);
	issue(env_bind(a->c>2&&lb(a->lv[2])?ev():NULL,k,v),prog);return r;
}
void init_interns(void){for(int z=0;z<=383;z++){lv*t=&interned[z];t->t=0,t->c=1,t->nv=z-128;}}
void init(lv*e){state.p=lml(0),state.t=lml(0),state.e=lml(0),state.pcs=idx_new(0);ll_add(state.e,e);}
void pushstate(lv*e){
	if(!state.p)printf("trying to save an uninitialized state!\n");
	gc.st[gc.ss++]=state;init(e);
}
void popstate(void){
	idx_free(&state.pcs);
	if(gc.ss){state=gc.st[--gc.ss];}else{printf("state underflow!\n");}
}
void runexpr(lv*env,lv*x){issue(env,x);}
void runfunc(lv*env,lv*f,lv*args){
	if(f&&lion(f)){lv*b=lmblk();blk_lit(b,f),blk_lit(b,args),blk_op(b,CALL),blk_op(b,DROP);issue(env,b);}
}
void halt(void){state.e->c=0,state.t->c=0,state.p->c=0,state.pcs.c=0;}
lv*run(lv*x,lv*rootenv){
	init(rootenv),issue(rootenv,x);int c=0;while(running()){runop(),c++;if(c%100==0)lv_collect();}
	if(state.p->c<1)return LNIL;lv*r=arg();
	while(state.p->c)printf("STACK JUNK: "),debug_show(arg());return r;
}

// Standard Library

#define ivalue(x,k)    dget(x->b,lmistr(k))
#define ifield(x,k)    ((lv*(*)(lv*,lv*,lv*))x->f)(x,lmistr(k),NULL)
#define iindex(x,k,v)  ((lv*(*)(lv*,lv*,lv*))x->f)(x,lmn(k),v)
#define iwrite(x,k,v)  ((lv*(*)(lv*,lv*,lv*))x->f)(x,k,v)

void show(str*s,lv*x,int toplevel){
	if(!x){str_addz(s,"<NULL>");}
	else if(linil(x)){if(!toplevel)str_addz(s,"nil");}
	else if(lin(x)){wnum(s,x->nv);}
	else if(lil(x)){
		str_addc(s,'(');EACH(z,x){if(z)str_addc(s,',');show(s,x->lv[z],0);}str_addc(s,')');
	}
	else if(lid(x)){
		str_addc(s,'{');EACH(z,x){
			if(z)str_addc(s,',');show(s,x->kv[z],0),str_addc(s,':'),show(s,x->lv[z],0);
		}str_addc(s,'}');
	}
	else if(lis(x)){
		str_addc(s,'"');EACH(z,x){
			char c=x->sv[z];if(c=='\n'?(c='n',1):!!strchr("\"\\",c))str_addc(s,'\\');str_addc(s,c);
		}str_addc(s,'"');
	}
	else if(lion(x)){
		str_addz(s,"on "),str_addz(s,x->sv);
		EACH(z,x)str_addc(s,' '),str_addl(s,x->lv[z]);
		str_addz(s," do ... end");
	}
	else if(linat(x)){str_addz(s,"on native x do ... end");}
	else if(lit(x)&&!toplevel){
		str_addz(s,"insert ");EACH(z,x)str_addl(s,x->kv[z]),str_addc(s,' ');
		str_addz(s,"with ");for(int r=0;r<x->n;r++)EACH(z,x)show(s,x->lv[z]->lv[r],0),str_addc(s,' ');
		str_addz(s,"end");return;
	}
	else if(lit(x)){
		if(x->c==0){str_addz(s,"++\n||\n++");return;}
		idx w=idx_new(x->c);lv*cols=lml(x->c);EACH(c,x){
			lv*col=lml(x->n+1);col->lv[0]=x->kv[c];w.iv[c]=col->lv[0]->c,cols->lv[c]=col;
			for(int r=0;r<x->n;r++){
				str i=str_new();show(&i,x->lv[c]->lv[r],0),col->lv[r+1]=lmstr(i);
				if(i.c>w.iv[c])w.iv[c]=MIN(40,i.c); // cap widths for saner output
			}
		}
#define sep()str_addc(s,'+');EACH(c,x){for(int i=0;i<w.iv[c]+2;i++)str_addc(s,'-');str_addc(s,'+');}
#define sepn()sep()str_addc(s,'\n');
#define dati(r,c){lv*t=cols->lv[c]->lv[r];str_addc(s,i<t->c?t->sv[i]:' ');}
#define dats(r,c)dati(r,c)str_add(s," | ",c==x->c-1?2:3);
#define dat(r) str_addz(s,"| ");EACH(c,x){for(int i=0;i<w.iv[c];i++)dats(r,c)}str_addc(s,'\n');
		sepn()dat(0)if(x->n){sepn()}for(int r=0;r<x->n;r++){dat(r+1)}sep()idx_free(&w);
	}
	else if(lii(x)){str_addc(s,'<'),str_addl(s,x->a),str_addc(s,'>');}
	else if(liblk(x)){char t[64];str_add(s,t,snprintf(t,sizeof(t),"<BLOCK: %d OPS, %d LOCALS>",blk_here(x),x->c));}
	else{char t[64];str_add(s,t,snprintf(t,sizeof(t),"<INVALID: %d>",x->t));}
}
lv*debug_show(lv*x){str s=str_new();show(&s,x,1);printf("%s\n",drom_to_utf8(lmstr(s))->sv);return x;}
lv*n_printf(lv*a,int newline,FILE*out){
	if(a->c<2){fprintf(out,"%s",drom_to_utf8(l_first(a))->sv);}
	else{a=l_format(ls(l_first(a)),l_drop(ONE,a));fprintf(out,"%s",drom_to_utf8(a)->sv);}
	if(newline)fprintf(out,"\n");return a;
}
#define fchar(x) (x=='I'?'i': x=='B'?'b': x=='L'?'s': x)
lv*n_readcsv(lv*self,lv*a){
	#define cm(x) t->sv[i]==x?(i++,1):0
	#define css   while(cm(' ')){}
	(void)self;lv*t=a->c>0?ls(a->lv[0]):lms(0);lv*r=lmt(),*s=(a->c>=2&&lis(a->lv[1]))?a->lv[1]:NULL;
	char delim=a->c>=3?ls(a->lv[2])->sv[0]:',';
	int i=0,n=0,slots=0,slot=0;;char b[32];while(i<t->c&&t->sv[i]!='\n'){
		css;str name=str_new();while(i<t->c&&t->sv[i]!='\n'&&t->sv[i]!=delim)str_addc(&name,t->sv[i++]);
		if(!s||(n<s->c&&s->sv[n]!='_'))dset(r,lmstr(name),lml(0));else free(name.sv);n++;
		if(t->sv[i++]=='\n')break;css;cm(delim);
	}while(s&&n<s->c)if(s->sv[n++]!='_')snprintf(b,32,"c%d",n-1),dset(r,lmcstr(b),lml(0));
	if(!s)s=l_take(lmn(r->c),lmistr("s"));EACH(z,s)if(s->sv[z]!='_')slots++;slots=MIN(slots,r->c);
	char f[]={'%',0,0};MAP(fmts,s)(f[1]=fchar(s->sv[z]),lmcstr(f));
	n=0;while(i<=t->c){
		css;str val=str_new();
		if(cm('"'))while(i<t->c)if(cm('"')){if(cm('"'))str_addc(&val,'"');else break;}else str_addc(&val,t->sv[i++]);
		else{while(i<t->c&&!strchr("\n\"",t->sv[i])&&t->sv[i]!=delim)str_addc(&val,t->sv[i++]);}
		if(n<s->c&&s->sv[n]!='_'){ll_add(r->lv[slot++],s->sv[n]=='s'?lmstr(val): l_parse(fmts->lv[n],lmstr(val)));}
		else{free(val.sv);}n++;
		if(i>=t->c||t->sv[i]=='\n'){
			while(n<s->c){char u=s->sv[n++];if(u!='_'&&slot<slots)ll_add(r->lv[slot++],LNIL);}
			if(t->sv[i]=='\n'&&i==t->c-1)break;i++,n=0,slot=0;
		}else{css;cm(delim);}
	}return torect(r);
}
lv*n_writecsv(lv*self,lv*a){
	(void)self;str r=str_new();lv*t=lt(l_first(a)),*s=a->c>1?ls(a->lv[1]):l_take(lmn(t->c),lmistr("s"));
	char delim=a->c>=3?ls(a->lv[2])->sv[0]:',';
	int n=0;EACH(c,s)if(s->sv[c]!='_'){
		if(n++)str_addc(&r,delim);char b[32];
		if(c>=t->c)str_add(&r,b,snprintf(b,32,"c%d",c+1));else str_addl(&r,t->kv[c]);
	}
	for(int z=0;z<t->n;z++){
		str_addc(&r,'\n');int n=0;EACH(c,s)if(s->sv[c]!='_'){
			if(n++)str_addc(&r,delim);
			str rc=str_new();format_type_simple(&rc,c>=t->c?lms(0):t->lv[c]->lv[z],fchar(s->sv[c]));lv*o=lmstr(rc);
			int e=0;EACH(z,o)e|=(!!strchr("\n\"",o->sv[z]))||o->sv[z]==delim;
			if(e)str_addc(&r,'"');EACH(z,o){if(o->sv[z]=='"')str_addc(&r,'"');str_addc(&r,o->sv[z]);}if(e)str_addc(&r,'"');
		}
	}return lmstr(r);
}
void writexmlstr(str*s,lv*x){
	#define xc(a,b) if(c==a)str_addz(s,"&" #b ";"),c=0;
	EACH(z,x){char c=x->sv[z];xc('&',amp)xc('\'',apos)xc('"',quot)xc('>',gt)xc('<',lt)if(c)str_addc(s,c);}
}
void writexmlrec(str*s,lv*x,int tab,int fmt){
	if(lil(x)){EACH(z,x)writexmlrec(s,x->lv[z],tab,fmt);return;}
	if(lii(x)&&!strcmp(x->a->sv,"array")){
		lv*ck=lmistr("cast"),*c=ifield(x,"cast");iwrite(x,ck,lmistr("char"));
		str_addl(s,iwrite(x,lml2(ZERO,ifield(x,"size")),NULL)),iwrite(x,ck,c);
	}
	if(!lid(x)){writexmlstr(s,ls(x));if(tab&&fmt)str_addc(s,'\n');return;}
	lv*t=ls(dgetv(x,lmistr("tag"))),*a=ld(dgetv(x,lmistr("attr"))),*c=dget(x,lmistr("children"));
	str_addc(s,'<'),str_addz(s,t->sv);EACH(z,a){
		str_addc(s,' '),str_addz(s,ls(a->kv[z])->sv);
		str_addz(s,"=\""),writexmlstr(s,ls(a->lv[z])),str_addc(s,'"');
	}
	c=c?ll(c):lml(0);if(!c->c){str_addz(s,fmt?"/>\n":"/>");return;}str_addz(s,fmt?">\n":">");
	#define indent(n) for(int z=0;fmt&&z<n;z++)str_addc(s,' ');
	EACH(z,c){indent(tab+2);writexmlrec(s,c->lv[z],tab+2,fmt);}
	indent(tab);str_addz(s,"</"),str_addz(s,t->sv);str_addz(s,fmt?">\n":">");
}
lv*n_writexml(lv*self,lv*a){(void)self;str r=str_new();writexmlrec(&r,l_first(a),0,a->c>1?lb(a->lv[1]):0);return lmstr(r);}
lv*readxmltext(char*t,int*i,char stop){
	str r=str_new();while(t[*i]&&!(stop==' '&&strchr(">/ \n",t[*i]))){
		#define xs      while(isspace(t[*i]))++*i
		#define xi(a,b) !strncmp(&t[*i],a,strlen(a))? ((*i)+=strlen(a),b):
		#define xr(a,b) xi("&" #a ";",b)
		int w=0;xs,w=1;if(w)str_addc(&r,' ');if(stop==t[*i]||!t[*i])break;
		str_addc(&r,xr(amp,'&')xr(apos,'\'')xr(quot,'"')xr(gt,'>')xr(lt,'<')xr(nbsp,' ')((*i)++,t[*i-1]));
	}if(strchr("'\"",stop)&&t[*i])++*i;return lmstr(r);
}
lv*readxmlname(char*t,int*i){
	str r=str_new();xs;while(!strchr(">/= \n\0",t[*i]))str_addc(&r,drom_tolower(t[(*i)++]));xs;return lmstr(r);
}
lv*readxmlrec(char*t,int*i,char*currtag){
	#define xcl   while(t[*i]&&t[*i]!='>')++*i;++*i;
	#define xm(x) t[*i]==x?(++*i,1):0
	lv*r=lml(0);while(t[*i]){
		int w=0;xs,w=1;
		if(!strncmp(&t[*i],"<![CDATA[",9)){
			(*i)+=9;str c=str_new();
			while(t[*i]&&strncmp(&t[*i],"]]>",3))str_addc(&c,t[(*i)++]);
			(*i)+=3;ll_add(r,lmstr(c));continue;
		}
		if(t[*i]!='<'){if(w)(*i)--;ll_add(r,readxmltext(t,i,'<'));continue;}
		++*i;xs;
		if(xm('!')||xm('?')){xcl;continue;}// skip pragmas/comments/prolog
		if(xm('/')){lv*n=readxmlname(t,i);xm('>');if(!strcmp(currtag,n->sv))break;continue;}
		lv*tag=lmd(),*attr=lmd(),*n=readxmlname(t,i);ll_add(r,tag);
		dset(tag,lmistr("tag"),n),dset(tag,lmistr("attr"),attr),dset(tag,lmistr("children"),lml(0));
		while(!strchr("/>\0",t[*i])){
			lv*n=readxmlname(t,i);
			if(xm('=')){xs;dset(attr,n,readxmltext(t,i,(xm('\''))?'\'':(xm('"'))?'"':' '));}
			else{dset(attr,n,ONE);}
		}
		if(xm('/')){xcl;}else{if(t[*i])++*i;dset(tag,lmistr("children"),readxmlrec(t,i,n->sv));}
	}return r;
}
lv*n_readxml(lv*self,lv*a){(void)self;int i=0;return readxmlrec(ls(l_first(a))->sv,&i,"");}

#ifdef _WIN32
#include <windows.h>
lv* time_ms(void){
	FILETIME t; GetSystemTimeAsFileTime(&t);
	ULARGE_INTEGER i; i.LowPart=t.dwLowDateTime, i.HighPart=t.dwHighDateTime;
	return lmn(i.QuadPart*1e-4);
}
#else
#ifndef __COSMOPOLITAN__
#include <sys/time.h>
#endif
lv* time_ms(void){
	struct timeval now;gettimeofday(&now,NULL);
	return lmn((((long long)now.tv_sec)*1000)+(now.tv_usec/1000));
}
#endif

int randint(int x){unsigned int y=seed;y^=(y<<13),y^=(y>>17),(y^=(y<<15));return mod(seed=y,x);}
lv* n_random(lv*self,lv*z){
	#define rand_elt lin(x)?lmn(randint(ln(x))): x->c<1?LNIL: lis(x)?l_at(x,lmn(randint(x->c))): lit(x)?l_at(x,lmn(randint(x->n))): x->lv[randint(x->c)]
	if(z->c==0){randint(1);return lmn(((float)(seed&0x7FFFFFFF))/0x7FFFFFFF);}
	(void)self;lv*x=l_first(z);if(z->c<2)return rand_elt;
	int y=ln(z->lv[1]);if(y>=0){GEN(r,y)rand_elt;return r;}
	x=lin(x)?l_range(x):ll(x);if(x->c<1)x=l_list(LNIL);idx pv=idx_new(x->c);EACH(z,x)pv.iv[z]=z;
	for(int i=x->c-1;i>0;i--){int j=randint(i+1);int t=pv.iv[j];pv.iv[j]=pv.iv[i],pv.iv[i]=t;}
	GEN(r,abs(y))x->lv[pv.iv[z%x->c]];idx_free(&pv);return r;
}
int frame_count=0;
#if defined(__APPLE__) && defined(__MACH__)
#define PLATFORM "mac"
#elif defined(__unix__) || defined(__unix)
#define PLATFORM "unix"
#elif defined(_WIN32) || defined(_WIN64)
#define PLATFORM "win"
#else
#define PLATFORM "other"
#endif
#define ikey(name) if(i&&lis(i)&&!strcmp(i->sv,name))
lv*interface_sys(lv*self,lv*i,lv*x){
	if(x&&lis(i)){
		ikey("seed"      ){seed=0xFFFFFFFF&(long long int)ln(x);return x;}
	}else if(lis(i)){
		ikey("version"   )return lmistr(VERSION);
		ikey("platform"  )return lmistr(PLATFORM);
		ikey("seed"      )return lmn(seed);
		ikey("frame"     )return lmn(frame_count);
		ikey("now"       ){time_t now;time(&now);return lmn(now);}
		ikey("ms"        )return time_ms();
		ikey("workspace"){
			lv*r=lmd();
			dset(r,lmistr("allocs"  ),lmn(gc.allocs));
			dset(r,lmistr("frees"   ),lmn(gc.frees ));
			dset(r,lmistr("gcs"     ),lmn(gc.g     ));
			dset(r,lmistr("live"    ),lmn(gc.live  ));
			dset(r,lmistr("heap"    ),lmn(gc.size  ));
			dset(r,lmistr("depth"   ),lmn(gc.depth ));
			return r;
		}
	}return x?x:LNIL;(void)self;
}
