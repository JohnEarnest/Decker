// lil: Learning in Layers

let allocs=0,calldepth=0
lmn  =x      =>(allocs++,{t:'num',v:isFinite(x)?+x:0}),   lin  =x=>x&&x.t=='num'
lms  =x      =>(allocs++,{t:'str',v:''+x }),              lis  =x=>x&&x.t=='str'
lml  =x      =>(allocs++,{t:'lst',v:x    }),              lil  =x=>x&&x.t=='lst'
lmd  =(k,v)  =>(allocs++,{t:'dic',k:k||[],v:v||[]}),      lid  =x=>x&&x.t=='dic'
lmt  =x      =>(allocs++,{t:'tab',v:x    }),              lit  =x=>x&&x.t=='tab'
lmi  =(f,n,x)=>(allocs++,{t:'int',f,n}),                  lii  =x=>x&&x.t=='int'
lmon =(n,a,b)=>(allocs++,{t:'on' ,n:n,a:a,b:b,c:null}),   lion =x=>x&&x.t=='on'
lmnat=f      =>(allocs++,{t:'nat',f:f}),                  linat=x=>x&&x.t=='nat'
lmblk=_      =>(allocs++,{t:'blk',b:[],locals:[]}),       liblk=x=>x&&x.t=='blk'
lmenv=p      =>{allocs++;const r={t:'env',v:{},p:p};r.local=(n,x)=>env_local(r,lms(n),x);return r}

NONE=lmn(0), ONE=lmn(1), seed=0x12345, max=Math.max, min=Math.min, abs=Math.abs
ISODATE=lms('%04i-%02i-%02iT%02i:%02i:%02iZ%n%m'), PARTS=['year','month','day','hour','minute','second'].map(lms)
clchar=x=>{const c=x.charCodeAt(0);return x=='\t'?' ':(c>=32&&c<=126)||(x=='\n')?x:'?'}
clchars=x=>x.replace(/\r/g,'').replace(/\t/g,' ').replace(/[\u201C\u201D]/g,'"').replace(/[\u2018\u2019]/g,"'").replace(/[^ -~\n]/g,'?')
wnum=y=>{
	let w='',d='',s=y<0?(y=-y,'-'):'',i=Math.floor(y);while(i>0){w=(0|i%10)+w,i=i/10}
	y=Math.round((y-Math.floor(y))*1000000);for(let z=0;z<6;z++){d=(0|y%10)+d,y=y/10}
	return s+('0'+w).replace(/^(0+)(?=[^0])/,'')+('.'+d).replace(/(\.?0+)$/,'')
}
mod=(x,y)=>x-y*Math.floor(x/y)
range=x=>Array.from(Array(x)).map((_,i)=>i)
torect=t=>{const n=Object.values(t).reduce((x,y)=>max(x,lil(y)?count(y):1),0);Object.keys(t).map(k=>t[k]=take(n,lil(t[k])?t[k].v:[t[k]]))}
count=x=>lin(x)?1: lis(x)||lil(x)||lid(x)?x.v.length: lit(x)?(Object.values(x.v)[0]||[]).length:0
rows=x=>{const t=lt(x);return lml(range(count(t)).map(i=>lmd(Object.keys(t.v).map(lms),Object.values(t.v).map(x=>x[i]))))}
coltab=x=>{
	const n=lmn(x.v.reduce((x,y)=>max(x,lil(y)?count(y):1),0))
	return lmt(x.k.reduce((r,k,i)=>(r[ls(k)]=dyad.take(n,lil(x.v[i])?x.v[i]:lml([x.v[i]])).v,r),{}))
}
rowtab=x=>{
	const ok=[],t={};x.v.map(r=>r.k.map(k=>{if(!t[ls(k)])ok.push(k);t[ls(k)]=[]}))
	x.v.map(x=>Object.keys(t).map((k,i)=>t[k].push(dget(x,ok[i])||NONE)));return lmt(t)
}
listab=x=>{
	const m=x.v.reduce((r,x)=>max(r,count(x)),0);const t={};for(let z=0;z<m;z++)t['c'+z]=[]
	x.v.map(row=>{for(let z=0;z<m;z++)t['c'+z].push(z>=count(row)?NONE:row.v[z])});return lmt(t)
}
zip=(x,y,f)=>{const n=count(x),o=count(y)<n?take(n,y.v):y.v;return x.v.map((x,i)=>f(x,o[i%n]))}
match=(x,y)=>x==y?1: (x.t!=y.t)||(count(x)!=count(y))?0: (lin(x)||lis(y))?x.v==y.v:
	         lil(x)?x.v.every((z,i)=>dyad['~'](z,y.v[i]).v): lit(x)?dyad['~'](rows(x),rows(y)).v:
	         lid(x)?x.v.every((z,i)=>dyad['~'](z,y.v[i]).v&&dyad['~'](x.k[i],y.k[i]).v):0
splicet=(f,x,t)=>{const r={};Object.keys(t.v).map(k=>r[k]=f(x,t.v[k]));return lmt(r)}
splice=(f,x,y)=>lis(y)?lms(f(x,ll(y)).map(ls).join('')): lid(y)?lmd(f(x,y.k),f(x,y.v)): lit(y)?splicet(f,x,y): lml(f(x,ll(y)))
ina=(x,y)=>lmn(lis(y)?y.v.indexOf(ls(x))>=0: lil(y)?y.v.some(y=>match(x,y)): lid(y)?dget(y,x)!=undefined: lit(y)?ls(x)in y.v: x==y)
filter=(i,x,y)=>{
	x=lis(x)?monad.list(x):lml(ll(x))
	if(lid(y)){const r=lmd();y.k.forEach((k,v)=>i==lb(ina(k,x))&&dset(r,k,y.v[v]));return r}
	if(!lit(y))return lml(ll(y).filter(z=>i==lb(ina(z,x)))); const n=x.v.every(lin),nx=x.v.map(ln),ix=y.v[Object.keys(y.v)[0]]||[]
	if(n&&i){const r=dyad.take(NONE,y);nx.forEach(i=>{Object.keys(y.v).map(c=>{const v=y.v[c][i];if(v)r.v[c].push(v)})});return r}
	if(n&&!i){const r=dyad.take(NONE,y);ix.forEach((_,i)=>{if(nx.indexOf(i)<0)Object.keys(y.v).map(c=>r.v[c].push(y.v[c][i]))});return r}
	return lmt(Object.keys(y.v).reduce((t,k)=>(i==lb(ina(lms(k),x))&&(t[k]=y.v[k]),t),{}))
}
take=(x,y)=>{const n=y.length, s=x<0?mod(x,n):0; return range(abs(x)).map(z=>y[mod(z+s,n)])}
dkix=(dict,key)=>dict.k.findIndex(x=>match(key,x)), dget=(dict,key)=>dict.v[dkix(dict,key)]
dvix=(dict,val)=>dict.v.findIndex(x=>match(val,x)), dkey=(dict,val)=>dict.k[dvix(dict,val)]
dset=(d,k,v)=>{const i=d.k.findIndex(x=>match(x,k));if(i<0){d.k.push(k),d.v.push(v)}else{d.v[i]=v};return d}
union=(x,y)=>{const r=lmd(x.k.slice(0),x.v.slice(0));y.k.forEach(k=>dset(r,k,dget(y,k)));return r}
amend=(x,i,y)=>{
	if(lii(x))return x.f(x,i,y)
	if(!lis(x)&&!lil(x)&&!lid(x))return amend(lml([]),i,y)
	if((lil(x)||lis(x))&&(!lin(i)||(ln(i)<0||ln(i)>count(x))))return amend(ld(x),i,y)
	if(lil(x)){const r=lml(x.v.slice(0));r.v[ln(i)|0]=y;return r}
	return lid(x)?dset(lmd(x.k.slice(0),x.v.slice(0)),i,y): lis(x)?lms(ls(x).slice(0,ln(i))+ls(y)+ls(x).slice(1+ln(i))): lml([])
}
l_at=(x,y)=>{
	if(lii(x))return lis(y)&&y.v=='type'?lms(x.n): x.f(x,y)
	if(lit(x)&&lin(y))x=monad.rows(x); if((lis(x)||lil(x))&&!lin(y))x=ld(x)
	return lis(x)?lms(x.v[ln(y)|0]||''): lil(x)?x.v[ln(y)|0]||NONE: lid(x)?dget(x,y)||NONE: lit(x)&&lis(y)?lml(x.v[ls(y)]): NONE
}
amendv=(x,i,y,n,tla)=>{
	if(lii(x))tla.v=0;const f=monad.first(i[n]||NONE)
	return (!tla.v&&n+1 <i.length)?amendv(l_at(x,f),i,y,n+1,tla):
	       (n+1<i.length)?amend(x,f,amendv(l_at(x,f),i,y,n+1,tla)): (n+1==i.length)?amend(x,f,y): y
}
lb=x=>lin(x)?x.v!=0: lis(x)?x.v!='': lil(x)||lid(x)?x.v.length: 1
ln=x=>lin(x)?x.v: lis(x)?(isFinite(x.v)?+x.v:0): lil(x)||lid(x)?ln(x.v[0]): 0
ls=x=>lin(x)?wnum(x.v): lis(x)?x.v: lil(x)?x.v.map(ls).join(''): ''
ll=x=>lis(x)?x.v.split('').map(lms): lil(x)||lid(x)?x.v: lit(x)?rows(x).v: [x]
ld=x=>lid(x)?x:lit(x)?monad.cols(x):lil(x)||lis(x)?lmd(range(count(x)).map(lmn),lis(x)?ll(x):x.v):lmd()
lt=x=>lit(x)?x: lid(x)||lil(x)?lmt((lid(x)?['key','value']:['value']).reduce((t,k,i)=>(t[k]=x[k[0]],t),{})): lmt({value:ll(x)})
vm=f=>{const r=x=>lil(x)?lml(x.v.map(r)):f(x);return r}
vd=f=>{const r=(x,y)=>lil(x)&lil(y)?lml(zip(x,y,r)): lil(x)&!lil(y)? lml(x.v.map(x=>r(x,y))): !lil(x)&lil(y)?lml(y.v.map(y=>r(x,y))): f(x,y);return r}
vmnl=f=>{const r=x=>lil(x)?(ll(x).some(x=>!lin(x))?lml(x.v.map(r)):f(x)):f(x);return r}
tflip=x=>{
	const c=Object.keys(x.v), kk=c.indexOf('key')>-1?'key':c[0], k=(x.v[kk]||[]).map(ls), cc=c.filter(k=>k!=kk)
	return lmt(k.reduce((r,k,i)=>{r[k]=cc.map(c=>x.v[c][i]);return r},{key:cc.map(lms)}))
}
tcat=(x,y)=>{
	const r={}
	Object.keys(x.v).map(k=>r[k]=x.v[k].concat(y.v[k]?[]:range(count(y)).map(x=>NONE)))
	Object.keys(y.v).map(k=>r[k]=(x.v[k]?x.v[k]:range(count(x)).map(x=>NONE)).concat(y.v[k]))
	return lmt(r)
}
fstr=x=>{
	let ct=0;return x.split('').map(x=>{
		let e=0;if(x=='<'){ct=1}else if(x=='/'&&ct){e=1}else if(x!=' '&&x!='\n'){ct=0}
		return e?'\\/':({'\n':'\\n','\\':'\\\\','"':'\\"'})[x]||x
	}).join('')
}
fjson=x=>lin(x)?wnum(x.v): lil(x)?`[${x.v.map(fjson).join(',')}]`:
         lis(x)?`"${fstr(x.v)}"`:lid(x)?`{${x.k.map((k,i)=>`${fjson(lms(ls(k)))}:${fjson(x.v[i])}`).join(',')}}`:'null'
