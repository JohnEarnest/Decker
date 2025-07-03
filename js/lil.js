// lil: Learning in Layers

let allocs=0,calldepth=0,do_panic=0
NIL  =                   {t:'nil'},                       linil=x=>x&&x.t=='nil'
lmn  =x      =>(allocs++,{t:'num',v:isFinite(x)?+x:0}),   lin  =x=>x&&x.t=='num'
lms  =x      =>(allocs++,{t:'str',v:''+x }),              lis  =x=>x&&x.t=='str'
lml  =x      =>(allocs++,{t:'lst',v:x    }),              lil  =x=>x&&x.t=='lst'
lmd  =(k,v)  =>(allocs++,{t:'dic',k:k||[],v:v||[]}),      lid  =x=>x&&x.t=='dic'
lmt  =_      =>(allocs++,{t:'tab',v:new Map()}),          lit  =x=>x&&x.t=='tab'
lmi  =(f,n,x)=>(allocs++,{t:'int',f,n}),                  lii  =x=>x&&x.t=='int'
lmon =(n,a,b)=>(allocs++,{t:'on' ,n:n,a:a,b:b,c:null}),   lion =x=>x&&x.t=='on'
lmnat=f      =>(allocs++,{t:'nat',f:f}),                  linat=x=>x&&x.t=='nat'
lmblk=_      =>(allocs++,{t:'blk',b:[],locals:[]}),       liblk=x=>x&&x.t=='blk'
lmbool=x     =>          x?ONE:ZERO
lmenv=p      =>{allocs++;const r={t:'env',v:new Map(),p:p};r.local=(n,x)=>env_local(r,lms(n),x);return r}

ZERO=lmn(0), ONE=lmn(1), seed=0x12345, max=Math.max, min=Math.min, abs=Math.abs
ISODATE=lms('%04i-%02i-%02iT%02i:%02i:%02iZ%n%m'), PARTS=['year','month','day','hour','minute','second'].map(lms)
drom_toupper=x=>x.replace(/([ßẞ])|([^ßẞ])/g,(_,s,e)=>s?'ẞ': e.toUpperCase())
drom_tolower=x=>x.replace(/([ßẞ])|([^ßẞ])/g,(_,s,e)=>s?'ß': e.toLowerCase())
clchars=x=>x.normalize("NFC").replace(
	/(\r)|(\t)|([‘’])|([“”])|([^\x20-\x7E\n…ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿĀāĂăĄąĆćĒēĘęĪīıŁłŃńŌōŐőŒœŚśŠšŪūŰűŸŹźŻżŽžȘșȚțẞ¡¿«»€°]$)/gm,
	(_,r,t,sq,dq)=>r?'': t?' ': sq?`'`: dq?`"`: '�'
)
drom_chars=`…ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿĀāĂăĄąĆćĒēĘęĪīıŁłŃńŌōŐőŒœŚśŠšŪūŰűŸŹźŻżŽžȘșȚțẞ¡¿«»€°`.split('')
drom_idx=new Map();drom_chars.forEach((c,i)=>drom_idx.set(c.charCodeAt(0),127+i))
drom_to_ord=x=>{const i=x.charCodeAt(0);return i==10||(i>=32&&i<=126)?i: drom_idx.get(i)||255}
drom_from_ord=x=>x==9?' ': x==10?'\n': x==13?'': (x>=32&&x<=126)?String.fromCharCode(x): (x>=127&&x<=240)?drom_chars[x-127]: '�'
wnum=y=>{
	let w='',d='',s=y<0?(y=-y,'-'):'',i=Math.floor(y);y=Math.round((y-Math.floor(y))*1000000);if(y>=1000000)i++
	while(i>0){w=(0|i%10)+w,i=i/10}for(let z=0;z<6;z++){d=(0|y%10)+d,y=y/10}
	return s+('0'+w).replace(/^(0+)(?=[^0])/,'')+('.'+d).replace(/(\.?0+)$/,'')
}
mod=(x,y)=>x-y*Math.floor(x/y)
range=x=>Array.from(Array(x)).map((_,i)=>i)
tab_get=(t,c)=>t.v.get(c)
tab_has=(t,c)=>t.v.get(c)!=undefined
tab_cell=(t,c,i)=>(t.v.get(c)||[])[i]
tab_set=(t,c,v)=>t.v.set(c,v)
tab_cols=t        =>{const r=[]   ;for(let k of t.v.keys())r.push(k);return r}
tab_row=(t,i)     =>{const r=lmd();for(let k of t.v.keys())dset(r,lms(k),tab_cell(t,k,i));return r}
tab_splice=(f,x,t)=>{const r=lmt();for(let k of t.v.keys())tab_set(r,k,f(x,tab_get(t,k)));return r}
tab_clone=t       =>{const r=lmt();for(let k of t.v.keys())tab_set(r,k,tab_get(t,k).slice(0));return r}
tab_rowcount=t=>!t.v.size?0: t.v.values().next().value.length
torect=t=>{
	let n=0;for(let x of t.v.values())n=max(n,lil(x)?count(x):1);
	for(let k of t.v.keys()){const v=tab_get(t,k);tab_set(t,k,take(n,lil(v)?v.v:[v]))}
}
count=x=>lin(x)?1: lis(x)||lil(x)||lid(x)?x.v.length: lit(x)?tab_rowcount(x): 0
rows=x=>{const t=lt(x);return lml(range(tab_rowcount(t)).map(i=>tab_row(t,i)))}
coltab=x=>{
	const n=lmn(x.v.reduce((x,y)=>max(x,lil(y)?count(y):1),0)),r=lmt()
	x.k.map((k,i)=>tab_set(r,ls(k),dyad.take(n,dyad.take(n,lil(x.v[i])?x.v[i]:lml([x.v[i]]))).v));return r
}
rowtab=x=>{
	const ok=[],t=lmt()
	x.v.map(r=>r.k.map(k=>{if(!tab_has(t,ls(k)))ok.push(k);tab_set(t,ls(k),[])}))
	x.v.map(x=>ok.map(k=>tab_get(t,ls(k)).push(dget(x,k)||NIL)));return t
}
listab=x=>{
	const m=x.v.reduce((r,x)=>max(r,count(x)),0);const t=lmt();for(let z=0;z<m;z++)tab_set(t,'c'+z,[])
	x.v.map(row=>{for(let z=0;z<m;z++)tab_get(t,'c'+z).push(z>=count(row)?NIL:row.v[z])});return t
}
tflip=x=>{
	const c=tab_cols(x),kk=c.indexOf('key')>-1?'key':c[0],k=(tab_get(x,kk)||[]).map(ls),cc=c.filter(k=>k!=kk),r=lmt()
	tab_set(r,'key',cc.map(lms));k.map((k,i)=>tab_set(r,k,cc.map(c=>tab_cell(x,c,i))));return r
}
tcat=(x,y)=>{
	const r=lmt()
	tab_cols(x).map(k=>tab_set(r,k,tab_get(x,k).concat(tab_has(y,k)?[]:range(count(y)).map(x=>NIL))))
	tab_cols(y).map(k=>tab_set(r,k,(tab_has(x,k)?tab_get(x,k):range(count(x)).map(x=>NIL)).concat(tab_get(y,k))));return r
}
zip=(x,y,f)=>{const n=count(x),o=count(y)<n?take(n,y.v):y.v;return x.v.map((x,i)=>f(x,o[i%n]))}
dzip=(x,y,f)=>{
	const r=lmd(x.k.slice(0),x.v.map((z,i)=>f(z,dget(y,x.k[i])||NIL)))
	y.k.filter(k=>!dget(x,k)).map(k=>dset(r,k,f(NIL,dget(y,k))));return r
}
match=(x,y)=>x==y?1: (x.t!=y.t)||(count(x)!=count(y))?0: (lin(x)||lis(y))?x.v==y.v:
	         lil(x)?x.v.every((z,i)=>dyad['~'](z,y.v[i]).v): lit(x)?dyad['~'](rows(x),rows(y)).v:
	         lid(x)?x.v.every((z,i)=>dyad['~'](z,y.v[i]).v&&dyad['~'](x.k[i],y.k[i]).v):0
splice=(f,x,y)=>lis(y)?lms(f(x,ll(y)).map(ls).join('')): lid(y)?lmd(f(x,y.k),f(x,y.v)): lit(y)?tab_splice(f,x,y): lml(f(x,ll(y)))
ina=(x,y)=>lmn(lis(y)?y.v.indexOf(ls(x))>=0: lil(y)?y.v.some(y=>match(x,y)): lid(y)?dget(y,x)!=undefined: lit(y)?tab_has(y,ls(x)): x==y)
filter=(i,x,y)=>{
	x=(lis(x)||linil(x))?monad.list(x):lml(ll(x))
	if(lid(y)){const r=lmd();y.k.forEach((k,v)=>i==lb(ina(k,x))&&dset(r,k,y.v[v]));return r}
	if(!lit(y))return lml(ll(y).filter(z=>i==lb(ina(z,x))))
	const n=x.v.every(lin),nx=x.v.map(ln),ix=range(tab_rowcount(y))
	if(n&& i){const r=dyad.take(ZERO,y);nx.forEach(i=>{for(c of y.v.keys()){const v=tab_cell(y,c,i);if(v)tab_get(r,c).push(v)}});return r}
	if(n&&!i){const r=dyad.take(ZERO,y);ix.forEach(i=>{if(nx.indexOf(i)<0){for(let c of y.v.keys())tab_get(r,c).push(tab_cell(y,c,i))}});return r}
	const r=lmt();for(let k of y.v.keys())if(i==lb(ina(lms(k),x)))tab_set(r,k,tab_get(y,k));return r
}
take=(x,y)=>{const n=y.length, s=x<0?mod(x,n):0; return range(abs(x)).map(z=>y[mod(z+s,n)]||NIL)}
dkix=(dict,key)=>dict.k.findIndex(x=>match(key,x)), dget=(dict,key)=>dict.v[dkix(dict,key)]
dvix=(dict,val)=>dict.v.findIndex(x=>match(val,x)), dkey=(dict,val)=>dict.k[dvix(dict,val)]
dset=(d,k,v)=>{const i=d.k.findIndex(x=>match(x,k));if(i<0){d.k.push(k),d.v.push(v)}else{d.v[i]=v};return d}
union=(x,y)=>{const r=lmd(x.k.slice(0),x.v.slice(0));y.k.forEach(k=>dset(r,k,dget(y,k)));return r}
amend=(x,i,y)=>{
	if(lii(x))return x.f(x,i,y)
	if(lit(x)&&lin(i)){
		const r=tab_clone(x), rn=count(x), cols=ll(monad.keys(x)), ri=0|ln(i); y=lid(y)?y: lmd(cols, new Array(cols.length).fill(y))
		if(ri>=0&&ri<rn)y.k.map((k,i)=>{k=ls(k);if(tab_has(r,k))tab_get(r,k)[ri]=y.v[i]});return r
	}
	if(lit(x)&&lis(i)){
		const r=tab_clone(x), rn=count(x), c=lil(y)?ll(y).slice(0,rn): new Array(rn).fill(y)
		while(c.length<rn)c.push(NIL);tab_set(r,ls(i),c);return r
	}
	if(!lis(x)&&!lil(x)&&!lid(x))return amend(lml([]),i,y)
	if((lil(x)||lis(x))&&(!lin(i)||(ln(i)<0||ln(i)>count(x))))return amend(ld(x),i,y)
	if(lil(x)){const r=lml(x.v.slice(0));r.v[ln(i)|0]=y;return r}
	return lid(x)?dset(lmd(x.k.slice(0),x.v.slice(0)),i,y): lis(x)?lms(ls(x).slice(0,ln(i))+ls(y)+ls(x).slice(1+ln(i))): lml([])
}
l_at=(x,y)=>{
	if(linil(x))return NIL; if(lii(x))return lis(y)&&y.v=='type'?lms(x.n): x.f(x,y)
	if(lit(x)&&lin(y))x=monad.rows(x); if((lis(x)||lil(x))&&linil(y))y=ZERO; if((lis(x)||lil(x))&&!lin(y))x=ld(x)
	if(lit(x)&&lis(y)){const r=tab_get(x,ls(y));return r?lml(r):NIL}
	return lis(x)?lms(x.v[ln(y)|0]||''): lil(x)?x.v[ln(y)|0]||NIL: lid(x)?dget(x,y)||NIL: NIL
}
amendv=(x,i,y,n,tla)=>{
	if(lii(x))tla.v=0;const f=monad.first(i[n]||NIL)
	return (!tla.v&&n+1 <i.length)?amendv(l_at(x,f),i,y,n+1,tla):
	       (n+1<i.length)?amend(x,f,amendv(l_at(x,f),i,y,n+1,tla)): (n+1==i.length)?amend(x,f,y): y
}
lb=x=>linil(x)?0: lin(x)?x.v!=0: lis(x)?x.v!='': lil(x)||lid(x)?x.v.length: 1
ln=x=>linil(x)?0: lin(x)?x.v: lis(x)?(isFinite(x.v)?+x.v:0): lil(x)||lid(x)?ln(x.v[0]): 0
ls=x=>lin(x)?wnum(x.v): lis(x)?x.v: lil(x)?x.v.map(ls).join(''): ''
ll=x=>linil(x)?[]: lis(x)?x.v.split('').map(lms): lil(x)||lid(x)?x.v: lit(x)?rows(x).v: [x]
ld=x=>lid(x)?x:lit(x)?monad.cols(x):lil(x)||lis(x)?lmd(range(count(x)).map(lmn),lis(x)?ll(x):x.v):lmd()
lt=x=>{if(lit(x))return x;const r=lmt();if(linil(x))return r;if(lid(x)){tab_set(r,'key',x.k.slice(0))};tab_set(r,'value',ll(x));return r}
vm=f=>{const r=x=>lid(x)?lmd(x.k,x.v.map(r)):lil(x)?lml(x.v.map(r)):f(x);return r}
vd=f=>{const r=(x,y)=>
	 lid(x)&lid(y)?dzip(x,y,r)    :lid(x)&!lid(y)?lmd(x.k.slice(0),x.v.map(x=>r(x,y))):!lid(x)&lid(y)?lmd(y.k.slice(0),y.v.map(y=>r(x,y))):
	 lil(x)&lil(y)?lml(zip(x,y,r)):lil(x)&!lil(y)?lml(x.v.map(x=>r(x,y)))             :!lil(x)&lil(y)?lml(y.v.map(y=>r(x,y))): f(x,y);return r}
vmnl=f=>{const r=x=>lil(x)?(ll(x).some(x=>!lin(x))?lml(x.v.map(r)):f(x)):f(x);return r}
fstr=x=>{
	let ct=0;return x.split('').map(x=>{
		let e=0;if(x=='<'){ct=1}else if(x=='/'&&ct){e=1}else if(x!=' '&&x!='\n'){ct=0}
		return e?'\\/':({'\n':'\\n','\\':'\\\\','"':'\\"'})[x]||x
	}).join('')
}
fjson=x=>lin(x)?wnum(x.v): lit(x)?fjson(rows(x)): lil(x)?`[${x.v.map(fjson).join(',')}]`:
         lis(x)?`"${fstr(x.v)}"`:lid(x)?`{${x.k.map((k,i)=>`${fjson(lms(ls(k)))}:${fjson(x.v[i])}`).join(',')}}`:'null'
fii  =x=>{const e=ifield(x,'encoded');return linil(e)?'null':ls(e)}
flove=x=>lin(x)||lis(x)?fjson(x): lil(x)?`[${x.v.map(flove).join(',')}]`:
         lid(x)?`{${x.k.map((k,i)=>`${flove(k)}:${flove(x.v[i])}`).join(',')}}`:
         lit(x)?`<${tab_cols(x).map(k=>`${flove(lms(k))}:${flove(lml(tab_get(x,k)))}`).join(',')}>`:
         lii(x)?fii(x): 'null'
