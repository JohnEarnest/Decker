#!/usr/bin/awk -f

###########################################################
#
#  "The Name's *Lila*"
#
#  A Lil interpreter implemented in POSIX Awk,
#  providing a large subset of the Lilt standard library.
#  For more information on Lil, see:
#
#  http://beyondloom.com/tools/trylil.html
#
###########################################################

BEGIN{
	VERSION="1.53";tnames=0
	MATH_PI=3.141592653589793
	for(z=0;z<256;z++){ordinal[z]=sprintf("%c",z);ascii[sprintf("%c",z)]=z;}
	for(z=0;z<16 ;z++){hexdigits[sprintf("%x",z)]=z;hexdigits[sprintf("%X",z)]=z}
}
function max(x,y){return (x>y)?x:y}
function min(x,y){return (x<y)?x:y}
function abs(x){return x<0?-x:x}
function tan(x){return sin(x)/cos(x)}
function floor(n,  t){t=int(n);return n==t?n: n>=0?t: t-1}
function ceil(n,  t){t=int(n);return n==t?n: n>=0?t+1: t}
function mod(x,y){return y==0?0: x-y*floor(x/y)}
function tempname(){return sprintf("@t%i",tnames++)}
function repeat_char(chr,n,  r,z){r="";for(z=0;z<n;z++)r=r chr;return r}
function print_error(text){print text > "/dev/stderr"}
function readline(  r){(getline r < "/dev/stdin");return r}
function get_epoch(){
	# FIXME: this idiom currently doesn't work correctly in goawk.
	# shelling out to `date -u +%s` is tempting, but not actually POSIX-compatible.
	# `date` also cannot portably format with ms/ns precision.
	return srand(srand())
}
function epoch_to_isodate(x,  fmt,gnu,c,r){
	# FIXME: doing this in an even moderately portable fashion is a complete nightmare.
	# detect GNU coreutils "date" by seeing whether "date" responds to "--version" or pukes:
	fmt="+'%Y-%m-%dT%H:%M:%SZ'"
	gnu=system("date --version >/dev/null 2>&1")==0
	if(gnu){c="date -u -d '1970-01-01 UTC + "x" seconds' "fmt} # GNU coreutils-style
	else   {c="date -u -r "x" "fmt                           } # BSD/MacOS-style
	c | getline r;close(c);return r
}
function readbytesraw(filename,arr,  cmd,i,z){	
	# rather annoyingly, the -w option for 'od' is not portable.
	cmd="od -tu1 -An -v \""filename"\""
	c=0;while((cmd | getline)>0)for(z=1;z<=NF;z++)arr[c++]=int($z)
	close(cmd);return c
}
function writebytesraw(filename,arr,c){
	# FIXME: under gawk, requires the -b flag or LANG=C environment variable set;
	# otherwise bytes are mangled into UTF-8 codepoints. thanks, GNU.
	printf("")>filename;for(z=0;z<c;z++)printf("%c",arr[z])>>filename;close(filename)
}
function utf8_strip(arr,c,  r,z,t){
	# Lil strings represent a subset of ASCII;
	# UTF-8 data needs to be down-converted at ingestion:
	r="";for(z=0;z<c;z++){
		t=arr[z]
		if(t==226&&arr[z+1]==128&&(arr[z+2]==152||arr[z+2]==153)){t=39;z+=2} # '
		if(t==226&&arr[z+1]==128&&(arr[z+2]==156||arr[z+2]==157)){t=34;z+=2} # "
		if(t>=240){t=1;z+=3} # skip 4-byte codepoints
		if(t>=224){t=1;z+=2} # skip 3-byte codepoints
		if(t>=192){t=1;z+=1} # skip 2-byte codepoints
		r=r (t==13?"": t==9?" ": (t==10||(t>=32&&t<=126))?ordinal[t]: "?")
	};return r
}
function readtext(filename,  arr,t,r){
	t=readbytesraw(filename,arr);return utf8_strip(arr,t)
	# in a world without unicode, we could use a much simpler approach:
	if((getline t<filename)<0)return;r=t "\n"
	while((getline t<filename)>0)r=r t "\n";close(filename);return r
}
function writetext(filename,text){printf(text)>filename;close(filename);return 1}
function round(x,  i,a,f){
   i=int(x);if(i==x)return i
   if(x<0){a=-x;i=int(a);f=a-i;return f>=.5?int(x)-1:int(x)}
   else{f=x-i;return f>=.5?i+1:i}
}
function wnum(y,  w,d,s,i,z,t){
	w="";d="";if(y<0){y=-y;s="-"}else{s=""};i=floor(y);y=round((y-floor(y))*1000000);if(y>=1000000)i++
	while(i>.1){t=int(i%10);t=t<0?0: t>10?10:t;w=t w;i=int(i/10)};w="0"w;while(length(w)>1&&substr(w,1,1)=="0")w=substr(w,2)
	for(z=0;z<6;z++){d=int(y%10)d;y=int(y/10)};d="."d;gsub(/\.?0+$/,"",d)
	return s w d
}
# some awk implementations provide native bitwise operations,
# but they aren't portable and often come with jagged edges.
# reimplement them from scratch, at the cost of performance:
function lbits(x){return mod(int(x),4294967295)}
function bitwise_and(x,y,  v,r){v=1;r=0;while(x>0||y>0){if((x%2)==1&&(y%2)==1)r+=v;x=int(x/2);y=int(y/2);v*=2};return r}
function bitwise_or(x,y,   v,r){v=1;r=0;while(x>0||y>0){if((x%2)==1||(y%2)==1)r+=v;x=int(x/2);y=int(y/2);v*=2};return r}
function bitwise_xor(x,y,  v,r){v=1;r=0;while(x>0||y>0){if((x%2)   !=(y%2)   )r+=v;x=int(x/2);y=int(y/2);v*=2};return r}
# many awk builtins interpret strings as regular expressions,
# so we have to escape special chars to use them as literals:
function regex_esc(x){gsub(/[.^$*+?(){}\\|[]/,"\\\\&",x);return x}  # . ^ $ * + ? ( ) [ { } \ |

###########################################################
#
#  Heap and Garbage Collection
#
###########################################################

BEGIN{
	gc_lo=0     # Minimum available heap index
	gc_hi=0     # Maximum used heap index (high-water mark)
	gc_allocs=0 # Heap values allocated (total)
	gc_frees=0  # Heap values freed (total)
	gc_gen=0    # Garbage collections performed

	gc_intern_base=128+256 # indices reserved for interned ints
}
function heap_make(type,  r){
	while(heap_type[gc_lo])gc_lo++
	gc_hi=max(gc_hi,gc_lo)
	heap_type[gc_lo]=type
	heap_name[gc_lo]="" # func/native/interface name, block opstream
	heap_c   [gc_lo]=0  # list/dict count, table colcount
	heap_n   [gc_lo]=0  # table rowcount, block opcount, string/num raw value
	heap_k   [gc_lo]="" # key list (dict,table,env),            function args
	heap_v   [gc_lo]="" # val list (list,dict,table,block,env), function block (1-list), image/array data
	heap_aux [gc_lo]=-1 # function closure, env parent, native self, array slice-root; -1 if unbound
	gc_allocs++;return gc_lo
}
function heap_walk(x,  z,c,arr){
	if(x<=gc_intern_base||gc_found[x]==1)return;gc_found[x]=1
	if(heap_aux[x]!=-1)heap_walk(heap_aux[x])
	if(lienv(x)){c=split(heap_name[x],arr,"|");for(z=3;z<=c;z+=2)heap_walk(0+arr[z]);return}
	if(lion(x)){heap_walk(lon_body(x));heap_walk(lon_args(x));return}
	if(lil(x)||liblk(x)||lid(x)||lit(x)){for(z=0;z<count(x);z++)heap_walk(lst_get(x,z))}
	if(lid(x)||lit(x)                  ){for(z=0;z<count(x);z++)heap_walk(dic_getk(x,z))}
}
function heap_collect(  z){
	delete gc_found;gc_gen++
	for(z=0;z<state_pc;z++)heap_walk(state_p[z])
	for(z=0;z<state_ec;z++)heap_walk(state_e[z])
	for(z=0;z<state_tc;z++)heap_walk(state_t[z])
	for(z=gc_intern_base+1;z<gc_hi;z++)if(heap_type[z]!=0&&gc_found[z]!=1){heap_type[z]=0;gc_frees++;gc_lo=min(gc_lo,z)}
}

###########################################################
#
#  Core Datatypes
#
###########################################################

# as a point of discipline, only the functions within this section should
# be directly aware of the internal representation of data structures,
# which includes knowledge of their typecodes and other fields.
# exceptions are made for the "array" and "image" interfaces.

BEGIN{split("number string list dict table function function",typecodes," ")}
function l_typeof(ptr){return lms(lii(ptr)?li_name(ptr): typecodes[heap_type[ptr]])}

# small integers (signed+unsigned byte values) are interned to reduce heap pressure:
function lin(ptr){return heap_type[ptr]==1}
function lvn(ptr){return heap_n[ptr]}
function ln(ptr){return lin(ptr)?lvn(ptr): lis(ptr)?0+lvs(ptr): ln(l_first(ptr))}
BEGIN{for(z=-128;z<=255;z++){r=heap_make(1);heap_n[r]=z;heap_c[r]=1};NONE=lmn(0);ONE=lmn(1)}
function lmn(val,  r){val=0+val;if(val==int(val)&&val>=-128&&val<=255)return val+128;r=heap_make(1);heap_n[r]=val;heap_c[r]=1;return r}

# strings are just boxed awk strings; nothing complicated here.
# there might be some potential in interning constant-value strings, perhaps with negative heap indices?
function lis(ptr){return heap_type[ptr]==2}
function lms(val,  r){r=heap_make(2);heap_n[r]=string_clean(val);heap_c[r]=length(heap_n[r]);return r}
function lvs(ptr){return heap_n[ptr]}
function ls(ptr){return lis(ptr)?ptr: lin(ptr)?lms(wnum(lvn(ptr))): lil(ptr)?l_fuse(lms(""),ptr): lms("")}
function string_clean(x){gsub(/[\t]/," ",x);gsub(/[‘]/,"'",x);gsub(/[“]/,"\"",x);gsub(/[^ -~\n]/,"?",x);return x}

# lists are represented as fixed-width strings of numeric heap indices (pointers),
# allowing for reasonably efficient random access indexing, appending, and prepending:
function lil(ptr){return heap_type[ptr]==3}
function lml(){return heap_make(3)}
function lst_get(ptr,idx        ){return 0+substr(heap_v[ptr],1+(10*idx),10)}
function lst_set(ptr,idx,val,  t){t=heap_v[ptr];heap_v[ptr]=substr(t,1,1+(10*idx-1)) sprintf("%10d",val) substr(t,1+(10*(idx+1)))}
function lst_add(ptr,val,      t){t=heap_v[ptr];heap_v[ptr]=t sprintf("%10d",val);heap_c[ptr]++           }
function lst_addall(ptr,lst,   z){t=heap_v[ptr];heap_v[ptr]=t heap_v[lst]        ;heap_c[ptr]+=heap_c[lst]}
function lst_pre(ptr,val,      t){t=heap_v[ptr];heap_v[ptr]=sprintf("%10d",val) t;heap_c[ptr]++}
function lst_clone(ptr,        r){r=lml();heap_v[r]=heap_v[ptr];heap_c[r]=heap_c[ptr];return r}
function lst_find(ptr,key,     z){for(z=0;z<heap_c[ptr];z++)if(matchr(key,lst_get(ptr,z)))return z;return -1}
function lst_allnum(ptr,       z){for(z=0;z<heap_c[ptr];z++)if(!lin(lst_get(ptr,z)))return 0;return 1}
function lst_alllst(ptr,       z){for(z=0;z<heap_c[ptr];z++)if(!lil(lst_get(ptr,z)))return 0;return 1}
function lst_alldic(ptr,       z){for(z=0;z<heap_c[ptr];z++)if(!lid(lst_get(ptr,z)))return 0;return 1}
function l_list(x,             r){r=lml();heap_c[r]=1;heap_v[r]=sprintf("%10d"        ,x    );return r}
function lml2(x,y,             r){r=lml();heap_c[r]=2;heap_v[r]=sprintf("%10d%10d"    ,x,y  );return r}
function lml3(x,y,z,           r){r=lml();heap_c[r]=3;heap_v[r]=sprintf("%10d%10d%10d",x,y,z);return r}
function ll(ptr,  r,z,t){
	if(lid(ptr))return dic_vals(ptr)
	if(lis(ptr)){r=lml();t=lvs(ptr);for(z=0;z<heap_c[ptr];z++)lst_add(r,lms(substr(t,z+1,1)));return r}
	return lil(ptr)?ptr: lit(ptr)?l_rows(ptr): l_list(ptr)
}

# like lists, dictionaries are represented with fixed-width strings;
# a list of key pointers and a list of value pointers.
function lid(ptr){return heap_type[ptr]==4}
function lmd(){return heap_make(4)}
function ld(ptr){return lid(ptr)?ptr: lit(ptr)?l_cols(ptr):  lil(ptr)||lis(ptr)?l_dict(l_range(lmn(count(ptr))),ll(ptr)): lmd()}
function dic_getk(ptr,idx       ){return 0+substr(heap_k[ptr],1+(10*idx),10)}
function dic_getv(ptr,idx       ){return 0+substr(heap_v[ptr],1+(10*idx),10)}
function dic_keys(ptr,         r){r=lml();heap_v[r]=heap_k[ptr];heap_c[r]=heap_c[ptr];return r}
function dic_vals(ptr,         r){r=lml();heap_v[r]=heap_v[ptr];heap_c[r]=heap_c[ptr];return r}
function dic_clone(ptr,        r){r=lmd();heap_k[r]=heap_k[ptr];heap_v[r]=heap_v[ptr];heap_c[r]=heap_c[ptr];return r}
function dgetv(ptr,key,        r){r=dget(ptr,key);return r==-1?NONE:r}
function dget(ptr,key,         z){for(z=0;z<heap_c[ptr];z++)if(matchr(key,dic_getk(ptr,z)))return dic_getv(ptr,z);return -1}
function dgeti(ptr,key,        z){for(z=0;z<heap_c[ptr];z++)if(matchr(key,dic_getk(ptr,z)))return z              ;return -1}
function dsetuq(ptr,key,val,   z){dset(ptr,dget(ptr,key)!=-1?lms(lvs(ls(key))"_"):key,val)}
function dset(ptr,key,val,   z){
	for(z=0;z<heap_c[ptr];z++)if(matchr(key,dic_getk(ptr,z))){lst_set(ptr,z,val);return}
	heap_k[ptr]=heap_k[ptr] sprintf("%10d",key)
	heap_v[ptr]=heap_v[ptr] sprintf("%10d",val)
	heap_c[ptr]++
}

# structurally, a table is a dictionary with some extra constraints:
function lit(ptr){return heap_type[ptr]==5}
function lmt(){return heap_make(5)}
function lt(ptr){
	if(lit(ptr))return ptr
	if(lid(ptr)){r=lmt();tab_set(r,lms("key"),dic_keys(ptr));tab_set(r,lms("value"),dic_vals(ptr));return torect(r)}
	ptr=ll(ptr);r=lmt();tab_set(r,lms("value"),ptr);return torect(r)
}
function tab_getk(ptr,idx){return dic_getk(ptr,idx)}
function tab_getv(ptr,idx){return dic_getv(ptr,idx)}
function tab_getki(ptr,key){return dgeti(ptr,key)}
function tab_get(ptr,col){return dget(ptr,col)}
function tab_set(ptr,key,val){dset(ptr,key,val)} # note: you must call torect() after adding/replacing table columns!
function tab_setuq(ptr,key,val){dsetuq(ptr,key,val)}
function tab_row(ptr,row,  r,z){r=lmd();for(z=0;z<count(ptr);z++)dset(r,tab_getk(ptr,z),lst_get(tab_getv(ptr,z),row));return r}
function tab_clone(ptr,  r,z){r=lmt();for(z=0;z<count(ptr);z++)tab_set(r,tab_getk(ptr,z),lst_clone(tab_getv(ptr,z)));heap_n[r]=heap_n[ptr];return r}
function tab_addrow(ptr,dic,  z){for(z=0;z<count(ptr);z++)lst_add(tab_getv(ptr,z),dgetv(dic,tab_getk(ptr,z)));heap_n[ptr]++}
function torect(ptr,     z,t,v){
	heap_n[ptr]=0;t=""
	for(z=0;z<count(ptr);z++){v=dic_getv(ptr,z);heap_n[ptr]=max(heap_n[ptr],lil(v)?count(v):1)}
	for(z=0;z<count(ptr);z++){v=dic_getv(ptr,z);t=t sprintf("%10d",l_take(lmn(heap_n[ptr]),lil(v)?v:l_list(v)))}
	heap_v[ptr]=t;return ptr
}

# functions ("on ...") reference an arglist, a block, and (when bound) a closure:
function lion(ptr){return heap_type[ptr]==6}
function lmon(name,args,blk,  r){r=heap_make(6);heap_name[r]=name;heap_k[r]=args;heap_v[r]=blk;return r}
function lon_bind(ptr,env,  r){r=heap_make(6);heap_name[r]=heap_name[ptr];heap_k[r]=heap_k[ptr];heap_v[r]=heap_v[ptr];heap_aux[r]=env;return r}
function lon_name(ptr){return heap_name[ptr]}
function lon_args(ptr){return heap_k[ptr]}
function lon_body(ptr){return heap_v[ptr]}
function lon_closure(ptr){return heap_aux[ptr]}

# natives are just a "handle" to a native function, since we can't actually carry around function pointers:
function linat(ptr){return heap_type[ptr]==7}
function lmnat(name,inner,self,  r){r=heap_make(7);heap_name[r]=name;heap_n[r]=inner;heap_aux[r]=self;return r}
function lnat_name(ptr){return heap_name[ptr]}
function lnat_inner(ptr){return heap_n[ptr]}
function lnat_self(ptr){return heap_aux[ptr]}

# likewise, interfaces are just a "handle",
# though some interfaces may have their own additional state occupying other slots:
function lii(ptr){return heap_type[ptr]==8}
function lmi(name,  r){r=heap_make(8);heap_name[r]=name;return r}
function li_name(ptr){return heap_name[ptr]}

# some utilities:
function lb(ptr){return lin(ptr)?lvn(ptr)!=0: lis(ptr)||lil(ptr)||lid(ptr)?count(ptr): 1} # truthiness
function count(ptr){return heap_c[ptr]}
function rowcount(ptr){return heap_n[ptr]}
function matchr(x,y,  z){
	if(x==y)return 1
	if(heap_type[x]!=heap_type[y]||heap_c[x]!=heap_c[y]||heap_n[x]!=heap_n[x])return 0
	if(lin(x))return lvn(x)==lvn(y)
	if(lis(x))return lvs(x)==lvs(y)
	if(lil(x)){for(z=0;z<count(x);z++)if(!matchr(lst_get(x,z),lst_get(y,z)))return 0;return 1}
	if(lid(x)){for(z=0;z<count(x);z++)if(!matchr(dic_getk(x,z),dic_getk(y,z))||!matchr(dic_getv(x,z),dic_getv(y,z)))return 0;return 1}
	if(lit(x)){for(z=0;z<count(x);z++)if(!matchr(tab_getk(x,z),tab_getk(y,z))||!matchr(tab_getv(x,z),tab_getv(y,z)))return 0;return 1}
	return 0
}

# blocks are not a user-facing datatype;
# they represent runs of lil bytecode, bridging the parser and interpreter.
# the opcode stream is a list of variable-width elements in a string.
# this makes random writes very inefficient, but those only occur during parsing/codegen:
# note that a blk invasively _is_ a list of interned local values (ints,strings,funcs...)
function liblk(ptr){return heap_type[ptr]==9}
function lmblk(  r){return heap_make(9)} # heap_name is the opcode stream, heap_n is the opcode count
BEGIN{
	split("JUMP,JUMPF,LIT,DUP,DROP,SWAP,OVER,BUND,OP1,OP2,OP3,GET,SET,LOC,AMEND,TAIL,CALL,BIND,ITER,EACH,NEXT,COL,IPRE,IPOST",ops,",")
	split("2   ,2    ,2  ,1  ,1   ,1   ,1   ,2   ,2  ,2  ,2  ,2  ,2  ,2  ,2    ,1   ,1   ,1   ,1   ,2   ,2   ,1  ,2   ,2    ",len,",")
	for(key in ops)oplens[ops[key]]=0+len[key]
}
function blk_here(x){return heap_n[x]}
function blk_set_at(x,i,v,  c,arr){c=blk_getops(x,arr);arr[i]=v;blk_setops(x,arr,c)}
function blk_opraw(x,o){heap_name[x]=heap_name[x] (length(heap_name[x])?"`":"")o    ;heap_n[x]+=1;}
function blk_op(x,o)   {heap_name[x]=heap_name[x] (length(heap_name[x])?"`":"")o    ;heap_n[x]+=1;if(o=="COL")blk_op(x,"SWAP")}
function blk_opa(x,o,i){heap_name[x]=heap_name[x] (length(heap_name[x])?"`":"")o"`"i;heap_n[x]+=2;return heap_n[x]}
function blk_getops(blk,arr){return split(heap_name[blk],arr,"`")}
function blk_setops(blk,arr,c,  t,z){t=c==0?"":arr[1];for(z=2;z<=c;z++)t=t"`"arr[z];heap_name[blk]=t}
function blk_imm(x,o,k,  i){i=lst_find(x,k);if(i==-1){lst_add(x,k);i=count(x)-1};blk_opa(x,o,i);}
function blk_getimm(x,i){return lst_get(x,i)}
function blk_op1(x,n){blk_opa(x,"OP1",n)}
function blk_op2(x,n){blk_opa(x,"OP2",n)}
function blk_op3(x,n){blk_opa(x,"OP3",n)}
function blk_set(x,n){blk_opa(x,"SET",n)}
function blk_loc(x,n){blk_opa(x,"LOC",n)}
function blk_get(x,n){blk_opa(x,"GET",n)}
function blk_lit(x,v){blk_imm(x,"LIT",v)}
function blk_cat(x,y,  c,base,arr,z,b,i){
	c=blk_getops(y,arr);base=blk_here(x);z=1;while(z<=c){
		b=arr[z]
		if(b~/^LIT/){blk_imm(x,b,blk_getimm(y,arr[z+1]))}
		else if(b~/^JUMP|JUMPF|EACH|NEXT$/){blk_opa(x,b,arr[z+1]+base)}
		else if(oplens[b]==1){blk_opraw(x,b)}
		else{blk_opa(x,b,arr[z+1])};z+=oplens[b]
	}
}
function blk_loop(b,names,body,  head,each){
	blk_op(b,"ITER");head=blk_here(b);blk_lit(b,names);each=blk_opa(b,"EACH",0)
	blk_cat(b,body);blk_opa(b,"NEXT",head);blk_set_at(b,each,blk_here(b))
}
function blk_end(x,  c,arr,z,b,t,i){
	c=blk_getops(x,arr)
	z=1;while(z<=c){
		b=arr[z];z+=oplens[b]
		if(b!="CALL")continue
		t=1;i=z;while(i<=c){
			if(arr[i]!="JUMP"){t=0;break}
			if(arr[i+1]<=i){t=0;break}
			i=1+arr[i+1]
		}
		if(t)arr[z-1]="TAIL"
	}
	blk_setops(x,arr,c);return x
}

# environments are specialized string-keyed dictionaries,
# chained together to represent nested lexical scopes.
# we use yet another string representation alternating variable-width keys
# with fixed-width pointer values so that we can use index() for fast(er) lookups:
function lienv(ptr){return heap_type[ptr]==10}
function lmenv(  r){r=heap_make(10);return r}
function eget(env,k,  i,d){
	k="|"k"|";d=heap_name[env];i=index(d,k)
	return i==0?-1: 0+substr(d,length(k)+i,10)
}
function eset(env,k,v,  i,d){
	k="|"k"|";v=sprintf("%10d",0+v);d=heap_name[env];i=index(d,k)
	heap_name[env]=i==0?d k v: substr(d,1,length(k)+i-1) v substr(d,length(k)+i+10)
}
function env_bind(env,dic,  r,z,ka,va,c,v){
	for(z=0;z<count(dic);z++)v=v sprintf("|%s|%10d",lvs(ls(dic_getk(dic,z))),dic_getv(dic,z))
	r=lmenv();heap_name[r]=v;heap_aux[r]=env;return r
}
function env_enumerate(env,  r,z,c,arr){
	c=split(heap_name[env],arr,"|")
	r=lmd();for(z=2;z<=c;z+=2)dset(r,lms(arr[z]),0+arr[z+1]);return r
}
function env_parent(ptr){return heap_aux[ptr]}
function env_getr(env,k,  r){if(env==-1)return -1;r=eget(env,k);return r!=-1?r: env_getr(env_parent(env),k)}
function env_setr(env,k,v,  r){if(env==-1)return;r=eget(env,k);if(r!=-1){eset(env,k,v)}else{env_setr(env_parent(env),k,v)}}
function env_loc(env,k,v){eset(env,k,v)}
function env_get(env,k,  r){r=env_getr(env,k);return r==-1?NONE:r}
function env_set(env,k,v,  r){r=env_getr(env,k);if(r==-1){env_loc(env,k,v)}else{env_setr(env,k,v)}}

###########################################################
#
#  Lil Primitives
#
###########################################################

function l_count(x){return lmn(lit(x)?rowcount(x): lii(x)?1: count(x))}
function l_first(x){return lis(x)?lms(substr(lvs(x),1,1)): lit(x)?l_first(l_rows(x)): lion(x)?lms(lon_name(x)): linat(x)?lms("native"): count(x)?lst_get(ll(x),0): NONE}
function l_last(x){return lis(x)?lms(substr(lvs(x),count(x),1)): lit(x)?l_last(l_rows(x)): count(x)?lst_get(ll(x),count(x)-1): NONE}
function l_keys(x,  r,z){return lii(x)?lml(): lion(x)?lon_args(x): dic_keys(ld(x))}
function l_rows(x,  r,z){x=lt(x);r=lml();for(z=0;z<rowcount(x);z++)lst_add(r,tab_row(x,z))            ;return r}
function l_cols(x,  r,z){x=lt(x);r=lmd();for(z=0;z<count(x)   ;z++)dset(r,tab_getk(x,z),tab_getv(x,z));return r}
function l_range(x,  r,z){if(lin(x)){x=max(int(ln(x)),0);r=lml();for(z=0;z<x;z++)lst_add(r,lmn(z));return r};return dic_vals(x)}
function table_flip(x,  r,z,zz,ks,ki,kl,k,c){
	r=lmt();ks=lms("key");ki=tab_getki(x,ks);if(ki==-1)ki=0;
	kl=lml();for(z=0;z<count(x);z++)if(z!=ki)lst_add(kl,tab_getk(x,z));tab_set(r,ks,kl)
	if(count(x)){
		k=tab_getv(x,ki)
		for(zz=0;zz<count(k);zz++){c=lml();tab_set(r,ls(lst_get(k,zz)),c);for(z=0;z<count(x);z++)if(z!=ki)lst_add(c,lst_get(tab_getv(x,z),zz))}
	};return torect(r)
}
function l_flip(x,  r,z,w,t,i,c){
	if(lit(x))return table_flip(x);x=ll(x);r=lml();w=0;for(z=0;z<count(x);z++){t=lst_get(x,z);w=max(w,lil(t)?count(t):1)}
	for(i=0;i<w;i++){c=lml();lst_add(r,c);for(z=0;z<count(x);z++){t=lst_get(x,z);lst_add(c,!lil(t)?t: i<count(t)?lst_get(t,i):NONE)}}
	return r
}
function l_table(x,  r,z,m,t,row,col,k,ok){
	if(lid(x)){r=lmt();for(z=0;z<count(x);z++)tab_set(r,ls(dic_getk(x,z)),dic_getv(x,z));return torect(r)}
	if(lil(x)&&lst_alldic(x)){
		r=lmt();ok=lml();
		for(row=0;row<count(x);row++){
			t=lst_get(x,row)
			for(col=0;col<count(t);col++){k=dic_getk(t,col);tab_set(r,ls(k),lml());if(count(r)>count(ok))lst_add(ok,k)}
		}
		for(row=0;row<count(x);row++){
			t=lst_get(x,row)
			for(col=0;col<count(r);col++){lst_add(tab_getv(r,col),dgetv(t,lst_get(ok,col)))}
		}
		return torect(r)
	}
	if(lil(x)&&lst_alllst(x)){
		m=0;for(z=0;z<count(x);z++)m=max(m,count(lst_get(x,z)));r=lmt();for(c=0;c<m;c++)tab_set(r,lms("c"c),lml())
		for(z=0;z<count(x);z++){t=lst_get(x,z);for(c=0;c<m;c++)lst_add(tab_getv(r,c),c>=count(t)?NONE: lst_get(t,c))}
		return torect(r)
	}
	return lt(x)
}
function l_raze(x, r){
	if(lit(x))return l_dict(count(x)>0?tab_getv(x,0):lml(), count(x)>1?tab_getv(x,1):lml())
	x=ll(x);r=l_first(x);for(z=1;z<count(x);z++)r=l_cat(r,lst_get(x,z));return r
}
function monads_atomic(name,x,  r,z){
	if(lid(x)){r=lmd();for(z=0;z<count(x);z++)dset(r,dic_getk(x,z),monads_atomic(name,dic_getv(x,z)));return r}
	if(lil(x)){r=lml();for(z=0;z<count(x);z++)lst_add(r,monads_atomic(name,lst_get(x,z)));return r}
	if(name=="-"    )return lmn(-ln(x))
	if(name=="!"    )return lb(x)?NONE:ONE
	if(name=="floor")return lmn(floor(ln(x)))
	if(name=="cos"  )return lmn(cos(ln(x)))
	if(name=="sin"  )return lmn(sin(ln(x)))
	if(name=="tan"  )return lmn(sin(ln(x))/cos(ln(x)))
	if(name=="exp"  )return lmn(exp(ln(x)))
	if(name=="ln"   )return lmn(log(ln(x)))
	if(name=="sqrt" )return lmn(sqrt(ln(x)))
	if(name=="unit" )return lml2(lmn(cos(ln(x))),lmn(sin(ln(x))))
	return -1
}
function monads_special(name,x,  r,z){
	if(lil(x)&&!lst_allnum(x)){r=lml();for(z=0;z<count(x);z++)lst_add(r,monads_special(name,lst_get(x,z)));return r}
	if(name=="mag"    ){x=ll(x);r=0;for(z=0;z<count(x);z++)r+=ln(lst_get(x,z))^2;return lmn(sqrt(r))}
	if(name=="heading"){x=ll(x);r=ln(count(x)>0?lst_get(x,0):NONE);z=ln(count(x)>1?lst_get(x,1):NONE);return lmn(atan2(z,r))}
	return -1
}
function monads(name,x,  r,z){
	if(name=="count" )return l_count(x)
	if(name=="first" )return l_first(x)
	if(name=="last"  )return l_last(x)
	if(name=="keys"  )return l_keys(x)
	if(name=="range" )return l_range(x)
	if(name=="list"  )return l_list(x)
	if(name=="flip"  )return l_flip(x)
	if(name=="rows"  )return l_rows(x)
	if(name=="cols"  )return l_cols(x)
	if(name=="table" )return l_table(x)
	if(name=="raze"  )return l_raze(x)
	if(name=="typeof")return l_typeof(x)
	if(name=="sum"   ){x=ll(x);r=NONE      ;for(z=0;z<count(x);z++)r=dyads_atomic("+",r,lst_get(x,z));return r}
	if(name=="prod"  ){x=ll(x);r=ONE       ;for(z=0;z<count(x);z++)r=dyads_atomic("*",r,lst_get(x,z));return r}
	if(name=="max"   ){x=ll(x);r=l_first(x);for(z=1;z<count(x);z++)r=dyads_atomic("|",r,lst_get(x,z));return r}
	if(name=="min"   ){x=ll(x);r=l_first(x);for(z=1;z<count(x);z++)r=dyads_atomic("&",r,lst_get(x,z));return r}
	if(name=="@tab"){
		x=lt(x);r=tab_clone(x);z=lmn(rowcount(r));
		tab_set(r,lms("index" ),l_range(z))
		tab_set(r,lms("gindex"),l_range(z))
		tab_set(r,lms("group" ),l_take(z,NONE))
		return torect(r)
	}
	if(name=="mag"||name=="heading")return monads_special(name,x)
	return monads_atomic(name,x)
}

function ina(x,y,  z){
	if(lil(y))for(z=0;z<count(y);z++)if(matchr(lst_get(y,z),x))return 1
	return lis(y)?lvs(y)~lvs(x): lid(y)?dget(y,x)!=-1: lit(y)?tab_get(y,x)!=-1: 0
}
function l_in(x,y,  r,z){if(lil(x)){r=lml();for(z=0;z<count(x);z++)lst_add(r,l_in(lst_get(x,z),y));return r};return lmn(ina(x,y))}
function filter(i,x,y,  r,z,v,k){
	x=lis(x)?l_list(x):ll(x)
	if( lid(y)){r=lmd();        for(z=0;z<count(y);z++){k=dic_getk(y,z);if(i==ina(k,x))dset(r,k,dic_getv(y,z))};return r}
	if(!lit(y)){r=lml();y=ll(y);for(z=0;z<count(y);z++){v=lst_get(y,z) ;if(i==ina(v,x))lst_add(r,v)}           ;return r}
	if(lst_allnum(x)){
		r=l_take(NONE,y)
		if(i){for(i=0;i<count(x);i++){v=ln(lst_get(x,i));if(v>=0&&v<rowcount(y))for(z=0;z<count(y);z++)lst_add(tab_getv(r,z),lst_get(tab_getv(y,z),v))}}
		else{for(z=0;z<rowcount(y);z++){if(!ina(lmn(z),x))for(i=0;i<count(y);i++)lst_add(tab_getv(r,i),lst_get(tab_getv(y,i),z))}}
		return torect(r)
	}
	r=lmt();for(z=0;z<count(y);z++){k=tab_getk(y,z);if(i==ina(k,x))tab_set(r,k,tab_getv(y,z))};return torect(r)
}
function dic_index(dic,ix){r=lmd();for(z=0;z<count(ix);z++){i=ln(lst_get(ix,z));dset(r,dic_getk(dic,i),dic_getv(dic,i))};return r}
function l_take(x,y,  r,z,s,i){
	if(!lin(x))return filter(1,x,y);if(lil(y)&&ln(x)==count(y))return y
	if(lis(y)){
		if(ln(x)< 0&&abs(int(ln(x)))<=count(y))return lms(substr(lvs(y),1+count(y)+ln(x),-ln(x)))
		if(ln(x)>=0&&        ln(x)  <=count(y))return lms(substr(lvs(y),1,ln(x)))
		return l_fuse(lms(""),l_take(x,ll(y)))
	}
	if(lid(y))return dic_index(y,l_take(x,l_range(lmn(count(y)))))
	if(lit(y)){r=tab_clone(y);for(z=0;z<count(r);z++)tab_set(r,tab_getk(r,z),l_take(x,tab_getv(r,z)));return torect(r)}
	r=lml();x=ln(x);y=ll(y);s=x<0?mod(x,count(y)):0;
	for(z=0;z<abs(x);z++)lst_add(r,count(y)?lst_get(y,mod(z+s,count(y))):NONE);return r
}
function l_drop(x,y,  r,z,s){
	if(!lin(x))return filter(0,x,y)
	if(lis(y)){
		if(ln(x)< 0)return lms(substr(lvs(y),1,count(y)+ln(x)))
		if(ln(x)>=0)return lms(substr(lvs(y),ln(x)+1))
		return l_fuse(lms(""),l_drop(x,ll(y)))
	}
	if(lid(y))return dic_index(y,l_drop(x,l_range(lmn(count(y)))))
	if(lit(y)){r=tab_clone(y);for(z=0;z<count(r);z++)tab_set(r,tab_getk(r,z),l_drop(x,tab_getv(r,z)));return torect(r)}
	r=lml();x=ln(x)
	if(x>0){for(z=0;z<max(0,count(y)-x);z++)lst_add(r,lst_get(y,x+z))}
	else   {for(z=0;z<max(0,count(y)+x);z++)lst_add(r,lst_get(y,z  ))};return r
}
function l_limit(x,y){return (lit(y)?rowcount(y):count(y))>ln(x)?l_take(lmn(ln(x)),y):y}
function table_cat(x,y,  r,i,z,c){
	r=lmt()
	for(i=0;i<count(x);i++){
		c=lst_clone(tab_getv(x,i))
		if(tab_get(y,tab_getk(x,i))==-1){for(z=0;z<rowcount(y);z++)lst_add(c,NONE)}
		tab_set(r,tab_getk(x,i),c)
	}
	for(i=0;i<count(y);i++){
		c=tab_get(r,tab_getk(y,i))
		if(c==-1){c=lml();for(z=0;z<rowcount(x);z++)lst_add(c,NONE);tab_set(r,tab_getk(y,i),c)}
		lst_addall(c,tab_getv(y,i))
	}
	return torect(r)
}
function l_cat(x,y,  r,z){
	if(lit(x)&&lit(y))return table_cat(x,y)
	if(lid(x)){r=dic_clone(x);y=ld(y);for(z=0;z<count(y);z++)dset(r,dic_getk(y,z),dic_getv(y,z));return r}
	if(lis(x))x=l_list(x);r=lst_clone(ll(x));
	if(lis(y))y=l_list(y);y=ll(y);for(z=0;z<count(y);z++)lst_add(r,lst_get(y,z));return r
}
function l_split(x,y,  r,z,n){
	x=ls(x);y=ls(y);if(!count(x))return ll(y);r=lml();n=0;for(z=0;z<count(y);z++){
		if(substr(lvs(y),z+1,count(x))!=lvs(x))continue
		lst_add(r,lms(substr(lvs(y),n+1,z-n)));z+=count(x)-1;n=z+1
	}if(n<=count(y)){lst_add(r,lms(substr(lvs(y),n+1,count(y)-n)))};return r
}
function l_fuse(x,y,  z,r){x=ls(x);y=ll(y);r=""   ;for(z=0;z<count(y);z++){if(z)r=r lvs(x);r=r lvs(ls(lst_get(y,z)))};return lms(r)}
function l_dict(x,y,  z,r){x=ll(x);y=ll(y);r=lmd();for(z=0;z<count(x);z++)dset(r,lst_get(x,z),z>=count(y)?NONE:lst_get(y,z));return r}
function l_less(x,y){return lmn(lin(x)&&lin(y)?lvn(x)<lvn(y): lvs(ls(x))<lvs(ls(y)))}
function l_more(x,y){return lmn(lin(x)&&lin(y)?lvn(x)>lvn(y): lvs(ls(x))>lvs(ls(y)))}

function like_convert(p,  r,z,c){ # convert lil's glob patterns into awk regexes
	r="^";for(z=1;z<=length(p);z++){
		c=substr(p,z,1)
		if     (c=="*"){r=r ".*"}
		else if(c=="#"){r=r "[0-9]"}
		else if(c=="."){r=r "."}
		else if(c=="`"){r=r (z==length(p)?"`":regex_esc(substr(p,z+1,1)));z++}
		else           {r=r regex_esc(c)}
	};return r"$"
}
function like_test(x,pats,  key){for(key in pats){if(x~pats[key])return ONE};return NONE}
function l_like(x,y,  r,z,pats,p,t){
	if(!lil(y))y=l_list(y);for(z=0;z<count(y);z++){pats[z]=like_convert(lvs(ls(lst_get(y,z))))}
	if(lil(x)){r=lml();for(z=0;z<count(x);z++)lst_add(r,like_test(lvs(ls(lst_get(x,z))),pats));return r}
	else{return like_test(lvs(ls(x)),pats)}	# }
}
function l_window(x,y,  r,z,t,i){
	x=int(ln(x));r=lml();if(lis(y)){y=lvs(y)
		if(x>0){     for(z=0;z    <length(y);z+=x){lst_add(r,lms(substr(y,z+1,x)))}}
		if(x<0){x=-x;for(z=0;z+x-1<length(y);z+=1){lst_add(r,lms(substr(y,z+1,x)))}}
	}else{y=ll(y)
		if(x>0){     for(z=0;z    <count(y) ;z+=x){t=lml();lst_add(r,t);for(i=0;i<x&&z+i<count(y);i++)lst_add(t,lst_get(y,z+i))}}
		if(x<0){x=-x;for(z=0;z+x-1<count(y) ;z+=1){t=lml();lst_add(r,t);for(i=0;i<x              ;i++)lst_add(t,lst_get(y,z+i))}}
	}return r
}

function format_has_names(str,  z,d,t){
	for(z=1;z<=length(str);){
		if(substr(str,z,1)!="%"){z++;continue};z++;if(substr(str,z,1)=="[")return 1
		if(substr(str,z,1)=="*")z++;if(substr(str,z,1)=="-")z++;if(substr(str,z,1)=="0")z++
		while(substr(str,z,1)~/[0-9]/)z++;if(substr(str,z,1)==".")z++
		d=0;while(substr(str,z,1)~/[0-9]/){d=d*10+substr(str,z,1);z++}
		if(z>length(str))break;t=substr(str,z,1);z++;if(t~/[ro]/)while(d&&z<=length(str)){d--;f++}
	}return 0
}
BEGIN{json_t="";json_i=0;json_f=0;json_n=0;}
function json_c(){return json_n&&json_f?substr(json_t,json_i,1):""}
function json_nx(){if(json_n&&json_f){json_n--;return substr(json_t,json_i++,1)};return ""}
function json_m(x){if(json_c()==x){json_nx();return 1};return 0}
function json_s(){while(json_c()~/[ \n]/)json_nx()}
function json_d(){while(json_c()~/[0-9]/)json_nx()}
function json_lit(x,  l){l=length(x);if(json_n>=l&&substr(json_t,json_i,l)==x){json_i+=l;json_n-=l;return 1};return 0}
function json_hex(  z,r){r=0;for(z=0;z<4;z++){r=(r*16)+hexdigits[json_nx()]};return r<256?ordinal[r]:"?"}
function json_parse(  r,mc,k,e,ns){
	if(json_lit("true"))return ONE;if(json_lit("null")||json_lit("false"))return NONE
	if(json_m("[")){
		r=lml();while(json_c()!=""){
			json_s();if(json_m("]"))break;lst_add(r,json_parse());json_s();json_m(",")
		};return r
	}
	if(json_m("{")){
		r=lmd();while(json_c()!=""){
			json_s();if(json_m("}"))break;k=json_parse()json_s();json_m(":")
			json_s();if(json_f)dset(r,k,json_parse());json_s();json_m(",")
		}return r
	}
	if(json_c()~/["']/){
		mc=json_c();json_nx();r="";while(json_c()!=""&&!json_m(mc)){
			if(json_m("\\")){e=json_nx();r=r (e=="n"?"\n": e~/[\\"\/']/?e: e=="u"&&json_n>=4?json_hex(): " ")}
			else{r=r json_nx()}
		}return lms(r)
	}
	ns=json_i;json_m("-");json_d();json_m(".");json_d()
	if(json_m("e")||json_m("E")){json_m("-")||json_m("+");json_d()}
	if(json_i<=ns){json_f=0;return NONE};return lmn(substr(json_t,ns,min(json_i-ns,512)))
}
function love_parse(  r,k){
	if(json_m("[")){
		r=lml();while(json_c()!=""){
			json_s();if(json_m("]"))break;lst_add(r,love_parse());json_s();json_m(",")
		};return r
	}
	if(json_m("{")){
		r=lmd();while(json_c()!=""){
			json_s();if(json_m("}"))break;k=love_parse()json_s();json_m(":")
			json_s();if(json_f)dset(r,k,love_parse());json_s();json_m(",")
		}return r
	}
	if(json_m("<")){
		r=lmd();while(json_c()!=""){
			json_s();if(json_m(">"))break;k=love_parse()json_s();json_m(":")
			json_s();if(json_f)dset(r,ls(k),ll(love_parse()));json_s();json_m(",")
		}return l_table(r)
	}
	if(json_m("%")){
		json_m("%");r="%%"
		while(json_c()~/[a-zA-Z0-9+\/=]/)r=r json_nx()
		if(substr(r,1,5)=="%%IMG")return image_read(r)
		if(substr(r,1,5)=="%%DAT")return array_read(r)
		return NONE
	}
	return json_parse()
}
BEGIN{
	ISODATE=lms("%[year]04i-%[month]02i-%[day]02iT%[hour]02i:%[minute]02i:%[second]02iZ%n%m")
	split("0,31,59,90,120,151,181,212,243,273,304,334",cumulative_month_days,",")
}
function leap_year(year){return year%400==0?1: year%100==0?0: year%4==0?1: 0}
function l_parse(x,y,  r,z,f,h,m,pi,named,nk,hn,si,sk,lf,n,d,t,v,s,p,mc,yr,k){
	if(lil(y)){r=lml();for(z=0;z<count(y);z++)lst_add(r,l_parse(x,lst_get(y,z)));return r}
	x=lvs(ls(x));y=lvs(ls(y));f=1;h=1;m=1;pi=0;named=format_has_names(x);r=named?lmd():lml()
	while(f<=length(x)){
		if(substr(x,f,1)!="%"){if(m&&substr(x,f,1)==substr(y,h,1)){h++}else{m=0};f++;continue}f++
		nk="";hn=0;if(substr(x,f,1)=="["){hn=1;f++;while(f<=length(x)&&substr(x,f,1)!="]"){nk=nk substr(x,f,1);f++};if(substr(x,f,1)=="]")f++}
		si=h;v=-1
		sk=substr(x,f,1)=="*";if(sk)f++
		lf=substr(x,f,1)=="-";if(lf)f++
		if(substr(x,f,1)=="0")f++
		n=0;while(substr(x,f,1)~/[0-9]/){n=n*10+substr(x,f,1);f++};if(substr(x,f,1)==".")f++
		d=0;while(substr(x,f,1)~/[0-9]/){d=d*10+substr(x,f,1);f++};if(f>length(x))break
		t=substr(x,f,1);f++
		if(t~/[^%mnzsluqarojJ]/){while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)~/[ \n]/)h++;}
		if     (t=="%"){if(m&&h<=length(y)&&(n?h-si<n:1)){h++}else{m=0}}
		else if(t=="m")v=m?ONE:NONE
		else if(t=="n")v=lmn(h-1)
		else if(t=="z")v=(m&&h==length(y)+1)?ONE:NONE
		else if(t=="j"){json_t=y;json_i=h;json_f=1;json_n=n?n:length(y);v=m?json_parse():NONE;h=json_i;}
		else if(t=="J"){json_t=y;json_i=h;json_f=1;json_n=n?n:length(y);v=m?love_parse():NONE;h=json_i;}
		else if(t=="i"){
			if(m&&substr(y,h,1)=="-"){h++;s=-1}else{s=1}
			m=m&&substr(y,h,1)~/[0-9]/
			z=0;while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)~/[0-9]/){z=z*10+substr(y,h++,1)};v=lmn(z*s)
		}
		else if(t~/[fcC]/){
			if(substr(y,h,1)=="-"){h++;s=-1}else{s=1}
			if(t=="c"&&m&&substr(y,h,1)=="$")h++
			m=m&&substr(y,h,1)~/[0-9.]/
			z=0;while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)~/[0-9]/){z=z*10+substr(y,h++,1)}
			if(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)==".")h++
			p=10;while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)~/[0-9]/){z+=substr(y,h++,1)/p;p*=10};v=lmn(z*s)
		}
		else if(t~/[hH]/){
			m=m&&substr(y,h,1)~/[0-9a-fA-F]/
			z=0;while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)~/[0-9a-fA-F]/){z=(z*16)+hexdigits[substr(y,h++,1)]};v=lmn(z)
		}
		else if(t~/[slu]/){
			z="";while(m&&h<=length(y)&&(n?h-si<n:1)&&(n?1:substr(y,h,1)!=substr(x,f,1)))z=z substr(y,h++,1)
			v=lms(t=="l"?tolower(z): t=="u"?toupper(z): z)
		}
		else if(t=="a"){
			v=lml();while(m&&h<=length(y)&&(n?h-si<n:1)&&(n?1:substr(y,h,1)!=substr(x,f,1)))lst_add(v,lmn(ascii[substr(y,h++,1)]))
		}
		else if(t=="b"){
			v=(substr(y,h,1)~/[tTyYx1]/)?ONE:NONE
			while(m&&h<=length(y)&&(n?h-si<n:1)&&(n?1:substr(y,h,1)!=substr(x,f,1)))h++
		}
		else if(t=="v"){
			m=m&&substr(y,h,1)~/[a-zA-Z_?]/
			z="";while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)~/[a-zA-Z0-9_?]/)z=z substr(y,h++,1);v=lms(z)
		}
		else if(t=="q"){
			m=m&&substr(y,h,1)=="\"";if(m)h++
			z="";while(m&&h<=length(y)&&(n?h-si<n:1)&&substr(y,h,1)!="\""){
				if(substr(y,h,1)=="\\"){h++;m=m&&substr(y,h,1)~/[\\"n]/;if(m){p=substr(y,h,1);z=z (p=="n"?"\n":p)}}
				else{z=z substr(y,h,1)}
				h++
			}
			m=m&&substr(y,h,1)=="\"";if(m)h++;v=lms(z)
		}
		else if(t~/[ro]/){
			d=max(1,d);p=f;for(z=0;m&&z<d;z++){if(f>length(x)){m=0}else{f++}};p=substr(x,p,d);
			v="";while(m&&h<=length(y)&&(n?h-si<n:1)){
				if((0<index(p,substr(y,h,1)))==lf){if(n)m=0;break;}
				v=v substr(y,h++,1);if(t=="o")break
			}v=lms(m?v:"")
		}
		else if(t~/[ep]/){
			if(m){
				p=l_parse(ISODATE,lms(substr(y,h)))
				k=lms("year" );dset(p,k,lmn(ln(dget(p,k))-1900))
				k=lms("month");dset(p,k,lmn(ln(dget(p,k))-   1))
				if(lb(dic_getv(p,7))){h+=ln(dic_getv(p,6))}else{m=0}
			}else{p=lmd()}
			if(t=="e"){
				yr=ln(dget(p,lms("year")))
				mo=ln(dget(p,lms("month")))
				z =ln(dget(p,lms("second")))+(60*ln(dget(p,lms("minute"))))+(3600*ln(dget(p,lms("hour"))))+86400*(ln(dget(p,lms("day")))-1)
				z+=(cumulative_month_days[mo+1]+leap_year(yr+1900))*86400
				z+=((yr-70)*31536000)+(int((yr-69)/4)*86400)-(int((yr-1)/100)*86400)+(int((yr+299)/400)*86400)
				v=lmn((!m)?0:z)
			}
			else{
				v=lmd()
				k=lms("year"  );dset(v,k,lmn(1900+ln(dget(p,k))))
				k=lms("month" );dset(v,k,lmn(   1+ln(dget(p,k))))
				k=lms("day"   );dset(v,k,            dget(p,k)  )
				k=lms("hour"  );dset(v,k,            dget(p,k)  )
				k=lms("minute");dset(v,k,            dget(p,k)  )
				k=lms("second");dset(v,k,            dget(p,k)  )
			}
		}
		else{m=0}
		while(n&&h<=length(y)&&h-si<n){h++;m=0}
		if(!sk&&v!=-1){if(named){dset(r,hn?lms(nk):lmn(pi),v)}else{lst_add(r,v)};pi++}
	};
	return named?r: count(r)==1?l_first(r): r
}
function format_json(x,  r,z,ct,c,e){
	if(lin(x))return wnum(lvn(x));if(lit(x))return format_json(l_rows(x))
	if(lil(x)){r="[";for(z=0;z<count(x);z++){if(z)r=r",";r=r format_json(lst_get(x,z))};return r"]"}
	if(lid(x)){r="{";for(z=0;z<count(x);z++){if(z)r=r",";r=r format_json(dic_getk(x,z))":"format_json(dic_getv(x,z))};return r"}"}
	if(lis(x)){
		x=lvs(x);r="";ct=0;for(z=1;z<=length(x);z++){
			c=substr(x,z,1);e=0
			if(c=="<"){ct=1}else if(c=="/"&&ct){e=1}else if(c!=" "&&c!="\n"){ct=0}
			if(c=="\n"){c="\\n"}else if(e||c~/[\\"]/){c="\\"c};r=r c
		}return "\""r"\""
	}return "null"
}
function format_love(x, r,z){
	if(lin(x)||lis(x))return format_json(x)
	if(lil(x)){r="[";for(z=0;z<count(x);z++){if(z)r=r",";r=r format_love(lst_get(x,z))};return r"]"}
	if(lid(x)){r="{";for(z=0;z<count(x);z++){if(z)r=r",";r=r format_love(dic_getk(x,z))":"format_love(dic_getv(x,z))};return r"}"}
	if(lit(x)){r="<";for(z=0;z<count(x);z++){if(z)r=r",";r=r format_love(tab_getk(x,z))":"format_love(tab_getv(x,z))};return r">"}
	if(lii(x)){return lvs(ls(interfaces(x,lms("encoded"),-1)))}
	return "null"
}
function format_type(a,t,n,d,lf,pz,  o,v,vn,r,z){
	o=""
	if     (t=="%")o="%"
	else if(t~/[sluvro]/)o=lvs(ls(a))
	else if(t=="a"){a=ll(a);for(z=0;z<count(a);z++){v=ordinal[int(ln(lst_get(a,z)))%256];o=o (length(v)?v:"?");}o=string_clean(o)}
	else if(t=="b"){o=lb(a)?"true":"false"}
	else if(t=="f"){o=d?sprintf("%.*f",d,ln(a)): wnum(ln(a))}
	else if(t=="c"){v=ln(a);o=sprintf("%s$%.*f",v<0?"-":"",d?d:2,abs(v))}
	else if(t=="C"){v=ln(a);o=sprintf("%s%.*f" ,v<0?"-":"",d?d:2,abs(v))}
	else if(t=="i"){o=""int(ln(a))}
	else if(t=="h"){o=sprintf("%x",int(ln(a)))}
	else if(t=="H"){o=sprintf("%X",int(ln(a)))}
	else if(t=="j"){o=format_json(a)}
	else if(t=="J"){o=format_love(a)}
	else if(t=="q"){o=format_json(ls(a))}
	else if(t=="e"){o=epoch_to_isodate(int(ln(a)))}
	else if(t=="p"){o=lvs(l_format(ISODATE,ld(a)))}
	vn=length(o);if(d&&t~/[fcC]/)d=0;if(d&&lf)vn=min(d,vn);if(t=="l")o=tolower(o);if(t=="u")o=toupper(o)
	r="";if(n&&!lf)for(z=0;z<n-vn;z++)r=r (pz?"0":" ")
	z=d&&!lf?max(0,vn-d):0;r=r substr(o,z+1,vn-z)
	if(n&&lf)for(z=0;z<n-vn;z++)r=r (pz?"0":" ");return r
}
function format_simple(x,t){return format_type(x,t,0,0,0,0)}
function format_rec(i,x,y,  fuse,named,a,r,z){
	if(i>=count(x))return y
	fuse=(count(x)-i)%2?0:1;named=format_has_names(lvs(ls(lst_get(x,i+fuse))))
	a=lit(y)?l_rows(y):ll(y);r=lml();for(z=0;z<count(a);z++){
		lst_add(r,l_format(lst_get(x,i+fuse),format_rec(i+fuse+1,x,lit(y)&&!named?ll(lst_get(a,z)):lst_get(a,z))))
	}return fuse?l_fuse(lst_get(x,i),r):r
}
function l_format(x,y,  r,f,h,named,nk,hn,sk,lf,pz,n,d,t,a,an){
	if(lil(x))return format_rec(0,x,y)
	r="";f=1;h=0;x=lvs(ls(x));named=format_has_names(x);y=named?ld(y): lil(y)?y: l_list(y)
	while(f<=length(x)){
		if(substr(x,f,1)!="%"){r=r substr(x,f,1);f++;continue};f++
		nk="";hn=0;if(substr(x,f,1)=="["){
			hn=1;f++;while(f<=length(x)&&substr(x,f,1)!="]"){nk=nk substr(x,f,1);f++;}
			if(substr(x,f,1)=="]")f++
		}
		sk=substr(x,f,1)=="*";if(sk)f++
		lf=substr(x,f,1)=="-";if(lf)f++
		pz=substr(x,f,1)=="0";if(pz)f++
		n=0;while(substr(x,f,1)~/[0-9]/){n=n*10+substr(x,f,1);f++};if(substr(x,f,1)==".")f++
		d=0;while(substr(x,f,1)~/[0-9]/){d=d*10+substr(x,f,1);f++};if(f>length(x))break
		t=substr(x,f,1);f++
		a=(t~/[sluvroq]/)?lms(""):NONE; an=named?dget(y,hn?lms(nk):lmn(h)): -1
		a=t=="%"?NONE: named?(an!=-1?an:a): (!sk&&h<count(y))?lst_get(y,h): a
		if(t~/[ro]/){lf=1;d=max(1,d);while(d&&substr(x,f,1)){d--;f++};d=n}
		r=r format_type(a,t,n,d,lf,pz);if(t!="%"&&!sk)h++
	}return lms(r)
}

function dyads_atomic(name,x,y,  r,z,k){
	if(lid(x)&&lid(y)){r=lmd()
		for(z=0;z<count(x);z++){k=dic_getk(x,z);dset(r,k,dyads_atomic(name,dic_getv(x,z),dgetv(y,k)))}
		for(z=0;z<count(y);z++){k=dic_getk(y,z);if(dget(x,k)==-1)dset(r,k,dyads_atomic(name,NONE,dic_getv(y,z)))};return r
	}
	if( lil(x)&& lil(y)){r=lml();for(z=0;z<count(x);z++)lst_add(r,dyads_atomic(name,lst_get(x,z),count(y)?lst_get(y,z%count(y)):NONE));return r}
	if( lid(x)&&!lid(y)){r=lmd();for(z=0;z<count(x);z++)dset(r,dic_getk(x,z),dyads_atomic(name,dic_getv(x,z),y));return r}
	if(!lid(x)&& lid(y)){r=lmd();for(z=0;z<count(y);z++)dset(r,dic_getk(y,z),dyads_atomic(name,x,dic_getv(y,z)));return r}
	if( lil(x)&&!lil(y)){r=lml();for(z=0;z<count(x);z++)lst_add(r,dyads_atomic(name,lst_get(x,z),y));return r}
	if(!lil(x)&& lil(y)){r=lml();for(z=0;z<count(y);z++)lst_add(r,dyads_atomic(name,x,lst_get(y,z)));return r}
	if(name=="+")return lmn(ln(x)+ln(y))
	if(name=="-")return lmn(ln(x)-ln(y))
	if(name=="*")return lmn(ln(x)*ln(y))
	if(name=="/")return ln(y)==0?NONE: lmn(ln(x)/ln(y))
	if(name=="%")return ln(x)==0?NONE: lmn(mod(ln(y),ln(x)))
	if(name=="^")return lmn(ln(x)^ln(y))
	if(name=="<")return l_less(x,y)
	if(name==">")return l_more(x,y)
	if(name=="=")return lmn(lii(x)||lii(y)?x==y: lin(x)&&lin(y)?lvn(x)==lvn(y): lvs(ls(x))==lvs(ls(y)))
	if(name=="&")return lin(x)||lin(y)?lmn(min(ln(x),ln(y))): lvs(ls(x))<lvs(ls(y))?ls(x):ls(y)
	if(name=="|")return lin(x)||lin(y)?lmn(max(ln(x),ln(y))): lvs(ls(x))>lvs(ls(y))?ls(x):ls(y)
	if(name=="bits_and")return lmn(bitwise_and(lbits(ln(x)),lbits(ln(y))))
	if(name=="bits_or" )return lmn(bitwise_or( lbits(ln(x)),lbits(ln(y))))
	if(name=="bits_xor")return lmn(bitwise_xor(lbits(ln(x)),lbits(ln(y))))
	return -1
}
function dyads(name,x,y,  r,z,u,gi,gr,ki,t,ik,dk,ai,bi,m){
	if(name=="split" )return l_split(x,y)
	if(name=="fuse"  )return l_fuse(x,y)
	if(name=="dict"  )return l_dict(x,y)
	if(name=="take"  )return l_take(x,y)
	if(name=="drop"  )return l_drop(x,y)
	if(name=="limit" )return l_limit(x,y)
	if(name=="in"    )return l_in(x,y)
	if(name=="like"  )return l_like(x,y)
	if(name=="window")return l_window(x,y)
	if(name=="parse" )return l_parse(x,y)
	if(name=="format")return l_format(x,y)
	if(name==","     )return l_cat(x,y)
 	if(name=="~"     )return matchr(x,y)?ONE:NONE
 	if(name=="unless")return lin(y)&&lvn(y)==0?x:y
	if(name=="join"  ){
		if(!lit(x)||!lit(y)){
			x=lin(x)?l_range(x):ll(x);y=lin(y)?l_range(y):ll(y)
			r=lml();for(z=0;z<count(x);z++)lst_add(r,l_cat(lst_get(x,z),count(y)==0?NONE:lst_get(y,z%count(y))));return r
		}
		ik=lml();dk=lml();r=l_take(NONE,x);for(z=0;z<count(y);z++){
			if(tab_getki(x,tab_getk(y,z))>=0){lst_add(ik,tab_getk(y,z))}else{lst_add(dk,lmn(z));tab_set(r,tab_getk(y,z),lml())}
		}
		for(ai=0;ai<rowcount(x);ai++)for(bi=0;bi<rowcount(y);bi++){
			m=1;for(z=0;z<count(ik);z++){if(!matchr(lst_get(tab_get(x,lst_get(ik,z)),ai),lst_get(tab_get(y,lst_get(ik,z)),bi))){m=0;break}}
			if(!m)continue
			for(z=0;z<count(x );z++)lst_add(tab_getv(r,z         ),lst_get(tab_getv(x,z                     ),ai))
			for(z=0;z<count(dk);z++)lst_add(tab_getv(r,z+count(x)),lst_get(tab_getv(y,int(ln(lst_get(dk,z)))),bi))
		};return torect(r)
	}
	if(name=="cross"){
		if(lin(x))x=l_range(x);if(lin(y))y=l_range(y);if(!lit(x)||!lit(y)){x=ll(x);y=ll(y)}
		if(lil(x)&&lil(y)){r=lml();for(z=0;z<count(x)*count(y);z++)lst_add(r,lml2(lst_get(x,z%count(x)),lst_get(y,int(z/count(x)))));return r}
		x=lt(x);y=lt(y);r=lmt();gr=rowcount(x)*rowcount(y)
		for(z=0;z<count(x);z++){t=lml();for(u=0;u<gr;u++)lst_add(t,lst_get(tab_getv(x,z),    u%rowcount(x) ));tab_set(  r,tab_getk(x,z),t)}
		for(z=0;z<count(y);z++){t=lml();for(u=0;u<gr;u++)lst_add(t,lst_get(tab_getv(y,z),int(u/rowcount(x))));tab_setuq(r,tab_getk(y,z),t)}
		return torect(r)
	}
	if(name=="@where"){
		x=l_take(lmn(rowcount(y)),ll(x))
		r=lml();for(z=0;z<count(x);z++)if(lb(lst_get(x,z)))lst_add(r,lmn(z));r=l_take(r,y)
		tab_set(r,lms("gindex"),l_range(lmn(rowcount(r))));return r
	}
	if(name=="@by"){
		x=l_take(lmn(rowcount(y)),ll(x));u=lmd();gi=lms("gindex");gr=lms("group")
		for(z=0;z<count(x);z++){
			ki=dgeti(u,lst_get(x,z));if(ki==-1){dset(u,lst_get(x,z),l_take(NONE,y));ki=count(u)-1}
			t=tab_row(y,z);dset(t,gi,lmn(rowcount(dic_getv(u,ki))));dset(t,gr,lmn(ki));tab_addrow(dic_getv(u,ki),t)
		};return ll(u)
	}
	return dyads_atomic(name,x,y)
}

BEGIN{merge_ix=-1}
function merge(vals,keys,widen,  i,t,r,z){
	i=lms("@index")
	if(!widen){merge_ix=lml();for(z=0;z<count(vals);z++)lst_addall(merge_ix,dget(lst_get(vals,z),i))}
	if(widen){t=lml();for(z=0;z<count(vals);z++)if(count(dget(lst_get(vals,z),i)))lst_add(t,lst_get(vals,z));vals=t}
	if(count(vals)==0){t=lmd();for(z=0;z<count(keys);z++)dset(t,lst_get(keys,z),lml());lst_add(vals,t)}
	r=lml();for(z=0;z<count(vals);z++)lst_add(r,l_table(widen?lst_get(vals,z):l_drop(i,lst_get(vals,z))));r=l_raze(r)
	if(widen){merge_ix=dget(r,i);r=l_drop(i,r)}
	return r
}
function disclose(x){return l_drop(lml3(lms("index"),lms("gindex"),lms("group")),x)}
function l_select(orig,vals,keys,  r){merge_ix=-1;r=merge(vals,keys,0);return (count(keys)>1)?r:l_take(merge_ix,disclose(orig))}

function lex_list(x,y,a,ix,  xv,yv){
	if(count(x)<ix&&count(y)<ix)return 0
	xv=count(x)>ix?lst_get(x,ix):NONE
	yv=count(y)>ix?lst_get(y,ix):NONE
	return lex_less(xv,yv)?a: lex_more(xv,yv)?!a: lex_list(x,y,a,ix+1)
}
function lex_less(x,y){return lil(x)&&lil(y)?lex_list(x,y,1,0): lb(l_less(x,y))}
function lex_more(x,y){return lil(x)&&lil(y)?lex_list(x,y,0,0): lb(l_more(x,y))}
function sort_swap(perm,i,j,  t){t=perm[i];perm[i]=perm[j];perm[j]=t}
function sort_compare(order_dir,order_vec,perm,i,j,  iv,jv){
	iv=lst_get(order_vec,perm[i])
	jv=lst_get(order_vec,perm[j])
	if(lex_less(iv,jv))return  order_dir
	if(lex_more(iv,jv))return -order_dir
	return i-j
}
function sort_by(order_dir,order_vec,  r,z,i,j,perm){
	# an insertion-sort, to keep things simple.
	# this routine assembles a permutation vector through which
	# the original data may be indexed to place it in order.
	# FIXME: swap out for mergesort or quicksort?
	for(z=0;z<count(order_vec);z++)perm[z]=z
	for(i=1;i<count(order_vec);i++){
		for(j=i;j>0&&sort_compare(order_dir,order_vec,perm,j-1,j)>0;j--)sort_swap(perm,j,j-1)
	}
	r=lml();for(z=0;z<count(order_vec);z++)lst_add(r,lmn(perm[z]));return r
}

function triads(name,x,y,z,  r,rc,i,j,col,ci,t){
	if(name=="@sel")return l_select(x,y,z)
	if(name=="@ext"){
		r=l_cols(l_select(x,y,z));
		return count(z)==1?(count(r)?l_first(r):lml()): count(r)!=1||count(dic_getk(r,0))?r:l_first(r)
	}
	if(name=="@upd"){
		x=disclose(x);merge_ix=-1;r=merge(y,z,1)
		for(i=0;i<count(r);i++){
			if(tab_getv(r,i)==merge_ix)continue
			t=tab_getk(r,i);ci=tab_getki(x,t);col=lml()
			for(j=0;j<rowcount(x);j++)lst_add(col,ci==-1?NONE:lst_get(tab_getv(x,ci),j));tab_set(x,t,col)
			for(j=0;j<count(merge_ix);j++){lst_set(col,int(ln(lst_get(merge_ix,j))),lst_get(tab_getv(r,i),j))}
		}return x
	}
	if(name=="@ins"){
		rc=ceil(count(x)/count(y));r=lml();for(i=0;i<count(y);i++){
			col=lml();lst_add(r,col);for(j=0;j<rc;j++){t=(count(y)*j)+i;lst_add(col,t>=count(x)?NONE:lst_get(x,t))}
		};r=l_table(l_dict(y,r));return lin(z)?r:l_cat(lt(z),r)
	}
	if(name=="@orderby"){
		x=l_take(lmn(rowcount(y)),ll(x))
		r=l_take(i=sort_by(ln(z),x),y);tab_set(r,lms("gindex"),l_range(lmn(rowcount(r))));return r
	}
}

###########################################################
#
#  Lil Natives
#
###########################################################

function show_dat(row,w,cols,  r,z,c){
	r="|";for(z=0;z<count(cols);z++){c=lvs(l_limit(lmn(w[z]),lst_get(dic_getv(cols,z),row)));r=r " "c repeat_char(" ",w[z]-length(c))" |"};return r
}
function show(ptr,toplevel,  r,z,t,w,cols,col,row,fmt,arr,c,d){
	if(ptr==-1)return "NULL"
	if(lin(ptr))return wnum(lvn(ptr))
	if(lis(ptr)){
		t=lvs(ptr);r=""
		for(z=1;z<=length(t);z++){c=substr(t,z,1);if(c=="\n"){c="\\n"}else if(c~/[\\"]/){c="\\"c};r=r c}
		return "\""r"\""
	}
	if(lil(ptr)){r="(";for(z=0;z<count(ptr);z++){if(z)r=r",";r=r show(lst_get(ptr,z))};return r")"}
	if(lid(ptr)){r="{";for(z=0;z<count(ptr);z++){if(z)r=r",";r=r show(dic_getk(ptr,z))":"show(dic_getv(ptr,z))};return r"}"}
	if(lion(ptr)){return "on "lon_name(ptr)" "lvs(l_fuse(lms(" "),lon_args(ptr)))(count(lon_args(ptr))?" ":"")"do ... end"}
	if(linat(ptr)){return "on native x do ... end"}
	if(lii(ptr)){return "<"li_name(ptr)">"}
	if(lit(ptr)&&!toplevel){
		d=rowcount(ptr)?lvs(l_fuse(lms(" "),l_raze(l_flip(l_range(ptr)))))" ":"";
		return "insert "lvs(ls(l_fuse(lms(" "),l_keys(ptr))))" with "d"end"
	}
	if(lit(ptr)){
		if(!count(ptr))return"++\n||\n++"
		cols=lml();for(z=0;z<count(ptr);z++){
			col=lml();lst_add(col,tab_getk(ptr,z));w[z]=count(tab_getk(ptr,z));lst_add(cols,col)
			for(row=0;row<rowcount(ptr);row++){fmt=show(lst_get(tab_getv(ptr,z),row));lst_add(col,lms(fmt));w[z]=min(40,max(w[z],length(fmt)))}
		}
		fmt="+";for(z=0;z<count(ptr);z++){fmt=fmt repeat_char("-",w[z]+2)"+"}
		r=fmt"\n"show_dat(0,w,cols)"\n";if(rowcount(ptr))r=r fmt "\n";for(z=1;z<=rowcount(ptr);z++){r=r show_dat(z,w,cols)"\n"}
		return r fmt
	}
	if(liblk(ptr)){
		c=blk_getops(ptr,arr)
		r="block {";for(z=0;z<c;z++)r=r"\n "z": "arr[z+1]
		r=r"\n} locals: <<";for(z=0;z<count(ptr);z++){if(z)r=r",";r=r show(lst_get(ptr,z))};return r">>"
	}
	if(lienv(ptr))return "<ENVIRONMENT>"
	if(ptr=="")return "INVALID"
	print "nyi debug show of ptr " ptr " with type " heap_type[ptr]
	exit 1
}

function writecsv(args,  r,z,t,s,delim,n,c,o,e,badchar){
	r="";t=lt(l_first(args));delim=count(args)>2?lvs(l_first(ls(lst_get(args,2)))):","
	badchar="[\\n\""regex_esc(delim)"]"
	s=count(args)>1?lvs(ls(lst_get(args,1))): repeat_char("s",count(t));gsub(/I/,"i",s);gsub(/B/,"b",s);gsub(/L/,"s",s)
	n=0;for(c=1;c<=length(s);c++)if(substr(s,c,1)!="_"){if(n++)r=r delim;r=r (c>count(t)?("c"c): lvs(tab_getk(t,c-1)))}
	for(z=0;z<rowcount(t);z++){
		r=r"\n";n=0;for(c=1;c<=length(s);c++)if(substr(s,c,1)!="_"){
			o=format_simple(c>count(t)?lms(""): lst_get(tab_getv(t,c-1),z),substr(s,c,1))
			gsub(/["]/,"\"\"",o);r=r (n++?delim:"")((o~badchar)?("\""o"\""):o)
		}
	}return lms(r)
}
function readcsv(args,  r,z,t,s,hass,delim,i,n,slots,slot,key,fmts,val,u){
	r=lmt();t=lvs(ls(l_first(args)));delim=count(args)>2?lvs(l_first(lst_get(args,2))):","
	s=count(args)>1?lvs(ls(lst_get(args,1))):""; hass=count(args)>1; i=1;n=0;slot=0
	while(i<=length(t)&&substr(t,i,1)!="\n"){
		while(substr(t,i,1)==" ")i++
		key="";while(i<=length(t)&&substr(t,i,1)!="\n"&&substr(t,i,1)!=delim)key=key substr(t,i++,1)
		if(!hass||(n<length(s)&&substr(s,n+1,1)!="_"))tab_set(r,lms(key),lml());n++
		if(substr(t,i++,1)=="\n")break; while(substr(t,i,1)==" ")i++; if(substr(t,i,1)==delim)i++
	}
	while(hass&&n<length(s))if(substr(s,++n,1)!="_")tab_set(r,lms("c"(n-1)),lml())
	if(!hass)s=repeat_char("s",count(r));gsub(/I/,"i",s);gsub(/B/,"b",s);gsub(/L/,"s",s)
	slots=0;for(z=1;z<=length(s);z++)if(substr(s,z,1)!="_")slots++;slots=min(slots,count(r))
	fmts=lml();for(z=1;z<=length(s);z++)lst_add(fmts,lms("%"substr(s,z,1)))
	n=0;while(i<=length(t)){
		while(substr(t,i,1)==" ")i++;val=""
		if(substr(t,i,1)=="\""){
			i++;while(i<=length(t)){
				if(substr(t,i,1)=="\""){i++;if(substr(t,i,1)=="\""){val=val "\"";i++}else{break}}
				else{val=val substr(t,i++,1)}
			}
		}
		else{while(i<=length(t)&&substr(t,i,1)~/[^\n"]/&&substr(t,i,1)!=delim)val=val substr(t,i++,1)}
		if(n<length(s)&&substr(s,n+1,1)!="_"){lst_add(tab_getv(r,slot++),l_parse(lst_get(fmts,n),lms(val)))};n++
		if(i>=length(t)||substr(t,i,1)=="\n"){
			while(n<length(s)){u=substr(s,++n,1); if(u!="_"&&slot<slots)lst_add(tab_getv(r,slot++),u~/[sluvroq]/?lms(""):NONE)}
			if(substr(t,i,1)=="\n"&&i==length(s))break; i++;n=0;slot=0
		}else{while(substr(t,i,1)==" ")i++;if(substr(t,i,1)==delim)i++}
	};return torect(r)
}

function writexmlstr(x,  r,z,c){
	x=lvs(ls(x));r=""
	for(z=1;z<=length(x);z++){c=substr(x,z,1);r=r (c=="&"?"&amp;": c=="'"?"&apos;": c=="\""?"&quot;": c==">"?"&gt;": c=="<"?"&lt;": c)}
	return r
}
function writexmlrec(x,tab,fmt,  r,z,t,a,c,ck){
	if(lil(x)){r="";for(z=0;z<count(x);z++)r=r writexmlrec(lst_get(x,z),tab,fmt);return r}
	if(lii(x)&&array_is(x)){ck=array_cast(x);array_set_cast(x,"char");r=lvs(array_get(x,0,array_size(x)));array_set_cast(x,ck);return r}
	if(!lid(x)){return writexmlstr(x) (tab&&fmt?"\n":"")}
	t=lvs(ls(dgetv(x,lms("tag"))));a=ld(dgetv(x,lms("attr")));c=dget(x,lms("children"));c=c==-1?lml():ll(c)
	r="<"t;for(z=0;z<count(a);z++){r=r" "lvs(ls(dic_getk(a,z)))"=\""writexmlstr(dic_getv(a,z))"\""}
	if(!count(c)){return r"/>"(fmt?"\n":"")};r=r">"(fmt?"\n":"")
	for(z=0;z<count(c);z++){if(fmt)r=r repeat_char(" ",tab+2);r=r writexmlrec(lst_get(c,z),tab+2,fmt)}
	if(fmt)r=r repeat_char(" ",tab);return r"</"t">"(fmt?"\n":"")
}
function writexml(args){return lms(writexmlrec(l_first(args),0,count(args)>1?lb(lst_get(args,1)):0))}

BEGIN{xml_t="";xml_i=1;}
function xml_hn(){return xml_i<=length(xml_t)}
function xml_nx(){return substr(xml_t,xml_i++,1)}
function xml_ws(  r){r=0;while(substr(xml_t,xml_i,1)~/[ \n]/){xml_i++;r=1};return r}
function xml_cl(){while(xml_hn()&&substr(xml_t,xml_i,1)~/[^>]/)xml_i++;xml_i++}
function xml_m(x){if(substr(xml_t,xml_i,1)==x){xml_i++;return 1}return 0}
function readxmltext(stop,  r){
	r="";while(xml_hn()){
		if(stop==" "&&(substr(xml_t,xml_i,1)~/[>\/ ]/||substr(xml_t,xml_i,1)=="\n"))break
		if(xml_ws())r=r" ";if(!xml_hn()||substr(xml_t,xml_i,1)==stop)break
		if     (substr(xml_t,xml_i,5)=="&amp;" ){xml_i+=5;r=r"&"}
		else if(substr(xml_t,xml_i,6)=="&apos;"){xml_i+=6;r=r"'"}
		else if(substr(xml_t,xml_i,6)=="&quot;"){xml_i+=6;r=r"\""}
		else if(substr(xml_t,xml_i,4)=="&gt;"  ){xml_i+=4;r=r">"}
		else if(substr(xml_t,xml_i,4)=="&lt;"  ){xml_i+=4;r=r"<"}
		else if(substr(xml_t,xml_i,6)=="&nbsp;"){xml_i+=6;r=r" "}
		else{r=r xml_nx()}
	}
	if(xml_hn()&&stop~/['"]/)xml_i++;return lms(r)
}
function readxmlname(  r){
	xml_ws();
	r="";while(xml_hn()&&substr(xml_t,xml_i,1)~/[^>\/= \n]/)r=r xml_nx()
	xml_ws();return lms(tolower(r))
}
function readxmlrec(currtag,  r,w,tag,attr,n,an,c){
	r=lml();while(xml_hn()){
		w=xml_ws()
		if(substr(xml_t,xml_i,9)=="<![CDATA["){
			xml_i+=9;c=""
			while(xml_hn()&&substr(xml_t,xml_i,3)!="]]>")c=c xml_nx()
			xml_i+=3;lst_add(r,lms(c));continue
		}
		if(substr(xml_t,xml_i,1)!="<"){if(w)xml_i--;lst_add(r,readxmltext("<"));continue}
		xml_i++;xml_ws()
		if(xml_m("!")||xml_m("?")){xml_cl();continue} # skip pragmas/comments/prologue
		if(xml_m("/")){an=readxmlname();xml_m(">");if(currtag==lvs(an))break;continue}

		tag=lmd();lst_add(r,tag); n=readxmlname();dset(tag,lms("tag"),n)
		attr=lmd();dset(tag,lms("attr"),attr); dset(tag,lms("children"),lml())

		while(xml_hn()&&substr(xml_t,xml_i,1)~/[^\/>]/){
			an=readxmlname()
			if(xml_m("=")){xml_ws();dset(attr,an,readxmltext((xml_m("'"))?"'":(xml_m("\""))?"\"":" "));}
			else{dset(attr,an,ONE)}
		}
		if(xml_m("/")){xml_cl()}else{if(xml_hn())xml_i++;dset(tag,lms("children"),readxmlrec(lvs(n)))}
	};return r
}
function readxml(args){xml_t=lvs(ls(l_first(args)));xml_i=1;return readxmlrec("")}

BEGIN{random_seed=74565}
function random_int(x,  t){
	# xorshift RNG: crude, but straightforward.
	# we're (ab)using the built-in awk rng for other purposes,
	# so we'd need to write something from scratch anyway.
	t=random_seed;t=unsigned_i(t)
	t=bitwise_xor(t,   (t*  8192));t=mod(t,4294967296) # t^=(t<<13)
	t=bitwise_xor(t,int(t/131072));t=mod(t,4294967296) # t^=(t>>17)
	t=bitwise_xor(t,   (t* 32768));t=mod(t,4294967296) # t^=(t<<15)
	t=signed_i(t);random_seed=t;
	return mod(random_seed,x)
}
function random_element(x){
	return lin(x)?lmn(random_int(ln(x))): count(x)<1?NONE: lit(x)?l_at(x,lmn(random_int(rowcount(x)))): l_at(x,lmn(random_int(count(x))))
}
function conform_bits(name,args,  r,z){
	if(count(args)<2)args=l_first(args)
	r=l_first(args);for(z=1;z<count(args);z++)r=dyads_atomic(name,r,lst_get(args,z))
	return r
}
function enumerate_dir(str,  c,t,r,rd,rn,rt,p,pc){
	c="ls -la "str;rd=lml();rn=lml();rt=lml();r=lmt()
	tab_set(r,lms("dir"),rd);tab_set(r,lms("name"),rn);tab_set(r,lms("type"),rt)
	while(1){
		t=0;c | getline t;if(t==0)break;if(t~/^total/)continue
		pc=split(t,p," ");if(p[pc]~/^\./)continue
		lst_add(rd,p[1]~/d/?ONE:NONE);lst_add(rn,lms(p[pc]))
		pc=split(p[pc],p,".");lst_add(rt,pc>1?lms("."p[pc]):lms(""))
	}close(c);return torect(r)
}

function natives(name,self,args,  z,r,prog,vars,n,t,pv,j,file){
	if(name=="exit")exit ln(l_first(args))
	if(name=="show"){
		if(count(args)<2){print show(l_first(args),1)}
		else{r="";for(z=0;z<count(args);z++){if(z)r=r" ";r=r show(lst_get(args,z),0)};print r}
		return l_first(args)
	}
	if(name=="print"){
		if(count(args)==1&&array_is(l_first(args))){printf array_print(l_first(args));return l_first(args)}
		if(count(args)<2){print lvs(ls(l_first(args)));return args}
		else{r=l_format(ls(l_first(args)),l_drop(ONE,args));print lvs(r);return r}
	}
	if(name=="error"){
		if(count(args)==1&&array_is(l_first(args))){print_error(array_print(l_first(args)));return l_first(args)}
		if(count(args)<2){print_error(lvs(ls(l_first(args))));return args}
		else{r=l_format(ls(l_first(args)),l_drop(ONE,args));print_error(lvs(r));return r}
	}
	if(name=="input"){
		printf lvs(count(args)<2?ls(l_first(args)): l_format(ls(l_first(args)),l_drop(ONE,args)))
		r=readline();return r==0?NONE:lms(r)
	}
	if(name=="random"){
		if(count(args)<1){random_int(1);return lmn(bitwise_and(unsigned_i(random_seed),2147483647)/2147483647)}
		x=l_first(args);if(count(args)<2)return random_element(x);n=ln(lst_get(args,1))
		if(n>=0){r=lml();for(z=0;z<n;z++)lst_add(r,random_element(x));return r}
		x=lin(x)?l_range(x):ll(x);if(count(x)<1)x=l_list(NONE);for(z=0;z<count(x);z++)pv[z]=z
		for(z=count(x)-1;z>0;z--){j=random_int(z+1);t=pv[j];pv[j]=pv[z];pv[z]=t}
		r=lml();for(z=0;z<abs(n);z++){lst_add(r,lst_get(x,pv[z%count(x)]))};return r
	}
	if(name=="dir"){return enumerate_dir(lvs(ls(l_first(args))))}
	if(name=="shell"){
		args=lvs(ls(l_first(args)));n=""
		# FIXME: I don't see a straightforward way to capture both stdout and exit code:
		while(1){t=0;args | getline t;if(t==0)break;n=n (length(n)?"\n":"")t};close(args)
		r=lmd();dset(r,lms("exit"),lmn(-1));dset(r,lms("out"),lms(n));return r
	}
	if(name=="eval"){
		r=lmd();dset(r,lms("value"),NONE)
		vars=count(args)>1?ld(lst_get(args,1)):lmd();dset(r,lms("vars" ),vars)
		prog=parse(lvs(ls(l_first(args))));
		if(perr!=0){dset(r,lms("error"),lms(perr));dset(r,lms("errorpos"),lml2(lmn(parse_r),lmn(parse_c)));return r}
		blk_opa(prog,"BUND",2);blk_lit(prog,lmnat("feval","feval"));blk_op(prog,"SWAP");blk_op(prog,"CALL")
		issue(env_bind(count(args)>2&&lb(lst_get(args,2))?ev():lmenv(),vars), prog);return r
	}
	if(name=="feval"){
		r=lst_get(args,0)
		dset(r,lms("value"),lst_get(args,1))
		dset(r,lms("vars" ),env_enumerate(ev()))
		return r
	}
	if(name=="import"){
		file=readtext(lvs(ls(l_first(args))));if(!length(file))return NONE
		prog=parse(file);if(perr!=0)return NONE
		blk_opa(prog,"BUND",2);blk_lit(prog,lmnat("fimport","fimport"));blk_op(prog,"SWAP");blk_op(prog,"CALL")
		issue(env_bind(ev(),lmd()),prog);return NONE
	}
	if(name=="fimport"){return env_enumerate(ev())}
	if(name=="uplevel"){
		z=2;r=-1;n=lvs(ls(args));vars=ev();
		while(vars!=-1&&z){r=eget(vars,n);if(r!=-1)z--;vars=env_parent(vars)}
		return r==-1?NONE:r
	}
	if(name=="read"){
		t=lvs(ls(l_first(args)))
		return (count(args)>1&&lvs(ls(lst_get(args,1)))=="array")?readbytes(t): lms(readtext(t))
	}
	if(name=="write"){
		r=count(args)>1?lst_get(args,1):lms("");t=lvs(ls(l_first(args)))
		return lmn(array_is(r)?writebytes(t,r): writetext(t,lvs(ls(r))))
	}
	if(name=="writecsv"       )return writecsv(args)
	if(name=="readcsv"        )return readcsv(args)
	if(name=="writexml"       )return writexml(args)
	if(name=="readxml"        )return readxml(args)
	if(name=="array"          )return n_array(args)
	if(name=="image"          )return n_image(args)
	if(name=="bits_and"       )return conform_bits(name,args)
	if(name=="bits_or"        )return conform_bits(name,args)
	if(name=="bits_xor"       )return conform_bits(name,args)
	if(name=="rtext_make"     )return n_rtext_make(args)
	if(name=="rtext_len"      )return n_rtext_len(args)
	if(name=="rtext_get"      )return n_rtext_get(args)
	if(name=="rtext_index"    )return n_rtext_index(args)
	if(name=="rtext_string"   )return n_rtext_string(args)
	if(name=="rtext_span"     )return n_rtext_span(args)
	if(name=="rtext_split"    )return n_rtext_split(args)
	if(name=="rtext_replace"  )return n_rtext_replace(args)
	if(name=="rtext_find"     )return n_rtext_find(args)
	if(name=="rtext_cat"      )return n_rtext_cat(args)
	if(name=="image_map"      )return n_image_map(self,args)
	if(name=="image_merge"    )return n_image_merge(self,args)
	if(name=="image_transform")return n_image_transform(self,args)
	if(name=="image_rotate"   )return n_image_rotate(self,args)
	if(name=="image_translate")return n_image_translate(self,args)
	if(name=="image_scale"    )return n_image_scale(self,args)
	if(name=="image_copy"     )return n_image_copy(self,args)
	if(name=="image_paste"    )return n_image_paste(self,args)
	if(name=="array_struct"   )return n_array_struct(self,args)
	if(name=="array_slice"    )return n_array_slice(self,args)
	if(name=="array_copy"     )return n_array_copy(self,args)
	if(name=="array_cat"      )return n_array_cat(self,args)
	return NONE
}

###########################################################
#
#  Lil Interfaces
#
###########################################################

function interfaces(self,i,x,  r){
	if(li_name(self)=="system"){
		if(x!=-1&&lis(i)){
			if(lvs(i)=="seed"){random_seed=int(ln(x));return x}
		}else if(lis(i)){
			if(lvs(i)=="version"  )return lms(VERSION)
			if(lvs(i)=="platform" )return lms("awk")
			if(lvs(i)=="seed"     )return lmn(random_seed)
			if(lvs(i)=="now"      )return lmn(     get_epoch())
			if(lvs(i)=="ms"       )return lmn(1000*get_epoch())
			if(lvs(i)=="workspace"){
				r=lmd()
				dset(r,lms("allocs"),lmn(gc_allocs         ))
				dset(r,lms("frees" ),lmn(gc_frees          ))
				dset(r,lms("gcs"   ),lmn(gc_gen            ))
				dset(r,lms("live"  ),lmn(gc_allocs-gc_frees))
				dset(r,lms("heap"  ),lmn(gc_hi             ))
				dset(r,lms("depth" ),lmn(call_depth        ))
				return r
			}
		}
	}
	else if(li_name(self)=="bits"){
		if(lvs(i)=="and")return lmnat("and","bits_and")
		if(lvs(i)=="or" )return lmnat("or" ,"bits_or" )
		if(lvs(i)=="xor")return lmnat("xor","bits_xor")
	}
	else if(li_name(self)=="rtext"){
		if(lvs(i)=="end"    )return lmn(RTEXT_END)
		if(lvs(i)=="make"   )return lmnat("make"   ,"rtext_make"   )
		if(lvs(i)=="len"    )return lmnat("len"    ,"rtext_len"    )
		if(lvs(i)=="get"    )return lmnat("get"    ,"rtext_get"    )
		if(lvs(i)=="index"  )return lmnat("index"  ,"rtext_index"  )
		if(lvs(i)=="string" )return lmnat("string" ,"rtext_string" )
		if(lvs(i)=="span"   )return lmnat("span"   ,"rtext_span"   )
		if(lvs(i)=="split"  )return lmnat("split"  ,"rtext_split"  )
		if(lvs(i)=="replace")return lmnat("replace","rtext_replace")
		if(lvs(i)=="find"   )return lmnat("find"   ,"rtext_find"   )
		if(lvs(i)=="cat"    )return lmnat("cat"    ,"rtext_cat"    )
	}
	else if(li_name(self)=="image"){
		if(i&&lil(i)){return x!=-1?n_image_set_px(self,i,x):n_image_get_px(self,i)}
		if(lvs(i)=="pixels"){return x!=-1?n_image_set_pixels(self,x):n_image_get_pixels(self)}
		if(lvs(i)=="size"  ){return x!=-1?n_image_resize(self,x):lml2(lmn(image_w(self)),lmn(image_h(self)))}
		if(lvs(i)=="map"      )return lmnat("map"      ,"image_map"      ,self)
		if(lvs(i)=="merge"    )return lmnat("merge"    ,"image_merge"    ,self)
		if(lvs(i)=="transform")return lmnat("transform","image_transform",self)
		if(lvs(i)=="rotate"   )return lmnat("rotate"   ,"image_rotate"   ,self)
		if(lvs(i)=="translate")return lmnat("translate","image_translate",self)
		if(lvs(i)=="scale"    )return lmnat("scale"    ,"image_scale"    ,self)
		if(lvs(i)=="copy"     )return lmnat("copy"     ,"image_copy"     ,self)
		if(lvs(i)=="paste"    )return lmnat("paste"    ,"image_paste"    ,self)
		if(lvs(i)=="encoded"  )return image_write(self)
		if(lvs(i)=="hist"     )return image_hist(self)
		if(lvs(i)=="bounds"   )return image_bounds(self)
	}
	else if(li_name(self)=="array"){
		if(!lis(i))return x==-1?n_array_get(self,i):n_array_set(self,i,x)
		if(x!=-1){
			if(lvs(i)=="cast"   ){array_set_cast(self,lvs(ls(x)));return x}
			if(lvs(i)=="size"   ){array_set_lsize(self,int(ln(x)));return x}
			if(lvs(i)=="here"   ){array_set_here(self,int(max(0,ln(x))));return x}
		}else{
			if(lvs(i)=="encoded")return array_write(self)
			if(lvs(i)=="cast"   )return lms(array_cast(self))
			if(lvs(i)=="size"   )return lmn(array_lsize(self))
			if(lvs(i)=="here"   )return lmn(array_here(self))
			if(lvs(i)=="struct" )return lmnat("struct","array_struct",self)
			if(lvs(i)=="slice"  )return lmnat("slice" ,"array_slice" ,self)
			if(lvs(i)=="copy"   )return lmnat("copy"  ,"array_copy"  ,self)
			if(lvs(i)=="cat"    )return lmnat("cat"   ,"array_cat"   ,self)
		}
	}
	return x!=-1?x:NONE
}

BEGIN{
	base64_chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
	for(z=1;z<=length(base64_chars);z++){
		base64_enc[z-1]=substr(base64_chars,z,1)
		base64_dec[substr(base64_chars,z,1)]=z-1
	}
	base64_dec["="]=-1
}
function base64_read(str,arr,  c,z,tmp,o){
	c=0;for(z=1;z<=length(str);z+=4){
		for(o=0;o<4&&z+o<=length(str);o++){tmp[o]=base64_dec[substr(str,z+o,1)];if(tmp[o]==-1)break}
		if(o>=2){arr[c++]=((tmp[0]* 4)%256)+int(tmp[1]/16)}
		if(o>=3){arr[c++]=((tmp[1]*16)%256)+int(tmp[2]/ 4)}
		if(o>=4){arr[c++]=((tmp[2]*64)%256)+int(tmp[3]   )}
	};return c
}
function base64_write(arr,c,  z,r){
	r="";for(z=0;z<c;z+=3){
		r=r base64_enc[          (int(arr[z  ]/4)                                   )%64]
		r=r base64_enc[          (((  arr[z  ]*16)%256)+(z+1>=c?0: int(arr[z+1]/16)))%64]
		r=r base64_enc[z+1>=c?64:(((  arr[z+1]* 4)%256)+(z+2>=c?0: int(arr[z+2]/64)))%64]
		r=r base64_enc[z+2>=c?64:(                      (          int(arr[z+2]   )))%64]
	}return r
}
function data_fmt(str){return substr(str,6,1)}
function data_read(type,str,arr){return length(str)<6||substr(str,1,5)!=("%%"type)?0: base64_read(substr(str,7),arr)}
function data_write(type,arr,c){return "%%"type base64_write(arr,c)}

###########################################################
#
#  The Array Interface
#
###########################################################

BEGIN{
	for(z=0;z<256;z++){hex=sprintf("%02X",z);b2h[z]=hex;h2b[hex]=z}
	cast_size["u8"  ]=1;cast_decode["0"]="u8"  ;cast_encode["u8"  ]="0"
	cast_size["i8"  ]=1;cast_decode["1"]="i8"  ;cast_encode["i8"  ]="1"
	cast_size["u16b"]=2;cast_decode["2"]="u16b";cast_encode["u16b"]="2"
	cast_size["u16l"]=2;cast_decode["3"]="u16l";cast_encode["u16l"]="3"
	cast_size["i16b"]=2;cast_decode["4"]="i16b";cast_encode["i16b"]="4"
	cast_size["i16l"]=2;cast_decode["5"]="i16l";cast_encode["i16l"]="5"
	cast_size["u32b"]=4;cast_decode["6"]="u32b";cast_encode["u32b"]="6"
	cast_size["u32l"]=4;cast_decode["7"]="u32l";cast_encode["u32l"]="7"
	cast_size["i32b"]=4;cast_decode["8"]="i32b";cast_encode["i32b"]="8"
	cast_size["i32l"]=4;cast_decode["9"]="i32l";cast_encode["i32l"]="9"
	cast_size["char"]=1;cast_decode[":"]="char";cast_encode["char"]=":"
}
function array_is(x){return x!=-1&&lii(x)&&li_name(x)=="array"}
function array_make(cast,size){
	r=lmi("array")
	heap_k[r]=cast                          # cast: data interpretation
	heap_c[r]=size*cast_size[cast]          # size: raw byte count
	heap_v[r]=repeat_char("00",2*heap_c[r]) # data: string of hex bytes OR base: offset to slice
	#heap_n[r]=0                            # here: offset for struct[] and general use
	#heap_aux[r]=-1                         # if not -1, this is a slice; refer to that array's data.
	return r
}
function array_cast(arr){return heap_k[arr]}
function array_here(arr){return heap_n[arr]}
function array_size(arr){return heap_c[arr]}
function array_is_slice(arr,x){return heap_aux[arr]!=-1}
function array_base(arr){return 0+heap_v[arr]}
function array_set_cast(arr,x){if(!cast_size[x])x="u8";heap_k[arr]=x}
function array_set_here(arr,x){heap_n[arr]=x}
function array_lsize(arr){return int(array_size(arr)/cast_size[array_cast(arr)])}
function array_set_lsize(arr,x){array_resize(arr,x*cast_size[array_cast(arr)])}
function array_append_byte(arr,x){heap_v[arr]=heap_v[arr] b2h[x];heap_c[arr]++}
function array_get_byte(arr,x){
	if(array_is_slice(arr))return array_get_byte(heap_aux[arr],x+array_base(arr))
	if(x<0||x*2>=length(heap_v[arr]))return 0
	return h2b[substr(heap_v[arr],1+2*x,2)]
}
function array_set_byte(arr,x,v){
	if(array_is_slice(arr)){array_set_byte(heap_aux[arr],x+array_base(arr),v);return}
	if(x<0||x*2>=length(heap_v[arr]))return;v=mod(int(v),256)
	t=heap_v[arr];i=1+2*x;heap_v[arr]=substr(t,1,i-1) b2h[v] substr(t,i+2)
}
function array_resize(arr,bytes,  r){
	if(array_is_slice(arr))return
	r="";for(z=0;z<bytes;z++)r=r b2h[array_get_byte(arr,z)]
	heap_v[arr]=r;heap_c[arr]=bytes
}

function signed_b(n){return n<=       127?n:       -128+(n-128       )}
function signed_s(n){return n<=     32767?n:     -32768+(n-32768     )}
function signed_i(n){return n<=2147483647?n:-2147483648+(n-2147483648)}
function unsigned_b(n){return n>=0?n:        256+n}
function unsigned_s(n){return n>=0?n:      65536+n}
function unsigned_i(n){return n>=0?n: 4294967296+n}

function array_get_raw(arr,ix,  step){
	step=cast_size[array_cast(arr)]
	if(ix<0||ix>=array_lsize(arr))return 0;ix=step*ix
	if(array_cast(arr)=="u8"  )return           array_get_byte(arr,ix)
	if(array_cast(arr)=="i8"  )return signed_b( array_get_byte(arr,ix))
	if(array_cast(arr)=="u16b")return          (array_get_byte(arr,ix  )*2^8)+array_get_byte(arr,ix+1)
	if(array_cast(arr)=="u16l")return          (array_get_byte(arr,ix+1)*2^8)+array_get_byte(arr,ix  )
	if(array_cast(arr)=="i16b")return signed_s((array_get_byte(arr,ix  )*2^8)+array_get_byte(arr,ix+1))
	if(array_cast(arr)=="i16l")return signed_s((array_get_byte(arr,ix+1)*2^8)+array_get_byte(arr,ix  ))
	if(array_cast(arr)=="u32b")return          (array_get_byte(arr,ix  )*2^24)+(array_get_byte(arr,ix+1)*2^16)+(array_get_byte(arr,ix+2)*2^8)+array_get_byte(arr,ix+3)
	if(array_cast(arr)=="u32l")return          (array_get_byte(arr,ix+3)*2^24)+(array_get_byte(arr,ix+2)*2^16)+(array_get_byte(arr,ix+1)*2^8)+array_get_byte(arr,ix  )
	if(array_cast(arr)=="i32b")return signed_i((array_get_byte(arr,ix  )*2^24)+(array_get_byte(arr,ix+1)*2^16)+(array_get_byte(arr,ix+2)*2^8)+array_get_byte(arr,ix+3))
	if(array_cast(arr)=="i32l")return signed_i((array_get_byte(arr,ix+3)*2^24)+(array_get_byte(arr,ix+2)*2^16)+(array_get_byte(arr,ix+1)*2^8)+array_get_byte(arr,ix  ))
	return array_get_byte(arr,ix)
}
function array_set_raw(arr,ix,v){
	step=cast_size[array_cast(arr)]
	if(ix<0||ix>=array_lsize(arr))return 0;ix=step*ix
	v=array_cast(arr)~/32/?unsigned_i(v): array_cast(arr)~/16/?unsigned_s(v): unsigned_b(v)
	if     (array_cast(arr)~/16b/){array_set_byte(arr,ix,  v/(2^8));array_set_byte(arr,ix+1,v)}
	else if(array_cast(arr)~/16l/){array_set_byte(arr,ix+1,v/(2^8));array_set_byte(arr,ix  ,v)}
	else if(array_cast(arr)~/32b/){array_set_byte(arr,ix  ,v/(2^24));array_set_byte(arr,ix+1,v/(2^16));array_set_byte(arr,ix+2,v/(2^8));array_set_byte(arr,ix+3,v);}
	else if(array_cast(arr)~/32l/){array_set_byte(arr,ix+3,v/(2^24));array_set_byte(arr,ix+2,v/(2^16));array_set_byte(arr,ix+1,v/(2^8));array_set_byte(arr,ix  ,v);}
	else{array_set_byte(arr,ix,v)}
}
function array_get(arr,offset,len,  r,z,bytes){
	if(array_cast(arr)=="char"){if(len<0)len=1;for(z=0;z<len;z++)bytes[z]=array_get_raw(arr,offset+z);return lms(utf8_strip(bytes,z))}
	if(len<0)return lmn(array_get_raw(arr,offset))
	r=lml();for(z=0;z<len;z++)lst_add(r,lmn(array_get_raw(arr,offset+z)));return r
}
function array_set(arr,offset,len,v,  z){
	if(len<0)len=1
	if(array_is(v)){for(z=0;z<len;z++)array_set_raw(arr,offset+z,array_get_raw(v,z)                      )}
	else if(lis(v)){for(z=0;z<len;z++)array_set_raw(arr,offset+z,z>=count(v)?0: ascii[substr(lvs(v),1+z,1)])}
	else if(lil(v)){for(z=0;z<len;z++)array_set_raw(arr,offset+z,z>=count(v)?0: ln(lst_get(v,z)))}
	else{v=ln(v);   for(z=0;z<len;z++)array_set_raw(arr,offset+z,v)}
}
function n_array_slice(arr,args,  i,offset,len,cast,step,r){
	i=l_first(args);offset=int(ln(lil(i)?l_first(i):i));len=lil(i)?int(max(0,ln(l_last(i)))):-1
	cast=count(args)<2?array_cast(arr): lvs(ls(lst_get(args,1)));if(!cast_size[cast])cast="u8"
	step=cast_size[cast];offset*=cast_size[array_cast(arr)];if(len<0)len=int((array_size(arr)-offset)/step)
	r=array_make(cast,len);heap_aux[r]=arr;heap_v[r]=offset;return r
}
function n_array_copy(arr,args,  i,offset,len,cast,step,r,z){
	i=l_first(args);offset=int(ln(lil(i)?l_first(i):i));len=lil(i)?int(max(0,ln(l_last(i)))):-1
	cast=count(args)<2?array_cast(arr): lvs(ls(lst_get(args,1)));if(!cast_size[cast])cast="u8"
	step=cast_size[cast];offset*=cast_size[array_cast(arr)];if(len<0)len=int((array_size(arr)-offset)/step)
	r=array_make(cast,0);for(z=0;z<len*step;z++)array_append_byte(r,array_get_byte(arr,offset+z));return r
}

function struct_size(shape,  bit,r,z,type){
	if(lis(shape))return max(1,cast_size[lvs(shape)])
	if(lil(shape))return max(1,cast_size[lvs(ls(l_first(shape)))])*max(0,ln(l_last(shape)))
	if(!lid(shape))return 0
	bit=0;r=0;for(z=0;z<count(shape);z++){
		type=dic_getv(shape,z)
		if(!lin(type)&&bit){bit=0;r++}
		if(lin(type)){bit+=min(max(1,ln(type)),31);r+=int(bit/8);bit%=8}
		else{r+=struct_size(type)}
	}return r
}

function array_slice_here(arr,cast,  r){
	r=array_make(cast,array_size(arr)-array_here(arr));heap_aux[r]=arr;heap_v[r]=array_here(arr);return r
}
function struct_read(arr,shape,  cast,r,n,z,bit,type,v,t,slice){
	if(lis(shape)){
		r=array_get(array_slice_here(arr,lvs(shape)),0,-1)
		array_set_here(arr,array_here(arr)+cast_size[lvs(shape)])
		return r
	}
	if(lil(shape)){
		n=max(0,ln(l_last(shape)))
		t=lvs(ls(l_first(shape)))
		r=array_get(array_slice_here(arr,t),0,n)
		array_set_here(arr,array_here(arr)+n*cast_size[t])
		return r
	}
	if(!lid(shape))return NONE
	bit=0;r=lmd();for(z=0;z<count(shape);z++){
		type=dic_getv(shape,z);v=NONE
		if(!lin(type)&&bit){bit=0;array_set_here(arr,array_here(arr)+1)}
		if(lin(type)){
			n=min(max(1,ln(type)),31);t=0;cast=array_cast(arr);array_set_cast(arr,"u8")
			while(n>0){
				t=(2*t)+bitwise_and(1,int(array_get_raw(arr,array_here(arr))/(2^(7-bit))))
				n--;bit++;if(bit==8){bit=0;array_set_here(arr,array_here(arr)+1)}
			}
			v=lmn(t);array_set_cast(arr,cast)
		}
		else{v=struct_read(arr,type)}
		dset(r,dic_getk(shape,z),v)
	}return r
}
function struct_write(arr,shape,value,  cast,n,z,zz,bit,type,v,t,pos,dst){
	if(lis(shape)||lil(shape)){
		n=lis(shape)?1:max(0,ln(l_last(shape)))
		t=lis(shape)?lvs(shape): lvs(ls(l_first(shape)))
		array_set(array_slice_here(arr,t),0,n,value)
		array_set_here(arr,array_here(arr)+n*cast_size[t])
		return
	}
	if(!lid(shape))return;
	bit=0;for(z=0;z<count(shape);z++){
		type=dic_getv(shape,z);v=dgetv(value,dic_getk(shape,z))
		if(!lin(type)){
			if(bit){bit=0;array_set_here(arr,array_here(arr)+1)}
			struct_write(arr,type,v);continue
		}
		n=min(max(1,ln(type)),31);cast=array_cast(arr);array_set_cast(arr,"u8")
		t=bitwise_and(ln(v),(2^n)-1);for(zz=0;zz<n;zz++){
			pos=bit_mask[bit]
			dst=bitwise_and(array_get_raw(arr,array_here(arr)),(256-pos))
			array_set_raw(arr,array_here(arr),bitwise_and(t,(2^(n-1-zz)))?bitwise_or(dst,pos):dst)
			bit++;if(bit==8){bit=0;array_set_here(arr,array_here(arr)+1)}
		};array_set_cast(arr,cast)
	}
}
function n_array_struct(arr,args,  shape,value,size,r){
	shape=l_first(args);value=count(args)>1?lst_get(args,1):-1;size=struct_size(shape)
	if(value&&array_here(arr)+size>=array_size(arr)){array_resize(arr,array_here(arr)+size)}
	if(value!=-1){struct_write(arr,shape,value);return value}
	else{return struct_read(arr,shape)}
}
function n_array_cat(arr,args,  z,v,s){
	for(z=0;z<count(args);z++){
		v=lst_get(args,z)
		if     (lin(v)     ){s=lms(array_cast(arr))}
		else if(lil(v)     ){s=lml2(lms(array_cast(arr)),lmn(count(v)))}
		else if(array_is(v)){s=lml2(lms(array_cast(v)),lmn(array_size(v)))}
		else{v=ls(v);        s=lml2(lms("char"),lmn(count(v)))}
		n_array_struct(arr,lml2(s,v))
	}return arr
}
function array_write(arr,  data,c,z){
	if(!array_is(arr))return lms("");
	c=array_size(arr);for(z=0;z<c;z++)data[z]=array_get_byte(arr,z)
	tmp=lms(data_write("DAT" cast_encode[array_cast(arr)],data,c))
	return tmp
}
function array_read(str,  c,arr,f,r,z){
	c=data_read("DAT",str,arr);f=cast_decode[data_fmt(str)];r=array_make(f,0)
	for(z=0;z<c;z++)array_append_byte(r,arr[z]);return r
}
function n_array(args,  size,cast){
	if(lis(l_first(args)))return array_read(lvs(ls(l_first(args))))
	size=int(ln(l_first(args)))
	cast=count(args)<2?"u8": lvs(ls(lst_get(args,1)));if(!cast_size[cast])cast="u8"
	return array_make(cast,size)
}
function n_array_get(arr,i,  offset,len){
	offset=int(ln(lil(i)?l_first(i):i));len=lil(i)?int(max(0,ln(l_last(i)))):-1
	return array_get(arr,offset,len)
}
function n_array_set(arr,i,x,  offset,len){
	offset=int(ln(lil(i)?l_first(i):i));len=lil(i)?int(max(0,ln(l_last(i)))):-1
	array_set(arr,offset,len,x);return x
}
function readbytes(filename,  c,arr,r,z){
	c=readbytesraw(filename,arr)
	r=array_make("u8",0);for(z=0;z<c;z++)array_append_byte(r,arr[z]);return r
}
function writebytes(filename,array,  c,arr,z){
	c=array_size(array);for(z=0;z<c;z++)arr[z]=array_get_byte(array,z)
	writebytesraw(filename,arr,c);return 1
}
function array_print(arr,  c,r,z){
	c=array_size(arr);r=""
	for(z=0;z<c;z++)r=r sprintf("%c",array_get_byte(arr,z))
	return r
}

###########################################################
#
#  The Image Interface
#
###########################################################

# store images as a packed string of hex bytes;
# this has the same random access characteristics as the
# internal representation for lists, and minimizes storage overhead.
# still inherently slower than a true mutable 2d array for writes,
# so wherever possible we construct an entire replacement byte-string in a linear pass.
function image_is(x){return x!=-1&&lii(x)&&li_name(x)=="image"}
function image_w(img){return heap_c[img]}
function image_h(img){return heap_n[img]}
function image_make(x,y,  r){r=lmi("image");x=int(x);y=int(y);heap_c[r]=x;heap_n[r]=y;heap_v[r]=repeat_char("00",x*y);return r}
function image_clone(img,  r){r=lmi("image");heap_c[r]=heap_c[img];heap_n[r]=heap_n[img];heap_v[r]=heap_v[img];return r}
function image_empty(   r){r=lmi("image");heap_c[r]=0;heap_n[r]=0;heap_v[r]="";return r}
function image_get(img,z){return h2b[substr(heap_v[img],1+2*z,2)]}
function image_get_px(img,x,y){return h2b[substr(heap_v[img],1+2*(x+(y*image_w(img))),2)]}
function image_set_px(img,x,y,v,  t,i){t=heap_v[img];i=1+2*(x+(y*image_w(img)));heap_v[img]=substr(t,1,i-1) b2h[mod(v,256)] substr(t,i+2)}

function n_image_map(img,args,  z,m,f,r,k){
	if(count(args)>=2){f=mod(int(ln(lst_get(args,1))),256);for(z=0;z<256;z++)m[z]=b2h[f]}else{for(z=0;z<256;z++)m[z]=b2h[z];}
	f=ld(l_first(args));for(z=0;z<count(f);z++){k=int(ln(dic_getk(f,z)));if(k>=-128&&k<=255)m[mod(k,256)]=b2h[mod(int(ln(dic_getv(f,z))),256)]}
	r="";sx=image_w(img);sy=image_h(img);for(y=0;y<sy;y++)for(x=0;x<sx;x++)r=r m[image_get_px(img,x,y)];heap_v[img]=r
	return img
}
function n_image_merge(img,args,  src,op,y,x,r,sx,sy,bx,by,z,v){
	sx=image_w(img);sy=image_h(img);
	if(lis(l_first(args))){
		if(count(args)>=2&&image_is(lst_get(args,1))){
			src=lst_get(args,1);op=lvs(l_first(ls(l_first(args))))
			bx=image_w(src);by=image_h(src);if(bx==0||by==0)return img
			if(op=="+"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[mod(image_get_px(img,x,y)+image_get_px(src,x%bx,y%by),256)]};heap_v[img]=r}
			if(op=="-"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[mod(image_get_px(img,x,y)-image_get_px(src,x%bx,y%by),256)]};heap_v[img]=r}
			if(op=="*"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[mod(image_get_px(img,x,y)*image_get_px(src,x%bx,y%by),256)]};heap_v[img]=r}
			if(op=="&"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[min(image_get_px(img,x,y), image_get_px(src,x%bx,y%by))    ]};heap_v[img]=r}
			if(op=="|"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[max(image_get_px(img,x,y), image_get_px(src,x%bx,y%by))    ]};heap_v[img]=r}
			if(op==">"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[    image_get_px(img,x,y)> image_get_px(src,x%bx,y%by)     ]};heap_v[img]=r}
			if(op=="<"){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[    image_get_px(img,x,y)< image_get_px(src,x%bx,y%by)     ]};heap_v[img]=r}
			if(op=="="){r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){r=r b2h[    image_get_px(img,x,y)==image_get_px(src,x%bx,y%by)     ]};heap_v[img]=r}
		}
		return img
	}
	if(lil(l_first(args)))args=l_first(args)
	for(z=0;z<256;z++){v[z]=z>=count(args)?-1:lst_get(args,z);if(!image_is(v[z])||image_w(v[z])<1||image_h(v[z])<1)v[z]=-1}
	r="";for(y=0;y<sy;y++)for(x=0;x<sx;x++){src=v[image_get_px(img,x,y)];r=r b2h[src==-1?0:image_get_px(src,x%image_w(src),y%image_h(src))]}
	heap_v[img]=r;return img
}
function image_flip_h(img,  sx,sy,x,y,r){
	sx=image_w(img);sy=image_h(img);r=""
	for(y=0;y<sy;y++)for(x=sx-1;x>=0;x--)r=r b2h[image_get_px(img,x,y)]
	heap_v[img]=r
}
function image_flip_v(img,  sx,sy,x,y,r){
	sx=image_w(img);sy=image_h(img);r=""
	for(y=sy-1;y>=0;y--)for(x=0;x<sx;x++)r=r b2h[image_get_px(img,x,y)]
	heap_v[img]=r
}
function image_transpose(img,  sx,sy,x,y,r){
	sx=image_w(img);sy=image_h(img);r=""
	for(y=0;y<sx;y++)for(x=0;x<sy;x++)r=r b2h[image_get_px(img,y,x)]
	heap_v[img]=r;heap_c[img]=sy;heap_n[img]=sx
}
function image_shear_x(img,shear,  sx,sy,x,y,r,o){
	sx=image_w(img);sy=image_h(img);r=""
	for(y=0;y<sy;y++){o=int((y-(sy/2))*shear);for(x=0;x<sx;x++)r=r b2h[image_get_px(img,mod(x+o,sx),y)]}
	heap_v[img]=r
}
function image_shear_y(img,shear){
	sx=image_w(img);sy=image_h(img);r=""
	for(y=0;y<sy;y++)for(x=0;x<sx;x++){o=int((x-(sx/2))*shear);r=r b2h[image_get_px(img,x,mod(y+o,sy))]}
	heap_v[img]=r
}
function image_dither(img,  sx,sy,stride,m,e,ei,z,r,pix,col,err,x){
	sx=image_w(img);sy=image_h(img);stride=2*sx;m[0]=0;m[1]=1;m[2]=sx-2;m[3]=sx-1;m[4]=sx;m[5]=stride-1;ei=0;r=""
	for(z=0;z<sx*sy;z++){
		pix=(image_get(img,z)/256)+e[ei];col=pix>.5?1:0;err=(pix-col)/8;r=r b2h[!col]
		e[ei]=0;ei=(ei+1)%stride;for(x=0;x<6;x++)e[(ei+m[x])%stride]+=err
	};heap_v[img]=r
}
function n_image_transform(img,args){
	args=lvs(ls(l_first(args)))
	if     (args=="horiz" )image_flip_h(img)
	else if(args=="vert"  )image_flip_v(img)
	else if(args=="flip"  )image_transpose(img)
	else if(args=="left"  ){image_flip_h(img);image_transpose(img)}
	else if(args=="right" ){image_transpose(img);image_flip_h(img)}
	else if(args=="dither")image_dither(img)
	return img
}
function n_image_rotate(img,args,  n){
	n=-mod(ln(l_first(args)),2*MATH_PI)
	if(abs(n)>(MATH_PI/2)&&abs(n)<(MATH_PI*3/2)){image_flip_v(img);image_flip_h(img);n+=(n<0?1:-1)*MATH_PI}
	image_shear_x(img,-tan(n/2));image_shear_y(img,sin(n));image_shear_x(img,-tan(n/2))
	return img
}
function n_image_translate(img,args,  sx,sy,t,px,py,r,y,x,ix,iy){
	sx=image_w(img);sy=image_h(img);r=""
	t=ll(l_first(args));px=count(t)>=1?int(ln(lst_get(t,0))):0;py=count(t)>=2?int(ln(lst_get(t,1))):0
	if(px==0&&py==0){r=heap_v[img]}
	if(count(args)>=2&&lb(lst_get(args,1))){for(y=0;y<sy;y++)for(x=0;x<sx;x++)r=r b2h[image_get_px(img,mod(x-px,sx),mod(y-py,sy))]}
	else{for(y=0;y<sy;y++)for(x=0;x<sx;x++){ix=x-px;iy=y-py;r=r b2h[(ix<0||ix>=sx||iy<0||iy>=sy)?0:image_get_px(img,ix,iy)]}}
	heap_v[img]=r;return img
}
function image_paste(img,src,opaque,rx,ry,  sx,sy,dx,dy){
	sx=image_w(src);sy=image_h(src);dx=image_w(img);dy=image_h(img)
	for(y=0;y<sy;y++)for(x=0;x<sx;x++){
		if(rx+x<0||ry+y<0||rx+x>=dx||ry+y>=dy)continue
		if(opaque||image_get_px(src,x,y))image_set_px(img,rx+x,ry+y,image_get_px(src,x,y))
	}
}
function image_paste_scaled(img,src,opaque,rx,ry,rw,rh,  sx,sy,dx,dy,a,b,c,tx,ty){
	if(rw==0||rh==0)return;sx=image_w(src);sy=image_h(src);dx=image_w(img);dy=image_h(img);sx=rw/sx;sy=rh/sy;
	if(rw==sx&&rh==sy){image_paste(dst,src,opaque,rx,ry);return}
	for(a=0;a<rh;a++)for(b=0;b<rw;b++){
		c=image_get_px(src,int(b/sx),int(a/sy));if(!opaque&&c==0)continue
		tx=rx+b;ty=ry+a;if(tx>=0&&ty>=0&&tx<dx&&ty<dy)image_set_px(img,tx,ty,c)
	}
}
function n_image_scale(img,args,  r,af,rx,ry,src){
	af=count(args)>1&&lb(lst_get(args,1));args=l_first(args)
	if(lin(args)){rx=ln(args);ry=rx}else{args=ll(args);rx=count(args)>0?ln(lst_get(args,0)):0;ry=count(args)>1?ln(lst_get(args,1)):0}
	rx=int(max(0,rx*(af?1:image_w(img))));ry=int(max(0,ry*(af?1:image_h(img))));if(rx==0||ry==0){rx=0;ry=0;}
	src=image_clone(img);heap_c[img]=rx;heap_n[img]=ry;image_paste_scaled(img,src,1,0,0,rx,ry);return img
}
function n_image_paste(img,args,  src,pos,opaque,rx,ry,rw,rh){
	src=l_first(args);if(!image_is(src))src=image_empty();if(src==img)src=image_clone(img)
	pos=count(args)>=2?ll(lst_get(args,1)):lml();opaque=count(args)>=3?lb(lst_get(args,2)):1
	rx=count(pos)>0?int(ln(lst_get(pos,0))):0;ry=count(pos)>1?int(ln(lst_get(pos,1))):0
	if(count(pos)<=2){image_paste(img,src,opaque,rx,ry)}
	else{rw=count(pos)>2?int(ln(lst_get(pos,2))):0;rh=count(pos)>3?int(ln(lst_get(pos,3))):0;image_paste_scaled(img,src,opaque,rx,ry,rw,rh)}
	return img
}
function n_image_copy(img,args,  r,v,t,ax,ay,bx,by,an,c,y,x,rx,ry,w,h){
	if(!count(args))return image_clone(img)
	v[0]=0;v[1]=0;v[2]=image_w(img);v[3]=image_h(img)
	if(count(args)>=1){
		t=count(args)>0?ll(lst_get(args,0)):lml();ax=count(t)>=1?ln(lst_get(t,0)):0;ay=count(t)>=2?ln(lst_get(t,1)):0
		t=count(args)>1?ll(lst_get(args,1)):lml();bx=count(t)>=1?ln(lst_get(t,0)):0;by=count(t)>=2?ln(lst_get(t,1)):0
		if(bx<0){ax+=1+bx;bx*=-1};if(by<0){ay+=1+by;by*=-1};v[0]=ax;v[1]=ay;v[2]=bx;v[3]=by
	}
	if(count(args)>=2){
		an=lvs(ls(lst_get(args,2)))
		if(an~/center$/)v[0]-=int(v[1]/2) # x center
		if(an~/right$/ )v[0]-=    v[1]    # x right
		if(an~/^center/)v[1]-=int(v[3]/2) # y center
		if(an~/^bottom/)v[1]-=    v[3]    # y bottom
	}
	w=image_w(img);h=image_h(img);r=""
	for(y=0;y<v[3];y++)for(x=0;x<v[2];x++){rx=v[0]+x;ry=v[1]+y;r=r b2h[(rx<0||ry<0||rx>=w||ry>=h)?0: image_get_px(img,rx,ry)]}
	c=image_make(v[2],v[3]);heap_v[c]=r;return c
}
function image_write(img,  sx,sy,z,colors,t,tc,l,lc,c,p,stride,a,b,v,i){
	img=image_is(img)?img:image_empty();sx=image_w(img);sy=image_h(img);
	colors=0;for(z=0;z<sx*sy;z++)if(image_get(img,z)>1){colors=1;break}
	tc=0;t[tc++]=int(sx/256);t[tc++]=int(sx%256);t[tc++]=int(sy/256);t[tc++]=int(sy%256)
	lc=0;for(z=0;z<4;z++)l[lc++]=t[z]
	for(z=0;z<sx*sy;){c=0;p=image_get(img,z);while(c<255&&z<sx*sy&&image_get(img,z)==p){c++;z++};l[lc++]=p;l[lc++]=c}
	if(!colors&&lc>4+int(sx*sy/8)){
		stride=int(8*ceil(sx/8))
		for(a=0;a<sy;a++)for(b=0;b<stride;b+=8){v=0;for(i=0;i<8;i++)v=(2*v)+(b+i>=sx?0: image_get_px(img,b+i,a)?1:0);t[tc++]=v}
		return lms(data_write("IMG0",t,tc))
	}
	if(lc>4+sx*sy){for(z=0;z<sx*sy;z++)t[tc++]=image_get(img,z);return lms(data_write("IMG1",t,tc))}
	return lms(data_write("IMG2",l,lc))
}
BEGIN{for(z=0;z<8;z++)bit_mask[z]=2^(7-z)}
function image_read(str,  c,arr,w,h,r,v,i,o,p,n,s){
	c=data_read("IMG",str,arr);f=data_fmt(str);if(!c||c<4)return image_empty()
	w=(256*arr[0])+arr[1];h=(256*arr[2])+arr[3];r="";i=4
	if(f=="0"&&c-4>=w*h/8){s=ceil(w/8);for(a=0;a<h;a++)for(b=0;b<w;b++){r=r b2h[bitwise_and(arr[4+int(b/8)+a*s],bit_mask[b%8])?1:0]}}
	if(f=="1"&&c-4>=w*h){for(i=4;i<c;i++)r=r b2h[arr[i]]}
	if(f=="2"){while(i+2<=c){p=arr[i++];n=arr[i++];while(n){n--;r=r b2h[p]}}}
	v=image_make(w,h);heap_v[v]=r;return v
}
function image_hist(img,  r,c,sx,sy,x,y,z){
	sx=image_w(img);sy=image_h(img);r=lmd()
	for(y=0;y<sy;y++)for(x=0;x<sx;x++)c[image_get_px(img,x,y)]++
	for(z=0;z<256;z++)if(c[z]!=0)dset(r,lmn(z),lmn(c[z]));return r
}
function image_bounds(img,  r,sx,sy,dx,dy,dw,dh,x,y){
	sx=image_w(img);sy=image_h(img);r=lmd();dx=sx;dy=sy;dw=0;dh=0
	for(y=0;y<sy;y++)for(x=0;x<sx;x++)if(image_get_px(img,x,y)!=0){dx=min(dx,x);dy=min(dy,y);dw=max(dw,x);dh=max(dh,y)}
	dset(r,lms("pos" ),lml2(lmn(dx             ),lmn(dy             )))
	dset(r,lms("size"),lml2(lmn(min(sx,dw-dx+1)),lmn(min(sy,dh-dy+1))))
	return r
}
function n_image_set_px(img,i,x,  w,h,t,px,py){
	w=image_w(img);h=image_h(img)
	t=ll(i);px=int(count(t)>=1?ln(lst_get(t,0)):0);py=int(count(t)>=2?ln(lst_get(t,1)):0)
	if(px>=0&&py>=0&&px<w&&py<h)image_set_px(img,px,py,int(ln(x)));return x
}
function n_image_get_px(img,i){
	w=image_w(img);h=image_h(img)
	t=ll(i);px=int(count(t)>=1?ln(lst_get(t,0)):0);py=int(count(t)>=2?ln(lst_get(t,1)):0)
	return (px>=0&&py>=0&&px<w&&py<h)?lmn(image_get_px(img,px,py)): NONE
}
function n_image_get_pixels(img,  r,t,x,y){
	r=lml()
	for(y=0;y<image_h(img);y++){t=lml();lst_add(r,t);for(x=0;x<image_w(img);x++)lst_add(t,lmn(image_get(img,x+y*image_w(img))))}
	return r
}
function n_image_set_pixels(img,x,  t,c,r,z){
	t=l_raze(ll(x));c=image_w(img)*image_h(img)
	r="";for(z=0;z<c;z++){r=r b2h[z<count(t)?ln(lst_get(t,z)): image_get(img,z)]};heap_v[img]=r;return x
}
function n_image_resize(img,x,  w,h,t,sx,sy,r,a,b){
	t=ll(x);sx=count(t)>=1?max(0,ln(lst_get(t,0))):0;sy=count(t)>=2?max(0,ln(lst_get(t,1))):0
	w=image_w(img);h=image_h(img);if(w==sx&&h==sy)return img
	r="";for(a=0;a<sy;a++)for(b=0;b<sx;b++)r=r b2h[a>=h||b>=w?0: image_get_px(img,b,a)]
	heap_v[img]=r;heap_c[img]=sx;heap_n[img]=sy;return img
}
function n_image(args,  t,x,y){
	if(lis(l_first(args)))return image_read(lvs(l_first(args)))
	t=ll(l_first(args));x=count(t)>=1?ln(lst_get(t,0)):0;y=count(t)>=2?ln(lst_get(t,1)):0
	return image_make(x,y)
}

###########################################################
#
#  The RText Interface
#
###########################################################

BEGIN{RTEXT_END=2147483647}
function rtext_append(table,text,font,arg,  t,f,a,u){
	if(image_is(arg)){if(count(text)>1)text=lms("i");if(count(text)<1)return 0};if(!count(text))return 0
	t=tab_get(table,lms("text"))
	f=tab_get(table,lms("font"))
	a=tab_get(table,lms("arg" ))
	if(count(t)&&matchr(font,l_last(f))&&!image_is(arg)&&matchr(arg,l_last(a))){lst_set(t,count(t)-1,lms(lvs(l_last(t)) lvs(text)))}
	else{lst_add(t,text);lst_add(f,font);lst_add(a,arg)}
	torect(table);return count(text)
}
function rtext_appendr(table,suffix,  t,f,a,z){
	t=tab_get(suffix,lms("text"))
	f=tab_get(suffix,lms("font"))
	a=tab_get(suffix,lms("arg" ))
	for(z=0;z<count(t);z++)rtext_append(table,lst_get(t,z),lst_get(f,z),lst_get(a,z))
}
function rtext_cast(x,  t,f,a,tv,fv,av,r,v,z,i){
	if(x==-1)x=lms("")
	if(image_is(x))return n_rtext_make(lml3(lms(""),lms(""),x))
	if(lid(x))x=l_table(x);if(!lit(x))return n_rtext_make(l_list(ls(x)))
	t=lms("text");tv=tab_get(x,t)
	f=lms("font");fv=tab_get(x,f)
	a=lms("arg" );av=tab_get(x,a)
	if(count(x)==3&&tv!=-1&&fv!=-1&&av!=-1){ # already well-formed?
		v=1;for(z=0;z<count(tv);z++){
			if(!lis(lst_get(tv,z))||!lis(lst_get(fv,z))||!(lis(lst_get(av,z))&&!image_is(lst_get(av,z)) ))v=0;break
		};if(v)return x
	};r=lmt()
	v=tab_get(x,t);tab_set(r,t,v!=-1?v:l_list(lms("")))
	v=tab_get(x,f);tab_set(r,f,v!=-1?v:l_list(lms("")))
	v=tab_get(x,a);tab_set(r,a,v!=-1?v:l_list(lms("")))
	torect(r);tv=tab_get(r,t);fv=tab_get(r,f);av=tab_get(r,a)
	for(z=0;z<rowcount(r);z++){
		i=image_is(lst_get(av,z))
		lst_set(tv,z,i?lms("i")     :ls(lst_get(tv,z)))
		lst_set(fv,z,                ls(lst_get(fv,z)))
		lst_set(av,z,i?lst_get(av,z):ls(lst_get(av,z)))
	};return torect(r)
}
function rtext_string(table,x,y,  r,z,zz,i,a,b,t,s){
	r="";i=0;a=min(x,y);b=max(x,y);t=tab_get(table,lms("text"))
	for(z=0;z<count(t);z++){s=lvs(lst_get(t,z));for(zz=1;zz<=length(s);zz++){if(i>=a&&i<b)r=r substr(s,zz,1);i++}}
	return lms(r)
}
function rtext_span(table,x,y,  r,i,c,a,b,t,f,g,rr){
	r=l_take(NONE,table);i=0;c=0;a=min(x,y);b=max(x,y)
	t=tab_get(table,lms("text"))
	f=tab_get(table,lms("font"))
	g=tab_get(table,lms("arg" ))
	while(c<count(t)&&(i+count(lst_get(t,c)))<a)i+=count(lst_get(t,c++))
	if(c<count(t)&&i<=a){
		rr="";for(z=0;z<count(lst_get(t,c));z++){if(i>=a&&i<b)rr=rr substr(lvs(lst_get(t,c)),z+1,1);i++}
		rtext_append(r,lms(rr),lst_get(f,c),lst_get(g,c));c++
	}
	while(c<count(t)&&(i+count(lst_get(t,c)))<b){i+=rtext_append(r,lst_get(t,c),lst_get(f,c),lst_get(g,c));c++}
	if(c<count(t)&&i<b){
		rr="";for(z=0;z<count(lst_get(t,c));z++){if(i>=a&&i<b)rr=rr substr(lvs(lst_get(t,c)),z+1,1);i++}
		rtext_append(r,lms(rr),lst_get(f,c),lst_get(g,c))
	}return r
}

function n_rtext_make(args,  r,a,f,t){
	r=lmt()
	a=count(args)<3?lms(""): lst_get(args,2);a=image_is(a)?a:ls(a)
	f=count(args)<2?lms(""): ls(lst_get(args,1))
	t=image_is(a)?lms("i"): count(args)<1?lms(""): ls(lst_get(args,0))
	tab_set(r,lms("text"),l_list(t))
	tab_set(r,lms("font"),l_list(f))
	tab_set(r,lms("arg" ),l_list(a))
	return torect(r)
}
function n_rtext_len(args,  t,r,z){
	t=tab_get(rtext_cast(l_first(args)),lms("text"))
	r=0;for(z=0;z<count(t);z++)r+=count(lst_get(t,z));return lmn(r)
}
function n_rtext_get(args,  t,x,i,z){
	t=tab_get(rtext_cast(l_first(args)),lms("text"))
	x=count(args)<2?0:ln(lst_get(args,1))
	i=0;for(z=0;z<count(t);z++){i+=count(lst_get(t,z));if(i>=x)return lmn(z)};return lmn(-1)
}
function n_rtext_string(args,  t,x,y){
	if(count(args)<2){x=0;y=RTEXT_END}else{t=ll(lst_get(args,1));x=count(t)>=1?ln(lst_get(t,0)):0;y=count(t)>=2?ln(lst_get(t,1)):0}
	return rtext_string(rtext_cast(l_first(args)),x,y)
}
function n_rtext_span(args,  t,x,y){
	if(count(args)<2){x=0;y=RTEXT_END}else{t=ll(lst_get(args,1));x=count(t)>=1?ln(lst_get(t,0)):0;y=count(t)>=2?ln(lst_get(t,1)):0}
	return rtext_span(rtext_cast(l_first(args)),x,y)
}
function n_rtext_split(args,  r,d,v,t,n,z){
	r=lml();if(count(args)<2)return r
	d=ls(lst_get(args,0));if(count(d)<1)return r
	v=rtext_cast(lst_get(args,1))
	text=rtext_string(v,0,RTEXT_END)
	n=0;for(z=0;z<count(text);z++){
		if(substr(lvs(text),z+1,count(d))!=lvs(d))continue
		lst_add(r,rtext_span(v,n,z));z+=count(d)-1;n=z+1
	}if(n<=count(text))lst_add(r,rtext_span(v,n,count(text)));return r
}
function n_rtext_replace(args,  r,t,k,v,text,z,cx,cy,ki,any,key,val,f,i){
	if(count(args)<3)return l_first(args);r=lml()
	t=rtext_cast(l_first(args))
	k=lst_get(args,1);if(!lil(k))k=l_list(k)
	v=lst_get(args,2);if(!lil(v))v=l_list(v)
	text=rtext_string(t,0,RTEXT_END)
	nocase=count(args)>=4?lb(lst_get(args,3)):0
	k=l_take(lmn(max(count(k),count(v))),l_drop(lms(""),k));for(z=0;z<count(k);z++)lst_set(k,z,ls(        lst_get(k,z)))
	v=l_take(lmn(max(count(k),count(v))),v)                ;for(z=0;z<count(v);z++)lst_set(v,z,rtext_cast(lst_get(v,z)))
	cx=0;cy=0;while(cy<count(text)){
		any=0;for(ki=0;ki<count(k);ki++){
			key=lst_get(k,ki)
			val=lst_get(v,ki)
			f=nocase?tolower(lvs(key))==tolower(substr(lvs(text),cy+1,count(key))): lvs(key)==substr(lvs(text),cy+1,count(key))
			if(f){if(cx!=cy)lst_add(r,rtext_span(t,cx,cy));lst_add(r,val);cx=cy=(cy+count(key));any=1}
		}if(!any)cy++
	}if(cx<count(text))lst_add(r,rtext_span(t,cx,RTEXT_END));return n_rtext_cat(r)
}
function n_rtext_find(args,  r,nocase,text,ki,k,z,x,any,key,f){
	r=lml();if(count(args)<2)return r
	nocase=count(args)>=3&&lb(lst_get(args,2))
	text=lvs(lit(l_first(args))?ls(rtext_string(rtext_cast(l_first(args)),0,RTEXT_END)): ls(l_first(args)))
	ki=lst_get(args,1);if(!lil(ki))ki=l_list(ki);k=lml();for(z=0;z<count(ki);z++)lst_add(k,ls(lst_get(ki,z)))
	for(x=0;x<length(text);){
		any=0;for(ki=0;ki<count(k);ki++){
			key=lst_get(k,ki);f=1
			f=nocase?tolower(lvs(key))==tolower(substr(text,x+1,count(key))): lvs(key)==substr(text,x+1,count(key))
			if(f){lst_add(r,lml2(lmn(x),lmn(x+count(key))));x+=max(1,count(key));any=1;break}
		};if(!any)x++
	};return r
}
function n_rtext_index(args,  r,t,gx,gy){
	if(count(args)<1)return NONE;r=0
	t=lvs(rtext_string(rtext_cast(l_first(args)),0,RTEXT_END))
	gx=count(args)>1?ln(l_first(lst_get(args,1))):0
	gy=count(args)>1?ln(l_last(lst_get(args,1))):0
	while(r<length(t)&&gx>0){if(substr(t,r+1,1)=="\n"){gx--};r++}
	while(r<length(t)&&gy>0&&(substr(t,r+1,1)!="\n")){gy--;r++}
	return lmn(r)
}
function n_rtext_cat(args,  r,z){
	r=l_take(NONE,rtext_cast(lmt()))
	for(z=0;z<count(args);z++)rtext_appendr(r,rtext_cast(lst_get(args,z)))
	return r
}

###########################################################
#
#  Tokenizer
#
###########################################################

function tokenize(str){
	perr=0;parse_r=0;parse_c=0;parse_str=str;parse_pc=""
	tok_next=0
}
BEGIN{
	#        !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
	split(" s\" sss ()ssssdsdddddddddd: sssn@nnnnnnnnnnnnnnnnnnnnnnnnnn[ ]sn nnnnnnnnnnnnnnnnnnnnnnnnnn s s",tcc,"")
	esc["\\"]="\\";esc["\""]="\"";esc["n"]="\n"
}
function iw(){return parse_str~/^[ \t\n#]/}
function id(){return parse_str~/^[0-9]/}
function nc(  r){
	r=substr(parse_str,1,1);parse_pc=r;parse_str=substr(parse_str,2);
	if(r=="\n"){parse_r++;parse_c=0}else{parse_c++};
	return r
}
function ne(  r){r=nc();return r in esc?esc[r]: perr="Invalid escape character '\\"e"' in string."}
function nw(x,  v){v=+x;     while(id())v=(v*10)+(+nc());    return v}
function nf(   v,p){v=0;p=10;while(id()){v+=(+nc())/p;p*=10;}return v}
function token_make(type,val,row,col){return type","row","col":"val}
function token_end(){return token_make("e","",parse_r,parse_c)}
function token_type(tok){return substr(tok,1,1)}
function token_val(tok){return substr(tok,index(tok,":")+1)}
function token_row(tok,  arr){split(tok,arr,",");return 0+arr[2]}
function token_col(tok,  arr){split(tok,arr,",");return 0+arr[3]}

function num(x,sign,  v){
	if(x=="."&&!id())return token_make(".","",parse_r,parse_c)
	v=x;while(id())v=v nc();if(x!="."&&parse_str~/^\./)v=v nc();while(id())v=v nc()
	return token_make("d",sign v,parse_r,parse_c)
}
function tok(  w,cc,x,v,tr,tc){
	if(perr)return token_end()
	w=parse_pc==""||iw()||parse_pc~/[[(%&*+,\/:<=>@|~^-]/
	while(iw()){if(parse_str~/^#/){while(length(parse_str)&&parse_str~/^[^\n]/){nc()}}else{nc()}}
	if(!length(parse_str))return token_end()
	x=nc();cc=tcc[ascii[x]-31];if(cc==" "||cc==""){perr="Invalid character '"x"'.";return token_end()}
	if(x=="\""){
		v="";tr=parse_r;tc=parse_c;
		while(perr==0&&length(parse_str)&&((x=nc())!="\"")){
			if(x=="\\"){x=nc();if(!esc[x]){perr="Invalid escape character '\\"x"' in string.";break};v=v esc[x]}else{v=v x}
		}
		return token_make("s",v,tr,tc)
	}
	if(x=="-"&&w&&parse_str~/^[0-9.]/)return num(nc(),"-")
	if(cc=="n"){tr=parse_r;tc=parse_c;v=x;while(parse_str~/^[a-zA-Z0-9_?]/){v=v nc()};return token_make("n",v,tr,tc)}
	if(cc=="s")return token_make("m",x,parse_r,parse_c)
	if(cc=="d")return num(x,"")
	return token_make(x,"",parse_r,parse_c)
}
function next_token(  r){if(tok_next){r=tok_next;tok_next=0;}else{r=tok()};return r}
function peek(){if(!tok_next)tok_next=tok();return tok_next}
function peek2(  r,ts,tr,tc,tp,tn){
	ts=parse_str;tr=parse_r;tc=parse_c;tp=parse_pc;tn=tok_next; next_token();r=peek()
	parse_str=ts;parse_r=tr;parse_c=tc;parse_pc=tp;tok_next=tn; return r
}
function peek_type(){return token_type(peek())}
function hasnext(){return perr==0&&((tok_next&&token_type(tok_next)!="e")||peek_type()!="e")}
function tmatchsp(x){if(perr==0&&peek_type()==x){next_token();return 1};return 0}
function tmatchp(x){return perr==0&&peek_type()=="n"&&token_val(peek())==x}
function tmatch(x){if(tmatchp(x)){next_token();return 1};return 0}
function tsname(x){return x=="e"?"the end of the script":x=="d"?"number":x=="n"?"name":x=="s"?"string":x=="m"?"symbol":x}
function expect(type,  p){
	p=peek_type();if(p==type)return token_val(next_token());if(perr)return""
	perr="Expected "tsname(type)", but found "tsname(p)".";return ""
}
BEGIN{
	split("while|each|send|on|if|elseif|else|end|do|with|local|select|extract|update|insert|into|from|where|by|orderby|asc|desc",keywords,"|")
	split("floor|cos|sin|tan|exp|ln|sqrt|count|first|last|sum|min|max|raze|prod|range|keys|list|rows|cols|table|typeof|flip|mag|unit|heading",mops,"|")
	split("split|fuse|dict|take|drop|in|join|cross|parse|format|unless|limit|like|window",dops,"|")
}
function is_monad(name){for(key in mops)if(name==mops[key])return 1;return index("-!",name)!=0}
function is_dyad(name){for(key in dops)if(name==dops[key])return 1;return index("+-*/%^<>=&|,~",name)!=0}
function ident(name){for(key in keywords)if(name==keywords[key])return 1;return is_monad(name)||is_dyad(name)}
function name(type,  r){
	r=expect("n");if(perr||type=="member")return r
	if(ident(r))perr="'"r"' is a keyword, and cannot be used for a "type" name.";return r
}
function names(end,type,  r){r=lml();while(!tmatch(end)&&perr==0)lst_add(r,lms(name(type)));return r}

###########################################################
#
#  Parser
#
###########################################################

function iblock(b,  c){
	c=0;while(hasnext()){
		if(tmatch("end")){if(!c)blk_lit(b,NONE);return}
		if(c)blk_op(b,"DROP");expr(b);c++
	};if(perr==0)perr="Expected 'end' for block."
}
function quote(   r){r=lmblk();expr(r);blk_end(r);return r}
function block(   r){r=lmblk();iblock(r);return r}

function parseclause(b,isupdate,  ex,grouped,n,l,dir){
	if(tmatch("where")){
		ex=quote();grouped=parseclause(b,isupdate)
		if(!grouped)                            {blk_lit(b,ex);blk_op(b,"COL");blk_op2(b,"@where")}
		else{n=tempname();l=lmblk();blk_get(l,n);blk_lit(l,ex);blk_op(l,"COL");blk_op2(l,"@where");blk_loop(b,l_list(lms(n)),l)}
		return grouped
	}
	if(tmatch("orderby")){
		ex=quote();dir=1;if(tmatch("asc")){dir=-1}else if(tmatch("desc")){dir=1}
		else if(perr==0)perr="Expected 'asc' or 'desc'.";grouped=parseclause(b,isupdate)
		if(!grouped)                             {blk_lit(b,ex);blk_op(b,"COL");blk_lit(b,lmn(dir));blk_op3(b,"@orderby")}
		else{n=tempname();l=lmblk();blk_get(l,n);blk_lit(l,ex);blk_op(l,"COL");blk_lit(l,lmn(dir));blk_op3(l,"@orderby");blk_loop(b,l_list(lms(n)),l)}
		return grouped
	}
	if(tmatch("by")){
		ex=quote();grouped=parseclause(b,isupdate);if(grouped)blk_op1(b,"raze")
		blk_lit(b,ex);blk_op(b,"COL");blk_op2(b,"@by");return 1
	}
	if(!tmatch("from")&&perr==0)perr="Expected 'from'."
	expr(b);blk_op1(b,"@tab");blk_op(b,"DUP");return 0
}
function parsequery(b,op,dcol,  cols,x,set,li,lp,nm,get,unique,grouped,idx,keys,n,l,z){
	cols=lmd();while(perr==0&&!tmatchp("from")&&!tmatchp("where")&&!tmatchp("by")&&!tmatchp("orderby")){
		set=token_type(peek2())==":";li=peek_type()=="s";lp=peek_type()=="n";x=lms("")
		nm=li?(set?lms(token_val(peek())):lms("")):lms(token_val(peek()))
		get=!ident(lvs(nm));unique=(li||lp)&&count(nm)&&dgeti(cols,nm)==-1
		if     (set&&li&&!unique ){next_token();next_token()}
		if     (set&&unique      ){x=nm;next_token();next_token()}
		else if(get&&unique&&dcol){x=nm}
		else if(dcol             ){x=lms("c"count(cols))}
		dset(cols,x,quote())
	}
	grouped=parseclause(b,op=="@upd");idx=lmblk()
	blk_get(idx,"index");if(!grouped)blk_op1(b,"list")
	keys=l_cat(l_keys(cols),lms("@index"));n=tempname();l=lmblk()
	blk_lit(l,keys);blk_get(l,n);for(z=0;z<count(cols);z++){blk_lit(l,dic_getv(cols,z));blk_op(l,"COL")}
	blk_lit(l,idx);blk_op(l,"COL");blk_op(l,"DROP");blk_opa(l,"BUND",count(keys));blk_op2(l,"dict")
	blk_loop(b,l_list(lms(n)),l);blk_lit(b,keys);blk_op3(b,op)
}
function parseinsert(b,  n,v,i){
	n=lml();while(perr==0&&!tmatch("with"))lst_add(n,lms(peek_type()=="s"?token_val(next_token()):name("column")));if(count(n)<1)lst_add(n,lms("value"))
	v=0;i=0;while(perr==0){if(tmatch("into")){i=1;break};if(tmatch("end")){i=0;break};expr(b);v++}
	blk_opa(b,"BUND",v);blk_lit(b,n);if(i){expr(b)}else{blk_lit(b,NONE)};blk_op3(b,"@ins")
}
function quotesub(  c,r){c=0;r=lmblk();while(hasnext()&&!tmatchsp("]")){expr(r);c++};blk_opa(r,"BUND",c);return r}
function quotedot(    r){r=lmblk();blk_lit(r,l_list(lms(name("member"))));return r}

function parseindex(b,nm,  i,ix,z,l,vn){
	ix=0;while(perr==0&&peek_type()~/^[\[.]/){
		if(tmatchsp("[")){i[ix++]=quotesub()}
		if(tmatchsp(".")){
			if(peek_type()~/^[\[.]/){
				vn=tempname()
				for(z=0;z<ix;z++){blk_cat(b,i[z]);blk_op(b,"CALL")}
				l=lmblk();blk_get(l,vn);parseindex(l);blk_loop(b,l_list(lms(vn)),l);return
			}else{i[ix++]=quotedot()}
		}
	}
	if(tmatchsp(":")){
		for(z=0;z<ix;z++)blk_cat(b,i[z]);blk_opa(b,"BUND",ix);blk_op(b,"OVER")
		for(z=0;z<ix-1;z++){blk_opa(b,"IPRE",z);blk_opa(b,"IPOST",z)}
		expr(b);blk_opa(b,"AMEND",nm?nm:"")
	}
	else{for(z=0;z<ix;z++){blk_cat(b,i[z]);blk_op(b,"CALL")}}
}
function parse_if(b,  fin,fi,c,e,nx,z){
	fi=0;c=0;e=0;expr(b);nx=blk_opa(b,"JUMPF",0);while(hasnext()){
		if(fi>=4096){perr="Too many elseif clauses.";return}
		if(tmatch("elseif")){
			if(e){perr="Expected 'end'.";return}
			if(!c)blk_lit(b,NONE);c=0;fin[fi++]=blk_opa(b,"JUMP",0);blk_set_at(b,nx,blk_here(b));expr(b);nx=blk_opa(b,"JUMPF",0);continue
		}
		if(tmatch("else")){
			if(e){perr="Expected 'end'.";return}
			if(!c)blk_lit(b,NONE);c=0;e=1;fin[fi++]=blk_opa(b,"JUMP",0);blk_set_at(b,nx,blk_here(b));nx=-1;continue
		}
		if(tmatch("end")){
			if(!c)blk_lit(b,NONE);c=0;if(!e)fin[fi++]=blk_opa(b,"JUMP",0);if(nx!=-1)blk_set_at(b,nx,blk_here(b));if(!e)blk_lit(b,NONE)
			for(z=0;z<fi;z++)blk_set_at(b,fin[z],blk_here(b));return
		}
		if(c)blk_op(b,"DROP");expr(b);c++
	}
}
function parse_while(b,  t,n){
	blk_lit(b,NONE);head=blk_here(b);expr(b);cond=blk_opa(b,"JUMPF",0)
	blk_op(b,"DROP");iblock(b);blk_opa(b,"JUMP",head);blk_set_at(b,cond,blk_here(b))
}
function parse_send(b){
	blk_lit(b,lmnat("uplevel","uplevel"));blk_lit(b,lms(name("function")));blk_op(b,"CALL");expect("[");blk_cat(b,quotesub());blk_op(b,"CALL")
}
function parse_atmonad(b,n,  l,li,depth,t,m,mi){
	depth=0;while(tmatchsp("@"))depth++
	expr(b);l=lmblk();li=lmblk();blk_get(li,"v");blk_op1(li,n);blk_loop(l,l_list(lms("v")),li)
	while(depth-->0){t=tempname();m=lmblk();mi=lmblk();blk_get(mi,t);blk_cat(mi,l);blk_loop(m,l_list(lms(t)),mi);l=m}
	blk_cat(b,l)
}
function parse_atdyad(b,  f,l,li,z,depth,t,m,mi){
	depth=0;while(tmatchsp("@"))depth++
	f=tempname();blk_set(b,f);blk_op(b,"DROP");expr(b)
	li=lmblk();blk_get(li,f);blk_get(li,"v");blk_opa(li,"BUND",1);blk_op(li,"CALL");
	l=lmblk();blk_loop(l,l_list(lms("v")),li);
	while(depth-->0){t=tempname();m=lmblk();mi=lmblk();blk_get(mi,t);blk_cat(mi,l);blk_loop(m,l_list(lms(t)),mi);l=m}
	blk_cat(b,l)
}
function term(b,  n,t,v){
	if(peek_type()=="d"){blk_lit(b,lmn(token_val(next_token())));return}
	if(peek_type()=="s"){blk_lit(b,lms(token_val(next_token())));return}
	if(tmatch("if")){parse_if(b);return}
	if(tmatch("while")){parse_while(b);return}
	if(tmatch("each")){t=names("in","variable");expr(b);blk_loop(b,t,block());return}
	if(tmatch("on")){
		n=name("function");v=tmatchsp(".")&&tmatchsp(".")&&tmatchsp(".");t=names("do","argument");
		if(!perr&&v&&count(t)!=1){perr="Variadic functions must take exactly one named argument.";return;}
		if(v&&count(t)==1)t=l_list(lms("..." lvs(l_first(t))))
		blk_lit(b,lmon(n,t,blk_end(block())));blk_op(b,"BIND");return
	}
	if(tmatch("send"   )){parse_send(b);return}
	if(tmatch("local"  )){n=name("variable");expect(":");expr(b);blk_loc(b,n);return}
	if(tmatch("select" )){parsequery(b,"@sel",1);return}
	if(tmatch("extract")){parsequery(b,"@ext",0);return}
	if(tmatch("update" )){parsequery(b,"@upd",1);return}
	if(tmatch("insert" )){parseinsert(b);return}
	if(tmatchsp("(")){if(tmatchsp(")")){blk_lit(b,lml());return};expr(b);expect(")");return}
	n=token_val(peek())
	if(peek_type()~/^[mn]/&&is_monad(n)){next_token();if(tmatchsp("@")){parse_atmonad(b,n)}else{expr(b);blk_op1(b,n)};return}
	n=name("variable")
	if(tmatchsp(":")){expr(b);blk_set(b,n);return}
	blk_get(b,n);parseindex(b,n)
}
function expr(b,  s){
	term(b);if(peek_type()~/^[\[.]/)parseindex(b)
	if(tmatchsp("@")){parse_atdyad(b);return}
	s=token_val(peek());if(peek_type()~/^[mn]/&&is_dyad(s)){next_token();expr(b);blk_op2(b,s)}
}
function parse(text,  r){
	tokenize(text);r=lmblk()
	if(hasnext())expr(r);while(hasnext()){blk_op(r,"DROP");expr(r)}
	if(blk_here(r)==0)blk_lit(r,NONE);return r
}

###########################################################
#
#  Root Environment
#
###########################################################

BEGIN{
	rootenv=lmenv()
	env_loc(rootenv,"pi"      ,lmn(MATH_PI))
	env_loc(rootenv,"e"       ,lmn(2.718281828459045))
	env_loc(rootenv,"sys"     ,lmi("system"))
	env_loc(rootenv,"app"     ,lmi("app"   ))
	env_loc(rootenv,"bits"    ,lmi("bits"  ))
	env_loc(rootenv,"rtext"   ,lmi("rtext" ))
	env_loc(rootenv,"show"    ,lmnat("show"    ,"show"    ))
	env_loc(rootenv,"print"   ,lmnat("print"   ,"print"   ))
	env_loc(rootenv,"error"   ,lmnat("error"   ,"error"   ))
	env_loc(rootenv,"input"   ,lmnat("input"   ,"input"   ))
	env_loc(rootenv,"exit"    ,lmnat("exit"    ,"exit"    ))
	env_loc(rootenv,"eval"    ,lmnat("eval"    ,"eval"    ))
	env_loc(rootenv,"import"  ,lmnat("import"  ,"import"  ))
	env_loc(rootenv,"dir"     ,lmnat("dir"     ,"dir"     ))
	env_loc(rootenv,"shell"   ,lmnat("shell"   ,"shell"   ))
	env_loc(rootenv,"read"    ,lmnat("read"    ,"read"    ))
	env_loc(rootenv,"write"   ,lmnat("write"   ,"write"   ))
	env_loc(rootenv,"readcsv" ,lmnat("readcsv" ,"readcsv" ))
	env_loc(rootenv,"writecsv",lmnat("writecsv","writecsv"))
	env_loc(rootenv,"readxml" ,lmnat("readxml" ,"readxml" ))
	env_loc(rootenv,"writexml",lmnat("writexml","writexml"))
	env_loc(rootenv,"array"   ,lmnat("array"   ,"array"   ))
	env_loc(rootenv,"image"   ,lmnat("image"   ,"image"   ))
	env_loc(rootenv,"random",lmnat("random","random"))
	split("white|yellow|orange|red|magenta|purple|blue|cyan|green|darkgreen|brown|tan|lightgray|mediumgray|darkgray|black",color_arr,"|")
	color_dic=lmd();for(key in color_arr)dset(color_dic,lms(color_arr[key]),lmn(key+31));env_loc(rootenv,"colors",color_dic)
	arg_list =lml();for(z=0;z<ARGC;z++)lst_add(arg_list,lms(ARGV[z]))                   ;env_loc(rootenv,"args"  ,arg_list )
	env_dic=lmd();env_loc(rootenv,"env",env_dic);for(k in ENVIRON)dset(env_dic,lms(k),lms(ENVIRON[k]));
}

###########################################################
#
#  Bytecode Interpreter
#
###########################################################

function l_at(x,y,  r){
	if(lii(x))return lis(y)&&lvs(y)=="type"?lms(li_name(x)): interfaces(x,y,-1)
	if(lit(x)&&lin(y))x=l_rows(x)
	if((lis(x)||lil(x))&&!lin(y))x=ld(x)
	if(lis(x)){y=int(ln(y));return lms(y<0||y>=count(x)?"": substr(lvs(x),1+y,1))}
	if(lil(x)){y=int(ln(y));return y<0||y>=count(x)?NONE: lst_get(x,y)}
	if(lid(x))return dgetv(x,y)
	if(lit(x)&&lis(y)){r=tab_get(x,y);return r==-1?NONE:r}
	return NONE
}
function amend(x,i,y,  r,rn,ri,z,zi,t){
	if(lii(x))return interfaces(x,i,y)
	if(lit(x)&&lin(i)){
		ri=int(ln(i));rn=lmn(rowcount(x));r=l_take(rn,x)
		if(!lid(y)){t=lmd();for(z=0;z<count(x);z++)dset(t,tab_getk(x,z),y);y=t}
		if(ri>=0&&ri<rowcount(x))for(z=0;z<count(y);z++){
			zi=dgeti(r,ls(dic_getk(y,z)))
			if(zi!=-1)tab_set(r,tab_getk(r,zi),amend(tab_getv(r,zi),lmn(ri),dic_getv(y,z)))
		}return r
	}
	if(lit(x)&&lis(i)){
		rn=lmn(rowcount(x));r=l_take(rn,x)
		t=lil(y)?l_take(lmn(min(count(y),rowcount(x))),ll(y)): l_take(rn,l_list(y))
		while(count(t)<rowcount(x))lst_add(t,NONE);dset(r,ls(i),t);return r
	}
	if(!lis(x)&&!lil(x)&&!lid(x))return amend(lml(),i,y)
	if((lil(x)||lis(x))&&(!lin(i)||(lvn(i)<0||lvn(i)>count(x))))return amend(ld(x),i,y)
	if(lil(x)){r=lst_clone(x);i=int(ln(i));if(i>=0&&i<count(r))lst_set(r,i,y);if(i==count(x))lst_add(r,y);return r}
	if(lid(x)){r=dic_clone(x);dset(r,i,y);return r}
	if(lis(x)){x=lvs(x);i=int(ln(i));return lms(substr(x,1,i) lvs(ls(y)) substr(x,i+2))}
	return lml()
}
BEGIN{amend_tla=0}
function amendv(x,i,y,n){
	if(lii(x))amend_tla=0
	if(!amend_tla&&n+1<count(i))return amendv(l_at(x,l_first(l_at(i,n))),i,y,n+1)
	if(n+1<count(i))return amend(x,l_first(lst_get(i,n)),amendv(l_at(x,l_first(lst_get(i,n))),i,y,n+1))
	if(n+1==count(i))return amend(x,l_first(lst_get(i,n)),y)
	return y
}

function state_init(env){
	delete state_p; state_pc=0;  # parameter stack, ptr
	delete state_e; state_ec=0;  # environment stack, ptr
	delete state_t; state_tc=0;  # task stack, ptr
	delete state_i; state_ic=0;  # task index stack, ptr
	state_e[state_ec++]=env
}
function block_init(){
	curr_pc=state_i[state_ic-1]
	curr_bl=state_t[state_tc-1]
	curr_bc=blk_getops(curr_bl,curr_ops)
}
function ev(){return state_e[state_ec-1]}
function issue(env,blk){state_i[state_ic-1]=curr_pc;state_e[state_ec++]=env;state_t[state_tc++]=blk;state_i[state_ic++]=0;block_init()}
function descope(){state_tc--;state_ec--;state_ic--;block_init()}
function ret(x){state_p[state_pc++]=x}
function arg(){return state_p[--state_pc]}

BEGIN{call_depth=0}
function docall(f,a,tail,  la,d){
	if(linat(f)){ret(natives(lnat_inner(f),lnat_self(f),a));return}
	if(!lion(f)){ret(l_at(f,l_first(a)));return}
	if(tail)descope()
	la=lon_args(f);d=(count(la)==1&&lvs(l_first(la))~/^\./) ?l_dict(l_list(lms(substr(lvs(l_first(la)),4))),l_list(a)): l_dict(la,a)
	issue(env_bind(lon_closure(f),d),lon_body(f))
	call_depth=max(call_depth,state_ec)
}
function run_block(prog,  steps,pc,op,imm,a,b,c,r,n,ni,z){
	steps=0;state_init(rootenv);issue(rootenv,prog)
	while(state_tc>0){
		op=curr_ops[curr_pc+1];a=oplens[op]
		imm=a==2?curr_ops[curr_pc+2]:0
		curr_pc+=a
		steps++
		if     (op=="DROP" ){arg()}
		else if(op=="DUP"  ){a=arg();ret(a);ret(a)}
		else if(op=="SWAP" ){a=arg();b=arg();ret(a);ret(b);}
		else if(op=="OVER" ){a=arg();b=arg();ret(b);ret(a);ret(b)}
		else if(op=="JUMP" ){curr_pc=0+imm}
		else if(op=="JUMPF"){a=arg();if(!lb(a))curr_pc=0+imm}
		else if(op=="LIT"  ){ret(blk_getimm(curr_bl,0+imm))}
		else if(op=="GET"  ){ret(env_get(ev(),imm))}
		else if(op=="SET"  ){a=arg();env_set(ev(),imm,a);ret(a)}
		else if(op=="LOC"  ){a=arg();env_loc(ev(),imm,a);ret(a)}
		else if(op=="BUND" ){a=lml();for(b=0;b<0+imm;b++)lst_pre(a,arg());ret(a)}
		else if(op=="OP1"  ){                ret(monads(imm,arg()))}
		else if(op=="OP2"  ){        a=arg();ret(dyads(imm,arg(),a))}
		else if(op=="OP3"  ){b=arg();a=arg();ret(triads(imm,arg(),a,b))}
		else if(op=="CALL" ){a=arg();docall(arg(),a,0)}
		else if(op=="TAIL" ){a=arg();docall(arg(),a,1)}
		else if(op=="BIND" ){a=lon_bind(arg(),ev());env_loc(ev(),lon_name(a),a);ret(a);}
		else if(op=="COL"  ){a=arg();b=arg();ret(b);c=dic_clone(b);dset(c,lms("column"),b);issue(env_bind(ev(),c),a)}
		else if(op=="IPRE" ){a=arg();b=arg();ret(b);docall(a,lst_get(b,0+imm),0);if(lion(a)||lii(a)||linat(a)){for(c=0;c<=0+imm;c++)lst_set(b,c,-1)}}
		else if(op=="IPOST"){a=arg();b=arg();c=arg();ret(lst_get(b,0+imm)!=-1?c:a);ret(b);ret(a)}
		else if(op=="AMEND"){
			a=arg();b=arg();c=arg();r=arg();amend_tla=1
			if(count(c)&&lst_get(c,0)==-1){ni=lml();for(z=0;z<count(c);z++)if(lst_get(c,z)!=-1)lst_add(ni,lst_get(c,z));c=ni;amend_tla=0}
			b=amendv(r,c,a,0);if(amend_tla&&imm!="")env_set(ev(),imm,b);ret(b)
		}
		else if(op=="ITER" ){a=arg();ret(lil(a)?a:ld(a));ret(lid(a)?lmd():lml());}
		else if(op=="EACH" ){
			a=arg();b=arg();c=arg();if(count(b)==count(c)){curr_pc=0+imm;ret(b)}else{
				r=lml3(lil(c)?lst_get(c,count(b)):dic_getv(c,count(b)),lil(c)?lmn(count(b)):dic_getk(c,count(b)),lmn(count(b)))
				r=env_bind(ev(),l_dict(a,r));state_e[state_ec++]=r;ret(c);ret(b)
			}
		}
		else if(op=="NEXT"){
			a=arg();b=arg();c=arg();state_ec--
			if(lid(b)){dset(b,dic_getk(c,count(b)),a)}else{lst_add(b,a)}
			ret(c);ret(b);curr_pc=0+imm
		}
		else{print "unknown opcode <"op">";exit 1}
		while(curr_pc>=curr_bc&&state_tc>0)descope()
		if(steps%5000==0)heap_collect()
	}
	return state_pc<1?NONE:arg()
}
function run(text,  prog){
	prog=parse(text)
	if(perr){print_error(sprintf("(%d:%d) %s",parse_r+1,parse_c+1,perr));return}
	return run_block(prog)
}

###########################################################
#
#  Top-Level Interpreter
#
###########################################################

function runfile(filename){run(readtext(filename))}
function repl(  t,p,x){
	while(1){
		printf " ";t=readline();if(!length(t))continue;p=parse(t)
		if(perr){print repeat_char(" ",parse_c)"^\n"perr}
		else{x=run_block(p);env_loc(rootenv,"_",x);print show(x,1)}
	}
}
BEGIN{if(ARGV[1]~/\.lil$/){runfile(ARGV[1])}else{repl()}}