pjson=(y,h,n)=>{
	const si=h, hn=_=>m&&y[h]&&(n?h-si<n:1), hnn=x=>m&&h+x<=y.length&&(n?h+x-si<n:1)
	const jd=_=>{while(hn()&&/[0-9]/.test(y[h]))h++}, jm=x=>hn()&&y[h]==x?(h++,1):0, iw=_=>/[ \n]/.test(y[h]), ws=_=>{while(hn()&&iw())h++}
	const esc=e=>e=='n'?'\n': /[\\/"]/.test(e)?e: e=='u'&&hnn(4)?String.fromCharCode(parseInt(y.slice(h,h+=4),16)):' '
	let f=1, m=1, rec=_=>{
		const t={null:NONE,false:NONE,true:ONE};for(let k in t)if(hnn(k.length)&&y.slice(h,h+k.length)==k)return h+=k.length,t[k]
		if(jm('[')){const r=lml([]);while(f&&hn()){ws();if(jm(']'))break;r.v.push(rec()),ws(),jm(',')}return r}
		if(jm('{')){const r=lmd();while(f&&hn()){ws();if(jm('}'))break;const k=rec();ws(),jm(':');if(f)dset(r,k,rec());ws(),jm(',')}return r}
		if(jm('"')){let r='';while(f&&hn()&&!jm('"'))r+=hnn(2)&&jm('\\')?esc(y[h++]):y[h++];return lms(r)}
		const ns=h;jm('-'),jd(),jm('.'),jd();if(jm('e')||jm('E')){jm('-')||jm('+');jd();}return h<=ns?(f=0,NONE): lmn(+y.slice(ns,h))
	}, r=rec();return {value:r,index:h}
}
monad={
	'-':    vm(x=>lmn(-ln(x))),
	'!':    vm(x=>lb(x)?NONE:ONE),
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
	sum:    x=>ll(x).reduce(dyad['+'],NONE),
	raze:   x=>ll(x).slice(1).reduce(dyad[','],monad.first(x)),
	max:    x=>ll(x).slice(1).reduce(dyad['|'],monad.first(x)),
	min:    x=>ll(x).slice(1).reduce(dyad['&'],monad.first(x)),
	count:  x=>lmn(count(x)),
	first:  x=>lit(x)?monad.first(rows(x)): count(x)?ll(x)[0]: NONE,
	last:   x=>lit(x)?monad.last (rows(x)): count(x)?ll(x)[count(x)-1]: NONE,
	range:  x=>lml(lin(x)?range(max(0,0|ln(x))).map(lmn): ld(x).k),
	list:   x=>lml([x]),
	typeof: x=>lms(({num:"number",str:"string",lst:"list",dic:"dict",tab:"table",on:"function",nat:"function"})[x.t]||x.n||"interface"),
	flip:   x=>lit(x)?tflip(x):lml(range(ll(x).reduce((w,z)=>max(w,lil(z)?count(z):1),0)).map(i=>lml(ll(x).map(c=> !lil(c)?c: i<count(c)?c.v[i]: NONE)))),
	rows:   x=>rows(x),
	cols:   x=>{const t=lt(x).v;return lmd(Object.keys(t).map(lms),Object.values(t).map(lml))},
	table:  x=>lid(x)?coltab(x): lil(x)&&x.v.every(lid)?rowtab(x): lil(x)&&x.v.every(lil)?listab(x): lt(x),
	'@tab': t=>{t=lt(t);const r=lmt({});Object.keys(t.v).map(k=>r.v[k]=t.v[k].slice(0)),r.v.index=range(count(r)).map(lmn);return r},
}
dyad={
	'+':  vd((x,y)=>lmn(ln(x)+ln(y))),
	'-':  vd((x,y)=>lmn(ln(x)-ln(y))),
	'*':  vd((x,y)=>lmn(ln(x)*ln(y))),
	'/':  vd((x,y)=>lmn(ln(x)/ln(y))),
	'%':  vd((x,y)=>lmn(mod(ln(y),ln(x)))),
	'^':  vd((x,y)=>lmn(Math.pow(ln(x),ln(y)))),
	'<':  vd((x,y)=>lmn(lin(x)&&lin(y)?ln(x)< ln(y): ls(x)< ls(y))),
	'>':  vd((x,y)=>lmn(lin(x)&&lin(y)?ln(x)> ln(y): ls(x)> ls(y))),
	'=':  vd((x,y)=>lmn(lii(x)||lii(y)?x==y: lin(x)&&lin(y)?ln(x)==ln(y): ls(x)==ls(y))),
	'&':  vd((x,y)=>lin(x)||lin(y)?lmn(min(ln(x),ln(y))): lms(ls(x)<ls(y)?ls(x):ls(y))),
	'|':  vd((x,y)=>lin(x)||lin(y)?lmn(max(ln(x),ln(y))): lms(ls(x)>ls(y)?ls(x):ls(y))),
	split:(x,y)=>lml(ls(y).split(ls(x)).map(lms)),
	fuse: (x,y)=>lms(ll(y).map(ls).join(ls(x))),
	dict: (x,y)=>(y=ll(y),ll(x).reduce((d,x,i)=>dset(d,x,y[i]||NONE),lmd())),
	take: (x,y)=>lin(x)?splice(take,ln(x),y):filter(1,x,y),
	drop: (x,y)=>lin(x)?splice((x,y)=>x<0?y.slice(0,x):y.slice(x),ln(x),y):filter(0,x,y),
	limit:(x,y)=>count(y)>ln(x)?dyad.take(lmn(ln(x)),y):y,
	in:   (x,y)=>lil(x)?lml(x.v.map(x=>dyad.in(x,y))):ina(x,y),
	',':  (x,y)=>lit(x)&&lit(y)?tcat(x,y): lid(x)?union(x,ld(y)):
                 lis(x)?dyad[','](lml([x]),y): lis(y)?dyad[','](x,lml([y])): lml(ll(x).concat(ll(y))),
	'~':  (x,y)=>match(x,y)?ONE:NONE,
	unless:(x,y)=>lin(y)&&ln(y)==0?x:y,
	join: (x,y)=>{ // natural join on tables.
		const f=x=>lin(x)?monad.range(x):lml(ll(x));if(!lit(x)||!lit(y))return lml(zip(f(x),f(y),dyad[',']))
		const a=x,b=y,ak=Object.keys(a.v),bk=Object.keys(b.v), ik=bk.filter(x=>ak.indexOf(x)>=0),dk=bk.filter(x=>ak.indexOf(x)<0)
		const r=lmt({}); ak.forEach(k=>r.v[k]=[]), dk.forEach(k=>r.v[k]=[])
		for(let ai=0;ai<count(a);ai++)for(let bi=0;bi<count(b);bi++)
			if(ik.every(k=>match(a.v[k][ai],b.v[k][bi])))ak.forEach(k=>r.v[k].push(a.v[k][ai])),dk.forEach(k=>r.v[k].push(b.v[k][bi]))
		return r
	},
	cross: (x,y)=>{ // cartesian join; force columns to be unique:
		const f=x=>lt(lin(x)?monad.range(x):lml(ll(x)));if(!lit(x)||!lit(y))return lml(rows(dyad.cross(f(x),f(y))).v.map(x=>lml(x.v)))
		const a=lt(x),b=lt(y), ak=Object.keys(a.v),bk=Object.keys(b.v), uk=bk.map(x=>ak.indexOf(x)>=0?x+'_':x)
		const r=lmt({}); ak.forEach(k=>r.v[k]=[]), uk.forEach(k=>r.v[k]=[])
		for(let bi=0;bi<count(b);bi++)for(let ai=0;ai<count(a);ai++)ak.forEach(k=>r.v[k].push(a.v[k][ai])),bk.forEach((k,i)=>r.v[uk[i]].push(b.v[k][bi]))
		return r
	},
	parse: (x,y)=>{
		if(lil(y))return lml(y.v.map(y=>dyad.parse(x,y)))
		x=ls(x),y=ls(y);let r=[],f=0,h=0,m=1;while(x[f]){
			if(x[f]!='%'){if(m&&x[f]==y[h]){h++}else{m=0}f++;continue}f++
			let n=0,d=0,v=null,si=h,sk=x[f]=='*'&&(f++,1),lf=x[f]=='-'&&(f++,1);if(x[f]=='0')f++
			const hn=_=>m&&y[h]&&(n?h-si<n:1), id=x=>/[0-9]/.test(x), ix=_=>/[0-9a-fA-F]/.test(y[h]), iw=_=>/[ \n]/.test(y[h])
			while(id(x[f]))n=n*10+(+x[f++]);x[f]=='.'&&f++
			while(id(x[f]))d=d*10+(+x[f++]);if(!x[f])break;const t=x[f++]
			if('%mnzsluaro'.indexOf(t)<0)while(hn()&&iw())h++
			if(t=='%'){if(m&&t==y[h]){h++}else{m=0}}
			else if(t=='m'){v=m?ONE:NONE}
			else if(t=='n'){v=lmn(h)}
			else if(t=='z'){v=m&&h==y.length?ONE:NONE}
			else if(t=='s'||t=='l'||t=='u'){v=lms('');while(hn()&&(n?1:y[h]!=x[f]))v.v+=y[h++];if(t=='l')v.v=v.v.toLowerCase();if(t=='u')v.v=v.v.toUpperCase()}
			else if(t=='a'){v=lml([]);while(hn()&&(n?1:y[h]!=x[f]))v.v.push(lmn(y[h++].charCodeAt(0)))}
			else if(t=='b'){v=/[tTyYx1]/.test(y[h])?ONE:NONE;while(hn()&&(n?1:y[h]!=x[f]))h++}
			else if(t=='i'){v=lmn(0);const s=(y[h]=='-')?(h++,-1):1;m&=id(y[h]);while(hn()&&id(y[h]))v.v=v.v*10+(+y[h++]);v.v*=s}
			else if(t=='h'||t=='H'){v=lmn(0),                       m&=ix();    while(hn()&&ix())v.v=v.v*16+parseInt(y[h++],16)}
			else if(t=='j'){if(m){const j=pjson(y,h,n);h=j.index,v=j.value}else{v=NONE}}
			else if(t=='f'||t=='c'){
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
				if(l&&ln(dma)){h+=l}else{m=0}; v=t=='e'?lmn(m?0|(d.getTime()/1000):0):lmd(PARTS,[dy,dm,dd,dh,dmi,ds])
			}else{m=0}while(n&&y[h]&&h-si<n)h++,m=0;if(!sk&&v!=null)r.push(v)
		}return r.length==1?r[0]:lml(r)
	},
	format: (x,y)=>{
		const frec=(i,x,y)=>{
			if(i>=count(x))return y
			const fuse=(count(x)-i)%2?0:1,r=lml(ll(lit(y)?rows(y):y).map(z=>dyad.format(x.v[i+fuse],frec(i+fuse+1,x,lit(y)?lml(ll(z)):z))))
			return fuse?dyad.fuse(x.v[i],r):r
		};if(lil(x))return frec(0,x,y)
		x=ls(x),y=lil(y)?y:monad.list(y);let r='',f=0,h=0;while(x[f]){
			if(x[f]!='%'){r+=x[f++];continue}f++
			let n=0,d=0,sk=x[f]=='*'&&(f++,1),lf=x[f]=='-'&&(f++,1),pz=x[f]=='0'&&(f++,1)
			const hn=_=>m&&y[h]&&(n?h-si<n:1), id=x=>/[0-9]/.test(x)
			while(id(x[f]))n=n*10+(+x[f++]);x[f]=='.'&&f++
			while(id(x[f]))d=d*10+(+x[f++]);if(!x[f])break;const t=x[f++]
			let o='', a=t=='%'?NONE: (!sk&&h<count(y))?y.v[h++]: 'sluro'.indexOf(t)>=0?lms(''):NONE
			if     (t=='%'){o='%'}
			else if(t=='s'){o=ls(a)}
			else if(t=='l'){o=ls(a).toLowerCase()}
			else if(t=='u'){o=ls(a).toUpperCase()}
			else if(t=='r'||t=='o'){o=ls(a),lf=1;d=max(1,d);while(d&&x[f])d--,f++;d=n;}
			else if(t=='a'){o=ll(a).map(x=>clchar(String.fromCharCode(ln(x)))).join('')}
			else if(t=='b'){o=lb(a)?'true':'false'}
			else if(t=='f'){o=d?ln(a).toFixed(min(100,d)):wnum(ln(a))}
			else if(t=='c'){const v=ln(a);o=(v<0?'-':'')+'$'+abs(v).toFixed(min(100,d)||2)}
			else if(t=='i'){o=''+Math.trunc(ln(a))}
			else if(t=='h'||t=='H'){o=ln(a).toString(16);if(t=='H')o=o.toUpperCase()}
			else if(t=='e'){o=new Date(ln(a)*1000).toISOString().split('.')[0]+'Z'}
			else if(t=='p'){const d=ld(a);o=dyad.format(ISODATE,lml(PARTS.map(x=>dget(d,x)))).v}
			else if(t=='j'){o=fjson(a);}
			let vn=o.length; if(d&&(t=='f'||t=='c'))d=0;if(d&&lf)vn=min(d,vn)
			if(n&&!lf)for(let z=0;z<n-vn;z++)r+=pz?'0':' '
			for(let z=d&&!lf?max(0,vn-d):0;z<vn;z++)r+=o[z]
			if(n&&lf)for(let z=0;z<n-vn;z++)r+=pz?'0':' '
		}return lms(r)
	},
}
merge=(vals,keys,widen)=>{
	const i=lms('@index');let ix=null
	if(!widen){ix=lml([]);vals.v.map((val,z)=>{dget(val,i).v.map(x=>ix.v.push(x))})}
	if(count(vals)==0)vals.v.push(keys.v.reduce((x,y)=>dset(x,y,lml([])),lmd()))
	let r=monad.raze(lml(vals.v.map(x=>monad.table(widen?x:dyad.drop(i,x)))))
	if(widen){ix=lml(r.v['@index']),r=dyad.drop(i,r)}
	return {r,ix}
}
n_uplevel=([a])=>{let i=2, e=getev(), r=null, name=ls(a); while(e&&i){r=e.v[name];if(r)i--;e=e.p};return r||NONE}
n_eval=([x,y])=>{
	y=y?ld(y):lmd();const yy=lmd(y.k.slice(0),y.v.slice(0)), r=lmd(['value','vars','error'].map(lms),[NONE,yy,lms('')])
	const feval=([r,x])=>{dset(r,lms('value'),x);const b=dget(r,lms('vars')), v=getev().v; Object.keys(v).map(k=>dset(b,lms(k),v[k]));return r}
	try{
		const prog=parse(x?ls(x):'')
		blk_opa(prog,op.BUND,2),blk_lit(prog,lmnat(feval)),blk_op(prog,op.SWAP),blk_op(prog,op.CALL)
		issue(env_bind(null,yy.k.map(ls),lml(yy.v)),prog)
	}catch(e){dset(r,lms('error'),lms(e.x))};return r
}
triad={
	'@sel': (orig,vals,keys)=>{const mv=merge(vals,keys,0);return count(keys)>1?mv.r: dyad.take(mv.ix,dyad.drop(lms('index'),orig))},
	'@ext': (orig,vals,keys)=>{
		const r=monad.cols(triad['@sel'](orig,vals,keys));if(r.v.every(x=>lil(x)&&count(x)==1))r.v=r.v.map(monad.first)
		return count(r)!=1||count(r.k[0])?r: monad.first(r)
	},
	'@upd': (orig,vals,keys)=>{
		orig=dyad.drop(lms('index'),orig);const mv=merge(vals,keys,1),r=mv.r,ix=mv.ix
		Object.keys(r.v).map(c=>{
			if(r.v[c]==ix)return;const ci=c in orig.v, col=range(count(orig)).map(z=>ci?orig.v[c][z]:NONE)
			orig.v[c]=col,ix.v.map((x,row)=>col[0|ln(x)]=r.v[c][row])
		});return orig
	},
	'@ins': (v,n,x)=>{
		const torect=t=>{
			const n=lmn(Object.values(t.v).reduce((x,y)=>max(x,y.length),0))
			Object.keys(t.v).map(k=>t.v[k]=ll(dyad.take(n,lml(t.v[k]))))
		}, r=monad.table(dyad.dict(n,v));torect(r);return lin(x)?r:dyad[','](lt(x),r)
	},
}

findop=(n,prims)=>Object.keys(prims).indexOf(n), as_enum=x=>x.split(',').reduce((x,y,i)=>{x[y]=i;return x},{})
let tnames=0;tempname=_=>lms(`@t${tnames++}`)
op=as_enum('JUMP,JUMPF,LIT,DROP,SWAP,OVER,BUND,OP1,OP2,OP3,GET,SET,LOC,AMEND,TAIL,CALL,BIND,ITER,EACH,NEXT,COL,QUERY,IPRE,IPOST')
oplens=   [ 3   ,3    ,3  ,1   ,1   ,1   ,3   ,3  ,3  ,3  ,3  ,3  ,3  ,3    ,1   ,1   ,1   ,1   ,3   ,3   ,1  ,3    ,3   ,3     ]
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
		else if(b==op.JUMP||b==op.JUMPF||b==op.EACH||b==op.NEXT){blk_opa(x,b,blk_gets(y,z+1)+base)}
		else{for(let i=0;i<oplens[b];i++)blk_addb(x,blk_getb(y,z+i))}z+=oplens[b]
	}
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
	const ne=_=>{const e=nc();return e in esc?esc[e]: er(`Invalid escape character '\\${e}' in string.`)}
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
		if(cc=='"'){let v='',c;while(i<text.length&&(c=nc())!='"')v+=(c=='\\'?ne():clchar(c));return{t:'string',v,r:tr,c:tc}}
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
		const kw={while:1,each:1,send:1,on:1,if:1,else:1,end:1,do:1,with:1,local:1,select:1,extract:1,update:1,insert:1,
			into:1,from:1,where:1,by:1,orderby:1,asc:1,desc:1};return !(n in kw||n in monad||n in dyad)
	}
	const name=n=>{const r=expect('name');if(!ident(r)&&n!='member')er(`'${r}' is a keyword, and cannot be used for a ${n} name.`);return r}
	const names=(x,n)=>{const r=[];while(!match(x))r.push(name(n));return r}
	const quote=_=>{const r=lmblk();expr(r);blk_end(r);return r}
	const block=_=>{const r=lmblk();iblock(r);return r}
	const quotesub=_=>{let c=0,r=lmblk();while(hasnext()&&!matchsp(']'))expr(r),c++;blk_opa(r,op.BUND,c);return r}
	const quotedot=_=>{const r=lmblk();blk_lit(r,lml([lms(name('member'))]));return r}
	const iblock=r=>{let c=0;while(hasnext()){if(match('end')){if(!c)blk_lit(r,NONE);return}if(c)blk_op(r,op.DROP);expr(r),c++};er(`Expected 'end' for block.`)}
	const parsequery=(b,func,dcol)=>{
		const cols=lmd([],[]);while(!matchp('from')&&!matchp('where')&&!matchp('by')&&!matchp('orderby')){
			let set=peek2().t==':', lit=peek().t=='string', name=lms(lit?(set?peek().v:''):peek().t=='name'?peek().v:'')
			let get=ident(ls(name)), unique=ls(name).length&&dkix(cols,name)==-1; if(set&&lit&&!unique)next(),next()
			const x=set&&unique?(next(),next(),name): get&&unique&&dcol?name: lms(dcol?`c${cols.k.length}`: '')
			cols.k.push(x),cols.v.push(quote())
		}
		let cw,cb,co,dir=-1,index=lmblk();blk_get(index,lms('index'))
		if(match('where'  )){cw=quote()}else{cw=lmblk(),blk_lit(cw,ONE )}
		if(match('by'     )){cb=quote()}else{cb=lmblk(),blk_lit(cb,NONE)}
		if(match('orderby')){co=quote();dir=match('asc')?-1: match('desc')?1: er(`Expected 'asc' or 'desc'.`)}
		else{co=lmblk(),blk_get(co,lms('index'))}if(!match('from'))er(`Expected 'from'.`)
		expr(b),blk_op1(b,'@tab')
		blk_lit(b,cw),blk_op(b,op.COL),blk_lit(b,cb),blk_op(b,op.COL),blk_lit(b,co),blk_op(b,op.COL),blk_lit(b,lmn(dir)),blk_opa(b,op.QUERY,func=='@upd')
		const keys=lml(cols.k.concat([lms('@index')])),name=tempname()
		blk_op(b,op.ITER);const head=blk_here(b);blk_lit(b,[ls(name)]);const each=blk_opa(b,op.EACH,0);
			blk_lit(b,keys),blk_get(b,name),cols.v.map(x=>(blk_lit(b,x),blk_op(b,op.COL)))
			blk_lit(b,index),blk_op(b,op.COL),blk_op(b,op.DROP),blk_opa(b,op.BUND,count(keys)),blk_op2(b,'dict')
		blk_opa(b,op.NEXT,head),blk_sets(b,each,blk_here(b)),blk_lit(b,keys),blk_op3(b,func)
	}
	const parseindex=(b,name)=>{
		const i=[];while(peek().t in{'[':1,'.':1}){
			if(matchsp('['))i.push(quotesub())
			if(matchsp('.')){
				if(peek().t in{'[':1,'.':1}){
					i.map(v=>(blk_cat(b,v),blk_op(b,op.CALL)))
					blk_op(b,op.ITER);const head=blk_here(b);blk_lit(b,['x'])
					const each=blk_opa(b,op.EACH,0);blk_get(b,lms('x')),parseindex(b)
					blk_opa(b,op.NEXT,head),blk_sets(b,each,blk_here(b));return
				}else{i.push(quotedot())}
			}
		}
		if(matchsp(':')){
			i.map(v=>blk_cat(b,v)),blk_opa(b,op.BUND,i.length),blk_op(b,op.OVER)
			for(let z=0;z<i.length-1;z++)blk_opa(b,op.IPRE,z),blk_opa(b,op.IPOST,z);expr(b),blk_imm(b,op.AMEND,name||NONE)
		}else{i.map(v=>(blk_cat(b,v),blk_op(b,op.CALL)))}
	}
	const term=b=>{
		if(peek().t=='number'){blk_lit(b,lmn(next().v));return}
		if(peek().t=='string'){blk_lit(b,lms(next().v));return}
		if(match('if')){
			expr(b);let bail=blk_opa(b,op.JUMPF,0),e=0,c=0
			while(hasnext()&&!match('end')&&!(e=match('else'))){if(c)blk_op(b,op.DROP);expr(b),c++;}
			if(!c)blk_lit(b,NONE);const exit=blk_opa(b,op.JUMP,0);blk_sets(b,bail,blk_here(b))
			if(e){iblock(b)}else{blk_lit(b,NONE)}blk_sets(b,exit,blk_here(b));return
		}
		if(match('while')){
			blk_lit(b,NONE);const head=blk_here(b);expr(b);const cond=blk_opa(b,op.JUMPF,0)
			blk_op(b,op.DROP),iblock(b),blk_opa(b,op.JUMP,head),blk_sets(b,cond,blk_here(b));return
		}
		if(match('each')){
			const n=names('in','variable');expr(b),blk_op(b,op.ITER);const head=blk_here(b);blk_lit(b,n)
			const each=blk_opa(b,op.EACH,0);iblock(b),blk_opa(b,op.NEXT,head),blk_sets(b,each,blk_here(b));return
		}
		if(match('on')){
			const n=name('function'),a=names('do','argument')
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
			const n=lml([]);while(!match('into')){n.v.push(lms(peek().t=='string'?next().v:name('column'))),expect(':'),expr(b)}
			blk_opa(b,op.BUND,count(n)),blk_lit(b,n),expr(b),blk_op3(b,'@ins');return
		}
		if(matchsp('(')){if(matchsp(')')){blk_lit(b,lml([]));return}expr(b),expect(')');return}
		const s=peek().v;if(findop(s,monad)>=0&&peek().t in{'symbol':1,'name':1}){next(),expr(b),blk_op1(b,s);return}
		const n=lms(name('variable'));if(matchsp(':')){expr(b),blk_set(b,n);return}blk_get(b,n),parseindex(b,n)
	}
	const expr=b=>{
		term(b);if(peek().t in {'[':1,'.':1}){parseindex(b)}
		if(matchsp('@')){
			const temp=tempname(),names=['v','k','i'];blk_set(b,temp),blk_op(b,op.DROP)
			expr(b),blk_op(b,op.ITER);const head=blk_here(b);blk_lit(b,names);const each=blk_opa(b,op.EACH,0)
				blk_get(b,temp),names.map(n=>blk_get(b,lms(n))),blk_opa(b,op.BUND,3),blk_op(b,op.CALL)
			blk_opa(b,op.NEXT,head),blk_sets(b,each,blk_here(b));return
		}
		const s=peek().v;if(findop(s,dyad)>=0&&peek().t in {'symbol':1,'name':1}){next(),expr(b),blk_op2(b,s)}
	}
	const b=lmblk();if(hasnext())expr(b);while(hasnext())blk_op(b,op.DROP),expr(b)
	if(blk_here(b)==0)blk_lit(b,NONE);return b
}