pjson=(y,h,n)=>{
	const si=h, hn=_=>m&&y[h]&&(n?h-si<n:1), hnn=x=>m&&h+x<=y.length&&(n?h+x-si<n:1)
	const jd=_=>{while(hn()&&/[0-9]/.test(y[h]))h++}, jm=x=>hn()&&y[h]==x?(h++,1):0, iw=_=>/[ \n]/.test(y[h]), ws=_=>{while(hn()&&iw())h++}
	const esc=e=>e=='n'?'\n': /[\\/"']/.test(e)?e: e=='u'&&hnn(4)?String.fromCharCode(parseInt(y.slice(h,h+=4),16)):' '
	let f=1, m=1, rec=_=>{
		const t={null:NIL,false:ZERO,true:ONE};for(let k in t)if(hnn(k.length)&&y.slice(h,h+k.length)==k)return h+=k.length,t[k]
		if(jm('[')){const r=lml([]);while(f&&hn()){ws();if(jm(']'))break;r.v.push(rec()),ws(),jm(',')}return r}
		if(jm('{')){const r=lmd();while(f&&hn()){ws();if(jm('}'))break;const k=rec();ws(),jm(':'),ws();if(f)dset(r,k,rec());ws(),jm(',')}return r}
		if(jm('"')){let r='';while(f&&hn()&&!jm('"'))r+=hnn(2)&&jm('\\')?esc(y[h++]):y[h++];return lms(clchars(r))}
		if(jm("'")){let r='';while(f&&hn()&&!jm("'"))r+=hnn(2)&&jm('\\')?esc(y[h++]):y[h++];return lms(clchars(r))}
		const ns=h;jm('-'),jd(),jm('.'),jd();if(jm('e')||jm('E')){jm('-')||jm('+');jd();}return h<=ns?(f=0,NIL): lmn(+y.slice(ns,h))
	}, r=rec();return {value:r,index:h}
}
idecode=x=>{
	const p=x.slice(0,5).toLowerCase()
	return p=='%%img'?image_read(x): p=='%%snd'?sound_read(x): p=='%%dat'?array_read(x): NIL
}
plove=(y,h,n)=>{
	const si=h, hn=_=>m&&y[h]&&(n?h-si<n:1), hnn=x=>m&&h+x<=y.length&&(n?h+x-si<n:1)
	const jd=_=>{while(hn()&&/[0-9]/.test(y[h]))h++}, jm=x=>hn()&&y[h]==x?(h++,1):0, iw=_=>/[ \n]/.test(y[h]), ws=_=>{while(hn()&&iw())h++}
	const esc=e=>e=='n'?'\n': /[\\/"']/.test(e)?e: e=='u'&&hnn(4)?String.fromCharCode(parseInt(y.slice(h,h+=4),16)):' '
	let f=1, m=1, rec=_=>{
		const t={null:NIL,false:ZERO,true:ONE};for(let k in t)if(hnn(k.length)&&y.slice(h,h+k.length)==k)return h+=k.length,t[k]
		if(jm('[')){const r=lml([]);while(f&&hn()){ws();if(jm(']'))break;r.v.push(rec()),ws(),jm(',')}return r}
		if(jm('{')){const r=lmd();while(f&&hn()){ws();if(jm('}'))break;const k=rec();ws(),jm(':'),ws();if(f)dset(r,       k,         rec()  );ws(),jm(',')}return r}
		if(jm('<')){const r=lmd();while(f&&hn()){ws();if(jm('>'))break;const k=rec();ws(),jm(':'),ws();if(f)dset(r,lms(ls(k)),lml(ll(rec())));ws(),jm(',')}return monad.table(r)}
		if(jm('%')){jm('%');let r='%%';while(f&&hn()&&/[a-zA-Z0-9+/=]/.test(y[h]))r+=y[h++];return idecode(r)}
		if(jm('"')){let r='';while(f&&hn()&&!jm('"'))r+=hnn(2)&&jm('\\')?esc(y[h++]):y[h++];return lms(r)}
		if(jm("'")){let r='';while(f&&hn()&&!jm("'"))r+=hnn(2)&&jm('\\')?esc(y[h++]):y[h++];return lms(r)}
		const ns=h;jm('-'),jd(),jm('.'),jd();if(jm('e')||jm('E')){jm('-')||jm('+');jd();}return h<=ns?(f=0,NIL): lmn(+y.slice(ns,h))
	}, r=rec();return {value:r,index:h}
}
format_has_names=x=>{
	let f=0;while(x[f]){
		if(x[f]!='%'){f++;continue;}f++;if(x[f]=='[')return 1
		if(x[f]=='*')f++;if(x[f]=='-')f++;if(x[f]=='0')f++;while(/[0-9]/.test(x[f]))f++;if(x[f]=='.')f++
		let d=0;while(/[0-9]/.test(x[f]))d=d*10+(+x[f++]);if(!x[f])break;const t=x[f++];if(t=='r'||t=='o')while(d&&x[f])d--,f++
	}return 0
}
razetab=x=>{const k=tab_cols(x);return dyad.dict(lml(tab_get(x,k[0])||[]),lml(tab_get(x,k[1])||[]))}
monad={
	'-':    vm(x=>lmn(-ln(x))),
	'!':    vm(x=>lmbool(!lb(x))),
	floor:  vm(x=>lmn(Math.floor(ln(x)))),
	cos:    vm(x=>lmn(Math.cos(ln(x)))),
	sin:    vm(x=>lmn(Math.sin(ln(x)))),
	tan:    vm(x=>lmn(Math.tan(ln(x)))),
	exp:    vm(x=>lmn(Math.exp(ln(x)))),
	ln:     vm(x=>lmn(Math.log(ln(x)))),
	sqrt:   vm(x=>lmn(Math.sqrt(ln(x)))),
	unit:   vm(x=>{const n=ln(x);return lml([lmn(Math.cos(n)),lmn(Math.sin(n))])}),
	mag:    vmnl(x=>lmn(Math.sqrt(ll(x).reduce((x,y)=>x+Math.pow(ln(y),2),0)))),
	heading:vmnl(x=>{const a=getpair(x);return lmn(Math.atan2(a.y,a.x))}),
	sum:    x=>ll(x).reduce(dyad['+'],ZERO),
	prod:   x=>ll(x).reduce(dyad['*'],ONE),
	raze:   x=>lit(x)?razetab(x) :ll(x).slice(1).reduce(dyad[','],monad.first(x)),
	max:    x=>ll(x).slice(1).reduce(dyad['|'],monad.first(x)),
	min:    x=>ll(x).slice(1).reduce(dyad['&'],monad.first(x)),
	count:  x=>lmn(count(x)),
	first:  x=>linil(x)?NIL: lis(x)?(x.v.length?lms(x.v[0           ]):NIL): lit(x)?monad.first(rows(x)): lion(x)?lms(x.n): linat(x)?lms('native'): count(x)?ll(x)[0]: NIL,
	last:   x=>linil(x)?NIL: lis(x)?(x.v.length?lms(x.v[x.v.length-1]):NIL): lit(x)?monad.last (rows(x)): count(x)?ll(x)[count(x)-1]: NIL,
	keys:   x=>lml(lii(x)?[]: lion(x)?x.a.map(lms): ld(x).k),
	range:  x=>lml(lin(x)?range(max(0,0|ln(x))).map(lmn): ld(x).v),
	list:   x=>lml([x]),
	typeof: x=>lms(({num:"number",str:"string",lst:"list",dic:"dict",tab:"table",on:"function",nat:"function",nil:"nil"})[x.t]||x.n||"interface"),
	flip:   x=>lit(x)?tflip(x):lml(range(ll(x).reduce((w,z)=>max(w,lil(z)?count(z):1),0)).map(i=>lml(ll(x).map(c=> !lil(c)?c: i<count(c)?c.v[i]: NIL)))),
	rows:   x=>rows(x),
	cols:   x=>{const t=lt(x),k=tab_cols(t);return lmd(k.map(lms),k.map(x=>lml(tab_get(t,x))))},
	table:  x=>lid(x)?coltab(x): lil(x)&&x.v.every(lid)?rowtab(x): lil(x)&&x.v.every(lil)?listab(x): lt(x),
	'@tab': t=>{
		t=lt(t);const r=tab_clone(t)
		tab_set(r,'index' ,range(count(r)).map(lmn))
		tab_set(r,'gindex',range(count(r)).map(lmn))
		tab_set(r,'group' ,range(count(r)).map(x=>ZERO))
		return r
	},
}
dyad={
	'+':  vd((x,y)=>lmn(ln(x)+ln(y))),
	'-':  vd((x,y)=>lmn(ln(x)-ln(y))),
	'*':  vd((x,y)=>lmn(ln(x)*ln(y))),
	'/':  vd((x,y)=>lmn(ln(x)/ln(y))),
	'%':  vd((x,y)=>lmn(mod(ln(y),ln(x)))),
	'^':  vd((x,y)=>lmn(Math.pow(ln(x),ln(y)))),
	'<':  vd((x,y)=>lmn((linil(x)||lin(x))&&(linil(y)||lin(y))?ln(x)< ln(y): ls(x)< ls(y))),
	'>':  vd((x,y)=>lmn((linil(x)||lin(x))&&(linil(y)||lin(y))?ln(x)> ln(y): ls(x)> ls(y))),
	'=':  vd((x,y)=>lmn(lii(x)||lii(y)?x==y: linil(x)||linil(y)?x==y: lin(x)&&lin(y)?ln(x)==ln(y): ls(x)==ls(y))),
	'&':  vd((x,y)=>lin(x)||lin(y)?lmn(min(ln(x),ln(y))): lms(ls(x)<ls(y)?ls(x):ls(y))),
	'|':  vd((x,y)=>lin(x)||lin(y)?lmn(max(ln(x),ln(y))): lms(ls(x)>ls(y)?ls(x):ls(y))),
	split:(x,y)=>lml(ls(y).split(ls(x)).map(lms)),
	fuse: (x,y)=>lms(ll(y).map(ls).join(ls(x))),
	dict: (x,y)=>(y=ll(y),ll(x).reduce((d,x,i)=>dset(d,x,y[i]||NIL),lmd())),
	take: (x,y)=>lis(y)&&lin(x)&&ln(x)<0&&abs(ln(x))<=count(y)?lms(y.v.slice(count(y)+ln(x))):
                 lis(y)&&lin(x)&&ln(x)>=0&&ln(x)<=count(y)?lms(y.v.slice(0,ln(x))):
	             lin(x)?splice(take,ln(x),y):filter(1,x,y),
	drop: (x,y)=>lis(y)&&lin(x)&&ln(x)>=0?lms(y.v.slice(ln(x))):
                 lis(y)&&lin(x)&&ln(x)<0 ?lms(y.v.slice(0,max(0,count(y)+ln(x)))):
	             lin(x)?splice((x,y)=>x<0?y.slice(0,x):y.slice(x),ln(x),y):filter(0,x,y),
	limit:(x,y)=>count(y)>ln(x)?dyad.take(lmn(ln(x)),y):y,
	in:   (x,y)=>lil(x)?lml(x.v.map(x=>ina(x,y))):ina(x,y),
	',':  (x,y)=>lit(x)&&lit(y)?tcat(x,y): lid(x)?union(x,ld(y)):
                 (lis(x)||linil(x))?dyad[','](lml([x]),y): (lis(y)||linil(y))?dyad[','](x,lml([y])): lml(ll(x).concat(ll(y))),
	'~':  (x,y)=>lmbool(match(x,y)),
	unless:(x,y)=>linil(y)?x:y,
	join: (x,y)=>{ // natural join on tables.
		const f=x=>lin(x)?monad.range(x):lml(ll(x));if(!lit(x)||!lit(y))return lml(zip(f(x),f(y),dyad[',']))
		const a=x,b=y,ak=tab_cols(a),bk=tab_cols(b), ik=bk.filter(x=>ak.indexOf(x)>=0),dk=bk.filter(x=>ak.indexOf(x)<0)
		const r=lmt(); ak.forEach(k=>tab_set(r,k,[])), dk.forEach(k=>tab_set(r,k,[]))
		for(let ai=0;ai<count(a);ai++)for(let bi=0;bi<count(b);bi++)if(ik.every(k=>match(tab_cell(a,k,ai),tab_cell(b,k,bi))))
			ak.forEach(k=>tab_get(r,k).push(tab_cell(a,k,ai))),dk.forEach(k=>tab_get(r,k).push(tab_cell(b,k,bi)))
		return r
	},
	cross: (x,y)=>{ // cartesian join; force columns to be unique:
		const f=x=>lt(lin(x)?monad.range(x):lml(ll(x)));if(!lit(x)||!lit(y))return lml(rows(dyad.cross(f(x),f(y))).v.map(x=>lml(x.v)))
		const a=lt(x),b=lt(y), ak=tab_cols(a),bk=tab_cols(b), uk=bk.map(x=>ak.indexOf(x)>=0?x+'_':x)
		const r=lmt(); ak.forEach(k=>tab_set(r,k,[])), uk.forEach(k=>tab_set(r,k,[]))
		for(let bi=0;bi<count(b);bi++)for(let ai=0;ai<count(a);ai++){
			ak.forEach(k=>tab_get(r,k).push(tab_cell(a,k,ai)))
			bk.forEach((k,i)=>tab_get(r,uk[i]).push(tab_cell(b,k,bi)))
		}return r
	},
	parse: (x,y)=>{
		if(lil(y))return lml(y.v.map(y=>dyad.parse(x,y)))
		x=ls(x),y=ls(y);let f=0,h=0,m=1,pi=0,named=format_has_names(x),r=named?lmd():lml([]);while(x[f]){
			if(x[f]!='%'){if(m&&x[f]==y[h]){h++}else{m=0}f++;continue}f++
			let nk=null;if(x[f]=='['){f++;nk='';while(x[f]&&x[f]!=']')nk+=x[f++];if(x[f]==']')f++}
			let im=m,n=0,d=0,v=null,si=h,sk=x[f]=='*'&&(f++,1),lf=x[f]=='-'&&(f++,1);if(x[f]=='0')f++
			const hn=_=>m&&y[h]&&(n?h-si<n:1), id=x=>/[0-9]/.test(x), ix=_=>/[0-9a-fA-F]/.test(y[h]), iw=_=>/[ \n]/.test(y[h])
			while(id(x[f]))n=n*10+(+x[f++]);x[f]=='.'&&f++
			while(id(x[f]))d=d*10+(+x[f++]);if(!x[f])break;const t=x[f++]
			if('%mnzsluqarojJ'.indexOf(t)<0)while(hn()&&iw())h++
			if(t=='%'){if(m&&t==y[h]){h++}else{m=0}}
			else if(t=='m'){v=lmbool(m)}
			else if(t=='n'){v=lmn(h)}
			else if(t=='z'){v=lmbool(m&&h==y.length)}
			else if(t=='s'||t=='l'||t=='u'){v=lms('');while(hn()&&(n?1:y[h]!=x[f]))v.v+=y[h++];if(t=='l')v.v=drom_tolower(v.v);if(t=='u')v.v=drom_toupper(v.v)}
			else if(t=='a'){v=lml([]);while(hn()&&(n?1:y[h]!=x[f]))v.v.push(lmn(drom_to_ord(y[h++])))}
			else if(t=='b'){v=lmbool(/[tTyYx1]/.test(y[h]));while(hn()&&(n?1:y[h]!=x[f]))h++}
			else if(t=='i'){v=lmn(0);const s=(y[h]=='-')?(h++,-1):1;m&=id(y[h]);while(hn()&&id(y[h]))v.v=v.v*10+(+y[h++]);v.v*=s}
			else if(t=='h'||t=='H'){v=lmn(0),                       m&=ix();    while(hn()&&ix())v.v=v.v*16+parseInt(y[h++],16)}
			else if(t=='j'){if(m){const j=pjson(y,h,n);h=j.index,v=j.value}else{v=NIL}}
			else if(t=='J'){if(m){const j=plove(y,h,n);h=j.index,v=j.value}else{v=NIL}}
			else if(t=='v'){v=lms(''),m&=!id(y[h]);while(hn()&&/[0-9a-zA-Z_?]/.test(y[h]))v.v+=y[h++];m&=count(v)>0}
			else if(t=='q'){
				v=lms(''),m&=y[h]=='"';if(m)h++;while(hn()&&y[h]!='"'){
					if(y[h]=='\\'){h++;if(/[n\\"]/.test(y[h])){v.v+=y[h]=='n'?'\n':y[h]}else{m=0}}else{v.v+=y[h]}h++
				}if(m&=y[h]=='"')h++;if(!m)v=NIL
			}
			else if(t=='f'||t=='c'||t=='C'){
				v=lmn(0);let p=10,s=(y[h]=='-')?(h++,-1):1; if(t=='c'&&m&&y[h]=='$')h++
				m&=id(y[h])||y[h]=='.';  while(hn()&&id(y[h]))v.v=v.v*10+(+y[h++])
				m&&hn()&&y[h]=='.'&&h++; while(hn()&&id(y[h]))v.v+=(+y[h++])/p,p*=10;v.v*=s
			}
			else if(t=='r'||t=='o'){
				let cc=x.slice(f,f+(d||1));v=lms(''),f+=(d||1);
				while(hn()){if(cc.indexOf(y[h])>=0==lf?1:0){if(n)m=0;break}v.v+=y[h++];if(t=='o')break;}if(!m)v.v='';
			}
			else if(t=='e'||t=='p'){
				const [dy,dm,dd,dh,dmi,ds,dl,dma]=ll(dyad.parse(ISODATE,lms(y.slice(h)))), l=ln(dl), d=new Date(y.slice(h,h+l))
				if(l&&ln(dma)){h+=l}else{m=0}; v=t=='e'?lmn(m?0|(d.getTime()/1000):0):lmd(PARTS,[dy,dm,dd,dh,dmi,ds].map(x=>lmn(ln(x))))
			}else{m=0}while(n&&y[h]&&h-si<n)h++,m=0;
			if(!sk&&v){
				if     (!im&&'%mnz'.indexOf(t)==-1)v=NIL              // some previous pattern failed
				else if((h-si)==0&&'fcCihHv'.indexOf(t)>=0)v=NIL,m=0  // no characters consumed
				named?dset(r,nk!=null?lms(nk):lmn(pi),v):r.v.push(v);pi++
			}
		}return named?r: r.v.length==1?r.v[0]:r
	},
	format: (x,y)=>{
		const frec=(i,x,y)=>{
			if(i>=count(x))return y
			const fuse=(count(x)-i)%2?0:1,named=format_has_names(ls(x.v[i+fuse]))
			const r=lml(ll(lit(y)?rows(y):y).map(z=>dyad.format(x.v[i+fuse],frec(i+fuse+1,x,lit(y)&&!named?lml(ll(z)):z))))
			return fuse?dyad.fuse(x.v[i],r):r
		};if(lil(x))return frec(0,x,y)
		x=ls(x);let r='',f=0,h=0,named=format_has_names(x);y=named?ld(y):lil(y)?y:monad.list(y);while(x[f]){
			if(x[f]!='%'){r+=x[f++];continue}f++
			let nk=null;if(x[f]=='['){f++;nk='';while(x[f]&&x[f]!=']')nk+=x[f++];if(x[f]==']')f++}
			let o='',n=0,d=0,sk=x[f]=='*'&&(f++,1),lf=x[f]=='-'&&(f++,1),pz=x[f]=='0'&&(f++,1)
			const hn=_=>m&&y[h]&&(n?h-si<n:1), id=x=>/[0-9]/.test(x)
			while(id(x[f]))n=n*10+(+x[f++]);x[f]=='.'&&f++
			while(id(x[f]))d=d*10+(+x[f++]);if(!x[f])break;const t=x[f++]
			const an=named?dget(y,nk!=null?lms(nk):lmn(h)): null
			const a=t=='%'?NIL: named?(an?an:NIL): (!sk&&h<count(y))?y.v[h]: NIL; if(t!='%'&&!sk)h++
			if     (t=='%'){o='%'}
			else if(t=='s'||t=='v'){o=ls(a)}
			else if(t=='l'){o=drom_tolower(ls(a))}
			else if(t=='u'){o=drom_toupper(ls(a))}
			else if(t=='r'||t=='o'){o=ls(a),lf=1;d=max(1,d);while(d&&x[f])d--,f++;d=n;}
			else if(t=='a'){o=ll(a).map(x=>drom_from_ord(ln(x))).join('')}
			else if(t=='b'){o=lb(a)?'true':'false'}
			else if(t=='f'){o=d?ln(a).toFixed(min(100,d)):wnum(ln(a))}
			else if(t=='c'){const v=ln(a);o=(v<0?'-':'')+'$'+abs(v).toFixed(min(100,d)||2)}
			else if(t=='C'){const v=ln(a);o=(v<0?'-':'')    +abs(v).toFixed(min(100,d)||2)}
			else if(t=='i'){o=''+Math.trunc(ln(a))}
			else if(t=='h'||t=='H'){o=ln(a).toString(16);if(t=='H')o=o.toUpperCase()}
			else if(t=='e'){o=new Date(ln(a)*1000).toISOString().split('.')[0]+'Z'}
			else if(t=='p'){const d=ld(a);o=dyad.format(ISODATE,lml(PARTS.map(x=>dget(d,x)))).v}
			else if(t=='j'){o=fjson(a)}
			else if(t=='J'){o=flove(a)}
			else if(t=='q'){o=fjson(lms(ls(a)))}
			let vn=o.length; if(d&&(t=='f'||t=='c'||t=='C'))d=0;if(d&&lf)vn=min(d,vn)
			if(n&&!lf)for(let z=0;z<n-vn;z++)r+=pz?'0':' '
			for(let z=d&&!lf?max(0,vn-d):0;z<vn;z++)r+=o[z]
			if(n&&lf)for(let z=0;z<n-vn;z++)r+=pz?'0':' '
		}return lms(r)
	},
	like: (x,y)=>{
		if(!lil(y))y=monad.list(y);const pats=y.v.map(pat=>{
			const r={m:'',l:'',a:[]},ch=ls(pat).split('');for(let i=0;i<ch.length;i++){
				r.m+=ch[i]=='`'&&i<ch.length-1?(r.l+=ch[++i],'a'): ch[i]in{'.':1,'*':1,'#':1}?(r.l+='!',ch[i]): (r.l+=ch[i],'a')
				while(ch[i]=='*'&&ch[i+1]=='*')i++
			}return r
		})
		const test=str=>{
			for(let pi=0;pi<pats.length;pi++){
				const m=pats[pi].m, l=pats[pi].l, sc=m.length, a=new Uint8Array(sc);a[0]=m[0]=='*'
				if(!sc&&!str.length){return ONE}else if(!sc)continue;for(let ci=0;ci<str.length;ci++){
					const c=str[ci];for(let si=sc-1;si>=0;si--){
						const prev=(si>0&&a[si-1])||(si==0&&ci==0)||(si>1&&m[si-1]=='*'&&a[si-2])
						a[si]=m[si]=='*'?a[si]||prev: m[si]=='.'?prev: m[si]=='#'?/[0-9]/.test(c)&&prev: c==l[si]&&prev
					}
				}if(a[sc-1]||(sc>1&&m[sc-1]=='*'&&a[sc-2]))return ONE
			}return ZERO
		};return lil(x)?lml(x.v.map(x=>test(ls(x)))): test(ls(x))
	},
	window: (x,y)=>{
		let n=ln(x), r=[], con;if(lis(y)){y=ls(y),con=lms}else{y=ll(y),con=lml}
		if(n>0){     for(let z=0;z    <y.length;z+=n)r.push(con(y.slice(z,z+n)))}
		if(n<0){n=-n;for(let z=0;z+n-1<y.length;z++ )r.push(con(y.slice(z,z+n)))}
		return lml(r)
	},
	fill: (x,y)=>{
		if(lit(y)){const r=lmt();for(let k of y.v.keys())tab_set(r,k,ll(dyad.fill(x,lml(tab_get(y,k)))));return r}
		return lil(y)?lml(ll(y).map(z=>dyad.fill(x,z))): lid(y)?lmd(y.k.slice(0),y.v.map(z=>dyad.fill(x,z))): linil(y)?x: y
	},
	'@where': (col,tab)=>{
		const w=dyad.take(lmn(count(tab)),lml(ll(col)))
		const p=lml(range(count(tab)).filter(i=>lb(w.v[i])).map(lmn))
		const r=dyad.take(p,tab);tab_set(r,'gindex',range(count(r)).map(lmn));return r
	},
	'@by': (col,tab)=>{
		const b=dyad.take(lmn(count(tab)),lml(ll(col))), r=monad.rows(tab), u=b.v.reduce((x,y)=>{dset(x,y,y);return x},lmd([],[]))
		const rows=u.v.map((v,group)=>{
			const p=lml(range(count(tab)).filter(x=>match(v,b.v[x])).map(lmn))
			const s=dyad.take(p,tab);tab_set(s,'gindex',range(count(s)).map(lmn)),tab_set(s,'group',range(count(s)).map(_=>lmn(group)));return s
		});return lml(rows)
	},
}
merge=(vals,keys,widen)=>{
	const i=lms('@index');let ix=null
	if(!widen){ix=lml([]);vals.v.map((val,z)=>{dget(val,i).v.map(x=>ix.v.push(x))})}
	if(widen)vals.v=vals.v.filter(x=>count(dget(x,i)))
	if(count(vals)==0)vals.v.push(keys.v.reduce((x,y)=>dset(x,y,lml([])),lmd()))
	let r=monad.raze(lml(vals.v.map(x=>monad.table(widen?x:dyad.drop(i,x)))))
	if(widen){ix=lml(tab_get(r,'@index')),r=dyad.drop(i,r)}
	return {r,ix}
}
n_uplevel=([a])=>{let i=2, e=getev(), r=null, name=ls(a); while(e&&i){r=e.v.get(name);if(r)i--;e=e.p};return r||NIL}
n_eval=([x,y,extend])=>{
	y=y?ld(y):lmd();const yy=lmd(y.k.slice(0),y.v.slice(0)), r=lmd(['value','vars'].map(lms),[NIL,yy])
	const feval=([r,x])=>{
		dset(r,lms('value'),x);const b=dget(r,lms('vars')), v=getev().v;
		for(let k of v.keys()){dset(b,lms(k),v.get(k))};return r
	}
	try{
		const prog=parse(x?ls(x):'')
		blk_opa(prog,op.BUND,2),blk_lit(prog,lmnat(feval)),blk_op(prog,op.SWAP),blk_op(prog,op.CALL)
		issue(env_bind(extend&&lb(extend)?getev():null,yy.k.map(ls),lml(yy.v)),prog)
	}catch(e){dset(r,lms('error'),lms(e.x)),dset(r,lms('errorpos'),lml([lmn(e.r),lmn(e.c)]))};return r
}
triad={
	'@orderby': (col,tab,order_dir)=>{
		const lex_list=(x,y,a,ix)=>{
			if(x.length<ix&&y.length<ix)return 0;const xv=x[ix]||NIL,yv=y[ix]||NIL
			return lex_less(xv,yv)?a: lex_more(xv,yv)?!a: lex_list(x,y,a,ix+1)
		}
		const lex_less=(a,b)=>lil(a)&&lil(b)? lex_list(a.v,b.v,1,0): lb(dyad['<'](a,b))
		const lex_more=(a,b)=>lil(a)&&lil(b)? lex_list(a.v,b.v,0,0): lb(dyad['>'](a,b))
		const o=dyad.take(lmn(count(tab)),lml(ll(col)));order_dir=ln(order_dir)
		const pv=range(count(tab)).sort((a,b)=>{
			if(lex_less(o.v[a],o.v[b]))return  order_dir
			if(lex_more(o.v[a],o.v[b]))return -order_dir
			return a-b // produce a stable sort
		})
		const rt=dyad.take(lml(pv.map(lmn)),tab)
		tab_set(rt,'gindex',range(count(tab)).map(lmn));return rt
	},
	'@sel': (orig,vals,keys)=>{
		const mv=merge(vals,keys,0);return count(keys)>1?mv.r: dyad.take(mv.ix,dyad.drop(lml(['index','gindex','group'].map(lms)),orig))
	},
	'@ext': (orig,vals,keys)=>{
		const r=monad.cols(triad['@sel'](orig,vals,keys))
		return count(keys)==1?(count(r)?monad.first(r):lml([])): count(r)!=1||count(r.k[0])?r: monad.first(r)
	},
	'@upd': (orig,vals,keys)=>{
		orig=dyad.drop(lml(['index','gindex','group'].map(lms)),orig);const mv=merge(vals,keys,1),r=mv.r,ix=mv.ix
		tab_cols(r).map(c=>{
			if(tab_get(r,c)==ix)return;const ci=tab_has(orig,c),col=range(count(orig)).map(z=>ci?tab_cell(orig,c,z):NIL)
			tab_set(orig,c,col),ix.v.map((x,row)=>col[0|ln(x)]=tab_cell(r,c,row))
		});return orig
	},
	'@ins': (v,n,x)=>{
		const nc=count(n), rc=Math.ceil(count(v)/nc), r=monad.table(lmd(n.v,n.v.map((_,z)=>lml(range(rc).map(r=>v.v[nc*r+z]||NIL)))))
		return lin(x)?r:dyad[','](lt(x),r)
	},
}

findop=(n,prims)=>Object.keys(prims).indexOf(n), as_enum=x=>x.split(',').reduce((x,y,i)=>{x[y]=i;return x},{})
let tnames=0;tempname=_=>lms(`@t${tnames++}`)
op=as_enum('JUMP,JUMPF,LIT,DUP,DROP,SWAP,OVER,BUND,OP1,OP2,OP3,GET,SET,LOC,AMEND,TAIL,CALL,BIND,ITER,EACH,NEXT,COL,IPRE,IPOST,FIDX,FMAP')
oplens=   [ 3   ,3    ,3  ,1  ,1   ,1   ,1   ,3   ,3  ,3  ,3  ,3  ,3  ,3  ,3    ,1   ,1   ,1   ,1   ,3   ,3   ,1  ,3   ,3    ,3   ,3    ]
blk_addb=(x,n  )=>x.b.push(0xFF&n)
blk_here=(x    )=>x.b.length
blk_setb=(x,i,n)=>x.b[i]=0xFF&n
blk_getb=(x,i  )=>0xFF&x.b[i]
blk_adds=(x,n  )=>{blk_addb(x,n>>8),blk_addb(x,n)}
blk_sets=(x,i,n)=>{blk_setb(x,i,n>>8),blk_setb(x,i+1,n)}
blk_gets=(x,i  )=>0xFFFF&(blk_getb(x,i)<<8|blk_getb(x,i+1))
blk_op  =(x,o  )=>{blk_addb(x,o);if(o==op.COL)blk_addb(x,op.SWAP)}
blk_opa =(x,o,i)=>{blk_addb(x,o),blk_adds(x,i);return blk_here(x)-2}
blk_imm =(x,o,k)=>{let i=x.locals.findIndex(x=>match(x,k));if(i==-1)i=x.locals.length,x.locals.push(k);blk_opa(x,o,i)}
blk_op1 =(x,n)=>blk_opa(x,op.OP1,findop(n,monad))
blk_op2 =(x,n)=>blk_opa(x,op.OP2,findop(n,dyad ))
blk_op3 =(x,n)=>blk_opa(x,op.OP3,findop(n,triad))
blk_lit =(x,v)=>blk_imm(x,op.LIT,v)
blk_set =(x,n)=>blk_imm(x,op.SET,n)
blk_loc =(x,n)=>blk_imm(x,op.LOC,n)
blk_get =(x,n)=>blk_imm(x,op.GET,n)
blk_getimm=(x,i)=>x.locals[i]
blk_cat=(x,y)=>{
	let z=0,base=blk_here(x);while(z<blk_here(y)){
		const b=blk_getb(y,z);if(b==op.LIT||b==op.GET||b==op.SET||b==op.LOC||b==op.AMEND){blk_imm(x,b,blk_getimm(y,blk_gets(y,z+1)))}
		else if(b==op.JUMP||b==op.JUMPF||b==op.EACH||b==op.NEXT||b==op.FIDX){blk_opa(x,b,blk_gets(y,z+1)+base)}
		else{for(let i=0;i<oplens[b];i++)blk_addb(x,blk_getb(y,z+i))}z+=oplens[b]
	}
}
blk_loop=(b,names,f)=>{
	blk_op(b,op.ITER);const head=blk_here(b);blk_lit(b,names);const each=blk_opa(b,op.EACH,0)
	f(),blk_opa(b,op.NEXT,head),blk_sets(b,each,blk_here(b))
}
blk_end=x=>{
	let z=0;while(z<blk_here(x)){
		let b=blk_getb(x,z);z+=oplens[b];if(b!=op.CALL)continue
		let t=1,i=z;while(i<blk_here(x)){if(blk_getb(x,i)!=op.JUMP){t=0;break}const a=blk_gets(x,i+1);if(a<=i){t=0;break}i=a}if(t)blk_setb(x,z-1,op.TAIL)
	}return x
}

parse=text=>{
	let i=0,r=0,c=0, tq=null // text index, row, column, token queued
	const er=x=>{throw {x,r,c,i,stack:new Error().stack}}
	const nc=_=>{const x=text[i++];x=='\n'?(r++,c=0):(c++);return x}
	const iw=_=>text[i]in{' ':1,'\t':1,'\n':1,'#':1}
	const sw=_=>{while(iw())if(text[i]=='#')while(i<text.length&&text[i]!='\n')nc();else nc()}
	const id=_=>0<='0123456789'.indexOf(text[i])
	//          !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
	const tcc=' s" sss ()ssssdsdddddddddd: sssn@nnnnnnnnnnnnnnnnnnnnnnnnnn[ ]sn nnnnnnnnnnnnnnnnnnnnnnnnnn s s'
	const ncc='                nnnnnnnnnn     n nnnnnnnnnnnnnnnnnnnnnnnnnn    n nnnnnnnnnnnnnnnnnnnnnnnnnn    '
	const mcc='     xx x xxxx x          x xxx x                          x  x                             x x'
	const esc={'\\':'\\','"':'"','n':'\n'}
	const ne=_=>{const e=nc();return esc[e]?esc[e]: er(`Invalid escape character '\\${e}' in string.`)}
	const nw=x=>{let v=+x;    while(id())v=(v*10)+(+nc());  return v} 
	const nf=_=>{let v=0,p=10;while(id())v+=(+nc())/p,p*=10;return v}
	const nn=(x,tr,tc,v,sign)=>{
		if(x=='.'&&!id())return{t:'.',r:tr,c:tc}
		if(x=='.')v=nf();else v=nw(x),v+=(text[i]=='.')?(nc(),nf()):0
		return {t:'number',v:sign*v,r:tr,c:tc}
	}
	const tok=_=>{
		const w=iw()||i==0||(mcc[text[i-1].charCodeAt(0)-32]=='x');sw();if(i>=text.length)return{t:'the end of the script'}
		const tr=r,tc=c, x=nc(), cc=tcc[x.charCodeAt(0)-32]; let v=0
		if(cc==' '||cc==undefined)er(`Invalid character '${x}'.`)
		if(x=='-'&&w&&tcc[(text[i]||'').charCodeAt(0)-32]=='d')return nn(nc(),r,c,v,-1)
		if(cc=='n'){let v=x; while(ncc[(text[i]||' ').charCodeAt(0)-32]=='n')v+=nc();return {t:'name',v,r:tr,c:tc}}
		if(cc=='"'){let v='',c;while(i<text.length&&(c=nc())!='"')v+=(c=='\\'?ne():clchars(c));return{t:'string',v,r:tr,c:tc}}
		return cc=='s'?{t:'symbol',v:x,r:tr,c:tc}: cc=='d'?nn(x,tr,tc,v,1):{t:x,r:tr,c:tc}
	}
	const peek=_=>{if(!tq)tq=tok();return tq}
	const hasnext=_=>peek().t!='the end of the script'
	const peek2=_=>{const pi=i,pr=r,pc=c,pq=tq;next();const v=peek();i=pi,r=pr,c=pc,tq=pq;return v}
	const next=_=>{if(tq){const r=tq;tq=null;return r};return tok()}
	const matchp=k=>peek().t=='name'&&peek().v==k
	const match=k=>matchp(k)?(next(),1):0
	const matchsp=k=>peek().t==k?(next(),1):0
	const expect=t=>peek().t==t?next().v:er(`Expected ${t}, but found ${peek().t}.`)
	const ident=n=>{
		const kw={while:1,each:1,send:1,on:1,if:1,elseif:1,else:1,end:1,do:1,with:1,local:1,select:1,extract:1,update:1,insert:1,
			into:1,from:1,where:1,by:1,orderby:1,asc:1,desc:1};return !(kw.hasOwnProperty(n)||monad.hasOwnProperty(n)||dyad.hasOwnProperty(n))
	}
	const name=n=>{const r=expect('name');if(!ident(r)&&n!='member')er(`'${r}' is a keyword, and cannot be used for a ${n} name.`);return r}
	const names=(x,n)=>{const r=[];while(!match(x))r.push(name(n));return r}
	const quote=_=>{const r=lmblk();expr(r);blk_end(r);return r}
	const block=_=>{const r=lmblk();iblock(r);return r}
	const quotesub=_=>{let c=0,r=lmblk();while(hasnext()&&!matchsp(']'))expr(r),c++;blk_opa(r,op.BUND,c);return r}
	const quotedot=_=>{const r=lmblk();blk_lit(r,lml([lms(name('member'))]));return r}
	const iblock=r=>{let c=0;while(hasnext()){if(match('end')){if(!c)blk_lit(r,NIL);return}if(c)blk_op(r,op.DROP);expr(r),c++};er(`Expected 'end' for block.`)}
	const parseclause=(b,func)=>{
		const iter_group=(g,f)=>{if(g){const n=tempname();blk_loop(b,[ls(n)],_=>{blk_get(b,n),f()})}else{f()}}
		if(match('where')){
			const ex=quote(),grouped=parseclause(b,func)
			iter_group(grouped,_=>{blk_lit(b,ex),blk_op(b,op.COL),blk_op2(b,'@where')});return grouped
		}
		if(match('orderby')){
			const ex=quote(),dir=match('asc')?-1: match('desc')?1: er(`Expected 'asc' or 'desc'.`), grouped=parseclause(b,func)
			iter_group(grouped,_=>{blk_lit(b,ex),blk_op(b,op.COL),blk_lit(b,lmn(dir)),blk_op3(b,'@orderby')});return grouped
		}
		if(match('by')){
			const ex=quote(),grouped=parseclause(b,func);if(grouped)blk_op1(b,'raze')
			blk_lit(b,ex),blk_op(b,op.COL),blk_op2(b,'@by');return 1
		}
		if(!match('from'))er(`Expected 'from'.`);expr(b),blk_op1(b,'@tab'),blk_op(b,op.DUP);return 0
	}
	const parsequery=(b,func,dcol)=>{
		const cols=lmd([],[]);while(!matchp('from')&&!matchp('where')&&!matchp('by')&&!matchp('orderby')){
			let set=peek2().t==':', lit=peek().t=='string', name=lms(lit?(set?peek().v:''):peek().t=='name'?peek().v:'')
			let get=ident(ls(name)), unique=ls(name).length&&dkix(cols,name)==-1; if(set&&lit&&!unique)next(),next()
			const x=set&&unique?(next(),next(),name): get&&unique&&dcol?name: lms(dcol?`c${cols.k.length}`: '')
			cols.k.push(x),cols.v.push(quote())
		}
		const grouped=parseclause(b,func), index=lmblk();blk_get(index,lms('index'))
		const keys=lml(cols.k.concat([lms('@index')])),n=tempname();if(!grouped)blk_op1(b,'list')
		blk_loop(b,[ls(n)],_=>{
			blk_lit(b,keys),blk_get(b,n),cols.v.map(x=>(blk_lit(b,x),blk_op(b,op.COL)))
			blk_lit(b,index),blk_op(b,op.COL),blk_op(b,op.DROP),blk_opa(b,op.BUND,count(keys)),blk_op2(b,'dict')
		}),blk_lit(b,keys),blk_op3(b,func)
	}
	const parseindex=(b,name)=>{
		const i=[];while(({'[':1,'.':1})[peek().t]){
			if(matchsp('['))i.push(quotesub())
			if(matchsp('.')){
				if(({'[':1,'.':1})[peek().t]){
					const vn=tempname()
					i.map(v=>(blk_cat(b,v),blk_op(b,op.CALL))),blk_loop(b,[vn.v],_=>{blk_get(b,vn),parseindex(b)});return
				}else{i.push(quotedot())}
			}
		}
		if(matchsp(':')){
			i.map(v=>blk_cat(b,v)),blk_opa(b,op.BUND,i.length),blk_op(b,op.OVER)
			for(let z=0;z<i.length-1;z++)blk_opa(b,op.IPRE,z),blk_opa(b,op.IPOST,z);expr(b),blk_imm(b,op.AMEND,name||ZERO)
		}else{i.map(v=>(blk_cat(b,v),blk_op(b,op.CALL)))}
	}
	const term=b=>{
		if(peek().t=='number'){blk_lit(b,lmn(next().v));return}
		if(peek().t=='string'){blk_lit(b,lms(next().v));return}
		if(match('if')){
			const fin=[];let c=0,e=0,next=-1;expr(b);next=blk_opa(b,op.JUMPF,0);while(hasnext()){
				if(match('elseif')){
					if(e)er(`Expected 'end'.`)
					if(!c)blk_lit(b,NIL);c=0;fin.push(blk_opa(b,op.JUMP,0)),blk_sets(b,next,blk_here(b)),expr(b),next=blk_opa(b,op.JUMPF,0);continue
				}
				if(match('else')){
					if(e)er(`Expected 'end'.`)
					if(!c)blk_lit(b,NIL);c=0,e=1;fin.push(blk_opa(b,op.JUMP,0)),blk_sets(b,next,blk_here(b)),next=-1;continue
				}
				if(match('end')){
					if(!c)blk_lit(b,NIL);c=0;if(!e)fin.push(blk_opa(b,op.JUMP,0));if(next!=-1)blk_sets(b,next,blk_here(b));if(!e)blk_lit(b,NIL)
					fin.map(x=>blk_sets(b,x,blk_here(b)));return
				}
				if(c)blk_op(b,op.DROP);expr(b),c++
			}
		}
		if(match('while')){
			blk_lit(b,NIL);const head=blk_here(b);expr(b);const cond=blk_opa(b,op.JUMPF,0)
			blk_op(b,op.DROP),iblock(b),blk_opa(b,op.JUMP,head),blk_sets(b,cond,blk_here(b));return
		}
		if(match('each')){const n=names('in','variable');expr(b),blk_loop(b,n,_=>iblock(b));return}
		if(match('on')){
			const n=name('function'),v=matchsp('.')&&matchsp('.')&&matchsp('.');let a=names('do','argument')
			if(v&&a.length!=1)return er(`Variadic functions must take exactly one named argument.`);if(v)a=['...'+a[0]]
			blk_lit(b,lmon(n,a,blk_end(block()))),blk_op(b,op.BIND);return
		}
		if(match('send')){
			blk_lit(b,lmnat(n_uplevel)),blk_lit(b,lml([lms(name('function'))])),blk_op(b,op.CALL)
			expect('['),blk_cat(b,quotesub()),blk_op(b,op.CALL);return
		}
		if(match('local')){const n=lms(name('variable'));expect(':'),expr(b),blk_loc(b,n);return}
		if(match('select' )){parsequery(b,'@sel',1);return}
		if(match('extract')){parsequery(b,'@ext',0);return}
		if(match('update' )){parsequery(b,'@upd',1);return}
		if(match('insert')){
			const n=lml([]);while(!match('with')){n.v.push(lms(peek().t=='string'?next().v:name('column')))}
			let v=0,i=0;while(1){if(match('into')){i=1;break}if(match('end')){i=0;break}expr(b),v++}
			if(n.v.length==0&&v==0&&i==0){blk_lit(b,lmt());return}
			if(n.v.length<1)n.v.push(lms('value'));blk_opa(b,op.BUND,v),blk_lit(b,n);if(i){expr(b)}else{blk_lit(b,ZERO)}
			blk_op3(b,'@ins');return
		}
		if(matchsp('(')){if(matchsp(')')){blk_lit(b,lml([]));return}expr(b),expect(')');return}
		const s=peek().v;if(findop(s,monad)>=0&&({'symbol':1,'name':1})[peek().t]){
			next();if(matchsp('@')){
				let depth=0,l=lmblk();while(matchsp('@'))depth++
				expr(b),blk_opa(l,op.FMAP,findop(s,monad))
				while(depth-->0){const t=tempname(),m=lmblk();blk_loop(m,[ls(t)],_=>{blk_get(m,t),blk_cat(m,l)}),l=m}
				blk_cat(b,l)
			}else{expr(b),blk_op1(b,s)};return
		}const n=lms(name('variable'));if(matchsp(':')){expr(b),blk_set(b,n);return}blk_get(b,n),parseindex(b,n)
	}
	const expr=b=>{
		term(b);if(({'[':1,'.':1})[peek().t]){parseindex(b)}
		if(matchsp('@')){
			let depth=0;while(matchsp('@'))depth++
			const func=tempname();blk_set(b,func),blk_op(b,op.DROP),expr(b)
			let l=lmblk();blk_get(l,func),blk_op(l,op.SWAP);const fidx=blk_opa(l,op.FIDX,0)
			blk_loop(l,['v'],_=>{blk_get(l,func),blk_get(l,lms('v')),blk_opa(l,op.BUND,1),blk_op(l,op.CALL)})
			blk_sets(l,fidx,blk_here(l))
			while(depth-->0){const t=tempname(),m=lmblk();blk_loop(m,[ls(t)],_=>{blk_get(m,t),blk_cat(m,l)}),l=m}
			blk_cat(b,l);return
		}const s=peek().v;if(findop(s,dyad)>=0&&({'symbol':1,'name':1})[peek().t]){next(),expr(b),blk_op2(b,s)}
	}
	const b=lmblk();if(hasnext())expr(b);while(hasnext())blk_op(b,op.DROP),expr(b)
	if(blk_here(b)==0)blk_lit(b,NIL);return b
}

env_local=(e,n,x)=>{e.v.set(ls(n),x)}
env_getr =(e,n  )=>{const k=ls(n);r=e.v.get(k);return r?r: e.p?env_getr(e.p,n): null}
env_setr =(e,n,x)=>{const k=ls(n);r=e.v.get(k);return r?e.v.set(k,x): e.p?env_setr(e.p,n,x): null}
env_get  =(e,n  )=>env_getr(e,n)||NIL
env_set  =(e,n,x)=>{const r=env_getr(e,n);r?env_setr(e,n,x):env_local(e,n,x)}
env_bind =(e,k,v)=>{const r=lmenv(e); k.map((a,i)=>env_local(r,lms(a),v.v[i]||NIL));return r}
const monadi=Object.values(monad), dyadi=Object.values(dyad), triadi=Object.values(triad), states=[]; let state=null
pushstate=env=>{if(state){states.push(state)};state={e:[env],p:[],t:[],pcs:[]}}
popstate =_=>{state=states.pop()}
halt     =_=>{state.p=[],state.t=[],state.e=[state.e[0]]}
running  =_=>state.t.length>0
getev    =_=>state.e  [state.e  .length-1]
getblock =_=>state.t  [state.t  .length-1]
getpc    =_=>state.pcs[state.pcs.length-1]
setpc    =x=>state.pcs[state.pcs.length-1]=x
issue    =(env,blk)=>(state.e.push(env),state.t.push(blk),state.pcs.push(0))
descope  =_=>(state.e.pop(),state.t.pop(),state.pcs.pop())
ret      =x=>state.p.push(x)
arg      =_=>state.p.pop()
docall=(f,a,tail)=>{
	if(linat(f)){ret(f.f(ll(a)));return}
	if(!lion(f)){ret(l_at(f,monad.first(a)));return}
	if(tail){descope()}
	issue(f.a.length==1&&f.a[0][0]=='.'?env_bind(f.c,[f.a[0].slice(3)],monad.list(a)): env_bind(f.c,f.a,a),f.b)
	calldepth=max(calldepth,state.e.length)
}
runop=_=>{
	const b=getblock();if(!liblk(b))ret(state.t.pop())
	const pc=getpc(),o=blk_getb(b,pc),imm=(oplens[o]==3?blk_gets(b,1+pc):0); setpc(pc+oplens[o])
	switch(o){
		case op.DROP :arg();break
		case op.DUP  :{const a=arg();ret(a),ret(a);break}
		case op.SWAP :{const a=arg(),b=arg();ret(a),ret(b);break}
		case op.OVER :{const a=arg(),b=arg();ret(b),ret(a),ret(b);break}
		case op.JUMP :setpc(imm);break
		case op.JUMPF:if(!lb(arg()))setpc(imm);break
		case op.LIT  :ret(blk_getimm(b,imm));break
		case op.GET  :{ret(env_get(getev(),blk_getimm(b,imm)));break}
		case op.SET  :{const v=arg();env_set(getev(),blk_getimm(b,imm),v),ret(v);break}
		case op.LOC  :{const v=arg();env_local(getev(),blk_getimm(b,imm),v),ret(v);break}
		case op.BUND :{const r=[];for(let z=0;z<imm;z++)r.push(arg());r.reverse(),ret(lml(r));break}
		case op.OP1  :{                      ret(monadi[imm](arg()    ));break}
		case op.OP2  :{const         y=arg();ret(dyadi [imm](arg(),y  ));break}
		case op.OP3  :{const z=arg(),y=arg();ret(triadi[imm](arg(),y,z));break}
		case op.IPRE :{const s=arg(),i=arg();ret(i),docall(s,i.v[imm]);if(lion(s)||lii(s)||linat(s)){for(let z=0;z<=imm;z++)i.v[z]=null}break}
		case op.IPOST:{const s=arg(),i=arg(),r=arg();ret(i.v[imm]?r:s),ret(i),ret(s);break}
		case op.AMEND:{
			let v=arg(),r=arg(),i=ll(arg()),ro=arg(),n=blk_getimm(b,imm),t={v:1}
			if(i.length&&!i[0]){i=i.filter(x=>x),t.v=0}r=amendv(ro,i,v,0,t);if(t.v&&!lin(n))env_set(getev(),n,r);ret(r);break
		}
		case op.CALL : // fall through:
		case op.TAIL :{const a=arg(),f=arg();docall(f,a,o==op.TAIL);break}
		case op.BIND :{const f=arg(),r=lmon(f.n,f.a,f.b);r.c=getev(),env_local(getev(),lms(f.n),r),ret(r);break}
		case op.ITER :{const x=arg();ret(lil(x)?x:ld(x));ret(lid(x)?lmd():lml([]));break}
		case op.FIDX :{const x=arg(),f=arg();if((lid(f)||lil(f)||lis(f))&&lil(x)){ret(lml(x.v.map(x=>l_at(f,x))));setpc(imm)}else{ret(x)};break}
		case op.FMAP :{const x=arg(),f=monadi[imm];ret(lid(x)?lmd(x.k,x.v.map(f)):lml(ll(x).map(f)));break}
		case op.EACH :{
			const n=arg(),r=arg(),s=arg();if(count(r)==count(s)){setpc(imm),ret(r);break}
			const z=count(r), v=lml([s.v[z],lid(s)?s.k[z]:lmn(z),lmn(z)]);
			state.e.push(env_bind(getev(),n,v)),ret(s),ret(r);break
		}
		case op.NEXT :{const v=arg(),r=arg(),s=arg();state.e.pop();if(lid(r))r.k.push(s.k[r.v.length]);r.v.push(v),ret(s),ret(r),setpc(imm);break}
		case op.COL  :{
			const ex=arg(),t=arg(),n=tab_cols(t),v=ll(monad.cols(t));ret(t)
			n.push('column'),v.push(t),issue(env_bind(getev(),n,lml(v)),ex);break
		}
	}while(running()&&getpc()>=blk_here(getblock()))descope()
}

fchar=x=>x=='I'?'i': x=='B'?'b': x=='L'?'s': x
n_writecsv=([x,y,d])=>{
	let r='', spec=y?ls(y).split(''):[];const t=lt(x), c=tab_cols(t).length; d=d?ls(d)[0]:','
	while(spec.length<c)spec.push('s')
	let n=0;spec.forEach((x,i)=>{if(x=='_')return;if(n)r+=d;n++;r+=tab_cols(t)[i]||`c${i+1}`})
	rows(t).v.forEach(row=>{
		r+='\n';let n=0;spec.forEach((x,i)=>{
			if(x=='_')return;if(n)r+=d;n++
			const vv=row.v[i], fc=fchar(x), sv=dyad.format(lms('%'+fc),fc=='j'||fc=='a'?monad.list(vv):vv).v
			r+=(/["\n]/.test(sv)||sv.indexOf(d)>=0?`"${sv.replace(/"/g,'""')}"`:sv)
		})
	});return lms(r)
}
n_readcsv=([x,y,d])=>{
	let i=0,n=0, spec=y&&lis(y)?ls(y):null, text=count(x)?ls(x):'', r=lmt(); d=d?ls(d)[0]:','
	const nv=_=>{let r='';while(text[i]&&text[i]!='\n'&&text[i]!=d)r+=text[i++];return r}, match=x=>text[i]==x?(i++,1):0
	while(i<text.length&&text[i]!='\n'){
		while(match(' '));const v=nv();if(!spec||(n<spec.length&&spec[n]!='_'))tab_set(r,v,[]);n++;if(match('\n'))break;while(match(' '));match(d)
	}
	while(spec&&n<spec.length){if(spec[n]!='_'){tab_set(r,'c'+n,[])};n++}
	if(!spec)spec='s'.repeat(tab_cols(r).length)
	let slots=0,slot=0;spec.split('').map(z=>{if(z!='_')slots++;});slots=min(slots,tab_cols(r).length),n=0
	if(i>=text.length)return r;while(i<=text.length){
		while(match(' '));
		let val='';if(match('"')){while(text[i]){if(match('"')){if(match('"')){val+='"'}else{break}}else{val+=text[i++]}}}else{val=nv()}
		if(spec[n]&&spec[n]!='_'){
			const k=tab_cols(r)[slot], x=(val[0]||'').toLowerCase(), s=spec[n]
			let sign=1,o=0; if(val[o]=='-')sign=-1,o++;if(val[o]=='$')o++;
			tab_get(r,k).push(dyad.parse(lms('%'+fchar(s)),lms(val))),slot++
		};n++
		if(i>=text.length||text[i]=='\n'){
			while(n<spec.length){const u=spec[n++];if(u!='_'&&slot<slots)tab_get(r,tab_cols(r)[slot++]).push(NIL);}
			if(text[i]=='\n'&&i==text.length-1)break;i++,n=0,slot=0
		}else{while(match(' '));match(d)}
	};return r
}
n_writexml=([x,fmt])=>{
	fmt=fmt?lb(fmt):0
	const esc=x=>{const e={'&':'amp',"'":'apos','"':'quot','<':'lt','>':'gt'};return ls(x).replace(/[&'"<>]/g,x=>e[x]?`&${e[x]};`:x)}
	const rec=(x,tab)=>{
		if(array_is(x)){
			const ck=lms('cast'),c=ifield(x,'cast');iwrite(x,ck,lms('char'))
			const r=iwrite(x,lml([ZERO,ifield(x,'size')]));iwrite(x,ck,c);return ls(r)
		}
		if(lil(x))return x.v.map(x=>rec(x,tab)).join(''); if(!lid(x))return esc(x)+((tab&&fmt)?'\n':'')
		const t=ls(dget(x,lms('tag'))||lms('')),a=ld(dget(x,lms('attr'))||lmd()),c=ll(dget(x,lms('children'))||lml([]))
		const r=`<${t}${a.k.map((k,i)=>` ${ls(k)}="${esc(a.v[i])}"`).join('')}${c.length?'':'/'}>${fmt?'\n':''}`
		return c.length?`${r}${c.map(x=>(' '.repeat(fmt?tab+2:0))+rec(x,tab+2)).join('')}${' '.repeat(fmt?tab:0)}</${t}>${fmt?'\n':''}`:r
	};return lms(rec(x,0))
}
n_readxml=([x])=>{
	let i=0,t=ls(x)
	const xm=x=>t[i]==x?(i++,1):0
	const xc=_=>{while(t[i]&&t[i]!='>')i++;i++}
	const xs=_=>{let w=0;while(/[ \n]/.test(t[i]))i++,w=1;return w}
	const xe=(a,b)=>t.slice(i,i+2+a.length)==`&${a};`?(i+=2+a.length,b):null
	const name=_=>{let r='';xs();while(t[i]&&!/[>/= \n]/.test(t[i]))r+=t[i++];xs();return r.toLowerCase()}
	const text=stop=>{
		let r='';while(t[i]&&!(stop==' '&&/[>/ \n]/.test(t[i]))){
			if(xs())r+=' ';if(stop==t[i]||!t[i])break;r+=xe('amp','&')||xe('apos',"'")||xe('quot','"')||xe('lt','<')||xe('gt','>')||xe('nbsp',' ')||t[i++]
		}if(/['"]/.test(stop)&&t[i])i++;return lms(r)
	}
	const rec=ctag=>{
		let r=[];while(t[i]){
			const w=xs();if(t.slice(i,i+9)=='<![CDATA['){i+=9;let c='';while(t[i]&&t.slice(i,i+3)!=']]>')c+=t[i++];i+=3;r.push(lms(c));continue}
			if(t[i]!='<'){if(w)i--;r.push(text('<'));continue}i++,xs()
			if(xm('!')||xm('?')){xc();continue};if(xm('/')){const n=name();xm('>');if(ctag==n)break;continue}
			const tag=lmd(),attr=lmd(),n=name();r.push(tag),dset(tag,lms('tag'),lms(n)),dset(tag,lms('attr'),attr),dset(tag,lms('children'),lml([]))
			while(t[i]&&!/[>/]/.test(t[i])){const n=lms(name());if(xm('=')){xs(),dset(attr,n,text(xm("'")?"'":xm('"')?'"':' '))}else{dset(attr,n,ONE)}}
			if(xm('/')){xc()}else{if(t[i])i++;dset(tag,lms('children'),rec(n))}
		}return lml(r)
	}
	return rec('')
}
n_random=z=>{
	const randint=x=>{let y=seed;y^=(y<<13),y^=(y>>>17),(y^=(y<<15));return mod(seed=y,x);} // xorshift32
	const randelt=x=>lin(x)?lmn(randint(ln(x))): count(x)<1?NIL: l_at(x,lmn(randint(count(x))))
	if(z.length==0){randint(1);return lmn((seed&0x7FFFFFFF)/0x7FFFFFFF)}
	let x=z[0]||NIL;if(lid(x))x=monad.range(x);if(z.length<2)return randelt(x)
	const y=ln(z[1]);if(y>=0){const r=[];for(let z=0;z<y;z++)r.push(randelt(x));return lml(r);}
	x=lin(x)?monad.range(x).v:ll(x);if(x.length<1)x=[NIL];const p=range(x.length),r=[]
	for(let i=x.length-1;i>0;i--){const j=randint(i+1),t=p[j];p[j]=p[i],p[i]=t}
	for(let z=0;z<abs(y);z++)r.push(x[p[z%x.length]]);return lml(r)
}
let frame_count=0
interface_system=lmi((self,i,x)=>{
	if(!i)return NIL
	if(x){if(lis(i)&&i.v=='seed'){seed=0|ln(x);return x}}
	if(lis(i)&&i.v=='version'   )return lms(VERSION)
	if(lis(i)&&i.v=='platform'  )return lms('web')
	if(lis(i)&&i.v=='seed'      )return lmn(seed)
	if(lis(i)&&i.v=='frame'     )return lmn(frame_count)
	if(lis(i)&&i.v=='now'       )return lmn(0|(new Date().getTime()/1000))
	if(lis(i)&&i.v=='ms'        )return lmn(0|(Date.now()))
	if(lis(i)&&i.v=='workspace' )return lmd(['allocs','depth'].map(lms),[allocs,calldepth].map(lmn))
	return x?x:NIL
},'system')
showt=(x,toplevel)=>{
	if(!toplevel){
		const d=monad.rows(x).v.map(r=>r.v.map(v=>show(v)).join(' ')).join(' ')
		return `insert ${tab_cols(x).map(x=>x+' ').join('')}with ${d?d+' ':''}end`
	}
	try{
	const w=tab_cols(x).map(k=>tab_get(x,k).reduce((x,y)=>max(x,show(y).length+2),k.length+2))
	const s='+'+tab_cols(x).map((x,i)=>"-".repeat(w[i])).join('+')+'+'
	const v=range(tab_rowcount(x)).map(r=>tab_cols(x).map(k=>' '+show(tab_cell(x,k,r))).map((f,i)=>f+(' '.repeat(max(0,w[i]-f.length)))))
	         .map(x=>`|${x.join('|')}|`).join('\n')
	return `${s}\n|${tab_cols(x).map((x,i)=>` ${x+(' '.repeat(w[i]-x.length-2))} `).join('|')}|\n${s}${v.length?'\n'+v+'\n'+s:''}`
	}catch(err){console.log('cannot serialize',x);throw err}
}
show=(x,toplevel)=>linat(x)?'on native x do ... end': linil(x)?(toplevel?'':'nil'):
		lil(x)?`(${x.v.map(x=>show(x)).join(',')})`: lit(x)?showt(x,toplevel): lion(x)?`on ${x.n}${x.a.map(x=>' '+x).join('')} do ... end`:
		lis(x)?`"${x.v.split('').map(x=>({'\n':'\\n','\\':'\\\\','"':'\\"'})[x]||x).join('')}"`:
		lin(x)?fjson(x):lid(x)?`{${x.k.map((k,i)=>`${show(k)}:${show(x.v[i])}`).join(',')}}`:
		lii(x)?`<${x.n}>`:`<INVALID ${x}>`

// dom + utilities

FORMAT_VERSION=1, RTEXT_END=2147483647, SFX_RATE=8000, ANTS=255
FRAME_QUOTA=MODULE_QUOTA=10*4096, TRANS_QUOTA=2*4096, LOOP_QUOTA=1*4096, ATTR_QUOTA=1*4096, BRUSH_QUOTA=128
sleep_frames=0, sleep_play=0, pending_popstate=0
DEFAULT_HANDLERS=`
on link x do go[x] end
on drag x do if !me.locked|me.draggable me.line[(pointer.prev-me.offset)/me.scale x] end end
on order x do if !me.locked me.value:select orderby me.value[x] asc from me.value end end
on changecell x do
	f:me.format[me.col] f:if count f f else "s" end
	me.cellvalue:("%%%l" format f) parse x
	me.event["change" me.value]
end
on navigate x do if x~"right" go["Next"] end if x~"left" go["Prev"] end end
on loop x do x end
`
DEFAULT_TRANSITIONS=`
transition[on SlideRight c a b t do  c.paste[a c.size*t,0   ] c.paste[b c.size*(t-1),0]      end]
transition[on SlideLeft  c a b t do  c.paste[a c.size*(-t),0] c.paste[b c.size*(1-t),0]      end]
transition[on SlideDown  c a b t do  c.paste[a c.size*0,t   ] c.paste[b c.size*0,t-1  ]      end]
transition[on SlideUp    c a b t do  c.paste[a c.size*0,-t  ] c.paste[b c.size*0,1-t  ]      end]
transition[on WipeRight  c a b t do  c.rect[0,0        c.size*t,1    ]          c.merge[a b] end]
transition[on WipeLeft   c a b t do  c.rect[0,0        c.size*(1-t),1]          c.merge[b a] end]
transition[on WipeDown   c a b t do  c.rect[0,0        c.size*1,t    ]          c.merge[a b] end]
transition[on WipeUp     c a b t do  c.rect[0,0        c.size*1,1-t  ]          c.merge[b a] end]
transition[on BoxIn      c a b t do  c.rect[c.size/2   c.size*t   "center"]     c.merge[a b] end]
transition[on BoxOut     c a b t do  c.rect[c.size/2   c.size*1-t "center"]     c.merge[b a] end]
`
FONTS={
	body:"%%FNT1CAoBIAEAAAAAAAAAAAAAIQEAgICAgIAAgAAAIgMAoKAAAAAAAAAAIwUAAFD4UPhQAAAAJAUgcKigcCiocCAAJQg"+
	"Af5KUbhkpRgAAJgcwSFAgVIiUYgAAJwEAgIAAAAAAAAAAKAMgQICAgICAQCAAKQOAQCAgICAgQIAAKgUAUCD4IFAAAAAA"+
	"KwUAACAg+CAgAAAALAIAAAAAAAAAQECALQQAAAAA8AAAAAAALgEAAAAAAAAAgAAALwQQECAgQECAgAAAMAUAcIiIiIiIc"+
	"AAAMQUAIGAgICAgIAAAMgUAcIgIECBA+AAAMwUA+BAgcAiIcAAANAUAEDBQkPgQEAAANQUA+IDwCAiIcAAANgUAMECA8I"+
	"iIcAAANwUA+AgQECAgIAAAOAUAcIiIcIiIcAAAOQUAcIiIeAgQYAAAOgIAAABAAAAAQAAAOwMAAAAgAAAAICBAPAQAABA"+
	"gQCAQAAAAPQUAAAD4APgAAAAAPgQAAEAgECBAAAAAPwUAMEgIECAAIAAAQAcAOESaqqqcQDgAQQUAICBQUPiIiAAAQgUA"+
	"8IiI8IiI8AAAQwUAcIiAgICIcAAARAUA4JCIiIiQ4AAARQQA8ICA4ICA8AAARgQA8ICA4ICAgAAARwUAcIiAmIiIcAAAS"+
	"AUAiIiI+IiIiAAASQIAQEBAQEBAQAAASgUACAgICIiIcAAASwUAiJCgwKCQiAAATAQAgICAgICA8AAATQcAgsaqkoKCgg"+
	"AATgUAyMioqJiYiAAATwUAcIiIiIiIcAAAUAUA8IiI8ICAgAAAUQUAcIiIiIiocBAAUgUA8IiI8KCQiAAAUwUAcIiAcAi"+
	"IcAAAVAUA+CAgICAgIAAAVQUAiIiIiIiIcAAAVgUAiIiIUFAgIAAAVwcAgoJUVCgoKAAAWAUAiIhQIFCIiAAAWQUAiIhQ"+
	"ICAgIAAAWgQA8BAgQICA8AAAWwNgQEBAQEBAQGAAXASAgEBAICAQEAAAXQNgICAgICAgIGAAXgMAQKAAAAAAAAAAXwYAA"+
	"AAAAAAA/AAAYAIAgEAAAAAAAAAAYQQAAABgEHCQcAAAYgQAgIDgkJCQ4AAAYwQAAABgkICQYAAAZAQAEBBwkJCQcAAAZQ"+
	"QAAABgkPCAYAAAZgQAMEDgQEBAQAAAZwQAAABwkJCQcBBgaAQAgIDgkJCQkAAAaQIAQADAQEBAQAAAagMAIABgICAgICD"+
	"AawQAgICQoMCgkAAAbAIAwEBAQEBAQAAAbQcAAADskpKSkgAAbgQAAADgkJCQkAAAbwQAAABgkJCQYAAAcAQAAADgkJCQ"+
	"4ICAcQQAAABwkJCQcBAQcgQAAACwwICAgAAAcwQAAABwgGAQ4AAAdAMAQEDgQEBAIAAAdQQAAACQkJCQcAAAdgUAAACIU"+
	"FAgIAAAdwcAAACCVFQoKAAAeAUAAACIUCBQiAAAeQQAAACQkJCQcBBgegQAAADwIECA8AAAewMgQEBAgEBAQCAAfAGAgI"+
	"CAgICAgIAAfQOAQEBAIEBAQIAAfgUAaLAAAAAAAAAAfwUAAAAAAAAAqAAAgAVAIAAgUFD4iAAAgQUQIAAgUFD4iAAAggU"+
	"gUAAgUFD4iAAAgwUoUAAgUFD4iAAAhAVQACAgUFD4iAAAhQUgUCAgUFD4iAAAhgYAPDBQUPiQnAAAhwUAcIiAgICIcCBg"+
	"iARAIADwgOCA8AAAiQQgQADwgOCA8AAAigQgUADwgOCA8AAAiwRQAPCA4ICA8AAAjAKAQABAQEBAQAAAjQMgQABAQEBAQ"+
	"AAAjgNAoABAQEBAQAAAjwOgAEBAQEBAQAAAkAUA4JCI6IiQ4AAAkQUoUADIqKiYmAAAkgVAIHCIiIiIcAAAkwUQIHCIiI"+
	"iIcAAAlAUgUHCIiIiIcAAAlQUoUHCIiIiIcAAAlgVQAHCIiIiIcAAAlwcAOkRMVGREuAAAmAVAIIiIiIiIcAAAmQUQIIi"+
	"IiIiIcAAAmgUgUACIiIiIcAAAmwVQAIiIiIiIcAAAnAUQIIiIUCAgIAAAnQUAgPCIiPCAgAAAngUAcIiwiIiIsAAAnwRA"+
	"IABgEHCQcAAAoAQgQABgEHCQcAAAoQQgUABgEHCQcAAAogRQoABgEHCQcAAAowQAUABgEHCQcAAApAQgUCBgEHCQcAAAp"+
	"QcAAAB8En6QfAAApgQAAABgkICQYCBgpwRAIABgkPCAYAAAqAQgQABgkPCAYAAAqQQgUABgkPCAYAAAqgQAUABgkPCAYA"+
	"AAqwKAQADAQEBAQAAArAMgQADAQEBAQAAArQNAoADAQEBAQAAArgMAoADAQEBAQAAArwUAaBAoeIiIcAAAsARQoADgkJC"+
	"QkAAAsQRAIABgkJCQYAAAsgQgQABgkJCQYAAAswQgUABgkJCQYAAAtARQoABgkJCQYAAAtQQAUABgkJCQYAAAtgYAAAA0"+
	"SHhIsAAAtwRAIACQkJCQcAAAuAQgQACQkJCQcAAAuQQgUACQkJCQcAAAugQAUACQkJCQcAAAuwQgQACQkJCQcBBgvAUAA"+
	"ICwyIjIsIAAvQQAUACQkJCQcBBgvgVwICBQUPiIiAAAvwQAcABgEHCQcAAAwAVQcCAgUFD4iAAAwQRQcABgEHCQcAAAwg"+
	"UAICBQUPiIiBAIwwQAAABgEHCQcCAQxAUQIHCIgICIcAAAxQQgQABgkICQYAAAxgRwAPCA4ICA8AAAxwQAcABgkPCAYAA"+
	"AyAQA8ICA4ICA8CAQyQQAAABgkPCAYEAgygPgAEBAQEBAQAAAywMA4ADAQEBAQAAAzAIAAADAQEBAQAAAzQUAQEBQYMBA"+
	"eAAAzgUAYCgwYKAgIAAAzwUQIMjIqKiYmAAA0AQgQADgkJCQkAAA0QVwAHCIiIiIcAAA0gQAcABgkJCQYAAA0wVIkHCIi"+
	"IiIcAAA1AVIkABgkJCQYAAA1QcAfpCQnJCQfgAA1gYAAAB4pLygeAAA1wUQIHiAcAiIcAAA2AQgQABwgGAQ4AAA2QVQIH"+
	"iAcAiIcAAA2gRQIABwgGAQ4AAA2wVwAIiIiIiIcAAA3AQAcACQkJCQcAAA3QVIkACIiIiIcAAA3gVIkACQkJCQcAAA3wV"+
	"QAIiIUCAgIAAA4AQgQPAQIECA8AAA4QQgQADwIECA8AAA4gQgAPAQIECA8AAA4wQAIADwIECA8AAA5ARQIPAQIECA8AAA"+
	"5QRQIADwIECA8AAA5gUAcIiAcAiIcCBA5wQAAABwgGAQ4CBA6AUA+CAgICAgACAg6QMAQEDgQEBAIEBA6gUA+IiQsIiIs"+
	"AAA6wEAAIAAgICAgIAA7AUAABAAECBASDAA7QUAAChQoFAoAAAA7gUAAKBQKFCgAAAA7wYAOETwQPBEOAAA8AUwSEgwAA"+
	"AAAAAA/wYA/My05Pzs/AAA",
	menu:"%%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"+
	"D/CAAAfwBjAF0AXQB7AHcAdwB/AHcAfwAAAAAA",
	mono:"%%FNT1BQsBIAUAAAAAAAAAAAAAACEFAAAgICAgIAAgAAAiBQAAUFBQAAAAAAAAIwUAAFD4UPhQAAAAACQFACBwqKBwKKh"+
	"wIAAlBQAASKhQIFCokAAAJgUAAGCQoECokGgAACcFACAgIAAAAAAAAAAoBQAQICBAQEAgIBAAKQUAIBAQCAgIEBAgACoF"+
	"AAAgqHCoIAAAAAArBQAAACAg+CAgAAAALAUAAAAAAAAAYGAgQC0FAAAAAAD4AAAAAAAuBQAAAAAAAAAwMAAALwUICBAQI"+
	"CBAQICAADAFAABwiJioyIhwAAAxBQAAIGAgICAgIAAAMgUAAHCICBAgQPgAADMFAABwiAgwCIhwAAA0BQAAEDBQkPgQEA"+
	"AANQUAAPiA8AgIiHAAADYFAABwgPCIiIhwAAA3BQAA+AgIECAgIAAAOAUAAHCIiHCIiHAAADkFAABwiIiIeAhwAAA6BQA"+
	"AADAwAAAwMAAAOwUAAABgYAAAYGAgQDwFAAAIECBAIBAIAAA9BQAAAAD4APgAAAAAPgUAAEAgEAgQIEAAAD8FAABwiAgQ"+
	"IAAgAABABQBwiIio6LCAiHAAQQUAAHCIiPiIiIgAAEIFAADwiIjwiIjwAABDBQAAcIiAgICIcAAARAUAAPCIiIiIiPAAA"+
	"EUFAAD4gIDwgID4AABGBQAA+ICA8ICAgAAARwUAAHCIgJiIiHAAAEgFAACIiIj4iIiIAABJBQAAICAgICAgIAAASgUAAA"+
	"gICAiIiHAAAEsFAACIkKDAoJCIAABMBQAAgICAgICA+AAATQUAAIjYqIiIiIgAAE4FAACIyKiYiIiIAABPBQAAcIiIiIi"+
	"IcAAAUAUAAPCIiPCAgIAAAFEFAABwiIiIiIhwCABSBQAA8IiI8IiIiAAAUwUAAHCIgHAIiHAAAFQFAAD4ICAgICAgAABV"+
	"BQAAiIiIiIiIcAAAVgUAAIiIiFBQICAAAFcFAACIiIiIqNiIAABYBQAAiFAgICBQiAAAWQUAAIiIiFAgICAAAFoFAAD4C"+
	"BAgQID4AABbBQAwICAgICAgIDAAXAWAgEBAICAQEAgIAF0FADAQEBAQEBAQMABeBQAgUIgAAAAAAAAAXwUAAAAAAAAAAP"+
	"gAAGAFAEAgEAAAAAAAAABhBQAAAAB4iIiYaAAAYgUAAICA8IiIiPAAAGMFAAAAAHCIgIB4AABkBQAACAh4iIiIeAAAZQU"+
	"AAAAAcIj4gHgAAGYFAAAYIHAgICAgAABnBQAAAAB4iIiIeAhwaAUAAICA8IiIiIgAAGkFAAAgACAgICAgAABqBQAAIAAg"+
	"ICAgICDAawUAAICAkKDgkIgAAGwFAAAgICAgICAwAABtBQAAAADwqKioqAAAbgUAAAAAsMiIiIgAAG8FAAAAAHCIiIhwA"+
	"ABwBQAAAADwiIiI8ICAcQUAAAAAeIiIiHgICHIFAAAAALDIgICAAABzBQAAAAB4gHAI8AAAdAUAACAgeCAgIBgAAHUFAA"+
	"AAAIiIiJhoAAB2BQAAAACIiFBQIAAAdwUAAAAAqKioqFAAAHgFAAAAAIhQIFCIAAB5BQAAAACIiIiIeAhwegUAAAAA+BA"+
	"gQPgAAHsFABggICDAICAgGAB8BSAgICAgICAgICAgfQUAwCAgIBggICDAAH4FAABosAAAAAAAAAB/BQAAAAAAAAAAqAAA"+
	"gAVAIHCIiPiIiIgAAIEFECBwiIj4iIiIAACCBSBQcIiI+IiIiAAAgwUoUHCIiPiIiIgAAIQFUABwiIj4iIiIAACFBSBQI"+
	"FCI+IiIiAAAhgUAADhQUPiQkJgAAIcFAABwiICAgIhwIGCIBUAg+ICA8ICA+AAAiQUQIPiAgPCAgPgAAIoFIFD4gIDwgI"+
	"D4AACLBVAA+ICA8ICA+AAAjAVAIAAgICAgICAAAI0FECAAICAgICAgAACOBSBQACAgICAgIAAAjwVQACAgICAgICAAAJA"+
	"FAADwiIjoiIjwAACRBShQiMiomIiIiAAAkgVAIHCIiIiIiHAAAJMFECBwiIiIiIhwAACUBSBQcIiIiIiIcAAAlQUoUHCI"+
	"iIiIiHAAAJYFUABwiIiIiIhwAACXBQAAaJCoqKhIsAAAmAVAIIiIiIiIiHAAAJkFECCIiIiIiIhwAACaBSBQAIiIiIiIc"+
	"AAAmwVQAIiIiIiIiHAAAJwFECCIiIhQICAgAACdBQAAgPCIiPCAgAAAngUAAOCQoJCIiLAAAJ8FAEAgAHiIiJhoAACgBQ"+
	"AQIAB4iIiYaAAAoQUAIFAAeIiImGgAAKIFAChQAHiIiJhoAACjBQAAUAB4iIiYaAAApAUAIFAgeIiImGgAAKUFAAAAAHC"+
	"ouKBYAACmBQAAAABwiICAeCBgpwUAQCAAcIj4gHgAAKgFABAgAHCI+IB4AACpBQAgUABwiPiAeAAAqgUAAFAAcIj4gHgA"+
	"AKsFAEAgACAgICAgAACsBQAQIAAgICAgIAAArQUAIFAAICAgICAAAK4FAABQAGAgICAgAACvBQBoECh4iIiIcAAAsAUAK"+
	"FAAsMiIiIgAALEFAEAgAHCIiIhwAACyBQAQIABwiIiIcAAAswUAIFAAcIiIiHAAALQFAChQAHCIiIhwAAC1BQAAUABwiI"+
	"iIcAAAtgUAAABokKioSLAAALcFAEAgAIiIiJhoAAC4BQAQIACIiIiYaAAAuQUAIFAAiIiImGgAALoFAABQAIiIiJhoAAC"+
	"7BQAQIACIiIiIeAhwvAUAAICwyIjIsIAAAL0FAABQAIiIiIh4CHC+BXAAcIiI+IiIiAAAvwUAAHAAeIiImGgAAMAFUCBw"+
	"iIj4iIiIAADBBQBQIAB4iIiYaAAAwgUAAHCIiPiIiIgQCMMFAAAAAHiIiJhoEAjEBRAgcIiAgICIcAAAxQUAABAgcIiAg"+
	"HgAAMYFcAD4gIDwgID4AADHBQAAcABwiPiAeAAAyAUAAPiAgPCAgPAQCMkFAAAAAHCI+IBwEAjKBXAAICAgICAgIAAAyw"+
	"UAAHAAYCAgICAAAMwFAAAAAGAgICAgAADNBQAAQFBgQMBAeAAAzgUAACAgKDBgoDAAAM8FECCIyKiYiIiIAADQBQAQIAC"+
	"wyIiIiAAA0QVwAHCIiIiIiHAAANIFAABwAHCIiIhwAADTBUiQAHCIiIiIcAAA1AUASJAAcIiIiHAAANUFAAB4oKCwoKB4"+
	"AADWBQAAAABQqLigWAAA1wUQIHCIgHAIiHAAANgFABAgAHiAcAjwAADZBVAgcIiAcAiIcAAA2gUAUCAAeIBwCPAAANsFc"+
	"ACIiIiIiIhwAADcBQAAcACIiIiYaAAA3QVIkACIiIiIiHAAAN4FAEiQAIiIiJhoAADfBVAAiIiIUCAgIAAA4AUQIPgIEC"+
	"BAgPgAAOEFABAgAPgQIED4AADiBSAA+AgQIECA+AAA4wUAACAA+BAgQPgAAOQFUCD4CBAgQID4AADlBQBQIAD4ECBA+AA"+
	"A5gUAcIiAcAiIcAAgIOcFAAAAeIBwCPAAICDoBQAA+CAgICAgACAg6QUAACAgeCAgGAAgIOoFAAD4iJCgkIiwAADrBQAA"+
	"ACAAICAgICAA7AUAAAAgACBAgIhwAO0FAAAAKFCgUCgAAADuBQAAAKBQKFCgAAAA7wUAADBI4EDgSDAAAPAFADBISDAAA"+
	"AAAAAD/BQAA+IiIiIiI+AAA",
}

COLORS=[
	0xFFFFFFFF,0xFFFFFF00,0xFFFF6500,0xFFDC0000,0xFFFF0097,0xFF360097,0xFF0000CA,0xFF0097FF,
	0xFF00A800,0xFF006500,0xFF653600,0xFF976536,0xFFB9B9B9,0xFF868686,0xFF454545,0xFF000000,
]
DEFAULT_COLORS=COLORS.slice(0)
BRUSHES=[
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
]
ALIGN={left:0,center:1,right:2}
DEFAULT_ANIMS='[[13,9,5,1,5,9],[4,4,8,14,14,8],[18,18,20,19,19,20],[0,0,0,0,1,1,1,1]]'
DEFAULT_PATTERNS=
	'%%IMG0AAgA4AAAAAAAAAAA//////////+AgID/CAgI/yBAgMEiHAgQgAAIAIAACAD/d//d/3f/3XEiF49HInT4iFAgAgW'+
	'IiIiIACIAiAAiAHfdd9133XfdQIAACAQCACABAQOESDAMAogiiCKIIogiqlWqVapVqlWABEAIASACEMAMjbEwAxvYqgCq'+
	'AKoAqgD/Vf9V/1X/Vf8A/wD/AP8AqqqqqqqqqqpEiBEiRIgRIt27d+7du3fuQIABAgQIECC/f/79+/fv3wgAqgAIAIgAj'+
	'493mPj4d4mqAIgUIkGIALCwsL8Av7+w'

array_is      =x=>lii(x)&&x.n=='array'
image_is      =x=>lii(x)&&x.n=='image'
sound_is      =x=>lii(x)&&x.n=='sound'
font_is       =x=>lii(x)&&x.n=='font'
button_is     =x=>lii(x)&&x.n=='button'
field_is      =x=>lii(x)&&x.n=='field'
grid_is       =x=>lii(x)&&x.n=='grid'
slider_is     =x=>lii(x)&&x.n=='slider'
canvas_is     =x=>lii(x)&&x.n=='canvas'
contraption_is=x=>lii(x)&&x.n=='contraption'
proxy_is      =x=>lii(x)&&x.n=='proxy'
prototype_is  =x=>lii(x)&&x.n=='prototype'
deck_is       =x=>lii(x)&&x.n=='deck'
card_is       =x=>lii(x)&&x.n=='card'
patterns_is   =x=>lii(x)&&x.n=='patterns'
module_is     =x=>lii(x)&&x.n=='module'
widget_is     =x=>lii(x)&&({button:1,field:1,grid:1,slider:1,canvas:1,contraption:1,proxy:1})[x.n]
ikey  =(x,k)=>lis(x)&&x.v==k
ivalue=(x,k,d)=>x.hasOwnProperty(k)?x[k]:d
ifield=(x,k)  =>x.f(x,lms(k))
iindex=(x,k,v)=>x.f(x,lmn(k),v)
iwrite=(x,k,v)=>x.f(x,k,v)
value_inherit=(self,key)=>{
	let r=self[key];if(typeof r=='string')r=lms(r);if(typeof r=='number'||typeof r=='boolean')r=lmn(r);
	const card=self.card;if(!contraption_is(card))return r
	const p=dget(card.def.widgets,ifield(self,'name'));if(!p)return r
	const v=ifield(p,key);if(r&&v&&match(r,v))delete self[key];return r||v
}
init_field=(dst,key,src)=>{const k=lms(key),v=dget(src,k);if(v)iwrite(dst,k,v)}
normalize_enum=(x,v)=>x.hasOwnProperty(v)?v:Object.keys(x)[0]
normalize_font=(x,v)=>ls(dkey(x,v)||x.k[dkix(x,v)]||lms('body'))
data_enc=x=>x[5]==undefined?-1:+x[5]
data_read=(type,x)=>(x.slice(0,2)!='%%'||x.slice(2,5)!=type)?null:new Uint8Array(atob(x.slice(6)).split('').map(x=>x.charCodeAt(0)))
data_write=(type,x)=>`%%${type}${btoa(Array.from(x).map(x=>String.fromCharCode(x)).join(''))}`
is_rooted=x=>card_is(x)?!x.dead: widget_is(x)?(is_rooted(x.card)&&!x.dead): 1

ceil=Math.ceil, clamp=(a,x,b)=>x<a?a:x>b?b:x, sign=x=>x>0?1:-1
last=x=>x[x.length-1]
rect=(x,y,w,h)=>({x:x||0,y:y||0,w:w||0,h:h||0})
rpair=(a,b)=>rect(a.x,a.y,b.x,b.y)
rcopy=r=>rect(r.x,r.y,r.w,r.h)
rint=r=>rect(0|r.x,0|r.y,0|r.w,0|r.h)
radd=(a,b)=>rect(a.x+b.x,a.y+b.y,a.w+b.w,a.h+b.h)
rsub=(a,b)=>rect(a.x-b.x,a.y-b.y,a.w-b.w,a.h-b.h)
rmul=(a,n)=>rect(a.x*n,a.y*n,a.w*n,a.h*n)
rdiv=(a,n)=>rint(rmul(a,1/n))
inset=(r,n)=>rect(r.x+n,r.y+n,r.w-2*n,r.h-2*n)
rin=(r,p)=>p.x>=r.x&&p.y>=r.y&&p.x<r.x+r.w&&p.y<r.y+r.h           // point-in-rect
ron=(a,b)=>b.x+b.w>=a.x&&b.x<=a.x+a.w&&b.y+b.h>=a.y&&b.y<=a.y+a.h // rect-overlaps-rect
requ=(a,b)=>a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h
rmax=(a,b)=>rect(max(a.x,b.x),max(a.y,b.y),max(a.w,b.w),max(a.h,b.h))
rmin=(a,b)=>rect(min(a.x,b.x),min(a.y,b.y),min(a.w,b.w),min(a.h,b.h))
rclip=(a,b)=>{const c=rmax(a,b);return rect(c.x,c.y,min(a.x+a.w,b.x+b.w)-c.x,min(a.y+a.h,b.y+b.h)-c.y)}
runion=(a,b)=>{const c=rmin(a,b);return rect(c.x,c.y,max(a.x+a.w,b.x+b.w)-c.x,max(a.y+a.h,b.y+b.h)-c.y)}
rcenter=(a,b)=>rint(rect(a.x+(a.w-b.x)/2,ceil(a.y+(a.h-b.y)/2.0),b.x,b.y))
rnorm=r=>{r=rcopy(r);if(r.w<0)r.w*=-1,r.x-=r.w;if(r.h<0)r.h*=-1,r.y-=r.h;return rint(r)}
rclamp=(a,b,c)=>rmin(rmax(a,b),c)
lmpair=r=>lml([lmn(r.x),lmn(r.y)])
lmrect=r=>lml([lmn(r.x),lmn(r.y),lmn(r.w),lmn(r.h)])
getpair=x=>(!x||!lil(x))?rect(): rect(x.v.length>0?ln(x.v[0]):0, x.v.length>1?ln(x.v[1]):0)
getrect=x=>(!x||!lil(x))?rect(): rect(x.v.length>0?ln(x.v[0]):0, x.v.length>1?ln(x.v[1]):0, x.v.length>2?ln(x.v[2]):0, x.v.length>3?ln(x.v[3]):0)
getimage=x=>(!x||!image_is(x))? image_make(rect()): x
ukey=(dict,name,root,original)=>{
	if(original&&match(name,original))return name
	if(name&&lis(name)&&!dget(dict,name))return name
	let i=1;while(1){let n=lms(root+(i++));if(!dget(dict,n))return n}
}
uset=(dict,name,root,x)=>(dset(dict,ukey(dict,name,root),x),x)
reorder=(dict,a,b)=>{
	b=clamp(0,b,count(dict)-1);const k=dict.k[a],v=dict.v[a]
	if(b<a){for(let z=a;z>b;z--)dict.k[z]=dict.k[z-1],dict.v[z]=dict.v[z-1]}
	else   {for(let z=a;z<b;z++)dict.k[z]=dict.k[z+1],dict.v[z]=dict.v[z+1]}
	dict.k[b]=k,dict.v[b]=v
}
anchors={top_left:0,top_center:1,top_right:2,center_left:3,center:4,center_right:5,bottom_left:6,bottom_center:7,bottom_right:8}
anchor=(r,a)=>{
	if(a==undefined)return rint(r);a=anchors[ls(a)]||0
	if(a==1||a==4||a==7)r.x-=r.w/2; if(a==2||a==5||a==8)r.x-=r.w
	if(a==3||a==4||a==5)r.y-=r.h/2; if(a==6||a==7||a==8)r.y-=r.h
	return rint(r)
}
unpack_rect=(z,size)=>{
	let s=size||frame.image.size, v=rect(0,0,s.x,s.y)
	if(z.length>=1){const a=rint(getpair(z[0])),b=rint(getpair(z[1]));if(b.x<0)a.x+=1+b.x,b.x*=-1;if(b.y<0)a.y+=1+b.y,b.y*=-1;v=rpair(a,b)}
	return anchor(v,z[2])
}
unpack_poly=z=>{
	const r=[];z.map(x=>{if(lil(x)&&x.v.every(x=>!lin(x))){ll(x).map(x=>r.push(getpair(x)))}else{r.push(getpair(x))}})
	if(r.length==1)r.push(r[0]);return r
}
readcolor=(cr,cg,cb,grayscale)=>{
	if(grayscale){
		// perceptually weighted gray: http://entropymine.com/imageworsener/grayscale/
		const rf=0.2126*Math.pow(cr,2.2), gf=0.7152*Math.pow(cg,2.2), bf=0.0722*Math.pow(cb,2.2), gg=Math.pow(rf+gf+bf,1/2.2)
		return clamp(0,0|gg,255)
	}
	let ci=0,cd=1e20;for(let c=0;c<16;c++){
		const dr=abs(((COLORS[c]>>16)&0xFF)/256.0-cr/256.0),
			  dg=abs(((COLORS[c]>> 8)&0xFF)/256.0-cg/256.0),
			  db=abs(((COLORS[c]    )&0xFF)/256.0-cb/256.0),
			  diff=(dr*dr)+(dg*dg)+(db*db)
		if(diff<cd)ci=c,cd=diff
	}if(ci==15)return 1;return ci+32
}

let audio_playing=0
interface_app=lmi((self,i,x)=>{
	if(x&&lis(i)){
		if(i.v=='fullscreen')return set_fullscreen(lb(x)),x
	}else if(lis(i)){
		if(i.v=='fullscreen')return lmn(is_fullscreen())
		if(i.v=='playing'   )return lmn(audio_playing)
		if(i.v=='save'      )return lmnat(_=>((modal_enter&&modal_enter('save_deck'),NIL)))
		if(i.v=='exit'      )return lmnat(_=>NIL) // does nothing in web-decker
		if(i.v=='show')return lmnat(z=>{
			const sp=x=>lis(x)?ls(x): lin(x)?ln(x): show(x)
			if(lit(z[0])){console.table(rows(z[0]).v.map(r=>r.k.reduce((a,k,i)=>{a[ls(k)]=sp(r.v[i]);return a},{})))}
			else{console.log(z.map(x=>show(x,z.length==1)).join(' '))}
			return z[0]||NIL
		})
		if(i.v=='print')return lmnat(z=>{
			const r=ls(z.length>1?dyad.format(z[0],lml(z.slice(1))):z[0]||NIL);console.log(r)
			return lms(r)
		})
		if(i.v=='render')return draw_con==undefined?NIL:lmnat(([a])=>{
			return widget_is(a)?draw_widget(a): card_is(a)?draw_con(a,1): image_make(rect())
		})
	}return x?x:NIL
},'app')

interface_bits=lmi((self,i,x)=>{
	const lbits=x=>0xFFFFFFFF&ln(x), cb=f=>lmnat(a=>{
		let l=a.length<2?ll(a[0]||NIL): a, r=l[0]||NIL
		for(let z=1;z<l.length;z++)r=vd(f)(r,l[z]);return r
	})
	if(ikey(i,'and'))return cb((x,y)=>lmn((lbits(x)&lbits(y))>>>0))
	if(ikey(i,'or' ))return cb((x,y)=>lmn((lbits(x)|lbits(y))>>>0))
	if(ikey(i,'xor'))return cb((x,y)=>lmn((lbits(x)^lbits(y))>>>0))
	return x?x:NIL
},'bits')

let frame=null
inclip=p=>rin(frame.clip,p)
gpix=p=>frame.image.pix[p.x+p.y*frame.image.size.x]
pix=(p,v)=>frame.image.pix[p.x+p.y*frame.image.size.x]=v
pal_pat=(pal,p,x,y)=>pal[(x%8)+(8*(y%8))+(8*8*p)]
draw_pattern=(pal,pix,pos)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,pos.x,pos.y)&1
draw_hline=(x0,x1,y,pattern)=>{
	if(y<frame.clip.y||y>=frame.clip.y+frame.clip.h)return
	x0=max(frame.clip.x,x0),x1=min(frame.clip.x+frame.clip.w,x1);for(let z=x0;z<x1;z++)pix(rect(z,y),pattern)
}
draw_vline=(x,y0,y1,pattern)=>{
	if(x<frame.clip.x||x>=frame.clip.x+frame.clip.w)return
	y0=max(frame.clip.y,y0),y1=min(frame.clip.y+frame.clip.h,y1);for(let z=y0;z<y1;z++)pix(rect(x,z),pattern)
}
draw_rect=(r,pattern)=>{r=rclip(r,frame.clip);for(let a=r.y;a<r.y+r.h;a++)for(let b=r.x;b<r.x+r.w;b++)pix(rect(b,a),pattern)}
draw_invert_raw=(pal,r)=>{r=rclip(r,frame.clip);for(let a=r.y;a<r.y+r.h;a++)for(let b=r.x;b<r.x+r.w;b++){const h=rect(b,a);pix(h,1^draw_pattern(pal,gpix(h),h))}}
draw_icon=(p,i,pattern)=>{const s=i.size;for(let a=0;a<s.y;a++)for(let b=0;b<s.x;b++){const h=rect(p.x+b,p.y+a);if(i.pix[b+(a*s.x)]&&inclip(h))pix(h,pattern)}}
draw_iconc=(r,i,pattern)=>draw_icon(rcenter(r,i.size),i,pattern)

draw_line_simple=(r,brush,pattern)=>{
	r=rint(r);const bsh=(z,x,y)=>(BRUSHES[(z*8)+y]>>(7-x))&1
	let dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1;while(1){
		if(brush==0){if(inclip(r))pix(r,pattern)}
		else{for(let b=0;b<8;b++)for(let a=0;a<8;a++){const h=rect(r.x+a-3,r.y+b-3);if(bsh(brush,a,b)&&inclip(h))pix(h,pattern)}}
		if(r.x==r.w&&r.y==r.h)break;let e2=err*2; if(e2>=dy)err+=dy,r.x+=sx; if(e2<=dx)err+=dx,r.y+=sy
	}
}
draw_line_custom=(r,mask,pattern)=>{
	let dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1, ms=mask.size, mc=rint(rdiv(ms,2));while(1){
		for(let b=0;b<ms.y;b++)for(let a=0;a<ms.x;a++){const h=rect(r.x+a-mc.x,r.y+b-mc.y), mp=mask.pix[a+b*ms.x];if(mp&&inclip(h))pix(h,mp==1?pattern: mp==47?1: mp)}
		if(r.x==r.w&&r.y==r.h)break;let e2=err*2; if(e2>=dy)err+=dy,r.x+=sx; if(e2<=dx)err+=dx,r.y+=sy
	}
}
draw_line_function=(r,func,pattern)=>{
	const a=lml([lmpair(rect(r.w-r.x,r.h-r.y)),ONE]),p=lmblk(),e=lmenv();blk_lit(p,func),blk_lit(p,a),blk_op(p,op.CALL),pushstate(e)
	let dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1;while(!do_panic){
		state.e=[e],state.t=[],state.pcs=[];issue(e,p);let quota=BRUSH_QUOTA;while(quota&&running())runop(),quota--;const v=running()?ZERO:arg()
		if(image_is(v)){
			const ms=v.size, mc=rint(rdiv(ms,2))
			for(let b=0;b<ms.y;b++)for(let a=0;a<ms.x;a++){const h=rect(r.x+a-mc.x,r.y+b-mc.y), mp=v.pix[a+b*ms.x];if(mp&&inclip(h))pix(h,mp==1?pattern: mp==47?1: mp)}
		}if(r.x==r.w&&r.y==r.h)break;let e2=err*2; if(e2>=dy)err+=dy,r.x+=sx; if(e2<=dx)err+=dx,r.y+=sy; a.v[1]=ZERO
	}popstate()
}
draw_line=(r,brush,pattern,deck)=>{
	if(brush>=0&&brush<=23){draw_line_simple(r,brush,pattern);return}
	const b=deck.brushes;if(brush<0||brush-24>=b.v.length)return;const f=b.v[brush-24]
	if(image_is(f)){draw_line_custom(r,f,pattern)}else if(lion(f)){draw_line_function(r,f,pattern)}
}
n_brush=(z,deck)=>{
	const b=deck.brushes,bt=deck.brusht,f=z[0],s=z[1]
	if(lion(f)){
		const k=lms(f.n),v=image_make(rect(64,32)),t=frame; dset(b,k,f),frame=({size:v.size,clip:rect(0,0,64,32),image:v})
		draw_line(rect(16,16,32,16),24+dkix(b,k),1,deck)
		draw_line(rect(32,16,40,16),24+dkix(b,k),1,deck)
		draw_line(rect(40,16,44,16),24+dkix(b,k),1,deck)
		draw_line(rect(44,16,48,16),24+dkix(b,k),1,deck)
		frame=t,dset(bt,lms(f.n),v)
	}
	if(lis(f)&&s&&image_is(s))dset(b,f,s),dset(bt,f,s)
	return b
}

draw_box=(r,brush,pattern)=>{
	const size=frame.image.size
	if(r.w==0||r.h==0||!ron(r,rect(0,0,size.x,size.y)))return
	if(r.y         >=0)draw_line_simple(rect(r.x      ,r.y      ,r.x+r.w-1,r.y      ),brush,pattern)
	if(r.y+r.h<=size.y)draw_line_simple(rect(r.x      ,r.y+r.h-1,r.x+r.w-1,r.y+r.h-1),brush,pattern)
	if(r.x         >=0)draw_line_simple(rect(r.x      ,r.y      ,r.x      ,r.y+r.h-1),brush,pattern)
	if(r.x+r.w<=size.x)draw_line_simple(rect(r.x+r.w-1,r.y      ,r.x+r.w-1,r.y+r.h-1),brush,pattern)
}
draw_boxf=(r,brush,pattern,deck)=>{
	const size=frame.image.size
	if(r.w==0||r.h==0||!ron(r,rect(0,0,size.x,size.y)))return
	if(r.y         >=0)draw_line(rect(r.x      ,r.y      ,r.x+r.w-1,r.y      ),brush,pattern,deck)
	if(r.y+r.h<=size.y)draw_line(rect(r.x      ,r.y+r.h-1,r.x+r.w-1,r.y+r.h-1),brush,pattern,deck)
	if(r.x         >=0)draw_line(rect(r.x      ,r.y      ,r.x      ,r.y+r.h-1),brush,pattern,deck)
	if(r.x+r.w<=size.x)draw_line(rect(r.x+r.w-1,r.y      ,r.x+r.w-1,r.y+r.h-1),brush,pattern,deck)
}
draw_lines=(poly,brush,pattern,deck)=>{for(let z=0;z<poly.length-1;z++)draw_line(rpair(poly[z],poly[z+1]),brush,pattern,deck)}
poly_bounds=poly=>{
	const d=rect(frame.clip.x+frame.clip.w,frame.clip.y+frame.clip.h,frame.clip.x,frame.clip.y)
	for(let z=0;z<poly.length;z++){const p=poly[z];d.x=min(d.x,p.x),d.y=min(d.y,p.y),d.w=max(d.w,p.x),d.h=max(d.h,p.y)}
	d.w-=d.x,d.h-=d.y,d.w++,d.h++;return d
}
poly_in=(poly,pos)=>{
	let r=0;for(let i=0,j=poly.length-1;i<poly.length;i++){
		if(pos.x==poly[i].x&&pos.y==poly[i].y)return 1
		if(((poly[i].y>=pos.y)!=(poly[j].y>=pos.y))&&(pos.x<=(poly[j].x-poly[i].x)*(pos.y-poly[i].y)/(poly[j].y-poly[i].y)+poly[i].x))r^=1
		j=i
	}return r
}
draw_poly=(poly,pattern)=>{
	const r=rint(rclip(frame.clip,poly_bounds(poly)))
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){const h=rect(b+r.x,a+r.y);if(poly_in(poly,h))pix(h,pattern)}
}
draw_fill=(r,pattern,ref)=>{
	if(!inclip(r))return
	const src=ref||frame.image, visited=new Uint8Array(src.size.x*src.size.y)
	const spix=p=>src.pix[p.x+p.y*src.size.x], source=spix(r), fringe=[r]
	const OFFSETS=[rect(-1,0),rect(0,-1),rect(1,0),rect(0,1)], stride=frame.image.size.x
	while(fringe.length){
		const here=fringe.pop();if(gpix(here)==pattern)continue;pix(here,pattern)
		for(let z=0;z<4;z++){
			const there=radd(here,OFFSETS[z]),ti=there.x+there.y*stride
			if(inclip(there)&&!visited[ti]&&spix(there)==source)fringe.push(there),visited[ti]=1
		}
	}
}
draw_char=(pos,font,c,pattern)=>{
	const iw=font_w(font),ih=font_h(font);
	for(let a=0;a<ih;a++)for(let b=0;b<iw;b++)if(font_gpix(font,c,b,a)){
		const h=rect(pos.x+b,pos.y+a);if(inclip(h))pix(h,pattern)
	}
}
draw_text=(pos,text,font,pattern)=>{
	const cursor=rect(pos.x,pos.y);for(let z=0;z<text.length;z++){
		const c=text[z];
		if(c!='\n'){draw_char(cursor,font,c,pattern),cursor.x+=font_gw(font,c)+font_sw(font)}
		else{cursor.x=pos.x,cursor.y+=font_h(font)}
	}
}
const ELLIPSIS='…'
layout_plaintext=(text,font,align,mx)=>{
	let layout=[],lines=[],cursor=rect(0,0), lnl=_=>(cursor.x=0,cursor.y+=1), ws=x=>x=='\n'||x==' '
	const fh=font_h(font),fs=font_sw(font)
	for(let z=0;z<text.length;z++){
		let a=z,w=text[z]=='\n'?0:(font_gw(font,text[z])+fs)
		if(!ws(text[z]))while(text[z+1]&&!ws(text[z+1]))w+=font_gw(font,text[++z])+fs // find word
		if(cursor.x+w>=mx.x&&cursor.x>0)lnl() // word won't fit this line
		for(let i=a;i<=z;i++){                 // append word to line
			const c=text[i], size=rect(c=='\n'?0:font_gw(font,c)+fs,fh)
			if(c==' '&&cursor.x==0&&layout.length>0&&!ws(last(layout).char))size.x=0 // squish lead space after a soft-wrap
			if(cursor.x+size.x>=mx.x)lnl() // hard-break overlong words
			layout.push({pos:rpair(cursor,size),line:cursor.y,char:c,font,arg:NIL})
			if(c=='\n'){lnl()}else{cursor.x+=size.x}
			if(cursor.y>=(mx.y/fh)){
				layout=layout.slice(0,max(1,layout.length-3))
				layout[layout.length-1].c=ELLIPSIS,layout[layout.length-1].pos.w=font_gw(font,ELLIPSIS)+fs
				z=text.length-1;break
			}
		}
	}
	let y=0;for(let i=0,line=0;i<layout.length;i++,line++){
		let a=i;while(i<(layout.length-1)&&(layout[i+1].pos.y==line))i++          // find bounds of line
		let h=0;for(let z=a;z<=i;z++)h=max(h,layout[z].pos.h)                     // find height of line
		let w=(a&&a==i)?0:(layout[i].pos.x+layout[i].pos.w)                       // find width of line
		let x=align==ALIGN.center?0|((mx.x-w)/2): align==ALIGN.right?(mx.x-w): 0  // justify
		lines.push({pos:rect(x,y,w,h),range:rect(a,i)})
		for(let z=a;z<=i;z++){const g=layout[z].pos;g.y=y+0|((h-g.h)/2);g.x+=x;}if(i<layout.length-1)y+=h
	}return {size:rect(mx.x,y+fh),layout,lines}
}
layout_richtext=(deck,table,font,align,width)=>{
	const layout=[],lines=[],cursor=rect(0,0), lnl=_=>(cursor.x=0,cursor.y+=1), ws=x=>x=='\n'||x==' '
	const texts=tab_get(table,'text'), fonts=tab_get(table,'font'), args=tab_get(table,'arg')
	for(let chunk=0;chunk<texts.length;chunk++){
		const f=dget(deck.fonts,fonts[chunk])||font, fh=font_h(f), fs=font_sw(f)
		if(image_is(args[chunk])){
			const size=args[chunk].size;if(cursor.x+size.x>=width&&cursor.x>0)lnl()
			layout.push({pos:rpair(cursor,size),line:cursor.y,char:'i',font:f,arg:args[chunk]})
			cursor.x+=size.x;continue
		}
		const t=ls(texts[chunk]);for(let z=0;z<t.length;z++){
			let a=z, w=t[z]=='\n'?0:(font_gw(f,t[z])+fs)
			if(!ws(t[z]))while(z+1<t.length&&!ws(t[z+1]))w+=font_gw(f,t[++z])+fs
			if(cursor.x+w>=width&&cursor.x>0)lnl()
			for(let i=a;i<=z;i++){
				const c=t[i], size=rect(c=='\n'?0:font_gw(f,c)+fs,fh)
				if(c==' '&&cursor.x==0&&layout.length>0&&!ws(last(layout).char))size.x=0
				if(cursor.x+size.x>=width)lnl()
				layout.push({pos:rpair(cursor,size),line:cursor.y,char:c,font:f,arg:args[chunk]})
				if(c=='\n'){lnl()}else{cursor.x+=size.x}
			}
		}
	}
	let y=0;for(let i=0,line=0;i<layout.length;i++,line++){
		let a=i;while(i<(layout.length-1)&&(layout[i+1].pos.y==line))i++            // find bounds of line
		let h=0;for(let z=a;z<=i;z++)h=max(h,layout[z].pos.h)                       // find height of line
		let w=(a&&a==i)?0:(layout[i].pos.x+layout[i].pos.w)                         // find width of line
		let x=align==ALIGN.center?0|((width-w)/2): align==ALIGN.right?(width-w): 0  // justify
		lines.push({pos:rect(x,y,w,h),range:rect(a,i)})
		for(let z=a;z<=i;z++){const g=layout[z].pos;g.y=y+0|((h-g.h)/2);g.x+=x;}y+=h
	}return {size:rect(width,y),layout,lines}
}
draw_text_wrap=(r,l,pattern)=>{
	r=rint(r);const oc=frame.clip;frame.clip=r;for(let z=0;z<l.layout.length;z++){
		const g=l.layout[z];if(g.pos.w<1)continue
		draw_char(radd(g.pos,r),g.font,g.char,pattern)
	}frame.clip=oc;
}
draw_text_rich=(r,l,pattern,opaque)=>{
	const oc=frame.clip;frame.clip=r;for(let z=0;z<l.layout.length;z++){
		const g=l.layout[z];if(g.pos.w<1)continue
		if(g.pos.y+g.pos.h<0||g.pos.y>r.h)continue; g.pos.x+=r.x, g.pos.y+=r.y
		if(lis(g.arg)&&count(g.arg))draw_hline(g.pos.x,g.pos.x+g.pos.w,g.pos.y+g.pos.h-1,19)
		if(image_is(g.arg)){image_paste(g.pos,frame.clip,g.arg,frame.image,opaque)}
		else{draw_char(g.pos,g.font,g.char,pattern)}
	}frame.clip=oc
}
draw_9seg=(r,dst,src,m,clip,opaque,pal)=>{
	const o=rect(r.x,r.y), s=src.size, ss=s, ds=dst.size; if(s.x<1||s.y<1)return
	const draw_wrapped=(o,sr)=>{
		const r=rclip(o,clip),d=rsub(r,o);sr=rclip(sr,rpair(rect(0,0),ss));if(r.w<=0||r.h<=0||sr.w<=0||sr.h<=0)return
		if(!pal){ // solid/opaque
			for(let y=0;y<r.h;y++)for(let x=0;x<r.w;x++){const c=src.pix[(sr.x+((x+d.x)%sr.w))+(sr.y+((y+d.y)%sr.h))*ss.x];if(opaque||c)dst.pix[(r.x+x)+(r.y+y)*ds.x]=c}
		}
		else{ // invert
			const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
			for(let y=0;y<r.h;y++)for(let x=0;x<r.w;x++){
				let dx=r.x+x,dy=r.y+y, c=draw_pattern(src.pix[(sr.x+((x+d.x)%sr.w))+(sr.y+((y+d.y)%sr.h))*ss.x],dx,dy), di=(r.x+x)+(r.y+y)*ds.x
				dst.pix[di]=c^draw_pattern(dst.pix[di],dx,dy)
			}
		}
	}
	draw_wrapped(radd(rect(0      ,0      ,m.x          ,m.y          ),o),rect(0      ,0      ,m.x          ,m.y          )) // NW
	draw_wrapped(radd(rect(0      ,r.h-m.h,m.x          ,m.h          ),o),rect(0      ,s.y-m.h,m.x          ,m.h          )) // SW
	draw_wrapped(radd(rect(r.w-m.w,0      ,m.w          ,m.y          ),o),rect(s.x-m.w,0      ,m.w          ,m.y          )) // NE
	draw_wrapped(radd(rect(r.w-m.w,r.h-m.h,m.w          ,m.h          ),o),rect(s.x-m.w,s.y-m.h,m.w          ,m.h          )) // SE
	draw_wrapped(radd(rect(0      ,m.y    ,m.x          ,r.h-(m.y+m.h)),o),rect(0      ,m.y    ,m.x          ,s.y-(m.y+m.h))) // W
	draw_wrapped(radd(rect(m.x    ,0      ,r.w-(m.x+m.w),m.y          ),o),rect(m.x    ,0      ,s.x-(m.x+m.w),m.y          )) // N
	draw_wrapped(radd(rect(r.w-m.w,m.y    ,m.w          ,r.h-(m.y+m.h)),o),rect(s.x-m.w,m.y    ,m.w          ,s.y-(m.y+m.h))) // E
	draw_wrapped(radd(rect(m.x    ,r.h-m.h,r.w-(m.x+m.w),m.h          ),o),rect(m.x    ,s.y-m.h,s.x-(m.x+m.w),m.h          )) // S
	draw_wrapped(radd(rect(m.x    ,m.y    ,r.w-(m.x+m.w),r.h-(m.y+m.h)),o),rect(m.x    ,m.y    ,s.x-(m.x+m.w),s.y-(m.y+m.h))) // C
}

pointer={f:(self,i,x)=>{
	if(ikey(i,'held' ))return lmn(self.held)
	if(ikey(i,'down' ))return lmn(self.down)
	if(ikey(i,'up'   ))return lmn(self.up)
	if(ikey(i,'pos'  ))return lmpair(self.pos)
	if(ikey(i,'start'))return lmpair(self.start)
	if(ikey(i,'prev' ))return lmpair(self.prev)
	if(ikey(i,'end'  ))return lmpair(self.end)
	return x?x:NIL
},t:'int',n:'pointer',held:0,down:0,up:0,pos:rect(),start:rect(),prev:rect(),end:rect()}

keystore_read=x=>{
	let store=lmd();if(x)x.k.filter((k,i)=>!linil(x.v[i])).map((k,i)=>dset(store,k,x.v[i]))
	return {f:(self,i,x)=>{
		i=ls(i);if(i=='keys')return monad.keys(self.data)
		if(x){
			const f=lms('%J'),val=dyad.parse(f,dyad.format(f,x))
			if(linil(val)){self.data=dyad.drop(lms(i),self.data)}else{dset(self.data,lms(i),val)}
			return x
		}else{return dget(self.data,lms(i))||NIL}
	},t:'int',n:'keystore',data:store}
}

module_read=(x,deck)=>{
	const ri=lmi((self,i,x)=>{
		if(x){
			if(ikey(i,'description'))return self.description=ls(x),x
			if(ikey(i,'version'))return self.version=ln(x),x
			if(ikey(i,'name')){
				if(ls(x)=='')return x
				self.name=ls(ukey(self.deck.modules,lms(ls(x)),ls(x),lms(self.name)))
				self.deck.modules.k[dvix(self.deck.modules,self)]=self.name
				return x
			}
			if(ikey(i,'script')){
				self.script=ls(x),self.error='',self.value=lmd();try{
					const prog=parse(self.script),root=lmenv();primitives(root,deck),constants(root),root.local('data',self.data)
					pushstate(root),issue(root,prog);let q=MODULE_QUOTA;while(running()&&q>0)runop(),q--
					if(running()){self.error='initialization took too long.'}
					else{self.value=ld(arg())};popstate()
				}catch(e){self.error=e.x;return x}
			}
		}else{
			if(ikey(i,'name'       ))return lms(self.name)
			if(ikey(i,'data'       ))return self.data
			if(ikey(i,'description'))return lms(self.description||'')
			if(ikey(i,'version'    ))return lmn(self.version||0.0)
			if(ikey(i,'script'     ))return lms(self.script||'')
			if(ikey(i,'error'      ))return lms(self.error||'')
			if(ikey(i,'value'      ))return self.value
		}return x?x:NIL
	},'module')
	const n=dget(x,lms('name'))
	ri.deck=deck
	ri.name=ls(ukey(deck.modules,n&&lis(n)&&count(n)==0?null:n,'module'))
	ri.data=keystore_read(dget(x,lms('data')))
	ri.value=lmd()
	init_field(ri,'description',x)
	init_field(ri,'version',x)
	init_field(ri,'script',x)
	return ri
}
module_write=x=>{
	const r=lmd()
	dset(r,lms('name'  ),ifield(x,'name'))
	dset(r,lms('data'  ),x.data.data)
	dset(r,lms('script'),ifield(x,'script'))
	if(x.description)dset(r,lms('description'),ifield(x,'description'))
	if(x.version)dset(r,lms('version'),ifield(x,'version'))
	return r
}

const casts={u8:1,i8:1,u16b:2,u16l:2,i16b:2,i16l:2,u32b:4,u32l:4,i32b:4,i32l:4,char:1}
array_write=x=>data_write('DAT'+String.fromCharCode(48+Object.keys(casts).indexOf(x.cast)),x.data.slice(x.base,x.base+x.size))
array_read=x=>{const f=x.charCodeAt(5),d=data_read('DAT',x);return d?array_make(d.length,Object.keys(casts)[clamp(0,f-48,10)],0,d):array_make(0,'u8',0)}
n_array=([x,y])=>{if(lis(x))return array_read(ls(x));const size=ln(x),cast=y?normalize_enum(casts,ls(y)):'u8';return array_make(size,cast,0)}
array_make=(size,cast,base,buffer)=>{
	const offset=x=>({offset:ln(lil(x)?monad.first(x):x),len:lil(x)?max(0,ln(monad.last(x))):-1})
	const shift=(a,here)=>({here:0, size:a.size-here, base:a.base+here, cast:a.cast, data:a.data})
	const resize=(a,size)=>{
		if(a.slice)return;size=max(0,size);const old=a.data;a.data=new Uint8Array(size)
		for(let z=0;z<a.data.length;z++)a.data[z]=z>=old.length?0:old[z];a.size=size
	}
	const get_raw=(a,index)=>{
		const step=casts[a.cast];if(index<0||index>=(0|(a.size/step)))return 0
		const ix=a.base+step*index;if(ix<0||ix+step>a.data.length)return 0
		const ur=(i,s)=>a.data[ix+i]<<s
		const s8 =x=>x<<24>>24
		const s16=x=>x<<16>>16
		const u32=(a,b,c,d)=>(2*a)+(b|c|d)
		if(a.cast=='u8'  )return     ur(0,0)
		if(a.cast=='i8'  )return  s8(ur(0,0))
		if(a.cast=='u16b')return    (ur(0,8)|ur(1,0))
		if(a.cast=='u16l')return    (ur(1,8)|ur(0,0))
		if(a.cast=='i16b')return s16(ur(0,8)|ur(1,0))
		if(a.cast=='i16l')return s16(ur(1,8)|ur(0,0))
		if(a.cast=='u32b')return u32(ur(0,23),ur(1,16),ur(2,8),ur(3,0))
		if(a.cast=='u32l')return u32(ur(3,23),ur(2,16),ur(1,8),ur(0,0))
		if(a.cast=='i32b')return    (ur(0,24)|ur(1,16)|ur(2,8)|ur(3,0))
		if(a.cast=='i32l')return    (ur(3,24)|ur(2,16)|ur(1,8)|ur(0,0))
		return drom_from_ord(ur(0,0))
	}
	const set_raw=(a,index,v)=>{
		if('string'==typeof v)v=drom_to_ord(v)
		const step=casts[a.cast];if(index<0||index>=(0|(a.size/step)))return
		const ix=a.base+step*index;if(ix<0||ix+step>a.data.length)return
		const uw=(i,s)=>a.data[ix+i]=v>>>s
		if     (a.cast=='u16b'||a.cast=='i16b')uw(0,8),uw(1,0)
		else if(a.cast=='u16l'||a.cast=='i16l')uw(1,8),uw(0,0)
		else if(a.cast=='u32b'||a.cast=='i32b')uw(0,24),uw(1,16),uw(2,8),uw(3,0)
		else if(a.cast=='u32l'||a.cast=='i32l')uw(3,24),uw(2,16),uw(1,8),uw(0,0)
		else uw(0,0)
	}
	const get=(a,index,len)=>{
		if(a.cast=='char'&&len<0)len=1
		if(a.cast=='char'){
			const t=a.cast;a.cast='u8';
			const r=range(len).map(x=>drom_from_ord(get_raw(a,index+x))).join('')
			return a.cast=t,lms(r)
		}
		return len<0?lmn(get_raw(a,index)): lml(range(len).map(x=>lmn(get_raw(a,index+x))))
	}
	const set=(a,index,len,v)=>{
		if(len<0)len=1
		if(array_is(v)){for(let z=0;z<len;z++)set_raw(a,index+z,get_raw(v,z))}             // array copy
		else if(lis(v)){for(let z=0;z<len;z++)set_raw(a,index+z,z>=count(v)?0:v.v[z])}     // copy chars up to len
		else if(lil(v)){for(let z=0;z<len;z++)set_raw(a,index+z,z>=count(v)?0:ln(v.v[z]))} // copy numbers up to len
		else{const vv=ln(v);for(let z=0;z<len;z++)set_raw(a,index+z,vv)}                   // spread a number up to len
	}
	const slice=(a,z)=>{
		const o=offset(z[0]||ZERO),cast=z[1]?normalize_enum(casts,ls(z[1])):a.cast, step=casts[cast];o.offset*=casts[a.cast]
		if(o.len<0)o.len=0|((a.size-o.offset)/step);const r=array_make(o.len,cast,o.offset,a.data);r.slice=1;return r
	}
	const copy=(a,z)=>{
		const o=offset(z[0]||ZERO),cast=z[1]?normalize_enum(casts,ls(z[1])):a.cast, step=casts[cast];o.offset*=casts[a.cast]
		if(o.len<0)o.len=0|((src.size-o.offset)/step);const r=array_make(o.len,cast,0)
		for(let z=0;z<o.len;z++)set_raw(r,z,get_raw(shift(a,o.offset),z));return r
	}
	const struct_size=shape=>{
		if( lis(shape))return (casts[ls(shape)]||1)
		if( lil(shape))return (casts[ls(monad.first(shape))]||1)*max(0,ln(monad.last(shape)))
		if(!lid(shape))return 0
		let bit=0, r=0;shape.v.map(type=>{
			if(!lin(type)&&bit)bit=0,r++
			if(lin(type)){bit+=clamp(1,ln(type),31),r+=0|(bit/8),bit%=8}else{r+=struct_size(type)}
		});return r
	}
	const struct_read=(a,shape)=>{
		if(lis(shape)){a.cast=normalize_enum(casts,ls(shape));const r=get(shift(a,a.here),0,-1);a.here+=casts[a.cast];return r}
		if(lil(shape)){
			const n=max(0,ln(monad.last(shape)))
			a.cast=normalize_enum(casts,ls(monad.first(shape)));const r=get(shift(a,a.here),0,n);a.here+=n*casts[a.cast];return r
		}
		if(!lid(shape))return ZERO;let bit=0,r=lmd();shape.v.map((type,i)=>{
			let v=ZERO;if(!lin(type)&&bit)bit=0,a.here++
			if(lin(type)){
				let n=clamp(1,ln(type),31),t=0;a.cast='u8'
				while(n>0){t=(t<<1)|(1&(get_raw(a,a.here)>>(7-bit))),bit++,n--;if(bit==8)bit=0,a.here++}v=lmn(t)
			}else{v=struct_read(a,type)}dset(r,shape.k[i],v)
		});return r
	}
	const struct_write=(a,shape,value)=>{
		if(lis(shape)||lil(shape)){
			let n=lis(shape)?1:max(0,ln(monad.last(shape)));a.cast=normalize_enum(casts,ls(lis(shape)?shape:monad.first(shape)))
			set(shift(a,a.here),0,n,value),a.here+=n*casts[a.cast];return
		}if(!lid(shape))return
		let bit=0;shape.v.map((type,i)=>{
			let v=dget(value,shape.k[i])||ZERO
			if(!lin(type)){if(bit)bit=0,a.here++;struct_write(a,type,v);return}
			let n=clamp(1,ln(type),31), t=ln(v),m=(1<<n)-1;t&=m,a.cast='u8';for(let z=0;z<n;z++){
				let pos=1<<(7-bit),dst=get_raw(a,a.here)&~pos
				set_raw(a,a.here,t&(1<<(n-1-z))?dst|pos:dst),bit++;if(bit==8)bit=0,a.here++
			}
		})
	}
	const struct=(a,z)=>{
		const oc=a.cast, shape=z[0]||ZERO, value=z[1], size=struct_size(shape);if(value&&a.here+size>=a.size)resize(a,a.here+size)
		const r=value?(struct_write(a,shape,value),value):struct_read(a,shape);return a.cast=oc,r
	}
	const cat=(a,z)=>{
		return z.map(v=>{
			const s=lin(v)?lms(a.cast): lil(v)?lml([lms(a.cast),monad.count(v)]):
			      array_is(v)?lml([ifield(v,'cast'),ifield(v,'size')]): (v=lms(ls(v)),lml([lms('char'),monad.count(v)]))
			struct(a,[s,v])
		}),a
	}
	const ri=lmi((self,i,x)=>{
		if(!lis(i)){const o=offset(i);if(x){set(self,o.offset,o.len,x);return x;}else{return get(self,o.offset,o.len);}}
		if(x){
			if(ikey(i,'size'))return resize(self,ln(x)*casts[self.cast]),x
			if(ikey(i,'cast'))return self.cast=normalize_enum(casts,ls(x)),x
			if(ikey(i,'here'))return self.here=max(0,ln(x)),x
		}else{
			if(ikey(i,'encoded'))return lms(array_write(self))
			if(ikey(i,'cast'   ))return lms(self.cast)
			if(ikey(i,'size'   ))return lmn(self.size/casts[self.cast])
			if(ikey(i,'here'   ))return lmn(self.here)
			if(ikey(i,'slice'  ))return lmnat(z=>slice (self,z))
			if(ikey(i,'copy'   ))return lmnat(z=>copy  (self,z))
			if(ikey(i,'struct' ))return lmnat(z=>struct(self,z))
			if(ikey(i,'cat'    ))return lmnat(z=>cat   (self,z))
		}return x?x:NIL
	},'array')
	ri.size=size*casts[cast],ri.here=0,ri.base=base,ri.cast=cast,ri.data=buffer||new Uint8Array(ri.size)
	return ri
}

find_occupied=(image,mask)=>{
	const s=image.size,d=rcopy(s);for(let z=0;z<image.pix.length;z++){
		if(image.pix[z]==mask)continue;const x=z%s.x, y=0|(z/s.x);d.x=min(d.x,x), d.y=min(d.y,y), d.w=max(d.w,x), d.h=max(d.h,y)
	}d.w-=d.x,d.h-=d.y,d.w++,d.h++;return d
}
image_copy=(i,r)=>{
	r=r?rint(r):rect(0,0,i.size.x,i.size.y);const c=image_make(rect(r.w,r.h)), clip=rect(0,0,i.size.x,i.size.y)
	for(let y=0;y<r.h;y++)for(let x=0;x<r.w;x++)c.pix[x+r.w*y]=rin(clip,rect(r.x+x,r.y+y))?i.pix[(r.x+x)+i.size.x*(r.y+y)]:0
	return c
}
image_paste=(r,clip,src,dst,opaque)=>{
	r=rint(r);const s=src.size,ds=dst.size
	for(let y=0;y<s.y;y++)for(let x=0;x<s.x;x++)if(rin(clip,rect(r.x+x,r.y+y))&&(opaque||src.pix[x+s.x*y]))dst.pix[r.x+x+ds.x*(r.y+y)]=src.pix[x+s.x*y]
}
lerp_scale=(r,s)=>rect(r.w/s.x,r.h/s.y)
image_paste_scaled=(r,clip,src,dst,opaque)=>{
	r=rint(r);if(r.w==0||r.h==0)return;const s=src.size,ds=dst.size,sc=lerp_scale(r,s)
	if(r.w==s.x&&r.h==s.y)return image_paste(r,clip,src,dst,opaque)
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		let sx=0|(b/sc.x), sy=0|(a/sc.y), c=src.pix[sx+sy*s.x]
		if((opaque||c!=0)&&rin(clip,rect(r.x+b,r.y+a)))dst.pix[r.x+b+ds.x*(r.y+a)]=c
	}
}
image_dither=i=>{
	const stride=2*i.size.x, m=[0,1,i.size.x-2,i.size.x-1,i.size.x,stride-1], e=new Float32Array(stride)
	for(let ei=0,z=0;z<i.pix.length;z++){
		const pix=((0xFF&i.pix[z])/256.0)+e[ei], col=pix>.5?1:0, err=(pix-col)/8.0
		e[ei]=0, ei=(ei+1)%stride; for(let x=0;x<6;x++)e[(ei+m[x])%stride]+=err; i.pix[z]=!col
	}
}
image_flip_h=i=>{const s=i.size;for(let z=0;z<s.y;z++){let a=z*s.x,b=(z+1)*s.x-1;while(a<b){let t=i.pix[a];i.pix[a]=i.pix[b];i.pix[b]=t;a++;b--}}}
image_flip_v=i=>{const s=i.size;for(let z=0;z<s.x;z++){let a=z,b=z+s.x*(s.y-1);while(a<b){let t=i.pix[a];i.pix[a]=i.pix[b];i.pix[b]=t;a+=s.x;b-=s.x}}}
image_flip=i=>{const s=i.size,r=image_make(rect(s.y,s.x));for(let a=0;a<s.y;a++)for(let b=0;b<s.x;b++)r.pix[a+s.y*b]=i.pix[b+s.x*a];i.pix=r.pix,i.size=r.size}
image_resize=(i,size)=>{
	const os=i.size,ob=i.pix;size=rint(rmax(size,rect()));if(requ(os,size))return i;if(size.x==0||size.y==0)size=rect()
	i.pix=new Uint8Array(size.x*size.y),i.size=size;
	for(let a=0;a<size.y;a++)for(let b=0;b<size.x;b++)i.pix[b+a*size.x]=a>=os.y||b>=os.x?0: ob[b+a*os.x];return i
}
buffer_map=(buff,x,fill)=>{
	const m=new Uint8Array(256);for(let z=0;z<256;z++)m[z]=fill?ln(fill):z;x=ld(x)
	for(let z=0;z<x.k.length;z++){let k=0|ln(x.k[z]);if(k>=-128&&k<=255)m[0xFF&k]=0xFF&ln(x.v[z])}
	for(let z=0;z<buff.length;z++)buff[z]=m[buff[z]]
}
buffer_hist=(buff,sign)=>{
	const b_extend=u=>(u)|(0-((u)&0x80)),r=lmd(),c=new Float32Array(256);for(let z=0;z<buff.length;z++)c[buff[z]]++
	for(let z=0;z<256;z++)if(c[z]!=0)dset(r,lmn(sign?b_extend(z):z),lmn(c[z]));return r
}
image_bounds=i=>{
	const s=i.size,d=rect(s.x,s.y,0,0)
	for(let z=0;z<i.pix.length;z++)if(i.pix[z]!=0){const x=z%s.x,y=0|(z/s.x);d.x=min(d.x,x),d.y=min(d.y,y),d.w=max(d.w,x),d.h=max(d.h,y)}
	return lmd(["pos","size"].map(lms),[d,rmin(s,rect(d.w-d.x+1,d.h-d.y+1))].map(lmpair))
}
image_merge_op=(target,src,op)=>{
	const ts=target.size,bs=src.size,t=target.pix,b=src.pix;if(bs.x==0||bs.y==0)return
	if(op=='+')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]+=        b[(x%bs.x)+(y%bs.y)*bs.x]
	if(op=='-')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]-=        b[(x%bs.x)+(y%bs.y)*bs.x]
	if(op=='*')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]*=        b[(x%bs.x)+(y%bs.y)*bs.x]
	if(op=='&')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]=min(t[i],b[(x%bs.x)+(y%bs.y)*bs.x])
	if(op=='|')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]=max(t[i],b[(x%bs.x)+(y%bs.y)*bs.x])
	if(op=='<')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]=t[i] <   b[(x%bs.x)+(y%bs.y)*bs.x]
	if(op=='>')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]=t[i] >   b[(x%bs.x)+(y%bs.y)*bs.x]
	if(op=='=')for(let y=0,i=0;y<ts.y;y++)for(let x=0;x<ts.x;x++,i++)t[i]=t[i]==   b[(x%bs.x)+(y%bs.y)*bs.x]
}
image_make=size=>{
	size=rint(size)
	const f=(self,i,x)=>{
		const s=self.size
		if(i&&lil(i)){ // read/write single pixels
			const p=rint(getpair(i)),ib=p.x>=0&&p.y>=0&&p.x<s.x&&p.y<s.y
			if(x){if(ib)self.pix[p.x+p.y*s.x]=ln(x);return x}
			return ib?lmn(self.pix[p.x+p.y*s.x]):NIL
		}
		if(ikey(i,'pixels')){ // read/write all pixels
			if(x){ll(monad.raze(lml(ll(x)))).forEach((v,i)=>self.pix[i]=ln(v));return x}
			const r=[];for(let y=0;y<s.y;y++){const t=[];for(let x=0;x<s.x;x++)t.push(lmn(self.pix[x+y*s.x]));r.push(lml(t))}return lml(r)
		}
		if(ikey(i,'encoded'))return lms(image_write(self))
		if(ikey(i,'hist'))return buffer_hist(self.pix,0)
		if(ikey(i,'bounds'))return image_bounds(self)
		if(ikey(i,'size'))return x?(image_resize(self,getpair(x)),x): lmpair(self.size)
		if(ikey(i,'map'))return lmnat(([x,fill])=>(buffer_map(self.pix,x,fill),self))
		if(ikey(i,'merge'))return lmnat(z=>{
			if(lis(z[0])){if(image_is(z[1]))image_merge_op(self,z[1],ls(z[0])[0]);return self}
			if(lil(z[0]))z=ll(z[0]);const nice=x=>x&&image_is(x)&&x.size.x>0&&x.size.y>0, s=self.size
			const v=new Uint8Array(256),sx=new Uint32Array(256),sy=new Uint32Array(256)
			for(let p=0;p<z.length&&p<256;p++)if(nice(z[p]))v[p]=1,sx[p]=z[p].size.x,sy[p]=z[p].size.y
			for(let y=0,i=0;y<s.y;y++)for(let x=0;x<s.x;x++,i++){const p=self.pix[i],c=v[p]?z[p].pix[(x%sx[p])+(y%sy[p])*sx[p]]:0;self.pix[i]=c}
			return self
		})
		if(ikey(i,'transform'))return lmnat(([x])=>{
			if(x.v=='horiz')image_flip_h(self); if(x.v=='vert')image_flip_v(self); if(x.v=='flip')image_flip(self); if(x.v=='dither')image_dither(self)
			if(x.v=='left' )image_flip_h(self),image_flip(self); if(x.v=='right')image_flip(self),image_flip_h(self)
			return self
		})
		if(ikey(i,'rotate'))return lmnat(([n])=>{
			n=-(ln(n)%(2*Math.PI));if(abs(n)>Math.PI/2&&abs(n)<Math.PI*3/2)image_flip_v(self),image_flip_h(self),n+=(n<0?1:-1)*Math.PI
			const s=self.size,t=image_make(s)
			const shx=n=>{for(let y=0;y<s.y;y++){const o=0|(n*(y-s.y/2));for(let x=0;x<s.x;x++)t.pix[x+y*s.x]=self.pix[mod(x+o,s.x)+y*s.x]};self.pix.set(t.pix)}
			const shy=n=>{for(let x=0;x<s.x;x++){const o=0|(n*(x-s.x/2));for(let y=0;y<s.y;y++)t.pix[x+y*s.x]=self.pix[x+mod(y+o,s.y)*s.x]};self.pix.set(t.pix)}
			shx(-Math.tan(n/2)),shy(Math.sin(n)),shx(-Math.tan(n/2));return self
		})
		if(ikey(i,'translate'))return lmnat(([x,y])=>{
			const o=rint(getpair(x)), w=y?lb(y):0;if(o.x==0&&o.y==0)return self;const s=self.size,t=image_make(s)
			if(w){for(let y=0,z=0;y<s.y;y++)for(let x=0;x<s.x;x++,z++)                           t.pix[z]=self.pix[mod(x-o.x,s.x)+mod(y-o.y,s.y)*s.x]}
			else {for(let y=0,z=0;y<s.y;y++)for(let x=0;x<s.x;x++,z++){const i=rect(x-o.x,y-o.y);t.pix[z]=(i.x<0||i.x>=s.x||i.y<0||i.y>=s.y)?0: self.pix[i.x+i.y*s.x]}}
			self.pix.set(t.pix);return self
		})
		if(ikey(i,'scale'))return lmnat(([z,a])=>{
			const o=image_copy(self), n=lin(z)?rect(ln(z),ln(z)):getpair(z), r=rmax(rect(),rint(a&&lb(a)?n:rect(n.x*o.size.x,n.y*o.size.y))), d=rpair(rect(),r)
			image_resize(self,r),image_paste_scaled(d,d,o,self,1);return self
		})
		if(ikey(i,'outline'))return lmnat(([pat])=>{
			const p=ln(pat),s=self.size;if(p<1||p>47)return self;const t=image_copy(self)
			for(let a=0,i=0;a<s.y;a++)for(let b=0;b<s.x;b++,i++){
				if(t.pix[i])continue;let n=0
				if(b>0    )n|=t.pix[(b-1)+(a  )*s.x]
				if(b<s.x-1)n|=t.pix[(b+1)+(a  )*s.x]
				if(a>0    )n|=t.pix[(b  )+(a-1)*s.x]
				if(a<s.y-1)n|=t.pix[(b  )+(a+1)*s.x]
				if(n)self.pix[i]=p
			}return self
		})
		if(ikey(i,'copy'))return lmnat(z=>image_copy(self,unpack_rect(z,self.size)))
		if(ikey(i,'paste'))return lmnat(([img,pos,t])=>{
			img=getimage(img), pos=(pos?ll(pos):[]).map(ln); let solid=t?!lb(t):1, cl=rect(0,0,self.size.x,self.size.y); if(img==self)img=image_copy(img)
			image_paste_scaled(pos.length<=2?rect(pos[0],pos[1],img.size.x,img.size.y):rect(pos[0],pos[1],pos[2],pos[3]),cl,img,self,solid);return self
		})
		return x?x:NIL
	};return {t:'int',f:f,n:'image',size:size,pix:new Uint8Array(size.x*size.y)}
}
image_read=x=>{
	const data=data_read('IMG',x);if(!data||data.length<4)return image_make(rect())
	const f=data_enc(x), w=(data[0]<<8)|data[1], h=(data[2]<<8)|data[3], r=image_make(rect(w,h))
	if(f==0&&data.length-4>=w*h/8){let s=ceil(w/8),o=0;for(let a=0;a<h;a++)for(let b=0;b<w;b++)r.pix[o++]=data[4+(0|b/8)+a*s]&(1<<(7-(b%8)))?1:0}
	if(f==1&&data.length-4>=w*h){r.pix=data.slice(4)}
	if(f==2){let i=4,o=0;while(i+2<=data.length){let p=data[i++],c=0xFF&data[i++];while(c&&o+1<=r.pix.length)c--,r.pix[o++]=p;}}
	return r
}
image_write=x=>{
	x=image_is(x)?x:image_make(rect());let f=0,s=x.size,t=[0xFF&(s.x>>8),0xFF&s.x,0xFF&(s.y>>8),0xFF&s.y],l=t.slice(0)
	for(let z=0;z<x.pix.length;){let c=0,p=x.pix[z];while(c<255&&z<x.pix.length&&x.pix[z]==p)c++,z++;l.push(p),l.push(c)}
	if(!x.pix.some(x=>x>1)&&(l.length>4+s.x*s.y/8)){
		f='0';let stride=8*ceil(s.x/8);for(let a=0;a<s.y;a++)for(let b=0;b<stride;b+=8)
		{let v=0;for(let i=0;i<8;i++)v=(v<<1)|(b+i>=s.x?0: x.pix[b+i+a*s.x]?1:0);t.push(v)}
	}else if(l.length>4+s.x*s.y){f='1';for(let z=0;z<s.x*s.y;z++)t.push(x.pix[z])}else{f='2',t=l}
	return data_write('IMG'+f,t)
}
n_image=([size])=>lis(size)?image_read(ls(size)):image_make(getpair(size))
is_blank=x=>!image_is(x)?0: !x.pix.some(x=>x>0)

sound_make=data=>{
	const sign_extend=x=>(x<<24>>24)
	const ri=lmi((self,i,x)=>{
		const fetch=ix=>ix<0||ix>=self.data.length?NIL:lmn(sign_extend(self.data[ix]))
		if(i&&lin(i)){ // read/write single samples
			return x?((self.data[ln(i)]=0xFF&ln(x)),x): fetch(ln(i))
		}
		if(i&&lil(i)){ // read/write ranges
			const n=getpair(i);n.y=max(0,n.y);if(x){
				const s=ll(x),dc=self.data.length,sc=s.length, r=new Uint8Array(clamp(0,(dc-n.y)+sc,10*SFX_RATE))
				for(let z=0;z<n.x         ;z++)r[z]=self.data[z]
				for(let z=0;z<sc          ;z++)r[n.x+z   ]=0xFF&ln(s[z])
				for(let z=0;z<dc-(n.x+n.y);z++)r[n.x+sc+z]=0xFF&self.data[n.x+n.y+z]
				return self.data=r,x
			}else{return lml(range(n.y).map(x=>fetch(x+n.x)))}
		}
		if(ikey(i,'encoded'))return lms(sound_write(self))
		if(ikey(i,'hist'))return buffer_hist(self.data,1)
		if(ikey(i,'size')){
			if(!x)return lmn(self.data.length)
			const n=clamp(0,ln(x),10*SFX_RATE),o=self.data;self.data=new Uint8Array(n)
			for(let z=0;z<o.length&&z<n;z++)self.data[z]=o[z];return x
		}
		if(ikey(i,'duration'))return lmn(self.data.length/SFX_RATE)
		if(ikey(i,'map'))return lmnat(([x,fill])=>(buffer_map(self.data,x,fill),self))
		return x?x:NIL
	},'sound')
	if(data&&data.length>10*SFX_RATE)data=data.slice(0,10*SFX_RATE)
	ri.data=data||new Uint8Array(0)
	return ri
}
sound_read=x=>sound_make((typeof x=='string')?data_read('SND',x):new Uint8Array(clamp(0,+x,10*SFX_RATE)))
sound_write=x=>data_write('SND0',x.data)
n_sound=([x])=>!x?sound_read(0): lis(x)?sound_read(ls(x)): lin(x)?sound_read(ln(x)): sound_make(Uint8Array.from(ll(x).map(ln)))

pal_col_get=(pal,c)=>{const b=(8*224)+(3*c);return 0xFF000000|((pal[b]<<16)|(pal[b+1]<<8)|pal[b+2])}
pal_col_set=(pal,c,x)=>{const b=(8*224)+(3*c);pal[b]=0xFF&(x>>16),pal[b+1]=0xFF&(x>>8),pal[b+2]=0xFF&x}
pick_palette=deck=>{for(let z=0;z<16;z++)COLORS[z]=pal_col_get(deck.patterns.pal.pix,z)}
patterns_read=x=>{
	const set=(pal,p,x,y,v)=>pal[(x%8)+(8*(y%8))+(8*8*p)]=v
	const ri=lmi((self,i,x)=>{
		let r=null, t=i&&ln(i)?ln(i):0
		if(x){
			if(t>= 2&&t<=27&&image_is(x)){for(let a=0;a<8;a++)for(let b=0;b<8;b++)set(self.pal.pix,t,b,a,lb(iwrite(x,lmpair(rect(b,a)))))}
			if(t>=28&&t<=31){r=ll(x);if(r.length>256)r=r.slice(0,256);self.anim[t-28]=r.map(x=>{const f=clamp(0,ln(x),47);return f>=28&&f<=31?0:f});r=lml(r)}
			if(t>=32&&t<=47){pal_col_set(self.pal.pix,t-32,0xFF000000|ln(x));r=x}
		}else{
			if(t>= 0&&t<=27){r=image_copy(self.pal,rect(0,t*8,8,8))}
			if(t>=28&&t<=31){r=lml(self.anim[t-28].map(lmn))}
			if(t>=32&&t<=47){r=lmn(0xFFFFFF&pal_col_get(self.pal.pix,t-32))}
		}return r?r:x?x:NIL
	},'patterns')
	let i=image_read(x.patterns?ls(x.patterns):DEFAULT_PATTERNS)
	if(i.size.x!=8||i.size.y!=224+6){i=image_resize(i,rect(8,224+6));for(let z=0;z<16;z++)pal_col_set(i.pix,z,DEFAULT_COLORS[z])}
	ri.pal=i
	ri.anim=JSON.parse(DEFAULT_ANIMS);if(x.animations&&lil(x.animations))ll(x.animations).map((x,i)=>iindex(ri,28+i,x))
	return ri
}
patterns_write=x=>{
	const p=x.pal.pix, c=DEFAULT_COLORS.some((x,i)=>(0xFFFFFF&x)!=(0xFFFFFF&pal_col_get(p,i)))
	return image_write(image_resize(image_copy(x.pal),rect(8,224+(6*c))))
}
anims_write=x=>lml(x.anim.map(x=>lml(x.map(lmn))))

font_get=(i,f,v)=>{if(v!=undefined)f.pix[i]=v;return f.pix[i]}
font_w =(f,v)=>font_get(0,f,v)
font_h =(f,v)=>font_get(1,f,v)
font_sw=(f,v)=>font_get(2,f,v)
font_gs=(f,v)=>font_h(f)*ceil(font_w(f)/8)+1
font_gb=(f,c)=>3+drom_to_ord(c)*font_gs(f)
font_gbi=(f,c)=>3+c*font_gs(f)
font_gw=(f,c,v)=>font_get(font_gb(f,c),f,v)
font_gwi=(f,c,v)=>font_get(font_gbi(f,c),f,v)
font_pp=(f,c,x,y,v)=>font_get(font_gb(f,c)+1+y*ceil(font_w(f)/8)+Math.floor(x/8),f,v)
font_bit=(x,v)=>(v<<(7-(x%8)))
font_gpix=(f,c,x,y)=>((font_pp(f,c,x,y)&font_bit(x,1))?1:0)
font_spix=(f,c,x,y,v)=>font_pp(f,c,x,y,(font_pp(f,c,x,y)&~font_bit(x,1))|font_bit(x,v))
font_textsize=(f,t)=>{
	const cursor=rect(),size=rect(0,font_h(f))
	for(let z=0;t[z];z++){if(t[z]!='\n'){cursor.x+=font_gw(f,t[z])+font_sw(f),size.x=max(size.x,cursor.x)}else{cursor.x=0,size.y+=font_h(f)}}
	return size
}
font_read=arg=>{
	const ri=lmi((self,i,x)=>{
		if(lin(i)||(lis(i)&&count(i)==1)){ // read/write glyphs
			let ix=lin(i)?ln(i): drom_to_ord(ls(i));const ch=drom_from_ord(ix)
			if(x){
				if(ix<0||ix>255)return x;if(!image_is(x))x=image_make(rect());
				font_gwi(self,ix,min(x.size.x,font_w(self)));const s=rect(font_gwi(self,ix),font_h(self))
				for(let a=s.y-1;a>=0;a--)for(let b=s.x-1;b>=0;b--)font_spix(self,ch,b,a, b>=(x.size.x||a>=x.size.y)?0:x.pix[b+a*x.size.x]?1:0)
				return x
			}
			if(ix<0||ix>255)return image_make(rect())
			const s=rect(font_gwi(self,ix),font_h(self)),r=image_make(s)
			for(let a=s.y-1;a>=0;a--)for(let b=s.x-1;b>=0;b--)r.pix[b+a*s.x]=font_gpix(self,ch,b,a)
			return r
		}
		if(x){
			if(ikey(i,'space'))return font_sw(self,ln(x)),x
			if(ikey(i,'size')){
				const r=font_read(rmax(rint(getpair(x)),rect(1,1)));iwrite(r,lms('space'),ifield(self,'space'))
				for(let z=0;z<256;z++)iindex(r,z,iindex(self,z));self.pix=r.pix;return x
			}
		}else{
			if(ikey(i,'size'))return lmpair(rect(font_w(self),font_h(self)))
			if(ikey(i,'space'))return lmn(font_sw(self))
			if(ikey(i,'textsize'))return lmnat(([x])=>lmpair(font_textsize(self,x?ls(x):'')))
			if(ikey(i,'glyphs'))return lml(range(256).filter(x=>font_gwi(self,x)).map(lmn))
		}return x?x:NIL
	},'font')
	const make=sz=>{ri.pix=new Uint8Array(3+256*(1+sz.y*ceil(sz.x/8.0)));font_w(ri,sz.x),font_h(ri,sz.y),font_sw(ri,1)}
	if(typeof arg=='string'){
		const r=data_read('FNT',arg), s=rmax(rect(r[0],r[1]),rect(1,1)), gs=ceil(s.x/8)*s.y;make(s),font_sw(ri,r[2])
		if(arg[5]=='0'){
			for(let z=3,ci=32;(z<r.length)&&(ci<128);ci++,z+=gs){
				const gw=r[z++];if(z+gs>r.length)break
				for(let zz=0;zz<gs;zz++)font_get(font_gbi(ri,ci)+1+zz,ri,r[z+zz])
				font_gwi(ri,ci,gw)
			}
		}
		if(arg[5]=='1'){
			for(let z=3;z+1<r.length;z+=gs){
				const ci=r[z++],gw=r[z++];if(z+gs>r.length)break
				for(let zz=0;zz<gs;zz++)font_get(font_gbi(ri,ci)+1+zz,ri,r[z+zz])
				font_gwi(ri,ci,gw)
			}
		}
	}
	if(!ri.pix){make(rmax(rint(arg),rect(1,1)))}
	return ri
}
font_write=x=>{
	const s=rect(font_w(x),font_h(x)), gs=font_gs(x), r=[s.x,s.y,font_sw(x)]
	const dense=range(256).every(z=>(font_gwi(x,z)!=0)==(z>=32&&z<=127))
	if(dense){
		for(let ci=32;ci<128;ci++){
			r.push(font_gwi(x,ci))
			for(let z=0;z<gs-1;z++)r.push(font_get(font_gbi(x,ci)+z+1,x))
		};return data_write('FNT0',r)
	}else{
		for(let ci=0;ci<256;ci++)if(font_gwi(x,ci)){
			r.push(ci),r.push(font_gwi(x,ci))
			for(let z=0;z<gs-1;z++)r.push(font_get(font_gbi(x,ci)+z+1,x))
		};return data_write('FNT1',r)
	}
}

rtext_empty=_=>{const r=lmt();tab_set(r,'text',[]),tab_set(r,'font',[]),tab_set(r,'arg',[]);return r}
rtext_len=tab=>tab_get(tab,'text').reduce((x,y)=>x+ls(y).length,0)
rtext_get=(tab,n)=>{const t=tab_get(tab,'text');let i=0;for(let z=0;z<t.length;z++){i+=count(t[z]);if(i>=n)return z}return -1}
rtext_getr=(tab,x)=>{const t=tab_get(tab,'text');let i=0;for(let z=0;z<t.length;z++){const c=count(t[z]);if(i+c>=x)return rect(i,i+c);i+=c}return rect(x,x)}
rtext_make=(t,f,a)=>{
	a=!a?'':image_is(a)?a:ls(a), f=!f?'':ls(f), t=image_is(a)?'i':!t?'':count(t)?ls(t):''
	const r=lmt();tab_set(r,'text',[lms(t)]),tab_set(r,'font',[lms(f)]),tab_set(r,'arg',[image_is(a)?a:lms(a)]);return r
}
rtext_cast=x=>{
	if(!x)x=lms('');if(image_is(x))return rtext_make('','',x);if(lid(x))x=monad.table(x);if(!lit(x))return rtext_make(x)
	const tv=tab_get(x,'text'),fv=tab_get(x,'font'),av=tab_get(x,'arg')
	if(tv&&fv&&av&&tv.every((t,i)=>lis(t)&&lis(fv[i])&&(image_is(av[i])||lis(av[i]))))return x
	const r=lmt();tab_set(r,'text',tv||[lms('')]),tab_set(r,'font',fv||[lms('')]),tab_set(r,'arg',av||[lms('')])
	const tr=tab_get(r,'text'),fr=tab_get(r,'font'),ar=tab_get(r,'arg')
	tr.map((_,z)=>{const i=image_is(ar[z]);tr[z]=i?lms('i'):lms(ls(tr[z]));fr[z]=lms(ls(fr[z]));ar[z]=i?ar[z]:lms(ls(ar[z]))});return r
}
rtext_append=(tab,t,f,a)=>{
	if(image_is(a)){if(count(t)>1)t=lms('i');if(count(t)<1)return 0;}if(!count(t))return 0;
	const tv=tab_get(tab,'text'),fv=tab_get(tab,'font'),av=tab_get(tab,'arg')
	if(tv.length&&match(f,last(fv))&&!image_is(a)&&match(a,last(av))){tv[tv.length-1]=lms(ls(last(tv))+ls(t))}
	else{tv.push(t),fv.push(f),av.push(a)}return count(t)
}
rtext_appendr=(tab,row)=>{fv=tab_get(row,'font'),av=tab_get(row,'arg');tab_get(row,'text').map((t,i)=>rtext_append(tab,t,fv[i],av[i]))}
rtext_string=(tab,pos)=>{
	pos=pos||rect(0,RTEXT_END);let r='',i=0,a=min(pos.x,pos.y),b=max(pos.x,pos.y)
	tab_get(tab,'text').map(s=>{for(let z=0;z<s.v.length;z++,i++)if(i>=a&&i<b)r+=s.v[z]});return lms(r)
}
rtext_is_plain=x=>{
	if(!lit(x))return 0;const tv=tab_get(x,'text'),fv=tab_get(x,'font'),av=tab_get(x,'arg');
	if(!tv||!fv||!av||tv.length>1)return 0;if(tv.length==0)return 1
	return ls(fv[0])==''&&!image_is(av[0])&&ls(av[0])==''
}
rtext_is_image=x=>{
	let r=null,t=tab_get(x,'text'),a=tab_get(x,'arg'); // look for at least one image, and other spans must be only whitespace.
	for(let z=0;z<count(x);z++){if(image_is(a[z])){if(!r)r=a[z]}else if(ls(t[z]).trim()!=''){return null}}
	return r
}
rtext_read_images=x=>lml((tab_get(x,'arg')||[]).filter(image_is))
rtext_write_images=x=>rtext_cat(ll(x))
rtext_span=(tab,pos)=>{
	const tv=tab_get(tab,'text'),fv=tab_get(tab,'font'),av=tab_get(tab,'arg')
	let r=dyad.take(ZERO,tab), i=0,c=0,a=min(pos.x,pos.y),b=max(pos.x,pos.y), partial=_=>{
		let rr='';for(let z=0;z<count(tv[c]);z++,i++)if(i>=a&&i<b)rr+=tv[c].v[z]
		rtext_append(r,lms(rr),fv[c],av[c]),c++
	}
	while(c<tv.length&&(i+count(tv[c]))<a)i+=count(tv[c++])                       ;if(c<tv.length&&i<=a)partial()
	while(c<tv.length&&(i+count(tv[c]))<b)i+=rtext_append(r,tv[c],fv[c],av[c]),c++;if(c<tv.length&&i< b)partial()
	return r
}
rtext_splice=(tab,font,arg,text,cursor,endcursor)=>{
	const a=min(cursor.x,cursor.y),b=max(cursor.x,cursor.y),r=rtext_cast()
	rtext_appendr(r,rtext_span(tab,rect(0,a)))
	rtext_append (r,lms(text),font,arg)
	rtext_appendr(r,rtext_span(tab,rect(b,RTEXT_END)))
	endcursor.x=endcursor.y=a+text.length;return r
}
rtext_splicer=(tab,insert,cursor,endcursor)=>{
	const a=min(cursor.x,cursor.y),b=max(cursor.x,cursor.y),r=rtext_cast()
	rtext_appendr(r,rtext_span(tab,rect(0,a)))
	rtext_appendr(r,insert)
	rtext_appendr(r,rtext_span(tab,rect(b,RTEXT_END)))
	endcursor.x=endcursor.y=a+rtext_len(insert);return r
}
rtext_write=x=>{const r=monad.cols(x),arg=dget(r,lms('arg'));if(arg){arg.v=arg.v.map(x=>image_is(x)?lms(image_write(x)):x)};return r}
rtext_read=x=>{
	if(lis(x))return x;x=ld(x)
	const a=dget(x,lms('arg'));if(a){dset(x,lms('arg'),lml(ll(a).map(a=>ls(a).startsWith('%%IMG')?image_read(ls(a)):lms(ls(a)))))}
	return rtext_cast(x)
}
rtext_encode=x=>`%%RTX0${fjson(rtext_write(x))}`
rtext_decode=x=>rtext_read(pjson(x,6,x.length-6).value)
rtext_cat=x=>{let r=rtext_empty();x.map(x=>rtext_appendr(r,rtext_cast(x)));return r}
interface_rtext=lmi((self,i,x)=>{
	if(ikey(i,'end'   ))return lmn(RTEXT_END)
	if(ikey(i,'make'  ))return lmnat(([t,f,a])=>rtext_make(t,f,a))
	if(ikey(i,'len'   ))return lmnat(([t])=>lmn(rtext_len(rtext_cast(t))))
	if(ikey(i,'get'   ))return lmnat(([t,n])=>lmn(rtext_get(rtext_cast(t),n?ln(n):0)))
	if(ikey(i,'string'))return lmnat(([t,i])=>rtext_string(rtext_cast(t),i?getpair(i):undefined))
	if(ikey(i,'span'  ))return lmnat(([t,i])=>rtext_span  (rtext_cast(t),i?getpair(i):undefined))
	if(ikey(i,'cat'   ))return lmnat(rtext_cat)
	if(ikey(i,'split' ))return lmnat(([x,y])=>{
		const d=ls(x),v=rtext_cast(y),t=ls(rtext_string(v)),r=lml([]);if(d.length<1||!x||!y)return r
		let n=0;for(let z=0;z<t.length;z++){
			let m=1;for(let w=0;w<d.length;w++)if(d[w]!=t[z+w]){m=0;break}if(m){r.v.push(rtext_span(v,rect(n,z))),z+=d.length-1,n=z+1}
		}if(n<=t.length)r.v.push(rtext_span(v,rect(n,t.length)));return r
	})
	if(ikey(i,'replace'))return lmnat(([tab,k,v,i])=>{
		if(!k||!v)return tab||NIL;const t=rtext_cast(tab),r=[],tx=ls(rtext_string(t)),nocase=i&&lb(i),text=nocase?tx.toLowerCase():tx,c=rect(0,0)
		if(!lil(k))k=monad.list(k);if(!lil(v))v=monad.list(v)
		k=dyad.take(lmn(max(count(k),count(v))),k),k.v=k.v.map(nocase?x=>ls(x).toLowerCase():ls)
		v=dyad.take(lmn(max(count(k),count(v))),v),v.v=v.v.map(rtext_cast)
		while(c.y<text.length){
			let any=0;for(let ki=0;ki<k.v.length;ki++){
				const key=k.v[ki],val=v.v[ki];if(key.length<1)break;let f=1;for(let i=0;i<key.length;i++)if(text[c.y+i]!=key[i]){f=0;break}
				if(f){if(c.x!=c.y)r.push(rtext_span(t,c));r.push(r,val),c.x=c.y=(c.y+key.length),any=1}
			};if(!any)c.y++
		}if(c.x<text.length)r.push(rtext_span(t,rect(c.x,RTEXT_END)));return rtext_cat(r)
	})
	if(ikey(i,'find'))return lmnat(([tab,k,i])=>{
		const r=[];if(!tab||!k)return lml(r);const nocase=i&&lb(i),tx=ls(rtext_string(rtext_cast(tab))),text=nocase?tx.toLowerCase():tx
		k=lil(k)?ll(k):[k];k=k.map(x=>nocase?ls(x).toLowerCase(): ls(x))
		for(let x=0;x<text.length;){
			let any=0;for(let ki=0;ki<k.length;ki++){
				const key=k[ki];let f=1;for(let i=0;i<key.length;i++)if(text[x+i]!=key[i]){f=0;break}
				if(f){r.push(lml([lmn(x),lmn(x+key.length)])),x+=max(1,key.length),any=1;break}
			}if(!any)x++
		}return lml(r)
	})
	if(ikey(i,'index'))return lmnat(([tab,g])=>{
		if(!tab)return ZERO;let r=0;const t=ls(rtext_string(rtext_cast(tab)));g=g?rint(getpair(g)):rect()
		while(r<t.length&&g.x>0)if(t[r++]=='\n')g.x--;while(r<t.length&&g.y>0&&t[r]!='\n'){g.y--,r++};return lmn(r)
	})
	return x?x:NIL
},'rtext')
button_styles={round:1,rect:1,check:1,invisible:1}
normalize_shortcut=x=>ls(x).toLowerCase().replace(/[^a-z0-9 ]/g,'').slice(0,1)
button_read=(x,card)=>{
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NIL
		if(x){
			if(ikey(i,'value'   ))return self.value=lb(x),x
			if(ikey(i,'text'    ))return self.text=ls(x),x
			if(ikey(i,'style'   ))return self.style=normalize_enum(button_styles,ls(x)),x
			if(ikey(i,'shortcut'))return self.shortcut=normalize_shortcut(x),x
		}else{
			if(ikey(i,'value'   ))return value_inherit(self,ls(i))||ZERO
			if(ikey(i,'text'    ))return lms(ivalue(self,ls(i),''))
			if(ikey(i,'style'   ))return lms(ivalue(self,ls(i),'round'))
			if(ikey(i,'size'    ))return lmpair(ivalue(self,ls(i),rect(60,20)))
			if(ikey(i,'shortcut'))return lms(ivalue(self,ls(i),''))
		}return interface_widget(self,i,x)
	},'button');ri.card=card
	init_field(ri,'text'    ,x)
	init_field(ri,'style'   ,x)
	init_field(ri,'value'   ,x)
	init_field(ri,'shortcut',x)
	return ri
}
button_write=x=>{
	const r=lmd([lms('type')],[lms('button')])
	if(x.text)dset(r,lms('text' ),lms(x.text))
	if(x.style&&x.style!='round')dset(r,lms('style'),lms(x.style))
	if(x.value!=undefined&&!x.volatile)dset(r,lms('value'),lmn(x.value))
	if(x.shortcut)dset(r,lms('shortcut'),lms(x.shortcut))
	return r
}
field_styles={rich:1,plain:1,code:1}
field_aligns={left:1,center:1,right:1}
field_read=(x,card)=>{
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NIL
		if(x){
			if(ikey(i,'text'  ))return self.value=rtext_cast(lms(ls(x))),field_notify(self),x
			if(ikey(i,'images'))return self.value=rtext_write_images(x),field_notify(self),x
			if(ikey(i,'data'  ))return self.value=rtext_cast(dyad.format(lms('%J'),monad.list(x))),field_notify(self),x
			if(ikey(i,'scroll'))return self.scroll=max(0,ln(x)),x
			if(ikey(i,'value' )){
				if(ls(ifield(self,'style'))!='rich'&&!rtext_is_plain(x))x=rtext_string(rtext_cast(x))
				return self.value=rtext_cast(x),field_notify(self),x
			}
			if(ikey(i,'border'   ))return self.border=lb(x),x
			if(ikey(i,'scrollbar'))return self.scrollbar=lb(x),x
			if(ikey(i,'style'    ))return self.style=normalize_enum(field_styles,ls(x)),iwrite(self,lms('value'),ifield(self,'value')),x
			if(ikey(i,'align'    ))return self.align=normalize_enum(field_aligns,ls(x)),x
		}else{
			if(ikey(i,'text'     )){const v=value_inherit(self,'value');return v!=undefined?rtext_string(v):lms('')}
			if(ikey(i,'images'   )){const v=value_inherit(self,'value');return v!=undefined?rtext_read_images(v):lml([])}
			if(ikey(i,'data'     )){const v=value_inherit(self,'value');return v!=undefined?dyad.parse(lms('%J'),rtext_string(v)):NIL}
			if(ikey(i,'border'   ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'value'    ))return value_inherit(self,ls(i))||rtext_cast()
			if(ikey(i,'scroll'   ))return value_inherit(self,ls(i))||ZERO
			if(ikey(i,'scrollbar'))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'style'    ))return lms(ivalue(self,ls(i),'rich'))
			if(ikey(i,'align'    ))return lms(ivalue(self,ls(i),'left'))
			if(ikey(i,'size'     ))return lmpair(ivalue(self,ls(i),rect(100,20)))
			if(ikey(i,'font'     ))return dget(self.card.deck.fonts,lms(self.font||(self.style=='code'?'mono':'body')))
			if(ikey(i,'scrollto' ))return lmnat(([x])=>{
				const bi=inset(rpair(getpair(ifield(self,'pos')),getpair(ifield(self,'size'))),2);if(lb(ifield(self,'scrollbar')))bi.w-=12+3
				const l=layout_richtext(self.card.deck,ifield(self,'value'),ifield(self,'font'),ALIGN[ls(ifield(self,'align'))],bi.w)
				const i=x?min(max(0,0|ln(monad.first(x))),l.layout.length-1):0, c=rcopy(l.layout[i].pos), os=ln(ifield(self,'scroll'));c.y-=os
				const ch=min(bi.h,c.h);let t=os;if(c.y<0){t+=c.y};if(c.y+ch>=bi.h){t+=((c.y+ch)-bi.h)}
				if(t!=os)iwrite(self,lms('scroll'),lmn(t));return self
			})
		}return interface_widget(self,i,x)
	},'field');ri.card=card
	{const k=lms('value'),v=dget(x,k);if(v)iwrite(ri,k,rtext_read(v))}
	init_field(ri,'border'   ,x)
	init_field(ri,'scrollbar',x)
	init_field(ri,'style'    ,x)
	init_field(ri,'align'    ,x)
	init_field(ri,'scroll'   ,x)
	return ri
}
field_write=x=>{
	const r=lmd([lms('type')],[lms('field')])
	if(x.border!=undefined)dset(r,lms('border'),lmn(x.border))
	if(x.scrollbar!=undefined)dset(r,lms('scrollbar'),lmn(x.scrollbar))
	if(x.style&&x.style!='rich')dset(r,lms('style'),lms(x.style))
	if(x.align&&x.align!='left')dset(r,lms('align'),lms(x.align))
	if(x.scroll&&!x.volatile)dset(r,lms('scroll'),lmn(x.scroll))
	if(x.value&&!x.volatile){if(rtext_is_plain(x.value)){const v=rtext_string(x.value);if(ls(v))dset(r,lms('value'),v)}else{dset(r,lms('value'),rtext_write(x.value))}}
	return r
}
slider_styles={horiz:1,vert:1,bar:1,compact:1}
slider_normalize=(self,n)=>{const i=getpair(ifield(self,'interval')),s=ln(ifield(self,'step'));return clamp(i.x,Math.round(n/s)*s,i.y)}
slider_read=(x,card)=>{
	const update=self=>iwrite(self,lms('value'),ifield(self,'value'))
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NIL
		if(x){
			if(ikey(i,'value'   ))return self.value=slider_normalize(self,ln(x)),x
			if(ikey(i,'step'    ))return self.step=max(0.000001,ln(x)),update(self),x
			if(ikey(i,'format'  ))return self.format=ls(x),x
			if(ikey(i,'style'   ))return self.style=normalize_enum(slider_styles,ls(x)),x
			if(ikey(i,'interval')){const v=getpair(x);return self.interval=rect(min(v.x,v.y),max(v.x,v.y)),update(self),x}
		}else{
			if(ikey(i,'value'   )){const v=getpair(ifield(self,'interval'));return value_inherit(self,ls(i))||lmn(clamp(v.x,0,v.y))}
			if(ikey(i,'format'  ))return lms(ivalue(self,ls(i),'%f'))
			if(ikey(i,'step'    ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'interval'))return lmpair(ivalue(self,ls(i),rect(0,100)))
			if(ikey(i,'style'   ))return lms(ivalue(self,ls(i),'horiz'))
			if(ikey(i,'size'    ))return lmpair(ivalue(self,ls(i),rect(100,25)))
		}return interface_widget(self,i,x)
	},'slider');ri.card=card
	init_field(ri,'interval',x)
	init_field(ri,'step'    ,x)
	init_field(ri,'value'   ,x)
	init_field(ri,'format'  ,x)
	init_field(ri,'style'   ,x)
	return ri
}
slider_write=x=>{
	const r=lmd([lms('type')],[lms('slider')])
	if(x.interval)dset(r,lms('interval'),lmpair(x.interval))
	if(x.value!=undefined&&x.value!=0&&!x.volatile)dset(r,lms('value'),lmn(x.value))
	if(x.step!=undefined&&x.step!=1)dset(r,lms('step'),lmn(x.step))
	if(x.format!=undefined&&x.format!='%f')dset(r,lms('format'),lms(x.format))
	if(x.style&&x.style!='horiz')dset(r,lms('style'),lms(x.style))
	return r
}
grid_scrollto=(t,g,s,r)=>{
	const head=g.headers?10+5:0, row=g.font?font_h(g.font):11, n=min(count(t),0|((g.size.h-head+1)/(row+5)));
	return (r-s<0)?r: (r-s>=n)?r-(n-1): s
}
grid_read=(x,card)=>{
	const ints=(x,n)=>{const r=[];for(let z=0;z<n&&z<x.length;z++)r.push(ln(x[z]));return r}
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NIL
		if(x){
			if(ikey(i,'value'    ))return self.value=lt(x),x
			if(ikey(i,'scroll'   ))return self.scroll=max(0,ln(x)),x
			if(ikey(i,'row'      ))return self.row=max(-1,ln(x)),x
			if(ikey(i,'col'      ))return (!lin(x)?iwrite(self,lms('colname'),x): self.col=max(-1,ln(x))),x
			if(ikey(i,'colname'  ))return iwrite(self,lms('col'),lmn(tab_cols(ifield(self,'value')).indexOf(ls(x)))),x
			if(ikey(i,'cell'     ))return iwrite(self,lms('col'),l_at(x,ZERO)),iwrite(self,lms('row'),l_at(x,ONE)),x
			if(ikey(i,'scrollbar'))return self.scrollbar=lb(x),x
			if(ikey(i,'headers'  ))return self.headers=lb(x),x
			if(ikey(i,'lines'    ))return self.lines=lb(x),x
			if(ikey(i,'bycell'   ))return self.bycell=lb(x),x
			if(ikey(i,'widths'   ))return self.widths=ints(ll(x),255),x
			if(ikey(i,'format'   ))return self.format=ls(x),x
			if(ikey(i,'rowvalue' ))return iwrite(self,lms('value'   ),amend(ifield(self,'value'   ),ifield(self,'row'    ),x)),x
			if(ikey(i,'cellvalue'))return iwrite(self,lms('rowvalue'),amend(ifield(self,'rowvalue'),ifield(self,'colname'),x)),x
		}else{
			if(ikey(i,'value'    ))return value_inherit(self,ls(i))||lmt()
			if(ikey(i,'scroll'   ))return value_inherit(self,ls(i))||ZERO
			if(ikey(i,'scrollbar'))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'headers'  ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'lines'    ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'bycell'   ))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'widths'   ))return lml((ivalue(self,ls(i),[])).map(lmn))
			if(ikey(i,'format'   ))return lms(ivalue(self,ls(i),''))
			if(ikey(i,'size'     ))return lmpair(ivalue(self,ls(i),rect(100,50)))
			if(ikey(i,'cell'     ))return lml([ifield(self,'col'),ifield(self,'row')])
			if(ikey(i,'row'      )){const r=value_inherit(self,ls(i))||lmn(-1);return lmn(clamp(-1,ln(r),count(ifield(self,'value'))-1))}
			if(ikey(i,'col'      )){const c=value_inherit(self,ls(i))||lmn(-1);return lmn(clamp(-1,ln(c),count(monad.keys(ifield(self,'value')))-1))}
			if(ikey(i,'colname'  )){const c=ln(ifield(self,'col'));k=tab_cols(ifield(self,'value'));return c<0||c>=k.length?NIL: lms(k[c])}
			if(ikey(i,'rowvalue' )){const r=ln(ifield(self,'row')),v=ifield(self,'value');return r<0||r>=count(v)?lmd():l_at(v,lmn(r))}
			if(ikey(i,'cellvalue')){
				const r=ln(ifield(self,'row')),c=ln(ifield(self,'col')),v=ifield(self,'value'),cn=tab_cols(v);
				return r<0||c<0||r>=count(v)||c>=tab_cols(v).length?NIL: tab_cell(v,cn[c],r)
			}
			if(ikey(i,'scrollto'))return lmnat(z=>{
				const sz=rpair(getpair(ifield(self,'pos')),getpair(ifield(self,'size'))), s=ln(ifield(self,'scroll'))
				const g={size:sz,font:ifield(self,'font'),headers:lb(ifield(self,'headers'))}
				const t=grid_scrollto(ifield(self,'value'),g,s,ln(z.length?z[0]:ZERO))
				if(t!=s)iwrite(self,lms('scroll'),lmn(t));return self
			})
		}return interface_widget(self,i,x)
	},'grid');ri.card=card
	init_field(ri,'scrollbar',x)
	init_field(ri,'headers'  ,x)
	init_field(ri,'lines'    ,x)
	init_field(ri,'bycell'   ,x)
	init_field(ri,'widths'   ,x)
	init_field(ri,'format'   ,x)
	init_field(ri,'scroll'   ,x)
	init_field(ri,'row'      ,x)
	init_field(ri,'col'      ,x)
	{const k=lms('value'),v=dget(x,k);if(v)iwrite(ri,k,monad.table(v))}
	return ri
}
grid_write=x=>{
	const r=lmd([lms('type')],[lms('grid')])
	if(x.scrollbar!=undefined)dset(r,lms('scrollbar'),lmn(x.scrollbar))
	if(x.headers!=undefined)dset(r,lms('headers'),lmn(x.headers))
	if(x.lines!=undefined)dset(r,lms('lines'),lmn(x.lines))
	if(x.bycell!=undefined)dset(r,lms('bycell'),lmn(x.bycell))
	if(x.widths)dset(r,lms('widths'),lml(x.widths.map(lmn)))
	if(x.format)dset(r,lms('format'),lms(x.format))
	if(x.value &&!x.volatile)dset(r,lms('value'),monad.cols(x.value))
	if(x.scroll&&!x.volatile)dset(r,lms('scroll'),lmn(x.scroll))
	if(x.row!=undefined&&x.row!=-1&&!x.volatile)dset(r,lms('row'),lmn(x.row))
	if(x.col!=undefined&&x.col!=-1&&!x.volatile)dset(r,lms('col'),lmn(x.col))
	return r
}
canvas_clip=(canvas,z)=>{
	const i=container_image(canvas,1),s=i.size,w=rect(0,0,s.x,s.y);canvas.clip=!z||z.length<1?w:rint(rclip(w,unpack_rect(z,w)))
}
canvas_pick=canvas=>{
	container_image(canvas,1)
	if(!canvas.brush  )iwrite(canvas,lms('brush'  ),ifield(canvas,'brush'  ))
	if(!canvas.pattern)iwrite(canvas,lms('pattern'),ifield(canvas,'pattern'))
	if(!canvas.font   )iwrite(canvas,lms('font'   ),ifield(canvas,'font'   ))
	if(!canvas.clip   )canvas_clip(canvas)
	frame=canvas
}
container_image=(canvas,build)=>{
	if(canvas.image||!build)return canvas.image
	const i=canvas.image,scale=!canvas_is(canvas)?1.0:ln(ifield(canvas,'scale')),size=getpair(ifield(canvas,'size'))
	canvas.image=i?image_copy(i):image_make(rect(ceil(size.x/scale),ceil(size.y/scale))),canvas_clip(canvas);return canvas.image
}
canvas_resize=(canvas,size)=>{
	if(!canvas.image)return
	const scale=ln(ifield(canvas,'scale'));image_resize(canvas.image,rect(ceil(size.x/scale),ceil(size.y/scale))),canvas_clip(canvas)
}
canvas_read=(x,card)=>{
	const wid_pal=x=>x.card.deck.patterns.pal.pix
	const wid_rect=(x,z)=>rint(unpack_rect(z,container_image(x).size))
	const wid_crect=(x,z)=>rint(rclip(unpack_rect(z,container_image(x).size),frame.clip))
	const text=(t,pos,a)=>{
		const font=ifield(frame,'font')
		if(pos&&lil(pos)&&count(pos)>=4){
			a=anchors[ls(a)]||0;const r=rint(getrect(pos)), align=(a==0||a==3||a==6)?ALIGN.left:(a==2||a==5||a==8)?ALIGN.right:ALIGN.center
			const valign=s=>rect(align==ALIGN.left?0:align==ALIGN.right?r.w-s.x:0|((r.w-s.x)/2), y=(a==0||a==1||a==2)?0:(a==6||a==7||a==8)?r.h-s.y:0|((r.h-s.y)/2))
			const rbox=s=>{const a=valign(s);return rclip(rint(rect(r.x+a.x,r.y+a.y,s.x,s.y)),frame.clip)}
			if(lit(t)){const l=layout_richtext(frame.card.deck,t,font,align,r.w);draw_text_rich(rbox(l.size),l,frame.pattern,0)}
			else      {const l=layout_plaintext(ls(t),font,align,rect(r.w,r.h)) ;draw_text_wrap(rbox(l.size),l,frame.pattern  )}
		}else{
			if(lit(t)){const p=getpair(pos);return text(t,lml([p.x,p.y,RTEXT_END/1000,RTEXT_END].map(lmn)))}
			const p=anchor(rpair(getpair(pos),font_textsize(font,ls(t))),a)
			draw_text(p,ls(t),font,frame.pattern)
		}
	}
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NIL
		if(x){
			if(ikey(i,'brush'    )){let n=0|max(0,ln(x));if(lis(x)){const v=dkix(self.card.deck.brushes,x);if(v!=-1)n=24+v};return self.brush=n,x}
			if(ikey(i,'pattern'  ))return self.pattern=0|clamp(0,ln(x),255),x
			if(ikey(i,'font'     ))return self.font=normalize_font(self.card.deck.fonts,x),x
			if(!lis(i)){const img=container_image(self,1);return img.f(img,i,x)}
			if(self.free)return x
			if(ikey(i,'border'   ))return self.border=lb(x),x
			if(ikey(i,'draggable'))return self.draggable=lb(x),x
			if(ikey(i,'lsize'    )){i=lms('size'),x=lmpair(rmul(getpair(x),ln(ifield(self,'scale'))))}
			if(ikey(i,'size'     )){canvas_resize(self,getpair(x))}
			if(ikey(i,'scale'    )){return self.scale=max(0.1,ln(x)),canvas_resize(self,getpair(ifield(self,'size'))),x}
		}else{
			if(!lis(i)){const img=container_image(self,0);return img?img.f(img,i,x):NIL}
			if(ikey(i,'border'   ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'draggable'))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'brush'    ))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'pattern'  ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'size'     ))return lmpair(ivalue(self,ls(i),rect(100,100)))
			if(ikey(i,'scale'    ))return lmn(ivalue(self,ls(i),1.0))
			if(ikey(i,'lsize'    )){const s=getpair(ifield(self,'size')),z=ln(ifield(self,'scale'));return lmpair(rect(ceil(s.x/z),ceil(s.y/z)))}
			if(ikey(i,'clip'     ))return lmnat(z=>(canvas_clip(self,z),self))
			if(ikey(i,'clear'    ))return lmnat(z=>(canvas_pick(self),draw_rect(wid_crect(self,z),0            )                          ,self))
			if(ikey(i,'rect'     ))return lmnat(z=>(canvas_pick(self),draw_rect(wid_crect(self,z),frame.pattern)                          ,self))
			if(ikey(i,'invert'   ))return lmnat(z=>(canvas_pick(self),draw_invert_raw(wid_pal(self),wid_crect(self,z))                    ,self))
			if(ikey(i,'box'      ))return lmnat(z=>(canvas_pick(self),draw_boxf(wid_rect(self,z),frame.brush,frame.pattern,self.card.deck),self))
			if(ikey(i,'poly'     ))return lmnat(z=>(canvas_pick(self),draw_poly(unpack_poly(z),frame.pattern)                             ,self))
			if(ikey(i,'line'     ))return lmnat(z=>(canvas_pick(self),draw_lines(unpack_poly(z),frame.brush,frame.pattern,self.card.deck) ,self))
			if(ikey(i,'fill'     ))return lmnat(([pos])=>(canvas_pick(self),draw_fill(rint(getpair(pos)),self.pattern)                    ,self))
			if(ikey(i,'copy'     ))return lmnat(z=>{const img=container_image(self,1);return image_copy(img,unpack_rect(z,img.size))})
			if(ikey(i,'paste'    ))return lmnat(([img,pos,t])=>{
				canvas_pick(self);const dst=container_image(self,1)
				img=getimage(img),pos=(pos?ll(pos):[]).map(ln); let solid=t?!lb(t):1
				image_paste_scaled(pos.length<=2?rect(pos[0],pos[1],img.size.x,img.size.y):rect(pos[0],pos[1],pos[2],pos[3]),frame.clip,img,dst,solid)
				return self
			})
			if(ikey(i,'merge'))return lmnat(z=>{
				canvas_pick(self)
				if(lis(z[0])){if(image_is(z[1]))image_merge_op(frame.image,z[1],ls(z[0])[0]);return self}
				if(lil(z[0]))z=ll(z[0]);const nice=x=>x&&image_is(x)&&x.size.x>0&&x.size.y>0, s=frame.image.size
				const v=new Uint8Array(256),sx=new Uint32Array(256),sy=new Uint32Array(256)
				for(let p=0;p<z.length&&p<256;p++)if(nice(z[p]))v[p]=1,sx[p]=z[p].size.x,sy[p]=z[p].size.y
				for(let y=0;y<s.y;y++)for(let x=0;x<s.x;x++){const h=rect(x,y);if(inclip(h)){const p=gpix(h),c=v[p]?z[p].pix[(x%sx[p])+(y%sy[p])*sx[p]]:0;pix(h,c)}}
				return self
			})
			if(ikey(i,'segment'))return lmnat(([img,x,y])=>{
				canvas_pick(self);if(!image_is(img))return self;const r=rint(getrect(x)),m=normalize_margin(y,img.size)
				r.w=max(r.w,m.x+m.w),r.h=max(r.h,m.y+m.h),draw_9seg(r,frame.image,img,m,frame.clip,0,null);return self
			})
			if(ikey(i,'text'))return lmnat(([x,pos,a])=>(canvas_pick(self),text(x=lit(x)?rtext_cast(x):lms(ls(x)),pos,a),self))
			if(ikey(i,'textsize'))return lmnat(([x,wid])=>{
				const l=layout_richtext(self.card.deck,rtext_cast(x||lms('')),ifield(self,'font'),ALIGN.left,wid?ln(wid):RTEXT_END)
				if(!wid)l.size.x=l.lines.reduce((x,y)=>max(x,y.pos.x+y.pos.w),0);return lmpair(l.size)
			})
		}return interface_widget(self,i,x)
	},'canvas');ri.card=card
	ri.card=card
	{const v=dget(x,lms('image'));if(v)ri.image=image_read(ls(v)),iwrite(ri,lms('size'),lmpair(ri.image.size))}
	{const v=dget(x,lms('clip' ));if(v)canvas_clip(ri,ll(v))}
	{const v=dget(x,lms('size' ));if(v)ri.size=getpair(v)}
	{const v=dget(x,lms('scale'));if(v)ri.scale=max(0.1,ln(v))}
	init_field(ri,'border'   ,x)
	init_field(ri,'draggable',x)
	init_field(ri,'brush'    ,x)
	init_field(ri,'pattern'  ,x)
	init_field(ri,'font'     ,x)
	return ri
}
canvas_write=x=>{
	const r=lmd([lms('type')],[lms('canvas')])
	if(x.border!=undefined)dset(r,lms('border'),lmn(x.border))
	if(x.image&&!is_blank(x.image)&&!x.volatile)dset(r,lms('image'),lms(image_write(x.image)))
	if(x.draggable)dset(r,lms('draggable'),lmn(x.draggable))
	if(x.brush    )dset(r,lms('brush'    ),lmn(x.brush))
	if(x.pattern!=undefined&&x.pattern!=1)dset(r,lms('pattern'),lmn(x.pattern))
	if(x.scale)dset(r,lms('scale'),lmn(x.scale))
	if(x.clip&&!requ(x.clip,rpair(rect(),getpair(ifield(x,'lsize')))))dset(r,lms('clip'),lml([x.clip.x,x.clip.y,x.clip.w,x.clip.h].map(lmn)))
	return r
}
contraption_read=(x,card)=>{
	x=ld(x);const dname=dget(x,lms('def')), def=dname?dget(card.deck.contraptions,dname):null;if(!def)return null
	const corner_reflow=(p,s,m,d)=>rect(
		(p.x<m.x)?p.x: (p.x>s.x-m.w)?d.x-(s.x-p.x): s.x==0?0:Math.round((p.x/s.x)*d.x), // left | right  | stretch horiz
		(p.y<m.y)?p.y: (p.y>s.y-m.h)?d.y-(s.y-p.y): s.y==0?0:Math.round((p.y/s.y)*d.y)  // top  | bottom | stretch vert
	)
	const reflow=c=>{
		const def=c.def, swids=def.widgets, dwids=c.widgets, m=def.margin, s=def.size, d=getpair(ifield(c,'size'))
		swids.k.map((k,i)=>{
			const swid=swids.v[i],dwid=dget(dwids,k);if(!dwid)return
			let a=getpair(ifield(swid,'pos')), b=radd(getpair(ifield(swid,'size')),a)
			a=corner_reflow(a,s,m,d), b=corner_reflow(b,s,m,d)
			iwrite(dwid,lms('pos'),lmpair(a)),iwrite(dwid,lms('size'),lmpair(rsub(b,a)))
		})
	}
	const masks={name:1,index:1,image:1,script:1,locked:1,animated:1,volatile:1,pos:1,show:1,font:1,toggle:1,event:1,offset:1,parent:1}
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NIL
		if(x){
			if(ikey(i,'def'  ))return x // not mutable!
			if(ikey(i,'image'))return x // not mutable!
			if(ikey(i,'size' )){
				const m=self.def.margin
				return self.size=rmax(rect(m.x+m.w,m.y+m.h),getpair(x)),reflow(self),x
			}
			if(lis(i)&&masks.hasOwnProperty(ls(i)))return interface_widget(self,i,x)
			return fire_attr_sync(self,'set_'+ls(i),x),x
		}else{
			if(ikey(i,'def'  ))return self.def
			if(ikey(i,'size' ))return lmpair((def.resizable?self.size:def.size)||def.size)
			if(ikey(i,'image'))return ifield(self.def,'image')
			if(lis(i)&&masks.hasOwnProperty(ls(i)))return interface_widget(self,i,x)
			return fire_attr_sync(self,'get_'+ls(i),null)
		}
	},'contraption')
	ri.card   =card
	ri.deck   =card.deck
	ri.def    =def
	ri.widgets=lmd()
	let w=dget(x,lms('widgets')),d=def.widgets;if(w){w=ld(w)}else{w=lmd();def.widgets.k.map(k=>dset(w,k,lmd()))}
	d.k.map((k,i)=>{const a=widget_write(d.v[i]),o=dget(w,k);widget_add(ri,o?dyad[','](a,o):a)})
	{const k=lms('size'),v=dget(x,k);iwrite(ri,k,v?v:ifield(def,'size'))}
	{ri.viewproxy=lmi(interface_widget,'proxy'),ri.viewproxy.card=ri}
	return ri
}
contraption_write=x=>{
	const wids=lmd(), r=lmd(['type','def','widgets'].map(lms),[lms('contraption'),ifield(x.def,'name'),wids])
	const dict_delta=(a,b)=>{const r=lmd();b.k.map((k,i)=>{const av=dget(a,k),bv=b.v[i];if(!av||!match(av,bv))dset(r,k,bv)});return r}
	x.widgets.v.map(w=>{
		let wid=widget_write(w), n=ifield(w,'name'), src=dget(x.def.widgets,n)
		dset(wids,n,dyad.drop(lms('name'),src?dict_delta(widget_write(src),wid):wid))
	});return r
}
widget_shows={solid:1,invert:1,transparent:1,none:1}
interface_widget=(self,i,x)=>{
	widget_rename=(card,a,b)=>{const w=card.widgets,i=dkix(w,a);w.k[i]=b,w.v[i].name=ls(b)}
	if(x){
		if(ikey(i,'name'    ))return widget_rename(self.card,lms(self.name),ukey(self.card.widgets,lms(ls(x)),ls(x),lms(self.name))),x
		if(ikey(i,'index'   ))return reorder(self.card.widgets,dvix(self.card.widgets,self),ln(x)),x
		if(ikey(i,'font'    ))return self.font=normalize_font(self.card.deck.fonts,x),x
		if(ikey(i,'script'  ))return self.script=ls(x),x
		if(ikey(i,'locked'  ))return self.locked=lb(x),x
		if(ikey(i,'animated'))return self.animated=lb(x),x
		if(ikey(i,'volatile'))return self.volatile=lb(x),x
		if(ikey(i,'size'    ))return self.size=rint(rclamp(rect(),getpair(x),rect(4096,4096))),x
		if(ikey(i,'pos'     ))return self.pos=rint(getpair(x)),x
		if(ikey(i,'show'    ))return self.show=normalize_enum(widget_shows,ls(x)),x
	}else{
		if(ikey(i,'name'    ))return lms(self.name)
		if(ikey(i,'index'   ))return lmn(dvix(self.card.widgets,self))
		if(ikey(i,'script'  ))return lms(ivalue(self,ls(i),''))
		if(ikey(i,'locked'  ))return lmn(ivalue(self,ls(i),0))
		if(ikey(i,'animated'))return lmn(ivalue(self,ls(i),0))
		if(ikey(i,'volatile'))return lmn(ivalue(self,ls(i),0))
		if(ikey(i,'pos'     ))return lmpair(ivalue(self,ls(i),rect()))
		if(ikey(i,'show'    ))return lms(ivalue(self,ls(i),'solid'))
		if(ikey(i,'font'    ))return dget(self.card.deck.fonts,lms(ivalue(self,ls(i),button_is(self)?'menu':'body')))
		if(ikey(i,'event'   ))return lmnat(args=>n_event(self,args))
		if(ikey(i,'parent'  ))return self.card
		if(ikey(i,'toggle'  ))return lmnat(([s,v])=>{
			const a=v==undefined;s=s||lms('solid'),v=v||ZERO;const o=ifield(self,'show'),n=lms('none')
			const r=(a?match(o,n):(lb(v)&&!match(v,n)))?s:n;iwrite(self,lms('show'),r);return r
		})
		if(ikey(i,'offset')){
			let c=getpair(ifield(self.card,'size')), p=getpair(ifield(self,'pos')), d=self.card.deck.size, con=self.card
			while(contraption_is(con)){p=radd(p,getpair(ifield(con,'pos'))),con=con.card,c=getpair(ifield(con,'size'))}
			return lmpair(radd(p,rcenter(rect(0,0,d.x,d.y),c)))
		}
	}return x?x:NIL
}
widget_read=(x,card)=>{
	const type=ls(dget(x,lms('type'))||lms('button')), tname=type=='contraption'?ls(dget(x,lms('def'))||lms(type)):type
	const ctors={button:button_read,field:field_read,slider:slider_read,grid:grid_read,canvas:canvas_read,contraption:contraption_read}
	const ri=(ctors[type]||button_read)(ld(x),card);if(!lii(ri))return null
	ri.name=ls(ukey(card.widgets,dget(x,lms('name')),tname))
	init_field(ri,'size'    ,x)
	init_field(ri,'script'  ,x)
	init_field(ri,'font'    ,x)
	init_field(ri,'locked'  ,x)
	init_field(ri,'animated',x)
	init_field(ri,'volatile',x)
	init_field(ri,'pos'     ,x)
	init_field(ri,'show'    ,x)
	return ri
}
widget_write=x=>{
	const r=lmd()
	dset(r,lms('name'),lms(x.name))
	dset(r,lms('type'),lms(x.n))
	dset(r,lms('size'),ifield(x,'size'))
	dset(r,lms('pos' ),ifield(x,'pos' ))
	if(x.size    )dset(r,lms('size'    ),lmpair(x.size))
	if(x.pos     )dset(r,lms('pos'     ),lmpair(x.pos))
	if(x.locked  )dset(r,lms('locked'  ),lmn(x.locked))
	if(x.animated)dset(r,lms('animated'),lmn(x.animated))
	if(x.volatile)dset(r,lms('volatile'),lmn(x.volatile))
	if(x.script  )dset(r,lms('script'  ),lms(x.script))
	if(x.font&&x.font!=(button_is(x)?"menu":"body"))dset(r,lms('font'),lms(x.font))
	if(x.show&&x.show!='solid')dset(r,lms('show'),lms(x.show))
	return dyad[','](r,button_is(x)?button_write(x): field_is (x)?field_write (x):slider_is(x)?slider_write(x):
	                   grid_is  (x)?grid_write  (x): canvas_is(x)?canvas_write(x):contraption_is(x)?contraption_write(x): lmd())
}

widget_strip=x=>dyad.take(lml([lms('name'),lms('type')]),x)
widget_add=(card,x)=>{const r=widget_read(x,card);if(lii(r))dset(card.widgets,ifield(r,'name'),r);return r}
card_add=(card,type,name,n2)=>{
	if(prototype_is(card)&&(contraption_is(type)||ls(type)=='contraption'))return NIL
	if(lis(type)){
		if(ls(type)=='contraption'){
			const defs=card.deck.contraptions, ct=lms(name?ls(name):''), def=dget(defs,ct);if(!def)return NIL
			const a=lmd(['type','def'].map(lms),[lms('contraption'),ct]);if(n2)dset(a,lms('name'),lms(ls(n2)));return widget_add(card,a)
		}
		if(!ls(type)in{button:1,field:1,slider:1,canvas:1,grid:1})return NIL
		const a=lmd([lms('type')],[type]);if(name)dset(a,lms('name'),lms(ls(name)));return widget_add(card,a)
	}
	if(widget_is(type)){const a=widget_write(type);if(name)dset(a,lms('name'),name);return widget_add(card,a)}
	return NIL
}
card_remove=(card,x)=>{
	if(lil(x)||lid(x))return x.v.reduce((x,y)=>x&card_remove(card,y),1)
	if(!widget_is(x)||!dkey(card.widgets,x))return 0
	const name=ifield(x,'name');dget(card.widgets,name).dead=true,card.widgets=dyad.drop(name,card.widgets);return 1
}
con_copy_raw=(card,z)=>z.filter(w=>widget_is(w)&&w.card==card).map(widget_write)
con_paste_raw=(card,payload)=>payload.map(p=>widget_add(card,ld(p)))
find_fonts=(deck,target,widgets)=>{
	let fonts=lmd([],[]);widgets.filter(widget_is).map(wid=>{
		if(wid.font)dset(fonts,lms(wid.font),dget(deck.fonts,lms(wid.font))) // directly on widgets
		if(contraption_is(wid)){ // inside contraption instances
			wid.widgets.v.map(w=>ifield(w,'font')).map(f=>dset(fonts,dkey(deck.fonts,f),f))
		}
		if(field_is(wid)&&match(ifield(wid,'style'),lms('rich'))){ // inside rtext field values
			tab_get(ifield(wid,'value'),'font').filter(n=>count(n)&&!dget(fonts,n)&&dget(deck.fonts,n)).map(n=>dset(fonts,n,dget(deck.fonts,n)))
		}
	})
	fonts=dyad.drop(lml(['body','menu','mono'].map(lms)),fonts),fonts.v.map((x,i)=>{fonts.v[i]=lms(font_write(x))})
	if(count(fonts))dset(target,lms('f'),fonts)
}
merge_fonts=(deck,f)=>{
	if(!f)return;f=ld(f)
	f.v.map((x,i)=>{const k=f.k[i],v=font_read(ls(x));if(font_is(v)&&!dget(deck.fonts,k))dset(deck.fonts,k,v)})
}
con_copy=(card,z)=>{
	z=lil(z)||lid(z)?ll(z):[z];
	const wids=lml(con_copy_raw(card,z)),defs=lmd(),v=lmd(['w','d'].map(lms),[wids,defs])
	const condefs=card.deck.contraptions;find_fonts(card.deck,v,z),wids.v.map(wid=>{
		const type=dget(wid,lms('type')),def=dget(wid,lms('def'))
		if(ls(type)=='contraption'&&dget(defs,def)==null)dset(defs,def,prototype_write(dget(condefs,def)))
	});return lms(`%%WGT0${fjson(v)}`)
}
merge_prototypes=(deck,defs,uses)=>{
	const condefs=deck.contraptions;defs.v.map(def=>{
		const name=dget(def,lms('name'));let desc=dget(def,lms('description'));if(!lis(name))return;if(!desc)desc=lms('')
		const ver=dget(def,lms('version')),version=ver?ln(ver):0.0
		if(condefs.v.some(con=>{
			const r=match(name,ifield(con,'name'))&&match(desc,ifield(con,'description'))
			if(r&&ln(ifield(con,'version'))<version)deck_add(deck,prototype_read(def,deck_read('')))
			return r
		}))return
		const p=prototype_read(def,deck),nn=ifield(p,'name');dset(condefs,nn,p)
		uses.map(wid=>{
			const type=dget(wid,lms('type')),def=dget(wid,lms('def'))
			if(lis(type)&&ls(type)=='contraption'&&lis(def)&&ls(def)==ls(name))dset(wid,lms('def'),nn)
		})
	})
}
con_paste=(card,z)=>{
	if(!lis(z)||!z.v.startsWith('%%WGT0'))return NIL
	const v=ld(pjson(ls(z),6,count(z)-6).value),defs=dget(v,lms('d'));let wids=dget(v,lms('w'));wids=wids?ll(wids):[]
	merge_fonts(card.deck,dget(v,lms('f'))),merge_prototypes(card.deck,defs?ld(defs):lmd(),wids);return lml(con_paste_raw(card,wids))
}
card_read=(x,deck,cdata)=>{
	x=ld(x);const nav_dirs={right:1,left:1,up:1,down:1},ri=lmi((self,i,x)=>{
		if(self.dead)return NIL
		if(x){
			if(ikey(i,'name')){
				if(ls(x).length==0)return x;const n=ukey(deck.cards,lms(ls(x)),ls(x),lms(self.name))
				deck.cards.k[dvix(deck.cards,self)]=n,self.name=ls(n);return x
			}
			if(ikey(i,'script'))return self.script=ls(x),x
			if(ikey(i,'image' ))return self.image=image_is(x)?x:image_make(rect()),x
			if(ikey(i,'index'))return reorder(self.deck.cards,dvix(self.deck.cards,self),ln(x)),self.deck.history=[ln(ifield(self,'index'))],x
		}else{
			if(ikey(i,'name'   ))return lms(self.name)
			if(ikey(i,'size'   ))return lmpair(deck.size)
			if(ikey(i,'index'  ))return lmn(dvix(deck.cards,self))
			if(ikey(i,'script' ))return lms(self.script||'')
			if(ikey(i,'widgets'))return self.widgets
			if(ikey(i,'parent' ))return self.deck
			if(ikey(i,'image'  ))return self.image
			if(ikey(i,'add'    ))return lmnat(([t,n1,n2])=>card_add(self,t,n1,n2))
			if(ikey(i,'remove' ))return lmnat(([x])=>lmn(card_remove(self,x)))
			if(ikey(i,'event'  ))return lmnat(args=>n_event(self,args))
			if(ikey(i,'copy'   ))return lmnat(([z])=>con_copy(self,z))
			if(ikey(i,'paste'  ))return lmnat(([z])=>con_paste(self,z))
		}return x?x:NIL
	},'card')
	const n=dget(x,lms('name'))
	ri.deck=deck
	ri.widgets=lmd()
	ri.name=ls(ukey(deck.cards,n&&lis(n)&&count(n)==0?null:n,'card'))
	ri.script=ls(dget(x,lms('script'))||lms(''))
	{const v=dget(x,lms('image'));ri.image=v?image_read(ls(v)):image_make(deck.size)}
	ll(dget(x,lms('widgets'))||lml([])).filter(w=>dget(w,lms('name'))).map(w=>{const i=widget_read(w,ri);if(lii(i))dset(ri.widgets,ifield(i,'name'),i)})
	return ri
}
card_write=card=>{
	const r=lmd(),wids=lmd()
	dset(r,lms('name'),lms(card.name)),dset(r,lms('widgets'),wids)
	if(card.script.length)dset(r,lms('script'),lms(card.script))
	if(card.image&&!is_blank(card.image))dset(r,lms('image'),lms(image_write(card.image)))
	card.widgets.k.map((k,i)=>{
		let wid=widget_write(card.widgets.v[i]),n=dget(wid,lms('name'))
		wid=dyad.drop(lms('name'),wid)
		if(count(wid))dset(wids,n,wid)
	});return r
}
contraption_update=(deck,def)=>{
	const contraption_strip=x=>{
		const r=lmd();x.widgets.v.map(w=>{
			let p=widget_write(w)
			if(button_is(w))p=dyad.take(lms('value'),p)
			if(slider_is(w))p=dyad.take(lms('value'),p)
			if(field_is (w))p=dyad.take(lml(['value','scroll'].map(lms)),p)
			if(grid_is  (w))p=dyad.take(lml(['value','scroll','row','col'].map(lms)),p)
			if(canvas_is(w))p=dyad.take(lms('image'),p)
			dset(r,ifield(w,'name'),p)
		});return r
	}
	deck.cards.v.map(card=>{
		card.widgets.v.filter(x=>contraption_is(x)&&x.def==def).map(widget=>{
			const d=widget_write(widget), n=widget.name
			dset(d,lms('widgets'),contraption_strip(widget))
			for(var k in widget)delete widget[k];Object.assign(widget,widget_read(d,card));widget.name=n
		})
	})
}
normalize_margin=(x,s)=>{
	const m=rint(getrect(x))
	return rmax(rect(min(m.x,s.x),min(m.y,s.y),min(m.w,s.x-m.x),min(m.h,s.y-m.y)),rect(0,0,0,0))
}
prototype_read=(x,deck)=>{
	x=ld(x)
	const attribute_types={'':1,bool:1,number:1,string:1,code:1,rich:1}
	const normalize_attributes=x=>{
		const r=lmt();tab_set(r,'name',[]),tab_set(r,'label',[]),tab_set(r,'type',[]);if(!lit(x))return r
		const sn=tab_get(x,'name'),sl=tab_get(x,'label')||sn,st=tab_get(x,'type')
		if(sn&&st)sn.filter(n=>lis(n)&&count(n)).map((n,i)=>{
			const type=normalize_enum(attribute_types,ls(st[i]))
			if(type.length)tab_get(r,'name').push(n),tab_get(r,'label').push(lms(ls(sl[i]))),tab_get(r,'type').push(lms(type))
		});return r
	}
	const prototype_pos=self=>lmpair(rcenter(rect(0,0,deck.size.x,deck.size.y),self.size))
	const ri=lmi((self,i,x)=>{
		if(self.dead)return NIL
		if(x){
			if(ikey(i,'name')){
				const defs=self.deck.contraptions, o=self.name, n=ukey(defs,lms(ls(x)),ls(x),lms(o))
				defs.k[dvix(defs,self)]=n,self.name=ls(n);return x
			}
			if(ikey(i,'description'))return self.description=ls(x),x
			if(ikey(i,'version'    ))return self.version=ln(x),x
			if(ikey(i,'size'       ))return self.size=rint(getpair(x)),contraption_update(deck,self),x
			if(ikey(i,'margin'     ))return self.margin=normalize_margin(x,getpair(ifield(self,'size'))),contraption_update(deck,self),x
			if(ikey(i,'resizable'  ))return self.resizable=lb(x),contraption_update(deck,self),x
			if(ikey(i,'image'      ))return self.image=image_is(x)?x:image_make(rect(0,0)),x
			if(ikey(i,'script'     ))return self.script=ls(x),x
			if(ikey(i,'template'   ))return self.template=ls(x),x
			if(ikey(i,'attributes' ))return self.attributes=normalize_attributes(x),x
		}else{
			if(ikey(i,'name'       ))return lms(self.name)
			if(ikey(i,'description'))return lms(self.description||'')
			if(ikey(i,'version'    ))return lmn(self.version||0.0)
			if(ikey(i,'script'     ))return lms(self.script||'')
			if(ikey(i,'template'   ))return lms(self.template||'')
			if(ikey(i,'font'       ))return monad.first(self.deck.fonts)
			if(ikey(i,'show'       ))return lms('solid')
			if(ikey(i,'parent'     ))return ifield(self.deck,'card')
			if(ikey(i,'size'       ))return lmpair(self.size)
			if(ikey(i,'margin'     ))return lmrect(self.margin)
			if(ikey(i,'resizable'  ))return lmn(self.resizable)
			if(ikey(i,'image'      ))return self.image
			if(ikey(i,'widgets'    ))return self.widgets
			if(ikey(i,'attributes' ))return self.attributes||normalize_attributes(NIL)
			if(ikey(i,'offset'     ))return prototype_pos(self)
			if(ikey(i,'pos'        ))return prototype_pos(self)
			if(ikey(i,'add'        ))return lmnat(([t,n1,n2])=>{const r=card_add(self,t,n1,n2);if(widget_is(r))contraption_update(deck,self);return r})
			if(ikey(i,'remove'     ))return lmnat(([x])=>{const r=card_remove(self,x);if(lb(r))contraption_update(deck,self);return r})
			if(ikey(i,'update'     ))return lmnat(_=>{contraption_update(deck,self);return NIL})
		}return x?x:NIL
	},'prototype')
	ri.deck   =deck
	ri.widgets=lmd()
	{const v=dget(x,lms('name'      ));ri.name=ls(ukey(deck.contraptions,v&&lis(v)&&count(v)==0?null:v,'prototype'))}
	{const v=dget(x,lms('attributes'));if(v)iwrite(ri,lms('attributes'),monad.table(v))}
	{const v=dget(x,lms('size'      ));ri.size=v?rint(getpair(v)):rect(100,100)}
	{const v=dget(x,lms('image'     ));ri.image=v?image_read(ls(v)):image_make(ri.size)}
	{const v=dget(x,lms('resizable' ));ri.resizable=v?lb(v):0}
	let w=dget(x,lms('widgets'));if(lid(w)){w.v.map((v,i)=>dset(v,lms('name'),w.k[i]))}
	(w?ll(w):[]).map(w=>{const n=dget(w,lms('name'));if(n){const i=widget_read(w,ri);if(lii(i))dset(ri.widgets,ifield(i,'name'),i)}})
	init_field(ri,'description',x)
	init_field(ri,'version'    ,x)
	init_field(ri,'script'     ,x)
	init_field(ri,'template'   ,x)
	ri.margin=normalize_margin(dget(x,lms('margin'))||NIL,getpair(ifield(ri,'size')))
	return ri
}
prototype_write=x=>{
	const r=lmd(), wids=lmd(), nice=x=>x&&image_is(x)&&x.size.x>0&&x.size.y>0&&!is_blank(x)
	dset(r,lms('name'),lms(x.name))
	dset(r,lms('size'),ifield(x,'size'))
	if(x.resizable)dset(r,lms('resizable'),ONE)
	dset(r,lms('margin'),ifield(x,'margin'))
	if(x.description&&x.description.length)dset(r,lms('description'),lms(x.description))
	if(x.version    &&x.version!=0.0      )dset(r,lms('version'    ),lmn(x.version    ))
	if(x.script     &&x.script.length     )dset(r,lms('script'     ),lms(x.script     ))
	if(x.template   &&x.template.length   )dset(r,lms('template'   ),lms(x.template   ))
	if(nice(x.image)                      )dset(r,lms('image'      ),lms(image_write(x.image)))
	if(x.attributes                       )dset(r,lms('attributes' ),monad.cols(x.attributes))
	x.widgets.v.map(v=>{
		let wid=widget_write(v),n=dget(wid,lms('name'))
		wid=dyad.drop(lms('name'),wid);if(count(wid))dset(wids,n,wid)
	}),dset(r,lms('widgets'),wids);return r
}
rename_sound=(deck,sound,name)=>{
	const sounds=deck.sounds,oldname=dkey(sounds,sound)
	sounds.k[dkix(sounds,oldname)]=ukey(sounds,name,ls(name),oldname)
}
deck_add=(deck,type,y,z)=>{
	const unpack_name=x=>x?lms(ls(x)):NIL
	if(font_is(type))return uset(deck.fonts,unpack_name(y),'font',font_read(font_write(type)))
	if(ikey(type,'font'))return uset(deck.fonts,unpack_name(z),'font',font_read(getpair(y)))
	if(sound_is(type))return uset(deck.sounds,unpack_name(y),'sound',sound_read(sound_write(type)))
	if(ikey(type,'sound'))return uset(deck.sounds,unpack_name(z),'sound',sound_read(y?ln(y):0))
	if(module_is(type)){
		if((!y)&&dget(deck.modules,ifield(type,'name'))){
			deck_remove(deck,dget(deck.modules,ifield(type,'name')))
			const r=module_read(module_write(type),deck);
			return dset(deck.modules,ifield(r,'name'),r),r
		}else{
			const a=module_write(type);if(y)dset(a,lms('name'),lms(ls(y)));
			const r=module_read(a,deck);return dset(deck.modules,ifield(r,'name'),r),r
		}
	}
	if(ikey(type,'module')){const a=lmd();if(y)dset(a,lms('name'),lms(ls(y)));const r=module_read(a,deck);return dset(deck.modules,ifield(r,'name'),r),r}
	if(card_is(type))return deck_paste(deck,deck_copy(deck,type),y?lms(ls(y)):null)
	if(ikey(type,'card')){const a=lmd();if(y)dset(a,lms('name'),lms(ls(y)));const r=card_read(a,deck);return dset(deck.cards,ifield(r,'name'),r),r}
	if(prototype_is(type)){
		if((!y)&&dget(deck.contraptions,ifield(type,'name'))){
			const name=ifield(type,'name'),r=dget(deck.contraptions,name)
			for(var k in r)delete r[k];Object.assign(r,prototype_read(prototype_write(type),deck));r.name=ls(name)
			contraption_update(deck,r);return r
		}else{
			const a=prototype_write(type);if(y)dset(a,lms('name'),lms(ls(y)));
			const r=prototype_read(a,deck);return dset(deck.contraptions,ifield(r,'name'),r),r
		}
	}
	if(ikey(type),'contraption'){const a=lmd();if(y)dset(a,lms('name'),lms(ls(y)));const r=prototype_read(a,deck);return dset(deck.contraptions,ifield(r,'name'),r),r}
	return NIL
}
deck_remove=(deck,t)=>{
	if(widget_is(t)&&is_rooted(t))return card_remove(t.card,t)
	if(patterns_is(t)){const r=patterns_read({});return t.pal=r.pal,t.anim=r.anim,1}
	if(module_is(t)){const k=dkey(deck.modules,t);if(k)return deck.modules=dyad.drop(k,deck.modules),1}
	if(sound_is(t)){const k=dkey(deck.sounds,t);if(k)return deck.sounds=dyad.drop(k,deck.sounds),1}
	if(font_is(t)){
		const k=dkey(deck.fonts,t);if(!k||ls(k)in{body:1,menu:1,mono:1})return 0
		const remove=w=>w.v.map(w=>{if(w.font==ls(k))w.font='body';if(contraption_is(w))remove(w.widgets)})
		deck.cards.v.map(c=>remove(c.widgets))
		deck.contraptions.v.map(c=>remove(c.widgets))
		return deck.fonts=dyad.drop(k,deck.fonts),1
	}
	if(prototype_is(t)){
		const k=dkey(deck.contraptions,t);if(!k)return 0
		deck.cards.v.map(card=>card.widgets.v.filter(w=>contraption_is(w)&&w.def==t).map(w=>card_remove(card,w)))
		return deck.contraptions=dyad.drop(k,deck.contraptions),t.dead=true,1
	}
	if(card_is(t)){
		if(count(deck.cards)<=1)return 0
		deck.cards=dyad.drop(dkey(deck.cards,t)||NIL,deck.cards),t.dead=true
		if(deck.card>=count(deck.cards))deck.card=count(deck.cards-1)
		deck.history=[ln(ifield(ifield(deck,'card'),'index'))]
		return 1
	}return 0
}
deck_copy=(deck,z)=>{
	if(!card_is(z))return NIL;const defs=lmd(),v=lmd(['c','d'].map(lms),[card_write(z),defs]);find_fonts(deck,v,z.widgets.v)
	z.widgets.v.filter(contraption_is).map(wid=>{const d=wid.def,n=ifield(d,'name');if(dget(defs,n)==null)dset(defs,n,prototype_write(d))})
	return lms(`%%CRD0${fjson(v)}`)
}
deck_paste=(deck,z,name)=>{
	if(!lis(z)||!ls(z).startsWith('%%CRD0'))return NIL
	const v=ld(pjson(ls(z),6,count(z)-6).value);let payload=dget(v,lms('c')),defs=dget(v,lms('d'));payload=payload?ld(payload):lmd()
	const wids=dget(payload,lms('widgets'));if(wids&&lid(wids))wids.v.map((v,i)=>dset(v,lms('name'),wids.k[i]))
	merge_fonts(deck,dget(v,lms('f')))
	merge_prototypes(deck,defs?ld(defs):lmd(),wids?ll(wids):[]);const r=card_read(payload,deck);dset(deck.cards,name||ifield(r,'name'),r);return r
}
widget_purge=x=>{
	if(!x.volatile)return
	if(button_is(x))iwrite(x,lms('value'),ZERO)
	if(slider_is(x))iwrite(x,lms('value'),ZERO)
	if(field_is (x))iwrite(x,lms('value'),lms('')),iwrite(x,lms('scroll'),ZERO)
	if(grid_is  (x))iwrite(x,lms('value'),lmt()  ),iwrite(x,lms('scroll'),ZERO)
	if(canvas_is(x)){const t=frame;canvas_pick(x),draw_rect(frame.clip,0);frame=t}
	if(contraption_is(x))x.widgets.v.map(w=>widget_purge(w))
}
deck_purge=x=>{x.cards.v.map(c=>c.widgets.v.map(w=>widget_purge(w)))}
deck_read=x=>{
	const deck={},scripts=new Map(),cards={},modules={},defs={}, fonts=lmd(),sounds=lmd(); let i=0,m=0,md=0,lc=0
	Object.keys(FONTS).map(k=>dset(fonts,lms(k),font_read(FONTS[k])))
	const match=k=>x.startsWith(k,i)?(i+=k.length,1):0
	const end=_=>i>=x.length||x.startsWith('<\/script>',i)
	const str=e=>{let r='';while(!end()&&!match(e))r+=match('{l}')?'{': match('{r}')?'}': match('{c}')?':': match('{s}')?'/': x[i++];return clchars(r)}
	const last=dict=>{const k=Object.keys(dict);return dict[k[k.length-1]]||lmd()}
	match('<meta charset="UTF-8">');
	match('<body><script language="decker">');while(!end()){
		if(x[i]=='\n')i++
		else if(x[i]=='#')while(!end()&&x[i]!='\n')i++
		else if(match('{deck}\n'   ))m=1
		else if(match('{fonts}\n'  ))m=2
		else if(match('{sounds}\n' ))m=3
		else if(match('{widgets}\n'))m=4
		else if(match('{card:')){const k=str('}');cards['~'+k]=lmd(['name','widgets'].map(lms),[lms(k),lml([])]),m=5,lc=0}
		else if(match('{script:')){const k=str('}\n');scripts.set(k,str('\n{end}'))}
		else if(match('{module:')){const k=str('}');modules['~'+k]=lmd(['name','script','data'].map(lms),[lms(k),lms(''),lmd()]),m=6,md=0}
		else if(match('{contraption:')){const k=str('}');defs['~'+k]=lmd(['name','widgets'].map(lms),[lms(k),lml([])]),m=7,lc=1}
		else if(m==6&&match('{data}\n')){md=1}
		else if(m==6&&match('{script}\n')){dset(last(modules),lms('script'),lms(str('\n{end}'))),m=1}
		else{
			const k=str(':'),j=plove(x,i,x.length-i),v=j.value;i=j.index
			if(m==1)deck[k]=v
			if(m==2)dset(fonts,lms(k),font_read(ls(v)))
			if(m==3)dset(sounds,lms(k),sound_read(ls(v)))
			if(m==4&&!lc){if(Object.keys(cards).length)dget(last(cards),lms('widgets')).v.push(dset(ld(v),lms('name'),lms(k)))}
			if(m==4&& lc){if(Object.keys(defs ).length)dget(last(defs ),lms('widgets')).v.push(dset(ld(v),lms('name'),lms(k)))}
			if(m==5)dset(last(cards),lms(k),v)
			if(m==6)dset(md?dget(last(modules),lms('data')):last(modules),lms(k),v)
			if(m==7)dset(last(defs),lms(k),v)
		}
	}
	const dscript=x=>{const k=lms('script'),s=dget(x,k);if(s)dset(x,k,lms(scripts.get(ls(s))))}
	Object.values(cards).map(c=>{dscript(c),dget(c,lms('widgets')).v.map(dscript)})
	Object.values(defs ).map(c=>{dscript(c),dget(c,lms('widgets')).v.map(dscript)})
	const ri=lmi((self,i,x)=>{
		if(x){
			if(ikey(i,'locked'))return self.locked=lb(x),x
			if(ikey(i,'name'  ))return self.name=ls(x),x
			if(ikey(i,'author'))return self.author=ls(x),x
			if(ikey(i,'script'))return self.script=ls(x),x
			if(ikey(i,'card'  ))return n_go([x],self),x
		}else{
			if(ikey(i,'version' ))return lmn(self.version)
			if(ikey(i,'locked'  ))return lmn(self.locked)
			if(ikey(i,'name'    ))return lms(self.name)
			if(ikey(i,'author'  ))return lms(self.author)
			if(ikey(i,'script'  ))return lms(self.script)
			if(ikey(i,'patterns'))return self.patterns
			if(ikey(i,'sounds'  ))return dyad.drop(ZERO,self.sounds)
			if(ikey(i,'fonts'   ))return dyad.drop(ZERO,self.fonts)
			if(ikey(i,'cards'   ))return self.cards
			if(ikey(i,'modules' ))return self.modules
			if(ikey(i,'contraptions'))return self.contraptions
			if(ikey(i,'card'    ))return self.cards.v[min(count(self.cards)-1,self.card)]
			if(ikey(i,'add'     ))return lmnat(([x,y,z])=>deck_add(self,x,y,z))
			if(ikey(i,'remove'  ))return lmnat(([x])=>lmn(deck_remove(self,x)))
			if(ikey(i,'event'   ))return lmnat(args=>n_event(self,args))
			if(ikey(i,'copy'    ))return lmnat(([x])=>deck_copy(self,x))
			if(ikey(i,'paste'   ))return lmnat(([x])=>deck_paste(self,x))
			if(ikey(i,'purge'   ))return lmnat(()=>{deck_purge(self);return self})
			if(ikey(i,'encoded' ))return lms(deck_write(self))
		}return x?x:NIL
	},'deck')
	ri.fonts       =fonts
	ri.sounds      =sounds
	ri.contraptions=lmd()
	ri.cards       =lmd()
	ri.modules     =lmd()
	ri.transit     =lmd()
	ri.brushes     =lmd()
	ri.brusht      =lmd()
	ri.patterns    =patterns_read(deck)
	ri.version     =deck.hasOwnProperty('version')?ln(deck.version):1
	ri.locked      =deck.hasOwnProperty('locked' )?lb(deck.locked ):0
	ri.name        =deck.hasOwnProperty('name'   )?ls(deck.name   ):''
	ri.author      =deck.hasOwnProperty('author' )?ls(deck.author ):''
	ri.script      =deck.hasOwnProperty('script' )?scripts.get(ls(deck.script)):''
	ri.card        =deck.hasOwnProperty('card'   )?clamp(0,ln(deck.card),Object.keys(cards).length-1):0
	ri.size        =deck.hasOwnProperty('size'   )?rclamp(rect(8,8),getpair(deck.size),rect(4096,4096)):rect(512,342)
	if(Object.keys(cards).length==0)cards.home=lmd(['name'].map(lms),[lms('home')])
	const root=lmenv();constants(root),primitives(root,ri)
	pushstate(root),issue(root,parse(DEFAULT_TRANSITIONS));while(running())runop();popstate()
	Object.values(defs   ).map(x=>{const v=prototype_read(x,ri)      ;dset(ri.contraptions,ifield(v,'name'),v)})
	Object.values(cards  ).map(x=>{const v=card_read     (x,ri,cards);dset(ri.cards       ,ifield(v,'name'),v)})
	Object.values(modules).map(x=>{const v=module_read   (x,ri)      ;dset(ri.modules     ,ifield(v,'name'),v)})
	ri.history=[ln(ifield(ifield(ri,'card'),'index'))]
	return ri
}
deck_write=(x,html)=>{
	if(!deck_is(x))return '';let deck=x,scripts=lmd(),si=0,sci=0,r=(html?'<meta charset=\"UTF-8\"><body><script language=\"decker\">\n':'')+'{deck}\nversion:1\n'
	const esc_write=(id,x)=>{
		let c='\0',lc=c,r='';for(let z=0;z<x.length;z++){
			lc=c,c=x[z],r+=c=='{'?'{l}': c=='}'?'{r}': c==':'&&id?'{c}': c=='/'&&lc=='<'?'{s}': c
		}return r
	}
	const script_ref=(base,x,suff)=>{
		if(ls(x)=='undefined')throw new Error('welp')
		for(let z=0;z<scripts.v.length;z++)if(match(scripts.v[z],x))return scripts.k[z]
		const k=lms(base?`${base}.${sci}${suff||''}`:`${sci}${suff||''}`);sci++;dset(scripts,k,x);return k
	}
	const write_scripts=_=>{while(si<scripts.v.length)r+=`\n{script:${esc_write(1,ls(scripts.k[si]))}}\n${esc_write(0,ls(scripts.v[si++]))}\n{end}\n`}
	const write_line=(s,k,p,f)=>{const v=s[k];if(p(v))r+=`${k}:${fjson(f(v))}\n`}
	const write_key =(s,k,p,f)=>{const v=dget(s,lms(k));if(p(v))r+=`${k}:${fjson(f(v))}\n`}
	const write_dict=(k,x,f)=>r+=`${count(x)?k:''}${x.k.map((k,i)=>`${esc_write(1,ls(k))}:${flove(f(x.v[i]))}\n`).join('')}`
	const pp=patterns_write(x.patterns),pa=anims_write(x.patterns),da=dyad.parse(lms('%j'),lms(DEFAULT_ANIMS))
	let f=deck.fonts;const strip_fnt=n=>{const k=lms(n),v=dget(f,k);if(font_write(v)==FONTS[n])f=dyad.drop(lml([k]),f)}
	strip_fnt('body'),strip_fnt('menu'),strip_fnt('mono')
	write_line(x,'card'      ,x=>1                                  ,lmn                       )
	write_line(x,'size'      ,x=>1                                  ,lmpair                    )
	write_line(x,'locked'    ,x=>x                                  ,lmn                       )
	write_line(x,'script'    ,x=>x.length                           ,x=>script_ref(null,lms(x)))
	write_line(x,'name'      ,x=>x.length                           ,lms                       )
	write_line(x,'author'    ,x=>x.length                           ,lms                       )
	write_line(x,'patterns'  ,x=>pp!=DEFAULT_PATTERNS               ,x=>lms(pp)                )
	write_line(x,'animations',x=>!match(pa,da)                      ,x=>pa                     )
	write_scripts()
	write_dict('\n{fonts}\n',f,x=>lms(font_write(x)))
	write_dict('\n{sounds}\n',deck.sounds,x=>lms(sound_write(x)))
	deck.cards.v.map(c=>{
		const data=card_write(c),wids=dget(data,lms('widgets')),base=ls(dget(data,lms('name')));sci=0
		r+=`\n{card:${esc_write(1,base)}}\n`
		write_key(data,'image' ,x=>x ,x=>x                 )
		write_key(data,'script',count,x=>script_ref(base,x))
		wids.v.map(wid=>{const k=lms('script'),v=dget(wid,k);if(v)dset(wid,k,script_ref(base,v))})
		write_dict('{widgets}\n',wids,x=>x)
		write_scripts()
	})
	deck.modules.v.map(m=>{
		const data=module_write(m)
		r+=`\n{module:${esc_write(1,ls(dget(data,lms('name'))))}}\n`
		write_key(data,'description',x=>x,x=>x)
		write_key(data,'version'    ,x=>x,x=>x)
		write_dict('{data}\n',dget(data,lms('data')),x=>x)
		r+=`{script}\n${esc_write(0,ls(ifield(m,'script')))}\n{end}\n`
	})
	deck.contraptions.v.map(def=>{
		const data=prototype_write(def),wids=dget(data,lms('widgets')),base=ls(dget(data,lms('name')));sci=0
		r+=`\n{contraption:${esc_write(1,base)}}\n`
		write_key(data,'size'       ,x=>1       ,x=>x)
		write_key(data,'resizable'  ,x=>x&&lb(x),x=>x)
		write_key(data,'margin'     ,x=>1       ,x=>x)
		write_key(data,'description',x=>x       ,x=>x)
		write_key(data,'version'    ,x=>x       ,x=>x)
		write_key(data,'image'      ,x=>x       ,x=>x)
		write_key(data,'script'     ,x=>count(x),x=>script_ref(base,x,'p'))
		write_key(data,'template'   ,x=>count(x),x=>x)
		write_key(data,'attributes' ,x=>count(x),x=>x)
		wids.v.map(wid=>{const k=lms('script'),v=dget(wid,k);if(v)dset(wid,k,script_ref(base,v,'p'))})
		write_dict('{widgets}\n',wids,x=>x)
		write_scripts()
	})
	return r+'\n'+(html?'<\/script>\nRuntime stub is NYI.':'')
}

n_go=([x,t,delay],deck)=>{
	let r=null, i=deck.card
	if(lin(x))r=clamp(0,ln(x),count(deck.cards)-1)
	else if(card_is(x)){const i=dvix(deck.cards,x);if(i>=0)r=i}
	else{
		x=ls(x);if(deck.history.length>1&&x=='Back'){
			deck.history.pop();const ix=last(deck.history);
			if(ix>=0&&ix<count(deck.cards)){go_notify(deck,ix,t,x,delay),deck.card=ix;return lmn(deck.card)}
		}
		else if(x=='First')r=0
		else if(x=='Last' )r=count(deck.cards)-1
		else if(x=='Prev' )r=mod(i-1,count(deck.cards))
		else if(x=='Next' )r=mod(i+1,count(deck.cards))
		else{const ix=dkix(deck.cards,lms(x));if(ix>=0)r=ix}
	}if(r!=null){go_notify(deck,r,t,x,delay),deck.card=r;if(i!=r)deck.history.push(r)}else{go_notify(deck,-1,t,x,delay)}return lmn(deck.card)
}
n_sleep=([z])=>{if(lis(z)&&ls(z)=='play'){sleep_play=1}else{sleep_frames=max(1,ln(z))};return z}
n_transition=(f,deck)=>{const t=deck.transit;if(lion(f))dset(t,lms(f.n),f);return t}

const ext={}
const ext_constants={}
constants=env=>{
	env.local('sys'    ,interface_system)
	env.local('app'    ,interface_app)
	env.local('bits'   ,interface_bits)
	env.local('rtext'  ,interface_rtext)
	env.local('pointer',pointer)
	env.local('pi'   ,lmn(3.141592653589793))
	env.local('e'    ,lmn(2.718281828459045))
	env.local('colors',lmd(
		'white|yellow|orange|red|magenta|purple|blue|cyan|green|darkgreen|brown|tan|lightgray|mediumgray|darkgray|black'.split('|').map(lms),
		range(16).map(x=>lmn(x+32))
	))
	Object.keys(ext_constants).map(key=>env.local(key,ext_constants[key]))
}
primitives=(env,deck)=>{
	env.local('show'      ,lmnat(n_show    ))
	env.local('print'     ,lmnat(n_print   ))
	env.local('panic'     ,lmnat(n_panic   ))
	env.local('play'      ,lmnat(n_play    ))
	env.local('go'        ,lmnat(([x,t,d])=>n_go([x,t,d],deck)))
	env.local('transition',lmnat(([f])=>n_transition(f,deck)))
	env.local('brush'     ,lmnat(x=>n_brush(x,deck)))
	env.local('sleep'     ,lmnat(n_sleep   ))
	env.local('eval'      ,lmnat(n_eval    ))
	env.local('random'    ,lmnat(n_random  ))
	env.local('array'     ,lmnat(n_array   ))
	env.local('image'     ,lmnat(n_image   ))
	env.local('sound'     ,lmnat(n_sound   ))
	env.local('newdeck'   ,lmnat(([x])=>deck_read(lis(x)?ls(x):'')))
	env.local('readcsv'   ,lmnat(n_readcsv ))
	env.local('writecsv'  ,lmnat(n_writecsv))
	env.local('readxml'   ,lmnat(n_readxml ))
	env.local('writexml'  ,lmnat(n_writexml))
	env.local('alert'     ,lmnat(n_alert   ))
	env.local('read'      ,lmnat(n_open    ))
	env.local('write'     ,lmnat(n_save    ))
}
let in_attr=0
fire_attr_sync=(target,name,a)=>{
	if(in_attr>=2)return NIL;in_attr++;const bf=frame;
	const root=lmenv();primitives(root,target.deck),constants(root)
	root.local('me',target),root.local('card',target),root.local('deck',target.deck),root.local('patterns',target.deck.patterns)
	const b=lmblk();target.widgets.v.map((v,i)=>{blk_lit(b,v),blk_loc(b,target.widgets.k[i]),blk_op(b,op.DROP)})
	try{blk_cat(b,parse(target.def.script)),blk_op(b,op.DROP)}catch(e){}
	blk_get(b,lms(name)),blk_lit(b,lml(a?[a]:[])),blk_op(b,op.CALL)
	pushstate(root),issue(root,b);let q=ATTR_QUOTA;while(running()&&q>0)runop(),q--;const r=running()?NIL:arg();popstate();frame=bf;return in_attr--,r
}
parent_deck=x=>deck_is(x)?x: card_is(x)||prototype_is(x)?x.deck: parent_deck(x.card)
event_invoke=(target,name,arg,hunk,nodiscard)=>{
	const scopes=lmd([ZERO],[parse(DEFAULT_HANDLERS)]); let deck=null
	const ancestors_record=(target,src)=>{try{dset(scopes,target,parse(ls(ifield(src,'script'))))}catch(e){dset(scopes,target,lmblk())}}
	const ancestors_inner=target=>{
		if(deck_is(target)){deck=target;return}
		if(contraption_is(target)){deck=target.card.deck}
		else if(card_is(target)||prototype_is(target)){deck=target.deck}
		else{ancestors_inner(target.card)}
		ancestors_record(target,contraption_is(target)?ifield(target,'def'):target)
	}
	const ancestors_outer=target=>{
		if(deck_is(target)){deck=target}
		else if(widget_is(target)){ancestors_outer(target.card)}
		else{ancestors_outer(target.deck)}
		ancestors_record(target,target)
	}
	(prototype_is(target)||prototype_is(target.card)||contraption_is(target.card))?ancestors_inner(target):ancestors_outer(target)
	const bind=(b,n,v)=>{blk_lit(b,v),blk_loc(b,n),blk_op(b,op.DROP)}
	const func=(b,n,v)=>{blk_lit(b,lmon(n,[],blk_end(v))),blk_op(b,op.BIND),blk_op(b,op.DROP),name=n,arg=lml([])}
	let core=null
	for(let z=scopes.v.length-1;z>=0;z--){
		let t=scopes.k[z], b=lmblk(), sname='!widget_scope'
		if(lin(t))sname='!default_handlers'
		if(deck_is(t)){
			t.modules.v.map((v,i)=>bind(b,t.modules.k[i],ifield(v,'value')))
			t.cards  .v.map((v,i)=>bind(b,t.cards  .k[i],v                ))
			sname='!deck_scope'
		}
		if(card_is(t)||prototype_is(t)||(contraption_is(t)&&target!=t)){
			bind(b,lms('card'),t)
			t.widgets.v.map((v,i)=>bind(b,t.widgets.k[i],v))
			sname='!card_scope'
		}
		blk_cat(b,scopes.v[z]),blk_op(b,op.DROP)
		if(!core&&hunk){func(b,'!hunk',hunk)}
		else if(core){func(b,sname,core)}
		blk_get(b,lms(name)),blk_lit(b,arg),blk_op(b,op.CALL);if(!hunk&&!nodiscard)blk_op(b,op.DROP);core=b
	}
	const r=lmblk();bind(r,lms('me'),proxy_is(target)?ivalue(target,'card'):target)
	bind(r,lms('deck'),deck),bind(r,lms('patterns'),deck.patterns)
	return blk_cat(r,core),r
}
fire_async=(target,name,arg,hunk,nest)=>{
	const root=lmenv();primitives(root,parent_deck(target)),constants(root)
	if(nest)pushstate(root),pending_popstate=1;issue(root,event_invoke(target,name,arg,hunk,0))
}
fire_event_async=(target,name,x)=>fire_async(target,name,lml([x]),null,1)
fire_hunk_async=(target,hunk)=>fire_async(target,null,lml([]),hunk,1)
n_event=(self,args)=>{
	const root=lmenv();primitives(root,parent_deck(self)),constants(root)
	const b=lmblk();blk_op(b,op.DROP),blk_cat(b,event_invoke(self,ls(args[0]),lml(args.slice(1)),null,1))
	return issue(root,b),NIL
}
readgif=(data,hint)=>{
	const gray=hint=='gray'||hint=='gray_frames', frames=hint=='frames'||hint=='gray_frames'
	let i=0;const ub=_=>data[i++]||0, s=_=>ub()|(ub()<<8), struct=(f,d)=>lmd(['frames','delays'].map(lms),[lml(f),lml(d)])
	function readcolors(r,packed){const c=1<<((packed&0x07)+1);for(let z=0;z<c;z++)r[z]=readcolor(ub(),ub(),ub(),gray);return r}
	if(ub()!=71||ub()!=73||ub()!=70)return frames?struct([],[]):image_make(rect());i+=3;const r_frames=[],r_delays=[],r_disposal=[],r_dict=lmd()
	const w=s(),h=s(),gpal=new Uint8Array(256),packed=ub(),back=ub();ub()
	let hastrans=0,trans=255,delay=0,dispose=0,r=image_make(rect(w,h));if(packed&0x80)readcolors(gpal,packed)
	while(i<data.length){
		const type=ub()
		if(type==0x3B)break // end
		if(type==0x21){ // text, gce, comment, app...?
			if((0xFF&ub())==0xF9){
				ub();const packed=ub();delay=s()
				const tindex=ub();ub();dispose=(packed>>2)&7
				if(packed&1){hastrans=1,trans=tindex}else{hastrans=0}
			}else{while(1){const s=ub();if(!s)break;i+=s}}
		}
		if(type==0x2C){ // image descriptor
			const xo=s(),yo=s(),iw=s(),ih=s(),packed=ub(),local=packed&0x80
			const lpal=new Uint8Array(gpal);if(local)readcolors(lpal,packed);if(hastrans)lpal[trans]=gray?255:0
			const min_code=ub(),src=new Uint8Array(iw*ih*2),dst=new Uint8Array(iw*ih);let si=0, di=0
			while(1){const s=ub();if(!s)break;for(let z=0;z<s;z++)src[si++]=ub()}
			const prefix=new Int32Array(4096),suffix=new Int32Array(4096),code=new Int32Array(4096)
			const clear=1<<min_code; let size=min_code+1, mask=(1<<size)-1, next=clear+2, old=-1, first=0, i=0,b=0,d=0
			for(let z=0;z<clear;z++)suffix[z]=z
			while(i<si){
				while(b<size)d+=(0xFF&src[i++])<<b, b+=8
				let t=d&mask; d>>=size, b-=size
				if(t>next||t==clear+1)break
				if(t==clear){size=min_code+1, mask=(1<<size)-1, next=clear+2, old=-1}
				else if (old==-1) dst[di++]=suffix[old=first=t]
				else{
					let ci=0,tt=t
					if   (t==next)code[ci++]=first,    t=old
					while(t>clear)code[ci++]=suffix[t],t=prefix[t]
					dst[di++]=first=suffix[t]
					while(ci>0)dst[di++]=code[--ci]
					if(next<4096){prefix[next]=old, suffix[next++]=first;if((next&mask)==0&&next<4096)size++, mask+=next}
					old=tt
				}
			}
			for(let y=0;y<ih;y++)for(let x=0;x<iw;x++)if(xo+x>=0&&yo+y>=0&&xo+x<w&&yo+y<h&&(!hastrans||dst[x+y*iw]!=trans))r.pix[(xo+x)+(yo+y)*w]=lpal[dst[x+y*iw]]
			r_frames.push(image_copy(r)),r_delays.push(lmn(delay)),r_disposal.push(dispose);if(!frames)break
			if(dispose==2){r.pix.fill(hastrans?0:lpal[back])} // dispose to background
			if(dispose==3){let i=r_frames.length-2;while(i&&r_disposal[i]>=2)i--;for(let z=0;z<r.pix.length;z++)r.pix[z]=r_frames[i].pix[z];}// dispose to previous
		}
	}return frames?struct(r_frames,r_delays): r_frames.length?r_frames[0]: image_make(rect())
}
