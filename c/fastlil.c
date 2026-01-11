// Learning in Layers
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

struct num_payload{double v;};
struct str_payload{int size;char v[1];};                        // linear char buffer
struct lst_payload{int size,capacity;struct lvs*v[1];};         // pointer array
struct dic_payload{int size,capacity;struct lvs*parent,*v[1];}; // alternating k(any)/v(any) entries
struct tab_payload{int rows,cols,capacity;struct lvs*v[1];};    // alternating k(str)/v(lst) entries
struct fun_payload{struct lvs*name,*args,*body,*env;};
struct ifc_payload{struct lvs*name,*self;struct lvs*(*f)(struct lvs*,struct lvs*,struct lvs*);};
struct nat_payload{struct lvs      *self;struct lvs*(*f)(struct lvs*,struct lvs*            );};
struct blk_payload{int lsize,lcapacity;struct lvs**l;int bsize,bcapacity;char*b;}; // (locals), (bytecode)
typedef struct lvs{
	char type;
	int g; // garbage collection generaton tag
	union{
		struct num_payload num;
		struct str_payload str;
		struct lst_payload lst;
		struct dic_payload dic;
		struct tab_payload tab;
		struct fun_payload fun;
		struct ifc_payload ifc;
		struct nat_payload nat;
		struct blk_payload blk;
	};
}lv;

#define MAX(a,b)  ((a)<(b)?(b):(a))
#define MIN(a,b)  ((a)<(b)?(a):(b))
#define LNIL      (&nilvalue)
#define EMPTYLIST (&emptylistvalue)
#define NUM       512 // number parsing/formatting buffer size
#define ONE       lmn(1)
#define ZERO      lmn(0)
int     mod(int    x,int    y){x=y==0?0:x%y      ;if(x<0)x+=y;return x;}
double dmod(double x,double y){x=y==0?0:fmod(x,y);if(x<0)x+=y;return x;}

lv nilvalue={0};
lv emptylistvalue={3,0,{0}};
int linil(lv*x){return x==&nilvalue;}
int lin  (lv*x){return x!=NULL&&x->type==1;}
int lis  (lv*x){return x!=NULL&&x->type==2;}
int lil  (lv*x){return x!=NULL&&x->type==3;}
int lid  (lv*x){return x!=NULL&&x->type==4;}
int lit  (lv*x){return x!=NULL&&x->type==5;}
int lifun(lv*x){return x!=NULL&&x->type==6;}
int lii  (lv*x){return x!=NULL&&x->type==7;}
int linat(lv*x){return x!=NULL&&x->type==8;}
int liblk(lv*x){return x!=NULL&&x->type==9;}

#define str_count(x) (x->str.size)
#define lst_count(x) (x->lst.size)
#define dic_count(x) (x->dic.size)
#define tab_cols(x)  (x->tab.cols)
#define tab_rows(x)  (x->tab.rows)
#define sget(x,n)    (x->str.v[n])
int str_cmp(lv*x,lv*y);
lv*lget(lv*x,int n);
lv*dgetk(lv*x,int n);lv*dgetv(lv*x,int n);lv*lmdsize(int n);void dseti(lv*x,int n,lv*k,lv*v);
lv*tgetk(lv*x,int n);lv*tgetv(lv*x,int n);lv*tgetc(lv*x,int c,int r);lv* tgetrow(lv*x,int r);

// Dynamic Strings

typedef struct{int c,size;char*sv;}str;
str str_new(void){return(str){0,32,calloc(32,1)};}
void str_free(str s){free(s.sv);}
void str_addraw(str*s,int x){if(s->c+1>=s->size)s->sv=realloc(s->sv,s->size*=2);s->sv[s->c++]=x;}
void str_addc(str*s,char x){if(x!='\r')str_addraw(s,x=='\t'?' ':((0xFF&x)>=32)||x=='\n'?x:255);}
void str_term(str*s){str_addraw(s,'\0');}
void str_add(str*s,char*x,int n){for(int z=0;z<n;z++)str_addc(s,x[z]);}
void str_addl(str*s,lv*x){str_add(s,x->str.v,str_count(x));}
void str_addz(str*s,char*x){str_add(s,x,strlen(x));}
void crev(char*t,int n){int i=0,j=n-1;while(i<j){char v=t[i];t[i]=t[j],t[j]=v,i++,j--;}}
void str_addn(str*x,double y){
	if(y<0)y=-y,str_addc(x,'-');char t[NUM*2]={0};int n=0,s=0;
	t[n++]='0';double i=floor(y);y=round((y-floor(y))*1000000.0);if(y>=1000000)i++;
	while(i>0){t[n++]=fmod(i,10)+'0',i=i/10;}crev(t+1,n-1);while(t[s]=='0'&&t[s+1])s++;t[n++]='.';
	for(int z=0;z<6;z++){t[n+5-z]=fmod(y,10)+'0',y=y/10;}n+=5;while(n>0&&t[n]=='0')n--;if(t[n]=='.')n--;
	str_add(x,t+s,n-s+1);
}