env_local=(e,n,x)=>{e.v[ls(n)]=x}
env_getr =(e,n  )=>{const r=e.v[ls(n)];return r?r: e.p?env_getr(e.p,n): null}
env_setr =(e,n,x)=>{const r=e.v[ls(n)];return r?e.v[ls(n)]=x: e.p?env_setr(e.p,n,x): null}
env_get  =(e,n  )=>env_getr(e,n)||NONE
env_set  =(e,n,x)=>{const r=env_getr(e,n);r?env_setr(e,n,x):env_local(e,n,x)}
env_bind =(e,k,v)=>{const r=lmenv(e); k.map((a,i)=>env_local(r,lms(a),v.v[i]||NONE));return r}
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
	if(tail){descope()}issue(env_bind(f.c,f.a,a),f.b),calldepth=max(calldepth,state.e.length)
}
runop=_=>{
	const b=getblock();if(!liblk(b))ret(state.t.pop())
	const pc=getpc(),o=blk_getb(b,pc),imm=(oplens[o]==3?blk_gets(b,1+pc):0); setpc(pc+oplens[o])
	switch(o){
		case op.DROP :arg();break
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
		case op.EACH :{
			const n=arg(),r=arg(),s=arg();if(count(r)==count(s)){setpc(imm),ret(r);break}
			const z=count(r), v=lml([s.v[z],lid(s)?s.k[z]:lmn(z),lmn(z)]);
			state.e.push(env_bind(getev(),n,v)),ret(s),ret(r);break
		}
		case op.NEXT :{const v=arg(),r=arg(),s=arg();state.e.pop();if(lid(r))r.k.push(s.k[r.v.length]);r.v.push(v),ret(s),ret(r),setpc(imm);break}
		case op.COL  :{
			const ex=arg(),t=arg(),n=Object.keys(t.v),v=lml(Object.values(t.v).map(lml));ret(t)
			n.push('column'),v.v.push(t),issue(env_bind(getev(),n,v),ex);break
		}
		case op.QUERY:{
			const order_dir=ln(arg()),t=arg(),ct=lmn(count(t))
			const o=dyad.take(ct,lml(ll(arg()))),b=dyad.take(ct,lml(ll(arg()))),w=dyad.take(ct,lml(ll(arg()))),r=monad.rows(t)
			const uk=r.v.reduce((x,_,z)=>{if(lb(w.v[z]))dset(x,b.v[z],b.v[z]);return x},lmd([],[]))
			const rows=uk.v.map((v,group)=>{ // sort groups, select rows
				const lex_list=(x,y,a,ix)=>{
					if(x.length<ix&&y.length<ix)return 0;const xv=x[ix]||NONE,yv=y[ix]||NONE
					return lex_less(xv,yv)?a: lex_more(xv,yv)?!a: lex_list(x,y,a,ix+1)
				}
				const lex_less=(a,b)=>lil(a)&&lil(b)? lex_list(a.v,b.v,1,0): lb(dyad['<'](a,b))
				const lex_more=(a,b)=>lil(a)&&lil(b)? lex_list(a.v,b.v,0,0): lb(dyad['>'](a,b))
				const gp=range(r.v.length).filter(x=>lb(w.v[x])&&match(v,b.v[x])).sort((a,b)=>{
					if(lex_less(o.v[a],o.v[b]))return  order_dir
					if(lex_more(o.v[a],o.v[b]))return -order_dir
					return a-b // produce a stable sort
				})
				return monad.table(lml(gp.map((v,z)=>{const t=r.v[v];dset(t,lms('gindex'),lmn(z)),dset(t,lms('group'),lmn(group));return t})))
			});if(rows.length==0&&!imm)rows.push(dyad.take(NONE,t))
			ret(t),ret(lml(rows));break
		}
	}while(running()&&getpc()>=blk_here(getblock()))descope()
}

