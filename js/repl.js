
readBinaryFile=path=>{try{const b=require('fs').readFileSync(path);return array_make(b.length,'u8',0,new Uint8Array(b))}catch(e){return array_make(0,'u8',0)}}
writeBinaryFile=(path,x)=>require('fs').writeFileSync(path,Buffer.from(x.data))
readTextFile=path=>clchars(require('fs').readFileSync(path,{encoding:'utf8'}).replace(/\uFEFF/g, ''))
writeTextFile=(path,text)=>require('fs').writeFileSync(path,text,{encoding:'utf8'})
go_notify=(deck,x,t)=>{/*console.log('go notify',x,t)*/}
field_notify=(field)=>{/*console.log('field notify',x,t)*/}
const env=lmenv()
n_play=([x])=>NIL
n_show=z=>{console.log(z.map(x=>show(x,z.length==1)).join(' '));return z[0]}
n_print=x=>{console.log(ls(x.length>1?dyad.format(x[0],lml(x.slice(1))):x[0]));return NIL}
n_alert  =([x])=>ONE
n_save   =([x])=>NIL
n_open   =(   )=>lms('')
n_panic  =(   )=>NIL
is_fullscreen=_=>0
set_fullscreen=_=>0
run      =prog=>{pushstate(env),issue(env,prog);while(running())runop();const r=arg();return popstate(),r}
env.local('read',lmnat(([x,y])=>
	y&&ls(y)=='array'?readBinaryFile(ls(x)):
	ls(x).toLowerCase().endsWith('.gif')?readgif(readBinaryFile(ls(x)).data,ls(y)):
	ls(x).toLowerCase().endsWith('.deck')?deck_read(readTextFile(ls(x))):
	lms(readTextFile(ls(x)))
))
env.local('newdeck',lmnat(([x])=>deck_read(lis(x)?ls(x):'')))
env.local('write',lmnat(([x,y])=>
	array_is(y)?writeBinaryFile(ls(x),y):
	deck_is(y)?writeTextFile(ls(x),deck_write(y,/\.html$/i.test(ls(x)))):
	writeTextFile(ls(x),ls(y))
))
env.local('exit',lmnat(([x])=>process.exit(ln(x))))
env.local('print',lmnat(n_print))
env.local('show',lmnat(n_show))
env.local('shell',lmnat(([x])=>{
	let o='',e=0;try{o=require('node:child_process').execSync(ls(x))}catch(err){o=err.stdout.toString(),e=err.status}
	return lmd(['exit','out'].map(lms),[lmn(e),lms(o)])
}))
env.local('dir',lmnat(([x])=>{
	const path=x?ls(x):'.', fs=require('fs'), pt=require('path')
	const rd=[],rn=[],rt=[],r=lmt();tab_set(r,'dir',rd),tab_set(r,'name',rn),tab_set(r,'type',rt)
	fs.readdirSync(path).map(name=>{
		rd.push(lmn(fs.lstatSync(`${path}/${name}`).isDirectory()))
		rn.push(lms(name)),rt.push(lms(pt.extname(name)))
	});return r
}))
env.local('random',lmnat(n_random))
env.local('array',lmnat(n_array))
env.local('image',lmnat(n_image))
env.local('sound',lmnat(n_sound))
env.local('eval',lmnat(n_eval))
env.local('writecsv',lmnat(n_writecsv))
env.local('readcsv',lmnat(n_readcsv))
env.local('writexml',lmnat(n_writexml))
env.local('readxml',lmnat(n_readxml))
env.local('args',lml(process.argv.slice(1).map(lms)))
constants(env)
if(process.argv.length>=3){
	try{run(parse(readTextFile(process.argv[2])))}
	catch(e){console.error('x' in e?`(${e.r+1}:${e.c+1}) ${e.x}`:e),process.exit(1)}
	process.exit(0)
}
rl=require('readline').createInterface({input:process.stdin,output:process.stdout,prompt:'  '});rl.prompt()
rl.on('close',_=>(console.log('\n'),process.exit(0)))
rl.on('line',line=>{
	try{if(line.trim()!=0){const x=run(parse(line.trim()));env.local('_',x);console.log(show(x,true))}}
	catch(e){console.error('x' in e?`${' '.repeat(e.c+1)}^\n${e.x}\n`:e)}
	rl.prompt()
})
