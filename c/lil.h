// Learning in Layers
#ifndef __COSMOPOLITAN__
#include <stdlib.h>
#include <stdio.h>
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
int seed=0x12345;lv interned[512]={{0}};int intern_count=100;
#define intern_num {if(x==floor(x)&&x>=0&&x<100)return &interned[(int)x];}
lv*n_show (lv*self,lv*a); // user-supplied function which displays raw to stdout.
lv*n_print(lv*self,lv*a); // user-supplied function which formats/displays to stdout.
lv*debug_show(lv*x);      // version of l_show() which _always_ goes to stdout, for internal use.

#define NUM          512 // number parsing/formatting buffer size
#define NONE         lmn(0)
#define ONE          lmn(1)
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
str str_new(){return(str){0,32,calloc(32,1)};}
char cl(char x){return x=='\t'?' ':(x>=32&&x<=126)||x=='\n'?x:'?';}
void str_provision(str*s,int size){if(s->size<size)s->sv=realloc(s->sv,s->size=size);}
void str_addraw(str*s,int x){if(s->c+1>=s->size)s->sv=realloc(s->sv,s->size*=2);s->sv[s->c++]=x;}
void str_term(str*s){str_addraw(s,'\0');}
void str_addc(str*s,char x){if(x!='\r')str_addraw(s,cl(x));}
void str_add(str*s,char*x,int n){
	for(int z=0;z<n;z++){unsigned char c=x[z];
		if(c==0xE2&&(unsigned char)x[z+1]==0x80&&((unsigned char)x[z+2]==0x98||(unsigned char)x[z+2]==0x99))c='\'',z+=2;
		if(c==0xE2&&(unsigned char)x[z+1]==0x80&&((unsigned char)x[z+2]==0x9C||(unsigned char)x[z+2]==0x9D))c='"' ,z+=2;
		if((c&0xF0)==0xF0)c=1,z+=3; // skip 4-byte codepoints
		if((c&0xE0)==0xE0)c=1,z+=2; // skip 3-byte codepoints
		if((c&0xC0)==0xC0)c=1,z+=1; // skip 2-byte codepoints
		str_addc(s,c);
	}
}
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
	if(x->lv)free(x->lv);if(x->kv)free(x->kv);if(x->sv)free(x->sv);free(x);gc.frees++,gc.live--;
}
void lv_grow(){
	gc.heap=realloc(gc.heap,(gc.size*2)*sizeof(lv*));
	memset(gc.heap+gc.size,0,gc.size*sizeof(lv*));gc.size*=2;
}
void lv_collect(){
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
lm(d  ,3)()                {lv*r=lmvv(3,16);r->c=0,r->kv=calloc(16,sizeof(lv*));         return r;}
lm(t  ,4)()                {lv*r=lmvv(4,16);r->c=0,r->kv=calloc(16,sizeof(lv*));         return r;}
lm(on ,5)(str n,lv*r,lv*b) {r->t=5,r->sv=n.sv,r->b=b;                                    return r;}
lm(i  ,6)(lv*(*f)(lv*,lv*,lv*),lv*n,lv*s){lv*r=lmv(6);r->f=(void*)f,r->a=n,r->b=s;       return r;}
lm(blk,7)()                {lv*r=lmvv(7,0);r->sv=calloc(32,sizeof(char)),r->ns=32,r->n=0;return r;}
lm(env,8)(lv*p)            {lv*r=lmd();r->t=8;r->env=p;                                  return r;}
lm(nat,9)(lv*(*f)(lv*,lv*),lv*c){lv*r=lmv(9);r->f=(void*)f,r->a=c;                       return r;}
lv* lmstr(str x){lv*r=lmv(1);str_term(&x),r->c=strlen(x.sv);r->sv=x.sv;return r;}
lv* lmcstr(char*x){lv*r=lmv(1);r->c=strlen(x),r->sv=calloc(r->c+1,1),memcpy(r->sv,x,r->c);return r;}
lv* lmutf8(char*x){str r=str_new();str_addz(&r,x);return lmstr(r);}
lv* lmistr(char*x){
	for(int z=0;z<intern_count;z++)if(interned[z].sv==x)return &interned[z];
	lv*r=&interned[intern_count++];r->t=1,r->c=strlen(x),r->sv=x;
	if(intern_count>511)printf("warning: intern heap is full!\n");return r;
}
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
	t[n++]='0';double i=floor(y);while(i>0){t[n++]=fmod(i,10)+'0',i=i/10;}crev(t+1,n-1);while(t[s]=='0'&&t[s+1])s++;t[n++]='.';
	y=round((y-floor(y))*1000000.0);for(int z=0;z<6;z++){t[n+5-z]=fmod(y,10)+'0',y=y/10;}n+=5;while(n>0&&t[n]=='0')n--;if(t[n]=='.')n--;
	str_add(x,t+s,n-s+1);
}
monad(l_rows);monad(l_cols);monad(l_range);monad(l_list);monad(l_first);
dyad(l_dict);dyad(l_fuse);dyad(l_take);void dset(lv*d,lv*k,lv*x);
int    lb(lv*x){return lin(x)?x->nv!=0:lis(x)||lil(x)||lid(x)?x->c!=0:1;}
double ln(lv*x){return lin(x)?x->nv:lis(x)?rnum(x->sv,x->c):(lil(x)||lid(x))&&x->c?ln(x->lv[0]):0;}
lv* ls(lv*x){
	if(lin(x)){str n=str_new();wnum(&n,x->nv);return lmstr(n);}
	return lis(x)?x: lil(x)?l_fuse(lmistr(""),x): lms(0);
}
lv* ll(lv*x){
	if(lid(x)){lv*r=lml(x->c);EACH(z,x)r->lv[z]=x->lv[z];return r;}
	if(lis(x)){lv*r=lml(x->c);EACH(z,x)r->lv[z]=lms(1),r->lv[z]->sv[0]=x->sv[z];return r;}
	return lil(x)?x: lit(x)?l_rows(x): l_list(x);
}
lv* ld(lv*x){return lid(x)?x:lit(x)?l_cols(x):lil(x)||lis(x)?l_dict(l_range(lmn(x->c)),ll(x)):lmd();}
lv* lt(lv*x){
	if(lit(x))return x;lv*r=lmt();if(lid(x)||lil(x)){
		if(lid(x)){lv*k=lml(x->c);EACH(z,x)k->lv[z]=x->kv[z];dset(r,lmistr("key"),k);}
		r->n=x->c; lv*v=lml(x->c);EACH(z,x)v->lv[z]=x->lv[z];dset(r,lmistr("value"),v);
	}else{x=ll(x);dset(r,lmistr("value"),x);r->n=x->c;}return r;
}
int equal(lv*x,lv*y){
	if(lin(x))return ln(x)==ln(y);
	if(lis(x)){y=ls(y);if(x->c==y->c){EACH(z,x)if(x->sv[z]!=y->sv[z])return 0;return 1;}}
	if(lil(x)&&lil(y)) if(x->c==y->c){EACH(z,x)if(!equal(x->lv[z],y->lv[z]))return 0;return 1;}
	if(lid(x)&&lid(y)&&x->c==y->c){
		EACH(z,x)if(!equal(x->lv[z],y->lv[z])||!equal(x->kv[z],y->kv[z]))return 0;return 1;
	}return x==y;
}
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
lv* dgetv(lv*d,lv*k){FIND(z,d,k)return d->lv[z];return NONE;}
int dgeti(lv*d,lv*k){EACH(z,d)if(matchr(d->kv[z],k))return z;return -1;}
lv* dkey(lv*d,lv*v){EACH(z,d)if(matchr(d->lv[z],v))return d->kv[z];return NONE;}
lv* amend(lv*x,lv*i,lv*y){
	if(lii(x))return ((lv*(*)(lv*,lv*,lv*))x->f)(x,i,y);
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
	if(lii(x))return l_ati(x,y);
	if(lit(x)&&lin(y))x=l_rows(x); if((lis(x)||lil(x))&&!lin(y))x=ld(x);
	if(lis(x)){int n=ln(y);lv*r=lms(1);r->sv[0]=(n<0||n>=x->c)?(r->c=0,'\0'):x->sv[n];return r;}
	if(lil(x)){int n=ln(y);return n<0||n>=x->c?NONE:x->lv[n];}
	return lid(x)||(lit(x)&&lis(y))?dgetv(x,y):NONE;
}
lv* amendv(lv*x,lv*i,lv*y,int n,int*tla){
	if(lii(x)){*tla=0;}if(!*tla&&n+1<i->c)return amendv(l_at(x,l_first(i->lv[n])),i,y,n+1,tla);
	return (n+1<i->c)?amend(x,l_first(i->lv[n]),amendv(l_at(x,l_first(i->lv[n])),i,y,n+1,tla)):
	(n+1==i->c)?amend(x,l_first(i->lv[n]),y): y;
}
lv* perfuse(lv*x,lv*(f(lv*))){if(lil(x)){MAP(r,x)perfuse(x->lv[z],f);return r;}return f(x);}
lv* nlperfuse(lv*x,lv*(f(lv*))){if(!lil(x))return f(ll(x));int n=1;EACH(z,x)if(!lin(x->lv[z])){n=0;break;}if(n)return f(x);MAP(r,x)nlperfuse(x->lv[z],f);return r;}
lv* conform(lv*x,lv*y,lv*(f(lv*,lv*))){
	if( lil(x)&& lil(y)){MAP(r,x)conform(x->lv[z],y->c==0?NONE:y->lv[z%y->c],f);return r;}
	if( lil(x)&&!lil(y)){MAP(r,x)conform(x->lv[z],y,f);return r;}
	if(!lil(x)&& lil(y)){MAP(r,y)conform(x,y->lv[z],f);return r;} return f(x,y);
}
lv* torect(lv*t){ // modifies t in-place to rectangularize columns!
	int n=0;EACH(z,t)n=MAX(n,lil(t->lv[z])?t->lv[z]->c:1);
	t->n=n; EACH(z,t)t->lv[z]=l_take(lmn(n),lil(t->lv[z])?t->lv[z]:l_list(t->lv[z]));return t;
}