fchar=x=>x=='I'?'i': x=='B'?'b': x=='L'?'s': x
n_writecsv=([x,y,d])=>{
	let r='', spec=y?ls(y).split(''):[];const t=lt(x), c=Object.keys(t.v).length; d=d?ls(d)[0]:','
	while(spec.length<c)spec.push('s')
	let n=0;spec.forEach((x,i)=>{if(x=='_')return;if(n)r+=d;n++;r+=Object.keys(t.v)[i]||`c${i+1}`})
	rows(t).v.forEach(row=>{
		r+='\n';let n=0, cols=Object.keys(row.v);spec.forEach((x,i)=>{
			if(x=='_')return;if(n)r+=d;n++
			const sv=dyad.format(lms('%'+fchar(x)),row.v[cols[i]]).v; r+=(/["\n]/.test(sv)||sv.indexOf(d)>=0?`"${sv.replace(/"/g,'""')}"`:sv)
		})
	});return lms(r)
}
n_readcsv=([x,y,d])=>{
	let i=0,n=0, spec=y&&lis(y)?ls(y):null, text=count(x)?ls(x):'', r=lmt({}); d=d?ls(d)[0]:','
	const nv=_=>{let r='';while(text[i]&&text[i]!='\n'&&text[i]!=d)r+=text[i++];return r}, match=x=>text[i]==x?(i++,1):0
	while(i<text.length&&text[i]!='\n'){
		while(match(' '));const v=nv();if(!spec||(n<spec.length&&spec[n]!='_'))r.v[v]=[];n++;if(match('\n'))break;while(match(' '));match(d)
	}
	while(spec&&n<spec.length){if(spec[n]!='_'){r.v['c'+n]=[]};n++}
	if(!spec)spec='s'.repeat(Object.keys(r.v).length)
	let slots=0,slot=0;spec.split('').map(z=>{if(z!='_')slots++;});slots=min(slots,Object.keys(r.v).length),n=0
	if(i>=text.length)return r;while(i<=text.length){
		while(match(' '));
		let val='';if(match('"')){while(text[i]){if(match('"')){if(match('"')){val+='"'}else{break}}else{val+=text[i++]}}}else{val=nv()}
		if(spec[n]&&spec[n]!='_'){
			const k=Object.keys(r.v)[slot], x=(val[0]||'').toLowerCase(), s=spec[n]
			let sign=1,o=0; if(val[o]=='-')sign=-1,o++;if(val[o]=='$')o++;
			r.v[k].push(dyad.parse(lms('%'+fchar(s)),lms(val))),slot++
		};n++
		if(i>=text.length||text[i]=='\n'){
			while(n<spec.length){const u=spec[n++];if(u!='_'&&slot<slots)r.v[Object.keys(r.v)[slot++]].push(u=='s'?lms(''):NONE);}
			if(text[i]=='\n'&&i==text.length-1)break;i++,n=0,slot=0
		}else{while(match(' '));match(d)}
	};return r
}
n_writexml=([x])=>{
	const esc=x=>{const e={'&':'amp',"'":'apos','"':'quot','<':'lt','>':'gt'};return ls(x).replace(/[&'"<>]/g,x=>e[x]?`&${e[x]};`:x)}
	const rec=(x,tab)=>{
		if(lil(x))return x.v.map(x=>rec(x,tab)).join(''); if(!lid(x))return esc(x)+(tab?'\n':'')
		const t=ls(dget(x,lms('tag'))||lms('')),a=ld(dget(x,lms('attr'))||lmd()),c=ll(dget(x,lms('children'))||lml([]))
		const r=`<${t}${a.k.map((k,i)=>` ${ls(k)}="${esc(a.v[i])}"`).join('')}${c.length?'':'/'}>\n`
		return c.length?`${r}${c.map(x=>(' '.repeat(tab+2))+rec(x,tab+2)).join('')}${' '.repeat(tab)}</${t}>\n`:r
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
			if(xs())r+=' ';if(stop==t[i])break;r+=xe('amp','&')||xe('apos',"'")||xe('quot','"')||xe('lt','<')||xe('gt','>')||xe('nbsp',' ')||t[i++]
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
	const randelt=x=>lin(x)?lmn(randint(ln(x))): count(x)<1?NONE: l_at(x,lmn(randint(count(x))))
	let x=z[0]||NONE;if(z.length<2)return randelt(x)
	const y=ln(z[1]);if(y>=0){const r=[];for(let z=0;z<y;z++)r.push(randelt(x));return lml(r);}
	x=lin(x)?monad.range(x).v:ll(x);const p=range(x.length),r=[]
	for(let i=x.length-1;i>0;i--){const j=randint(i+1),t=p[j];p[j]=p[i],p[i]=t}
	for(let z=0;z<abs(y);z++)r.push(x[p[z%x.length]]);return lml(r)
}
let frame_count=0,audio_playing=0
interface_system=lmi((self,i,x)=>{
	if(!i)return NONE
	if(x){
		if(lis(i)&&i.v=='seed'){seed=0|ln(x);return x}
		if(lis(i)&&i.v=='fullscreen')return set_fullscreen(lb(x)),x
	}
	if(lis(i)&&i.v=='version'   )return lms(VERSION)
	if(lis(i)&&i.v=='platform'  )return lms('web')
	if(lis(i)&&i.v=='seed'      )return lmn(seed)
	if(lis(i)&&i.v=='fullscreen')return lmn(is_fullscreen())
	if(lis(i)&&i.v=='playing'   )return lmn(audio_playing)
	if(lis(i)&&i.v=='frame'     )return lmn(frame_count)
	if(lis(i)&&i.v=='now'       )return lmn(0|(new Date().getTime()/1000))
	if(lis(i)&&i.v=='ms'        )return lmn(0|(Date.now()))
	if(lis(i)&&i.v=='workspace' )return lmd(['allocs','depth'].map(lms),[allocs,calldepth].map(lmn))
	return x?x:NONE
},'system')
showt=(x,toplevel)=>{
	if(!toplevel)return `<TABLE...${Object.keys(x.v).length}>`;try{
	const w=Object.keys(x.v).map(k=>x.v[k].reduce((x,y)=>max(x,show(y).length+2),k.length+2))
	const s='+'+Object.keys(x.v).map((x,i)=>"-".repeat(w[i])).join('+')+'+'
	const v=(Object.values(x.v)[0]||[]).map((_,r)=>Object.keys(x.v).map(k=>' '+show(x.v[k][r])).map((f,i)=>f+(' '.repeat(max(0,w[i]-f.length)))))
	         .map(x=>`|${x.join('|')}|`).join('\n')
	return `${s}\n|${Object.keys(x.v).map((x,i)=>` ${x+(' '.repeat(w[i]-x.length-2))} `).join('|')}|\n${s}${v.length?'\n'+v+'\n'+s:''}`
	}catch(err){console.log('cannot serialize',x);throw err}
}
show=(x,toplevel)=>linat(x)?'on native x do ... end':
		lil(x)?`(${x.v.map(x=>show(x)).join(',')})`: lit(x)?showt(x,toplevel): lion(x)?`on ${x.n}${x.a.map(x=>' '+x).join('')} do ... end`:
		lis(x)?`"${x.v.split('').map(x=>({'\n':'\\n','\\':'\\\\','"':'\\"'})[x]||x).join('')}"`:
		lin(x)?fjson(x):lid(x)?`{${x.k.map((k,i)=>`${show(k)}:${show(x.v[i])}`).join(',')}}`:
		lii(x)?`<${x.n}>`:`<INVALID ${x}>`

// dom + utilities

FORMAT_VERSION=1, RTEXT_END=2147483647, SFX_RATE=8000, FRAME_QUOTA=MODULE_QUOTA=10*4096, TRANS_QUOTA=2*4096, LOOP_QUOTA=1*4096, ATTR_QUOTA=2*1024, ANTS=255
sleep_frames=0, sleep_play=0, pending_popstate=0
DEFAULT_HANDLERS=`
on link x do go[x] end
on drag x do if !me.locked|me.draggable me.line[(pointer.prev-me.offset)/me.scale x] end end
on order x do if !me.locked me.value:select orderby me.value[x] asc from me.value end end
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
	menu:'%%FNT0EA0BAgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAAwADAAMAAwADAAMAAAADAAMAAAAAAAAAAAwAAoACgAKA'+
	'AAAAAAAAAAAAAAAAAAAAAAAAACBIAEgB/ACQAJAD+AEgASAAAAAAAAAAAAAAABSAAcACoAOAA4ABwADgAOACoAHAAIAAA'+
	'AAAACW4AkgCUAGQACAAIABMAFIAkgCMAAAAAAAAACAAAeADMAM0AYQDOAMwAzADMAHgAAAAAAAAAAQAAgACAAIAAAAAAA'+
	'AAAAAAAAAAAAAAAAAAAAyAAQADAAMAAwADAAMAAwADAAEAAIAAAAAAAA4AAQABgAGAAYABgAGAAYABgAEAAgAAAAAAABQ'+
	'AAIACoAHAAqAAgAAAAAAAAAAAAAAAAAAAABQAAAAAAACAAIAD4ACAAIAAAAAAAAAAAAAAAAgAAAAAAAAAAAAAAAAAAAAD'+
	'AAMAAQACAAAAABQAAAAAAAAAAAAD4AAAAAAAAAAAAAAAAAAAAAgAAAAAAAAAAAAAAAAAAAADAAMAAAAAAAAAABQgACAAQ'+
	'ABAAIAAgAEAAQACAAIAAAAAAAAAABgAAeADMAMwAzADMAMwAzADMAHgAAAAAAAAABgAAMABwADAAMAAwADAAMAAwADAAA'+
	'AAAAAAABgAAeACMAAwADAAYADAAYADAAPwAAAAAAAAABgAA/AAYADAAeAAMAAwADACMAHgAAAAAAAAABwAADAAcACwATA'+
	'CMAP4ADAAMAAwAAAAAAAAABgAA/ADAAMAA+AAMAAwADACMAHgAAAAAAAAABgAAOABgAMAA+ADMAMwAzADMAHgAAAAAAAA'+
	'ABgAA/AAMAAwADAAYADAAMAAwADAAAAAAAAAABgAAeADMAMwAzAB4AMwAzADMAHgAAAAAAAAABgAAeADMAMwAzADMAHwA'+
	'DAAYAHAAAAAAAAAAAgAAAAAAAMAAwAAAAAAAAADAAMAAAAAAAAAAAgAAAAAAAMAAwAAAAAAAAADAAMAAQACAAAAABQAAA'+
	'AAYADAAYADAAGAAMAAYAAAAAAAAAAAABgAAAAAAAAAA/AAAAPwAAAAAAAAAAAAAAAAABQAAAADAAGAAMAAYADAAYADAAA'+
	'AAAAAAAAAABgAAeACMAAwAGAAwADAAAAAwADAAAAAAAAAACQAAAAA+AEEAnICkgKSAmwBAAD4AAAAAAAAABgAAeADMAMw'+
	'AzAD8AMwAzADMAMwAAAAAAAAABgAA+ADMAMwAzAD4AMwAzADMAPgAAAAAAAAABgAAeADEAMAAwADAAMAAwADEAHgAAAAA'+
	'AAAABgAA+ADMAMwAzADMAMwAzADMAPgAAAAAAAAABQAA+ADAAMAAwADwAMAAwADAAPgAAAAAAAAABQAA+ADAAMAAwADwA'+
	'MAAwADAAMAAAAAAAAAABgAAeADEAMAAwADcAMwAzADMAHgAAAAAAAAABgAAzADMAMwAzAD8AMwAzADMAMwAAAAAAAAAAg'+
	'AAwADAAMAAwADAAMAAwADAAMAAAAAAAAAABgAADAAMAAwADAAMAMwAzADMAHgAAAAAAAAABwAAxgDMANgA8ADgAPAA2AD'+
	'MAMYAAAAAAAAABQAAwADAAMAAwADAAMAAwADAAPgAAAAAAAAACgAAgEDAwOHA88C+wJzAiMCAwIDAAAAAAAAABwAAggDC'+
	'AOIA8gC6AJ4AjgCGAIIAAAAAAAAABgAAeADMAMwAzADMAMwAzADMAHgAAAAAAAAABgAA+ADMAMwAzAD4AMAAwADAAMAAA'+
	'AAAAAAABgAAeADMAMwAzADMAMwAzADMAHgADAAAAAAABgAA+ADMAMwAzAD4AMwAzADMAMwAAAAAAAAABQAAcADIAMAA4A'+
	'BwADgAGACYAHAAAAAAAAAABgAA/AAwADAAMAAwADAAMAAwADAAAAAAAAAABgAAzADMAMwAzADMAMwAzADMAHgAAAAAAAA'+
	'ABgAAzADMAMwAzADMAMwAzADIAPAAAAAAAAAACgAAzMDMwMzAzMDMwMzAzMDMgP8AAAAAAAAABgAAzADMAMwAzAB4AMwA'+
	'zADMAMwAAAAAAAAABgAAzADMAMwAzAB4ADAAMAAwADAAAAAAAAAABgAA/AAMAAwAGAAwAGAAwADAAPwAAAAAAAAAA+AAw'+
	'ADAAMAAwADAAMAAwADAAMAA4AAAAAAABYAAgABAAEAAIAAgABAAEAAIAAgAAAAAAAAAA+AAYABgAGAAYABgAGAAYABgAG'+
	'AA4AAAAAAABQAAIABQAIgAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAAAAAAAAAAAAAAAAAAAAP8AAAAAAAAAA4AAQAAgAAA'+
	'AAAAAAAAAAAAAAAAAAAAAAAAABgAAAAAAAHgAjAB8AMwAzADMAHwAAAAAAAAABgAAwADAAPgAzADMAMwAzADMAPgAAAAA'+
	'AAAABQAAAAAAAHAAyADAAMAAwADIAHAAAAAAAAAABgAADAAMAHwAzADMAMwAzADMAHwAAAAAAAAABgAAAAAAAHgAzADMA'+
	'PwAwADEAHgAAAAAAAAABQAAOABgAPAAYABgAGAAYABgAGAAAAAAAAAABgAAAAAAAHwAzADMAMwAzADMAHwADACMAHgABg'+
	'AAwADAAPgAzADMAMwAzADMAMwAAAAAAAAAAgAAwAAAAMAAwADAAMAAwADAAMAAAAAAAAAABQAAGAAAABgAGAAYABgAGAA'+
	'YABgAGACYAHAABgAAwADAAMwA2ADwAOAA8ADYAMwAAAAAAAAAAgAAwADAAMAAwADAAMAAwADAAMAAAAAAAAAACgAAAAAA'+
	'AP+AzMDMwMzAzMDMwMzAAAAAAAAABgAAAAAAAPgAzADMAMwAzADMAMwAAAAAAAAABgAAAAAAAHgAzADMAMwAzADMAHgAA'+
	'AAAAAAABgAAAAAAAPgAzADMAMwAzADMAPgAwADAAAAABgAAAAAAAHwAzADMAMwAzADMAHwADAAMAAAABQAAAAAAANgA4A'+
	'DAAMAAwADAAMAAAAAAAAAABQAAAAAAAHAAyADgAHAAOACYAHAAAAAAAAAABAAAYABgAPAAYABgAGAAYABgADAAAAAAAAA'+
	'ABgAAAAAAAMwAzADMAMwAzADMAHwAAAAAAAAABgAAAAAAAMwAzADMAMwAzADIAPAAAAAAAAAACgAAAAAAAMzAzMDMwMzA'+
	'zMDMgP8AAAAAAAAABgAAAAAAAMwAzADMAHgAzADMAMwAAAAAAAAABgAAAAAAAMwAzADMAMwAzADMAHwADACMAHgABgAAA'+
	'AAAAPwADAAYADAAYADAAPwAAAAAAAAAAyAAQABAAEAAQACAAEAAQABAAEAAIAAAAAAAAYAAgACAAIAAgACAAIAAgACAAI'+
	'AAgAAAAAAAA4AAQABAAEAAQAAgAEAAQABAAEAAgAAAAAAABgAAAAAAAGQAmAAAAAAAAAAAAAAAAAAAAAAACAAAAAAAAAA'+
	'AAAAAAAAAAADbANsAAAAAAAAA',
	body:'%%FNT0CAoBAQAAAAAAAAAAAAABAICAgICAAIAAAAMAoKAAAAAAAAAABQAAUPhQ+FAAAAAFIHCooHAoqHAgAAgAf5KUbhk'+
	'pRgAABzBIUCBUiJRiAAABAICAAAAAAAAAAAMgQICAgICAQCAAA4BAICAgICBAgAAFAFAg+CBQAAAAAAUAACAg+CAgAAAA'+
	'AgAAAAAAAABAQIAEAAAAAPAAAAAAAAEAAAAAAAAAgAAABBAQICBAQICAAAAFAHCIiIiIiHAAAAUAIGAgICAgIAAABQBwi'+
	'AgQIED4AAAFAPgQIHAIiHAAAAUAEDBQkPgQEAAABQD4gPAICIhwAAAFADBAgPCIiHAAAAUA+AgQECAgIAAABQBwiIhwiI'+
	'hwAAAFAHCIiHgIEGAAAAMAAABAAAAAQAAABAAAACAAAAAgIEAEAAAQIEAgEAAAAAUAAAD4APgAAAAABAAAQCAQIEAAAAA'+
	'FADBICBAgACAAAAcAOESaqqqcQDgABQAgIFBQ+IiIAAAFAPCIiPCIiPAAAAUAcIiAgICIcAAABQDgkIiIiJDgAAAEAPCA'+
	'gOCAgPAAAAQA8ICA4ICAgAAABQBwiICYiIhwAAAFAIiIiPiIiIgAAAIAQEBAQEBAQAAABQAICAgIiIhwAAAFAIiQoMCgk'+
	'IgAAAQAgICAgICA8AAABwCCxqqSgoKCAAAFAMjIqKiYmIgAAAUAcIiIiIiIcAAABQDwiIjwgICAAAAFAHCIiIiIqHAQAA'+
	'UA8IiI8KCQiAAABQBwiIBwCIhwAAAFAPggICAgICAAAAUAiIiIiIiIcAAABQCIiIhQUCAgAAAHAIKCVFQoKCgAAAUAiIh'+
	'QIFCIiAAABQCIiFAgICAgAAAEAPAQIECAgPAAAANgQEBAQEBAQGAABICAQEAgIBAQAAADYCAgICAgICBgAAMAQKAAAAAA'+
	'AAAABgAAAAAAAAD8AAACAIBAAAAAAAAAAAQAAABgEHCQcAAABACAgOCQkJDgAAAEAAAAYJCAkGAAAAQAEBBwkJCQcAAAB'+
	'AAAAGCQ8IBgAAAEADBA4EBAQEAAAAQAAABwkJCQcBBgBACAgOCQkJCQAAACAEAAwEBAQEAAAAMAIABgICAgICDABACAgJ'+
	'CgwKCQAAACAMBAQEBAQEAAAAcAAADskpKSkgAABAAAAOCQkJCQAAAEAAAAYJCQkGAAAAQAAADgkJCQ4ICABAAAAHCQkJB'+
	'wEBAEAAAAsMCAgIAAAAQAAABwgGAQ4AAAAwBAQOBAQEAgAAAEAAAAkJCQkHAAAAUAAACIUFAgIAAABwAAAIJUVCgoAAAF'+
	'AAAAiFAgUIgAAAQAAACQkJCQcBBgBAAAAPAgQIDwAAADIEBAQIBAQEAgAAGAgICAgICAgIAAA4BAQEAgQEBAgAAFAGiwA'+
	'AAAAAAAAAUAAAAAAAAAqAAA',
	mono:'%%FNT0CAsBBQAAAAAAAAAAAAAABQAAICAgICAAIAAABQAAUFBQAAAAAAAABQAAUPhQ+FAAAAAABQAgcKigcCiocCAABQA'+
	'ASKhQIFCokAAABQAAYJCgQKiQaAAABQAgICAAAAAAAAAABQAQICBAQEAgIBAABQAgEBAICAgQECAABQAAIKhwqCAAAAAA'+
	'BQAAACAg+CAgAAAABQAAAAAAAABgYCBABQAAAAAA+AAAAAAABQAAAAAAAAAwMAAABQgIEBAgIEBAgIAABQAAcIiYqMiIc'+
	'AAABQAAIGAgICAgIAAABQAAcIgIECBA+AAABQAAcIgIMAiIcAAABQAAEDBQkPgQEAAABQAA+IDwCAiIcAAABQAAcIDwiI'+
	'iIcAAABQAA+AgIECAgIAAABQAAcIiIcIiIcAAABQAAcIiIiHgIcAAABQAAADAwAAAwMAAABQAAAGBgAABgYCBABQAACBA'+
	'gQCAQCAAABQAAAAD4APgAAAAABQAAQCAQCBAgQAAABQAAcIgIECAAIAAABQBwiIio6LCAiHAABQAAcIiI+IiIiAAABQAA'+
	'8IiI8IiI8AAABQAAcIiAgICIcAAABQAA8IiIiIiI8AAABQAA+ICA8ICA+AAABQAA+ICA8ICAgAAABQAAcIiAmIiIcAAAB'+
	'QAAiIiI+IiIiAAABQAAICAgICAgIAAABQAACAgICIiIcAAABQAAiJCgwKCQiAAABQAAgICAgICA+AAABQAAiNioiIiIiA'+
	'AABQAAiMiomIiIiAAABQAAcIiIiIiIcAAABQAA8IiI8ICAgAAABQAAcIiIiIiIcAgABQAA8IiI8IiIiAAABQAAcIiAcAi'+
	'IcAAABQAA+CAgICAgIAAABQAAiIiIiIiIcAAABQAAiIiIUFAgIAAABQAAiIiIiKjYiAAABQAAiFAgICBQiAAABQAAiIiI'+
	'UCAgIAAABQAA+AgQIECA+AAABQAwICAgICAgIDAABYCAQEAgIBAQCAgABQAwEBAQEBAQEDAABQAgUIgAAAAAAAAABQAAA'+
	'AAAAAAA+AAABQBAIBAAAAAAAAAABQAAAAB4iIiYaAAABQAAgIDwiIiI8AAABQAAAABwiICAeAAABQAACAh4iIiIeAAABQ'+
	'AAAABwiPiAeAAABQAAGCBwICAgIAAABQAAAAB4iIiIeAhwBQAAgIDwiIiIiAAABQAAIAAgICAgIAAABQAAIAAgICAgICD'+
	'ABQAAgICQoOCQiAAABQAAICAgICAgMAAABQAAAADwqKioqAAABQAAAACwyIiIiAAABQAAAABwiIiIcAAABQAAAADwiIiI'+
	'8ICABQAAAAB4iIiIeAgIBQAAAACwyICAgAAABQAAAAB4gHAI8AAABQAAICB4ICAgGAAABQAAAACIiIiYaAAABQAAAACIi'+
	'FBQIAAABQAAAACoqKioUAAABQAAAACIUCBQiAAABQAAAACIiIiIeAhwBQAAAAD4ECBA+AAABQAYICAgwCAgIBgABSAgIC'+
	'AgICAgICAgBQDAICAgGCAgIMAABQAAaLAAAAAAAAAABQAAAAAAAAAAqAAA'
}
COLORS=[
	0xFFFFFFFF,0xFFFFFF00,0xFFFF6500,0xFFDC0000,0xFFFF0097,0xFF360097,0xFF0000CA,0xFF0097FF,
	0xFF00A800,0xFF006500,0xFF653600,0xFF976536,0xFFB9B9B9,0xFF868686,0xFF454545,0xFF000000,
]
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
prototype_is  =x=>lii(x)&&x.n=='prototype'
deck_is       =x=>lii(x)&&x.n=='deck'
card_is       =x=>lii(x)&&x.n=='card'
patterns_is   =x=>lii(x)&&x.n=='patterns'
module_is     =x=>lii(x)&&x.n=='module'
widget_is     =x=>lii(x)&&x.n in {button:1,field:1,grid:1,slider:1,canvas:1,contraption:1}
ikey  =(x,k)=>lis(x)&&x.v==k
ivalue=(x,k,d)=>k in x?x[k]:d
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
normalize_enum=(x,v)=>v in x?v:Object.keys(x)[0]
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
draw_line=(r,brush,pattern)=>{
	r=rint(r);const bsh=(z,x,y)=>(BRUSHES[(z*8)+y]>>(7-x))&1
	let dx=abs(r.w-r.x), dy=-abs(r.h-r.y), err=dx+dy, sx=r.x<r.w ?1:-1, sy=r.y<r.h?1:-1;while(1){
		if(brush==0){if(inclip(r))pix(r,pattern)}
		else{for(let b=0;b<8;b++)for(let a=0;a<8;a++){const h=rect(r.x+a-3,r.y+b-3);if(bsh(brush,a,b)&&inclip(h))pix(h,pattern)}}
		if(r.x==r.w&&r.y==r.h)break;let e2=err*2; if(e2>=dy)err+=dy,r.x+=sx; if(e2<=dx)err+=dx,r.y+=sy
	}
}
draw_box=(r,brush,pattern)=>{
	const size=frame.image.size
	if(r.w==0||r.h==0||!ron(r,rect(0,0,size.x,size.y)))return
	if(r.y         >=0)draw_line(rect(r.x      ,r.y      ,r.x+r.w-1,r.y      ),brush,pattern)
	if(r.y+r.h<=size.y)draw_line(rect(r.x      ,r.y+r.h-1,r.x+r.w-1,r.y+r.h-1),brush,pattern)
	if(r.x         >=0)draw_line(rect(r.x      ,r.y      ,r.x      ,r.y+r.h-1),brush,pattern)
	if(r.x+r.w<=size.x)draw_line(rect(r.x+r.w-1,r.y      ,r.x+r.w-1,r.y+r.h-1),brush,pattern)
}
draw_lines=(poly,brush,pattern)=>{for(let z=0;z<poly.length-1;z++)draw_line(rpair(poly[z],poly[z+1]),brush,pattern)}
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
	for(let a=0;a<=r.h;a++)for(let b=0;b<=r.w;b++){const h=rect(b+r.x,a+r.y);if(poly_in(poly,h))pix(h,pattern)}
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
			layout.push({pos:rpair(cursor,size),line:cursor.y,char:c,font,arg:NONE})
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
	const texts=table.v.text, fonts=table.v.font, args=table.v.arg
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

pointer={f:(self,i,x)=>{
	if(ikey(i,'held' ))return lmn(self.held)
	if(ikey(i,'pos'  ))return lmpair(self.pos)
	if(ikey(i,'start'))return lmpair(self.start)
	if(ikey(i,'prev' ))return lmpair(self.prev)
	if(ikey(i,'end'  ))return lmpair(self.end)
	return x?x:NONE
},t:'int',n:'pointer',held:0,pos:rect(),start:rect(),prev:rect(),end:rect()}

keystore_read=x=>{
	let store=lmd();if(x)x.k.filter((k,i)=>!match(NONE,x.v[i])).map((k,i)=>dset(store,k,x.v[i]))
	return {f:(self,i,x)=>{
		i=ls(i);if(i=='keys')return monad.range(self.data)
		if(x){
			const f=lms('%j'),val=dyad.parse(f,dyad.format(f,x))
			if(match(NONE,val)){self.data=dyad.drop(lms(i),self.data)}else{dset(self.data,lms(i),val)}
			return x
		}else{return dget(self.data,lms(i))||NONE}
	},t:'int',n:'keystore',data:store}
}

module_read=(x,deck)=>{
	const ri=lmi((self,i,x)=>{
		if(x){
			if(ikey(i,'description'))return self.description=ls(x),x
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
			if(ikey(i,'script'     ))return lms(self.script||'')
			if(ikey(i,'error'      ))return lms(self.error||'')
			if(ikey(i,'value'      ))return self.value
		}return x?x:NONE
	},'module')
	const n=dget(x,lms('name'))
	ri.deck=deck
	ri.name=ls(ukey(deck.modules,n&&lis(n)&&count(n)==0?null:n,'module'))
	ri.data=keystore_read(dget(x,lms('data')))
	ri.value=lmd()
	init_field(ri,'description',x)
	init_field(ri,'script',x)
	return ri
}
module_write=x=>{
	const r=lmd()
	dset(r,lms('name'  ),ifield(x,'name'))
	dset(r,lms('data'  ),x.data.data)
	dset(r,lms('script'),ifield(x,'script'))
	if(x.description)dset(r,lms('description'),ifield(x,'description'))
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
		return String.fromCharCode(ur(0,0))
	}
	const set_raw=(a,index,v)=>{
		if('string'==typeof v)v=v.charCodeAt(0)
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
			const r=(new TextDecoder('utf-8')).decode(new Uint8Array(range(len).map(x=>get_raw(a,index+x))))
			return a.cast=t,lms(clchars(r))
		}
		return len<0?lmn(get_raw(a,index)): lml(range(len).map(x=>lmn(get_raw(a,index+x))))
	}
	const set=(a,index,len,v)=>{
		if(len<0)len=1
		if(array_is(v)){for(let z=0;z<len;z++)set_raw(a,index+z,array_get_raw(b,z))}       // array copy
		else if(lis(v)){for(let z=0;z<len;z++)set_raw(a,index+z,z>=count(v)?0:v.v[z])}     // copy chars up to len
		else if(lil(v)){for(let z=0;z<len;z++)set_raw(a,index+z,z>=count(v)?0:ln(v.v[z]))} // copy numbers up to len
		else{const vv=ln(v);for(let z=0;z<len;z++)set_raw(a,index+z,vv)}                   // spread a number up to len
	}
	const slice=(a,z)=>{
		const o=offset(z[0]||NONE),cast=z[1]?normalize_enum(casts,ls(z[1])):a.cast, step=casts[cast];o.offset*=step
		if(o.len<0)o.len=0|((a.size-o.offset)/step);const r=array_make(o.len,cast,o.offset,a.data);r.slice=1;return r
	}
	const copy=(a,z)=>{
		const o=offset(z[0]||NONE),cast=z[1]?normalize_enum(casts,ls(z[1])):a.cast, step=casts[cast];o.offset*=step
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
		if(!lid(shape))return NONE;let bit=0,r=lmd();shape.v.map((type,i)=>{
			let v=NONE;if(!lin(type)&&bit)bit=0,a.here++
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
			let v=dget(value,shape.k[i])||NONE
			if(!lin(type)){if(bit)bit=0,a.here++;struct_write(a,type,v);return}
			let n=clamp(1,ln(type),31), t=ln(v),m=(1<<n)-1;t&=m,a.cast='u8';for(let z=0;z<n;z++){
				let pos=1<<(7-bit),dst=get_raw(a,a.here)&~pos
				set_raw(a,a.here,t&(1<<(n-1-z))?dst|pos:dst),bit++;if(bit==8)bit=0,a.here++
			}
		})
	}
	const struct=(a,z)=>{
		const oc=a.cast, shape=z[0]||NONE, value=z[1], size=struct_size(shape);if(value&&a.here+size>=a.size)resize(a,a.here+size)
		const r=value?(struct_write(a,shape,value),value):struct_read(a,shape);return a.cast=oc,r
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
		}return x?x:NONE
	},'array')
	ri.size=size*casts[cast],ri.here=0,ri.base=base,ri.cast=cast,ri.data=buffer||new Uint8Array(ri.size)
	return ri
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
image_paste_scaled=(r,clip,src,dst,opaque)=>{
	r=rint(r);if(r.w==0||r.h==0)return;const s=src.size,ds=dst.size;if(r.w==s.x&&r.h==s.y)return image_paste(r,clip,src,dst,opaque)
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		let sx=0|((b*1.0)/r.w)*s.x, sy=0|((a*1.0)/r.h)*s.y, c=src.pix[sx+sy*s.x]
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
	const os=i.size,ob=i.pix;size=rint(rmax(size,rect()));if(requ(os,size))return i
	i.pix=new Uint8Array(size.x*size.y),i.size=size;
	for(let a=0;a<size.y;a++)for(let b=0;b<size.x;b++)i.pix[b+a*size.x]=a>=os.y||b>=os.x?0: ob[b+a*os.x];return i
}
image_make=size=>{
	const f=(self,i,x)=>{
		const s=self.size
		if(i&&lil(i)){ // read/write pixels
			const p=getpair(i),ib=p.x>=0&&p.y>=0&&p.x<s.x&&p.y<s.y
			if(x){if(ib)self.pix[p.x+p.y*s.x]=ln(x);return x}
			return ib?lmn(self.pix[p.x+p.y*s.x]):NONE
		}
		if(ikey(i,'encoded'))return lms(image_write(self))
		if(ikey(i,'size'))return x?(image_resize(self,getpair(x)),x): lmpair(self.size)
		if(ikey(i,'map'))return lmnat(([x,fill])=>{
			const m=new Uint8Array(256);for(let z=0;z<256;z++)m[z]=fill?ln(fill):z;x=ld(x)
			for(let z=0;z<x.k.length;z++)m[0xFF&ln(x.k[z])]=0xFF&ln(x.v[z])
			for(let z=0;z<self.pix.length;z++)self.pix[z]=m[self.pix[z]]
			return self
		})
		if(ikey(i,'transform'))return lmnat(([x])=>{
			if(x.v=='horiz')image_flip_h(self); if(x.v=='vert')image_flip_v(self); if(x.v=='flip')image_flip(self); if(x.v=='dither')image_dither(self)
			if(x.v=='left' )image_flip_h(self),image_flip(self); if(x.v=='right')image_flip(self),image_flip_h(self)
			return self
		})
		if(ikey(i,'copy'))return lmnat(z=>image_copy(self,unpack_rect(z,self.size)))
		if(ikey(i,'paste'))return lmnat(([img,pos,t])=>{
			img=getimage(img), pos=(pos?ll(pos):[]).map(ln); let solid=t?!lb(t):1, cl=rect(0,0,self.size.x,self.size.y); if(img==self)img=image_copy(img)
			image_paste_scaled(pos.length<=2?rect(pos[0],pos[1],img.size.x,img.size.y):rect(pos[0],pos[1],pos[2],pos[3]),cl,img,self,solid)
		})
		return x?x:NONE
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

sound_make=data=>{
	const sign_extend=x=>(x<<24>>24)
	const ri=lmi((self,i,x)=>{
		if(i&&lin(i)){ // read/write single samples
			return x?((self.data[ln(i)]=0xFF&ln(x)),x):lmn(sign_extend(self.data[ln(i)]))
		}
		if(i&&lil(i)){ // read/write ranges
			const n=getpair(i);n.y=max(0,n.y);if(x){
				const s=ll(x),dc=self.data.length,sc=s.length, r=new Uint8Array(clamp(0,(dc-n.y)+sc,10*SFX_RATE))
				for(let z=0;z<n.x         ;z++)r[z]=self.data[z]
				for(let z=0;z<sc          ;z++)r[n.x+z   ]=0xFF&ln(s[z])
				for(let z=0;z<dc-(n.x+n.y);z++)r[n.x+sc+z]=0xFF&self.data[n.x+n.y+z]
				return self.data=r,x
			}else{return lml(range(n.y).map(x=>lmn(sign_extend(self.data[x+n.x]))))}
		}
		if(ikey(i,'encoded'))return lms(sound_write(self))
		if(ikey(i,'size')){
			if(!x)return lmn(self.data.length)
			const n=clamp(0,ln(x),10*SFX_RATE),o=self.data;self.data=new Uint8Array(n)
			for(let z=0;z<o.length&&z<n;z++)self.data[z]=o[z];return x
		}
		if(ikey(i,'duration'))return lmn(self.data.length/SFX_RATE)
		return x?x:NONE
	},'sound')
	if(data&&data.length>10*SFX_RATE)data=data.slice(0,10*SFX_RATE)
	ri.data=data||new Uint8Array(0)
	return ri
}
sound_read=x=>sound_make((typeof x=='string')?data_read('SND',x):new Uint8Array(clamp(0,+x,10*SFX_RATE)))
sound_write=x=>data_write('SND0',x.data)
n_sound=([x])=>!x?sound_read(0): lis(x)?sound_read(ls(x)): lin(x)?sound_read(ln(x)): sound_make(Uint8Array.from(ll(x).map(ln)))

patterns_read=x=>{
	const set=(pal,p,x,y,v)=>pal[(x%8)+(8*(y%8))+(8*8*p)]=v
	const ri=lmi((self,i,x)=>{
		let r=null, t=i&&ln(i)?ln(i):0
		if(x){
			if(t>= 2&&t<=27&&image_is(x)){for(let a=0;a<8;a++)for(let b=0;b<8;b++)set(self.pal.pix,t,b,a,lb(iwrite(x,lmpair(rect(b,a)))))}
			if(t>=28&&t<=31){r=ll(x);if(r.length>8)r=r.slice(0,8);self.anim[t-28]=r.map(x=>{const f=clamp(0,ln(x),47);return f>=28&&f<=31?0:f});r=lml(r)}
			if(t>=32&&t<=47){COLORS[t-32]=0xFF000000|ln(x);r=x}
		}else{
			if(t>= 0&&t<=27){r=image_copy(self.pal,rect(0,t*8,8,8))}
			if(t>=28&&t<=31){r=lml(self.anim[t-28].map(lmn))}
			if(t>=32&&t<=47){r=lmn(0xFFFFFF&COLORS[t-32])}
		}return r?r:x?x:NONE
	},'patterns')
	ri.pal=image_resize(image_read(x.patterns?ls(x.patterns):DEFAULT_PATTERNS),rect(8,8*32))
	ri.anim=JSON.parse(DEFAULT_ANIMS);if(x.animations&&lil(x.animations))ll(x.animations).map((x,i)=>iindex(ri,28+i,x))
	return ri
}
patterns_write=x=>image_write(image_resize(image_copy(x.pal),rect(8,224)))
anims_write=x=>lml(x.anim.map(x=>lml(x.map(lmn))))

font_get=(i,f,v)=>{if(v!=undefined)f.pix[i]=v;return f.pix[i]}
font_w =(f,v)=>font_get(0,f,v)
font_h =(f,v)=>font_get(1,f,v)
font_sw=(f,v)=>font_get(2,f,v)
font_gs=(f,v)=>font_h(f)*ceil(font_w(f)/8)+1
font_gb=(f,c)=>3+(c.charCodeAt(0)-32)*font_gs(f)
font_gw=(f,c,v)=>font_get(font_gb(f,c),f,v)
font_pp=(f,c,x,y,v)=>font_get(font_gb(f,c)+1+y*ceil(font_w(f)/8)+Math.floor(x/8),f,v)
font_bit=(x,v)=>(v<<(7-(x%8)))
font_gpix=(f,c,x,y)=>((font_pp(f,c,x,y)&font_bit(x,1))?1:0)
font_spix=(f,c,x,y,v)=>font_pp(f,c,x,y,(font_pp(f,c,x,y)&~font_bit(x,1))|font_bit(x,v))
font_textsize=(f,t)=>{
	const cursor=rect(),size=rect(0,font_h(f))
	for(let z=0;t[z];z++){if(t[z]!='\n'){cursor.x+=font_gw(f,t[z])+font_sw(f),size.x=max(size.x,cursor.x)}else{cursor.x=0,size.y+=font_h(f)}}
	return size
}
font_read=s=>{
	const ri=lmi((self,i,x)=>{
		if(lin(i)||(lis(i)&&count(i)==1)){ // read/write glyphs
			let ix=lin(i)?ln(i): ls(i).charCodeAt(0)-32, c=String.fromCharCode(ix+32)
			if(x){
				if(!image_is(x))return x;font_gw(self,c,min(x.size.x,font_w(self)));const s=rect(font_gw(self,c),font_h(self))
				for(let a=s.y-1;a>=0;a--)for(let b=s.x-1;b>=0;b--)font_spix(self,c,b,a, b>=(x.size.x||a>=x.size.y)?0:x.pix[b+a*x.size.x]?1:0)
				return x
			}
			if(ix<0||ix>95)return image_make(rect())
			const s=rect(font_gw(self,c),font_h(self)),r=image_make(s)
			for(let a=s.y-1;a>=0;a--)for(let b=s.x-1;b>=0;b--)r.pix[b+a*s.x]=font_gpix(self,c,b,a)
			return r
		}
		if(x){
			if(ikey(i,'space'))return font_sw(self,ln(x)),x
			if(ikey(i,'size')){
				const r=font_read(getpair(x));iwrite(r,lms('space'),ifield(self,'space'))
				for(let z=0;z<96;z++)iindex(r,z,iindex(self,z));self.pix=r.pix;return x
			}
		}else{
			if(ikey(i,'size'))return lmpair(rect(font_w(self),font_h(self)))
			if(ikey(i,'space'))return lmn(font_sw(self))
			if(ikey(i,'textsize'))return lmnat(([x])=>lmpair(font_textsize(self,x?ls(x):'')))
		}return x?x:NONE
	},'font')
	if(typeof s=='string')ri.pix=data_read('FNT',s)
	if(!ri.pix){ri.pix=new Uint8Array(3+96*(1+s.y*ceil(s.x/8.0)));font_w(ri,s.x),font_h(ri,s.y),font_sw(ri,1)}
	return ri
}
font_write=x=>data_write('FNT0',x.pix)

rtext_len=tab=>tab.v.text.reduce((x,y)=>x+ls(y).length,0)
rtext_get=(tab,n)=>{let i=0;for(let z=0;z<tab.v.text.length;z++){i+=count(tab.v.text[z]);if(i>=n)return z}return -1}
rtext_getr=(tab,x)=>{let i=0;for(let z=0;z<tab.v.text.length;z++){const c=count(tab.v.text[z]);if(i+c>=x)return rect(i,i+c);i+=c}return rect(x,x)}
rtext_make=(t,f,a)=>{
	a=!a?'':image_is(a)?a:ls(a), f=!f?'':ls(f), t=image_is(a)?'i':!t?'':count(t)?ls(t):''
	return lmt({text:[lms(t)],font:[lms(f)],arg:[image_is(a)?a:lms(a)]})
}
rtext_cast=x=>{
	if(!x)x=lms('');if(lid(x))x=monad.table(x);if(!lit(x))return rtext_make(x)
	if(x.v.text&&x.v.font&&x.v.arg&&x.v.text.every((t,i)=>lis(t)&&lis(x.v.font[i])&&(image_is(x.v.arg[i])||lis(x.v.arg[i]))))return x
	let r={text:lml(x.v.text||[lms('')]),font:lml(x.v.font||[lms('')]),arg:lml(x.v.arg||[lms('')])};torect(r)
	r.text.map((_,z)=>{let i=image_is(r.arg[z]);r.text[z]=i?lms('i'):lms(ls(r.text[z])),r.font[z]=lms(ls(r.font[z])),r.arg[z]=i?r.arg[z]:lms(ls(r.arg[z]))})
	return lmt(r)
}
rtext_append=(tab,t,f,a)=>{
	if(image_is(a)){if(count(t)>1)t=lms('i');if(count(t)<1)return 0;}if(!count(t))return 0;
	if(tab.v.text.length&&match(f,last(tab.v.font))&&!image_is(a)&&match(a,last(tab.v.arg))){tab.v.text[tab.v.text.length-1]=lms(last(tab.v.text).v+t.v)}
	else{tab.v.text.push(t),tab.v.font.push(f),tab.v.arg.push(a)};return count(t)
}
rtext_appendr=(tab,row)=>{row.v.text.map((t,i)=>rtext_append(tab,t,row.v.font[i],row.v.arg[i]))}
rtext_string=(tab,pos)=>{
	pos=pos||rect(0,RTEXT_END);let r='',i=0,a=min(pos.x,pos.y),b=max(pos.x,pos.y)
	tab.v.text.map(s=>{for(let z=0;z<s.v.length;z++,i++)if(i>=a&&i<b)r+=s.v[z]});return lms(r)
}
rtext_is_plain=x=>{
	if(!lit(x)||!x.v.text||!x.v.font||!x.v.arg||x.v.text.length>1)return 0;if(x.v.text.length==0)return 1
	return ls(x.v.font[0])==''&&!image_is(x.v.arg[0])&&ls(x.v.arg[0])==''
}
rtext_is_image=x=>{
	let r=null,t=x.v.text,a=x.v.arg; // look for at least one image, and other spans must be only whitespace.
	for(let z=0;z<count(x);z++){if(image_is(a[z])){if(!r)r=a[z]}else if(ls(t[z]).trim()!=''){return null}}
	return r
}
rtext_span=(tab,pos)=>{
	let r=dyad.take(NONE,tab), i=0,c=0,a=min(pos.x,pos.y),b=max(pos.x,pos.y), partial=_=>{
		let rr='';for(let z=0;z<count(tab.v.text[c]);z++,i++)if(i>=a&&i<b)rr+=tab.v.text[c].v[z]
		rtext_append(r,lms(rr),tab.v.font[c],tab.v.arg[c]),c++
	}
	while(c<tab.v.text.length&&(i+count(tab.v.text[c]))<a)i+=count(tab.v.text[c++])                                      ;if(c<tab.v.text.length&&i<=a)partial()
	while(c<tab.v.text.length&&(i+count(tab.v.text[c]))<b)i+=rtext_append(r,tab.v.text[c],tab.v.font[c],tab.v.arg[c]),c++;if(c<tab.v.text.length&&i< b)partial()
	return r
}
rtext_splice=(tab,font,arg,text,cursor,endcursor)=>{
	const a=min(cursor.x,cursor.y),b=max(cursor.x,cursor.y),r=rtext_cast()
	rtext_appendr(r,rtext_span(tab,rect(0,a)))
	rtext_append (r,lms(text),font,arg)
	rtext_appendr(r,rtext_span(tab,rect(b,RTEXT_END)))
	endcursor.x=endcursor.y=a+text.length;return r
}
rtext_write=x=>{const r=monad.cols(x),arg=dget(r,lms('arg'));if(arg){arg.v=arg.v.map(x=>image_is(x)?lms(image_write(x)):x)};return r}
interface_rtext=lmi((self,i,x)=>{
	if(ikey(i,'end'   ))return lmn(RTEXT_END)
	if(ikey(i,'make'  ))return lmnat(([t,f,a])=>rtext_make(t,f,a))
	if(ikey(i,'len'   ))return lmnat(([t])=>lmn(rtext_len(rtext_cast(t))))
	if(ikey(i,'get'   ))return lmnat(([t,n])=>lmn(rtext_get(rtext_cast(t),n?ln(n):0)))
	if(ikey(i,'string'))return lmnat(([t,i])=>rtext_string(rtext_cast(t),i?getpair(i):undefined))
	if(ikey(i,'span'  ))return lmnat(([t,i])=>rtext_span  (rtext_cast(t),i?getpair(i):undefined))
	if(ikey(i,'cat'   ))return lmnat(x=>{let r=lmt({text:[],font:[],arg:[]});x.map(x=>rtext_appendr(r,rtext_cast(x)));return r})
	return x?x:NONE
},'rtext')
button_styles={round:1,rect:1,check:1,invisible:1}
button_read=(x,card)=>{
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NONE
		if(x){
			if(ikey(i,'value'))return self.value=lb(x),x
			if(ikey(i,'text' ))return self.text=ls(x),x
			if(ikey(i,'style'))return self.style=normalize_enum(button_styles,ls(x)),x
		}else{
			if(ikey(i,'value'))return value_inherit(self,ls(i))||NONE
			if(ikey(i,'text' ))return lms(ivalue(self,ls(i),''))
			if(ikey(i,'style'))return lms(ivalue(self,ls(i),'round'))
			if(ikey(i,'size' ))return lmpair(ivalue(self,ls(i),rect(60,20)))
		}return interface_widget(self,i,x)
	},'button');ri.card=card
	init_field(ri,'text' ,x)
	init_field(ri,'style',x)
	init_field(ri,'value',x)
	return ri
}
button_write=x=>{
	const r=lmd([lms('type')],[lms('button')])
	if(x.text)dset(r,lms('text' ),lms(x.text))
	if(x.style&&x.style!='round')dset(r,lms('style'),lms(x.style))
	if(x.value!=undefined)dset(r,lms('value'),lmn(x.value))
	return r
}
field_styles={rich:1,plain:1,code:1}
field_aligns={left:1,center:1,right:1}
field_read=(x,card)=>{
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NONE
		if(x){
			if(ikey(i,'text'  ))return self.value=rtext_cast(lms(ls(x))),x
			if(ikey(i,'scroll'))return self.scroll=max(0,ln(x)),x
			if(ikey(i,'value' )){
				if(ls(ifield(self,'style'))!='rich'&&!rtext_is_plain(x))x=rtext_string(rtext_cast(x))
				return self.value=rtext_cast(x),x
			}
			if(ikey(i,'border'   ))return self.border=lb(x),x
			if(ikey(i,'scrollbar'))return self.scrollbar=lb(x),x
			if(ikey(i,'style'    ))return self.style=normalize_enum(field_styles,ls(x)),iwrite(self,lms('value'),ifield(self,'value')),x
			if(ikey(i,'align'    ))return self.align=normalize_enum(field_aligns,ls(x)),x
		}else{
			if(ikey(i,'text'     )){const v=value_inherit(self,'value');return v!=undefined?rtext_string(v):lms('')}
			if(ikey(i,'border'   ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'value'    ))return value_inherit(self,ls(i))||rtext_cast()
			if(ikey(i,'scroll'   ))return value_inherit(self,ls(i))||NONE
			if(ikey(i,'scrollbar'))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'style'    ))return lms(ivalue(self,ls(i),'rich'))
			if(ikey(i,'align'    ))return lms(ivalue(self,ls(i),'left'))
			if(ikey(i,'size'     ))return lmpair(ivalue(self,ls(i),rect(100,20)))
			if(ikey(i,'font'     ))return dget(self.card.deck.fonts,lms(self.font||(self.style=='code'?'mono':'body')))
		}return interface_widget(self,i,x)
	},'field');ri.card=card
	const rtext_read=x=>{
		if(lis(x))return x;x=ld(x)
		const a=dget(x,lms('arg'));if(a){dset(x,lms('arg'),lml(ll(a).map(a=>ls(a).startsWith('%%IMG')?image_read(ls(a)):lms(ls(a)))))}
		return rtext_cast(x)
	}
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
	if(x.scroll)dset(r,lms('scroll'),lmn(x.scroll))
	if(x.value){if(rtext_is_plain(x.value)){const v=rtext_string(x.value);if(ls(v))dset(r,lms('value'),v)}else{dset(r,lms('value'),rtext_write(x.value))}}
	return r
}
slider_styles={horiz:1,vert:1,bar:1,compact:1}
slider_normalize=(self,n)=>{const i=getpair(ifield(self,'interval')),s=ln(ifield(self,'step'));return clamp(i.x,Math.round(n/s)*s,i.y)}
slider_read=(x,card)=>{
	const update=self=>iwrite(self,lms('value'),ifield(self,'value'))
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NONE
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
	if(x.value!=undefined&&x.value!=0)dset(r,lms('value'),lmn(x.value))
	if(x.step!=undefined&&x.step!=1)dset(r,lms('step'),lmn(x.step))
	if(x.format&&x.format!='%f')dset(r,lms('format'),lms(x.format))
	if(x.style&&x.style!='horiz')dset(r,lms('style'),lms(x.style))
	return r
}
grid_read=(x,card)=>{
	const ints=(x,n)=>{const r=[];for(let z=0;z<n&&z<x.length;z++)r.push(ln(x[z]));return r}
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NONE
		if(x){
			if(ikey(i,'value'    ))return self.value=lt(x),x
			if(ikey(i,'scroll'   ))return self.scroll=max(0,ln(x)),x
			if(ikey(i,'row'      ))return self.row=max(-1,ln(x)),x
			if(ikey(i,'scrollbar'))return self.scrollbar=lb(x),x
			if(ikey(i,'headers'  ))return self.headers=lb(x),x
			if(ikey(i,'lines'    ))return self.lines=lb(x),x
			if(ikey(i,'widths'   ))return self.widths=ints(ll(x),255),x
			if(ikey(i,'format'   ))return self.format=ls(x),x
		}else{
			if(ikey(i,'value'    ))return value_inherit(self,ls(i))||lmt({})
			if(ikey(i,'scroll'   ))return value_inherit(self,ls(i))||NONE
			if(ikey(i,'scrollbar'))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'headers'  ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'lines'    ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'widths'   ))return lml((ivalue(self,ls(i),[])).map(lmn))
			if(ikey(i,'format'   ))return lms(ivalue(self,ls(i),''))
			if(ikey(i,'size'     ))return lmpair(ivalue(self,ls(i),rect(100,50)))
			if(ikey(i,'row'      )){const r=value_inherit(self,ls(i))||lmn(-1);return lmn(clamp(-1,ln(r),count(ifield(self,'value'))-1))}
			if(ikey(i,'rowvalue' )){const r=ln(ifield(self,'row')),v=ifield(self,'value');return r<0||r>=count(v)?lmd():l_at(v,lmn(r))}
		}return interface_widget(self,i,x)
	},'grid');ri.card=card
	init_field(ri,'scrollbar',x)
	init_field(ri,'headers'  ,x)
	init_field(ri,'lines'    ,x)
	init_field(ri,'widths'   ,x)
	init_field(ri,'format'   ,x)
	init_field(ri,'scroll'   ,x)
	init_field(ri,'row'      ,x)
	{const k=lms('value'),v=dget(x,k);if(v)iwrite(ri,k,monad.table(v))}
	return ri
}
grid_write=x=>{
	const r=lmd([lms('type')],[lms('grid')])
	if(x.scrollbar!=undefined)dset(r,lms('scrollbar'),lmn(x.scrollbar))
	if(x.headers!=undefined)dset(r,lms('headers'),lmn(x.headers))
	if(x.lines!=undefined)dset(r,lms('lines'),lmn(x.lines))
	if(x.widths)dset(r,lms('widths'),lml(x.widths.map(lmn)))
	if(x.format)dset(r,lms('format'),lms(x.format))
	if(x.value)dset(r,lms('value'),monad.cols(x.value))
	if(x.scroll)dset(r,lms('scroll'),lmn(x.scroll))
	if(x.row!=undefined&&x.row!=-1)dset(r,lms('row'),lmn(x.row))
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
		}return NONE
	}
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NONE
		if(x){
			if(ikey(i,'brush'    ))return self.brush  =0|clamp(0,ln(x), 23),x
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
			if(!lis(i)){const img=container_image(self,0);return img?img.f(img,i,x):NONE}
			if(ikey(i,'border'   ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'draggable'))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'brush'    ))return lmn(ivalue(self,ls(i),0))
			if(ikey(i,'pattern'  ))return lmn(ivalue(self,ls(i),1))
			if(ikey(i,'size'     ))return lmpair(ivalue(self,ls(i),rect(100,100)))
			if(ikey(i,'scale'    ))return lmn(ivalue(self,ls(i),1.0))
			if(ikey(i,'lsize'    )){const s=getpair(ifield(self,'size')),z=ln(ifield(self,'scale'));return lmpair(rect(ceil(s.x/z),ceil(s.y/z)))}
			if(ikey(i,'clip'     ))return lmnat(z=>(canvas_clip(self,z),NONE))
			if(ikey(i,'clear'    ))return lmnat(z=>(canvas_pick(self),draw_rect(wid_crect(self,z),0            )           ,NONE))
			if(ikey(i,'rect'     ))return lmnat(z=>(canvas_pick(self),draw_rect(wid_crect(self,z),frame.pattern)           ,NONE))
			if(ikey(i,'invert'   ))return lmnat(z=>(canvas_pick(self),draw_invert_raw(wid_pal(self),wid_crect(self,z))     ,NONE))
			if(ikey(i,'box'      ))return lmnat(z=>(canvas_pick(self),draw_box(wid_rect(self,z),frame.brush,frame.pattern) ,NONE))
			if(ikey(i,'poly'     ))return lmnat(z=>(canvas_pick(self),draw_poly(unpack_poly(z),frame.pattern)              ,NONE))
			if(ikey(i,'line'     ))return lmnat(z=>(canvas_pick(self),draw_lines(unpack_poly(z),frame.brush,frame.pattern) ,NONE))
			if(ikey(i,'fill'     ))return lmnat(([pos])=>(canvas_pick(self),draw_fill(rint(getpair(pos)),self.pattern)     ,NONE))
			if(ikey(i,'copy'     ))return lmnat(z=>{const img=container_image(self,1);return image_copy(img,unpack_rect(z,img.size))})
			if(ikey(i,'paste'    ))return lmnat(([img,pos,t])=>{
				canvas_pick(self);const dst=container_image(self,1)
				img=getimage(img),pos=(pos?ll(pos):[]).map(ln); let solid=t?!lb(t):1
				image_paste_scaled(pos.length<=2?rect(pos[0],pos[1],img.size.x,img.size.y):rect(pos[0],pos[1],pos[2],pos[3]),frame.clip,img,dst,solid)
				return NONE
			})
			if(ikey(i,'merge'))return lmnat(z=>{
				canvas_pick(self);if(lil(z[0]))z=ll(z[0]);const nice=x=>x&&image_is(x)&&x.size.x>0&&x.size.y>0, size=frame.image.size
				for(let y=0;y<size.y;y++)for(let x=0;x<size.x;x++){
					const h=rect(x,y);if(!inclip(h))continue;let p=gpix(h),c=0;
					if(nice(z[p])){const i=z[p];c=i.pix[(x%i.size.x)+(y%i.size.y)*i.size.x]}
					pix(h,c)
				}return NONE
			})
			if(ikey(i,'text'))return lmnat(([x,pos,a])=>(canvas_pick(self),text(x=lit(x)?rtext_cast(x):lms(ls(x)),pos,a)))
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
	if(x.image    )dset(r,lms('image'    ),lms(image_write(x.image)))
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
	const masks={name:1,index:1,image:1,script:1,locked:1,animated:1,pos:1,show:1,font:1,event:1,offset:1}
	const ri=lmi((self,i,x)=>{
		if(!is_rooted(self))return NONE
		if(x){
			if(ikey(i,'def'  ))return x // not mutable!
			if(ikey(i,'image'))return x // not mutable!
			if(ikey(i,'size' )){
				const m=self.def.margin
				return self.size=rmax(rect(m.x+m.w,m.y+m.h),getpair(x)),reflow(self),x
			}
			if(lis(i)&&ls(i) in masks)return interface_widget(self,i,x)
			return fire_attr_sync(self,'set_'+ls(i),x),x
		}else{
			if(ikey(i,'def'  ))return self.def
			if(ikey(i,'size' ))return lmpair(self.size)
			if(ikey(i,'image'))return ifield(self.def,'image')
			if(lis(i)&&ls(i) in masks)return interface_widget(self,i,x)
			return fire_attr_sync(self,'get_'+ls(i),null)
		}
	},'contraption')
	ri.card   =card
	ri.deck   =card.deck
	ri.def    =def
	ri.size   =getpair(dget(x,lms('size'))||ifield(def,'size'))
	ri.widgets=lmd()
	let w=dget(x,lms('widgets')),d=def.widgets;if(w){w=ld(w)}else{w=lmd();def.widgets.k.map(k=>dset(w,k,lmd()))}
	d.k.map((k,i)=>{const a=widget_write(d.v[i]),o=dget(w,k);widget_add(ri,o?dyad[','](a,o):a)})
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
		if(ikey(i,'size'    ))return self.size=rint(rclamp(rect(),getpair(x),rect(4096,4096))),x
		if(ikey(i,'pos'     ))return self.pos=rint(getpair(x)),x
		if(ikey(i,'show'    ))return self.show=normalize_enum(widget_shows,ls(x)),x
	}else{
		if(ikey(i,'name'    ))return lms(self.name)
		if(ikey(i,'index'   ))return lmn(dvix(self.card.widgets,self))
		if(ikey(i,'script'  ))return lms(ivalue(self,ls(i),''))
		if(ikey(i,'locked'  ))return lmn(ivalue(self,ls(i),0))
		if(ikey(i,'animated'))return lmn(ivalue(self,ls(i),0))
		if(ikey(i,'pos'     ))return lmpair(ivalue(self,ls(i),rect()))
		if(ikey(i,'show'    ))return lms(ivalue(self,ls(i),'solid'))
		if(ikey(i,'font'    ))return dget(self.card.deck.fonts,lms(ivalue(self,ls(i),button_is(self)?'menu':'body')))
		if(ikey(i,'event'   ))return lmnat(args=>n_event(self,args))
		if(ikey(i,'offset')){
			let c=getpair(ifield(self.card,'size')), p=self.pos, d=self.card.deck.size, con=self.card
			while(contraption_is(con)){p=radd(p,con.pos),con=con.card,c=getpair(ifield(con,'size'))}
			return lmpair(radd(p,rcenter(rect(0,0,d.x,d.y),c)))
		}
	}return x?x:NONE
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
	if(x.script  )dset(r,lms('script'  ),lms(x.script))
	if(x.font&&x.font!=(button_is(x)?"menu":"body"))dset(r,lms('font'),lms(x.font))
	if(x.show&&x.show!='solid')dset(r,lms('show'),lms(x.show))
	return dyad[','](r,button_is(x)?button_write(x): field_is (x)?field_write (x):slider_is(x)?slider_write(x):
	                   grid_is  (x)?grid_write  (x): canvas_is(x)?canvas_write(x):contraption_is(x)?contraption_write(x): lmd())
}

