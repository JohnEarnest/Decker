
// Lilac

q=x=>document.querySelector(x)

let audio=null, samples_playing=0
const context=window.AudioContext||window.webkitAudioContext, MASTER_VOLUME=0.3
initaudio=_=>{if(!audio)audio=new context({sampleRate:44100})}
byte_to_sample=b=>(b<<24>>24)/128
n_play=([x])=>{
	const sfx=!x?x: sound_is(x)?x: dget(deck.sounds,lms(ls(x)));if(!sfx)return NONE;initaudio()
	const playback=audio.createBuffer(1,sfx.data.length*8,64000),dest=playback.getChannelData(0)
	for(let z=0;z<sfx.data.length;z++)for(let b=0;b<8;b++)dest[z*8+b]=MASTER_VOLUME*byte_to_sample(sfx.data[z])
	const playing=audio.createBufferSource();playing.buffer=playback,playing.connect(audio.destination)
	playing.addEventListener('ended',_=>samples_playing--),playing.start(),samples_playing++;return NONE
}
sfx_any=_=>samples_playing>0

n_alert  =([x])=>ONE
n_save   =([x])=>NONE
n_open   =(   )=>lms('')
n_show   =z=>{console.log(z.map(x=>show(x,z.length==1)).join(' '));return z[0]}
n_print  =x=>{console.log(ls(x.length>1?dyad.format(x[0],lml(x.slice(1))):x[0]));return NONE}

let pump=null, zoom=1, deck=deck_read('{card:home}\n{widgets}\ng:{"type":"canvas"}')
let card=deck.cards.v[0], canv=card.widgets.v[0], csize=rect(), env=lmenv()
let dir_up=0, dir_dn=0, dir_lf=0, dir_rt=0, key_action=0, key_cancel=0
let pending_click  =0, arg_click  =rect(-1,-1)
let pending_drag   =0, arg_drag   =rect(-1,-1)
let pending_release=0, arg_release=rect(-1,-1)
let pending_tick   =0

// standard library

go_notify=(deck,x,t)=>{}
constants(env)
env.local('print'   ,lmnat(n_print   ))
env.local('show'    ,lmnat(n_show    ))
env.local('play'    ,lmnat(n_play    ))
env.local('sleep'   ,lmnat(n_sleep   ))
env.local('random'  ,lmnat(n_random  ))
env.local('image'   ,lmnat(n_image   ))
env.local('sound'   ,lmnat(n_sound   ))
env.local('eval'    ,lmnat(n_eval    ))
env.local('writecsv',lmnat(n_writecsv))
env.local('readcsv' ,lmnat(n_readcsv ))
env.local('writexml',lmnat(n_writexml))
env.local('readxml' ,lmnat(n_readxml ))
env.local('canvas'  ,canv)
env.local('sounds'  ,deck.sounds)
env.local('fonts'   ,deck.fonts)
env.local('patterns',deck.patterns)
env.local('pointer' ,pointer)
env.local('images'  ,lmd(Object.keys(RAW_IMAGES).map(lms),Object.values(RAW_IMAGES).map(image_read)))
env.local('app',lmi((self,i,x)=>{
	if(ikey(i,'title')){if(x){document.title=ls(x)}return lms(document.title)}
	return x?x:NONE
},'app'))
env.local('keys',lmi((self,i,x)=>{
	if(ikey(i,'dir'   )){const d=rect(0,0);if(dir_lf)d.x--;if(dir_rt)d.x++;if(dir_up)d.y--;if(dir_dn)d.y++;return lmpair(d)}
	if(ikey(i,'action'))return lmn(key_action)
	if(ikey(i,'cancel'))return lmn(key_cancel)
	return x?x:NONE
},'keys'))
dokeys=(key,state)=>{
	if(key=='ArrowUp'   ||key=='w')dir_up=state
	if(key=='ArrowDown' ||key=='s')dir_dn=state
	if(key=='ArrowLeft' ||key=='a')dir_lf=state
	if(key=='ArrowRight'||key=='d')dir_rt=state
	if(key=='Enter'||key==' '||key=='e'||key=='z')key_action=state
	if(key=='q'||key=='x')key_cancel=state
}

// main loop

tick=_=>{
	const callfunc=(f,args)=>{const b=lmblk();blk_lit(b,f),blk_lit(b,lml(args)),blk_op(b,op.CALL),blk_op(b,op.DROP);return b}
	const runfunc=(name,args)=>{const f=env_get(getev(),lms(name));if(lion(f))issue(env,callfunc(f,args))}
	if(sleep_play&&sfx_any())return;sleep_play=0
	if(sleep_frames){sleep_frames--;return}
	let quota=10*4096;while(1){
		while(running()&&sleep_frames==0&&sleep_play==0&&quota>0)runop(),quota--
		if(!running())arg();if(quota<=0||sleep_frames||sleep_play)break
		if     (pending_click  )pending_click  =0,runfunc('click'  ,[lmpair(arg_click  )])
		else if(pending_drag   )pending_drag   =0,runfunc('drag'   ,[lmpair(arg_drag   )])
		else if(pending_release)pending_release=0,runfunc('release',[lmpair(arg_release)])
		else if(pending_tick   )pending_tick   =0,runfunc('tick'   ,[                   ])
		if(!running())break
	}
}