// Primitives
dyad(l_format);

#define vm(n,op,arg) monad(a_##n){return lmn(op(arg(x)));}monad(l_##n){return perfuse(x,a_##n);}
#define vd(n)        dyad(l_##n){return conform(x,y,a_##n);}
vm(not,!  ,lb) vm(negate,-  ,ln) vm(floor,floor,ln) vm(cos,cos,ln)
vm(sin,sin,ln) vm(tan   ,tan,ln) vm(exp  ,exp  ,ln) vm(ln ,log,ln) vm(sqrt,sqrt,ln)
monad(l_count){return lmn(lin(x)||lis(x)||lil(x)||lid(x)?x->c:lit(x)?x->n:0);}
monad(l_list ){lv*r=lml(1);r->lv[0]=x;return r;}
monad(l_first){if(lit(x))return l_first(l_rows(x));lv*l=ll(x);return!l->c?NONE:l->lv[0];}
monad(l_last ){if(lit(x))return l_last (l_rows(x));lv*l=ll(x);return!l->c?NONE:l->lv[l->c-1];}
monad(l_typeof){
	if(linat(x))return lmistr("function");if(lii(x))return x->a;
	char*n[]={"number","string","list","dict","table","function","INTERNAL"};return lmistr(n[MIN(6,x->t)]);
}
monad(l_keys){if(lii(x))return lml(0);x=ld(x);MAP(r,x)x->kv[z];return r;}
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
		EACH(z,x)for(int c=0;c<m;c++)ll_add(t->lv[c],c>=x->lv[z]->c?NONE:x->lv[z]->lv[c]);return t;
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
	for(int i=0;i<w;i++){MAP(c,x)!lil(x->lv[z])?x->lv[z]:i<x->lv[z]->c?x->lv[z]->lv[i]:NONE;ll_add(r,c);}
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
dyad(a_less){return lmn(lin(x)&&lin(y)? ln(x) <ln(y): strcmp(ls(x)->sv,ls(y)->sv)<0);}vd(less)
dyad(a_more){return lmn(lin(x)&&lin(y)? ln(x) >ln(y): strcmp(ls(x)->sv,ls(y)->sv)>0);}vd(more)
dyad(a_eq  ){return lmn(lii(x)||lii(y)?x==y: lin(x)&&lin(y)? ln(x)==ln(y): !strcmp(ls(x)->sv,ls(y)->sv));}vd(eq)
dyad(a_smin){lv*a=ls(x),*b=ls(y);return strcmp(a->sv,b->sv)<0?x:y;}
dyad(a_min ){if(lin(x)||lin(y)){double a=ln(x),b=ln(y);return lmn(a<b?a:b);}return a_smin(x,y);}vd(min)
dyad(a_smax){lv*a=ls(x),*b=ls(y);return strcmp(a->sv,b->sv)>0?x:y;}
dyad(a_max ){if(lin(x)||lin(y)){double a=ln(x),b=ln(y);return lmn(a>b?a:b);}return a_smax(x,y);}vd(max)
dyad(l_unless){return lin(y)&&ln(y)==0?x:y;}
dyad(l_match){return matchr(x,y)?ONE:NONE;}
dyad(l_dict){x=ll(x);lv*r=lmd();y=ll(y);EACH(z,x)dset(r,x->lv[z],z>=y->c?NONE:y->lv[z]);return r;}
dyad(l_split){
	x=ls(x),y=ls(y);if(x->c==0)return ll(y);lv*r=lml(0);int n=0;EACH(z,y){
		int m=1;EACH(w,x)if(x->sv[w]!=y->sv[z+w]){m=0;break;}if(!m)continue;
		lv*s=lms(z-n);memcpy(s->sv,y->sv+n,z-n);ll_add(r,s);z+=x->c-1,n=z+1;
	}if(n<=y->c){lv*s=lms(y->c-n);memcpy(s->sv,y->sv+n,y->c-n);ll_add(r,s);}return r;
}
dyad(l_fuse){
	str t=str_new();x=ls(x),y=ll(y);EACH(z,y){if(z)str_addl(&t,x);str_addl(&t,ls(y->lv[z]));}
	return lmstr(t);
}
dyad(l_ina){
	if(lil(y))EACH(z,y)if(equal(y->lv[z],x))return ONE;
	return lis(y)?(strstr(y->sv,ls(x)->sv)?ONE:NONE): (lid(y)||lit(y))&&dget(y,x)?ONE: NONE;
}
dyad(l_in){if(lil(x)){MAP(r,x)l_in(x->lv[z],y);return r;}return l_ina(x,y);}
lv*filter(int in,lv*x,lv*y){
	x=lis(x)?l_list(x):ll(x);int n=1;EACH(z,x)if(!lin(x->lv[z]))n=0;
	if(lid(y)){lv*r=lmd();EACH(z,y)if(in==lb(l_ina(y->kv[z],x)))dset(r,y->kv[z],y->lv[z]);return r;}
	if(!lit(y)){lv*r=lml(0);y=ll(y);EACH(z,y)if(in==lb(l_ina(y->lv[z],x)))ll_add(r,y->lv[z]);return r;}
	if(n&&in ){lv*r=l_take(NONE,y);EACH(i,x){int z=ln(x->lv[i]);if(z>=0&&z<y->n)EACH(c,y)ll_add(r->lv[c],y->lv[c]->lv[z]);}return torect(r);}
	if(n&&!in){lv*r=l_take(NONE,y);for(int z=0;z<y->n;z++)if(!lb(l_ina(lmn(z),x)))EACH(c,y)ll_add(r->lv[c],y->lv[c]->lv[z]);return torect(r);}
	lv*r=lmt();EACH(z,y)if(in==lb(l_ina(y->kv[z],x)))dset(r,y->kv[z],y->lv[z]);r->n=y->n;return r;
}
dyad(l_take){
	if(!lin(x))return filter(1,x,y);if(lil(y)&&x->nv==y->c)return y;
	if(lid(y)){lv*t=l_take(x,l_range(lmn(y->c))),*r=lmd();
		EACH(z,t){int i=t->lv[z]->nv;dset(r,y->kv[i],y->lv[i]);}return r;}
	if(lis(y))return l_fuse(lmistr(""),l_take(x,ll(y)));
	if(lit(y)){TMAP(r,y,l_take(x,y->lv[z]));r->n=fabs(ln(x));return r;}
	y=ll(y);int n=y->c,m=ln(x),s=m<0?mod(m,n):0;lv*r=lml(m<0?-m:m);
	EACH(z,r)r->lv[z]=n?y->lv[mod(z+s,n)]:NONE;return r;
}
dyad(l_drop){
	if(!lin(x))return filter(0,x,y); if(lis(y))return l_fuse(lmistr(""),l_drop(x,ll(y)));
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
	EACH(i,x){MAP(c,x->lv[i])x->lv[i]->lv[z];if(!dget(y,x->kv[i]))for(int z=0;z<y->n;z++)ll_add(c,NONE);dset(r,x->kv[i],c);}
	EACH(i,y){lv*c=dget(r,y->kv[i]);if(!c){GEN(nc,x->n)NONE;dset(r,y->kv[i],nc);c=nc;}EACH(z,y->lv[i])ll_add(c,y->lv[i]->lv[z]);}
	return torect(r);
}
dyad(l_comma){
	if(lit(x)&&lit(y))return l_tcomma(x,y);
	if(lid(x)){y=ld(y);DMAP(r,x,x->lv[z]);EACH(z,y)dset(r,y->kv[z],y->lv[z]);return r;}
	if(lis(x))return l_comma(l_list(x),y);if(lis(y))return l_comma(x,l_list(y));
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
		MAP(r,x)l_comma(x->lv[z],y->c==0?NONE:y->lv[z%y->c]);return r;
	}
	lv*ik=lml(0),*dk=lml(0);TMAP(r,x,lml(0));EACH(z,y){
		int i=-1;FIND(w,x,y->kv[z]){i=w;break;}
		if(i>=0){ll_add(ik,y->kv[z]);}else{ll_add(dk,lmn(z)),dset(r,y->kv[z],lml(0));}
	}
	for(int ai=0;ai<x->n;ai++)for(int bi=0;bi<y->n;bi++){
		int m=1;EACH(z,ik){if(!equal(dget(x,ik->lv[z])->lv[ai],dget(y,ik->lv[z])->lv[bi])){m=0;break;}}
		if(!m)continue;
		EACH(z,x )ll_add(r->lv[z     ],x->lv[z                   ]->lv[ai]);
		EACH(z,dk)ll_add(r->lv[x->c+z],y->lv[(int)(dk->lv[z]->nv)]->lv[bi]);r->n++;
	}return r;
}
monad(l_sum ){x=ll(x);lv*r=NONE      ;for(int z=0;z<x->c;z++)r=l_add  (r,x->lv[z]);return r;}
monad(l_amax){x=ll(x);lv*r=l_first(x);for(int z=1;z<x->c;z++)r=l_max  (r,x->lv[z]);return r;}
monad(l_amin){x=ll(x);lv*r=l_first(x);for(int z=1;z<x->c;z++)r=l_min  (r,x->lv[z]);return r;}
monad(l_raze){x=ll(x);lv*r=l_first(x);for(int z=1;z<x->c;z++)r=l_comma(r,x->lv[z]);return r;}
char esc(char e,int*i,char*t,int*n){
	char h[5]={0};return e=='n'?'\n':strchr("\\\"/",e)?e:
	e=='u'&&*n>=4?(memcpy(h,t+*i,4),(*i)+=4,(*n)-=4,strtol(h,NULL,16)):' ';
}
lv* pjson(char*t,int*i,int*f,int*n){
	#define jc()    (*n&&*f?t[*i]:0)
	#define jn()    (*n&&*f?(--*n,t[(*i)++]):0)
	#define jm(x)   jc()==x?(jn(),1):0
	#define js()    while(isspace(jc()))jn();
	#define jd()    while(isdigit(jc()))jn();
	#define jl(x,y) if((*n)>=(int)strlen(x)&&memcmp(t+*i,x,strlen(x))==0)return(*i)+=strlen(x),(*n)-=strlen(x),y;
	jl("null",NONE);jl("false",NONE);jl("true",ONE);
	if(jm('[')){lv*r=lml(0);while(jc()){js();if(jm(']'))break;ll_add(r,pjson(t,i,f,n));js();jm(',');}return r;}
	if(jm('{')){lv*r=lmd( );while(jc()){js();if(jm('}'))break;lv*k=pjson(t,i,f,n);js();jm(':');if(*f)dset(r,k,pjson(t,i,f,n));js();jm(',');}return r;}
	if(jm('"')){str r=str_new();while(jc()&&!(jm('"'))){str_addc(&r,(jm('\\'))?esc(jn(),i,t,n):jn());}return lmstr(r);}
	int ns=*i;jm('-');jd();jm('.');jd();if(jm('e')||jm('E')){jm('-')||jm('+');jd();}if(*i<=ns){*f=0;return NONE;}
	char tb[NUM];snprintf(tb,MIN(*i-ns+1,NUM),"%s",t+ns);return lmn(atof(tb));
}
time_t parts_to_epoch(struct tm *p){
	return p->tm_sec+p->tm_min*60+p->tm_hour*3600+
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
	#define ulc(x) t=='l'?tolower(x):t=='u'?toupper(x):x
	x=ls(x),y=ls(y);int f=0,h=0,m=1,pi=0,named=format_has_names(x);lv*r=named?lmd():lml(0);while(fc){
		if(fc!='%'){if(m&&fc==hc){h++;}else{m=0;}f++;continue;}f++;
		str nk={0};if(fc=='['){f++;nk=str_new();while(fc&&fc!=']')str_addc(&nk,fc),f++;if(fc==']')f++;}
		int n=0,d=0,si=h,sk=fc=='*'&&(f++,1),lf=fc=='-'&&(f++,1);if(fc=='0')f++;
		while(isdigit(fc))n=n*10+fc-'0',f++;if(fc=='.')f++;
		while(isdigit(fc))d=d*10+fc-'0',f++;if(!fc)break;char t=fc;f++;
		if(!strchr("%mnzsluqaroj",t))while(hn&&isspace(hc))h++;lv*v=NULL;
		if     (t=='%'){if(m&&t==hc){h++;}else{m=0;}}
		else if(t=='m')v=m?ONE:NONE;
		else if(t=='n')v=lmn(h);
		else if(t=='z')v=m&&h==y->c?ONE:NONE;
		else if(t=='i'){long long r=0,s=hc=='-'?(h++,-1):1;m&=!!isdigit(hc);while(hn&&isdigit(hc))r=r*10+hc-'0',h++;v=lmn(r*s);}
		else if(t=='h'||t=='H'){long long r=0;m&=!!isxdigit(hc);while(hn&&isxdigit(hc))r=r*16+(hc>'9'?tolower(hc)-'a'+10:hc-'0'),h++;v=lmn(r);}
		else if(strchr("slu",t)){str r=str_new();while(hn&&(n?1:hc!=fc))str_addc(&r,ulc(hc)),h++;v=lmstr(r);}
		else if(t=='a'){v=lml(0);while(hn&&(n?1:hc!=fc))ll_add(v,lmn(hc)),h++;}
		else if(t=='b'){v=strchr("tTyYx1",hc)?ONE:NONE;while(hn&&n?1:hc!=fc)h++;}
		else if(t=='j'){int f=1,c=n?n:y->c;v=m?pjson(y->sv,&h,&f,&c):NONE;}
		else if(t=='v'){str r=str_new();m&=!isdigit(hc);while(hn&&ncc[hc-32]=='n')str_addc(&r,hc),h++;v=lmstr(r);}
		else if(t=='q'){
			str r=str_new();m&=hc=='"';if(m)h++;while(hn&&hc!='"'){
				if(hc=='\\'){h++;if(m&=!!strchr("\\\"n",hc)){str_addc(&r,hc=='n'?'\n':hc);}}else{str_addc(&r,hc);}h++;
			}if(m&=hc=='"')h++;v=lmstr(r);
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
		else{m=0;}while(n&&hc&&h-si<n)h++,m=0;if(!sk&&v){named?dset(r,nk.sv?lmstr(nk):lmn(pi),v):ll_add(r,v);pi++;}
	}return named?r: r->c==1?r->lv[0]:r;
}
void fjson(str*s,lv*x){
	if(lin(x)){wnum(s,x->nv);}
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
	else if(t=='q'){str v=str_new();fjson(&v,ls(a));op=lmstr(v)->sv;}
	else if(t=='e'){time_t v=ln(a);strftime(o,NUM,"%FT%TZ",gmtime(&v));}
	else if(t=='p'){
		struct tm v={0};lv*d=ld(a);
		#define pg(x,f,o) {lv*p=dget(d,lmcstr(x));v.tm_##f=p?ln(p)-o:0;}
		pg("year",year,1900)pg("month",mon,1)pg("day",mday,0)pg("hour",hour,0)pg("minute",min,0)pg("second",sec,0)
		strftime(o,NUM,"%FT%TZ",&v);
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
		lv*a=strchr("sluvroq",t)?lmistr(""):NONE,*an=named?dget(y,nk.sv?lmstr(nk):lmn(h)): NULL;
		a=t=='%'?NONE: named?(an?an:a): (!sk&&h<y->c)?y->lv[h]: a;
		format_type(&r,a,t,n,d,lf,pz,&f,x->sv);if(t!='%'&&!sk)h++;
	}return lmstr(r);
}
lv* l_ins(lv*v,lv*n,lv*x){lv*r=torect(l_table(l_dict(n,v)));return lin(x)?r:l_comma(lt(x),r);}
lv* l_tab(lv*t){t=lt(t);TMAP(r,t,t->lv[z]);torect(r);dset(r,lmistr("index"),l_range(lmn(r->n)));return r;}
lv* merge(lv*vals,lv*keys,int widen,lv**ix){
	lv*i=lmistr("@index");
	if(!widen){*ix=lml(0);EACH(z,vals){lv*x=dget(vals->lv[z],i);EACH(z,x)ll_add(*ix,x->lv[z]);}}
	if(vals->c==0){lv*d=lmd();EACH(z,keys)dset(d,keys->lv[z],lml(0));ll_add(vals,d);}
	GEN(r,vals->c)l_table(widen?vals->lv[z]:l_drop(i,vals->lv[z]));r=l_raze(r);
	if(widen){*ix=dget(r,i);r=l_drop(i,r);}return r;
}
lv* l_select(lv*orig,lv*vals,lv*keys){
	lv*ix=NULL,*r=merge(vals,keys,0,&ix);return keys->c>1?r:l_take(ix,l_drop(lmistr("index"),orig));
}
lv* l_extract(lv*orig,lv*vals,lv*keys){
	lv*r=l_cols(l_select(orig,vals,keys));
	int c=1;EACH(z,r)if(!lil(r->lv[z])||r->lv[z]->c!=1)c=0;if(c)EACH(z,r)r->lv[z]=l_first(r->lv[z]);
	return (r->c!=1||r->kv[0]->c)?r: l_first(r);
}
lv* l_update(lv*orig,lv*vals,lv*keys){
	orig=l_drop(lmistr("index"),orig);lv*ix=NULL,*r=merge(vals,keys,1,&ix);EACH(c,r){
		if(r->lv[c]==ix)continue;lv*k=r->kv[c];
		int ci=dgeti(orig,k);GEN(col,orig->n)ci==-1?NONE:orig->lv[ci]->lv[z];dset(orig,k,col);
		EACH(row,ix){col->lv[(int)ln(ix->lv[row])]=r->lv[c]->lv[row];}
	}return orig;
}
#define prim(n,f) {n,(void*)f}
primitive monads[]={
	prim("-",l_negate),prim("!",l_not),prim("floor",l_floor),prim("cos",l_cos),prim("sin",l_sin),
	prim("tan",l_tan),prim("exp",l_exp),prim("ln",l_ln),prim("sqrt",l_sqrt),
	prim("sum",l_sum),prim("raze",l_raze),prim("max",l_amax),prim("min",l_amin),
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
	prim("format",l_format),prim("unless",l_unless),prim("limit",l_limit),prim("",NULL)
};
primitive triads[]={
	prim("@sel",l_select),prim("@ext",l_extract),prim("@upd",l_update),prim("@ins",l_ins),prim("",NULL)
};

// Bytecode

int findop(char*n,primitive*p){if(n)for(int z=0;p[z].name[0];z++)if(!strcmp(n,p[z].name))return z;return -1;}
int tnames=0;lv* tempname(){char t[64];snprintf(t,sizeof(t),"@t%d",tnames++);return lmcstr(t);}
enum opcodes {JUMP,JUMPF,LIT,DROP,SWAP,OVER,BUND,OP1,OP2,OP3,GET,SET,LOC,AMEND,TAIL,CALL,BIND,ITER,EACH,NEXT,COL,QUERY,IPRE,IPOST};
int oplens[]={3   ,3    ,3  ,1   ,1   ,1   ,3   ,3  ,3  ,3  ,3  ,3  ,3  ,3    ,1   ,1   ,1   ,1   ,3   ,3   ,1  ,3    ,3   ,3    };
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
		else if(b==JUMP||b==JUMPF||b==EACH||b==NEXT){blk_opa(x,b,blk_gets(y,z+1)+base);}
		else{for(int i=0;i<oplens[b];i++)blk_addb(x,blk_getb(y,z+i));}z+=oplens[b];
	}
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
char tc(){return (par.i>=par.tl)?' ':par.text[par.i];}
char ccc(){char x=tc();return x>=32&&x<=126?tcc[x-32]:' ';}
char ccn(){char x=tc();return x>=32&&x<=126?ncc[x-32]:' ';}
int  iw(){char x=tc();return x==' '||x=='\t'||x=='\n'||x=='#';}
int mprev(){if(par.i<1)return 0;char x=par.text[par.i-1];return x>=32&&x<=126?mcc[x-32]=='x':0;}
char nc(){
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
token* next(){
	if(par.next.type){par.here=par.next;par.next.type=0;}else{tok(&par.here);}
	return &par.here;
}
token* peek(){if(!par.next.type)tok(&par.next);return &par.next;}
token peek2(){parser t=par;next();token r=*peek();par=t;return r;}
int hasnext(){return !perr()&&((par.next.type&&par.next.type!='e')||peek()->type!='e');}
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
		"select","extract","update","insert","into","from","where","by","orderby","asc","desc"
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
lv* quote(){lv*r=lmblk();expr(r);blk_end(r);return r;}
void iblock(lv*r){
	int c=0;while(hasnext()){
		if(match("end")){if(!c)blk_lit(r,NONE);return;}if(c)blk_op(r,DROP);expr(r),c++;
	}if(!perr())snprintf(par.error,sizeof(par.error),"Expected 'end' for block.");
}
lv* block(){lv*r=lmblk();iblock(r);return r;}
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
	lv*cw,*cb,*co;int dir=-1;lv*index=lmblk();blk_get(index,lmistr("index"));
	if(match("where"  )){cw=quote();}else{cw=lmblk();blk_lit(cw,ONE);}
	if(match("by"     )){cb=quote();}else{cb=lmblk();blk_lit(cb,NONE);}
	if(match("orderby")){
		co=quote();if(match("asc"))dir=-1;else if(match("desc"))dir=1;
		else if(!perr())snprintf(par.error,sizeof(par.error),"Expected 'asc' or 'desc'.");
	}else{co=lmblk();blk_get(co,lmistr("index"));}
	if(!match("from")&&!perr())snprintf(par.error,sizeof(par.error),"Expected 'from'.");
	expr(b),blk_op1(b,"@tab");
	blk_lit(b,cw),blk_op(b,COL);blk_lit(b,cb),blk_op(b,COL);blk_lit(b,co),blk_op(b,COL);
	blk_lit(b,lmn(dir));blk_opa(b,QUERY,!strcmp(op,"@upd"));
	lv*name=tempname(),*names=l_list(name),*keys=l_comma(l_keys(cols),lmistr("@index"));
	blk_op(b,ITER);int head=blk_here(b);blk_lit(b,names);
	int each=blk_opa(b,EACH,0);
		blk_lit(b,keys);blk_get(b,name);EACH(z,cols){blk_lit(b,cols->lv[z]);blk_op(b,COL);}
		blk_lit(b,index);blk_op(b,COL);blk_op(b,DROP);blk_opa(b,BUND,keys->c),blk_op2(b,"dict");
	blk_opa(b,NEXT,head),blk_sets(b,each,blk_here(b));blk_lit(b,keys),blk_op3(b,op);
}
lv* quotesub(){int c=0;lv*r=lmblk();while(hasnext()&&!matchsp(']'))expr(r),c++;blk_opa(r,BUND,c);return r;}
lv* quotedot(){lv*r=lmblk();blk_lit(r,l_list(lmstr(name("member"))));return r;}
void parseindex(lv*b,lv*name){
	lv*i=lml(0);while(!perr()&&strchr("[.",peek()->type)){
		if(matchsp('['))ll_add(i,quotesub());
		if(matchsp('.')){
			if(strchr("[.",peek()->type)){
				EACH(z,i){blk_cat(b,i->lv[z]),blk_op(b,CALL);}
				blk_op(b,ITER);int head=blk_here(b);blk_lit(b,l_list(lmistr("x")));
				int each=blk_opa(b,EACH,0);blk_get(b,lmistr("x"));parseindex(b,NULL);
				blk_opa(b,NEXT,head);blk_sets(b,each,blk_here(b));return;
			}else{ll_add(i,quotedot());}
		}
	}
	if(matchsp(':')){
		EACH(z,i)blk_cat(b,i->lv[z]);blk_opa(b,BUND,i->c);blk_op(b,OVER);
		for(int z=0;z<i->c-1;z++)blk_opa(b,IPRE,z),blk_opa(b,IPOST,z);expr(b);blk_imm(b,AMEND,name?name:NONE);
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
				if(!c)blk_lit(b,NONE);c=0;fin[fi++]=blk_opa(b,JUMP,0);blk_sets(b,next,blk_here(b));expr(b);next=blk_opa(b,JUMPF,0);continue;
			}
			if(match("else")){
				if(e){snprintf(par.error,sizeof(par.error),"Expected 'end'.");return;}
				if(!c)blk_lit(b,NONE);c=0,e=1;fin[fi++]=blk_opa(b,JUMP,0);blk_sets(b,next,blk_here(b)),next=-1;continue;
			}
			if(match("end")){
				if(!c)blk_lit(b,NONE);c=0;if(!e)fin[fi++]=blk_opa(b,JUMP,0);if(next!=-1)blk_sets(b,next,blk_here(b));if(!e)blk_lit(b,NONE);
				for(int z=0;z<fi;z++)blk_sets(b,fin[z],blk_here(b));return;
			}
			if(c)blk_op(b,DROP);expr(b),c++;
		}
	}
	if(match("while")){
		blk_lit(b,NONE);int head=blk_here(b);expr(b);int cond=blk_opa(b,JUMPF,0);
		blk_op(b,DROP);iblock(b);blk_opa(b,JUMP,head);blk_sets(b,cond,blk_here(b));return;
	}
	if(match("each")){
		lv*n=names("in","variable");expr(b);blk_op(b,ITER);int head=blk_here(b);blk_lit(b,n);
		int each=blk_opa(b,EACH,0);iblock(b);blk_opa(b,NEXT,head),blk_sets(b,each,blk_here(b));
		return;
	}
	if(match("on")){
		str n=name("function");lv*a=names("do","argument");
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
		lv*n=lml(0);while(!perr()&&!match("into")){ll_add(n,lmstr(peek()->type=='s'?literal_str(next()):name("column"))),expect(':',0),expr(b);}
		blk_opa(b,BUND,n->c),blk_lit(b,n),expr(b),blk_op3(b,"@ins");return;
	}
	if(matchsp('(')){if(matchsp(')')){blk_lit(b,lml(0));return;}expr(b);expect(')',0);return;}
	str s=token_str(peek());
	if(findop(s.sv,monads)>=0&&strchr("mn",peek()->type)){
		next();if(matchsp('@')){
			lv*names=l_list(lmistr("v"));
			expr(b),blk_op(b,ITER);int head=blk_here(b);blk_lit(b,names);int each=blk_opa(b,EACH,0);
			blk_get(b,names->lv[0]),blk_op1(b,s.sv),blk_opa(b,NEXT,head),blk_sets(b,each,blk_here(b));
		}else{expr(b),blk_op1(b,s.sv);}free(s.sv);return;
	}
	free(s.sv);lv* n=lmstr(name("variable"));
	if(matchsp(':')){expr(b),blk_set(b,n);return;}
	blk_get(b,n);parseindex(b,n);
}
void expr(lv*b){
	term(b);if(strchr("[.",peek()->type)){parseindex(b,NULL);}
	if(matchsp('@')){
		lv*temp=tempname();blk_set(b,temp),blk_op(b,DROP);
		lv*names=lml(3);names->lv[0]=lmistr("v"),names->lv[1]=lmistr("k"),names->lv[2]=lmistr("i");
		expr(b),blk_op(b,ITER);int head=blk_here(b);blk_lit(b,names);int each=blk_opa(b,EACH,0);
			blk_get(b,temp);EACH(z,names)blk_get(b,names->lv[z]);blk_opa(b,BUND,3);blk_op(b,CALL);
		blk_opa(b,NEXT,head),blk_sets(b,each,blk_here(b));return;
	}
	str s=token_str(peek());if(findop(s.sv,dyads)>=0&&strchr("mn",peek()->type)){next(),expr(b),blk_op2(b,s.sv);}free(s.sv);
}
lv* parse(char*text){
	par=(parser){0,0,0,strlen(text),text,{0},{0},"\0"};
	lv*b=lmblk();if(hasnext())expr(b);while(hasnext())blk_op(b,DROP),expr(b);
	if(blk_here(b)==0)blk_lit(b,NONE);return b;
}