widget_strip=x=>dyad.take(lml([lms('name'),lms('type')]),x)
widget_add=(card,x)=>{const r=widget_read(x,card);if(lii(r))dset(card.widgets,ifield(r,'name'),r);return r}
card_add=(card,type,name,n2)=>{
	if(prototype_is(card)&&(contraption_is(type)||ls(type)=='contraption'))return NONE
	if(lis(type)){
		if(ls(type)=='contraption'){
			const defs=card.deck.contraptions, ct=lms(name?ls(name):''), def=dget(defs,ct);if(!def)return NONE
			const a=lmd(['type','def'].map(lms),[lms('contraption'),ct]);if(n2)dset(a,lms('name'),lms(ls(n2)));return widget_add(card,a)
		}
		if(!ls(type)in{button:1,field:1,slider:1,canvas:1,grid:1})return NONE
		const a=lmd([lms('type')],[type]);if(name)dset(a,lms('name'),lms(ls(name)));return widget_add(card,a)
	}
	if(widget_is(type)){const a=widget_write(type);if(name)dset(a,lms('name'),name);return widget_add(card,a)}
	return NONE
}
card_remove=(card,x)=>{
	if(!widget_is(x)||!dkey(card.widgets,x))return 0
	const name=ifield(x,'name');dget(card.widgets,name).dead=true,card.widgets=dyad.drop(name,card.widgets);return 1
}
con_copy_raw=(card,z)=>z.filter(w=>widget_is(w)&&w.card==card).map(widget_write)
con_paste_raw=(card,payload)=>payload.map(p=>widget_add(card,ld(p)))
con_copy=(card,z)=>{
	z=lil(z)?ll(z):[z];const wids=lml(con_copy_raw(card,z)),defs=lmd(),v=lmd(['w','d'].map(lms),[wids,defs])
	const condefs=card.deck.contraptions;wids.v.map(wid=>{
		const type=dget(wid,lms('type')),def=dget(wid,lms('def'))
		if(ls(type)=='contraption'&&dget(defs,def)==null)dset(defs,def,prototype_write(dget(condefs,def)))
	});return lms(`%%WGT0${fjson(v)}`)
}
merge_prototypes=(deck,defs,uses)=>{
	const condefs=deck.contraptions;defs.v.map(def=>{
		const name=dget(def,lms('name'));let desc=dget(def,lms('description'));if(!lis(name))return;if(!desc)desc=lms('')
		if(condefs.v.some(con=>match(name,ifield(con,'name'))&&match(desc,ifield(con,'description'))))return
		const p=prototype_read(def,deck),nn=ifield(p,'name');dset(condefs,nn,p)
		uses.map(wid=>{
			const type=dget(wid,lms('type')),def=dget(wid,lms('def'))
			if(lis(type)&&ls(type)=='contraption'&&lis(def)&&ls(def)==ls(name))dset(wid,lms('def'),nn)
		})
	})
}
con_paste=(card,z)=>{
	if(!lis(z)||!z.v.startsWith('%%WGT0'))return NONE
	const v=ld(pjson(ls(z),6,count(z)-6).value),defs=dget(v,lms('d'));let wids=dget(v,lms('w'));wids=wids?ll(wids):[]
	merge_prototypes(card.deck,defs?ld(defs):lmd(),wids);return lml(con_paste_raw(card,wids))
}
card_read=(x,deck,cdata)=>{
	x=ld(x);const nav_dirs={right:1,left:1,up:1,down:1},ri=lmi((self,i,x)=>{
		if(self.dead)return NONE
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
			if(ikey(i,'image'  ))return self.image||image_make(rect())
			if(ikey(i,'add'    ))return lmnat(([t,n1,n2])=>card_add(self,t,n1,n2))
			if(ikey(i,'remove' ))return lmnat(([x])=>lmn(card_remove(self,x)))
			if(ikey(i,'event'  ))return lmnat(args=>n_event(self,args))
			if(ikey(i,'copy'    )&&state.external)return lmnat(([z])=>con_copy(self,z))
			if(ikey(i,'paste'   )&&state.external)return lmnat(([z])=>con_paste(self,z))
		}return x?x:NONE
	},'card')
	const n=dget(x,lms('name'))
	ri.deck=deck
	ri.widgets=lmd()
	ri.name=ls(ukey(deck.cards,n&&lis(n)&&count(n)==0?null:n,'card'))
	ri.script=ls(dget(x,lms('script'))||lms(''))
	{const v=dget(x,lms('image'));if(v)ri.image=image_read(ls(v))}
	ll(dget(x,lms('widgets'))||lml([])).filter(w=>dget(w,lms('name'))).map(w=>{const i=widget_read(w,ri);if(lii(i))dset(ri.widgets,ifield(i,'name'),i)})
	return ri
}
card_write=card=>{
	const r=lmd(),wids=lmd()
	dset(r,lms('name'),lms(card.name)),dset(r,lms('widgets'),wids)
	if(card.script.length)dset(r,lms('script'),lms(card.script))
	if(card.image)dset(r,lms('image'),lms(image_write(card.image)))
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
			if(grid_is  (w))p=dyad.take(lml(['value','scroll','row'].map(lms)),p)
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
normalize_margin=(x,p)=>{
	const m=rint(getrect(x)), s=getpair(ifield(p,'size'))
	return rmax(rect(min(m.x,s.x),min(m.y,s.y),min(m.w,s.x-m.x),min(m.h,s.y-m.y)),rect(0,0,0,0))
}
prototype_read=(x,deck)=>{
	x=ld(x)
	const attribute_types={'':1,bool:1,number:1,string:1,code:1,rich:1}
	const normalize_attributes=x=>{
		const r=lmt({name:[],label:[],type:[]});if(!lit(x))return r
		const sn=x.v.name,sl=x.v.label||sn,st=x.v.type;if(sn&&st)sn.filter(n=>lis(n)&&count(n)).map((n,i)=>{
			const type=normalize_enum(attribute_types,ls(st[i]))
			if(type.length)r.v.name.push(n),r.v.label.push(lms(ls(sl[i]))),r.v.type.push(lms(type))
		});return r
	}
	const ri=lmi((self,i,x)=>{
		if(self.dead)return NONE
		if(x){
			if(ikey(i,'name')){
				const defs=self.deck.contraptions, o=self.name, n=ukey(defs,lms(ls(x)),ls(x),lms(o))
				defs.k[dvix(defs,self)]=n,self.name=ls(n);return x
			}
			if(ikey(i,'description'))return self.description=ls(x),x
			if(ikey(i,'size'       ))return self.size=rint(getpair(x)),contraption_update(deck,self),x
			if(ikey(i,'margin'     ))return self.margin=normalize_margin(x,self),contraption_update(deck,self),x
			if(ikey(i,'resizable'  ))return self.resizable=lb(x),contraption_update(deck,self),x
			if(ikey(i,'image'      ))return self.image=image_is(x)?x:image_make(rect(0,0)),x
			if(ikey(i,'script'     ))return self.script=ls(x),x
			if(ikey(i,'template'   ))return self.template=ls(x),x
			if(ikey(i,'attributes' ))return self.attributes=normalize_attributes(x),x
		}else{
			if(ikey(i,'name'       ))return lms(self.name)
			if(ikey(i,'description'))return lms(self.description||'')
			if(ikey(i,'script'     ))return lms(self.script||'')
			if(ikey(i,'template'   ))return lms(self.template||'')
			if(ikey(i,'size'       ))return lmpair(self.size)
			if(ikey(i,'margin'     ))return lmrect(self.margin)
			if(ikey(i,'resizable'  ))return lmn(self.resizable)
			if(ikey(i,'image'      ))return self.image
			if(ikey(i,'widgets'    ))return self.widgets
			if(ikey(i,'attributes' ))return self.attributes||normalize_attributes(NONE)
			if(ikey(i,'add'        ))return lmnat(([t,n1,n2])=>{const r=card_add(self,t,n1,n2);if(widget_is(r))contraption_update(deck,self);return r})
			if(ikey(i,'remove'     ))return lmnat(([x])=>{const r=card_remove(self,x);if(lb(r))contraption_update(deck,self);return r})
			if(ikey(i,'update'     ))return lmnat(_=>{contraption_update(deck,self);return NONE})
		}return x?x:NONE
	},'prototype')
	ri.deck   =deck
	ri.widgets=lmd()
	{const v=dget(x,lms('name'      ));ri.name=ls(ukey(deck.contraptions,v&&lis(v)&&count(v)==0?null:v,'prototype'))}
	{const v=dget(x,lms('image'     ));ri.image=v?image_read(ls(v)):image_make(range(0,0))}
	{const v=dget(x,lms('attributes'));if(v)iwrite(ri,lms('attributes'),monad.table(v))}
	{const v=dget(x,lms('size'      ));ri.size=v?rint(getpair(v)):rect(100,100)}
	{const v=dget(x,lms('resizable' ));ri.resizable=v?lb(v):0}
	let w=dget(x,lms('widgets'));if(lid(w)){w.v.map((v,i)=>dset(v,lms('name'),w.k[i]))}
	(w?ll(w):[]).map(w=>{const n=dget(w,lms('name'));if(n){const i=widget_read(w,ri);if(lii(i))dset(ri.widgets,ifield(i,'name'),i)}})
	init_field(ri,'description',x)
	init_field(ri,'script'     ,x)
	init_field(ri,'template'   ,x)
	ri.margin=normalize_margin(dget(x,lms('margin'))||NONE,ri)
	return ri
}
prototype_write=x=>{
	const r=lmd(), wids=lmd(), nice=x=>x&&image_is(x)&&x.size.x>0&&x.size.y>0
	dset(r,lms('name'),lms(x.name))
	dset(r,lms('size'),ifield(x,'size'))
	if(x.resizable)dset(r,lms('resizable'),ONE)
	dset(r,lms('margin'),ifield(x,'margin'))
	if(x.description&&x.description.length)dset(r,lms('description'),lms(x.description))
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
	const unpack_name=x=>x?lms(ls(x)):NONE
	if(font_is(type))return uset(deck.fonts,unpack_name(y),'font',font_read(font_write(type)))
	if(ikey(type,'font'))return uset(deck.fonts,unpack_name(z),'font',font_read(getpair(y)))
	if(sound_is(type))return uset(deck.sounds,unpack_name(y),'sound',sound_read(sound_write(type)))
	if(ikey(type,'sound'))return uset(deck.sounds,unpack_name(z),'sound',sound_read(y?ln(y):0))
	if(module_is(type)){const a=module_write(type);if(y)dset(a,lms('name'),lms(ls(y)));const r=module_read(a,deck);return dset(deck.modules,ifield(r,'name'),r),r}
	if(ikey(type,'module')){const a=lmd();if(y)dset(a,lms('name'),lms(ls(y)));const r=module_read(a,deck);return dset(deck.modules,ifield(r,'name'),r),r}
	if(card_is(type))return deck_paste(deck,deck_copy(deck,type),y?lms(ls(y)):null)
	if(ikey(type,'card')){const a=lmd();if(y)dset(a,lms('name'),lms(ls(y)));const r=card_read(a,deck);return dset(deck.cards,ifield(r,'name'),r),r}
	if(prototype_is(type)){const a=prototype_write(type);if(y)dset(a,lms('name'),lms(ls(y)));const r=prototype_read(a,deck);return dset(deck.contraptions,ifield(r,'name'),r),r}
	if(ikey(type),'contraption'){const a=lmd();if(y)dset(a,lms('name'),lms(ls(y)));const r=prototype_read(a,deck);return dset(deck.contraptions,ifield(r,'name'),r),r}
	return NONE
}
deck_remove=(deck,t)=>{
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
		deck.cards=dyad.drop(dkey(deck.cards,t)||NONE,deck.cards),t.dead=true
		if(deck.card>=count(deck.cards))deck.card=count(deck.cards-1)
		deck.history=[ln(ifield(ifield(deck,'card'),'index'))]
		return 1
	}return 0
}
deck_copy=(deck,z)=>{
	if(!card_is(z))return NONE;const defs=lmd(),v=lmd(['c','d'].map(lms),[card_write(z),defs])
	z.widgets.v.filter(contraption_is).map(wid=>{const d=wid.def,n=ifield(d,'name');if(dget(defs,n)==null)dset(defs,n,prototype_write(d))})
	return lms(`%%CRD0${fjson(v)}`)
}
deck_paste=(deck,z,name)=>{
	if(!lis(z)||!ls(z).startsWith('%%CRD0'))return NONE
	const v=ld(pjson(ls(z),6,count(z)-6).value);let payload=dget(v,lms('c')),defs=dget(v,lms('d'));payload=payload?ld(payload):lmd()
	const wids=dget(payload,lms('widgets'));if(wids&&lid(wids))wids.v.map((v,i)=>dset(v,lms('name'),wids.k[i]))
	merge_prototypes(deck,defs?ld(defs):lmd(),wids?ll(wids):[]);const r=card_read(payload,deck);dset(deck.cards,name||ifield(r,'name'),r);return r
}
deck_read=x=>{
	const deck={},scripts={},cards={},modules={},defs={}, fonts=lmd(),sounds=lmd(); let i=0,m=0,md=0,lc=0
	Object.keys(FONTS).map(k=>dset(fonts,lms(k),font_read(FONTS[k])))
	const match=k=>x.startsWith(k,i)?(i+=k.length,1):0
	const end=_=>i>=x.length||x.startsWith('<\/script>',i)
	const str=e=>{let r='';while(!end()&&!match(e))r+=match('{l}')?'{': match('{r}')?'}': match('{c}')?':': match('{s}')?'/': x[i++];return clchars(r)}
	const last=dict=>{const k=Object.keys(dict);return dict[k[k.length-1]]||lmd()}
	match('<body><script language="decker">');while(!end()){
		if(x[i]=='\n')i++
		else if(x[i]=='#')while(!end()&&x[i]!='\n')i++
		else if(match('{deck}\n'   ))m=1
		else if(match('{fonts}\n'  ))m=2
		else if(match('{sounds}\n' ))m=3
		else if(match('{widgets}\n'))m=4
		else if(match('{card:')){const k=str('}');cards[k]=lmd(['name','widgets'].map(lms),[lms(k),lml([])]),m=5,lc=0}
		else if(match('{script:')){const k=str('}\n');scripts[k]=str('\n{end}')}
		else if(match('{module:')){const k=str('}');modules[k]=lmd(['name','script','data'].map(lms),[lms(k),lms(''),lmd()]),m=6,md=0}
		else if(match('{contraption:')){const k=str('}');defs[k]=lmd(['name','widgets'].map(lms),[lms(k),lml([])]),m=7,lc=1}
		else if(m==6&&match('{data}\n')){md=1}
		else if(m==6&&match('{script}\n')){dset(last(modules),lms('script'),lms(str('\n{end}'))),m=1}
		else{
			const k=str(':'),j=pjson(x,i,x.length-i),v=j.value;i=j.index
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
	const dscript=x=>{const k=lms('script'),s=dget(x,k);if(s)dset(x,k,lms(scripts[ls(s)]))}
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
			if(ikey(i,'sounds'  ))return dyad.drop(NONE,self.sounds)
			if(ikey(i,'fonts'   ))return dyad.drop(NONE,self.fonts)
			if(ikey(i,'cards'   ))return self.cards
			if(ikey(i,'modules' ))return self.modules
			if(ikey(i,'contraptions'))return self.contraptions
			if(ikey(i,'card'    ))return self.cards.v[min(count(self.cards)-1,self.card)]
			if(ikey(i,'add'     ))return lmnat(([x,y,z])=>deck_add(self,x,y,z))
			if(ikey(i,'remove'  ))return lmnat(([x])=>lmn(deck_remove(self,x)))
			if(ikey(i,'event'   ))return lmnat(args=>n_event(self,args))
			if(ikey(i,'copy' )&&state.external)return lmnat(([x])=>deck_copy(self,x))
			if(ikey(i,'paste')&&state.external)return lmnat(([x])=>deck_paste(self,x))
		}return x?x:NONE
	},'deck')
	ri.fonts       =fonts
	ri.sounds      =sounds
	ri.contraptions=lmd()
	ri.cards       =lmd()
	ri.modules     =lmd()
	ri.transit     =lmd()
	ri.patterns    =patterns_read(deck)
	ri.version     ='version' in deck?ln(deck.version):1
	ri.locked      ='locked'  in deck?lb(deck.locked ):0
	ri.name        ='name'    in deck?ls(deck.name   ):''
	ri.author      ='author'  in deck?ls(deck.author ):''
	ri.script      ='script'  in deck?scripts[ln(deck.script)]:''
	ri.card        ='card'    in deck?clamp(0,ln(deck.card),Object.keys(cards).length-1):0
	ri.size        ='size'    in deck?rclamp(rect(320,240),getpair(deck.size),rect(4096,4096)):rect(512,342)
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
	if(!deck_is(x))return '';let deck=x,scripts=lmd(),si=0,sci=0,r=(html?'<body><script language=\"decker\">\n':'')+'{deck}\nversion:1\n'
	const esc_write=(id,x)=>{
		let c='\0',lc=c,r='';for(let z=0;z<x.length;z++){
			lc=c,c=x[z],r+=c=='{'?'{l}': c=='}'?'{r}': c==':'&&id?'{c}': c=='/'&&lc=='<'?'{s}': c
		}return r
	}
	const script_ref=(base,x)=>{
		if(ls(x)=='undefined')throw new Error('welp')
		for(let z=0;z<scripts.v.length;z++)if(match(scripts.v[z],x))return scripts.k[z]
		const k=lms(base?`${base}.${sci}`:`${sci}`);sci++;dset(scripts,k,x);return k
	}
	const write_scripts=_=>{while(si<scripts.v.length)r+=`\n{script:${esc_write(1,ls(scripts.k[si]))}}\n${esc_write(0,ls(scripts.v[si++]))}\n{end}\n`}
	const write_line=(s,k,p,f)=>{const v=s[k];if(p(v))r+=`${k}:${fjson(f(v))}\n`}
	const write_key =(s,k,p,f)=>{const v=dget(s,lms(k));if(p(v))r+=`${k}:${fjson(f(v))}\n`}
	const write_dict=(k,x,f)=>r+=`${count(x)?k:''}${x.k.map((k,i)=>`${esc_write(1,ls(k))}:${fjson(f(x.v[i]))}\n`).join('')}`
	const pp=patterns_write(x.patterns),pa=anims_write(x.patterns),da=dyad.parse(lms('%j'),lms(DEFAULT_ANIMS))
	write_line(x,'card'      ,x=>1                                  ,lmn                       )
	write_line(x,'size'      ,x=>1                                  ,lmpair                    )
	write_line(x,'locked'    ,x=>x                                  ,lmn                       )
	write_line(x,'script'    ,x=>x.length                           ,x=>script_ref(null,lms(x)))
	write_line(x,'name'      ,x=>x.length                           ,lms                       )
	write_line(x,'author'    ,x=>x.length                           ,lms                       )
	write_line(x,'patterns'  ,x=>pp!=DEFAULT_PATTERNS               ,x=>lms(pp)                )
	write_line(x,'animations',x=>!match(pa,da)                      ,x=>pa                     )
	write_scripts()
	write_dict('\n{fonts}\n',dyad.drop(lml(['body','menu','mono'].map(lms)),deck.fonts),x=>lms(font_write(x)))
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
		write_key(data,'image'      ,x=>x       ,x=>x)
		write_key(data,'script'     ,x=>count(x),x=>script_ref(base,x))
		write_key(data,'template'   ,x=>count(x),x=>x)
		write_key(data,'attributes' ,x=>count(x),x=>x)
		wids.v.map(wid=>{const k=lms('script'),v=dget(wid,k);if(v)dset(wid,k,script_ref(base,v))})
		write_dict('{widgets}\n',wids,x=>x)
		write_scripts()
	})
	return r+'\n'+(html?'<\/script>\nRuntime stub is NYI.':'')
}