let id=null
sync=_=>{
	const anim=deck.patterns.anim, pal=deck.patterns.pal.pix
	const anim_ants   =(x,y)=>(0|((x+y+(0|(frame_count/2)))/3))%2?15:0
	const anim_pattern=(pix,x,y)=>pix<28||pix>31?pix: anim[pix-28][(0|(frame_count/4))%max(1,anim[pix-28].length)]
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
	const draw_color  =(pix,x,y)=>pix==ANTS?anim_ants(x,y): pix>47?0: pix>31?pix-32: draw_pattern(pix,x,y)?15:0
	const i=canvas_image(canv,1)
	if(!id||id.width!=i.size.x||id.height!=i.size.y)id=new ImageData(i.size.x,i.size.y)
	for(let z=0,d=0,y=0;y<id.height;y++)for(let x=0;x<id.width;x++,z++,d+=4){
		const pix=i.pix[z], a=anim_pattern(pix,x,y), c=draw_color(a,x,y), cv=COLORS[c]
		id.data[d  ]=0xFF&(cv>>16)
		id.data[d+1]=0xFF&(cv>> 8)
		id.data[d+2]=0xFF&(cv    )
		id.data[d+3]=0xFF
	}
	const r=q('canvas.render');r.getContext('2d').putImageData(id,0,0)
	const g=q('canvas').getContext('2d');g.imageSmoothingEnabled=false,g.save(),g.scale(zoom,zoom),g.drawImage(r,0,0),g.restore()
	pending_tick=1,frame_count++
}

// events

let ev={pos:rect(-1,-1),mu:0,md:0,scroll:0}
getZoom=_=>zoom=max(1,0|min(q('body').clientWidth/csize.x,q('body').clientHeight/csize.y))
move=(x,y)=>{if(!pending_drag)pointer.prev=pointer.pos;pointer.pos=rect(x,y);if(pointer.held)pending_drag=1,arg_drag=pointer.pos}
down=(x,y)=>{move(x,y),pointer.held=1,pending_click  =1,arg_click=pointer.start=pointer.pos}
up  =(x,y)=>{move(x,y),pointer.held=0,pending_release=1,arg_release=pointer.end=pointer.pos}
mouse=(e,f)=>{const c=q('canvas').getBoundingClientRect(); f(0|((e.pageX-c.x)/zoom),0|((e.pageY-c.y)/zoom)); e.preventDefault()}
touch=(e,f)=>{const t=e.targetTouches[0]||{}; mouse({pageX:t.clientX, pageY:t.clientY, preventDefault:_=>e.preventDefault},f)}
loop=_=>{tick(),sync(),pump=setTimeout(loop,1000/60)} // 60fps
window.onresize=resize=_=>{
	csize=getpair(ifield(canv,'lsize'));getZoom()
	const c=q('canvas'       );c.width=csize.x*zoom,c.height=csize.y*zoom
	const r=q('canvas.render');r.width=csize.x     ,r.height=csize.y
	sync()
}
q('body').addEventListener('mousedown'  ,e=>mouse(e,down))
q('body').addEventListener('mouseup'    ,e=>mouse(e,up  ))
q('body').addEventListener('mousemove'  ,e=>mouse(e,move))
q('body').addEventListener('contextmenu',e=>mouse(e,down))
q('body').addEventListener('touchstart' ,e=>touch(e,down))
q('body').addEventListener('touchend'   ,e=>touch({targetTouches:e.changedTouches,preventDefault:e.preventDefault},up))
q('body').addEventListener('touchmove'  ,e=>touch(e,move))
q('body').onwheel=e=>ev.scroll=e.deltaY<0?-1:e.deltaY>0?1:0
q('body').onkeydown=e=>{dokeys(e.key,1),e.preventDefault()}
q('body').onkeyup  =e=>{dokeys(e.key,0),e.preventDefault()}

const canvas_interface=canv.f;canv.f=(self,i,x)=>{
	const r=canvas_interface(self,i,x)
	if(x&&self==canv&&(ls(i)in{size:1,lsize:1,scale:1}))resize()
	return r
}
Object.keys(RAW_SOUNDS).map(k=>dset(deck.sounds,lms(k),sound_read(RAW_SOUNDS[k])))
pushstate(env),issue(env,parse(q('script').innerText))
resize(),(pump&&clearTimeout(pump)),loop()