// DeckRoman Character Encoding
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
char DROM_INLEN[]={
	3,2,3,2,3,2,3,2,3,2,3,2,3,2,2,3,
	2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
	2,2,3,2,3,2,3,2,3,2,3,2,3,2,2,3,
	2,3,2,3,2,3,2,3,2,2,2,3,2,3,2,3,
	2,3,2,3,2,3,2,2,3,2,3,2,3,2,3,2,
	3,2,3,2,3,2,3,2,3,2,2,3,2,3,2,3,
	2,3,2,3,2,3,2,2,3,2,3,2,3,2,3,2,
	3,2,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
	2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
	2,2,2,2,3,2,3,2,3,2,3,2,3,2,3,2,
	2,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,
	3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,
	3,2,3,2,3,2,3,3,2,2,2,2,3,2,3,3,
	3,3, 0
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
#define drom_toupper(c) (DROM_TOUPPER[0xFF&(c)])
#define drom_tolower(c) (DROM_TOLOWER[0xFF&(c)])
char* drom_to_utf8(lv*x){
	str r=str_new();
	for(int z=0;z<str_count(x);z++){char*o=DROM_OUTPUT[0xFF&sget(x,z)];while(*o){str_addraw(&r,*o);o++;};}
	str_term(&r);return r.sv;
}
void utf8_to_drom(str*s,char*x,int n){
	for(int z=0;z<n;z++){unsigned char c=x[z];
		if     (c=='\r')continue;             // suppress windows newlines
		else if(c=='\t')c=' ';                // resolve the tabs vs. spaces debate once and for all
		else if(c<' '&&c!='\n'){c=255;}       // unix newlines are the only control code we respect
		else if(c>'~'||((z+1<n)&&(0xFF&x[z+1])==0xCC)){ // possible combining accent
			int f=0;for(int i=0;DROM_INKEY[i];i++){
				int ln=DROM_INLEN[i],ff=0;
				if(z+ln<=n){
					if(ln==2&&DROM_INKEY[i][0]==x[z]&&DROM_INKEY[i][1]==x[z+1]                          )ff=1;
					if(ln==3&&DROM_INKEY[i][0]==x[z]&&DROM_INKEY[i][1]==x[z+1]&&DROM_INKEY[i][2]==x[z+2])ff=1;
				}
				if(ff){c=DROM_INVAL[i];z+=(ln-1);f=1;break;}
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
char drom_from_codepoint(int n){
	// if we return 0, the result should be understood as 'no character'!
	if(n=='\r')return 0;if(n=='\t')return ' ';if(n<' '&&n!='\n')return 0xFF;if(n<='~')return n;
	for(int i=0;DROM_INPOINT[i];i++)if(n==DROM_INPOINT[i])return DROM_INVAL[i];return 0xFF;
}

// Common Utilities

int count(lv*x){
	return lin(x)?1:
	       lis(x)?str_count(x):
	       lil(x)?lst_count(x):
	       lid(x)?dic_count(x):
	       lit(x)?tab_rows(x): 0;
}
int matchr(lv*x,lv*y){
	if(x==y)return 1;
	if(x->type!=y->type)return 0;
	if(lin(x))return x->num.v==y->num.v;
	if(lis(x))return str_count(x)==str_count(y)&&!str_cmp(x,y);
	if(lil(x)){
		if(lst_count(x)!=lst_count(y))return 0;
		for(int z=0;z<lst_count(x);z++)if(!matchr(lget(x,z),lget(y,z)))return 0;return 1;
	}
	if(lid(x)){
		if(dic_count(x)!=dic_count(y))return 0;
		for(int z=0;z<dic_count(x);z++)if(!matchr(dgetk(x,z),dgetk(y,z))||!matchr(dgetv(x,z),dgetv(y,z)))return 0;return 1;
	}
	if(lit(x)){
		if((tab_rows(x)!=tab_rows(y))||(tab_cols(x)!=tab_cols(y)))return 0;
		for(int z=0;z<tab_cols(x);z++)if(!matchr(tgetk(x,z),tgetk(y,z))||!matchr(tgetv(x,z),tgetv(y,z)))return 0;return 1;
	}
	return 0;
}
void show(str*s,lv*x,int toplevel){
	if(!x){str_addz(s,"<NULL>");}
	else if(linil(x)){if(!toplevel)str_addz(s,"nil");}
	else if(lin(x)){str_addn(s,x->num.v);}
	else if(lil(x)){
		str_addc(s,'(');for(int z=0;z<lst_count(x);z++){
			if(z)str_addc(s,',');show(s,lget(x,z),0);}
		str_addc(s,')');
	}
	else if(lid(x)){
		str_addc(s,'{');for(int z=0;z<dic_count(x);z++){
			if(z)str_addc(s,',');show(s,dgetk(x,z),0),str_addc(s,':'),show(s,dgetv(x,z),0);
		}str_addc(s,'}');
	}
	else if(lis(x)){
		str_addc(s,'"');for(int z=0;z<str_count(x);z++){
			char c=sget(x,z);if(c=='\n'?(c='n',1):!!strchr("\"\\",c))str_addc(s,'\\');str_addc(s,c);
		}str_addc(s,'"');
	}
	else if(lifun(x)){
		str_addz(s,"on "),str_addl(s,x->fun.name);
		for(int z=0;z<lst_count(x->fun.args);z++)str_addc(s,' '),str_addl(s,lget(x->fun.args,z));
		str_addz(s," do ... end");
	}
	else if(linat(x)){str_addz(s,"on native x do ... end");}
	else if(lit(x)&&!toplevel){
		str_addz(s,"insert ");for(int z=0;z<tab_cols(x);z++)str_addl(s,tgetk(x,z)),str_addc(s,' ');
		str_addz(s,"with ");for(int r=0;r<tab_rows(x);r++)for(int c=0;c<tab_cols(x);c++)show(s,lget(tgetv(x,c),r),0),str_addc(s,' ');
		str_addz(s,"end");return;
	}
	else if(lit(x)){
		if(tab_cols(x)==0){str_addz(s,"++\n||\n++");return;}
		#define TABLE_CELL_SIZE 41
		int*w=calloc(tab_cols(x),sizeof(int));
		char*cells=calloc(tab_cols(x)*(tab_rows(x)+1)*TABLE_CELL_SIZE,sizeof(char));
		#define scell(c,r) (cells+(((tab_rows(x)+1)*TABLE_CELL_SIZE)*(c)+(TABLE_CELL_SIZE*(r))))
		for(int c=0;c<tab_cols(x);c++){
			w[c]=snprintf(scell(c,0),TABLE_CELL_SIZE,"%s",tgetk(x,c)->str.v);
			for(int r=0;r<tab_rows(x);r++){
				str i=str_new();show(&i,tgetc(x,c,r),0),str_term(&i),snprintf(scell(c,r+1),TABLE_CELL_SIZE,"%s",i.sv);str_free(i);
				if(i.c-1>w[c])w[c]=MIN(TABLE_CELL_SIZE-1,i.c-1);
			}
		}
		#define sep()     str_addc(s,'+');for(int c=0;c<tab_cols(x);c++){for(int i=0;i<w[c]+2;i++)str_addc(s,'-');str_addc(s,'+');}
		#define sepn()    sep()str_addc(s,'\n');
		#define dats(r,c) {int n=strlen(scell(c,r));str_addc(s,i<n?scell(c,r)[i]:' ');}str_add(s," | ",c==tab_cols(x)-1?2:3);
		#define dat(r)    str_addz(s,"| ");for(int c=0;c<tab_cols(x);c++){for(int i=0;i<w[c];i++)dats(r,c)}str_addc(s,'\n');
		sepn()dat(0)if(tab_rows(x)){sepn()}for(int r=0;r<tab_rows(x);r++){dat(r+1)}sep();free(w),free(cells);
	}
	else if(lii(x)){str_addc(s,'<'),str_addl(s,x->ifc.name),str_addc(s,'>');}
	else if(liblk(x)){str_addz(s,"<BLOCK>");}
	else{char t[64];str_add(s,t,snprintf(t,sizeof(t),"<INVALID: %d>",x->type));}
}

// Heap and Garbage Collector

typedef struct{lv*p,*t,*e;}pstate;pstate state={0}; // parameters, tasks, envs, index
typedef struct{int lo,hi,live,size,generation,ss;lv**heap;long frees,allocs,depth;pstate st[4];}gc_state;gc_state gc={0};
void lv_walk(lv*x){
	if(x==NULL||x->g==gc.generation){return;}x->g=gc.generation;
	if     (lin  (x)){}
	else if(lis  (x)){}
	else if(lil  (x)){for(int z=0;z<lst_count(x);z++){lv_walk(lget(x,z));}}
	else if(lid  (x)){for(int z=0;z<dic_count(x);z++){lv_walk(dgetk(x,z)),lv_walk(dgetv(x,z));}lv_walk(x->dic.parent);}
	else if(lit  (x)){for(int z=0;z<tab_cols (x);z++){lv_walk(tgetk(x,z)),lv_walk(tgetv(x,z));}}
	else if(lifun(x)){lv_walk(x->fun.name),lv_walk(x->fun.args),lv_walk(x->fun.body),lv_walk(x->fun.env);}
	else if(lii  (x)){lv_walk(x->ifc.name),lv_walk(x->ifc.self);}
	else if(liblk(x)){for(int z=0;z<x->blk.lsize;z++)lv_walk(x->blk.l[z]);}
}
void lv_free(lv*x){
	if(x==NULL)return;
	if(liblk(x))free(x->blk.l),free(x->blk.b);
	free(x);gc.frees++,gc.live--;
}
void lv_grow(void){
	gc.heap=realloc(gc.heap,(gc.size*2)*sizeof(lv*));
	memset(gc.heap+gc.size,0,gc.size*sizeof(lv*));gc.size*=2;
}
void lv_collect(void){
	if(gc.live+(gc.size*0.1)<gc.size)return;gc.generation++;
	for(int z=0;z<gc.ss;z++){lv_walk(gc.st[z].e),lv_walk(gc.st[z].p),lv_walk(gc.st[z].t);}
	lv_walk(state.e),lv_walk(state.p),lv_walk(state.t);
	for(int z=0;z<gc.hi;z++)if(gc.heap[z]){
		if(gc.heap[z]->g!=gc.generation){lv_free(gc.heap[z]),gc.heap[z]=NULL,gc.lo=MIN(gc.lo,z);}
		else{gc.hi=MAX(gc.hi,z);}
	}
}
int lv_stash(lv*x){
	while(gc.lo<gc.size&&gc.heap[gc.lo]!=NULL)gc.lo++;
	return(gc.lo>=gc.size)?0: (gc.heap[gc.lo]=x,gc.hi=MAX(gc.hi,gc.lo),gc.lo++,1);
}
lv* lmv(int type,size_t capacity){
	gc.allocs++,gc.live++;lv*r=calloc(1,sizeof(lv)+capacity);r->type=type;
	if(gc.heap==NULL){gc.size=64,gc.heap=calloc(gc.size,sizeof(lv*));}
	if(lv_stash(r))return r;lv_grow();lv_stash(r);return r;
}

// Number values (1)
lv interned_nums[383]={0};
void init_interns(void){for(int z=0;z<383;z++){lv*t=&interned_nums[z];t->type=1,t->num.v=z-128;}}
lv* lmn(double x){
	if(x==floor(x)&&x>=-128&&x<255)return &interned_nums[((int)x)+128];
	if(!isfinite(x))return lmn(0);
	lv*r=lmv(1,0);r->num.v=x;return r;
}
int lb(lv*x){return linil(x)?0: lin(x)?x->num.v!=0: lis(x)||lil(x)||lid(x)?count(x)!=0: 1;}
double rnum_len(char*x,int n,int*len){
	if(n==0)return 0;int i=0,sign=1;double r=0,p=10;while(isspace(x[i]))i++;
	if(x[0]=='-')sign=-1,i++;while(i<n&&isdigit(x[i]))r=r*10+(x[i]-'0'),i++;
	if(x[i]=='.')        i++;while(i<n&&isdigit(x[i]))r+=(x[i++]-'0')/p,p*=10;
	return (*len)=i,sign*r;
}
double rnum(char*x,int n){int i=0;return rnum_len(x,n,&i);}
double ln(lv*x){
	return lin(x)?x->num.v: linil(x)?0:
	       lis(x)?rnum(x->str.v,str_count(x)):
	       lil(x)&&lst_count(x)?ln(lget(x,0)):
	       lid(x)&&dic_count(x)?ln(dgetv(x,0)): 0;
}
lv* lbv(lv*x){return lmn(ln(x));}

// String values (2)
lv*interned_strings[512];char*interned_chars[512];unsigned int interned_string_count=0;
lv* lms(int n){lv*r=lmv(2,sizeof(char)*(n+1));r->str.size=n;return r;}
lv* lmstr(str x){str_term(&x);lv*r=lms(x.c);memcpy(r->str.v,x.sv,x.c);str_free(x);return r;}
lv* lmcstr(char*x){int n=strlen(x);lv*r=lms(n);memcpy(r->str.v,x,n);return r;}
lv* lmutf8(char*x){str r=str_new();utf8_to_drom(&r,x,strlen(x));return lmstr(r);}
lv* lmistr(char*x){
	for(unsigned int z=0;z<interned_string_count;z++){if(interned_chars[z]==x)return interned_strings[z];}
	lv*r=lmcstr(x);interned_chars[interned_string_count]=x;interned_strings[interned_string_count++]=r;
	if(interned_string_count>sizeof(interned_strings))printf("warning: intern heap is full!\n");return r;
}
int tnames=0;lv* tempname(void){char t[64];snprintf(t,sizeof(t),"@t%d",tnames++);return lmcstr(t);}
int str_cmp(lv*x,lv*y){return strcmp(x->str.v,y->str.v);}
lv* sgetchar(lv*x,int n){lv*r=lms(1);r->str.v[0]=sget(x,n);return r;}
lv* ls(lv*x){
	if(lis(x))return x;
	if(lin(x)){str r=str_new();str_addn(&r,ln(x));return lmstr(r);}
	if(lil(x)){str r=str_new();for(int z=0;z<lst_count(x);z++)str_addl(&r,ls(lget(x,z)));return lmstr(r);}
	return lms(0);
}
lv* debug_show(lv*x){str s=str_new();show(&s,x,1);char*r=drom_to_utf8(lmstr(s));printf("%s\n",r),free(r);return x;}

// List values (3)
lv* lml(int n){int c=MAX(8,n);lv*r=lmv(3,sizeof(lv*)*c);r->lst.size=n;r->lst.capacity=c;return r;}
lv* lst_clone(lv*x){lv*r=lml(lst_count(x));memcpy(r->lst.v,x->lst.v,sizeof(lv*)*lst_count(x));return r;}
lv* lget(lv*x,int n){return x->lst.v[n];}
lv* lgetmod(lv*x,int n){return lst_count(x)==0?LNIL: lget(x,n%lst_count(x));}
void lset(lv*x,int n,lv*y){x->lst.v[n]=y;}
lv* lml1(lv*x){lv*r=lml(1);lset(r,0,x);return r;}
lv* lml2(lv*x,lv*y){lv*r=lml(2);lset(r,0,x),lset(r,1,y);return r;}
lv* ll(lv*x){
	if(lil(x))return x;
	if(linil(x))return EMPTYLIST;
	if(lid(x)){lv*r=lml(dic_count(x));for(int z=0;z<dic_count(x);z++)lset(r,z,dgetv(x,z));return r;}
	if(lis(x)){lv*r=lml(str_count(x));for(int z=0;z<str_count(x);z++)lset(r,z,sgetchar(x,z));return r;}
	if(lit(x)){lv*r=lml(tab_rows(x));for(int z=0;z<tab_rows(x);z++)lset(r,z,tgetrow(x,z));return r;}
	return lml1(x);
}

// Dict values (4)
lv* lmd(void){int n=16;lv*r=lmv(4,sizeof(lv*)*n*2);r->dic.capacity=n;              return r;}
lv* lmdsize(int n){    lv*r=lmv(4,sizeof(lv*)*n*2);r->dic.capacity=n;r->dic.size=n;return r;}
lv* dic_clone(lv*x){lv*r=lmdsize(dic_count(x));memcpy(r->dic.v,x->dic.v,sizeof(lv*)*dic_count(x)*2);return r;}
lv* dgetk(lv*x,int n){return x->dic.v[n*2  ];}
lv* dgetv(lv*x,int n){return x->dic.v[n*2+1];}
void dseti(lv*x,int n,lv*k,lv*v){x->dic.v[n*2]=k,x->dic.v[n*2+1]=v;}
lv* ld(lv*x){
	if(lid(x))return x;
	if(lil(x)){lv*r=lmdsize(lst_count(x));for(int z=0;z<lst_count(x);z++)dseti(r,z,lmn(z),lget(x,z));return r;}
	if(lis(x)){lv*r=lmdsize(str_count(x));for(int z=0;z<str_count(x);z++)dseti(r,z,lmn(z),sgetchar(x,z));return r;}
	if(lit(x)){lv*r=lmdsize(tab_cols (x));for(int z=0;z<tab_cols (x);z++)dseti(r,z,tgetk(x,z),tgetv(x,z));return r;}
	return lmd();
}

// Table values (5)
lv* lmt(void){int n=16;lv*r=lmv(5,sizeof(lv*)*n*2);r->tab.capacity=n;              return r;}
lv* lmtsize(int n){   ;lv*r=lmv(5,sizeof(lv*)*n*2);r->tab.capacity=n;r->tab.cols=n;return r;}
lv* tgetk(lv*x,int c){return x->tab.v[c*2  ];}
lv* tgetv(lv*x,int c){return x->tab.v[c*2+1];}
lv* tgetc(lv*x,int c,int r){return lget(tgetv(x,c),r);}
void tseti(lv*x,int n,lv*k,lv*v){x->tab.v[n*2]=k,x->tab.v[n*2+1]=v;}
lv* tgetrow(lv*x,int n){lv*r=lmdsize(tab_cols(x));for(int c=0;c<tab_cols(x);c++)dseti(r,c,tgetk(x,c),lget(tgetv(x,c),n));return r;}
lv* lt(lv*x){
	if(lit(x))return x;
	if(linil(x))return lmt();
	if(lil(x)){lv*r=lmtsize(1);tseti(r,0,lmistr("value"),lst_clone(x));tab_rows(r)=lst_count(x);return r;}
	if(!lid(x)){lv*r=lmtsize(1),*v=ll(x);tseti(r,0,lmistr("value"),v);tab_rows(r)=lst_count(v);return r;}
	lv*r=lmtsize(2),*k=lml(dic_count(x)),*v=lml(dic_count(x));
	for(int z=0;z<dic_count(x);z++)lset(k,z,dgetk(x,z)),lset(v,z,dgetv(x,z));
	tseti(r,0,lmistr("key"),k),tseti(r,0,lmistr("value"),v);tab_rows(r)=dic_count(x);return r;
}

// Function values (6)
lv* lmfun(lv*name,lv*args,lv*body){lv*r=lmv(6,0);r->fun.name=name,r->fun.args=args,r->fun.body=body;return r;}
lv* fun_bind(lv*f,lv*env){lv*r=lmv(6,0);r->fun.name=f->fun.name,r->fun.args=f->fun.args,r->fun.body=f->fun.body,r->fun.env=env;return r;}
#define fun_name(x) (x->fun.name)
#define fun_args(x) (x->fun.args)

// Interface values (7)
lv* lmi(lv*(*f)(lv*,lv*,lv*),lv*name,lv*self){lv*r=lmv(7,0);r->ifc.f=f,r->ifc.name=name,r->ifc.self=self;return r;}
#define ifc_name(x) (x->ifc.name)

// Native values (8)
lv* lmnat(lv*(*f)(lv*,lv*),lv*self){lv*r=lmv(8,0);r->nat.f=f,r->nat.self=self;return r;}

// Block values (9)
lv* lmblk(void){
	lv*r=lmv(9,0);
	r->blk.lsize=0;r->blk.lcapacity=16;r->blk.l=calloc(r->blk.lcapacity,sizeof(lv*));
	r->blk.bsize=0;r->blk.bcapacity=32;r->blk.b=calloc(r->blk.bcapacity,sizeof(char));
	return r;
}
int blk_addl(lv*x,lv*k){
	for(int z=0;z<x->blk.lsize;z++)if(matchr(x->blk.l[z],k))return z;
	if(x->blk.lcapacity<x->blk.lsize+1)x->blk.l=realloc(x->blk.l,(x->blk.lcapacity*=2)*sizeof(lv*));x->blk.l[x->blk.lsize++]=k;
	if(x->blk.lsize>=65536)printf("TOO MANY LOCALS!\n"),exit(1);return x->blk.lsize-1;
}
void blk_addb(lv*x,int n){
	if(x->blk.bcapacity<x->blk.bsize+1)x->blk.b=realloc(x->blk.b,(x->blk.bcapacity*=2)*sizeof(char));x->blk.b[x->blk.bsize++]=n;
	if(x->blk.bsize>=65536)printf("TOO MUCH BYTECODE!\n"),exit(1);
}
enum opcodes {JUMP,JUMPF,JUMPT,LIT,DUP,DROP,SWAP,OVER,BUND,OP1,OP2,OP3,GET,SET,LOC,AMEND,TAIL,CALL,BIND,ITER,EACH,NEXT,COL,IPRE,IPOST,FIDX,FMAP};
int oplens[]={3   ,3    ,3    ,3  ,1  ,1   ,1   ,1   ,3   ,3  ,3  ,3  ,3  ,3  ,3  ,3    ,1   ,1   ,1   ,1   ,3   ,3   ,1  ,3   ,3    ,3   ,3   };
int  blk_here(lv*x){return x->blk.bsize;}
void blk_setb(lv*x,int i,int n){x->blk.b[i]=n&0xFF;}
int  blk_getb(lv*x,int i){return 0xFF&(x->blk.b[i]);}
void blk_adds(lv*x,int n){blk_addb(x,0xFF&(n>>8)),blk_addb(x,0xFF&n);}
void blk_sets(lv*x,int i,int n){blk_setb(x,i,n>>8),blk_setb(x,i+1,n);}
int  blk_gets(lv*x,int i){return 0xFFFF&(blk_getb(x,i)<<8|blk_getb(x,i+1));}
void blk_op  (lv*x,int o){blk_addb(x,o);if(o==COL)blk_addb(x,SWAP);}
int  blk_opa (lv*x,int o,int i){blk_addb(x,o),blk_adds(x,i);return blk_here(x)-2;}
void blk_imm(lv*x,int o,lv*k){blk_opa(x,o,blk_addl(x,k));}
lv* blk_getimm(lv*x,int i){return x->blk.l[i];}
typedef struct{char*name;void*func;}primitive;
int findop(char*n,primitive*p){if(n)for(int z=0;p[z].name[0];z++)if(!strcmp(n,p[z].name))return z;return -1;}
#define blk_op1(x,n) blk_opa(x,OP1,findop(n,monads))
#define blk_op2(x,n) blk_opa(x,OP2,findop(n,dyads ))
#define blk_op3(x,n) blk_opa(x,OP3,findop(n,triads))
#define blk_lit(x,v) blk_imm(x,LIT,v)
#define blk_set(x,n) blk_imm(x,SET,n)
#define blk_loc(x,n) blk_imm(x,LOC,n)
#define blk_get(x,n) blk_imm(x,GET,n)
void blk_cat(lv*x,lv*y){
	int z=0,base=blk_here(x);while(z<blk_here(y)){
		int b=blk_getb(y,z);if(b==LIT||b==GET||b==SET||b==LOC||b==AMEND){blk_imm(x,b,blk_getimm(y,blk_gets(y,z+1)));}
		else if(b==JUMP||b==JUMPF||b==JUMPT||b==EACH||b==NEXT||b==FIDX){blk_opa(x,b,blk_gets(y,z+1)+base);}
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

// Primitives

lv* l_count(lv*x){return lmn(count(x));}
lv* l_list(lv*x){return lml1(x);}
lv* l_rows(lv*x){return ll(lt(x));}// TODO: specialize?
lv* l_cols(lv*x){return ld(lt(x));}// TODO: specialize?
lv* l_first(lv*x){
	if(linat(x))return lmistr("native");
	if(lifun(x))return fun_name(x);
	if(lis  (x))return !str_count(x)?LNIL: sgetchar(x,0);
	if(lil  (x))return !lst_count(x)?LNIL: lget(x,0);
	if(lid  (x))return !dic_count(x)?LNIL: dgetv(x,0);
	if(lit  (x))return !tab_rows (x)?LNIL: tgetrow(x,0);
	return x;
}
lv* l_last(lv*x){
	if(lis  (x))return !str_count(x)?LNIL: sgetchar(x,str_count(x)-1);
	if(lil  (x))return !lst_count(x)?LNIL: lget(x,lst_count(x)-1);
	if(lid  (x))return !dic_count(x)?LNIL: dgetv(x,dic_count(x)-1);
	if(lit  (x))return !tab_rows (x)?LNIL: tgetrow(x,tab_rows(x)-1);
	return x;
}
lv* l_keys(lv*x){
	if(lifun(x))return fun_args(x);
	if(lis(x)){lv*r=lml(str_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,lmn(z));return r;}
	if(lil(x)){lv*r=lml(lst_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,lmn(z));return r;}
	if(lid(x)){lv*r=lml(dic_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,dgetk(x,z));return r;}
	if(lit(x)){lv*r=lml(tab_cols (x));for(int z=0;z<lst_count(r);z++)lset(r,z,tgetk(x,z));return r;}
	return EMPTYLIST;
}
lv* l_range(lv*x){
	if(lin(x)){lv*r=lml(ln(x))       ;for(int z=0;z<lst_count(r);z++)lset(r,z,lmn(z));return r;}
	if(lis(x)){lv*r=lml(str_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,sgetchar(x,z));return r;}
	if(lil(x)){lv*r=lml(lst_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,lget(x,z));return r;}
	if(lid(x)){lv*r=lml(dic_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,dgetv(x,z));return r;}
	if(lit(x)){lv*r=lml(tab_cols (x));for(int z=0;z<lst_count(r);z++)lset(r,z,tgetv(x,z));return r;}
	return EMPTYLIST;
}
lv* l_typeof(lv*x){
	if(linil(x))return lmistr("nil");
	if(lin  (x))return lmistr("number");
	if(lis  (x))return lmistr("string");
	if(lil  (x))return lmistr("list");
	if(lid  (x))return lmistr("dict");
	if(lit  (x))return lmistr("table");
	if(lifun(x))return lmistr("function");
	if(lii  (x))return ifc_name(x);
	if(linat(x))return lmistr("function");
	return lmistr("INTERNAL/UNKNOWN");
}
lv* l_table(lv*x){
	if(lid(x)){
		lv*r=lmtsize(dic_count(x));for(int z=0;z<tab_cols(r);z++)tseti(r,z,dgetk(x,z),dgetv(x,z));
		// return torect(t); // TODO! rectangularize AND mark table rowcount...
		return r;
	}
	if(!lil(x))return lt(x);
	int all_dic=1,all_lst=1,m=0;for(int z=0;z<lst_count(x);z++){
		lv*e=lget(x,z);
		all_dic&=linil(e)||lid(e);
		all_lst&=linil(e)||lil(e);
		if(lil(e))m=MAX(m,lst_count(e));
	}
	if(all_dic){
		// TODO: this seems to want mutable dicts, since we actually need to take a column-key union
// 		lv*t=lmt(),*ok=lml(0);t->n=x->c;
// 		EACH(r,x)EACH(c,x->lv[r]){lv*k=x->lv[r]->kv[c];dset(t,ls(k),NULL);if(t->c>ok->c)ll_add(ok,k);};EACH(z,t)t->lv[z]=lml(x->c);
// 		EACH(r,x)EACH(c,t){t->lv[c]->lv[r]=dgetv(x->lv[r],ok->lv[c]);}return t;
	}
	if(all_lst){
		lv*r=lmtsize(m);tab_rows(r)=m;for(int c=0;c<tab_cols(r);c++){
			char t[64];snprintf(t,sizeof(t),"c%d",c);lv*k=lmcstr(t);
			lv*col=lml(m);tseti(r,c,k,col);for(int z=0;z<lst_count(col);z++){lv*e=lget(x,c);lset(col,z,z<lst_count(e)?lget(e,z):LNIL);}
		}return r;
	}
	return lt(x);
}
lv* l_flip(lv*x){
	if(lit(x)){
		// TODO:
	// 	lv*r=lmt(),*k=NULL,*ks=lmistr("key");
	// 	int ki=dgeti(x,ks);if(ki==-1)ki=0;lv*kl=lml(0);EACH(z,x)if(z!=ki)ll_add(kl,x->kv[z]);dset(r,ks,kl);
	// 	if(x->c){k=x->lv[ki];EACH(zz,k){lv*c=lml(0);EACH(z,x)if(z!=ki)ll_add(c,x->lv[z]->lv[zz]);dset(r,ls(k->lv[zz]),c);}}
	// 	return torect(r);
	}
	x=ll(x);int w=0;for(int z=0;z<lst_count(x);z++){lv*e=lget(x,z);w=MAX(w,lil(e)?lst_count(e):1);}
	lv*r=lml(w);for(int i=0;i<lst_count(r);i++){
		lv*c=lml(lst_count(x));for(int z=0;z<lst_count(c);z++){
			lv*e=lget(x,z);lset(c,z,!lil(e)?e: i<lst_count(e)?lget(e,i): LNIL);
		}lset(r,i,c);
	}return r;
}

lv* conform_monad(lv*x,lv*(f(lv*))){
	if(lil(x)){lv*r=lml(lst_count(x))    ;for(int z=0;z<lst_count(r);z++)lset(r,z,conform_monad(lget(x,z),f));return r;}
	if(lid(x)){lv*r=lmdsize(dic_count(x));for(int z=0;z<dic_count(r);z++)dseti(r,z,dgetk(x,z),conform_monad(dgetv(x,z),f));return r;}
	return f(x);
}
lv* a_not    (lv*x){return lmn(!     lb(x) );}                            lv* l_not   (lv*x){return conform_monad(x,a_not   );}
lv* a_negate (lv*x){return lmn(-     ln(x) );}                            lv* l_negate(lv*x){return conform_monad(x,a_negate);}
lv* a_floor  (lv*x){return lmn(floor(ln(x)));}                            lv* l_floor (lv*x){return conform_monad(x,a_floor );}
lv* a_sin    (lv*x){return lmn(sin  (ln(x)));}                            lv* l_sin   (lv*x){return conform_monad(x,a_sin   );}
lv* a_cos    (lv*x){return lmn(cos  (ln(x)));}                            lv* l_cos   (lv*x){return conform_monad(x,a_cos   );}
lv* a_tan    (lv*x){return lmn(tan  (ln(x)));}                            lv* l_tan   (lv*x){return conform_monad(x,a_tan   );}
lv* a_exp    (lv*x){return lmn(exp  (ln(x)));}                            lv* l_exp   (lv*x){return conform_monad(x,a_exp   );}
lv* a_ln     (lv*x){return lmn(log  (ln(x)));}                            lv* l_ln    (lv*x){return conform_monad(x,a_ln    );}
lv* a_sqrt   (lv*x){return lmn(sqrt (ln(x)));}                            lv* l_sqrt  (lv*x){return conform_monad(x,a_sqrt  );}
lv* a_unit   (lv*x){double n=ln(x);return lml2(lmn(cos(n)),lmn(sin(n)));} lv* l_unit  (lv*x){return conform_monad(x,a_unit  );}

lv* conform_nlmonad(lv*x,lv*(f(lv*))){
	if(!lil(x))return f(ll(x));
	int n=1;for(int z=0;z<lst_count(x);z++)if(!lin(lget(x,z))){n=0;break;}if(n)return f(x);
	lv*r=lml(lst_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,conform_nlmonad(lget(x,z),f));return r;
}
lv* a_mag    (lv*x){double r=0;for(int z=0;z<lst_count(x);z++)r+=pow(ln(lget(x,z)),2);         return lmn(sqrt(r));}
lv* a_heading(lv*x){double a=lst_count(x)>0?ln(lget(x,0)):0, b=lst_count(x)>1?ln(lget(x,1)):0; return lmn(atan2(b,a));}
lv* l_mag    (lv*x){return conform_nlmonad(x,a_mag    );}
lv* l_heading(lv*x){return conform_nlmonad(x,a_heading);}

lv* conform(lv*x,lv*y,lv*(f(lv*,lv*))){
	if(lid(x)&&lid(y)){ // union keys, zero-fill unmatched elements.
		// TODO:
		//DMAP(r,x,conform(x->lv[z],dgetv(y,x->kv[z]),f));
		//EACH(z,y)if(!dget(x,y->kv[z]))dset(r,y->kv[z],conform(LNIL,y->lv[z],f));return r;
	}
	// TODO:
	//if( lid(x)&&!lid(y)){DMAP(r,x,conform(x->lv[z],y,f));return r;}
	//if(!lid(x)&& lid(y)){DMAP(r,y,conform(x,y->lv[z],f));return r;}
	if( lil(x)&& lil(y)){lv*r=lml(lst_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,conform(lget(x,z),lgetmod(y,z),f));return r;}
	if( lil(x)&&!lil(y)){lv*r=lml(lst_count(x));for(int z=0;z<lst_count(r);z++)lset(r,z,conform(lget(x,z),y           ,f));return r;}
	if(!lil(x)&& lil(y)){lv*r=lml(lst_count(y));for(int z=0;z<lst_count(r);z++)lset(r,z,conform(x        ,lget(y,z)   ,f));return r;}
	return f(x,y);
}
lv* a_add (lv*x,lv*y){return lmn(ln(x)+ln(y)           );} lv* l_add(lv*x,lv*y){return conform(x,y,a_add);}
lv* a_sub (lv*x,lv*y){return lmn(ln(x)-ln(y)           );} lv* l_sub(lv*x,lv*y){return conform(x,y,a_sub);}
lv* a_mul (lv*x,lv*y){return lmn(ln(x)*ln(y)           );} lv* l_mul(lv*x,lv*y){return conform(x,y,a_mul);}
lv* a_div (lv*x,lv*y){return lmn(ln(y)==0?0:ln(x)/ln(y));} lv* l_div(lv*x,lv*y){return conform(x,y,a_div);}
lv* a_mod (lv*x,lv*y){return lmn(dmod(ln(y),ln(x))     );} lv* l_mod(lv*x,lv*y){return conform(x,y,a_mod);}
lv* a_pow (lv*x,lv*y){return lmn(pow(ln(x),ln(y))      );} lv* l_pow(lv*x,lv*y){return conform(x,y,a_pow);}
lv* a_less(lv*x,lv*y){return lmn(lin(x)||lin(y)? ln(x) <ln(y): str_cmp(ls(x),ls(y))<0);}
lv* a_more(lv*x,lv*y){return lmn(lin(x)||lin(y)? ln(x) >ln(y): str_cmp(ls(x),ls(y))>0);}
lv* a_min (lv*x,lv*y){if(lin(x)||lin(y)){double a=ln(x),b=ln(y);return lmn(a<b?a:b);}return str_cmp(ls(x),ls(y))<0?x:y;}
lv* a_max (lv*x,lv*y){if(lin(x)||lin(y)){double a=ln(x),b=ln(y);return lmn(a>b?a:b);}return str_cmp(ls(x),ls(y))>0?x:y;}
lv* a_eq  (lv*x,lv*y){
	return lmn(
		lii  (x)||lii  (y)?x==y:
		linil(x)||linil(y)?x==y:
		lin  (x)&&lin  (y)?ln(x)==ln(y):
		!str_cmp(ls(x),ls(y))
	);
}
lv* l_less(lv*x,lv*y){return conform(x,y,a_less);}
lv* l_more(lv*x,lv*y){return conform(x,y,a_more);}
lv* l_min (lv*x,lv*y){return conform(x,y,a_min );}
lv* l_max (lv*x,lv*y){return conform(x,y,a_max );}
lv* l_eq  (lv*x,lv*y){return conform(x,y,a_eq  );}

lv* l_sum (lv*x){x=ll(x);lv*r=ZERO      ;for(int z=0;z<lst_count(x);z++)r=l_add(r,lget(x,z));return r;}
lv* l_prod(lv*x){x=ll(x);lv*r=ONE       ;for(int z=0;z<lst_count(x);z++)r=l_mul(r,lget(x,z));return r;}
lv* l_amax(lv*x){x=ll(x);lv*r=l_first(x);for(int z=1;z<lst_count(x);z++)r=l_max(r,lget(x,z));return r;}
lv* l_amin(lv*x){x=ll(x);lv*r=l_first(x);for(int z=1;z<lst_count(x);z++)r=l_min(r,lget(x,z));return r;}
// lv* l_raze(lv*x){
// 	if(lit(x)){return l_dict(x->c?x->lv[0]:lml(0), x->c>1?x->lv[1]:lml(0));}
// 	x=ll(x);lv*r=l_first(x);for(int z=1;z<lst_count(x);z++)r=l_comma(r,lget(x,z));return r;
// }

// Parser



// Bytecode Interpreter



// Entrypoint

int main(int argc,char**argv){
	(void)argc,(void)argv;
	printf("hello, fastlil!\n");
}