n_go=([x,t],deck)=>{
	let r=null, i=deck.card;t=t?ls(t):''
	if(lin(x))r=clamp(0,ln(x),count(deck.cards)-1)
	else if(card_is(x)){const i=dvix(deck.cards,x);if(i>=0)r=i}
	else{
		x=ls(x);if(deck.history.length>1&&x=='Back'){
			deck.history.pop();const ix=last(deck.history);
			if(ix>=0&&ix<count(deck.cards)){go_notify(deck,ix,t,x),deck.card=ix;return lmn(deck.card)}
		}
		else if(x=='First')r=0
		else if(x=='Last' )r=count(deck.cards)-1
		else if(x=='Prev' )r=mod(i-1,count(deck.cards))
		else if(x=='Next' )r=mod(i+1,count(deck.cards))
		else{const ix=dkix(deck.cards,lms(x));if(ix>=0)r=ix}
	}if(r!=null){go_notify(deck,r,t,x),deck.card=r;if(i!=r)deck.history.push(r)}else{go_notify(deck,-1,t,x)}return lmn(deck.card)
}
n_sleep=([z])=>{if(lis(z)&&ls(z)=='play'){sleep_play=1}else{sleep_frames=max(1,ln(z))};return z}
n_transition=(f,deck)=>{const t=deck.transit;if(lion(f))dset(t,lms(f.n),f);return t}