// Interpreter

void env_local(lv*e,lv*n,lv*x){SFIND(z,e,n->sv){e->lv[z]=x;return;}ld_add(e,n,x);}
lv* env_getr(lv*e,lv*n){SFIND(z,e,n->sv)return e->lv[z];return e->env?env_getr(e->env,n): NULL;}
void env_setr(lv*e,lv*n,lv*x){SFIND(z,e,n->sv){e->lv[z]=x;return;}if(e->env)env_setr(e->env,n,x);}
lv* env_get(lv*e,lv*n){lv*r=env_getr(e,n);return r?r:NONE;}
void env_set(lv*e,lv*n,lv*x){lv*r=env_getr(e,n);r?env_setr(e,n,x):env_local(e,n,x);}
lv* env_bind(lv*e,lv*k,lv*v){lv*r=lmenv(e);EACH(z,k)env_local(r,k->lv[z],z<v->c?v->lv[z]:NONE);return r;}
#define running()      (state.t->c)
#define ev()           (state.e->lv[state.e->c-1])
#define issue(env,blk) ll_add(state.e,env),ll_add(state.t,blk),idx_push(&state.pcs,0)
#define descope        ll_pop(state.t),ll_pop(state.e),idx_pop(&state.pcs)
#define ret(x)         ll_add(state.p,x)
#define arg()          ll_pop(state.p)
#define getblock()     ll_peek(state.t)
#define getpc()        idx_peek(&state.pcs)

