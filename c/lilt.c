// Lil Terminal
#include "lil.h"
#include "dom.h"
#ifndef __COSMOPOLITAN__
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "lib/bestline.h"
#include "lib/bestline.c"

lv*n_exit(lv*self,lv*a){(void)self;exit(ln(l_first(a)));}
lv*n_input(lv*self,lv*a){
	(void)self;char*line=bestline(drom_to_utf8(a->c<2?ls(l_first(a)): l_format(ls(l_first(a)),l_drop(ONE,a)))->sv);
	if(!line)return LNIL;lv*r=lmutf8(line);free(line);return r;
}
lv*n_readwav(lv*self,lv*a){
	// this polyfill is limited compared to the version in decker, but avoids SDL dependencies:
	(void)self;lv*name=ls(l_first(a));int offset=a->c>1?MAX(0,ln(a->lv[1])):0, size=0;
	struct stat st;if(stat(name->sv,&st)||st.st_size<13)return sound_make(lms(size));
	char*data=calloc(size=st.st_size,1);FILE*f=fopen(name->sv,"rb");
	if(fread(data,1,st.st_size,f)!=(unsigned)st.st_size)return fclose(f),free(data),sound_make(lms(0));fclose(f);
	char HEAD[]={'R','I','F','F',0xFF,0xFF,0xFF,0xFF,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,64,31,0,0,64,31,0,0,1,0,8,0,'d','a','t','a'};
	for(int z=0;z<40&&z<size;z++)if(0xFF!=(0xFF&HEAD[z])&&HEAD[z]!=data[z])return free(data),sound_make(lms(0));
	int samples=((0xFF&data[43])<<24)|((0xFF&data[42])<<16)|((0xFF&data[41])<<8)|(0xFF&data[40]);
	lv*r=lms(CLAMP(0,samples-offset,10*SFX_RATE));EACH(z,r)r->sv[z]=0xFF&(data[44+z+offset]-128);
	return free(data),sound_make(r);
}
lv*n_readfile(lv*self,lv*a){
	lv*name=ls(l_first(a));
	if(a->c>1&&matchr(lmistr("array"),a->lv[1]))return readbin(name);
	if(has_suffix(name->sv,".gif" ))return n_readgif(self,a);
	if(has_suffix(name->sv,".wav" ))return n_readwav(self,a);
	if(has_suffix(name->sv,".deck"))return n_readdeck(self,a);
	return n_read(self,a);
}
lv*runstring(char*t,lv*env){
	lv* prog=parse(t);if(perr())return fprintf(stderr,"(%d:%d) %s\n",par.r+1,par.c+1,drom_to_utf8(lmcstr(par.error))->sv),LNIL;
	return run(prog,env);
}
lv*runfile(char*path,lv*env){
	struct stat st;if(stat(path,&st)){fprintf(stderr,"unable to open '%s'\n",path);return LNIL;}
	return runstring(n_read(NULL,l_list(lmcstr(path)))->sv,env);
}
lv* print_array(lv*arr,FILE*out){array a=unpack_array(arr);for(int z=0;z<a.size;z++)fputc(0xFF&(int)array_get_raw(a,z),out);return arr;}
lv*n_print(lv*self,lv*a){(void)self;return a->c==1&&array_is(a->lv[0])?print_array(l_first(a),stdout):n_printf(a,1,stdout);}
lv*n_error(lv*self,lv*a){(void)self;return a->c==1&&array_is(a->lv[0])?print_array(l_first(a),stderr):n_printf(a,1,stderr);}

extern char **environ;

void go_notify(lv*deck,lv*args,int dest){(void)deck,(void)args,(void)dest;}
void field_notify(lv*field){(void)field;}
lv* n_panic(lv*self,lv*z){(void)self,(void)z;return LNIL;}
lv* n_alert(lv*self,lv*z){(void)self,(void)z;return ONE;}
lv* n_open (lv*self,lv*z){(void)self,(void)z;return lmistr("");}
lv* n_save (lv*self,lv*z){(void)self,(void)z;return LNIL;}
lv* n_play (lv*self,lv*z){(void)self;lv*x=l_first(z);return x;}
lv* n_show(lv*self,lv*a){
	(void)self;str s=str_new();EACH(z,a){if(z)str_addc(&s,' ');show(&s,a->lv[z],a->c==1);}
	printf("%s\n",drom_to_utf8(lmstr(s))->sv);return l_first(a);
}
lv*interface_app(lv*self,lv*i,lv*x){
	if(!x&&lis(i)){
		ikey("show"      )return lmnat(n_show,NULL);
		ikey("print"     )return lmnat(n_print,NULL);
	}return x?x:LNIL;(void)self;
}

