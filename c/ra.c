// Resource Assembler

// a simple utility for converting static resources into C header files.
// roughly equivalent to 'xxd -i'

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc,char**argv){
	if(argc<2){fprintf(stderr,"usage: %s [FILE]\n emit a C array declaration representing the contents of a file to stdout.\n",argv[0]);exit(1);}
	char*fn=argv[1];
	struct stat st;if(stat(fn,&st)){fprintf(stderr,"unable to access input file %s\n",argv[1]);exit(1);}
	unsigned int s=st.st_size;
	FILE*f=fopen(fn,"rb");if(!f){fprintf(stderr,"unable to read from input file %s\n",argv[1]);exit(1);}
	unsigned char*b=malloc(s);if(fread(b,1,s,f)!=s){fclose(f);fprintf(stderr,"read failure.\n");exit(1);}fclose(f);

	char vn[4096]={0};unsigned int l=strlen(fn);for(unsigned int z=0;z<sizeof(vn)-1&&z<l;z++){char c=fn[z];vn[z]=c=='.'?'_': c=='/'?'_': c=='\\'?'_': c;}
	printf("unsigned int %s_len=%d;\n",vn,s);
	printf("unsigned char %s[]={",vn);
	for(unsigned int z=0;z<s;z++){if(z!=0)printf(",");printf("0x%02x",b[z]);}
	printf("};\n");
	return 0;
}