lv*order_vec=NULL;int order_dir=0; // this is gross. qsort() is badly designed, and qsort_r is unportable.
int lex_less(lv*a,lv*b);int lex_more(lv*a,lv*b);// forward refs
int lex_list(lv*x,lv*y,int a,int ix){
	if(x->c<ix&&y->c<ix)return 0;lv*xv=x->c>ix?x->lv[ix]:NONE,*yv=y->c>ix?y->lv[ix]:NONE;
	return lex_less(xv,yv)?a: lex_more(xv,yv)?!a: lex_list(x,y,a,ix+1);
}
int lex_less(lv*a,lv*b){return lil(a)&&lil(b)? lex_list(a,b,1,0): lb(l_less(a,b));}
int lex_more(lv*a,lv*b){return lil(a)&&lil(b)? lex_list(a,b,0,0): lb(l_more(a,b));}
int orderby(const void*av,const void*bv){
	int a=*(int*)av,b=*(int*)bv;
	if(lex_less(order_vec->lv[a],order_vec->lv[b]))return  order_dir;
	if(lex_more(order_vec->lv[a],order_vec->lv[b]))return -order_dir;
	return a-b; // produce a stable sort
}
void docall(lv*f,lv*a,int tail){
	if(linat(f)){ret(((lv*(*)(lv*,lv*))f->f)(f->a,a));return;}
	if(!lion(f)){ret(l_at(f,l_first(a)));return;}
	if(tail){descope;}issue(env_bind(f->env,f,a),f->b);gc.depth=MAX(gc.depth,state.e->c);
}
void runop(){
	lv*b=getblock();if(!liblk(b))ret(ll_pop(state.t));
	int*pc=getpc(),op=blk_getb(b,*pc),imm=(oplens[op]==3?blk_gets(b,1+*pc):0);(*pc)+=oplens[op];
	switch(op){
		case DROP:arg();break;
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
		case QUERY:{
			order_dir=ln(arg());lv*t=arg(),*ct=lmn(t->n);
			lv*o=l_take(ct,ll(arg())),*b=l_take(ct,ll(arg())),*w=l_take(ct,ll(arg()));
			lv*r=l_rows(t);order_vec=o;idx cn=idx_new(r->c),gr=idx_new(r->c),gp=idx_new(r->c);
			lv*keys=lml(0);EACH(z,r){ // find groups
				if(!lb(w->lv[z])){gr.iv[z]=-1;continue;}
				int f=0;EACH(zz,keys)if(equal(keys->lv[zz],b->lv[z])){f=1,cn.iv[zz]++,gr.iv[z]=zz;break;}
				if(!f)ll_add(keys,b->lv[z]),cn.iv[keys->c-1]++,gr.iv[z]=keys->c-1;
			}
			lv*rows=lml(0);EACH(z,keys){ // sort groups, select rows
				int i=0;EACH(zz,r)if(gr.iv[zz]==z)gp.iv[i++]=zz;qsort(gp.iv,i,sizeof(int),orderby);
				GEN(rr,i)r->lv[gp.iv[z]],dset(rr->lv[z],lmistr("gindex"),lmn(z)),dset(rr->lv[z],lmistr("group"),lmn(rows->c));
				ll_add(rows,l_table(rr));
			}if(rows->c==0&&!imm){ll_add(rows,l_take(NONE,t));}
			idx_free(&cn),idx_free(&gr),idx_free(&gp),ret(t),ret(rows);break;
		}
	}while(running()&&*getpc()>=blk_here(getblock()))descope;
}
lv*n_uplevel(lv*self,lv*a){
	(void)self;int i=2;lv*e=ev(),*r=NULL,*name=ls(a);
	while(e&&i){r=NULL;SFIND(z,e,name->sv)r=e->lv[z];if(r)i--;e=e->env;}return r?r:NONE;
}
lv*n_feval(lv*self,lv*a){
	(void)self;lv*r=a->lv[0],*x=a->lv[1];dset(r,lmistr("value"),x);
	lv*b=dget(r,lmistr("vars"));EACH(z,ev())dset(b,lmcstr(ev()->kv[z]->sv),ev()->lv[z]);return r;
}
lv*n_eval(lv*self,lv*a){
	(void)self;lv*y=a->c>1?ld(a->lv[1]):lmd(),*r=lmd();DMAP(yy,y,y->lv[z]);
	dset(r,lmistr("value"),NONE),dset(r,lmistr("vars"),yy);
	lv* prog=parse(ls(l_first(a))->sv);dset(r,lmistr("error"),perr()?lmcstr(par.error):lms(0));
	if(perr())return r;GEN(k,yy->c)yy->kv[z];GEN(v,yy->c)yy->lv[z];
	blk_opa(prog,BUND,2),blk_lit(prog,lmnat(n_feval,NULL)),blk_op(prog,SWAP),blk_op(prog,CALL);
	issue(env_bind(a->c>2&&lb(a->lv[2])?ev():NULL,k,v),prog);return r;
}
void init_interns(){for(int z=0;z<100;z++){lv*t=&interned[z];t->t=0,t->c=1,t->nv=z;}}
void init(lv*e){state.p=lml(0),state.t=lml(0),state.e=lml(0),state.pcs=idx_new(0);ll_add(state.e,e);}
void pushstate(lv*e){
	if(!state.p)printf("trying to save an uninitialized state!\n");
	gc.st[gc.ss++]=state;init(e);
}
void popstate(){
	idx_free(&state.pcs);
	if(gc.ss){state=gc.st[--gc.ss];}else{printf("state underflow!\n");}
}
void runexpr(lv*env,lv*x){issue(env,x);}
void runfunc(lv*env,lv*f,lv*args){
	if(f&&lion(f)){lv*b=lmblk();blk_lit(b,f),blk_lit(b,args),blk_op(b,CALL),blk_op(b,DROP);issue(env,b);}
}
void halt(){state.e->c=0,state.t->c=0,state.p->c=0,state.pcs.c=0;}
lv*run(lv*x,lv*rootenv){
	init(rootenv),issue(rootenv,x);int c=0;while(running()){runop(),c++;if(c%100==0)lv_collect();}
	if(state.p->c<1)return NONE;lv*r=arg();
	while(state.p->c)printf("STACK JUNK: "),debug_show(arg());return r;
}