// Environment

lv*n_import(lv*self,lv*a); // forward ref
lv* globals(void){
	lv*env=lmenv(NULL);
	dset(env,lmistr("show"     ),lmnat(n_show,NULL));
	dset(env,lmistr("print"    ),lmnat(n_print,NULL));
	dset(env,lmistr("error"    ),lmnat(n_error,NULL));
	dset(env,lmistr("input"    ),lmnat(n_input,NULL));
	dset(env,lmistr("dir"      ),lmnat(n_dir,NULL));
	dset(env,lmistr("read"     ),lmnat(n_readfile,NULL));
	dset(env,lmistr("write"    ),lmnat(n_writefile,NULL));
	dset(env,lmistr("path"     ),lmnat(n_path,NULL));
	dset(env,lmistr("exit"     ),lmnat(n_exit,NULL));
	dset(env,lmistr("shell"    ),lmnat(n_shell,NULL));
	dset(env,lmistr("eval"     ),lmnat(n_eval,NULL));
	dset(env,lmistr("import"   ),lmnat(n_import,NULL));
	dset(env,lmistr("random"   ),lmnat(n_random,NULL));
	dset(env,lmistr("array"    ),lmnat(n_array,NULL));
	dset(env,lmistr("image"    ),lmnat(n_image,NULL));
	dset(env,lmistr("sound"    ),lmnat(n_sound,NULL));
	dset(env,lmistr("newdeck"  ),lmnat(n_newdeck,NULL));
	dset(env,lmistr("readcsv"  ),lmnat(n_readcsv,NULL));
	dset(env,lmistr("writecsv" ),lmnat(n_writecsv,NULL));
	dset(env,lmistr("readxml"  ),lmnat(n_readxml,NULL));
	dset(env,lmistr("writexml" ),lmnat(n_writexml,NULL));
	constants(env);
	return env;
}
lv*n_import(lv*self,lv*a){
	lv*filename=ls(l_first(a));if(has_suffix(filename->sv,".deck")||has_suffix(filename->sv,".html")){
		lv*d=n_readdeck(self,a),*m=ifield(d,"modules"),*r=lmd();
		EACH(z,m){dset(r,m->kv[z],ifield(m->lv[z],"value"));}return r;
	}
	lv*file=n_read(self,a);if(!file->c)return LNIL;
	lv*prog=parse(ls(file)->sv);if(perr())return LNIL;
	lv*root=lmenv(globals());pushstate(root),issue(root,prog);
	int c=0;while(running()){runop(),c++;if(c%100==0)lv_collect();}
	DMAP(r,root,root->lv[z]);return popstate(),r;
}

// Entrypoint

int main(int argc,char**argv){
	init_interns();
	lv* env=globals();
	lv* a=lml(argc);for(int z=0;z<argc;z++)a->lv[z]=lmutf8(argv[z]);
	dset(env,lmistr("args"),a);
	dset(env,lmistr("env"),env_enumerate());
	char*home=getenv("LIL_HOME");if(home){
		struct dirent*find;DIR*dir=opendir(home);if(dir){while((find=readdir(dir))){
			char path[4096];snprintf(path,sizeof(path),"%s/%s",home,find->d_name);
			if(has_suffix(path,".lil"))runfile(path,env);
		}}closedir(dir);
	}
	for(int z=1;z<argc;z++){
		if(!strcmp(argv[z],"-h")){
			printf("usage: %s [FILE.lil...] [-e EXPR...]\nif present, execute a FILE and exit\n",argv[0]);
			printf("-e : evaluate STRING and exit\n-h : display this information\n");
		}
		else if(!strcmp(argv[z],"-e")){
			if(z+1>=argc)fprintf(stderr,"no expression specified.\n"),exit(1);
			runstring(argv[z+1],env),z++;
		}
		else{runfile(argv[z],env);}
		if(z==argc-1)exit(0);
	}
	while(1){
		char*line=bestlineWithHistory(" ","lilt");
		if (!line)break;
		lv*prog=parse(lmutf8(line)->sv);free(line);
		if(perr()){for(int z=0;z<par.c+2;z++)printf(" ");printf("^\n%s\n",drom_to_utf8(lmcstr(par.error))->sv);}
		else{lv*x=run(prog,env);dset(env,lmistr("_"),x);debug_show(x);}
	}
}