constants=env=>{
	env.local('sys'    ,interface_system)
	env.local('rtext'  ,interface_rtext)
	env.local('pointer',pointer)
	env.local('pi'   ,lmn(3.141592653589793))
	env.local('e'    ,lmn(2.718281828459045))
	env.local('colors',lmd(
		'white|yellow|orange|red|magenta|purple|blue|cyan|green|darkgreen|brown|tan|lightgray|mediumgray|darkgray|black'.split('|').map(lms),
		range(16).map(x=>lmn(x+32))
	))
}
primitives=(env,deck)=>{
	env.local('show'      ,lmnat(n_show    ))
	env.local('print'     ,lmnat(n_print   ))
	env.local('play'      ,lmnat(n_play    ))
	env.local('go'        ,lmnat(([x,t])=>n_go([x,t],deck)))
	env.local('transition',lmnat(([f])=>n_transition(f,deck)))
	env.local('sleep'     ,lmnat(n_sleep   ))
	env.local('eval'      ,lmnat(n_eval    ))
	env.local('random'    ,lmnat(n_random  ))
	env.local('array'     ,lmnat(n_array   ))
	env.local('image'     ,lmnat(n_image   ))
	env.local('sound'     ,lmnat(n_sound   ))
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
	if(in_attr)return NONE;in_attr=1;const bf=frame;
	const root=lmenv();primitives(root,target.deck),constants(root),root.local('me',target)
	const b=lmblk();target.widgets.v.map((v,i)=>{blk_lit(b,v),blk_loc(b,target.widgets.k[i]),blk_op(b,op.DROP)})
	try{blk_cat(b,parse(target.def.script)),blk_op(b,op.DROP)}catch(e){}
	blk_get(b,lms(name)),blk_lit(b,lml(a?[a]:[])),blk_op(b,op.CALL)
	pushstate(root),issue(root,b);let q=ATTR_QUOTA;while(running()&&q>0)runop(),q--;const r=running()?NONE:arg();popstate();frame=bf;return in_attr=0,r
}
parent_deck=x=>deck_is(x)?x: card_is(x)||prototype_is(x)?x.deck: parent_deck(x.card)
event_invoke=(target,name,arg,hunk,isolate)=>{
	const scopes=lmd([NONE],[parse(DEFAULT_HANDLERS)]); let deck=null
	const ancestors=target=>{
		if(deck_is(target)){deck=target;if(isolate)return}
		if(contraption_is(target)){ancestors(target.card)}
		if(card_is(target)||prototype_is(target))ancestors(target.deck)
		if(widget_is(target)&&!contraption_is(target)){
			if(prototype_is(target.card)||contraption_is(target.card))isolate=1
			ancestors(target.card)
		}
		const t=isolate&&contraption_is(target)?ifield(target,'def'):target
		try{dset(scopes,target,parse(ls(ifield(t,'script'))))}
		catch(e){dset(scopes,target,lmblk());}
	};ancestors(target)
	const bind=(b,n,v)=>{blk_lit(b,v),blk_loc(b,n),blk_op(b,op.DROP)}
	const func=(b,n,v)=>{blk_lit(b,lmon(n,[],blk_end(v))),blk_op(b,op.BIND),name=n,arg=lml([])}
	let core=null
	for(let z=scopes.v.length-1;z>=0;z--){
		let t=scopes.k[z], b=lmblk(), sname='!widget_scope'
		if(lin(t))sname='!default_handlers'
		if(deck_is(t)){
			t.modules.v.map((v,i)=>bind(b,t.modules.k[i],ifield(v,'value')))
			t.cards  .v.map((v,i)=>bind(b,t.cards  .k[i],v                ))
			sname='!deck_scope'
		}
		if(card_is(t)||prototype_is(t)||contraption_is(t)){
			bind(b,lms('card'),t)
			t.widgets.v.map((v,i)=>bind(b,t.widgets.k[i],v))
			sname='!card_scope'
		}
		blk_cat(b,scopes.v[z]),blk_op(b,op.DROP)
		if(!core&&hunk){func(b,'!hunk',hunk)}
		else if(core){func(b,sname,core)}
		blk_get(b,lms(name)),blk_lit(b,arg),blk_op(b,op.CALL);if(!hunk)blk_op(b,op.DROP);core=b
	}
	const r=lmblk();bind(r,lms('me'),target)
	if(!isolate)bind(r,lms('deck'),deck),bind(r,lms('patterns'),deck.patterns)
	return blk_cat(r,core),r
}
fire_async=(target,name,arg,hunk,nest)=>{
	const root=lmenv();primitives(root,parent_deck(target)),constants(root)
	if(nest)pushstate(root),pending_popstate=1;issue(root,event_invoke(target,name,arg,hunk,0))
}
fire_event_async=(target,name,x)=>fire_async(target,name,lml([x]),null,1)
fire_hunk_async=(target,hunk)=>fire_async(target,null,lml([]),hunk,1)
n_event=(self,args)=>{fire_async(self,ls(args[0]),lml(args.slice(1)),null,0);return self}