// Standard Library

void show(str*s,lv*x,int toplevel){
	if(!x){str_addz(s,"<NULL>");}
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
	else if(lit(x)){
		if(!toplevel){char t[64];str_add(s,t,snprintf(t,sizeof(t),"<TABLE...%d>",x->c));return;}
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
lv*debug_show(lv*x){str s=str_new();show(&s,x,1);printf("%s\n",lmstr(s)->sv);return x;}
lv*n_printf(lv*a,int newline,FILE*out){
	if(a->c<2){fprintf(out,"%s",ls(l_first(a))->sv);}
	else{a=l_format(ls(l_first(a)),l_drop(ONE,a));fprintf(out,"%s",a->sv);}
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
	n=0;while(i<=t->c){
		css;str val=str_new();
		if(cm('"'))while(i<t->c)if(cm('"')){if(cm('"'))str_addc(&val,'"');else break;}else str_addc(&val,t->sv[i++]);
		else{while(i<t->c&&!strchr("\n\"",t->sv[i])&&t->sv[i]!=delim)str_addc(&val,t->sv[i++]);}
		if(n<s->c&&s->sv[n]!='_'){char f[]={'%',fchar(s->sv[n]),0};ll_add(r->lv[slot++],l_parse(lmcstr(f),lmstr(val)));}
		else{free(val.sv);}n++;
		if(i>=t->c||t->sv[i]=='\n'){
			while(n<s->c){char u=s->sv[n++];if(u!='_'&&slot<slots)ll_add(r->lv[slot++],strchr("sluvroq",u)?lms(0):NONE);}
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
void writexmlrec(str*s,lv*x,int tab){
	if(lil(x)){EACH(z,x)writexmlrec(s,x->lv[z],tab);return;}
	if(!lid(x)){writexmlstr(s,ls(x));if(tab)str_addc(s,'\n');return;}
	lv*t=ls(dgetv(x,lmistr("tag"))),*a=ld(dgetv(x,lmistr("attr"))),*c=dget(x,lmistr("children"));
	str_addc(s,'<'),str_addz(s,t->sv);EACH(z,a){
		str_addc(s,' '),str_addz(s,ls(a->kv[z])->sv);
		str_addz(s,"=\""),writexmlstr(s,ls(a->lv[z])),str_addc(s,'"');
	}
	c=c?ll(c):lml(0);if(!c->c){str_addz(s,"/>\n");return;}str_addz(s,">\n");
	#define indent(n) for(int z=0;z<n;z++)str_addc(s,' ');
	EACH(z,c){indent(tab+2);writexmlrec(s,c->lv[z],tab+2);}
	indent(tab);str_addz(s,"</"),str_addz(s,t->sv);str_addz(s,">\n");
}
lv*n_writexml(lv*self,lv*a){(void)self;str r=str_new();writexmlrec(&r,l_first(a),0);return lmstr(r);}
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
	str r=str_new();xs;while(!strchr(">/= \n\0",t[*i]))str_addc(&r,tolower(t[(*i)++]));xs;return lmstr(r);
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
lv* time_ms(){
	FILETIME t; GetSystemTimeAsFileTime(&t);
	ULARGE_INTEGER i; i.LowPart=t.dwLowDateTime, i.HighPart=t.dwHighDateTime;
	return lmn(i.QuadPart*1e-4);
}
#else
#ifndef __COSMOPOLITAN__
#include <sys/time.h>
#endif
lv* time_ms(){
	struct timeval now;gettimeofday(&now,NULL);
	return lmn((((long long)now.tv_sec)*1000)+(now.tv_usec/1000));
}
#endif

int randint(int x){unsigned int y=seed;y^=(y<<13),y^=(y>>17),(y^=(y<<15));return mod(seed=y,x);}
lv* n_random(lv*self,lv*z){
	#define rand_elt lin(x)?lmn(randint(ln(x))): x->c<1?NONE: lis(x)?l_at(x,lmn(randint(x->c))): x->lv[randint(x->c)]
	(void)self;lv*x=l_first(z);if(z->c<2)return rand_elt;
	int y=ln(z->lv[1]);if(y>=0){GEN(r,y)rand_elt;return r;}
	x=lin(x)?l_range(x):ll(x);idx pv=idx_new(x->c);EACH(z,x)pv.iv[z]=z;
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
	}return x?x:NONE;(void)self;
}
