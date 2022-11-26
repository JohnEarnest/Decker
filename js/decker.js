// Decker

let pump=null, zoom=1, deck=null, fb=null, context=null, dirty=0
let FONT_BODY=null,FONT_MENU=null,FONT_MONO=null

const ELLIPSIS=String.fromCharCode(95+32)
const DOUBLE_CLICK_DELAY=20
const FIELD_CURSOR_DUTY =20
const FIELD_CHANGE_DELAY=60
const LISTEN_LINES      =30
const LISTEN_SIZE       =_=>rect(frame.size.x-20,100)
const MASTER_VOLUME     =0.3
const FAT               =8
const BG_MASK           =100

q=x=>document.querySelector(x)
gcd=(x,y)=>{while(x!=y){if(x>y){x-=y}else{y-=x}}return x}
lcm=(x,y)=>{const r=gcd(x,y);return 0|((x*y)/(r?r:1))}
copy_object=x=>Object.keys(x).reduce((r,k)=>((r[k]=x[k]),r),{})
plain_or_rich=x=>lit(x)?rtext_cast(x):lms(x?ls(x):'')
fieldstr=x=>({table:rtext_cast(x),scroll:0})
gridtab=(x,r)=>({table:x,scroll:0,row:r==undefined?-1:r})
open_url=x=>{window.open(x,'_blank')}
open_file=(filter,f)=>{const s=q('#source');s.value='',s.accept=filter,s.onchange=_=>{if(s.files.length)f(s.files[0])},s.click()}
open_text=(filter,f)=>open_file(filter,file=>{const r=new FileReader();r.onload=_=>{f(clchars(r.result))},r.readAsText(file)})
load_array=(file,after)=>{const r=new FileReader();r.onload=_=>{const b=new Uint8Array(r.result);after(array_make(b.length,'u8',0,b))},r.readAsArrayBuffer(file)}
load_image=(file,hint,after)=>{
	const read_image=grayscale=>{
		const i=q('#loader'),w=i.width,h=i.height,r=image_make(rect(w,h)); if(w==0||h==0)return r
		const t=document.createElement('canvas');t.width=w,t.height=h
		const tg=t.getContext('2d');tg.drawImage(i,0,0)
		const d=tg.getImageData(0,0,w,h).data
		let src=0,dst=0;while(dst<r.pix.length){
			const cr=d[src++],cg=d[src++],cb=d[src++],ca=d[src++]
			r.pix[dst++]=(ca!=0xFF)?(grayscale?0xFF:0x00): readcolor(cr,cg,cb,grayscale)
		}return r
	}
	const import_image=_=>{
		if(after){after(read_image(hint=='gray'));return}
		let i=read_image(0),m=null; if(i.size.x==0||i.size.y==0)return
		let color=0,c=new Uint8Array(256);i.pix.forEach(p=>c[p]++)
		let tw=c[0],ow=c[32];c[32]=0,c[47]=0;for(let z=2;z<256;z++)if(c[z]){color=1;break}
		if(color&&tw)i.pix.forEach((p,z)=>i.pix[z]=p!=0),m=i
		if(color){i=read_image(1)}else if(ow&&!tw){i.pix.forEach((p,z)=>i.pix[z]=p!=32)}
		setmode('draw'),bg_paste(i);dr.limbo_dither=color,dr.fatbits=0,dr.omask=m
	}
	const r=new FileReader();r.onload=_=>{q('#loader').src=r.result;setTimeout(import_image,100)};r.readAsDataURL(file)
}
save_text=(n,x)=>{
	const u=URL.createObjectURL(new Blob([x])), t=q('#target')
	t.download=n,t.href=u,t.click(),setTimeout(_=>URL.revokeObjectURL(u),200)
}
save_bin=(n,x)=>{
	const u=URL.createObjectURL(new Blob([Uint8Array.from(x)])), t=q('#target')
	t.download=n,t.href=u,t.click(),setTimeout(_=>URL.revokeObjectURL(u),200)
}
writegif=frames=>{
	const size=frames.reduce((s,f)=>rmax(s,f.size),rect(1,1)), pal=deck.patterns.pal.pix; let frame_index=0, payload=[]
	const anim_ants       =(x,y)=>(0|((x+y+frame_index)/3))%2?15:0
	const draw_pattern    =(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
	const draw_color_trans=(pix,x,y)=>pix==ANTS?anim_ants(x,y): pix==0?16: pix>47?0: pix>31?pix-32: draw_pattern(pix,x,y)?15:0
	const b=x=>payload.push(x&0xFF), s=x=>{b(x);b(x>>8)}, t=x=>x.split('').forEach(x=>b(x.charCodeAt(0)))
	t('GIF89a'),s(size.x),s(size.y) // header, dimensions
	b(0xF4)                         // global colortable, 8-bits per channel, 32 colors
	b(0),b(0)                       // background color is 0, 1:1 pixel aspect ratio
	for(let z=0;z<16;z++)b(COLORS[z]>>16),b(COLORS[z]>>8),b(COLORS[z]) // global colortable
	for(let z=0;z<16;z++)b(0xFF         ),b(0xFF        ),b(0xFF     ) // padding entries
	s(0xFF21),b(11),t('NETSCAPE2.0'),b(3),b(1),s(0),b(0)               // NAB; loop gif forever
	for(let z=0;z<frames.length;z++){
		const frame=frames[z]
		s(0xF921),b(4)                            // graphic control extension
		b(9),s(20),b(16)                          // dispose to bg + has transparency, 20/100th of a second delay, color 16 is transparent
		b(0)                                      // end GCE
		b(0x2C)                                   // image descriptor
		s(0),s(0),s(frame.size.x),s(frame.size.y) // dimensions
		b(0),b(7)                                 // no local colortable,  minimum LZW code size
		let isize=frame.size.x*frame.size.y, off=0
		while(off<isize){
			const bsize=min(64,isize-off)
			b(1+bsize),b(0x80) // block size, LZW CLEAR
			for(let z=0;z<bsize;z++){
				const x=0|((off+z)%frame.size.x), y=0|((off+z)/frame.size.x)
				b(draw_color_trans(frame.pix[off+z],x,y))
			}off+=bsize
		}b(0),frame_index++ // end of frame
	};b(0x3B);return payload
}
writewav=sound=>{
	const payload=[], b=x=>payload.push(x&0xFF), s=x=>(b(x),b(x>>8))
	const d=x=>(b(x),b(x>>8),b(x>>16),b(x>>24)), t=x=>x.split('').forEach(x=>b(x.charCodeAt(0)))
	t('RIFF'),d(4+24+(8+sound.data.length)+(sound.data.length%2))
	t('WAVE'),t('fmt ')
	d(16)             // chunk size
	s(1),s(1),d(8000) // pcm, 1 channel, 8khz
	d(8000)           // 8000*(1 byte per sample)*(1 channel)
	s(1)              //      (1 byte per sample)*(1 channel)
	s(8)              // 8 bits per sample
	t('data'),d(sound.data.length)
	for(let z=0;z<sound.data.length;z++)b(128+sound.data[z])
	if(sound.data.length%2)b(0);return payload
}
writearray=array=>{
	const payload=[]
	for(let z=0;z<array.size;z++){const ix=array.base+z;payload.push(ix>=0&&ix<array.data.length?array.data[ix]:0)}
	return payload
}

keep_ratio=(r,s)=>{if(!ev.shift||s.x==0||s.y==0)return r;const scale=max(r.w/(s.x*1.0),r.h/(s.y*1.0));return rect(r.x,r.y,scale*s.x,scale*s.y)}
draw_frame=(image,clip)=>({brush:0,pattern:0,font:FONT_BODY,size:image.size,clip:clip||rect(0,0,image.size.x,image.size.y),image})
draw_pix=(x,y,pattern)=>{const h=rect(x,y);if(inclip(h))pix(h,pattern)}
draw_invert=(pal,r)=>{
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1; r=rclip(r,frame.clip)
	for(let a=r.y;a<r.y+r.h;a++)for(let b=r.x;b<r.x+r.w;b++){const h=rect(b,a), p=draw_pattern(gpix(h),b,a);pix(h,p==0||p==32?1:32)}
}
draw_shadow=(r,fcol,bcol,solid)=>{
	if(solid)draw_rect(rclip(r,frame.clip),bcol);draw_box(r,0,fcol),draw_hline(r.x+3,r.x+r.w,r.y+r.h,fcol),draw_vline(r.x+r.w,r.y+2,r.y+r.h+1,fcol)
}
draw_boxr=(r,fcol,bcol,background)=>{
	r=rint(r)
	draw_hline(r.x+2,r.x+r.w-2,r.y,fcol),draw_hline(r.x+2,r.x+r.w-2,r.y+r.h-1,fcol),draw_vline(r.x,r.y+2,r.y+r.h-2,fcol),draw_vline(r.x+r.w-1,r.y+2,r.y+r.h-2,fcol)
	draw_pix(r.x+1,r.y+1,fcol),draw_pix(r.x+r.w-2,r.y+1,fcol),draw_pix(r.x+1,r.y+r.h-2,fcol),draw_pix(r.x+r.w-2,r.y+r.h-2,fcol)
	if(background)draw_hline(r.x+2,r.x+r.w-2,r.y+1,bcol),draw_hline(r.x+2,r.x+r.w-2,r.y+r.h-2,bcol),draw_rect(rect(r.x+1,r.y+2,r.w-2,r.h-4),bcol)
}
draw_boxinv=(pal,r)=>{
	draw_invert(pal,rect(r.x,r.y,1,r.h))    ,draw_invert(pal,rect(r.x+r.w-1,r.y,1,r.h))
	draw_invert(pal,rect(r.x+1,r.y,r.w-2,1)),draw_invert(pal,rect(r.x+1,r.y+r.h-1,r.w-2,1))
}
draw_modalbox=s=>{
	const menu=16, r=rcenter(rect(0,menu,frame.size.x,frame.size.y-menu),s), o=inset(r,-5)
	draw_rect(inset(o,-5),32),draw_box(inset(o,-5),0,1),draw_box(inset(o,-2),0,1),draw_box(inset(o,-1),0,1);return r
}
draw_modal_rtext=extra=>{
	const size=rect(200,100)
	const l=lit(ms.message)?layout_richtext(deck,ms.message,FONT_BODY,ALIGN.center,size.x):layout_plaintext(ls(ms.message),FONT_BODY,ALIGN.center,size)
	const b=draw_modalbox(radd(l.size,extra)), tbox=rect(b.x,b.y,b.w,l.size.y)
	if(lit(ms.message)){draw_text_rich(tbox,l,1,1)}else{draw_text_wrap(tbox,l,1)}
	return b
}
draw_text_outlined=(pos,text,f)=>{
	([[-1,-1],[-1,0],[-1,1],[0,-1],[0,1],[1,-1],[1,0],[1,1]]).map(([x,y])=>draw_text(rect(pos.x+x,pos.y+y),text,f,32))
	draw_text(pos,text,f,1)
}
draw_textc=(r,text,font,pattern)=>{
	const size=font_textsize(font,text);if(size.x<r.w){draw_text(rcenter(r,size),text,font,pattern)}else{draw_text_fit(r,text,font,pattern)}
}
draw_text_fit=(r,text,font,pattern)=>{
	const glyphs=[], glyph_push=(pos,c)=>glyphs.push({pos,c})
	let x=0,y=0,fh=font_h(font),ew=font_gw(font,ELLIPSIS)
	for(let z=0;z<text.length&&(y+fh)<=r.h;z++){
		const c=text[z]
		if(c=='\n'){x=0,y+=fh}
		else if(x+font_gw(font,c)>=(r.w-ew)){glyph_push(rect(x,y),ELLIPSIS);while(z<text.length&&text[z]!='\n')z++;x=0;if(z<text.length){y+=fh}else{z--}}
		else{glyph_push(rect(x,y),c),x+=font_gw(font,c)+font_sw(font)}
	}
	let yo=ceil((r.h-(y+fh))/2.0);glyphs.map(g=>{g.pos.x+=r.x,g.pos.y+=yo+r.y,draw_char(g.pos,font,g.c,pattern)})
}
draw_scaled=(r,image,opaque)=>{
	if(r.w==0||r.h==0)return;const s=image.size
	if(r.w==s.x&&r.h==s.y){image_paste(r,frame.clip,image,frame.image,opaque);return}
	image_paste_scaled(r,frame.clip,image,frame.image,opaque)
}
draw_invert_scaled=(pal,r,image)=>{
	if(r.w==0||r.h==0)return;const s=image.size, fb=frame.image.pix, fs=frame.image.size.x
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal[(x%8)+(8*(y%8))+(8*8*pix)]&1
	const gpix=p=>fb[p.x+p.y*fs], pix=(p,v)=>fb[p.x+p.y*fs]=v
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		const sx=0|(s.x==r.w?b:((b*1.0)/r.w)*s.x), sy=0|(s.y==r.h?a:((a*1.0)/r.h)*s.y), dx=r.x+b, dy=r.y+a
		const c=draw_pattern(image.pix[sx+sy*s.x],dx,dy), h=rect(dx,dy)
		if(inclip(h))pix(h,c^draw_pattern(gpix(h),dx,dy))
	}
}
draw_fat=(src,pal,frame_count,mask,scale,offset)=>{
	const anim=deck.patterns.anim
	const anim_pattern=(pix,x,y)=>pix<28||pix>31?pix: anim[pix-28][(0|(frame_count/4))%max(1,anim[pix-28].length)]
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal[(x%8)+(8*(y%8))+(8*8*pix)]&1
	const s=src.size;for(let y=0;y<ceil(frame.size.y/scale);y++)for(let x=0;x<ceil(frame.size.x/scale);x++){
		if(offset.x+x>=s.x||offset.y+y>=s.y||offset.x+x<0||offset.y+y<0)continue
		const v=src.pix[(offset.x+x)+(offset.y+y)*frame.size.x];if(v==mask)continue
		const c=anim_pattern(v,offset.x+x,offset.y+y),p=draw_pattern(c,offset.x+x,offset.y+y)
		draw_rect(rect(x*scale,y*scale,scale,scale),c>=32?c: c==0?0: p?1:32)
	}
}
draw_fat_scaled=(r,image,opaque,pal,frame_count,scale,offset)=>{
	const anim=deck.patterns.anim
	const anim_pattern=(pix,x,y)=>pix<28||pix>31?pix: anim[pix-28][(0|(frame_count/4))%max(1,anim[pix-28].length)]
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal[(x%8)+(8*(y%8))+(8*8*pix)]&1
	if(r.w==0||r.h==0)return;const s=image.size
	for(let y=0;y<r.h;y++)for(let x=0;x<r.w;x++){
		const sx=0|(s.x==r.w?x:((x*1.0)/r.w)*s.x), sy=0|(s.y==r.h?y:((y*1.0)/r.h)*s.y), v=image.pix[sx+sy*s.x]
		const c=anim_pattern(v,r.x+x,r.y+y),p=draw_pattern(c,r.x+x,r.y+y)
		if(opaque||v!=0)draw_rect(rect((r.x-offset.x+x)*scale,(r.y-offset.y+y)*scale,scale,scale),c>=32?c: p?1:0)
	}
}
draw_dithered=(r,image,opaque,mask)=>{
	if(r.w==0||r.h==0)return;const s=image.size, stride=2*r.w, m=[0,1,r.w-2,r.w-1,r.w,stride-1], dither_err=new Float32Array(stride)
	for(let ei=0,a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		const sx=0|(((b*1.0)/r.w)*s.x), sy=0|(((a*1.0)/r.h)*s.y), src=0xFF&image.pix[sx+sy*s.x], ms=mask?mask.pix[sx+sy*s.x]:1
		const p=(src/256.0)+dither_err[ei], col=p>.5?1:0, err=(p-col)/8.0
		dither_err[ei]=0, ei=(ei+1)%stride; for(let z=0;z<6;z++)dither_err[(ei+m[z])%stride]+=err
		const c=!col;if(ms&&(opaque||c!=0)){const h=rect(r.x+b,r.y+a);if(inclip(h))pix(h,c)}
	}
}
draw_card=(card,active)=>{
	const im=ms.in_modal;ms.in_modal=active
	const rsize=getpair(ifield(card,'size')),r=image_make(rsize),t=frame,te=copy_object(ev);frame=draw_frame(r),ev=event_state()
	const back=ifield(card,'image'), bsize=back.size, wids=ifield(card,'widgets')
	if(bsize.x!=0&&bsize.y!=0)image_paste(rpair(rect(),bsize),frame.clip,back,frame.image,1)
	if(uimode!='draw'||dr.show_widgets)wids.v.map(w=>{
		if(button_is(w))widget_button(w,unpack_button(w),lb(ifield(w,'value')))
		if(slider_is(w))widget_slider(w,unpack_slider(w))
		if(canvas_is(w))widget_canvas(w,unpack_canvas(w),canvas_image(w,0))
		if(grid_is  (w))widget_grid  (w,unpack_grid  (w),unpack_grid_value(w))
		if(field_is (w))widget_field (w,unpack_field (w),unpack_field_value(w))
	});return ev=te,frame=t,ms.in_modal=im,r
}
draw_thumbnail=(card,r)=>{
	const back=ifield(card,'image');r=inset(r,1),draw_rect(r,0);if(back.size.x>0||back.size.y>0)draw_scaled(r,back,1)
	const wids=ifield(card,'widgets'),s=getpair(ifield(card,'size')), xr=r.w*(1.0/s.x), yr=r.h*(1.0/s.y)
	wids.v.map(wid=>{const w=unpack_widget(wid);draw_box(rect(r.x+w.size.x*xr,r.y+w.size.y*yr,w.size.w*xr,w.size.h*yr),0,w.show=='invert'?0:1)})
}
draw_lil=(size,align,bare,x)=>{
	const GAP=50, w=size.x-GAP; let xo=align==ALIGN.right?GAP: align==ALIGN.left?0: 0|(GAP/2)
	const r=image_make(size), t=frame; frame=draw_frame(r)
	if(lit(x)){
		const tk=Object.keys(x.v), tc=tk.length, tr=tk.length?x.v[tk[0]].length:0
		const hh=tc?3+font_h(FONT_BODY):3, ch=font_h(FONT_MONO), fh=font_h(FONT_BODY), rows=0|min((size.y-(hh+fh))/ch,tr)
		const f=dyad.take(NONE,x), cw=range(256).map(x=>0)
		for(let c=0;c<tc&&c<256;c++){
			const dr=rows<tr?rows-1:rows;cw[c]=0;for(let r=0;r<dr;r++){
				const s=show(x.v[tk[c]][r]);f.v[tk[c]].push(lms(s))
				cw[c]=max(cw[c],font_textsize(FONT_MONO,s).x+10)
			}if(rows<tr)f.v[tk[c]].push(lms(' \x7f'))
			cw[c]=min(100,max(cw[c],font_textsize(FONT_BODY,tk[c]).x+10))
		}
		let cols=0,tw=0,ve=0;for(let c=0;c<tc&&c<256;c++){if(tw+cw[c]>=w){ve=1;break};cols++;if(c+1<=tc&&c+1<=256)tw+=cw[c]}
		xo=align==ALIGN.right?size.x-tw: align==ALIGN.left?0: 0|((size.x-tw)/2)
		let cx=xo;for(let c=0;c<cols;c++){
			if(c)draw_vline(cx,0,hh+ch*rows+(rows==0),1);draw_text_fit(rect(cx+2,0,cw[c]-4,hh),tk[c],FONT_BODY,1)
			for(let r=0;r<rows;r++)draw_text_fit(rect(cx+2,hh+ch*r,cw[c]-4,ch),ls(f.v[tk[c]][r]),FONT_MONO,1)
			if(c+1<=cols)cx+=cw[c]
		}draw_hline(xo,cx,hh-1,1);const bh=hh+ch*rows+1+(rows==0);if(cx==xo)cx+=min(25,w)
		const desc=`(${tc} column${tc==1?'':'s'}, ${tr} row${tr==1?'':'s'}.)`
		const ds=font_textsize(FONT_BODY,desc);draw_text_fit(rect(xo+tw-ds.x,bh,w,fh),desc,FONT_BODY,1)
		draw_box(rect(xo,0,cx-xo,bh),0,1);if(ve)draw_vline(cx-1,0,bh,13)
	}
	else if(image_is(x)){
		const s=x.size, desc=`(${s.x} x ${s.y})`, ds=font_textsize(FONT_BODY,desc), mh=max(1,min(s.y,size.y-font_h(FONT_BODY))), mw=max(1,min(s.x,w))
		const scale=s.x==0&&s.y==0?1:min(mw/(s.x*1.0),mh/(s.y*1.0)), iw=0|(scale*s.x), ih=0|(scale*s.y)
		const b=rect(xo+(w-max(iw,ds.x)),0,iw,ih); draw_scaled(b,x,1),draw_box(b,0,1),draw_text(rect(b.x,b.y+b.h,ds.x,ds.y),desc,FONT_BODY,1)
	}
	else{const l=layout_plaintext(bare?ls(x): show(x,0),FONT_MONO,align,rect(w,size.y));draw_text_wrap(rect(xo,0,w,size.y),l,1)}
	for(let y=size.y-1;y>0;y--){let f=0;for(let x=0;x<size.x;x++)if(r.pix[x+(y*size.x)]){f=1;break}if(f){break}else{r.size.y--}}
	return frame=t,r
}
unpack_widget=x=>({
	size  :rpair(getpair(ifield(x,'pos')),getpair(ifield(x,'size'))),
	show  :ls(ifield(x,'show'  )),
	locked:lb(ifield(x,'locked')),
})
unpack_button=x=>({
	size  :rpair(getpair(ifield(x,'pos')),getpair(ifield(x,'size'))),
	text  :ls(ifield(x,'text'  )),
	font  :   ifield(x,'font'  ) ,
	style :ls(ifield(x,'style' )),
	show  :ls(ifield(x,'show'  )),
	locked:lb(ifield(x,'locked')),
})
unpack_slider=x=>({
	size  :rpair(getpair(ifield(x,'pos')),getpair(ifield(x,'size'))),
	font  :   ifield(x,'font'  ) ,
	format:ls(ifield(x,'format')),
	show  :ls(ifield(x,'show'  )),
	style :ls(ifield(x,'style' )),
	locked:lb(ifield(x,'locked')),
	step  :ln(ifield(x,'step'  )),
	value :ln(ifield(x,'value' )),
	min   :getpair(ifield(x,'interval')).x,
	max   :getpair(ifield(x,'interval')).y,
})
unpack_canvas=x=>({
	size     :rpair(getpair(ifield(x,'pos')),getpair(ifield(x,'size'))),
	scale    :ln(ifield(x,'scale'    )),
	border   :lb(ifield(x,'border'   )),
	draggable:lb(ifield(x,'draggable')),
	show     :ls(ifield(x,'show'     )),
	locked   :lb(ifield(x,'locked'   )),
})
unpack_field=x=>({
	size     :rpair(getpair(ifield(x,'pos')),getpair(ifield(x,'size'))),
	font     :   ifield(x,'font'     ) ,
	show     :ls(ifield(x,'show'     )),
	scrollbar:lb(ifield(x,'scrollbar')),
	border   :lb(ifield(x,'border'   )),
	style    :ls(ifield(x,'style'    )),
	locked   :lb(ifield(x,'locked'   )),
	align    :ALIGN[ls(ifield(x,'align'))],
})
unpack_field_value=x=>({
	table :   ifield(x,'value' ) ,
	scroll:ln(ifield(x,'scroll')),
})
unpack_grid=x=>({
	size     :rpair(getpair(ifield(x,'pos')),getpair(ifield(x,'size'))),
	widths   :ll(ifield(x,'widths')).map(ln),
	font     :   ifield(x,'font'     ) ,
	format   :ls(ifield(x,'format'   )),
	headers  :lb(ifield(x,'headers'  )),
	scrollbar:lb(ifield(x,'scrollbar')),
	lines    :lb(ifield(x,'lines'    )),
	show     :ls(ifield(x,'show'     )),
	locked   :lb(ifield(x,'locked'   )),
})
unpack_grid_value=x=>({
	table :   ifield(x,'value' ) ,
	scroll:ln(ifield(x,'scroll')),
	row   :ln(ifield(x,'row'   )),
})

let cursor={default:'default',point:'pointer',ibeam:'text',drag:'grabbing'}
image_tiles=(count,size,image)=>range(count).map(x=>image_copy(image,rect(0,x*size,size,size)))
TOOLS=image_tiles(12,16,image_read(
	'%%IMG0ABAAwAMABIAEgASABIAEgGTwlKxMqiQKJAIQAggCCAQEBAQEAAAAAAAAAAA//EACgAGAAYABgAFAAz/+H/wAAAAAAA'+
	'AAAAAA8+eAAYABAAAAAIABgAGAAQAAAACAAYAB588AAADwAIgBCAGQAnACIAQgBEAIQAiAEIARAB4AHAAYABAAAAAAAAP4HA'+
	'YgAUABgAGAAoAcuOBnAFQAOAAIAAgAEAAAAAAAAADAADAADAADAADAADAADAADAAAAAAAAAAAAAAcACIAJgArADLAInBCOIU'+
	'dAh4APgBdAJyBHEIcJBgYEAAAAADgcVjar69VVqqvVVaqrVVZqrDVYGrAHwAAAAAAAAAAA//+AAYABgAGAAYABgAGAAYABgA'+
	'H//wAAAAAAAAAAAAD//6qr1VWqq9VVqqvVVaqr1VWqq///AAAAAAAAAAAAAAAAB+AYGCAEQAKAAYABgAFAAiAEGBgH4AAAAA'+
	'AAAAAAAAAH4B1YKqxVVqqr1VWqq1VWKqwdWAfgAAAAAA=='
))
ARROWS=image_tiles(8,12,image_read(
	'%%IMG0AAwAYAAABgAJARCCIERAL/DxEIEQgRCBH4AAAAABH4EQgRCBEI/w9EAiIEEQgAkABgAAAAAABgAPAR+DP8d/7//xH4'+
	'EfgR+BH4AAAAABH4EfgR+BH4//93/jP8EfgA8ABgAAAAIABgAKARPiICRAJEAiICET4AoABgACAAQABgAFB3yEQERAJEAkQE'+
	'd8gAUABgAEAAIABgAOAR/jP+d/53/jP+Ef4A4ABgACAAQABgAHB3+Hf8d/53/nf8d/gAcABgAEAA=='
))
CHECK=image_read('%%IMG0AAkABwCAAcGDY8Y2bBw4CBAA')
CHECKS=[
	'%%IMG0AA8ADQAAf/BAEEAQQBBAEEAQQBBAEEAQQBB/8AAA',
	'%%IMG0AA8ADQAAf/BgMFBQSJBFEEIQRRBIkFBQYDB/8AAA',
	'%%IMG0AA8ADQAAVVAAAEAQAABAEAAAQBAAAEAQAABVUAAA',
	'%%IMG0AA8ADQAAVVAgIFBQCIBFEAIARRAIgFBQICBVUAAA',
].map(image_read)
LOCK     =image_read('%%IMG0AAgACDhERP7+/v4A')
ZOOM     =image_read('%%IMG0AAwADB4AIQBMgIxAv0C/QIxATIAhwB7gAHAAMA==')
CORNERS=['%%IMG0AAUABf/mxISA','%%IMG0AAUABfk4GAgI','%%IMG0AAUABYSGx+f4','%%IMG0AAUABQgIGT/4'].map(image_read)
GESTURES={
	left: image_read('%%IMG2ABAAEAAaIAQACiADAQIgAQAIIAMBBCABAAYgAwEGIAEABCADAQggBgEMIAIBDiAEAQwgAQACIAMBCCADAAQgAwEGIAEACCADAQQgAQAKIAMBAiABAAwgBAAi'),
	right:image_read('%%IMG2ABAAEAASIAQADCABAQIgAwAKIAEBBCADAAggAQEGIAMABCADAQggAwACIAEBDCAEAQ4gAgEMIAYBCCADAAQgAQEGIAMABiABAQQgAwAIIAEBAiADAAogBAAq'),
	up:   image_read('%%IMG2ABAAEAAHIAMADSABAQEgAQAMIAIBASACAAsgAQEDIAEACiACAQMgAgAJIAEBBSABAAggAgEFIAIAByABAQcgAQAGIAIBByACAAUgAQEJIAEABCACAQkgAgADIAEBCyABAAMgAQELIAEAAyAFAQMgBQAHIAEBAyABAAsgBQAF'),
	down: image_read('%%IMG2ABAAEAAFIAUACyABAQMgAQAHIAUBAyAFAAMgAQELIAEAAyABAQsgAQADIAIBCSACAAQgAQEJIAEABSACAQcgAgAGIAEBByABAAcgAgEFIAIACCABAQUgAQAJIAIBAyACAAogAQEDIAEACyACAQEgAgAMIAEBASABAA0gAwAH'),
}
RADIOS=[
	'%%IMG0ABAADgAAB4AYYCAQIBBACEAIQAhACCAQIBAYYAeAAAA=',
	'%%IMG0ABAADgAAB4Af4DhwMDBgGGAYYBhgGDAwOHAf4AeAAAA=',
	'%%IMG0ABAADgAAAAAAAAAAB4APwA/AD8APwAeAAAAAAAAAAAA=',
	'%%IMG0ABAADgAAB4Af4D/wP/B/+H/4f/h/+D/wP/Af4AeAAAA=',
].map(image_read)
ICONS=[
	'%%IMG0AAwADAAAAAM8BEPkQBRAFEAUQBRAFEAXf/AAAA==',
	'%%IMG0AAwADAABHwERgRFBEeEQIRAhECEQIRAhH+AAAA==',
	'%%IMG0AAwADAAAAgAGQA4jPqM+oz6gDiAGQAIAAAAAAA==',
	'%%IMG0AAwADAABH4EfwAHBH8M/wznDOcM5wz/hH+AAAA==',
	'%%IMG0AAwADAAAAgAFAAiBEEIjpERSLiEScAnwBWACAA==',
	'%%IMG0AAwADAAAAAEf4RShF6EUoRShF6EQoR/gAAAAAA==',
	'%%IMG0AAwADAABFUIqoRVCKqEVQiqhFUIqoRVCKqAAAA==',
	'%%IMG0AAwADAAAAAAAIABgAMRBhmMDNgEcAAgAAAAAAA==',
].map(image_read)
ICON={dir:0,doc:1,sound:2,font:3,app:4,lil:5,pat:6,chek:7,none:8}
PANGRAM='How razorback jumping-frogs can level six piqued gymnasts.'

// State

let uimode='interact', uicursor=0, enable_gestures=0, profiler=0
setmode=mode=>{
	n_play([NONE,lms('loop')])
	grid_exit(),field_exit(),bg_end_selection(),bg_end_lasso(),ob.sel=[],wid.active=-1,sc.others=[],dr.poly=[]
	msg.next_view   =(uimode!=mode)&&mode=='interact'
	msg.pending_loop=(uimode!=mode)&&mode=='interact'
	uimode=mode;if(mode!='interact')msg.pending_halt=1;if(mode!='draw')dr.fatbits=0
}

event_state=_=>({ // event state
	mu:0,md:0, clicktime:0,click:0,rdown:0,rup:0, dclick:0, clicklast:0,
	drag:0, tab:0, shift:0, alt:0, action:0, dir:0, exit:0, eval:0, scroll:0, hidemenu:0,
	pos:rect(), dpos:rect(), rawpos:rect(), rawdpos:rect(), shortcuts:{},
	callback:null, callback_rect:null, callback_drag:0
})
let ev=event_state()
over=r=>rin(r,ev.pos)
dover=r=>rin(r,ev.dpos)

wid_state=_=>({ // widget state
	active:0,count:0, scrolls:0,thumbid:0,thumbo:0, col_drag:0,col_num:0,col_orig:0,
	ingrid:0,g:null,gt:null,gv:null,pending_grid_edit:0,
	infield:0,f:null,ft:null,fv:null,field_dirty:0,change_timer:0,
	cursor:rect(),cursor_timer:0,
	hist:[],hist_cursor:0,
})
let wid=wid_state()
let ms={ // modal state
	type:null,subtype:null,in_modal:0,old_wid:null,
	filter:0, grid:null,grid2:null, text:null,name:null,form0:null,form1:null,form2:null,
	desc:'',path:'',filter:'', message:null,verb:null, cell:rect(),
	from_listener:0,from_action:0, act_go:0,act_card:0,act_gomode:0,act_trans:0,act_transo:0,act_sound:0,
	time_curr:0,time_end:0,time_start:0, carda:null,cardb:null,trans:null,canvas:null,
}
let msg={ // interpreter event messages
	pending_drag:0,pending_halt:0,pending_view:0,pending_loop:0,next_view:0,overshoot:0,
	target_click:null,target_drag:null,target_release:null,target_order:null,target_run:null,target_link:null,target_change:null,target_navigate:null,
	arg_click:rect(),arg_drag:rect(),lastdrag:rect(),arg_release:rect(),arg_order:null,arg_run:null,arg_link:null,arg_change:null,arg_navigate:null,
}
let li={hist:[],vars:{},scroll:0} // listener state
let ob={sel:[],show_bounds:1,show_names:0,show_cursor:0,move:0,move_first:0,resize:0,resize_first:0,handle:-1,prev:rect(),orig:rect()} // object editor state
let sc={target:null,others:[],next:null, f:null,prev_mode:null,xray:0,status:''} // script editor state
script_save=x=>{const k=lms('script');mark_dirty();if(sc.target)iwrite(sc.target,k,x);if(sc.others)sc.others.map(o=>iwrite(o,k,x))}

draw_state=_=>({ // drawing tools state
	tool:'pencil',brush:3,pattern:1,fill:0,erasing:0,
	show_widgets:1,show_anim:1,trans:0,trans_mask:0,fatbits:0,offset:rect(),
	show_grid:0,grid_size:rect(32,32), sel_here:rect(),sel_start:rect(),limbo:null,limbo_dither:0,
	scratch:null,mask:null,omask:null, pickfill:0, poly:[]
})
let dr=draw_state()
settool=tool=>{setmode('draw'),dr.tool=tool}
bg_pat=_=>(dr.trans_mask&&dr.pattern==0)?32:dr.pattern
bg_fill=_=>(dr.trans_mask&&dr.fill==0)?32:dr.fill
bg_has_sel=_=>dr.tool=='select'&&(dr.sel_here.w>0||dr.sel_here.h>0)
bg_has_lasso=_=>dr.tool=='lasso'&&dr.mask!=null
fat_offset=p=>{const d=rect(frame.size.x/FAT,frame.size.y/FAT);dr.offset=rint(rect(p.x-d.x/2,p.y-d.y/2))}
fat_clip=_=>dr.fatbits?rint(rect(dr.offset.x,dr.offset.y,frame.size.x/FAT,frame.size.y/FAT)): frame.clip
fat_to_card=a=>rint(rect((a.x/FAT)+dr.offset.x,(a.y/FAT)+dr.offset.y))
card_to_fat=a=>dr.fatbits?rect((a.x-dr.offset.x)*FAT,(a.y-dr.offset.y)*FAT,a.w*FAT,a.h*FAT):a
mark_dirty=_=>{dirty=1}

let au={target:null,mode:'stopped', head:0,sel:rect(), hist:[],hist_cursor:0, clip:null,tick:null, record_stream:null,norecord:0} // audio editor state
byte_to_sample=b=>(b<<24>>24)/128
sample_to_byte=s=>0xFF&clamp(-127,s*128,127)

// Menus

const menu={heads:[],items:[],x:0,active:-1,stick:-1,lw:-1,sz:rect()}
no_menu=_=>menu.active==-1&&menu.stick==-1
in_layer=_=>no_menu()&&(ms.type?ms.in_modal:1)&&((!running()&&!msg.overshoot)||ms.type!=null)
in_widgets=_=>ms.type!=null?ms.in_modal:1
menus_off=_=>lb(ifield(deck,'locked'))||(uimode=='draw'&&ev.hidemenu&&ms.type==null)
menu_head=(name,enabled,t,b)=>({name,enabled,t,b}) // string,bool,rect,rect
menu_entry=(name,enabled,check,shortcut,t,b)=>({name,enabled,check,shortcut,t,b}) // string,bool,bool,char,rect,rect
menus_clear=_=>(menu.active=-1,menu.stick=-1)
menu_setup=_=>(menu.x=10,menu.heads=[],menu.sz=rect(),menu.active=-1)
menu_bar=(name,enabled)=>{
	if(menus_off())enabled=0
	const t=rpair(rect(menu.x,2),font_textsize(FONT_MENU,name)), b=rect(t.x-5,0,t.w+10,t.h+3), i=menu.heads.length
	menu.heads.push(menu_head(name,enabled,t,b)), menu.x=b.x+b.w+5
	if(ev.click&&enabled&&over(b)){ev.mu=0;if(menu.stick==-1)menu.stick=i}
	if(menu.stick!=-1&&enabled&&over(b))menu.stick=i,menu.lw=0
	if(menu.stick==-1){
		if(ev.drag&&enabled&&over(b)&&ev.dpos.y<b.h)ev.dpos=ev.pos,menu.lw=0
		if((ev.drag||ev.mu)&&enabled&&rin(b,ev.dpos))menu.active=i
	}if(i==menu.active||i==menu.stick)menu.sz=rect(b.x,b.h,max(b.w,menu.lw),0),menu.items=[]
	if(ev.md&&over(b))ev.md=0
}
shortcut_w=c=>!c?0: 10+font_textsize(FONT_MENU,'^'+c).x
menu_check=(name,enabled,check,shortcut,func)=>{
	const sc=enabled&&shortcut&&ev.shortcuts[shortcut]; if(sc)delete ev.shortcuts[shortcut];
	if(menu.heads.length-1!=menu.active&&menu.heads.length-1!=menu.stick)return sc
	const t=name?rpair(rect(menu.sz.x+5+8,menu.sz.y+menu.sz.h+2),font_textsize(FONT_MENU,name)): rect(menu.sz.x,menu.sz.y+menu.sz.h+2,1,1)
	if(shortcut)t.w+=shortcut_w(shortcut)
	const b=rect(menu.sz.x,menu.sz.y+menu.sz.h,max(menu.sz.w,t.w+10+8),t.h+4)
	menu.items.push(menu_entry(name,enabled,check,shortcut,t,b)), menu.sz=runion(menu.sz,b)
	if(enabled&&over(b)&&func)ev.callback=func,ev.callback_rect=rcopy(b),ev.callback_drag=1
	return sc||(enabled&&ev.mu&&over(b))
}
menu_item=(name,enabled,shortcut,func)=>menu_check(name,enabled,-1,shortcut,func)
menu_separator=_=>menu_check(0,0,0,0,0)
menu_finish=_=>{
	if(menus_off())return
	const b=rect(0,0,context.size.x,3+font_h(FONT_MENU)); draw_rect(b,32),draw_hline(0,b.w,b.h,1); const pal=deck.patterns.pal.pix
	menu.heads.map((x,i)=>{
		let a=x.enabled&&(over(x.b)||i==menu.stick||i==menu.active);if(ev.drag&&!dover(b))a=0
		draw_text(x.t,x.name,FONT_MENU,x.enabled?1:13);if(a)draw_invert(pal,x.b)
	})
	if(!menu.sz.w)return
	draw_shadow(menu.sz,1,32,1);menu.lw=0;let sw=0;menu.items.map(x=>{menu.lw=max(menu.lw,x.b.w),sw=max(sw,shortcut_w(x.shortcut))})
	menu.items.map(x=>{
		const o=over(x.b)&&x.name&&x.enabled
		if(x.name){draw_text(x.t,x.name,FONT_MENU,x.enabled?1:13)}else{draw_rect(rect(x.t.x+2,x.t.y,menu.sz.w-5,1),19)}
		if(x.check==1)draw_icon(rect(menu.sz.x+2,x.t.y+3),CHECK,x.enabled?1:13)
		if(x.shortcut)draw_text(rect(menu.sz.x+menu.sz.w-3-sw+10,x.t.y),'^'+x.shortcut,FONT_MENU,x.enabled?1:13)
		if(o)draw_invert(pal,inset(x.b,1))
	});if(ev.mu)menu.stick=-1
}

// Widgets

widget_setup=_=>{
	if(ev.mu||wid.active==-1)wid.col_drag=0;if(wid.active>=wid.count)wid.active=0
	if((uimode=='interact'||ms.type!=null)&&ev.tab&&wid.count&&!(wid.infield&&wid.f.style=='code')){
		if(wid.ingrid)grid_exit()
		if(wid.infield)field_exit()
		wid.active+=ev.shift?-1:1
		if(wid.active<0)wid.active+=wid.count;wid.cursor=rect()
	}
	if(uimode!='interact'&&uimode!='script'&&ms.type==null){
		if(wid.ingrid ||wid.gv!=null)grid_exit()
		if(wid.infield||wid.fv!=null)field_exit()
		wid.active=-1
	}wid.count=0,wid.scrolls=0,wid.ingrid=0,wid.infield=0
}

scrollbar=(r,n,line,page,scroll,visible,inverted)=>{
	const addscroll=x=>scroll=clamp(0,scroll+x,n)
	addscroll(0);const sz=ARROWS[0].size.x+2, fcol=!in_layer()?13:inverted?32:1, bcol=inverted?1:32
	const b=rect(r.x+r.w-sz-2,r.y,sz+2,r.h), rr=rect(r.x+1,r.y,visible?r.w-b.w-1:r.w-2,r.h-2), pal=deck.patterns.pal.pix
	let dragging_thumb=(wid.thumbid==wid.scrolls++)&&dover(b)
	if(visible){
		draw_box(b,0,fcol)
		arrow=(bb,base,dir)=>{
			const a=n>0&&over(bb)&&!dragging_thumb, o=a&&(ev.mu||ev.drag)&&dover(r)
			draw_box(bb,0,fcol),draw_icon(rect(bb.x+2,bb.y+2),ARROWS[base+(o?2:0)],fcol)
			if(o&&ev.md)addscroll(line*dir);if(a&&!ev.drag)uicursor=cursor.point
		}
		arrow(rect(b.x,b.y         ,sz+2,sz+2),0,-1)
		arrow(rect(b.x,b.y+b.h-sz-2,sz+2,sz+2),1, 1)
		if(n<=0||!in_layer())return {size:rr,scroll}
		const s=rect(b.x+1,b.y+sz+2,b.w-2,b.h-2*(sz+2));draw_rect(s,inverted?9:12)
		const thumb_height=0|max(16,s.h/(1+n)), thumb_y=0|((s.h-thumb_height)*(scroll/n)), thumb=rect(s.x,s.y+thumb_y,s.w,thumb_height)
		if(in_layer()&&ev.md&&over(thumb)){wid.thumbid=wid.scrolls-1,wid.thumbo=ev.dpos.y-thumb.y,dragging_thumb=1}
		if(in_layer()&&ev.drag&&dragging_thumb){
			const capped=max(s.y,min(s.y+s.h-thumb.h,ev.pos.y-wid.thumbo))-s.y
			thumb.y=capped+s.y,scroll=max(0,0|min((capped/(s.h-thumb_height))*n,n)),uicursor=cursor.drag
		}
		if(in_layer()&&ev.mu&&dragging_thumb)wid.thumbid=-1
		draw_rect(thumb,bcol),draw_box(thumb,0,fcol)
		inner=(bb,dir)=>{
			if(!dragging_thumb&&over(bb))uicursor=cursor.point
			if(!dragging_thumb&&ev.mu&&over(bb))draw_invert(pal,bb),addscroll(page*dir)
		}
		if(thumb_y>0          )inner(rect(s.x,s.y                ,s.w,     thumb_y         ),-1)
		if(thumb_y+thumb.h<s.h)inner(rect(s.x,s.y+thumb_y+thumb.h,s.w,s.h-(thumb_y+thumb.h)), 1)
	}
	if(in_layer()&&over(runion(r,b))&&ev.scroll)addscroll(ev.scroll*line)
	return {size:rr,scroll}
}

widget_button=(target,x,value,func)=>{
	const l=x.locked||!in_layer(), pal=deck.patterns.pal.pix, font=x.font||FONT_MENU;let b=x.size
	const fcol=l?13:x.show=='invert'?32:1, bcol=x.show=='invert'?1:32, scol=x.show=='invert'?32:1
	const sel=!l&&x.show!='none'&&x.style!='invisible'&&wid.active==wid.count
	const a=!l&&dover(b)&&over(b), cs=sel&&!func&&ev.action, cl=cs||((ev.md||ev.drag)&&a), cr=cs||(ev.mu&&a)
	if(func&&a){ev.callback=func,ev.callback_rect=rcopy(b)}
	if(!l&&over(b)&&!ev.drag)uicursor=cursor.point
	if(x.show=='none')return cr; let ar=inset(b,2)
	if(x.style=='round'){
		draw_boxr(b,fcol,bcol,x.show!='transparent')
		draw_textc(inset(b,3),x.text,font,fcol)
		if(sel)draw_box(ar,0,13);if(cl)draw_invert(pal,ar)
	}
	if(x.style=='rect'){
		if(cl){b=rect(b.x+1,b.y+1,b.w-1,b.h-1),ar=rect(ar.x+1,ar.y+1,ar.w-1,ar.h-1);if(x.show!='transparent')draw_rect(b,bcol);draw_box(b,0,fcol)}
		else  {b=rect(b.x  ,b.y  ,b.w-1,b.h-1),ar=rect(ar.x  ,ar.y  ,ar.w-1,ar.h-1);draw_shadow(b,fcol,bcol,x.show!='transparent')}
		draw_textc(inset(b,3),x.text,font,fcol);if(sel)draw_box(ar,0,13)
	}
	if(x.style=='check'||x.style=='radio'){
		if(x.show!='transparent')draw_rect(b,bcol)
		const ts=font_textsize(font,x.text), cdim=(x.style=='check'?CHECKS[0]:RADIOS[0]).size, bh=max(ts.y,cdim.y)
		const br=rect(b.x,b.y+(0|((b.h-bh)/2)),b.w,bh), to=rclip(b,rect(br.x+cdim.x,0|(br.y+(br.h-ts.y)/2),b.w-cdim.x,ts.y))
		draw_rect(rect(br.x+1,br.y+1,cdim.x-4,cdim.y-3),bcol)
		if(x.style=='check'){draw_icon(rect(br.x,br.y),CHECKS[(value^(cl||cr))+2*x.locked],scol)}
		else{const p=rect(br.x,br.y);draw_icon(p,RADIOS[3],bcol),draw_icon(p,RADIOS[cl||cr?1:0],fcol);if(value)draw_icon(p,RADIOS[2],fcol)}
		draw_text_fit(to,x.text,font,fcol);ar=to;if(sel)draw_box(rect(to.x-2,to.y-1,to.w+2,to.h+2),0,13);if(cl)draw_invert(pal,ar)
	}
	if(x.style=='invisible'){draw_textc(inset(b,3),x.text,font,fcol);if(cl)draw_invert(pal,ar)}
	if(target&&cr)msg.target_click=target
	if(!x.locked&&in_widgets())wid.count++
	return cr
}

widget_slider=(target,x)=>{
	const l=x.locked||!in_layer(), pal=deck.patterns.pal.pix, font=x.font||FONT_MENU
	let b=x.size, fcol=l?13:x.show=='invert'?32:1, bcol=x.show=='invert'?1:32, bpat=x.show=='invert'?9:12
	const sel=!l&&x.show!='none'&&wid.active==wid.count, ov=x.value
	if(x.show=='none')return
	const t=ls((x.style=='bar'||x.style=='compact')?dyad.format(lms(x.format),lmn(x.value)):lms(''))
	const oc=frame.clip; frame.clip=rclip(b,frame.clip)
	const hv_btn=(bb,dir,ba)=>{
		const a=!l&&over(bb), o=a&&(ev.mu||ev.drag)&&dover(bb)
		if(o&&ev.md)x.value+=(dir*x.step); if(a)uicursor=cursor.point
		draw_rect(bb,bcol),draw_box(bb,0,fcol),draw_icon(rint(rect(bb.x+(bb.w-12)/2,bb.y+(bb.h-12)/2)),ARROWS[ba+(o?2:0)],fcol)
	}
	const hv_tsz=(axis,tf,av,as)=>{
		let drag=(wid.thumbid==wid.scrolls++)&&dover(b),
		n=0|((x.max-x.min)/x.step), ts=0|max(16,axis/(1+n)), tp=0|(((x.value-x.min)/max(1,(x.max-x.min)))*(axis-ts)), thumb=rint(tf(tp,ts))
		if(!l){
			if(ev.md&&over(thumb))wid.thumbid=wid.scrolls-1,wid.thumbo=ev.dpos[av]-thumb[av],drag=1
			if(ev.drag&&drag){
				const capped=max(b[av],min(b[av]+b[as]-thumb[as],ev.pos[av]-wid.thumbo))-b[av]
				thumb[av]=capped+b[av],uicursor=cursor.drag,x.value=x.min+capped*((x.max-x.min)/(b[as]-ts))
			}if(ev.mu&&drag)wid.thumbid=-1
		}draw_rect(thumb,bcol),draw_box(thumb,0,fcol);return {drag,thumb}
	}
	const hv_gap=(drag,bb,dir)=>{if(!l&&!drag&&over(bb)){uicursor=cursor.point;if(ev.mu)draw_invert(pal,bb),x.value+=(dir*10*x.step);}}
	if(x.style=='horiz'||x.style=='vert'){if(x.show!='transparent')draw_rect(b,bpat);draw_box(b,0,sel?13:fcol)}
	if(x.style=='horiz'){
		hv_btn(rect(b.x,b.y,16,b.h),-1,4),hv_btn(rect(b.x+b.w-16,b.y,16,b.h),1,5),b=rect(b.x+16,b.y+1,b.w-32,b.h-2)
		const a=hv_tsz(b.w,(tp,ts)=>rect(b.x+tp,b.y,ts,b.h),'x','w'), drag=a.drag, thumb=a.thumb
		hv_gap(drag,rect(b.x,b.y,thumb.x-b.x,b.h),-1),hv_gap(drag,rect(thumb.x+thumb.w,b.y,b.w-(thumb.x+thumb.w-b.x),b.h),1)
	}
	if(x.style=='vert'){
		hv_btn(rect(b.x,b.y,b.w,16),-1,0);hv_btn(rect(b.x,b.y+b.h-16,b.w,16),1,1),b=rect(b.x+1,b.y+16,b.w-2,b.h-32)
		const a=hv_tsz(b.h,(tp,ts)=>rect(b.x,b.y+tp,b.w,ts),'y','h'), drag=a.drag, thumb=a.thumb
		hv_gap(drag,rect(b.x,b.y,b.w,thumb.y-b.y),-1),hv_gap(drag,rect(b.x,thumb.y+thumb.h,b.w,b.h-(thumb.y+thumb.h-b.y)),1)
	}
	if(x.style=='bar'){
		fcol=x.locked?(x.show=='invert'?32:1):fcol
		if(x.show!='transparent')draw_rect(b,bcol);draw_box(b,0,sel?13:fcol)
		b=inset(b,2),draw_textc(b,t,font,fcol);if(!l&&over(b))uicursor=cursor.point
		const f=(x.max==x.min)?0:((x.value-x.min)/(x.max-x.min))*b.w;draw_invert(pal,rect(b.x,b.y,0|f,b.h))
		if(!l&&dover(b)&&(ev.md||ev.drag)){x.value=x.min+(ev.pos.x-b.x)*((x.max-x.min)/b.w),uicursor=cursor.drag}
	}
	if(x.style=='compact'){
		if(x.show=='transparent')draw_rect(rect(b.x+1,b.y+1,13,b.h-2),bcol),draw_rect(rect(b.x+b.w-14,b.y+1,13,b.h-2),bcol)
		draw_boxr(b,fcol,bcol,x.show!='transparent'),draw_textc(rect(b.x+14,b.y,b.w-28,b.h),t,font,fcol)
		const comp_btn=(xo,dir,ba,li)=>{
			const bb=rect(b.x+xo,b.y,14,b.h), a=!l&&over(bb), o=a&&(ev.mu||ev.drag)&&dover(bb)
			if(o&&ev.md)x.value+=(dir*x.step); if(a)uicursor=cursor.point
			draw_icon(rect(bb.x+1,0|(b.y+(b.h-12)/2)),ARROWS[ba+(o?2:0)],fcol),draw_vline(bb.x+li,b.y+1,b.y+b.h-1,sel?13:fcol)
		};comp_btn(0,-1,4,13),comp_btn(b.w-14,1,5,0)
	}
	if(sel&&ev.dir=='up')x.value-=x.step;if(sel&&ev.dir=='down')x.value+=x.step
	if(in_layer()&&over(frame.clip)&&ev.scroll)x.value+=x.step*ev.scroll; x.value=slider_normalize(target,x.value)
	if(target&&Math.abs(ov-x.value)>(x.step/2)){msg.target_change=target,msg.arg_change=lmn(x.value);iwrite(target,lms('value'),msg.arg_change),mark_dirty()}
	if(!x.locked&&in_widgets())wid.count++
	frame.clip=oc
}

widget_canvas=(target,x,image)=>{
	if(x.show=='none')return; const b=x.size, pal=deck.patterns.pal.pix
	if(x.show=='solid'){if(image){draw_scaled(b,image,1)}else{draw_rect(b,0)}}
	if(image&&x.show=='transparent')draw_scaled(b,image,0)
	if(image&&x.show=='invert')draw_invert_scaled(pal,b,image)
	if(x.border){if(x.show=='invert'){draw_boxinv(pal,b)}else{draw_box(b,0,1)}}
	if(!target||!in_layer())return
	if(x.draggable){
		const sel=ob.sel.length&&ob.sel[0]==target
		if     (ev.md&&dover(b))msg.target_click  =target,msg.arg_click  =rect(b.x,b.y),ob.sel=[target],ob.prev=rint(rsub(ev.pos,b))
		else if(ev.mu     &&sel)msg.target_release=target,msg.arg_release=rsub(ev.dpos,ob.prev),ob.sel=[]
		else if(ev.drag   &&sel)msg.target_drag   =target,msg.arg_drag   =rsub(ev.dpos,ob.prev)
	}else if(dover(b)){
		const p=rint(rect((ev.pos.x-b.x)/x.scale,(ev.pos.y-b.y)/x.scale)), dp=p.x!=msg.lastdrag.x||p.y!=msg.lastdrag.y, im=over(b)
		if     (ev.md          )msg.target_click  =target,msg.arg_click  =p,msg.lastdrag=p
		else if(ev.mu          )msg.target_release=target,msg.arg_release=p
		else if(ev.drag&&dp&&im)msg.target_drag   =target,msg.arg_drag   =p,msg.lastdrag=p
	}
}

grid_exit=_=>{
	wid.ingrid=0,wid.gv=null,wid.gt=null
	if(wid.hist)wid.hist=[],wid.hist_cursor=0
}
widget_grid=(target,x,value)=>{
	if(x.show=='none')return 0; const hfnt=FONT_BODY, hsize=x.headers?font_h(hfnt)+5:0, showscroll=x.size.h<=(50+hsize)||x.size.w<16?0:x.scrollbar
	const fnt=x.font?x.font:FONT_MONO, os=value.scroll, or=value.row, files=x.headers==2, headers=files||x.size.h<=hsize?0:x.headers
	const tk=Object.keys(value.table.v), nr=count(value.table), nc=tk.length, rh=font_h(fnt)+(x.lines?5:3)
	const fcol=!in_layer()?13:x.show=='invert'?32:1, bcol=x.show=='invert'?1:32, b=x.size, pal=deck.patterns.pal.pix
	let sel=in_layer()&&x.show!='none'&&wid.active==wid.count
	if(in_layer()&&dover(b)&&(ev.md||ev.mu||ev.drag)){if(!sel&&wid.gv)grid_exit(); wid.active=wid.count,sel=1}
	if(sel&&in_layer()&&!over(b)&&ev.md)sel=0,wid.active=-1,grid_exit()
	if(sel){if(wid.fv)field_exit();wid.ingrid=1,wid.g=x,wid.gv=value,wid.gt=target}; if(x.show!='transparent')draw_rect(b,bcol)
	const bh=rect(b.x,b.y,b.w,headers?font_h(hfnt)+5:0), nrd=0|min(nr,((b.h-bh.h+1)/rh)), scrollmax=nr-nrd
	const sbar=scrollbar(rect(b.x,b.y+(headers?bh.h-1:0),b.w,b.h-(headers?bh.h-1:0)),scrollmax,1,nrd,value.scroll,showscroll,x.show=='invert')
	const bb=sbar.size; value.scroll=sbar.scroll, hwt=x.widths.reduce((x,y)=>x+y,0)
	draw_box(x.lines?b:rect(b.x,bb.y,b.w,b.h-(bb.y-b.y)),0,sel?13:fcol)
	const cw=n=>0|(n>=x.widths.length?((bb.w-hwt)/(nc-x.widths.length)):x.widths[n])
	if(x.lines)draw_rect(bh,fcol);if(nc<=0)draw_textc(inset(bb,1),'(no data)',hfnt,fcol)
	for(let z=0,cols=0,cx=0;z<nc&&cx+cw(cols)<=bb.w;z++){
		const hs=rect(bh.x+4+cx,bh.y+1,cw(cols)-5,bh.h-2)
		if(headers){
			draw_textc(hs,tk[z],hfnt,x.lines?bcol:fcol)
			const oa=target&&in_layer()&&over(hs)&&((ev.drag||ev.mu)?dover(hs):1)&&!wid.col_drag
			if(oa&&(ev.md||ev.drag))draw_invert(pal,hs); if(oa&&!ev.drag)uicursor=cursor.point
			if(oa&&ev.mu)msg.target_order=target,msg.arg_order=lms(tk[z])
		}
		if(cols&&x.lines)draw_invert(pal,rect(hs.x-3,b.y+1,1,b.h-2));cx+=cw(cols),cols++
		for(let y=0;y<nrd;y++){
			const cell=rect(hs.x-3,bb.y+rh*y+1,hs.w+5,rh-1), v=value.table.v[tk[z]][y+value.scroll]
			const cf=ls(dyad.format(lms(`%${x.format[z]=='L'?'s':(x.format[z]||'s')}`),v)), ip=rcenter(cell,ICONS[0].size)
			const oc=frame.clip; frame.clip=rclip(cell,frame.clip)
			if     (x.format[z]=='I'){const i=clamp(0,ln(v),8);if(i<8)draw_icon(ip,ICONS[i],fcol)}
			else if(x.format[z]=='B'){if(lb(v))draw_icon(ip,ICONS[ICON.chek],fcol)}
			else{draw_text_fit(rect(hs.x+1,bb.y+rh*y,hs.w-2,rh),cf,fnt,fcol)}
			frame.clip=oc
			if(sel&&ev.dclick&&!x.locked&&over(cell)){
				const f=x.format[z]||'s', tc=rect(z,y+value.scroll)
				if     (f=='I'||f=='L'){} // no editing allowed
				else if(f=='B'||f=='b'){grid_edit_cell(tc,lmn(!lb(v)))} // toggle
				else{wid.pending_grid_edit=1,ms.cell=tc,ms.text=fieldstr(lms(cf))}
			}
		}
	}
	if(!x.locked&&in_layer()&&target)for(let z=0,cx=bh.x;z<nc;cx+=cw(z),z++){
		const h=rect(cx+cw(z)-1,bh.y,5,bh.h);if(h.x+h.w>b.x+b.w)break
		if(over(h))draw_vline(h.x+2,h.y,h.y+h.h,13)
		if(ev.md&&dover(h))wid.col_drag=1,wid.col_num=z,wid.col_orig=cw(z)
		if(sel&&wid.col_drag&&wid.col_num==z&&ev.drag){
			const s=min(max(10,wid.col_orig+(ev.pos.x-ev.dpos.x)),bb.w-10),i=z;uicursor=cursor.drag
			iwrite(target,lms('widths'),lml(range(max(x.widths.length,i+1)).map(z=>lmn(i==z?s:cw(z))))),mark_dirty()
		}
	}
	const rowb=n=>rect(bb.x,bb.y+rh*n,bb.w,rh)
	const rowh=n=>inset(rect(bb.x+1,bb.y+rh*n+2,bb.w-2,rh-3),x.lines?0:-1)
	let clicked=0,rsel=0;for(let y=0;y<nrd;y++){
		if(y&&x.lines)draw_hline(bb.x,bb.x+bb.w,bb.y+rh*y,fcol); const ra=in_layer()&&over(bb)&&over(rowb(y))
		if(ra&&(ev.md||ev.drag))draw_invert(pal,rowh(y)),rsel=1; if(ra&&ev.mu)clicked=1,value.row=y+value.scroll; if(ra&&!ev.drag)uicursor=cursor.point
	}
	const rr=value.row-value.scroll;if(!rsel&&rr>=0&&rr<nrd)draw_invert(pal,rowh(rr))
	if(target&&os!=value.scroll)iwrite(target,lms('scroll'),lmn(value.scroll)),mark_dirty()
	if(target&&or!=value.row)iwrite(target,lms('row'),lmn(value.row)),mark_dirty()
	if(target&&clicked)msg.target_click=target,msg.arg_click=rect(0,value.row)
	if(in_widgets())wid.count++
	return files?((clicked&&ev.dclick)||(sel&&ev.action)): clicked
}
grid_format=_=>lms(wid.g.format.length?wid.g.format:'s'.repeat(Object.keys(wid.gv.table.v).length))
grid_apply=v=>{
	wid.gv.table=v,wid.gv.row=-1;if(!wid.gt)return
	iwrite(wid.gt,lms('value'),v),iwrite(wid.gt,lms('row'),lmn(-1)),mark_dirty()
	msg.target_change=wid.gt,msg.arg_change=v
}
grid_undo=_=>{const x=wid.hist[--(wid.hist_cursor)];grid_apply(x[0])}
grid_redo=_=>{const x=wid.hist[(wid.hist_cursor)++];grid_apply(x[1])}
grid_edit=v=>{wid.hist=wid.hist.slice(0,wid.hist_cursor),wid.hist.push([wid.gv.table,v]),grid_redo()}
grid_deleterow=_=>grid_edit(dyad.drop(lml([lmn(wid.gv.row)]),wid.gv.table))
grid_insertrow=_=>{
	const f=grid_format(), x=wid.gv.table.v, r={}, s=wid.gv.row+1
	Object.keys(x).map((k,col)=>{r[k]=range(x[k].length+1).map(i=>(i==s)?('sluro'.indexOf(f[col])>=0?lms(''):NONE): x[k][i-(i>=s?1:0)])})
	grid_edit(lmt(r))
}
grid_edit_cell=(cell,v)=>{
	const r={};Object.keys(wid.gv.table.v).map((k,i)=>{r[k]=wid.gv.table.v[k].slice(0);if(i==cell.x)r[k][cell.y]=v})
	grid_edit(lmt(r))
}
grid_keys=(code,shift)=>{
	const fnt=wid.g.font?wid.g.font:FONT_MONO, hfnt=FONT_BODY;let m=0, nr=count(wid.gv.table), r=wid.gv.row
	const rh=font_h(fnt)+5, bh=wid.g.headers?font_h(hfnt)+5:0, nrd=min(nr,0|((wid.g.size.h-bh+1)/rh))
	if(code=='ArrowUp'  ){m=1;if(r==-1){r=0}else{r-=1}}
	if(code=='ArrowDown'){m=1;if(r==-1){r=0}else{r+=1}}
	if(code=='PageUp'   ){m=1;if(r==-1)r=0;r-=nrd}
	if(code=='PageDown' ){m=1;if(r==-1)r=0;r+=nrd}
	if(code=='Home'     ){m=1,r=0}
	if(code=='End'      ){m=1,r=nr-1}
	if(code=='Backspace'||code=='Delete')grid_deleterow()
	if(!m)return
	wid.gv.row=r=max(0,min(r,nr-1));if(wid.gt){iwrite(wid.gt,lms('row'),lmn(r)),mark_dirty(),msg.target_click=wid.gt,msg.arg_click=rect(0,r)}
	const os=wid.gv.scroll;if(r-os<0)wid.gv.scroll=r;if(r-os>=nrd)wid.gv.scroll=r-(nrd-1)
	if(wid.gt&&os!=wid.gv.scroll)iwrite(wid.gt,lms('scroll'),lmn(wid.gv.scroll)),mark_dirty()
}

layout_index=(x,p)=>{
	for(let z=0;z<x.lines.length;z++){
		const l=x.lines[z].pos  ;if(p.y>l.y+l.h)continue
		const r=x.lines[z].range;if(p.y<l.y    )return r.x
		for(let i=r.x;i<=r.y;i++){const g=x.layout[i].pos;if(p.x<g.x+(g.w/2))return i;}
		return z==x.lines.length-1?x.layout.length: r.y
	}return x.layout.length
}
layout_last=(x,font)=>x.layout.length>0?last(x.layout): {pos:rect(0,0,1,font_h(font)),line:0,char:'\0',font,arg:NONE}
layout_cursor=(x,index,font,f)=>{
	const bw=f.size.w-5-(f.scrollbar?ARROWS[0].size.x+3:0), bx=f.align=='center'?0|(bw/2): f.align==ALIGN.right?bw: 0
	const r=x.layout.length>0?rcopy(x.layout[min(index,x.layout.length-1)].pos):rect(bx,0,1,font_h(font))
	if(index>=x.layout.length){if(layout_last(x,font).char=='\n'){r.x=0,r.y+=r.h}else{r.x+=r.w-1}}
	r.w=1;return r
}
field_change=_=>{
	if(!wid.field_dirty||!wid.ft)return
	iwrite(wid.ft,lms('value'),wid.fv.table),mark_dirty()
	msg.target_change=wid.ft, msg.arg_change=rtext_string(wid.fv.table)
}
field_exit=_=>{field_change();wid.infield=0,wid.fv=null,wid.ft=null,wid.cursor=rect(),wid.field_dirty=0,wid.change_timer=0}
widget_field=(target,x,value)=>{
	if(x.show=='none')return; if(x.size.h<=50||x.size.w<16)x.scrollbar=0
	const l=!in_layer(), fnt=x.font?x.font: x.style=='code'?FONT_MONO: FONT_BODY, b=x.size, pal=deck.patterns.pal.pix
	const fcol=(l&&!x.locked)?13:x.show=='invert'?32:1, bcol=x.show=='invert'?1:32, os=value.scroll
	if(x.show!='transparent')draw_rect(rclip(b,frame.clip),bcol); if(x.border)draw_box(b,0,fcol)
	let bi=inset(b,2);if(x.scrollbar)bi.w-=ARROWS[0].size.x+3
	if(!l&&!x.locked&&over(bi)&&(ev.drag?dover(bi):1))uicursor=cursor.ibeam
	const layout=layout_richtext(deck,value.table,fnt,x.align,bi.w), last=layout_last(layout,fnt), eol=last.char!='\n'?0: last.pos.h
	const sbar=scrollbar(b,max(0,(last.pos.y+last.pos.h+eol)-bi.h),10,bi.h,value.scroll,x.scrollbar,x.show=='invert');value.scroll=sbar.scroll
	let sel=!x.locked&&!l&&wid.active==wid.count
	// find active link (if any)
	let alink=null;if(x.locked&&!sel&&in_layer()&&x.locked&&(ev.md||ev.drag))for(let z=0;z<layout.layout.length;z++){
		const g=layout.layout[z], pos=rcopy(g.pos);if(pos.w<1)continue // skip squashed spaces/newlines
		pos.y-=value.scroll;if(pos.y+pos.h<0||pos.y>bi.h)continue; pos.x+=bi.x, pos.y+=bi.y // coarse clip
		if(lis(g.arg)&&count(g.arg)&&dover(pos)&&over(pos)){alink=g.arg;break}
	}
	if(!x.locked&&!l&&dover(bi)&&(ev.md||ev.mu||ev.drag)){
		const i=layout_index(layout,rect(ev.pos.x-bi.x,ev.pos.y-bi.y+value.scroll))
		if(ev.md){wid.cursor.x=wid.cursor.y=i}else{wid.cursor.y=i}
		if(ev.dclick){ // double-click to select a word or whitespace span:
			let a=0, w=layout.layout.length&&/\s/g.test(layout.layout[min(wid.cursor.y,layout.layout.length-1)].char)
			a=wid.cursor.y;while(a>=0&&a<layout.layout.length&&(w^!/\s/g.test(layout.layout[a].char)))a--;wid.cursor.x=a+1
			a=wid.cursor.y;while(      a<layout.layout.length&&(w^!/\s/g.test(layout.layout[a].char)))a++;wid.cursor.y=a
		}
		const c=layout_cursor(layout,wid.cursor.y,fnt,x);c.y-=value.scroll, ch=min(bi.h,c.h)
		if(c.y<0)value.scroll-=4;if(c.y+ch>bi.h)value.scroll+=clamp(1,(c.y+ch)-bi.h,4) // drag to scroll!
		if(!sel&&wid.fv)field_exit(); wid.active=wid.count,sel=1
	}
	if(sel&&in_layer()&&!over(b)&&ev.md)sel=0,wid.active=-1,field_exit()
	if(sel){if(wid.gv)grid_exit();wid.infield=1,wid.f=x,wid.fv=value,wid.ft=target}
	// render
	bi=rclip(frame.clip,bi); const oc=frame.clip;frame.clip=bi
	for(let z=0;z<layout.layout.length;z++){
		const g=layout.layout[z], pos=rcopy(g.pos);if(pos.w<1)continue // skip squashed spaces/newlines
		pos.y-=value.scroll;if(pos.y+pos.h<0||pos.y>bi.h)continue; pos.x+=bi.x, pos.y+=bi.y // coarse clip
		if(lis(g.arg)&&count(g.arg)){
			draw_hline(pos.x,pos.x+pos.w,pos.y+pos.h-1,alink==g.arg?fcol:19)
			const a=x.locked&&in_layer()&&over(pos)&&target;if(a&&!ev.drag)uicursor=cursor.point
			if(a&&ev.mu&&dover(pos))msg.target_link=target,msg.arg_link=g.arg
		}
		const csel=sel&&wid.cursor.x!=wid.cursor.y&&z>=min(wid.cursor.x,wid.cursor.y)&&z<max(wid.cursor.x,wid.cursor.y)
		if(csel)draw_rect(rclip(pos,frame.clip),fcol)
		if(image_is(g.arg)){image_paste(pos,frame.clip,g.arg,frame.image,x.show!='transparent');if(csel)draw_invert(pal,pos)}
		else{draw_char(pos,g.font,g.char,csel?bcol:fcol)}
	}
	if(sel&&wid.cursor_timer<FIELD_CURSOR_DUTY){
		const c=layout_cursor(layout,wid.cursor.y,fnt,x);c.y-=value.scroll;c.y+=bi.y,c.x+=bi.x
		draw_invert(pal,rclip(c,frame.clip))
	}
	if(target&&os!=value.scroll)iwrite(target,lms('scroll'),lmn(value.scroll)),mark_dirty()
	frame.clip=oc;if(!x.locked&&in_widgets())wid.count++
}
field_showcursor=_=>{
	const b=wid.f.size, bi=inset(b,2);if(wid.f.scrollbar)bi.w-=ARROWS[0].size.x+3
	const fnt=wid.f.font?wid.f.font: wid.f.style=='code'?FONT_MONO: FONT_BODY
	const layout=layout_richtext(deck,wid.fv.table,fnt,wid.f.align,bi.w), os=wid.fv.scroll
	const c=layout_cursor(layout,wid.cursor.y,fnt,wid.f), ch=min(bi.h,c.h);c.y-=wid.fv.scroll
	if(c.y<0){wid.fv.scroll+=c.y}if(c.y+ch>=bi.h){wid.fv.scroll+=((c.y+ch)-bi.h)}
	if(wid.ft&&os!=wid.fv.scroll)iwrite(wid.ft,lms('scroll'),lmn(wid.fv.scroll)),mark_dirty()
	return layout
}
field_apply=(v,c)=>{
	wid.fv.table=v,wid.cursor=rect(c.x,c.y);if(wid.cursor.x<0)wid.cursor.x=0;if(wid.cursor.y<0)wid.cursor.y=0
	field_showcursor(),wid.field_dirty=1,wid.change_timer=FIELD_CHANGE_DELAY
}
field_undo=_=>{const x=wid.hist[--(wid.hist_cursor)];field_apply(x[0],x[1])}
field_redo=_=>{const x=wid.hist[(wid.hist_cursor)++];field_apply(x[2],x[3])}
field_edit=(font,arg,text,pos)=>{
	const c=rect(), spliced=rtext_splice(wid.fv.table,font,arg,text,pos,c); wid.hist=wid.hist.slice(0,wid.hist_cursor)
	wid.hist.push([
		wid.fv.table,rect(wid.cursor.x,wid.cursor.y), // before
		spliced     ,c                              , // after
	]),field_redo()
}
field_sel_lines=_=>{
	let a=min(wid.cursor.x,wid.cursor.y),b=max(wid.cursor.x,wid.cursor.y),l=field_showcursor()
	while(a                &&l.layout[a-1].char!='\n')a--
	while(b<l.layout.length&&l.layout[b  ].char!='\n')b++
	return {layout:l,sel:rect(a,b)}
}
field_comment=_=>{
	const s=field_sel_lines(), p=s.sel, layout=s.layout.layout; let ac=1,z=p.x;while(z<p.y){
		while(z<p.y&&layout[z].char==' ')z++
		if   (z<p.y&&layout[z].char!='#')ac=0
		while(z<p.y&&layout[z].char!='\n')z++;z++
	}
	let r='';z=p.x;while(z<p.y){
		while(z<p.y&&layout[z].char==' ')r+=' ',z++
		if(ac){if(layout[z].char=='#'){z++;if(z<p.y&&layout[z].char==' ')z++}}else{r+='# '}
		while(z<p.y&&layout[z].char!='\n')r+=layout[z++].char
		if(z<p.y&&layout[z].char=='\n')r+='\n',z++
	}field_edit(lms(''),lms(''),r,p),wid.cursor=rect(p.x,wid.cursor.y)
}
field_indent=add=>{
	const s=field_sel_lines(), p=s.sel, layout=s.layout.layout; let r='',z=p.x;while(z<p.y){
		if(add){r+=' '}else{if(layout[z].char==' ')z++}
		while(z<p.y&&layout[z].char==' ')r+=' ',z++
		while(z<p.y&&layout[z].char!='\n')r+=layout[z++].char
		if(z<p.y&&layout[z].char=='\n')r+='\n',z++
	}field_edit(lms(''),lms(''),r,p);wid.cursor=rect(p.x,wid.cursor.y)
}
field_stylespan=(font,arg)=>field_edit(font,arg,ls(rtext_string(wid.fv.table,wid.cursor)),wid.cursor)
field_input=text=>{
	if(text=='\n'){if(ms.type=='save')ev.action=1;if(ms.type=='save'||ev.shift)return}
	const rtext_font=(table,x)=>{const i=rtext_get(table,x);return i<0?lms(''):table.v.font[i]}
	field_edit(rtext_font(wid.fv.table,wid.cursor.y),lms(''),clchars(text),wid.cursor)
}
field_keys=(code,shift)=>{
	const b=wid.f.size, bi=inset(b,2);if(wid.f.scrollbar)bi.w-=ARROWS[0].size.x+3
	const fnt=wid.f.font?wid.f.font: wid.f.style=='code'?FONT_MONO: FONT_BODY, layout=layout_richtext(deck,wid.fv.table,fnt,wid.f.align,bi.w)
	let m=0, s=wid.cursor.x!=wid.cursor.y
	const l=wid.cursor.y>=layout.layout.length?layout.lines.length-1:layout.layout[wid.cursor.y].line, c=layout_cursor(layout,wid.cursor.y,fnt,wid.f)
	if(code=='ArrowLeft'   ){m=1;if(s&&!shift){wid.cursor.x=wid.cursor.y=min(wid.cursor.x,wid.cursor.y)}else{wid.cursor.y--}}
	if(code=='ArrowRight'  ){m=1;if(s&&!shift){wid.cursor.x=wid.cursor.y=max(wid.cursor.x,wid.cursor.y)}else{wid.cursor.y++}}
	if(code=='ArrowUp'     ){m=1;if(l>=0)wid.cursor.y=layout_index(layout,rect(c.x-1,layout.lines[l].pos.y                      -1   ))}
	if(code=='ArrowDown'   ){m=1;if(l>=0)wid.cursor.y=layout_index(layout,rect(c.x-1,layout.lines[l].pos.y+layout.lines[l].pos.h+1   ))}
	if(code=='PageUp'      ){m=1;if(l>=0)wid.cursor.y=layout_index(layout,rect(c.x-1,layout.lines[l].pos.y                      -bi.h))}
	if(code=='PageDown'    ){m=1;if(l>=0)wid.cursor.y=layout_index(layout,rect(c.x-1,layout.lines[l].pos.y+layout.lines[l].pos.h+bi.h))}
	if(code=='Home'        ){m=1,wid.cursor.y=0}
	if(code=='End'         ){m=1,wid.cursor.y=layout.layout.length;}
	if(code=='Backspace'   ){field_edit(lms(''),lms(''),'',s?wid.cursor:rect(wid.cursor.y-1,wid.cursor.y))}
	if(code=='Delete'      ){field_edit(lms(''),lms(''),'',s?wid.cursor:rect(wid.cursor.y,wid.cursor.y+1))}
	if(code=='Enter'       ){
		if(shift&&wid.ft){field_change(),msg.target_run=wid.ft,msg.arg_run=rtext_string(wid.fv.table,s?wid.cursor:rect(0,RTEXT_END))}
		else{
			let i=0;if(wid.f.style=='code'){
				const sl=field_sel_lines(),s=sl.sel,layout=sl.layout.layout
				while(s.x<layout.length&&layout[s.x].char==' ')i++,s.x++
			}field_input('\n'+' '.repeat(i))
		}
	}
	if(code=='Tab'&&wid.f.style=='code'){if(!shift&&!s){field_input(' ')}else{field_indent(!shift)}}
	const nl=layout_richtext(deck,wid.fv.table,fnt,wid.f.align,bi.w)
	wid.cursor.y=clamp(0,wid.cursor.y,nl.layout.length); if(!m)return
	wid.cursor_timer=0; if(!shift)wid.cursor.x=wid.cursor.y; field_showcursor()
}

ui_button  =(r,label,    enable,func )=>widget_button(null,{text:label,size:r,font:FONT_MENU,style:'round',show:             'solid',locked:!enable},0,func)
ui_toggle  =(r,label,inv,enable,func )=>widget_button(null,{text:label,size:r,font:FONT_MENU,style:'round',show:inv?'invert':'solid',locked:!enable},0,func)
ui_radio   =(r,label,    enable,value)=>widget_button(null,{text:label,size:r,font:FONT_BODY,style:'radio',show:             'solid',locked:!enable},value)
ui_checkbox=(r,label,    enable,value)=>widget_button(null,{text:label,size:r,font:FONT_BODY,style:'check',show:             'solid',locked:!enable},value)
ui_field   =(r,       value)=>widget_field(null,{size:r,font:FONT_BODY,show:'solid',scrollbar:0,border:1,style:'plain',align:ALIGN.left,locked:0},value)
ui_textedit=(r,border,value)=>widget_field(null,{size:r,font:FONT_BODY,show:'solid',scrollbar:1,border  ,style:'plain',align:ALIGN.left,locked:0},value)
ui_codeedit=(r,border,value)=>widget_field(null,{size:r,font:FONT_MONO,show:'transparent',scrollbar:1,border  ,style:'code' ,align:ALIGN.left,locked:running()},value)
ui_table   =(r,widths,format,value)=>widget_grid(null,{size:r,font:FONT_BODY,widths   ,format   ,headers:2,scrollbar:1,lines:0,show:'solid',locked:1},value)
ui_list    =(r,              value)=>widget_grid(null,{size:r,font:FONT_BODY,widths:[],format:'',headers:0,scrollbar:1,lines:0,show:'solid',locked:1},value)

// The Listener

listen_show=(align,bare,x)=>{
	frame=context;while(li.hist.length>=LISTEN_LINES)li.hist.shift()
	li.hist.push([draw_lil(rsub(LISTEN_SIZE(),rect(18,5)),align,bare,x),x])
	li.scroll=RTEXT_END
}
n_show=(a)=>{if(a.length<2){listen_show(ALIGN.right,0,a[0])}else{listen_show(ALIGN.right,1,lms(a.map(show).join(' ')))};return a[0]}
n_print=(a)=>{if(a.length<2){listen_show(ALIGN.right,1,lms(ls(a[0])))}else{listen_show(ALIGN.right,1,a[0]=dyad.format(a[0],lml(a.slice(1))))}return a[0]}
n_pre_listen=([a])=>{
	const ev=getev();Object.keys(li.vars).map(name=>{if(!ev.v[name])ev.v[name]=li.vars[name]})
	if(ob.sel.length&&uimode=='object')ev.v.selected=lml(ob.sel.slice(0))
	return a
}
n_post_listen=([a])=>{
	const ev=getev();Object.keys(ev.v).map(name=>{li.vars[name]=ev.v[name]})
	li.vars['_']=a,listen_show(ALIGN.right,0,a);return a
}
n_post_query=([a])=>{ms.grid=gridtab(lt(a));return a}

// Audio

let audio=null, samples_playing=0, audio_loop=null, audio_loop_playing=null
const audioContext=window.AudioContext||window.webkitAudioContext, offline=window.OfflineAudioContext||window.webkitOfflineAudioContext
initaudio=_=>{if(!audio)audio=new audioContext({sampleRate:44100})}
load_sound=(file,after)=>{
	decode_sound=data=>{
		const r=[];for(let z=0;z<data.length;z+=8)r.push(sample_to_byte(data[z]));
		if(after){after(sound_make(Uint8Array.from(r)));return;}
		au.target=deck_add(deck,sound_read(ln(0))),mark_dirty()
		sound_edit(sound_make(Uint8Array.from(r))),au.sel=rect(),au.head=0,modal_enter('recording')
	}
	import_sound=buffer=>{
		initaudio(),audio.decodeAudioData(buffer,payload=>{
			const conv=new offline(1,payload.length*64000/payload.sampleRate,64000),buff=conv.createBufferSource()
			conv.oncomplete=e=>{decode_sound(e.renderedBuffer.getChannelData(0))}
			buff.buffer=payload,buff.connect(conv.destination),buff.start(0),conv.startRendering()
		})
	}
	const r=new FileReader();r.onload=_=>import_sound(r.result),r.readAsArrayBuffer(file)
}
sfx_stoploop=_=>{audio_loop_playing.onended=null,audio_loop_playing.stop(),audio_loop=audio_loop_playing=null}
sfx_doloop=clear=>{
	const a=audio_loop||NONE,b=lmblk(),pp=pending_popstate;let r=NONE, quota=LOOP_QUOTA
	blk_get(b,lms('loop')),blk_lit(b,lml([a])),blk_op(b,op.CALL)
	fire_hunk_async(ifield(deck,'card'),b)
	while(quota>0&&running()){runop(),quota--}
	if(!running())r=arg();popstate(),pending_popstate=pp
	if(clear&&audio_loop)sfx_stoploop()
	n_play([r,lms('loop')]),msg.pending_loop=0
}
n_play=([x,hint])=>{
	const prepare=sfx=>{
		const playback=audio.createBuffer(1,sfx.data.length*8,64000),dest=playback.getChannelData(0)
		for(let z=0;z<sfx.data.length;z++)for(let b=0;b<8;b++)dest[z*8+b]=MASTER_VOLUME*byte_to_sample(sfx.data[z])
		const playing=audio.createBufferSource();playing.buffer=playback,playing.connect(audio.destination);return playing
	}
	if(hint&&ls(hint)=='loop'){
		if(lis(x))x=dget(ifield(deck,"sounds"),x)
		if(x&&audio_loop==x){} // don't re-trigger
		else if(sound_is(x)&&ln(ifield(x,'size'))>0){
			if(audio_loop)sfx_stoploop()
			audio_loop=x,audio_loop_playing=prepare(x)
			audio_loop_playing.onended=_=>{sfx_doloop(1)}
			audio_loop_playing.start()
		}
		else if(audio_loop){sfx_stoploop()}
		return NONE
	}
	const sfx=!x?x: sound_is(x)?x: dget(deck.sounds,lms(ls(x)));if(!sfx||ln(ifield(sfx,'size'))<1)return NONE;initaudio()
	const playing=prepare(sfx);playing.addEventListener('ended',_=>{samples_playing--,audio_playing=samples_playing>0})
	playing.start(),samples_playing++,audio_playing=1;return NONE
}
stop_sound_pump=_=>{
	if(au.clip)au.clip.stop();au.clip=null,clearInterval(au.tick),au.mode='stopped'
	au.head=au.sel.x==au.sel.y?0:au.sel.x
}
play_sound_pump=sfx=>{
	initaudio();if(sfx.data.length<1)return
	const playback=audio.createBuffer(1,sfx.data.length*8,64000),dest=playback.getChannelData(0)
	for(let z=0;z<sfx.data.length;z++)for(let b=0;b<8;b++)dest[z*8+b]=MASTER_VOLUME*byte_to_sample(sfx.data[z])
	const playing=audio.createBufferSource();playing.buffer=playback,playing.connect(audio.destination)
	playing.addEventListener('ended',_=>stop_sound_pump),au.clip=playing;let counter=0
	au.tick=setInterval(_=>{const d=0|(8000*0.05);au.head+=d,counter+=d;if(au.mode!='playing'||counter>=sfx.data.length)stop_sound_pump()},50)
	au.mode='playing',playing.start()
}
sfx_any=_=>samples_playing>0

sound_slice=range=>{const r=[];for(let z=range.x;z<range.y;z++)r.push(au.target.data[z]);return sound_make(Uint8Array.from(r))}
sound_selected=_=>sound_slice(au.sel.x==au.sel.y?rect(0,au.target.data.length):au.sel)
sound_apply=v=>{au.target.data=Uint8Array.from(v.data),mark_dirty()}
sound_undo=_=>{const x=au.hist[--(au.hist_cursor)];sound_apply(x[0])}
sound_redo=_=>{const x=au.hist[(au.hist_cursor)++];sound_apply(x[1])}
sound_edit=v=>{au.hist=au.hist.slice(0,au.hist_cursor),au.hist.push([sound_slice(rect(0,au.target.data.length)),v]),sound_redo()}
sound_delete=_=>{
	const len=au.target.data.length, sel=au.sel.x==au.sel.y?rect(0,len):rcopy(au.sel), r=[]
	for(let z=0;z<    sel.x;z++)r.push(au.target.data[z      ])
	for(let z=0;z<len-sel.y;z++)r.push(au.target.data[z+sel.y])
	sound_edit(sound_make(Uint8Array.from(r))),au.head=au.sel.y=au.sel.x
}
sound_replace=s=>{
	r=[]
	for(let z=0       ;z<au.sel.x                                     ;z++)r.push(au.target.data[z])
	for(let z=0       ;z<s        .data.length&&r.length<(10*SFX_RATE);z++)r.push(s        .data[z]);const a=r.length
	for(let z=au.sel.y;z<au.target.data.length&&r.length<(10*SFX_RATE);z++)r.push(au.target.data[z])
	au.sel.y=a,au.head=r.length;return sound_make(Uint8Array.from(r))
}
sound_finish=_=>{
	au.head=(au.sel.x!=au.sel.y)?au.sel.x:0
	sound_undo(),sound_redo(),au.mode='stopped'
}
sound_begin_record=_=>{
	stop_sound_pump(),sound_edit(sound_slice(rect(0,au.target.data.length)))
	if(au.sel.x!=au.sel.y)au.head=au.sel.x;au.mode='recording'
}
sound_record=_=>{
	initaudio();if(au.record_stream){sound_begin_record();return}
	function resample(input,after){
		// Safari refuses to downsample to or play low sample rates,
		// so convert to 64khz as an even multiple of 8khz
		// and discretize from that. messy, but better than nothing:
		const conv=new offline(1,input.length*64000/input.sampleRate,64000), buff=conv.createBufferSource()
		conv.oncomplete=event=>{after(event.renderedBuffer.getChannelData(0))}
		buff.buffer=input,buff.connect(conv.destination),buff.start(0),conv.startRendering()
	}
	navigator.mediaDevices.getUserMedia({audio:true}).then(stream=>{
		try{
			const source=audio.createMediaStreamSource(stream)
			const scriptNode=audio.createScriptProcessor(4096,1,1)
			scriptNode.onaudioprocess=event=>{
				if(au.mode!='recording')return
				resample(event.inputBuffer,data=>{
					const r=[];for(let z=0;z<data.length;z+=8)r.push(sample_to_byte(data[z]))
					let h=au.head, end=(au.sel.x!=au.sel.y)?au.sel.y:(10*SFX_RATE), edit=au.hist[au.hist_cursor-1][1]
					const appended=min(max(au.target.data.length,h+r.length),end)
					if(appended>au.target.data.length){
						const a=new Uint8Array(appended), b=new Uint8Array(appended)
						for(let z=0;z<appended;z++)a[z]=b[z]=au.target.data[z]
						au.target.data=a,edit.data=b
					}
					for(let z=0;z<r.length&&h<end;z++){au.target.data[h]=edit.data[h]=r[z],h++}
					au.head=h;if(h>=end){sound_finish()}
				})
			}
			source.connect(scriptNode),scriptNode.connect(audio.destination)
			au.record_stream=stream,sound_begin_record()
		}catch(err){console.log(err),au.norecord=1}
	}).catch(err=>{console.log(err),au.norecord=1})
}

// Modal Helpers

transit_enumerate=_=>monad.table(monad.range(deck.transit))
sounds_enumerate=_=>{
	const r={icon:[],name:[],bytes:[],secs:[]};deck.sounds.v.map((sn,i)=>{
		r.icon .push(lmn(ICON.sound)),r.name.push(deck.sounds.k[i])
		r.bytes.push(dyad.format(lms('%.2fkb'),lmn(ln(ifield(sn,'size'))/1000.0*1.33)))
		r.secs .push(dyad.format(lms('%.2fs' ),ifield(sn,'duration')))
	});return lmt(r)
}
res_enumerate=(source)=>{
	const r={icon:[],name:[],value:[]}
	const pat=source.patterns,pp=patterns_write(pat),pa=anims_write(pat),da=dyad.parse(lms('%j'),lms(DEFAULT_ANIMS))
	if(!match(pa,da)||pp!=DEFAULT_PATTERNS)r.icon.push(lmn(ICON.pat)),r.name.push(lms('patterns')),r.value.push(pat)
	const fonts=dyad.drop(lms('mono'),dyad.drop(lms('menu'),dyad.drop(lms('body'),source.fonts)))
	fonts.v.map((font,i)=>{r.icon.push(lmn(ICON.font)),r.name.push(fonts.k[i]),r.value.push(font)})
	const sounds=source.sounds
	sounds.v.map((sound,i)=>{r.icon.push(lmn(ICON.sound)),r.name.push(sounds.k[i]),r.value.push(sound)})
	const modules=source.modules
	modules.v.map((mod,i)=>{r.icon.push(lmn(ICON.lil)),r.name.push(modules.k[i]),r.value.push(mod)})
	return lmt(r)
}

// Modal Dialogues

modal_enter=type=>{
	ev.md=ev.mu=ev.dclick=0,menus_clear()
	if(ms.type=='trans')return
	ms.from_listener=ms.type=='listen'
	ms.type=ms.subtype=type
	ms.old_wid=wid,wid=wid_state()
	if(type=='listen'){
		if(uimode=='script'){
			try{const text=ls(rtext_string(sc.f.table));parse(text),script_save(text)}
			catch(e){listen_show(align_right,1,lms('note: this script contains an error.\nexecuting under last saved version!'))}
		}
		ms.text=fieldstr(lms('')),li.scroll=RTEXT_END
	}
	if(type=='query')ms.grid=gridtab(ms.old_wid.gv.table),ms.text=fieldstr(lms('select from me.value'))
	if(type=='recording'){
		au.head=0,au.sel=rect(),au.mode='stopped',au.hist=[],au.hist_cursor=0
		ms.name=fieldstr(dkey(deck.sounds,au.target)||lms('unknown sound'))
	}
	if(type=='cards')ms.grid=gridtab(null)
	if(type=='sounds')ms.grid=gridtab(sounds_enumerate())
	if(type=='fonts'){
		const r=lmt({icon:[],name:[]});let fi=-1
		deck.fonts.v.map((font,z)=>{
			if(uimode=='object'){if(ob.sel.every(x=>ifield(x,'font')==font))fi=z}
			else if(ms.old_wid.ft){if(ifield(ms.old_wid.ft,'font')==font)fi=z}
			r.v.icon.push(lmn(ICON.font)),r.v.name.push(deck.fonts.k[z])
		}),ms.grid=gridtab(r,fi)
	}
	if(type=='resources')ms.message=null,ms.grid=gridtab(lmt({})),ms.grid2=gridtab(res_enumerate(deck))
	if(type=='link'){
		const t=ms.old_wid.fv.table,ol=t.v.arg[rtext_get(t,ms.old_wid.cursor.y)]
		ms.text=fieldstr(ol);if(count(ol))ms.old_wid.cursor=rtext_getr(t,ms.old_wid.cursor.y)
	}
	if(type=='grid'        )ms.name=fieldstr(lmn(dr.grid_size.x     )),ms.text=fieldstr(lmn(dr.grid_size.y       ))
	if(type=='deck_props'  )ms.name=fieldstr(ifield(deck     ,'name')),ms.text=fieldstr(ifield(deck     ,'author'))
	if(type=='button_props')ms.name=fieldstr(ifield(ob.sel[0],'name')),ms.text=fieldstr(ifield(ob.sel[0],'text'  ))
	if(type=='field_props' )ms.name=fieldstr(ifield(ob.sel[0],'name')),ms.text=fieldstr(ifield(ob.sel[0],'value' ))
	if(type=='grid_props'  )ms.name=fieldstr(ifield(ob.sel[0],'name')),ms.text=fieldstr(ifield(ob.sel[0],'format'))
	if(type=='canvas_props')ms.name=fieldstr(ifield(ob.sel[0],'name')),ms.text=fieldstr(ifield(ob.sel[0],'scale' ))
	if(type=='slider_props'){
		ms.name =fieldstr(   ifield(ob.sel[0],'name'    )    )
		ms.text =fieldstr(   ifield(ob.sel[0],'format'  )    )
		ms.form0=fieldstr(ll(ifield(ob.sel[0],'interval'))[0])
		ms.form1=fieldstr(ll(ifield(ob.sel[0],'interval'))[1])
		ms.form2=fieldstr(   ifield(ob.sel[0],'step'    )    )
	}
	if(type=='action'){
		sc.target=ob.sel[0],sc.others=[]
		ms.act_go=1,ms.act_gomode=4,ms.act_trans=0,ms.act_sound=0,ms.act_card=ln(ifield(ifield(deck,'card'),'index'))
		ms.verb   =lms('') // card name
		ms.message=lms('') // sound name
		ms.grid=gridtab(transit_enumerate())
	}
	const dname=(x,e)=>{x=x||'untitled';return lms(/\.(deck|html)$/.test(x)?x:x+e)}
	if(type=='card_props')ms.name=fieldstr(ifield(ifield(deck,'card'),'name'))
	if(type=='link'||type=='gridcell'||type=='query')wid.cursor=rect(0,RTEXT_END)
	if(type=='alert_lil'     )ms.type=type='alert'
	if(type=='confirm_lil'   )ms.type=type='confirm'
	if(type=='input_lil'     )ms.type=type='input'
	if(type=='confirm_new'   )ms.type=type='confirm'
	if(type=='confirm_script')ms.type=type='confirm'
	if(type=='multiscript'   )ms.type=type='confirm'
	if(type=='save_deck'     )ms.type=type='save',ms.text=fieldstr(dname(deck.name,'.deck')),ms.desc='Save a .deck or .html file.'
	if(type=='save_locked'   )ms.type=type='save',ms.text=fieldstr(dname(deck.name,'.html')),ms.desc='Save locked standalone deck as an .html file.'
	if(type=='export_table'  )ms.type=type='save',ms.text=fieldstr(lms(ls(ifield(ms.old_wid.gt,'name'))+'.csv')),ms.desc='Save table as a .csv file.'
	if(type=='export_script' )ms.type=type='save',ms.text=fieldstr(lms('script.lil'  )),ms.desc='Save script as a .lil file.'
	if(type=='export_image'  )ms.type=type='save',ms.text=fieldstr(lms('image.gif'   )),ms.desc='Save a .gif image file.'
	if(type=='save_lil'      )ms.type=type='save',ms.text=fieldstr(lms('untitled.txt')),ms.desc='Save a text file.'
	if(type=='input'         )ms.text=fieldstr(lms(''))
}
modal_exit=value=>{
	wid=ms.old_wid
	if(ms.type=='gridcell'&&value)grid_edit_cell(ms.cell,dyad.parse(lms(`%${ls(grid_format())[ms.cell.x]||'s'}`),rtext_string(ms.text.table)))
	if(ms.type=='card_props'){iwrite(ifield(deck,'card'),lms('name'),rtext_string(ms.name.table)),mark_dirty()}
	if(ms.type=='action'&&value){
		let r='on click do\n'
		if(ms.act_sound)r+=`  play[${show(ms.message)}]\n`
		if(ms.act_go){
			r+='  go['
			r+=ms.act_gomode==0?'"First"': ms.act_gomode==1?'"Prev"': ms.act_gomode==2?'"Next"': ms.act_gomode==3?'"Last"': show(ms.verb)
			if(ms.act_trans)r+=` ${show(ms.grid.table.v.value[ms.grid.row])}`
			r+=']\n'
		}r+='end',script_save(lms(r))
	}
	if(ms.type=='recording'){
		const name=rtext_string(ms.name.table);rename_sound(deck,au.target,name)
		au.mode='stopped',modal_enter('sounds'),ms.grid.row=dkix(deck.sounds,name);return
	}
	if(ms.subtype=='confirm_new'   &&value)load_deck(deck_read(''))
	if(ms.subtype=='confirm_script'&&value)finish_script()
	if(ms.subtype=='multiscript'   &&value)setscript(ob.sel)
	if(ms.subtype=='alert_lil'  )arg(),ret(lmn(value))
	if(ms.subtype=='confirm_lil')arg(),ret(lmn(value))
	if(ms.subtype=='input_lil'  )arg(),ret(rtext_string(ms.text.table))
	if(ms.subtype=='choose_lil' )arg(),ret(ms.verb.v[ms.grid.row])
	ms.type=null
	if(ms.from_listener)modal_enter('listen')
	if(ms.type==null&&uimode=='interact')msg.next_view=1
}
modals=_=>{
	ms.in_modal=1
	const pal=deck.patterns.pal.pix
	if(ms.type=='about'){
		const b=draw_modalbox(rect(190,70))
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',1)||ev.exit)modal_exit(0)
		draw_text(b,`Decker v${VERSION}`,FONT_MENU,1),b.y+=15
		draw_text(b,'by John Earnest',FONT_BODY,1),b.y+=12
	}
	else if(ms.type=='listen'){
		const size=LISTEN_SIZE(), th=li.hist.reduce((x,y)=>x+y[0].size.y+5,0), h=min(th,size.y)
		const esize=rect(0|((frame.size.x-size.x)/2),frame.size.y-49,size.x,50), tsize=rect(esize.x,esize.y-(h?h+5:0),esize.w,h)
		const bsize=rect(esize.x-5,esize.y-5-(h?tsize.h+5:0),esize.w+10,(h?tsize.h+5:0)+esize.h+20)
		draw_shadow(bsize,1,32,1),ui_codeedit(esize,1,ms.text)
		if(h){
			const sbar=scrollbar(tsize,max(0,th-size.y),10,tsize.h,li.scroll,h>=size.y,0), b=sbar.size; li.scroll=sbar.scroll
			let cy=0;li.hist.map(x=>{
				const l=x[0], t=x[1], s=l.size; let lb=rect(b.x,b.y+cy-li.scroll,s.x,s.y)
				image_paste(lb,b,l,frame.image,0),cy+=s.y+5
				lb=rclip(b,lb);const v=lis(t)&&over(lb), a=v&&dover(lb)
				if(v)uicursor=cursor.point,draw_box(inset(lb,-1),0,13); if(a&&(ev.md||ev.drag))draw_invert(pal,lb)
				if(a&&ev.mu)ms.text=fieldstr(t)
			})
		}
		if(ev.eval){
			const str=rtext_string(ms.text.table);if(count(str)<1)return
			try{
				const prog=parse(ls(str)), b=lmblk(); ms.text=fieldstr(lms('')),listen_show(ALIGN.left,1,str)
				const target=uimode=='script'?sc.target: ob.sel.length==1?ob.sel[0]: ifield(deck,'card')
				blk_opa(b,op.BUND,1),blk_lit(b,lmnat(n_pre_listen )),blk_lit(b,NONE   ),blk_op(b,op.CALL),blk_op(b,op.DROP),blk_cat(b,prog)
				blk_opa(b,op.BUND,1),blk_lit(b,lmnat(n_post_listen)),blk_op (b,op.SWAP),blk_op(b,op.CALL),blk_op(b,op.DROP),fire_hunk_async(target,b)
			}catch(e){listen_show(ALIGN.right,1,lms(`error: ${e.x}`));return}
		}if(ev.exit)modal_exit(0)
	}
	else if(ms.type=='cards'){
		const b=draw_modalbox(rect(210,frame.size.y-46)), gsize=rect(b.x,b.y+15,b.w,b.h-20-20)
		const slot=30,ch=slot*count(deck.cards); let m=0,props=0,gutter=-1,curr=ifield(deck,'card')
		draw_textc(rect(b.x,b.y-5,b.w,20),'Cards',FONT_MENU,1),draw_box(gsize,0,1)
		const sbar=scrollbar(gsize,max(0,ch-(gsize.h-2)),10,gsize.h,ms.grid.scroll,ch>=gsize.h,0),bb=sbar.size;ms.grid.scroll=sbar.scroll,bb.y++
		const oc=frame.clip;frame.clip=bb;for(let z=0;z<count(deck.cards);z++){
			const c=rect(bb.x,bb.y+(z*slot)-ms.grid.scroll,bb.w,slot), card=deck.cards.v[z]
			if(c.y>bb.y+bb.h||c.y+c.h<bb.y)continue; const cb=rclip(c,bb) // coarse clip
			const p=rect(c.x+2,c.y+1,40,28), t=rect(p.x+p.w+5,p.y,bb.w-(2+p.w+5+5),font_h(FONT_MENU)), s=rect(t.x,t.y+t.h+2,t.w,font_h(FONT_BODY))
			const desc=has_parent(card)?`child of '${ls(ifield(ifield(card,'parent'),'name'))}'`:`${count(card.widgets)} widgets`
			if(ev.md&&dover(cb)){m=1,n_go([card],deck),curr=card,ms.grid.row=z}if(ev.dclick&&over(cb))props=1
			const col=ev.drag&&ms.grid.row==z?13:1
			draw_text_fit(t,ls(ifield(card,'name')),FONT_MENU,col),draw_text_fit(s,desc,FONT_BODY,col),draw_box(p,0,col)
			if(card==curr&&col==1)draw_invert(pal,c);draw_thumbnail(card,p)
			if((ev.drag||ev.mu)&&ms.grid.row!=-1){
				{const g=rect(c.x,c.y-3    ,c.w,7);if(over(g)){draw_hline(c.x,c.x+c.w,c.y      ,13),gutter=z  }}
				{const g=rect(c.x,c.y-3+c.h,c.w,7);if(over(g)){draw_hline(c.x,c.x+c.w,c.y+c.h-1,13),gutter=z+1}}
			}
		};frame.clip=oc
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',1)||ev.exit||ev.action)modal_exit(0)
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'New',1)){
			const c=deck_add(deck,lms('card')),n=ln(ifield(curr,'index'))
			iwrite(c,lms('index'),lmn(n+1)),m=1,n_go([c],deck)
		}c.x+=65
		if(ui_button(rect(c.x,c.y,80,20),'New Child',1)){
			const c=deck_add(deck,lms('card'),ifield(curr,'name'),curr),n=ln(ifield(curr,"index"))
			iwrite(c,lms('index'),lmn(n+1)),m=1,n_go([c],deck)
		}
		if(ev.mu){
			if(ms.grid.row!=-1&&gutter!=-1){
				const s=deck.cards.v[ms.grid.row], oi=ln(ifield(s,'index'))
				iwrite(s,lms('index'),lmn(gutter>oi?gutter-1:gutter)),m=1,n_go([s],deck)
			}ms.grid.row=-1
		}
		else if(ev.drag&&ms.grid.row!=-1){const r=rect(ev.pos.x-5,ev.pos.y-5,10,10);draw_rect(r,0),draw_box(r,0,1),uicursor=cursor.drag}
		else if(ev.dir=='up'  &&ev.shift){iwrite(curr,lms('index'),lmn(ln(ifield(curr,'index'))-1)),m=1,n_go([curr],deck)}
		else if(ev.dir=='down'&&ev.shift){iwrite(curr,lms('index'),lmn(ln(ifield(curr,'index'))+1)),m=1,n_go([curr],deck)}
		else if(ev.dir=='left' ||ev.dir=='up'  ){m=1,n_go([lms('Prev')],deck)}
		else if(ev.dir=='right'||ev.dir=='down'){m=1,n_go([lms('Next')],deck)}
		if(m){
			curr=ifield(deck,'card');const y=(ln(ifield(curr,'index'))*slot)-ms.grid.scroll
			if(y<0){ms.grid.scroll+=y}if(y+slot>=bb.h){ms.grid.scroll+=(y+slot)-bb.h}
		}if(props)modal_enter('card_props')
	}
	else if(ms.type=='sounds'){
		const b=draw_modalbox(rect(250,frame.size.y-16-30)), gsize=rect(b.x,b.y+15,b.w,b.h-16-(2*25))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Sounds',FONT_MENU,1)
		const s=ms.grid.row>=0?dget(deck.sounds,ms.grid.table.v.name[ms.grid.row]):null
		if(ui_table(gsize,[16,130],'Isss',ms.grid))n_play([s])
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',1)||ev.exit){
			if(ms.from_action){
				if(ms.grid.row>=0)ms.message=deck.sounds.k[ms.grid.row]
				ms.grid=gridtab(transit_enumerate(),ms.act_transno),ms.type='action',ms.from_action=0
			}else{modal_exit(1)}
		}
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Edit...',s!=null))au.target=s,modal_enter('recording')
		if(ui_button(rect(c.x,c.y-25,60,20),'New...',1))au.target=deck_add(deck,lms('sound')),mark_dirty(),modal_enter('recording');c.x+=65
		if(ui_button(rect(c.x,c.y,60,20),"Delete",s!=null))deck_remove(deck,s),mark_dirty(),ms.grid=gridtab(sounds_enumerate())
	}
	else if(ms.type=='recording'){
		const b=draw_modalbox(rect(frame.size.x-50,130)), samples=max(1,ln(ifield(au.target,'size')))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Audio Editor',FONT_MENU,1)
		au.head=clamp(0,au.head,samples), au.sel.x=clamp(0,au.sel.x,samples), au.sel.y=clamp(0,au.sel.y,samples)
		const gsize=rect(b.x,b.y+15,b.w,64), lsize=rint(rect(b.x,gsize.y+gsize.h+2,b.w/2,font_h(FONT_BODY)))
		const sndpos=x=>0|(x*((gsize.w*1.0)/samples))
		const possnd=x=>0|max(0,min(samples-1,((x/gsize.w)*samples)))
		if((ev.mu||ev.drag)&&dover(gsize)&&au.mode=='stopped'){
			const x=possnd(ev.dpos.x-gsize.x),y=possnd(ev.pos.x-gsize.x);field_exit()
			if(ev.mu)au.head=y;if(ev.drag)au.sel=rect(min(x,y),max(x,y)),au.head=au.sel.x
		}
		const sel=au.sel, sc=sel.y-sel.x, oc=frame.clip
		frame.clip=gsize;for(let z=0;z<gsize.w;z++){
			let v=0,base=possnd(z);for(let d=-1;d<=1;d++){
				if(base+d<0||base+d>=samples)continue;let s=byte_to_sample(au.target.data[base+d]);if(Math.abs(s)>Math.abs(v))v=s
			}let vp=0|(v*64);draw_vline(gsize.x+z,gsize.y+32,gsize.y+32+vp,1),draw_vline(gsize.x+z,gsize.y+32-vp,gsize.y+32,1)
		}frame.clip=oc
		if(sc)draw_invert(pal,rect(gsize.x+sndpos(sel.x),gsize.y,sndpos(sel.y)-sndpos(sel.x),gsize.h))
		draw_invert(pal,rect(gsize.x+sndpos(au.head)-1,gsize.y,3,gsize.h)),draw_box(gsize,0,1)
		const t=sc==0?ls(dyad.format(lms('%0.2fkb, %0.2fs'         ),lml([lmn(samples/1000.0*1.33),ifield(au.target,'duration')]))):
		              ls(dyad.format(lms('%0.2fkb, %0.2fs selected'),lml([lmn(sc     /1000.0*1.33),lmn(sc/SFX_RATE            )])))
		draw_text_fit(lsize,t,FONT_BODY,1)
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',1)||ev.exit)modal_exit(0)
		const c=rect(b.x,b.y+b.h-20)
		if(ui_toggle(rect(c.x,c.y,60,20),'Play',au.mode=='playing',1)){
			if(au.mode=='recording')sound_finish()
			au.head=sc?au.sel.x:0;if(au.mode=='playing'){stop_sound_pump()}else{play_sound_pump(sound_selected())}
		};c.x+=65
		if(ui_toggle(rect(c.x,c.y,60,20),'Record',au.mode=='recording',!au.norecord)){
			if(au.mode=='recording'){sound_finish()}else{sound_record()}
		};c.x+=65
		if(ui_button(rect(c.x,c.y,60,20),'Crop',sc&&au.mode=='stopped'))sound_edit(sound_slice(au.sel)),au.sel=rect(),au.head=0
		draw_text(rect(b.x+(b.w/2),gsize.y+gsize.h+9,37,20),'Name',FONT_MENU,1)
		ui_field(rint(rect(b.x+(b.w/2)+37,gsize.y+gsize.h+5,(b.w/2)-37,20)),ms.name)
	}
	else if(ms.type=='fonts'){
		const b=draw_modalbox(rect(170,170))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Fonts',FONT_MENU,1)
		const gsize=rect(b.x,b.y+15,b.w,b.h-20-50-25), choose=ui_table(gsize,[16],'Is',ms.grid)
		let psize=rect(b.x,gsize.y+gsize.h+5,b.w,50);draw_box(psize,0,1),psize=inset(psize,2)
		if(ms.grid.row>=0){
			const l=layout_plaintext(PANGRAM,deck.fonts.v[ms.grid.row],ALIGN.left,rect(psize.w,psize.h));draw_text_wrap(psize,l,1)}
		const c=rect(b.x+b.w-60,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'OK',ms.grid.row>=0)||choose){
			const nf=ms.grid.table.v.name[ms.grid.row];modal_exit(1)
			if(uimode=='object'){ob_edit_prop('font',nf)}
			else if(wid.ft&&wid.cursor.x!=wid.cursor.y){const c=wid.cursor;field_stylespan(nf,lms('')),wid.cursor=c,mark_dirty()}
			else if(wid.ft){iwrite(wid.ft,lms('font'),nf),wid.f=unpack_field(ms.old_wid.ft),wid.fv=unpack_field_value(ms.old_wid.ft),mark_dirty()}
		};c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='resources'){
		const b=draw_modalbox(rect(280,190))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Font/Deck Accessory Mover',FONT_MENU,1)
		const lgrid=rect(b.x            ,b.y+15 ,100    ,b.h-(15+15+5+20))
		const rgrid=rect(b.x+b.w-lgrid.w,lgrid.y,lgrid.w,lgrid.h         )
		if(ui_button(rect(rgrid.x+(rgrid.w-80)/2,b.y+b.h-20,80,20),'OK',1)||ev.exit)modal_exit(0)
		ui_table(lgrid,[16,lgrid.w-38],'Is',ms.grid );if(ms.grid .row>-1)ms.grid2.row=-1
		ui_table(rgrid,[16,rgrid.w-38],'Is',ms.grid2);if(ms.grid2.row>-1)ms.grid .row=-1
		draw_vline(lgrid.x+lgrid.w,lgrid.y+lgrid.h+5,b.y+b.h,18)
		draw_vline(rgrid.x        ,rgrid.y+rgrid.h+5,b.y+b.h,18)
		draw_textc(rect(lgrid.x,lgrid.y+lgrid.h+3,lgrid.w,15),ms.message?ls(ifield(ms.message,'name')):'(Choose a Deck)',FONT_BODY,1)
		draw_textc(rect(rgrid.x,rgrid.y+rgrid.h+3,rgrid.w,15),ls(ifield(deck,'name')),FONT_BODY,1)
		const cb=rect(lgrid.x+lgrid.w+5,lgrid.y+5,b.w-(lgrid.w+5+5+rgrid.w),20)
		const rvalue=(g,k)=>g.table.v[k][g.row]
		const sel=(ms.grid.table&&ms.grid.row>-1)?rvalue(ms.grid,'value'): ms.grid2.row>-1?rvalue(ms.grid2,'value'): null
		if(ui_button(cb,'>> Copy >>',ms.grid.row>-1)){
			if(patterns_is(sel)){const dst=ifield(deck,'patterns');for(let z=2;z<=31;z++)iindex(dst,z,iindex(sel,z))}
			else{deck_add(deck,sel,rvalue(ms.grid,'name'))}
			ms.grid2=gridtab(res_enumerate(deck)),mark_dirty();if(module_is(sel))validate_modules()
		}cb.y+=25
		if(ui_button(cb,'Remove',ms.grid2.row>-1)){
			if(patterns_is(sel)){deck.patterns=patterns_read(lmd())}
			else{deck_remove(deck,rvalue(ms.grid2,'value'))}
			ms.grid2=gridtab(res_enumerate(deck)),mark_dirty()
		}cb.y+=25
		const pre=rect(cb.x,cb.y,cb.w,b.h-(cb.y-b.y))
		if(sel&&font_is(sel)){const l=layout_plaintext(PANGRAM,sel,ALIGN.center,rect(pre.w,pre.h));draw_text_wrap(pre,l,1)}
		if(sel&&module_is(sel)){const l=layout_plaintext(ls(ifield(sel,'description')),FONT_BODY,ALIGN.center,rect(pre.w,pre.h));draw_text_wrap(pre,l,1)}
		if(sel&&sound_is(sel)){if(ui_button(cb,'Play',1))n_play([sel])}
		if(sel&&patterns_is(sel)){
			const c=frame.clip,pal=sel.pal.pix;frame.clip=pre;
			const anim_ants   =(x,y)=>(0|((x+y+(0|(frame_count/2)))/3))%2?15:0
			const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
			const draw_color  =(pix,x,y)=>pix==ANTS?anim_ants(x,y): pix>47?0: pix>31?pix-32: draw_pattern(pix,x,y)?15:0
			for(let z=0;z<32;z++)for(let y=0;y<16;y++)for(let x=0;x<16;x++){
				const h=rect(3+x+pre.x+16*(z%(0|(pre.w/16))), y+pre.y+16*(0|(z/(0|(pre.w/16)))))
				if(inclip(h))pix(h,32+draw_color(z,x,y))
			}frame.clip=c
		}
		ui_button(rint(rect(lgrid.x+(lgrid.w-80)/2,b.y+b.h-20,80,20)),'Choose...',1,_=>open_text('.html,.deck',text=>{
			ms.message=deck_read(text),ms.grid=gridtab(res_enumerate(ms.message))
		}))
	}
	else if(ms.type=='query'){
		const b=draw_modalbox(rect(frame.size.x-30,frame.size.y-16-30)),t=ms.grid.table
		const desc=t?`${Object.keys(t.v).length} column${Object.keys(t.v).length==1?'':'s'}, ${count(t)} row${count(t)==1?'':'s'}.`:'Executing Query.'
		let compiles=0,error=' ';try{parse(ls(rtext_string(ms.text.table))),compiles=1}catch(e){error=e.x}
		const dsize=font_textsize(FONT_BODY,desc);draw_text(b,desc,FONT_BODY,1)
		const msize=font_textsize(FONT_BODY,error), gsize=rint(rect(b.x,b.y+dsize.y,b.w,(b.h-(2*5)-dsize.y-20)/2))
		const esize=rect(b.x,gsize.y+gsize.h+5,b.w,gsize.h-msize.y);ui_codeedit(esize,1,ms.text)
		if(!compiles)draw_text_fit(rect(b.x,esize.y+esize.h,b.w,msize.y),error,FONT_BODY,1)
		const c=rect(b.x+b.w-60,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Run',compiles)||ev.eval){
			try{
				const prog=parse(ls(rtext_string(ms.text.table)))
				blk_opa(prog,op.BUND,1),blk_lit(prog,lmnat(n_post_query)),blk_op(prog,op.SWAP),blk_op(prog,op.CALL)
				fire_hunk_async(ms.old_wid.gt,prog),ms.grid=gridtab(null)
			}catch(e){console.log(e)}
		}c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Apply',ms.grid.table!=null&&ms.grid.table!=ms.old_wid.gv.table&&!ms.old_wid.g.locked)){
			const t=ms.grid.table;modal_exit(0),grid_edit(t)
			listen_show(ALIGN.right,1,lms('applied table query:')),listen_show(ALIGN.left,1,rtext_string(ms.text.table))
		}c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Close',1)||ev.exit)modal_exit(0)
		if(ms.grid.table){widget_grid(null,{size:gsize,font:FONT_MONO,widths:[],format:'',headers:1,scrollbar:1,lines:1,show:'solid',locked:1},ms.grid)}
		else{draw_box(gsize,0,1),draw_textc(gsize,'Working...',FONT_BODY,1)}
	}
	else if(ms.type=='url'){
		const b=draw_modalbox(rect(170,90))
		draw_textc(rect(b.x,b.y,b.w,20),'Do you wish to open this URL?',FONT_BODY,1)
		ui_textedit(rect(b.x,b.y+20+5,b.w,40),1,ms.text)
		const c=rect(b.x+b.w-(b.w-(2*60+5))/2-60,b.y+b.h-20)
		ui_button(rect(c.x,c.y,60,20),'Open',1,_=>{open_url(ls(rtext_string(ms.text.table)));modal_exit(0)});c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='link'){
		const b=draw_modalbox(rect(170,70))
		draw_textc(rect(b.x,b.y,b.w,20),'Enter a link string for\nthe selected text span:',FONT_BODY,1)
		ui_field(rect(b.x,b.y+20+5,b.w,20),ms.text)
		const c=rint(rect(b.x+b.w-(b.w-(2*60+5))/2-60,b.y+b.h-20))
		if(ui_button(rect(c.x,c.y,60,20),'OK',1)){
			const l=rtext_string(ms.text.table);modal_exit(0)
			const c=rcopy(wid.cursor);field_stylespan(lms(''),l),wid.cursor=c
		};c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='gridcell'){
		const b=draw_modalbox(rect(170,70))
		draw_textc(rect(b.x,b.y,b.w,20),'Enter a new value for\nthe selected cell:',FONT_BODY,1)
		ui_field(rect(b.x,b.y+20+5,b.w,20),ms.text)
		const c=rect(b.x+b.w-(b.w-(2*60+5))/2-60,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'OK',1))modal_exit(1);c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='save'){
		const l=layout_plaintext(ms.desc,FONT_BODY,ALIGN.center,rect(250,100))
		const b=draw_modalbox(radd(l.size,rect(0,20+5+20+5+20))), tbox=rect(b.x,b.y+20,b.w,l.size.y)
		draw_textc(rect(b.x,b.y-5,b.w,20),'Save File',FONT_MENU,1),draw_text_wrap(tbox,l,1)
		draw_text(rect(b.x   ,tbox.y+tbox.h+7,    56,20),'Filename',FONT_MENU,1)
		ui_field (rect(b.x+56,tbox.y+tbox.h+5,b.w-56,18),ms.text)
		const c=rect(b.x+b.w-60,b.y+b.h-20),subtype=ms.subtype
		const grid=ms.old_wid.gv, format=subtype!='export_table'?null:lms(ms.old_wid.g.format.length?ms.old_wid.g.format:'s'.repeat(Object.keys(grid.table.v).length))
		if(ui_button(rect(c.x,c.y,60,20),'Save',1,_=>{
			const name=ls(rtext_string(ms.text.table))
			const savedeck=_=>{
				let d=deck_write(deck)
				if(/\.html$/i.test(name)){q('script[language="decker"]').innerHTML='\n'+d,d=q('body').innerHTML}
				dirty=0,save_text(name,d)
			}
			const save_image=_=>{
				if(bg_has_sel()){const s=rcopy(dr.sel_here);bg_end_selection(),dr.sel_here=s}
				let i=draw_card(ifield(deck,'card'),1),off=rect(),f=1,a=0, bg=dr.trans?0:32, frames=[]
				const anim=deck.patterns.anim
				const anim_pattern=(pix,x,y,f)=>pix<28||pix>31?pix: anim[pix-28][f%max(1,anim[pix-28].length)]
				const draw_pattern=(pix,x,y  )=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal[(x%8)+(8*(y%8))+(8*8*pix)]&1
				if(bg_has_sel()||bg_has_lasso())i=image_copy(i,dr.sel_here),off=rcopy(dr.sel_here)
				for(let z=0;z<4&&dr.show_anim;z++){const c=anim[z].length;if(c)f=lcm(f,c)}
				for(let z=0;z<f;z++){
					const frame=image_copy(i)
					for(let y=0;y<i.size.y;y++)for(let x=0;x<i.size.x;x++){
						const v=frame.pix[x+(i.size.x*y)];if(v>=28&&v<=31)a=1
						const c=anim_pattern(v,x,y,z),p=draw_pattern(c,x+off.x,y+off.y)
						frame.pix[x+(i.size.x*y)]=c>=32?c: p?1:bg
					}frames.push(bg_has_lasso()?image_mask(frame,dr.mask):frame)
				}save_bin(name,writegif(a?frames:[frames[0]]))
			}
			if(subtype=='save_deck'    )savedeck()
			if(subtype=='save_locked'  )iwrite(deck,lms('locked'),ONE),savedeck(),iwrite(deck,lms('locked'),NONE)
			if(subtype=='export_script')save_text(name,ls(rtext_string(sc.f.table)))
			if(subtype=='export_table' )field_exit(),save_text(name,ls(n_writecsv([grid.table,format])))
			if(subtype=='export_image' )save_image()
			if(subtype=='save_lil'){
				let x=ms.verb;ret(ONE);if(image_is(x))x=lml([x])
				if(lil(x)&&ll(x).some(image_is)&&count(x)>0){save_bin(name,writegif(ll(x).filter(image_is)))}
				else if(sound_is(x))                        {save_bin(name,writewav(x))}
				else if(array_is(x))                        {save_bin(name,writearray(x))}
				else                                        {save_text(name,ls(x))}
			}
		}))modal_exit(1);c.x-=65;
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='alert'){
		const b=draw_modal_rtext(rect(0,5+20))
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='confirm'){
		const b=draw_modal_rtext(rect(0,5+20)),v=ms.verb?ls(ms.verb):'OK'
		const vs=font_textsize(FONT_MENU,v);vs.x=min(max(60,vs.x+10),200-65);const c=rect(b.x+b.w-vs.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,vs.x,20),v,1))modal_exit(1);c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='input'){
		const b=draw_modal_rtext(rect(0,5+20+5+20))
		ui_field(rect(b.x,b.y+b.h-(20+5+20),b.w,20),ms.text)
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='choose_lil'){
		const b=draw_modal_rtext(rect(0,5+60+5+20))
		const choose=ui_table(rect(b.x,b.y+b.h-(20+5+60),b.w,60),[],'s',ms.grid)
		if(ui_button(rect(b.x+b.w-60,b.y+b.h-20,60,20),'OK',ms.grid.row>=0)||choose)modal_exit(1)
	}
	else if(ms.type=='open_lil'){
		const b=draw_modalbox(rect(125,45))
		draw_textc(rect(b.x,b.y,b.w,16),'Click to open a file:',FONT_BODY,1)
		const c=rect(b.x+b.w-60,b.y+b.h-20)
		ui_button(rect(c.x,c.y,60,20),'Open',1,_=>{
			if     (ls(ms.verb)=='array'  )open_file(ms.filter,file=>{load_array(file,        array=>{arg(),ret(array)    ,modal_exit(1)})})
			else if(ms.filter=='image/*'  )open_file(ms.filter,file=>{load_image(file,ms.verb,image=>{arg(),ret(image)    ,modal_exit(1)})})
			else if(ms.filter=='audio/*'  )open_file(ms.filter,file=>{load_sound(file,        sound=>{arg(),ret(sound)    ,modal_exit(1)})})
			else if(ms.filter=='.csv,.txt')open_text(ms.filter,                               text =>{arg(),ret(lms(text)),modal_exit(1)  })
			else                           open_text(''       ,                               text =>{arg(),ret(lms(text)),modal_exit(1)  })
		}),c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
	}
	else if(ms.type=='brush'){
		const grid=rect(6,4), ss=25, gs=ss+4, m=5, lh=font_h(FONT_BODY)
		const b=draw_modalbox(rect(m+(grid.x*gs)+m,m+(grid.y*gs)+lh+m))
		draw_textc(rect(b.x,b.y+b.h-lh,b.w,lh),'Choose a brush shape.',FONT_BODY,1)
		for(let z=0;z<grid.x*grid.y;z++){
			const s=rect(b.x+m+2+gs*(z%grid.x),b.y+m+2+gs*(0|(z/grid.x)),ss,ss);
			const c=rint(rect(s.x+s.w/2,s.y+s.h/2));draw_line(rpair(c,c),z,1)
			if(z==dr.brush)draw_box(inset(s,-2),0,1)
			const a=dover(s)&&over(s), cs=(z==dr.brush&&ev.action), cl=cs||((ev.md||ev.drag)&&a), cr=cs||(ev.mu&&a)
			if(cl)draw_invert(pal,inset(s,-1)); if(cr){dr.brush=z,modal_exit(z);break}
		}
		if(ev.exit||(ev.mu&&!dover(b)&&!over(b)))modal_exit(-1),ev.mu=0
		if(ev.dir=='left' )dr.brush=((0|(dr.brush/grid.x))*grid.x)+((dr.brush+grid.x-1)%grid.x)
		if(ev.dir=='right')dr.brush=((0|(dr.brush/grid.x))*grid.x)+((dr.brush+       1)%grid.x)
		if(ev.dir=='up'   )dr.brush=(dr.brush+(grid.x*(grid.y-1)))%(grid.x*grid.y)
		if(ev.dir=='down' )dr.brush=(dr.brush+grid.x             )%(grid.x*grid.y)
	}
	else if(ms.type=='pattern'||ms.type=='fill'){
		const grid=rect(8,4), ss=25, gs=ss+4, m=5, lh=font_h(FONT_BODY), v=ms.type=='pattern'?dr.pattern:dr.fill
		const setv=x=>ms.type=='pattern'?dr.pattern=x:dr.fill=x
		const b=draw_modalbox(rect(m+(grid.x*gs)+m,m+(grid.y*gs)+lh+m))
		draw_textc(rect(b.x,b.y+b.h-lh,b.w,lh),`Choose a ${ms.type=='fill'?'fill':'stroke'} pattern.`,FONT_BODY,1)
		for(let z=0;z<grid.x*grid.y;z++){
			const s=rint(rect(b.x+m+2+gs*(z%grid.x),b.y+m+2+gs*(0|(z/grid.x)),ss,ss))
			draw_rect(s,z); if(z==v)draw_box(inset(s,-2),0,1)
			const a=dover(s)&&over(s), cs=(z==v&&ev.action), cl=cs||((ev.md||ev.drag)&&a), cr=cs||(ev.mu&&a)
			if(cl)draw_invert(pal,inset(s,-1)); if(cr){setv(z),modal_exit(z);break}
		}
		if(ev.exit||(ev.mu&&!dover(b)&&!over(b)))modal_exit(-1),ev.mu=0
		if(ev.dir=='left' )setv(((0|(v/grid.x))*grid.x)+((v+grid.x-1)%grid.x))
		if(ev.dir=='right')setv(((0|(v/grid.x))*grid.x)+((v+       1)%grid.x))
		if(ev.dir=='up'   )setv((v+(grid.x*(grid.y-1)))%(grid.x*grid.y))
		if(ev.dir=='down' )setv((v+grid.x             )%(grid.x*grid.y))
	}
	else if(ms.type=='grid'){
		const b=draw_modalbox(rect(120,100))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Grid Size',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Width' ,FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,42,20),'Height',FONT_MENU,1)
		ui_field(rect(b.x+42,b.y+20,b.w-42,18),ms.name)
		ui_field(rect(b.x+42,b.y+40,b.w-42,18),ms.text)
		dr.grid_size.x=ln(rtext_string(ms.name.table)),dr.grid_size.x=max(1,dr.grid_size.x)
		dr.grid_size.y=ln(rtext_string(ms.text.table)),dr.grid_size.y=max(1,dr.grid_size.y)
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='deck_props'){
		const b=draw_modalbox(rect(220,100))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Deck Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Name',FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,42,20),'Author',FONT_MENU,1)
		ui_field(rect(b.x+42,b.y+20,b.w-42,18),ms.name)
		ui_field(rect(b.x+42,b.y+40,b.w-42,18),ms.text)
		iwrite(deck,lms('name'  ),rtext_string(ms.name.table))
		iwrite(deck,lms('author'),rtext_string(ms.text.table)),mark_dirty()
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(deck),modal_exit(0);c.x+=65
		if(ui_button(rect(c.x,c.y,60,20),'Protect...',1))modal_enter('save_locked')
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='card_props'){
		const b=draw_modalbox(rect(220,100)),card=ifield(deck,'card')
		draw_textc(rect(b.x,b.y-5,b.w,20),'Card Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Name',FONT_MENU,1),ui_field(rect(b.x+42,b.y+20,b.w-42,18),ms.name)
		const parent=ifield(card,'parent');if(card_is(parent)){
			const t=`This card is a child of \"${ls(ifield(parent,'name'))}\"`
			const l=layout_plaintext(t,FONT_BODY,ALIGN.left,rect(b.w,30));draw_text_wrap(rect(b.x,b.y+45,b.w,30),l,1)
		}
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(card),modal_exit(0)
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='button_props'){
		const b=draw_modalbox(rect(220,170)),button=ob.sel[0]
		draw_textc(rect(b.x,b.y-5,b.w,20),'Button Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Name',FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,42,20),'Text',FONT_MENU,1)
		ui_field(rect(b.x+42,b.y+20,b.w-42,18),ms.name)
		ui_field(rect(b.x+42,b.y+40,b.w-42,18),ms.text)
		iwrite(button,lms('name'),rtext_string(ms.name.table))
		iwrite(button,lms('text'),rtext_string(ms.text.table)),mark_dirty()
		const style=ls(ifield(button,'style')), sb=rect(b.x,b.y+70)
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Round'    ,1,style=='round'    )){iwrite(button,lms('style'),lms('round'    )),mark_dirty()}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Rectangle',1,style=='rect'     )){iwrite(button,lms('style'),lms('rect'     )),mark_dirty()}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Checkbox' ,1,style=='check'    )){iwrite(button,lms('style'),lms('check'    )),mark_dirty()}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Invisible',1,style=='invisible')){iwrite(button,lms('style'),lms('invisible')),mark_dirty()}sb.y+=16
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(button),modal_exit(0); c.x+=65
		if(ui_button(rect(c.x,c.y,60,20),'Action...',1))modal_enter('action')
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='field_props'){
		const b=draw_modalbox(rect(220,170+60)),f=ob.sel[0], p=unpack_field(f)
		draw_textc(rect(b.x,b.y-5,b.w,20),'Field Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Name',FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,42,60),'Text',FONT_MENU,1)
		ui_field(rect(b.x+42,b.y+20,b.w-42,18),ms.name)
		widget_field(null,{size:rect(b.x+42,b.y+40,b.w-42,58),font:p.font,show:'solid',scrollbar:1,border:1,style:'plain',align:p.align,locked:0},ms.text)
		iwrite(f,lms('name' ),rtext_string(ms.name.table))
		iwrite(f,lms('value'),ms.text.table),mark_dirty()
		let border=lb(ifield(f,'border')), scrollbar=lb(ifield(f,"scrollbar")), cb=rect(b.x,b.y+50+60)
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Border'   ,1,border   )){border   ^=1,iwrite(f,lms('border'   ),lmn(border   )),mark_dirty()}cb.y+=16
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Scrollbar',1,scrollbar)){scrollbar^=1,iwrite(f,lms('scrollbar'),lmn(scrollbar)),mark_dirty()}cb.y+=16
		const style=ls(ifield(f,'style')), sb=rect(b.x,cb.y+10);let cp=0
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Rich Text' ,1,style=='rich' )){iwrite(f,lms('style'),lms('rich' )),mark_dirty()     }sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Plain Text',1,style=='plain')){iwrite(f,lms('style'),lms('plain')),mark_dirty(),cp=1}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Code'      ,1,style=='code' )){iwrite(f,lms('style'),lms('code' )),mark_dirty(),cp=1}sb.y+=16
		if(cp&&!rtext_is_plain(ms.text.table))ms.text.table=rtext_cast(rtext_string(ms.text.table))
		const align=ls(ifield(f,'align')), ab=rect(b.x+(b.w/2),cb.y+10)
		if(ui_radio(rint(rect(ab.x,ab.y,b.w/2,16)),'Align Left' ,1,align=='left'  )){iwrite(f,lms('align'),lms('left'  )),mark_dirty()}ab.y+=16
		if(ui_radio(rint(rect(ab.x,ab.y,b.w/2,16)),'Center'     ,1,align=='center')){iwrite(f,lms('align'),lms('center')),mark_dirty()}ab.y+=16
		if(ui_radio(rint(rect(ab.x,ab.y,b.w/2,16)),'Align Right',1,align=='right' )){iwrite(f,lms('align'),lms('right' )),mark_dirty()}ab.y+=16
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(f),modal_exit(0)
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='slider_props'){
		const b=draw_modalbox(rect(220,170)),f=ob.sel[0]
		draw_textc(rect(b.x,b.y-5,b.w,20),'Slider Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Name'  ,FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,42,20),'Format',FONT_MENU,1)
		ui_field(rect(b.x+50,b.y+20,b.w-50,18),ms.name)
		ui_field(rect(b.x+50,b.y+40,b.w-50,18),ms.text)
		iwrite(f,lms('name'  ),rtext_string(ms.name.table))
		iwrite(f,lms('format'),rtext_string(ms.text.table)),mark_dirty()
		const style=ls(ifield(f,'style')), sb=rect(b.x,b.y+70)
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Horizontal',1,style=='horiz'  )){iwrite(f,lms('style'),lms('horiz'  )),mark_dirty()}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Vertical'  ,1,style=='vert'   )){iwrite(f,lms('style'),lms('vert'   )),mark_dirty()}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Bar'       ,1,style=='bar'    )){iwrite(f,lms('style'),lms('bar'    )),mark_dirty()}sb.y+=16
		if(ui_radio(rint(rect(sb.x,sb.y,b.w/2,16)),'Compact'   ,1,style=='compact')){iwrite(f,lms('style'),lms('compact')),mark_dirty()}sb.y+=16
		const ib=rint(rect(b.x+b.w/2,b.y+70))
		draw_text(rect(ib.x+5,ib.y+2,40,20),'Min' ,FONT_MENU,1),ui_field(rint(rect(ib.x+40,ib.y,b.w/2-40,18)),ms.form0),ib.y+=20
		draw_text(rect(ib.x+5,ib.y+2,40,20),'Max' ,FONT_MENU,1),ui_field(rint(rect(ib.x+40,ib.y,b.w/2-40,18)),ms.form1),ib.y+=20
		draw_text(rect(ib.x+5,ib.y+2,40,20),'Step',FONT_MENU,1),ui_field(rint(rect(ib.x+40,ib.y,b.w/2-40,18)),ms.form2),ib.y+=20
		const c=rect(b.x,b.y+b.h-20)
		iwrite(f,lms('interval'),lml([rtext_string(ms.form0.table),rtext_string(ms.form1.table)]))
		iwrite(f,lms('step'),rtext_string(ms.form2.table)),mark_dirty()
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(f),modal_exit(0)
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='canvas_props'){
		const b=draw_modalbox(rect(220,141)),canvas=ob.sel[0]
		draw_textc(rect(b.x,b.y-5,b.w,20),'Canvas Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,42,20),'Name' ,FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,42,20),'Scale',FONT_MENU,1)
		ui_field(rect(b.x+42,b.y+20,b.w-42,18),ms.name)
		ui_field(rect(b.x+42,b.y+40,b.w-42,18),ms.text)
		iwrite(canvas,lms('name' ),rtext_string(ms.name.table))
		iwrite(canvas,lms('scale'),rtext_string(ms.text.table)),mark_dirty()
		let border=lb(ifield(canvas,'border')),draggable=lb(ifield(canvas,'draggable')),cb=rect(b.x,b.y+50+20)
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Border'   ,1,border   ))border   ^=1,iwrite(canvas,lms('border'   ),lmn(border   )),mark_dirty();cb.y+=16
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Draggable',1,draggable))draggable^=1,iwrite(canvas,lms('draggable'),lmn(draggable)),mark_dirty()
		const c=rect(b.x,b.y+b.h-20)
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(canvas),modal_exit(0)
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='grid_props'){
		const b=draw_modalbox(rect(220,140)),grid=ob.sel[0]
		draw_textc(rect(b.x,b.y-5,b.w,20),'Grid Properties',FONT_MENU,1)
		draw_text(rect(b.x,b.y+22,47,20),'Name'  ,FONT_MENU,1)
		draw_text(rect(b.x,b.y+42,47,20),'Format',FONT_MENU,1)
		ui_field(rect(b.x+47,b.y+20,b.w-47,18),ms.name)
		ui_field(rect(b.x+47,b.y+40,b.w-47,18),ms.text)
		iwrite(grid,lms('name'  ),rtext_string(ms.name.table))
		iwrite(grid,lms('format'),rtext_string(ms.text.table)),mark_dirty()
		let headers=lb(ifield(grid,'headers')), scrollbar=lb(ifield(grid,'scrollbar')), lines=lb(ifield(grid,'lines')), cb=rect(b.x,b.y+70)
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Column Headers',1,headers  )){headers  ^=1,iwrite(grid,lms('headers'  ),lmn(headers  )),mark_dirty()}cb.y+=16
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Scrollbar'     ,1,scrollbar)){scrollbar^=1,iwrite(grid,lms('scrollbar'),lmn(scrollbar)),mark_dirty()}cb.y+=16
		if(ui_checkbox(rect(cb.x,cb.y,b.w,16),'Grid Lines'    ,1,lines    )){lines    ^=1,iwrite(grid,lms('lines'    ),lmn(lines    )),mark_dirty()}cb.y+=16
		const c=rect(b.x,b.y+b.h-20), w=ll(ifield(grid,'widths'))
		if(ui_button(rect(c.x,c.y,60,20),'Script...',1))setscript(grid),modal_exit(0);c.x+=65
		if(ui_button(rect(c.x,c.y,90,20),'Reset Widths',w.length))iwrite(grid,lms('widths'),lml([])),mark_dirty()
		if(ui_button(rect(b.x+b.w-60,c.y,60,20),'OK',1)||ev.exit)modal_exit(1)
	}
	else if(ms.type=='action'){
		const b=draw_modalbox(rect(220,180))
		draw_textc(rect(b.x,b.y-5,b.w,20),'Button Action',FONT_MENU,1)
		const c=rect(b.x+b.w-60,b.y+b.h-20), cr=rect(b.x,b.y+36)
		const ready=(ms.act_go||ms.act_sound)&&(ms.act_go?(ms.act_gomode!=4||count(ms.verb)):1)&&(ms.act_sound?count(ms.message):1)
		if(ui_button(rect(c.x,c.y,60,20),'OK',ready))modal_exit(1);c.x-=65
		if(ui_button(rect(c.x,c.y,60,20),'Cancel',1)||ev.exit)modal_exit(0)
		if(ui_checkbox(rint(rect(b.x,b.y+20,b.w/2,16)),'Go to Card',1,ms.act_go))ms.act_go^=1
		if(ui_radio(rect(cr.x+5,cr.y,80,16),'First'   ,ms.act_go,ms.act_gomode==0))ms.act_gomode=0;cr.y+=16
		if(ui_radio(rect(cr.x+5,cr.y,80,16),'Previous',ms.act_go,ms.act_gomode==1))ms.act_gomode=1;cr.y+=16
		if(ui_radio(rect(cr.x+5,cr.y,80,16),'Next'    ,ms.act_go,ms.act_gomode==2))ms.act_gomode=2;cr.y+=16
		if(ui_radio(rect(cr.x+5,cr.y,80,16),'Last'    ,ms.act_go,ms.act_gomode==3))ms.act_gomode=3;cr.y+=16
		if(ui_radio(rect(cr.x+5,cr.y,45,16),'Pick:'   ,ms.act_go,ms.act_gomode==4))ms.act_gomode=4
		if(ms.act_go&&ms.act_gomode==4){
			const l=rect(cr.x+5+45,cr.y,b.w-5-45-5-60,16)
			draw_hline(l.x,l.x+l.w,l.y+l.h,13),draw_text_fit(inset(l,1),ls(ms.verb),FONT_BODY,1)
			if(ui_button(rect(b.x+b.w-60,cr.y,60,20),'Choose...',ms.act_go&&ms.act_gomode==4))ms.type='pick_card'
		}cr.y+=26;
		if(ui_checkbox(rect(cr.x,cr.y,80,16),'Play a Sound',1,ms.act_sound))ms.act_sound^=1
		if(ms.act_sound){
			const l=rect(cr.x+5+75,cr.y,b.w-5-75-5-60,16)
			draw_hline(l.x,l.x+l.w,l.y+l.h,13),draw_text_fit(inset(l,1),ls(ms.message),FONT_BODY,1)
			if(ui_button(rect(b.x+b.w-60,cr.y,60,20),'Choose...',ms.act_sound)){
				ms.act_transno=ms.grid.row,ms.grid=gridtab(sounds_enumerate()),ms.from_action=1,ms.type='sounds'
			}
		}
		if(ms.act_go){
			if(ui_checkbox(rint(rect(b.x+b.w/2,b.y+20,b.w/2,16)),'With Transition',1,ms.act_trans))ms.act_trans^=1
			if(ms.act_trans)ui_list(rect(b.x+b.w/2,b.y+36,b.w/2,55),ms.grid)
		}
	}
	else if(ms.type=='pick_card'){
		const b=draw_modalbox(rect(220,45))
		draw_textc(rect(b.x,b.y,b.w,16),'Pick a card- any card.',FONT_BODY,1)
		const c=rint(rect(b.x+(b.w-60-5-60-5-60)/2,b.y+b.h-20))
		if(ui_button(rect(c.x,c.y,60,20),'Previous',1)||ev.dir=='left')n_go([lms('Prev')],deck);c.x+=65
		if(ui_button(rect(c.x,c.y,60,20),'Choose',1))ms.verb=ifield(ifield(deck,'card'),'name'),n_go([lmn(ms.act_card)],deck),ms.type='action';c.x+=65
		if(ui_button(rect(c.x,c.y,60,20),'Next',1)||ev.dir=='right')n_go([lms('Next')],deck)
	}
	else if(ms.type=='trans'){
		const now=new Date().getTime()/1000
		const sofar=ms.time_start==-1?0:now-ms.time_start;if(ms.time_start==-1)ms.time_start=now
		let f=frame, tween=min(sofar*(ms.time_end/15),1.0)
		ms.canvas.image.pix=frame.image.pix;frame.image.pix.fill(0)
		const a=lml([ms.canvas,ms.carda,ms.cardb,lmn(tween)]), p=lmblk();blk_lit(p,ms.trans),blk_lit(p,a),blk_op(p,op.CALL)
		const e=lmenv();pushstate(e),issue(e,p);let quota=TRANS_QUOTA;while(quota&&running())runop(),quota--
		if(running()){listen_show(ALIGN.right,1,lms(`warning: transition ${ms.trans.n} exceeded quota and was halted.`)),tween=2}
		popstate(),frame=f,sleep_play=0,sleep_frames=0;if(tween>=1)modal_exit(0)
	}
	ms.in_modal=0
}
n_open=([type,hint])=>{
	modal_enter('open_lil');let t=type?ls(type):'',r=lms('');ms.filter=''
	if(t=='array')r=array_make(0,'u8',0)
	if(t=='sound')ms.filter='audio/*',r=sound_make(new Uint8Array(0))
	if(t=='image')ms.filter='image/*',r=image_make(rect())
	if(t=='text')ms.filter='.csv,.txt'
	ms.verb=t=='array'?lms(t): hint?ls(hint):'';return r
}
n_save=([x])=>{
	modal_enter('save_lil');x=x||NONE
	if(array_is(x)                              )ms.desc='Save a binary file.'    ,ms.text=fieldstr(lms('untitled.bin'))
	if(sound_is(x)                              )ms.desc='Save a .wav sound file.',ms.text=fieldstr(lms('sound.wav'))
	if(image_is(x)||(lil(x)&&x.v.some(image_is)))ms.desc='Save a .gif image file.',ms.text=fieldstr(lms('image.gif'))
	ms.verb=x;return NONE
}
n_alert=([t,p,x,y])=>{
	if(ls(p)=='bool'){modal_enter('confirm_lil'),ms.verb=x?lms(ls(x)):null}
	else if(ls(p)=='string'){modal_enter('input_lil');if(x)ms.text=fieldstr(x)}
	else if(ls(p)=='choose'){
		modal_enter('choose_lil')
		ms.verb=!x?ld(NONE): lil(x)?dyad.dict(x,x): ld(x); if(count(ms.verb)<1)ms.verb=ld(monad.list(NONE))
		ms.grid=gridtab(lt(monad.range(ms.verb)), y?dvix(ms.verb,y):-1)
	}else{modal_enter('alert_lil')}
	ms.message=plain_or_rich(t);return NONE
}
free_canvas=deck=>{ // make a drawing surface that isn't attached to the parent deck, but is aware of its resources:
	const d=deck_read('{deck}\n{card:home}\n{widgets}\nc:{"type":"canvas"}')
	const c=d.cards.v[0], r=c.widgets.v[0]
	iwrite(r,lms('size'),lmpair(deck.size)),d.fonts=deck.fonts,d.patterns=deck.patterns,r.free=1
	canvas_image(r,1);return r
}
go_notify=(deck,x,t,url)=>{
	if(url&&/(http|https|ftp|gopher|gemini):\/\//.test(url))modal_enter('url'),ms.text=fieldstr(lms(url))
	if(ms.type!='trans'&&x>=0&&t&&dget(deck.transit,lms(t))){
		modal_enter('trans'),ms.time_curr=0,ms.time_end=30,ms.time_start=-1;
		ms.trans=dget(deck.transit,lms(t)), ms.canvas=free_canvas(deck)
		ms.carda=draw_card(ifield(deck,'card')), ms.cardb=draw_card(ifield(deck,'cards').v[x])
	}
	const moved=x!=ln(ifield(ifield(deck,'card'),'index'))
	if(moved&&uimode=='interact')msg.pending_loop=1
	if(moved||t){
		grid_exit(),field_exit(),bg_end_selection(),bg_end_lasso(),ob.sel=[],wid.active=ms.type=='listen'?0:-1,mark_dirty()
	}
	if(uimode=='interact')msg.next_view=1
}

// General Purpose Edit History

let doc_hist=[], doc_hist_cursor=0
has_undo=_=>doc_hist_cursor>0
has_redo=_=>doc_hist_cursor<doc_hist.length
undo=_=>{const x=doc_hist[--(doc_hist_cursor)];apply(0,x)}
redo=_=>{const x=doc_hist[(doc_hist_cursor)++];apply(1,x)}
edit=x=>{doc_hist=doc_hist.slice(0,doc_hist_cursor),doc_hist.push(x),redo()}
apply=(fwd,x)=>{
	let card=ifield(deck,'card'), cn=ln(ifield(card,'index')), tn=x.card, t=x.type
	if(cn!=tn)n_go([lmn(tn)],deck),card=ifield(deck,'card')
	const wids=card.widgets
	if(t=='ob_create'){fwd=!fwd,t='ob_destroy'}
	if(t=='bg_block'){
		if(uimode!='draw')setmode('draw')
		const r=x.pos, p=x[fwd?'after':'before']
		let bg=canvas_image(card,1), s=bg.size
		const cs=getpair(ifield(card,'size'))
		if(s.x!=cs.x||s.y!=cs.y){bg=image_resize(bg,cs),iwrite(card,lms('image'),bg),s=cs}
		const cb=x.clr_before, ca=x.clr_after, clip=rect(0,0,s.x,s.y)
		if(fwd&&ca)image_paste(x.clr_pos,clip,ca,bg,1)
		image_paste(r,clip,p,bg,1)
		if(!fwd&&cb)image_paste(x.clr_pos,clip,cb,bg,1)
	}
	else if(t=='ob_props'){
		if(uimode!='object')setmode('object')
		const p=x[fwd?'after':'before']
		Object.keys(p).map(k=>{const d=p[k], w=dget(wids,lms(k));if(w)Object.keys(d).map(i=>iwrite(w,lms(i),d[i]))})
	}
	else if(t=='ob_destroy'){
		if(uimode!='object')setmode('object')
		if(fwd){
			const w=x.props.map(z=>dget(wids,dget(z,lms('name')))).filter(x=>x!=null)
			x.props=card_copy_raw(card,w),w.map(z=>card_remove(card,z))
		}
		else{
			ob.sel=[];const w=card_paste_raw(card,x.props);w.map((z,i)=>{
				dset(x.props[i],lms('name'),ifield(z,'name')),ob.sel.push(z)
				if(!dget(x.props[i],lms('pos')))iwrite(z,lms('pos'),lmpair(rcenter(frame.clip,getpair(ifield(z,'size')))))
			})
		}
	}mark_dirty()
}

// Draw Mode

image_overlay=(dst,src,mask)=>{for(let z=0;z<src.pix.length;z++)if(src.pix[z]!=mask)dst.pix[z]=src.pix[z]}
image_mask=(src,mask)=>{const r=image_copy(src);for(let z=0;z<mask.pix.length;z++)if(!mask.pix[z])r.pix[z]=0;return r}
bg_scratch_clear=_=>dr.scratch.pix.fill(BG_MASK)
bg_scratch=_=>{
	if(!dr.scratch)dr.scratch=image_make(frame.size)
	const s=dr.scratch.size;if(s.x!=frame.size.x||s.y!=frame.size.y)dr.scratch=image_make(frame.size)
	bg_scratch_clear()
}
bg_edit=_=>{
	const s=dr.scratch.size, d=rcopy(s)
	for(let z=0;z<dr.scratch.pix.length;z++){
		if(dr.scratch.pix[z]==BG_MASK)continue;const x=z%s.x, y=0|(z/s.x)
		d.x=min(d.x,x), d.y=min(d.y,y), d.w=max(d.w,x), d.h=max(d.h,y)
	}d.w-=d.x,d.h-=d.y,d.w++,d.h++
	const card=ifield(deck,'card'), back=ifield(card,'image')
	const after=image_copy(back,d);image_overlay(after,image_copy(dr.scratch,d),BG_MASK)
	edit({type:'bg_block',card:ln(ifield(card,'index')),pos:d,before:image_copy(back,d),after})
}
draw_limbo=(clip,scale)=>{
	if(!dr.limbo){/*nothing*/}
	else if(scale&&dr.limbo_dither){draw_rect      (card_to_fat(clip),21)}
	else if(scale                 ){draw_fat_scaled(clip,dr.limbo,!dr.trans,deck.patterns.pal.pix,frame_count,FAT,dr.offset)}
	else if(       dr.limbo_dither){draw_dithered  (clip,dr.limbo,!dr.trans,dr.omask)}
	else                           {draw_scaled    (clip,dr.limbo,!dr.trans)}
}
bg_scaled_limbo=_=>{
	const d=dr.sel_here, card=ifield(deck,'card'), back=ifield(card,'image')
	const r=dr.trans||dr.omask?image_copy(back,d):image_make(rect(d.w,d.h)), s=r.size, t=frame;frame=draw_frame(r)
	if(dr.trans){const c=dr.sel_start;draw_rect(rect(c.x-d.x,c.y-d.y,c.w,c.h),dr.fill)}
	draw_limbo(frame.clip,0),frame=t;return r
}
bg_edit_sel=_=>{
	if(!dr.limbo)return
	const d=rcopy(dr.sel_here), card=ifield(deck,'card'), back=ifield(card,'image')
	const r={type:'bg_block',card:ln(ifield(card,'index')),pos:d,before:image_copy(back,d),after:bg_scaled_limbo()}
	dr.limbo=null,dr.limbo_dither=0
	if(dr.sel_start.w>0||dr.sel_start.h>0){
		const after=image_make(rect(dr.sel_start.w,dr.sel_start.h));for(let z=0;z<after.pix.length;z++)after.pix[z]=dr.fill
		r.clr_pos=rcopy(dr.sel_start), r.clr_before=image_copy(back,dr.sel_start), r.clr_after=after, dr.sel_start=rect()
	}edit(r)
}
bg_copy_selection=s=>image_copy(canvas_image(ifield(deck,'card'),1),s)
bg_scoop_selection=_=>{if(dr.limbo)return;dr.sel_start=rcopy(dr.sel_here);dr.limbo=bg_copy_selection(dr.sel_start),dr.limbo_dither=0}
bg_draw_lasso=(r,show_ants,fill)=>{
	const o=dr.sel_start
	if(dr.omask)for(let a=0;a<o.h;a++)for(let b=0;b<o.w;b++){const h=rect(b+o.x,a+o.y);if(inclip(h)&&dr.omask.pix[b+a*o.w])pix(h,fill)}
	if(dr.mask)for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){const h=rect(b+r.x,a+r.y);if(inclip(h)&&dr.mask.pix[b+a*r.w]){
		const c=show_ants&&ANTS==(0xFF&dr.mask.pix[b+a*r.w]),p=c?ANTS:dr.limbo.pix[b+a*r.w];if(p||!dr.trans)pix(h,p)
	}}
}
bg_lasso_preview=_=>{
	if(!bg_has_lasso())return
	const pos=dr.fatbits?fat_to_card(ev.pos):ev.pos, dpos=dr.fatbits?fat_to_card(ev.dpos):ev.dpos
	const d=rect(pos.x-dpos.x,pos.y-dpos.y), dh=rect(dr.sel_here.x+d.x,dr.sel_here.y+d.y,dr.sel_here.w,dr.sel_here.h)
	const dd=rect(pos.x-dh.x,pos.y-dh.y), insel=dr.mask!=null&&rin(dh,pos)&&dr.mask.pix[dd.x+dd.y*dh.w], r=ev.drag&&insel?dh:dr.sel_here
	if(!dr.fatbits){bg_draw_lasso(r,1,dr.fill);return}
	const o=dr.sel_start, anim=deck.patterns.anim, pal=deck.patterns.pal.pix
	const anim_pattern=(pix,x,y)=>pix<28||pix>31?pix: anim[pix-28][(0|(frame_count/4))%max(1,anim[pix-28].length)]
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		if(!dr.omask.pix[b+a*o.w])continue
		draw_rect(rmul(rect(b+o.x-dr.offset.x,a+o.y-dr.offset.y,1,1),FAT),dr.fill)
	}
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		if(!dr.mask.pix[b+a*r.w])continue
		const v=dr.limbo.pix[b+a*r.w],c=anim_pattern(v,r.x+b,r.y+a),pat=draw_pattern(c,r.x+b,r.y+a)
		if(c||!dr.trans)draw_rect(rmul(rect(b+r.x-dr.offset.x,a+r.y-dr.offset.y,1,1),FAT),c>=32?c: c==0?c: pat?1:32)
	}
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++){
		if((0xFF&dr.mask.pix[b+a*r.w])!=ANTS)continue
		const p=rmul(rect(b+r.x-dr.offset.x,a+r.y-dr.offset.y),FAT)
		if(b<=    0||!dr.mask.pix[(b-1)+a*r.w])draw_vline(p.x      ,p.y,p.y+FAT-1,ANTS)
		if(b>=r.w-1||!dr.mask.pix[(b+1)+a*r.w])draw_vline(p.x+FAT-1,p.y,p.y+FAT-1,ANTS)
		if(a<=    0||!dr.mask.pix[b+(a-1)*r.w])draw_hline(p.x,p.x+FAT-1,p.y      ,ANTS)
		if(a>=r.h-1||!dr.mask.pix[b+(a+1)*r.w])draw_hline(p.x,p.x+FAT-1,p.y+FAT-1,ANTS)
	}
}
bg_tools=_=>{
	if     (!dr.fatbits&&ev.mu&&ev.alt){dr.fatbits=1,fat_offset(ev.pos);return}
	else if( dr.fatbits&&ev.mu&&ev.alt){dr.fatbits=0;return}if(ev.alt)return
	const pp=rcopy(pointer.prev), te=copy_object(ev)
	if(ev.md&&!over(frame.clip))ev.md=0
	if(dr.fatbits){ev.pos=fat_to_card(ev.pos),ev.dpos=fat_to_card(ev.dpos),pointer.prev=fat_to_card(pointer.prev)}
	if(dr.tool=='pencil'||dr.tool=='line'||dr.tool=='rect'||dr.tool=='fillrect'||dr.tool=='ellipse'||dr.tool=='fillellipse'){
		let clear=0;if(!dr.scratch)bg_scratch()
		if(ev.md){bg_scratch(),dr.erasing=ev.rdown||ev.shift}
		else if(ev.mu||ev.drag){
			const t=frame;frame=draw_frame(dr.scratch)
			if(dr.tool=='pencil'||dr.erasing){
				draw_line(rpair(pointer.prev,ev.pos),dr.brush,dr.erasing?0:bg_pat())
			}
			else if(dr.tool=='line'){
				let t=rcopy(ev.pos);if(ev.shift){ // snap to isometric angles
					const d=rsub(t,ev.dpos);t=rcopy(ev.dpos)
					if     (Math.abs(d.x)*4<Math.abs(d.y)){t.y+=d.y;}
					else if(Math.abs(d.y)*4<Math.abs(d.x)){t.x+=d.x;}
					else if(Math.abs(d.x)*2<Math.abs(d.y)){t.x+=sign(d.x)*Math.abs(d.y)/2,t.y+=d.y}
					else if(Math.abs(d.y)*2<Math.abs(d.x)){t.y+=sign(d.y)*Math.abs(d.x)/2,t.x+=d.x}
					else {t.x+=sign(d.x)*Math.abs(max(d.x,d.y)),t.y+=sign(d.y)*Math.abs(max(d.x,d.y))}
				}bg_scratch_clear(),draw_line(rpair(ev.dpos,rint(t)),dr.brush,bg_pat())
			}
			else if(dr.tool=='rect'||dr.tool=='fillrect'){
				const t=rsub(ev.pos,ev.dpos);if(ev.shift){t.x=t.y=max(t.x,t.y)} // snap to square
				bg_scratch_clear();const r=rnorm(rpair(ev.dpos,t))
				if(dr.tool=='fillrect')draw_rect(r,bg_fill());draw_box(r,dr.brush,bg_pat())
			}
			else if(dr.tool=='ellipse'||dr.tool=='fillellipse'){
				const t=rsub(ev.pos,ev.dpos);if(ev.shift){t.x=t.y=max(t.x,t.y)} // snap to circle
				bg_scratch_clear();const r=rnorm(rpair(ev.dpos,t))
				const c=rect(r.x+(r.w/2.0),r.y+(r.h/2.0)), divs=100, poly=range(divs).map(z=>{
					const a=z*(2*Math.PI)/divs;return rint(rect(c.x+(0.5+r.w/2.0)*Math.cos(a),c.y+(0.5+r.h/2.0)*Math.sin(a)))
				});poly.push(poly[0])
				if(dr.tool=='fillellipse')draw_poly(poly,bg_fill());draw_lines(poly,dr.brush,bg_pat())
			}
			frame=t;if(ev.mu)bg_edit(),clear=1
		}
		if(dr.scratch){
			if(dr.fatbits){draw_fat(dr.scratch,deck.patterns.pal.pix,frame_count,BG_MASK,FAT,dr.offset)}
			else{image_overlay(frame.image,dr.scratch,BG_MASK);}
		}
		if(clear)bg_scratch_clear()
	}
	if(dr.tool=='lasso'){
		const d=rect(ev.pos.x-ev.dpos.x,ev.pos.y-ev.dpos.y)
		const dh=rect(dr.sel_here.x+d.x,dr.sel_here.y+d.y,dr.sel_here.w,dr.sel_here.h)
		const dd=rect(ev.pos.x-dh.x,ev.pos.y-dh.y), insel=dr.mask!=null&&over(dh)&&dr.mask.pix[dd.x+dd.y*dh.w]
		if(ev.md&&!insel){bg_lasso_preview(),bg_end_lasso(),dr.poly.push(rcopy(ev.dpos))}
		else if(ev.drag&&!insel&&dr.poly.length>0){const l=last(dr.poly);if(ev.pos.x!=l.x||ev.pos.y!=l.y)dr.poly.push(rcopy(ev.pos))}
		else if(ev.mu&&insel){dr.sel_here=radd(dr.sel_here,d)}
		else if(ev.mu){
			const r=poly_bounds(dr.poly);if(r.w>1&&r.h>1){
				dr.mask=image_make(rect(r.w,r.h))
				const t=frame;frame=draw_frame(dr.mask)
				for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++)if(poly_in(dr.poly,rect(b+r.x,a+r.y)))pix(rect(b,a),1); dr.omask=image_copy(dr.mask)
				if(dr.poly.length>0)draw_lines(dr.poly.concat(dr.poly[0]).map(p=>rsub(p,rect(r.x,r.y))),0,ANTS)
				frame=t,dr.sel_here=rcopy(r),dr.sel_start=rcopy(r),bg_scoop_selection()
			}dr.poly=[]
		}
		if(dr.mask&&dr.limbo){
			if(ev.dir=='left' ){ev.dir=0,dr.sel_here.x--}
			if(ev.dir=='up'   ){ev.dir=0,dr.sel_here.y--}
			if(ev.dir=='right'){ev.dir=0,dr.sel_here.x++}
			if(ev.dir=='down' ){ev.dir=0,dr.sel_here.y++}
			if(ev.exit){dr.limbo=dr.mask=dr.omask=null,bg_end_lasso()}
		}
	}
	if(dr.tool=='poly'){
		if(ev.md){dr.poly=[rect(ev.dpos.x,ev.dpos.y)]}
		else if(ev.drag&&dr.poly.length>0){const l=last(dr.poly);if(ev.pos.x!=l.x||ev.pos.y!=l.y)dr.poly.push(rcopy(ev.pos))}
		else if(ev.mu){
			dr.poly.push(rcopy(ev.dpos)),bg_scratch();const t=frame;frame=draw_frame(dr.scratch)
			draw_poly(dr.poly,bg_fill()),draw_lines(dr.poly,dr.brush,bg_pat()),frame=t,bg_edit(),bg_scratch_clear(),dr.poly=[]
		}
	}
	if(dr.tool=='lasso'||dr.tool=='poly'){
		const o=rsub(dr.sel_here,dr.sel_start)
		draw_lines(dr.poly.map(p=>radd(o,card_to_fat(p))),dr.tool=='lasso'?0:dr.brush,dr.tool=='lasso'?ANTS:bg_pat())
	}
	if(dr.tool=='fill'&&ev.mu){
		const bg=canvas_image(ifield(deck,'card'),1), t=frame;bg_scratch(),frame=draw_frame(dr.scratch)
		draw_fill(ev.pos,ev.rup?bg_fill():bg_pat(),bg),frame=t,bg_edit(),bg_scratch_clear()
	}
	if(!bg_has_sel()&&!bg_has_lasso()){
		if(dr.fatbits){
			if(ev.dir=='left' )dr.offset.x--
			if(ev.dir=='right')dr.offset.x++
			if(ev.dir=='up'   )dr.offset.y--
			if(ev.dir=='down' )dr.offset.y++
			dr.offset=rint(rect(max(0,min(dr.offset.x,frame.size.x-(frame.size.x/8))),max(0,min(dr.offset.y,frame.size.y-(frame.size.y/8)))))
			if(ev.exit)dr.fatbits=0
		}
		else{
			if(ev.dir=='left' )n_go([lms('Prev')],deck)
			if(ev.dir=='right')n_go([lms('Next')],deck)
		}
	}
	ev=te,pointer.prev=pp
}
bg_end_lasso=_=>{
	if(uimode!='draw'||dr.tool!='lasso')return
	const data=dr.mask&&dr.limbo, diffrect=!requ(dr.sel_here,dr.sel_start);let diffmask=dr.omask==null;
	if(dr.omask)for(let z=0;data&&z<dr.mask.pix.length;z++)if((dr.mask.pix[z]>0)!=(dr.omask.pix[z]>0)){diffmask=1;break}
	if(data&&(diffrect||diffmask)){
		bg_scratch();const t=frame;frame=draw_frame(dr.scratch),bg_draw_lasso(dr.sel_here,0,dr.fill),frame=t,bg_edit(),bg_scratch_clear()
	}dr.poly=[],dr.mask=null,dr.omask=null,dr.limbo=null,dr.sel_here=rect(),dr.sel_start=rect()
}
bg_end_selection=_=>{
	if(uimode!='draw'||dr.tool!='select')return
	if(dr.sel_here.w<=0&&dr.sel_here.h<=0)return
	bg_edit_sel(),dr.sel_start=rcopy(ev.dpos),dr.sel_here=rcopy(ev.dpos)
}
bg_delete_selection=_=>{
	if(bg_has_lasso()){
		dr.mask=null,bg_scratch();const t=frame;frame=draw_frame(dr.scratch),bg_draw_lasso(dr.sel_here,0,dr.fill),frame=t
		bg_edit(),bg_scratch_clear(),bg_end_lasso();return
	}
	if(!bg_has_sel())return
	if(dr.limbo!=null&&dr.sel_start.w<=0&&dr.sel_start.h<=0){dr.sel_here=rect(),dr.limbo=null,dr.limbo_dither=0;return}
	if(dr.sel_start.w<=0&&dr.sel_start.h<=0)dr.sel_start=rcopy(dr.sel_here)
	dr.sel_here=rect(),dr.limbo=image_make(rect(1,1)),dr.limbo_dither=0,bg_edit_sel(),dr.sel_start=rcopy(ev.dpos),dr.sel_here=rcopy(ev.dpos)
}
bg_paste=image=>{
	const clip=fat_clip(), f=rect(clip.w*.75,clip.h*.75);let s=image.size
	if(s.x>f.x||s.y>f.y){const scale=min(f.x/s.x,f.y/s.y);s=rect(s.x*scale,s.y*scale)}if(!s.x)return
	if(bg_has_sel()){bg_scoop_selection(),dr.limbo=image,dr.limbo_dither=0}
	else{settool('select'),dr.sel_start=rect(),dr.sel_here=rcenter(clip,s),dr.limbo=image,dr.limbo_dither=0}
}
draw_handles=r=>{
	const h=5, pal=deck.patterns.pal.pix
	const x0=r.x+1-h, x2=r.x+r.w-1, y0=r.y+1-h, y2=r.y+r.h-1, x1=0|((x2-x0)/2+x0), y1=0|((y2-y0)/2+y0)
	draw_invert(pal,rclip(rect(x0,y0,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x0,y2,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x2,y0,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x2,y2,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x2,y1,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x1,y2,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x1,y0,h,h),frame.clip))
	draw_invert(pal,rclip(rect(x0,y1,h,h),frame.clip))
}
in_handle=r=>{
	const h=5, x0=r.x+1-h, x2=r.x+r.w-1, y0=r.y+1-h, y2=r.y+r.h-1, x1=0|((x2-x0)/2+x0), y1=0|((y2-y0)/2+y0)
	if(over(rect(x0,y0,h,h)))return 4
	if(over(rect(x0,y2,h,h)))return 6
	if(over(rect(x2,y0,h,h)))return 2
	if(over(rect(x2,y2,h,h)))return 0
	if(over(rect(x2,y1,h,h)))return 1
	if(over(rect(x1,y0,h,h)))return 3
	if(over(rect(x0,y1,h,h)))return 5
	if(over(rect(x1,y2,h,h)))return 7;return -1
}
bg_select=_=>{
	if(uimode!='draw'||dr.tool!='select')return rect(0,0,0,0)
	const te=copy_object(ev)
	if(dr.fatbits){ev.pos=fat_to_card(ev.pos),ev.dpos=fat_to_card(ev.dpos),pointer.prev=fat_to_card(pointer.prev)}
	let s=rcopy(dr.sel_here), has_sel=s.w>0||s.h>0, in_sel=has_sel&&dover(s)
	const ax=min(ev.dpos.x,ev.pos.x), bx=max(ev.dpos.x,ev.pos.x), ay=min(ev.dpos.y,ev.pos.y), by=max(ev.dpos.y,ev.pos.y), h=5
	const x0=s.x+1-h, x2=s.x+s.w-1, y0=s.y+1-h, y2=s.y+s.h-1, x1=0|((x2-x0)/2+x0), y1=0|((y2-y0)/2+y0), dx=ev.pos.x-ev.dpos.x, dy=ev.pos.y-ev.dpos.y
	const sz=dr.limbo?dr.limbo.size:rect(dr.sel_start.w,dr.sel_start.h)
	handle=(rw,rh,ox,oy,ow,oh)=>{if(has_sel&&(ev.mu||ev.drag)&&dover(rect(rw,rh,h,h))){
		s=rnorm(keep_ratio(rect(s.x+ox,s.y+oy,s.w+ow,s.h+oh),sz))
		if(ev.mu)dr.sel_here=s;bg_scoop_selection(),uicursor=cursor.drag;return 1
	};return 0}
	if(in_layer()){
		if     (handle(x2,y2,  0, 0, dx, dy)){} // se
		else if(handle(x0,y2, dx, 0,-dx, dy)){} // sw
		else if(handle(x2,y0,  0,dy, dx,-dy)){} // ne
		else if(handle(x0,y0, dx,dy,-dx,-dy)){} // nw
		else if(handle(x2,y1,  0, 0, dx,  0)){} // e
		else if(handle(x1,y0,  0,dy,  0,-dy)){} // n
		else if(handle(x0,y1, dx, 0,-dx,  0)){} // w
		else if(handle(x1,y2,  0, 0,  0, dy)){} // s
		else if(ev.md&&in_sel){bg_scoop_selection()} // begin move
		else if((ev.mu||ev.drag)&&in_sel){s.x+=dx, s.y+=dy;if(ev.mu)dr.sel_here=s} // move/finish
		else if(ev.md&&!in_sel){if(has_sel){draw_limbo(card_to_fat(s),dr.fatbits),bg_end_selection(),has_sel=0}s=rcopy(dr.sel_here)} // begin create
		else if(ev.mu||ev.drag){s=rect(ax,ay,bx-ax,by-ay);if(ev.mu)dr.sel_here=s} // size/finish
	}
	if(has_sel)draw_limbo(s,dr.fatbits)
	if(in_layer()){
		if(has_sel&&ev.dir=='left' ){ev.dir=0,bg_scoop_selection(),dr.sel_here.x--}
		if(has_sel&&ev.dir=='up'   ){ev.dir=0,bg_scoop_selection(),dr.sel_here.y--}
		if(has_sel&&ev.dir=='right'){ev.dir=0,bg_scoop_selection(),dr.sel_here.x++}
		if(has_sel&&ev.dir=='down' ){ev.dir=0,bg_scoop_selection(),dr.sel_here.y++}
		if(ev.exit){dr.limbo=null,dr.limbo_dither=0,bg_end_selection()}
	}
	ev=te;return card_to_fat(s)
}
bg_tighten=_=>{
	const r=dr.sel_here
	const set=(p,v)=>dr.mask.pix[p.x+p.y*dr.sel_here.w]=v
	const get=p=>(p.x<0||p.y<0||p.x>=dr.sel_here.w||p.y>=dr.sel_here.h)?0:dr.mask.pix[p.x+p.y*dr.sel_here.w]
	if(dr.tool=='select'){ // convert box selections into masked lasso selections
		dr.tool='lasso',bg_scoop_selection();const s=dr.sel_start, l=dr.limbo, t=frame
		dr.limbo=image_make(rect(r.w,r.h)),frame=draw_frame(dr.limbo)
		if(dr.limbo_dither){draw_dithered(frame.clip,l,1,dr.omask),dr.limbo_dither=0}else{draw_scaled(frame.clip,l,1)}frame=t
		dr.mask=image_make(rect(r.w,r.h)),dr.mask.pix.fill(1)
		if(s.w>0&&s.h>0){dr.omask=image_make(rect(s.w,s.h)),dr.omask.pix.fill(1)}else{dr.omask=null}
	}
	let changed=1,background=bg_fill();while(changed){changed=0 // erode the mask, iterating to a fixed point
		for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++)if(get(rect(b,a))&&dr.limbo.pix[b+a*r.w]==background){
			const n=get(rect(b-1,a))&&get(rect(b,a-1))&&get(rect(b+1,a))&&get(rect(b,a+1));if(!n)set(rect(b,a),0),changed=1
		}
	}
	for(let a=0;a<r.h;a++)for(let b=0;b<r.w;b++)if(get(rect(b,a))){ // regenerate the ANTS outline
		const n=get(rect(b-1,a))&&get(rect(b,a-1))&&get(rect(b+1,a))&&get(rect(b,a+1));if(!n)set(rect(b,a),ANTS)
	}
}

// Object Edit Mode

ob_order=_=>{ob.sel.sort((av,bv)=>ln(ifield(av,'index'))-ln(ifield(bv,'index')))}
ob_edit_prop=(key,value)=>{
	const before={}, after={}
	ob.sel.map(w=>{const n=ls(ifield(w,'name')),bp={},ap={}; bp[key]=ifield(w,key),ap[key]=value,before[n]=bp,after[n]=ap})
	edit({type:'ob_props',card:ln(ifield(ifield(deck,'card'),'index')),before,after})
}
ob_create=props=>{
	edit({type:'ob_create',card:ln(ifield(ifield(deck,'card'),'index')),props})
}
ob_destroy=_=>{
	if(ob.sel.length<1)return
	const props=ob.sel.map(w=>lmd([lms('name')],[ifield(w,'name')]))
	edit({type:'ob_destroy',card:ln(ifield(ifield(deck,'card'),'index')),props}),ob.sel=[]
}
can_coalesce=move=>{
	if(has_redo()||doc_hist.length==0)return false
	const prev=doc_hist[doc_hist_cursor-1];if(prev.type!='ob_props')return false
	if(Object.keys(prev.after).length!=ob.sel.length)return false
	for(let z=0;z<ob.sel.length;z++){
		const key=Object.keys(prev.after)[z], val=prev.after[key]
		if(ls(ifield(ob.sel[z],'name'))!=key)return false
		if(Object.keys(val).length!=(move?1:2))return false
		const keys=Object.keys(val)
		if(keys[0]!='pos')return false
		if(!move&&keys[1]!='size')return false
	}return true
}
ob_move=(delta,coalesce)=>{
	if(ob.sel.length<1)return
	if(coalesce&&can_coalesce(1)){
		const r=doc_hist[doc_hist_cursor-1];
		Object.values(r.after).map(z=>z.pos=lmpair(radd(getpair(z.pos),delta))),undo(),redo()
	}else{
		const before={},after={};ob.sel.map(w=>{
			const n=ls(ifield(w,'name')),f=ifield(w,'pos'),fv=getpair(f)
			before[n]={pos:f},after[n]={pos:lmpair(radd(fv,delta))}
		}),edit({type:'ob_props',card:ln(ifield(ifield(deck,'card'),'index')),before,after})
	}
}
ob_resize=(size,coalesce)=>{
	if(ob.sel.length!=1)return;const w=ob.sel[0]
	if(coalesce&&can_coalesce(0)){
		const r=doc_hist[doc_hist_cursor-1],after=Object.values(r.after)[0]
		after.pos =lmpair(rect(size.x,size.y))
		after.size=lmpair(rect(size.w,size.h)),undo(),redo()
	}else{
		const bp={pos:ifield(w,'pos')            ,size:ifield(w,'size')           }
		const ap={pos:lmpair(rect(size.x,size.y)),size:lmpair(rect(size.w,size.h))}
		const before={},after={},n=ls(ifield(w,'name'));before[n]=bp,after[n]=ap
		edit({type:'ob_props',card:ln(ifield(ifield(deck,'card'),'index')),before,after})
	}
}
object_properties=x=>{
	ob.sel=[x]
	if(button_is(x))modal_enter('button_props')
	if(field_is (x))modal_enter('field_props' )
	if(slider_is(x))modal_enter('slider_props')
	if(canvas_is(x))modal_enter('canvas_props')
	if(grid_is  (x))modal_enter('grid_props'  )
}
object_editor=_=>{
	const card=ifield(deck,'card'), wids=ifield(card,'widgets'), pal=deck.patterns.pal.pix
	if(ob.sel.length==0&&in_layer()){
		if(ev.dir=='left' )n_go([lms('Prev')],deck)
		if(ev.dir=='right')n_go([lms('Next')],deck)
	}
	wids.v.map(wid=>{
		const w=unpack_widget(wid), sel=ob.sel.some(x=>x==wid)
		if(has_parent(card))draw_rect(w.size,23)
		if(sel){draw_box(inset(w.size,-1),0,ANTS)}else if(ob.show_bounds){draw_boxinv(pal,inset(w.size,-1))}
		if(sel&&ob.sel.length==1){draw_handles(w.size)}
		if(w.locked&&ob.show_bounds){draw_rect(rect(w.size.x+w.size.w-10,w.size.y,10,10),1),draw_icon(rect(w.size.x+w.size.w-8,w.size.y+1),LOCK,32)}
	})
	if(has_parent(card)||!in_layer())return
	if(ob.sel.length>0){
		if(ev.dir=='left' )ob_move(rect(-1, 0),1)
		if(ev.dir=='right')ob_move(rect( 1, 0),1)
		if(ev.dir=='up'   )ob_move(rect( 0,-1),1)
		if(ev.dir=='down' )ob_move(rect( 0, 1),1)
	}
	const ish=ob.sel.length==1?in_handle(unpack_widget(ob.sel[0]).size):-1
	const isw=wids.v.some(w=>over(unpack_widget(w).size)&&ob.sel.some(x=>x==w))
	const a=ev.pos,b=ob.prev,dragged=a.x!=b.x||a.y!=b.y
	const sr=rnorm(rect(ev.dpos.x,ev.dpos.y,ev.pos.x-ev.dpos.x,ev.pos.y-ev.dpos.y)), box=sr.w>1||sr.h>1;
	if(isw&&ev.dclick){
		for(let z=count(wids)-1;z>=0;z--){
			const wid=wids.v[z], w=unpack_widget(wid)
			if(over(w.size)){object_properties(wid);break}
		}
	}
	else if(isw&&ev.mu&&!box){
		for(let z=ob.sel.length-1;z>=0;z--){
			const wid=ob.sel[z], w=unpack_widget(wid)
			if(over(w.size)){if(ev.shift){ob.sel=ob.sel.filter(x=>x!=wid)}else{ob.sel=[wid]};break}
		}
	}
	else if(ish!=-1&&ev.md){ob.resize=1,ob.resize_first=1,ob.handle=ish,ob.prev=rcopy(ev.pos),ob.orig=unpack_widget(ob.sel[0]).size}
	else if(isw    &&ev.md){ob.move  =1,ob.move_first  =1,ob.prev=rcopy(ev.pos)}
	else if(ob.resize&&(!ev.drag||!ob.sel.length)){ob.resize=0,ob.resize_first=0}
	else if(ob.move&&(!ev.drag||!ob.sel.length)){ob.move=0,ob.move_first=0}
	else if(ob.resize){
		const delta=rsub(ev.pos,ev.dpos), r=rcopy(ob.orig)
		if(ob.handle==0){r.w+=delta.x, r.h+=delta.y}
		if(ob.handle==2){r.w+=delta.x, r.h-=delta.y, r.y+=delta.y}
		if(ob.handle==6){r.w-=delta.x, r.h+=delta.y, r.x+=delta.x}
		if(ob.handle==4){r.w-=delta.x, r.h-=delta.y, r.x+=delta.x, r.y+=delta.y}
		if(ob.handle==1){r.w+=delta.x}
		if(ob.handle==3){r.h-=delta.y, r.y+=delta.y}
		if(ob.handle==5){r.w-=delta.x, r.x+=delta.x}
		if(ob.handle==7){r.h+=delta.y}
		r.w=max(8,r.w), r.h=max(8,r.h)
		if(dragged)ob_resize(r,!ob.resize_first),ob.resize_first=0,ob.prev=rcopy(ev.pos)
	}
	else if(ob.move){
		const delta=rsub(a,b)
		if(dragged)ob_move(delta,!ob.move_first),ob.move_first=0,ob.prev=rcopy(ev.pos)
	}else{
		if(box&&ev.drag)draw_box(sr,0,ANTS); if(box&&(ev.drag||ev.mu))ob.sel=[]
		let f=0;for(let z=count(wids)-1;z>=0;z--){ // backward pass for selection priority(!)
			const wid=wids.v[z], w=unpack_widget(wid), sel=ob.sel.some(x=>x==wid), overlap=rclip(w.size,sr)
			const insel=box?overlap.w>=1&&overlap.h>=1: over(w.size)&&dover(w.size), c=ev.mu&insel
			if(c)f=1; if(!c)continue; if(box){ob.sel.push(wid);continue}
			if(!sel){if(!ev.shift)ob.sel=[];ob.sel.push(wid);break}
		}if(ev.mu&&!f)ob.sel=[]
	}
}

// Toolbars

let toolbars_enable=0, tzoom=1 // should be off by default?
const tcellw=22, tcellh=19, tgap=1, toolsize=rect(tcellw*2+1,tcellh*18+tgap+1), tfb=image_make(toolsize), tid=new ImageData(toolsize.x,toolsize.y)
const tooltypes=['select','pencil','lasso','line','fill','poly','rect','fillrect','ellipse','fillellipse']
const patorder=[0,1,4,5,8,9,16,17,12,13,18,19,20,21,22,23,24,25,26,27,2,6,3,7,10,11,14,15,28,29,30,31] // pleasing visual ramps for 2 columns

toolbars=_=>{
	if(!toolbars_enable)return
	const toolbar=(element,render,behavior)=>{
		const c=element.getBoundingClientRect()
		const pos =rint(rect((ev.rawpos .x-c.x)/tzoom,(ev.rawpos .y-c.y)/tzoom))
		const dpos=rint(rect((ev.rawdpos.x-c.x)/tzoom,(ev.rawdpos.y-c.y)/tzoom))
		tfb.pix.fill(0),frame=draw_frame(tfb),draw_box(rpair(rect(),toolsize),0,1),behavior(pos,dpos)
		const anim=deck.patterns.anim, pal=deck.patterns.pal.pix
		const animated=rin(rect(c.x,c.y,c.width,c.height),ev.rawpos)&&dr.show_anim?(0|(frame_count/4)):0
		const anim_pattern=(pix,x,y)=>pix<28||pix>31?pix: anim[pix-28][animated%max(1,anim[pix-28].length)]
		const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
		const draw_color  =(pix,x,y)=>pix>47?0: pix>31?pix-32: draw_pattern(pix,x,y)?15:0
		const data=tid.data;for(let z=0,d=0,y=0;y<tid.height;y++)for(let x=0;x<tid.width;x++,z++){
			const pix=tfb.pix[z], a=anim_pattern(pix,x,y), c=draw_color(a,x,y), cv=COLORS[c]
			data[d++]=0xFF&(cv>>16),data[d++]=0xFF&(cv>>8),data[d++]=0xFF&(cv),data[d++]=0xFF
		}
		render.getContext('2d').putImageData(tid,0,0)
		const g=element.getContext('2d');g.imageSmoothingEnabled=false,g.save(),g.scale(tzoom,tzoom),g.drawImage(render,0,0),g.restore()
	}
	const toolbtn=(pos,dn,b,icon,active)=>{
		const i=rcenter(b,rect(16,16))
		draw_box(b,0,1);if(active)draw_rect(b,1);draw_icon(i,TOOLS[icon],active?0:1)
		if(rin(b,pos))uicursor=cursor.point;return rin(b,pos)&&rin(b,dn)&&ev.mu
	}
	const modebtn=(pos,dn,b,text,active)=>{
		draw_box(b,0,1);if(active)draw_rect(inset(b,2),1);draw_textc(b,text,FONT_BODY,active?0:1)
		if(rin(b,pos))uicursor=cursor.point;return rin(b,pos)&&rin(b,dn)&&ev.mu
	}
	const brushbtn=(pos,dn,b,brush)=>{
		const i=rint(rect(b.x+(b.w/2),b.y+(b.h/2)))
		draw_box(b,0,1),draw_line(rect(i.x,i.y,i.x,i.y),brush,1)
		if(dr.brush==brush)draw_box(inset(b,2),0,1); if(!rin(b,pos))return
		uicursor=cursor.point;if(!ev.mu||!rin(b,dn))return
		setmode('draw');if(dr.tool=='select'||dr.tool=='lasso'||dr.tool=='fill')settool('pencil');dr.brush=brush
	}
	const palbtn=(pos,dn,b,pattern)=>{
		if((dr.pickfill?dr.fill:dr.pattern)==pattern){draw_rect(inset(b,3),pattern),draw_box(inset(b,3),0,1)}else{draw_rect(b,pattern)}
		if(rin(b,pos)){uicursor=cursor.point;if(ev.mu&&rin(b,dn)){if(dr.pickfill){dr.fill=pattern}else{dr.pattern=pattern}}}draw_box(b,0,1)
	}
	toolbar(q('#ltools'),q('#lrender'),(pos,dn)=>{
		draw_rect(rect(0,6*tcellh,toolsize.x,tgap),1)
		if(toolbtn(pos,dn,rect(0     ,0,tcellw+1,tcellh+1),0,uimode=='interact'))setmode('interact'),ev.mu=ev.md=0
		if(toolbtn(pos,dn,rect(tcellw,0,tcellw+1,tcellh+1),1,uimode=='object'  ))setmode('object'  ),ev.mu=ev.md=0
		for(let z=0;z<2*12;z++)brushbtn(pos,dn,rect((z%2)*tcellw,(6+(0|(z/2)))*tcellh+tgap,tcellw+1,tcellh+1),((z*12)+(0|(z/2)))%24)
		for(let z=0;z<10;z++){
			if(toolbtn(pos,dn,rect((z%2)*tcellw,(1+(0|(z/2)))*tcellh,tcellw+1,tcellh+1),z+2,uimode=='draw'&&dr.tool==tooltypes[z])){
				settool(tooltypes[z]),ev.mu=ev.md=0
			}
		}
	})
	toolbar(q('#rtools'),q('#rrender'),(pos,dn)=>{
		draw_rect(rect(0,16*tcellh,toolsize.x,tgap),1)
		if(modebtn(pos,dn,rect(0,0     ,tcellw*2+1,tcellh+1),'Stroke',dr.pickfill==0))dr.pickfill=0
		if(modebtn(pos,dn,rect(0,tcellh,tcellw*2+1,tcellh+1),'Fill'  ,dr.pickfill==1))dr.pickfill=1
		for(let z=0;z<4*8;z++)palbtn(pos,dn,rect((z%2)*tcellw,(2+(0|(z/2)))*tcellh+(z>=28?tgap:0),tcellw+1,tcellh+1),patorder[z])
	})
}

// Script Editor

setscript=x=>{
	if(uimode!='script')sc.prev_mode=uimode;setmode('script')
	sc.status='', sc.target=lii(x)?x:x[0], sc.others=lii(x)?[]: x.slice(1)
	let v=ifield(sc.target,'script'), p=0
	if(!count(v))v=card_is  (sc.target)?(p=1,lms('on view do\n \nend')):
	               button_is(sc.target)?(p=1,lms('on click do\n \nend')):
	               grid_is  (sc.target)?(p=1,lms('on click row do\n \nend')):
	               field_is (sc.target)?(p=1,lms('on change val do\n \nend')):
	               slider_is(sc.target)?(p=1,lms('on change val do\n \nend')):
	               canvas_is(sc.target)?(p=1,lms('on click pos do\n \nend\n\non drag pos do\n \nend\n\non release pos do\n \nend')):v
	sc.status=p?'No existing script; populated a template.':'',sc.f=fieldstr(v)
}
finish_script=_=>{if(sc.next){setscript(sc.next),sc.next=null}else{setmode(sc.prev_mode)}}
close_script=next=>{
	sc.next=next,field_exit()
	try{const text=ls(rtext_string(sc.f.table));parse(text),script_save(lms(text)),finish_script()}
	catch(e){
		modal_enter('confirm_script')
		ms.message=lms(`The current script contains errors:\n\n${e.x}\n\nDo you wish to discard your changes?`),ms.verb=lms('Discard')
	}
}
script_editor=_=>{
	const field_position=(x,cursor)=>{
		if(x.layout.length<1)return rect(1,1);cursor=max(0,min(cursor,x.layout.length+1))
		const e=cursor>=x.layout.length?1:0, i=cursor-e, l=x.layout[i].line, c=i-x.lines[l].range.x
		return rect(l+1,c+1+e)
	}
	const mh=3+font_h(FONT_MENU), bb=rect(0,mh,frame.size.x+1,frame.size.y-2*mh)
	if(sc.xray){
		const card=ifield(deck,'card'),wids=ifield(card,'widgets');
		for(let z=0;z<wids.v.length;z++){
			const wid=wids.v[z],size=unpack_widget(wid).size
			draw_textc(size,ls(ifield(wid,'name')),FONT_BODY,44),draw_box(size,0,44)
			if(ls(ifield(wid,'script')).length)draw_icon(rect(size.x-1,size.y),ICONS[ICON.lil],44)
			if(ev.alt&&ev.mu&&over(size)&&dover(size)){close_script(wid),ev.md=ev.mu=0;break}
		}if(ev.alt&&ev.mu)close_script(card),ev.md=ev.mu=0
	}
	ui_codeedit(bb,0,sc.f),draw_hline(0,frame.size.x,frame.size.y-mh-1,1)
	if(sc.status.length){draw_text_fit(rect(3,frame.size.y-mh+3,frame.size.x,mh-6),sc.status,FONT_BODY,1)}
	else{
		let stat='';if(in_layer()&&wid.infield){
			const x=layout_richtext(deck,sc.f.table,FONT_MONO,ALIGN.left,bb.w)
			let a=min(wid.cursor.x,wid.cursor.y), b=max(wid.cursor.x,wid.cursor.y)
			if(a!=b&&a<x.layout.length){
				a=max(0,min(a,x.layout.length)), b=max(0,min(b,x.layout.length))
				const ap=field_position(x,a),bp=field_position(x,b), l=(bp.x-ap.x)+1, c=b-a
				stat=`${l} line${l==1?'':'s'}, ${c} character${c==1?'':'s'} selected`
			}else{const p=field_position(x,min(a,b));stat=`Line ${p.x}, Column ${p.y}`}
		}
		const l=font_textsize(FONT_BODY,stat);draw_text(rect(3,frame.size.y-mh+3,l.x,l.y),stat,FONT_BODY,1)
		stat=`script of ${sc.target.n}  '${ls(ifield(sc.target,'name'))}'${sc.others.length?` and ${sc.others.length} more`:''}`
		const t=layout_plaintext(stat,FONT_BODY,ALIGN.right,rect(frame.size.x-6-20-l.x,font_h(FONT_BODY)))
		draw_text_wrap(rect(3+l.x+20,frame.size.y-mh+3,t.size.x,t.size.y),t,1)
	}if(in_layer()&&ev.exit)close_script()
}

// Runtime

interpret=_=>{
	if(msg.overshoot&&!running()&&!msg.pending_view&&!msg.next_view)msg.overshoot=0
	if(msg.pending_halt){if(running())halt();sleep_frames=0,sleep_play=0,msg.pending_view=0,msg.next_view=0}
	if(sleep_play&&sfx_any())return 0;sleep_play=0
	if(sleep_frames){sleep_frames--;return 0}
	const nomodal=_=>(ms.type==null||ms.type=='query'||ms.type=='listen')
	let quota=FRAME_QUOTA;while(1){
		while(nomodal()&&running()&&sleep_frames==0&&sleep_play==0&&quota>0){runop(),quota--,mark_dirty()}frame=context
		if(quota<=0&&running())msg.overshoot=1
		if(!nomodal()||quota<=0||sleep_frames||sleep_play){if(sleep_frames)sleep_frames--;break}
		if(!running()&&pending_popstate)popstate(),pending_popstate=0
		if(msg.pending_halt||pending_popstate){/*suppress other new events until this one finishes*/}
		else if(msg.pending_view){fire_event_async(ifield(deck,'card'),'view',NONE),msg.pending_view=0}
		else if(msg.target_click){
			const arg=grid_is(msg.target_click)?lmn(msg.arg_click.y): canvas_is(msg.target_click)?lmpair(msg.arg_click): NONE
			fire_event_async(msg.target_click,'click',arg),msg.target_click=null
		}
		else if(msg.target_drag    ){fire_event_async(msg.target_drag    ,'drag'    ,lmpair(msg.arg_drag   )),msg.target_drag    =null}
		else if(msg.target_release ){fire_event_async(msg.target_release ,'release' ,lmpair(msg.arg_release)),msg.target_release =null}
		else if(msg.target_run     ){fire_event_async(msg.target_run     ,'run'     ,msg.arg_run            ),msg.target_run     =null}
		else if(msg.target_link    ){fire_event_async(msg.target_link    ,'link'    ,msg.arg_link           ),msg.target_link    =null}
		else if(msg.target_order   ){fire_event_async(msg.target_order   ,'order'   ,msg.arg_order          ),msg.target_order   =null}
		else if(msg.target_change  ){fire_event_async(msg.target_change  ,'change'  ,msg.arg_change         ),msg.target_change  =null}
		else if(msg.target_navigate){fire_event_async(msg.target_navigate,'navigate',msg.arg_navigate       ),msg.target_navigate=null}
		if(!running())break // not running, and no remaining events to process, so we're done for this frame
	}if(msg.next_view&&ms.type!='listen')msg.pending_view=1,msg.next_view=0 // no more than one view[] event per frame!
	return FRAME_QUOTA-quota
}

text_edit_menu=_=>{
	const selection=wid.fv!=null&&wid.cursor.x!=wid.cursor.y
	if(menu_item('Undo',wid.hist_cursor>0              ,'z'))field_undo()
	if(menu_item('Redo',wid.hist_cursor<wid.hist.length,'Z'))field_redo()
	menu_separator()
	if(menu_item('Cut',selection,'x',menucut)){}
	if(menu_item('Copy',selection,'c',menucopy)){}
	if(menu_item('Paste',wid.fv!=null,'v',menupaste)){}
	if(menu_item('Clear',wid.fv!=null))wid.cursor=rect(0,RTEXT_END),field_keys('Delete',0)
	menu_separator()
	if(menu_item('Select All',wid.fv!=null,'a'))wid.cursor=rect(0,RTEXT_END)
}
all_menus=_=>{
	const blocked=running()||msg.overshoot
	const canlisten=!blocked&&(ms.type=='listen'||ms.type==null)
	menu_bar('Decker',canlisten)
	if(menu_item('About...',1))modal_enter('about')
	if(menu_check('Listener',canlisten,ms.type=='listen','l')){if(ms.type!='listen'){modal_enter('listen')}else{modal_exit(0)}}
	menu_separator()
	if(menu_check('Nav Gestures',1,enable_gestures))enable_gestures^=1
	if(menu_check('Script Profiler',1,profiler))profiler^=1
	if(menu_check('Toolbars',tzoom>0,toolbars_enable))toolbars_enable^=1,resize()
	if(blocked){
		menu_bar('Script',1)
		if(menu_item('Stop',1)){msg.pending_halt=1;if(ms.type!=null&&ms.type!='query'&&ms.type!='listen')modal_exit(0)}
		menu_bar('Edit',(ms.type=='input'||ms.type=='save')&&wid.fv)
		text_edit_menu()
		return
	}
	menu_bar('File',ms.type==null||ms.type=='recording')
	if(uimode=='script'){
		if(menu_item('Close Script',1))close_script()
		if(menu_item('Save Script',1,'s')){
			try{const text=ls(rtext_string(sc.f.table));parse(text),script_save(lms(text));sc.status='Saved.'}
			catch(e){sc.status=`Error: ${e.x}`,wid.cursor=rect(e.i,e.i)}
		}
		menu_separator()
		menu_item('Import Script...',1,0,_=>open_text('.lil,.txt',text=>{field_exit(),sc.f=fieldstr(lms(text))}))
		if(menu_item('Export Script...',1))modal_enter('export_script')
		menu_separator()
		if(menu_item('Go to Deck',!deck_is(sc.target)           ))close_script(deck)
		if(menu_item('Go to Card',sc.target!=ifield(deck,'card')))close_script(ifield(deck,'card'))
		if(menu_check('X-Ray Specs',1,sc.xray))sc.xray^=1
	}
	else if(ms.type=='recording'){
		menu_item('Import Sound...',1,0,_=>open_file('audio/*',load_sound))
		menu_separator();
		if(menu_item('Close Sound',1))modal_exit(0)
	}
	else{
		if(menu_item('New Deck...',1)){
			if(dirty){
				modal_enter('confirm_new')
				ms.message=lms('The current deck has unsaved changes.\nAre you sure you want to discard it?')
				ms.verb=lms('Discard')
			}else{load_deck(deck_read(''))}
		}
		menu_separator()
		menu_item('Open...',1,0,_=>open_text('.html,.deck',text=>{load_deck(deck_read(text))}))
		if(menu_item('Save As...',1))modal_enter('save_deck')
		menu_separator()
		if(wid.gv){
			menu_item('Import Table...',!wid.g.locked,0,_=>open_text('.csv',text=>{
				const a=[lms(text)];if(wid.g.format.length)a.push(lms(wid.g.format))
				grid_edit(n_readcsv(a))
			}))
			if(menu_item('Export Table...',1))modal_enter('export_table')
		}
		else{
			menu_item("Import Image...",1,0,_=>{open_file('image/*',load_image)})
			if(menu_item("Export Image...",1))modal_enter('export_image')
		}
		menu_separator()
		if(menu_item('Cards...'     ,1,'C'))modal_enter('cards')
		if(menu_item('Sounds...'    ,1,'S'))modal_enter('sounds')
		if(menu_item('Resources...' ,1,   ))modal_enter('resources')
		if(menu_item('Properties...',1,   ))modal_enter('deck_props')
	}
	if(ms.type==null||wid.gv||wid.fv){
		menu_bar("Edit",wid.gv||wid.fv||(ms.type==null&&uimode=='interact')||uimode=='draw'||uimode=='object')
		if(wid.gv){
			const mutable=!wid.g.locked&&ms.type==null
			if(menu_item('Undo',wid.hist_cursor>0              ,'z'))grid_undo()
			if(menu_item('Redo',wid.hist_cursor<wid.hist.length,'Z'))grid_redo()
			menu_separator()
			if(menu_item('Copy Table',1,'c',menucopy)){}
			if(menu_item('Paste Table',mutable,'v',menupaste)){}
			menu_separator()
			if(menu_item('Delete Row',mutable&&wid.gv.row!=-1))grid_deleterow()
			if(menu_item('Add Row',mutable))grid_insertrow()
			if(menu_item('Query...',ms.type==null,'u'))modal_enter('query')
		}
		if(wid.fv)text_edit_menu()
		if(ms.type==null&&uimode=='interact'){
			if(menu_item('Undo',has_undo(),'z'))undo()
			if(menu_item('Redo',has_redo(),'Z'))redo()
			menu_separator()
			if(menu_item('Paste',1,'v',menupaste)){}
		}
		if(ms.type==null&&uimode=='draw'){
			const sel=bg_has_sel()||bg_has_lasso()
			if(menu_item('Undo',(!sel)&&has_undo(),'z'))undo()
			if(menu_item('Redo',(!sel)&&has_redo(),'Z'))redo()
			menu_separator()
			if(menu_item('Cut Image',sel,0,menucut)){}
			if(menu_item('Copy Image',sel,0,menucopy)){}
			if(menu_item('Paste',1,'v',menupaste)){}
			if(menu_item('Clear',1)){const t=dr.tool;if(!sel){settool('select'),dr.sel_here=rcopy(frame.clip)}bg_delete_selection(),settool(t)}
			menu_separator()
			if(menu_item('Select All',1,'a')){settool('select'),dr.sel_here=rcopy(frame.clip)}
			if(menu_item('Tight Selection',sel,'g'))bg_tighten()
			if(menu_item("Resize to Original",sel&&dr.tool=='select',0)){bg_scoop_selection();const s=dr.limbo.size;dr.sel_here.w=s.x,dr.sel_here.h=s.y}
			if(menu_item("Resize to Card"    ,sel&&dr.tool=='select',0)){bg_scoop_selection(),dr.sel_here=rect(0,0,frame.size.x,frame.size.y)}
			menu_separator()
			if(menu_item('Invert',sel&&!dr.limbo_dither,'i')){
				if(bg_has_sel())bg_scoop_selection()
				const s=dr.limbo.size, pal=deck.patterns.pal.pix
				for(let z=0;z<dr.limbo.pix.length;z++)dr.limbo.pix[z]=1^draw_pattern(dr.limbo.pix[z],(z%s.x),0|(z/s.x))
			}
			if(menu_item('Flip Horizontal',sel)){if(bg_has_sel())bg_scoop_selection();image_flip_h(dr.limbo);if(dr.mask)image_flip_h(dr.mask)}
			if(menu_item('Flip Vertical'  ,sel)){if(bg_has_sel())bg_scoop_selection();image_flip_v(dr.limbo);if(dr.mask)image_flip_v(dr.mask)}
			if(menu_item('Rotate Left',sel,',')){
				if(bg_has_sel())bg_scoop_selection()
				image_flip_h(dr.limbo),image_flip(dr.limbo)
				if(dr.mask)image_flip_h(dr.mask),image_flip(dr.mask)
				const s=dr.limbo.size;dr.sel_here.w=s.x,dr.sel_here.h=s.y
			}
			if(menu_item('Rotate Right',sel,'.')){
				if(bg_has_sel())bg_scoop_selection()
				image_flip(dr.limbo),image_flip_h(dr.limbo)
				if(dr.mask)image_flip(dr.mask),image_flip_h(dr.mask)
				const s=dr.limbo.size;dr.sel_here.w=s.x,dr.sel_here.h=s.y
			}
		}
		const card=ifield(deck,'card')
		if(ms.type==null&&uimode=='object'){
			if(menu_item('Undo',has_undo(),'z'))undo()
			if(menu_item('Redo',has_redo(),'Z'))redo()
			menu_separator()
			if(menu_item('Cut Widgets',ob.sel.length,'x',menucut)){}
			if(menu_item('Copy Widgets',ob.sel.length,'c',menucopy)){}
			if(menu_item('Copy Image',ob.sel.length==1&&canvas_is(ob.sel[0]),0,copycanvasimg)){}
			if(menu_item('Paste',1,'v',menupaste)){}
			menu_separator()
			if(menu_item('Paste as new Canvas',!has_parent(card),0,pasteascanvas)){}
			if(menu_item('Paste into Canvas',ob.sel.length==1&&canvas_is(ob.sel[0]),0,pasteintocanvas)){}
			menu_separator()
			if(menu_item('Select All',!has_parent(card),'a'))ob.sel=card.widgets.v.slice(0)
			if(menu_item('Move to Front',ob.sel.length))ob_order(),ob.sel                   .map(w=>iwrite(w,lms('index'),lmn(RTEXT_END))),mark_dirty()
			if(menu_item('Move to Back' ,ob.sel.length))ob_order(),ob.sel.slice(0).reverse().map(w=>iwrite(w,lms('index'),NONE          )),mark_dirty()
		}
		if(wid.fv&&wid.f){
			const selection=wid.fv!=null&&wid.cursor.x!=wid.cursor.y
			menu_bar('Text',ms.type!='field_props')
			if(wid.f.style=='rich'){
				if(menu_item('Heading'    ,selection))field_stylespan(lms('menu'),lms(''))
				if(menu_item('Body'       ,selection))field_stylespan(lms(''    ),lms(''))
				if(menu_item('Fixed Width',selection))field_stylespan(lms('mono'),lms(''))
				if(menu_item('Link...'    ,selection))modal_enter('link')
			}
			else if(wid.f.style=='code'){
				if(menu_item('Indent'        ,1    ))field_indent(1)
				if(menu_item('Unindent'      ,1    ))field_indent(0)
				if(menu_item('Toggle Comment',1,'/'))field_comment()
			}
			if(wid.f&&wid.f.style!='code'){
				if(menu_item('Font...',!has_parent(card)&&wid.f.style!='plain'))modal_enter('fonts')
			}
		}
	}
	if(ms.type=='recording'&&!wid.fv){
		menu_bar('Edit',au.mode=='stopped')
		if(menu_item('Undo',au.hist_cursor>0             ,'z'))sound_undo()
		if(menu_item('Redo',au.hist_cursor<au.hist.length,'Z'))sound_redo()
		menu_separator()
		if(menu_item('Cut Sound'  ,1,'x',menucut)){}
		if(menu_item('Copy Sound' ,1,'c',menucopy)){}
		if(menu_item('Paste Sound',1,'v',menupaste)){}
		if(menu_item('Clear',1,0))sound_delete()
		menu_separator()
		if(menu_item('Select All',1,'a'))au.head=0,au.sel=rect(0,au.target.data.length-1)
	}
	if(uimode=='interact'||uimode=='draw'||uimode=='object'){
		menu_bar('Card',ms.type==null)
		const card=ifield(deck,'card'), parent=ifield(card,'parent')
		if(menu_item('Go to First'   ,1))n_go([lms('First')],deck)
		if(menu_item('Go to Previous',1))n_go([lms('Prev' )],deck)
		if(menu_item('Go to Next'    ,1))n_go([lms('Next' )],deck)
		if(menu_item('Go to Last'    ,1))n_go([lms('Last' )],deck)
		if(menu_item('Go to Parent',has_parent(card)))n_go([parent],deck)
		menu_separator()
		if(menu_item('Cut Card',1,0,cutcard)){}
		if(menu_item('Copy Card',1,0,copycard)){}
		menu_separator()
		if(menu_item('Script...'    ,!has_parent(card)))setscript(card)
		if(menu_item('Properties...',1))modal_enter('card_props')
		menu_bar('Tool',ms.type==null)
		if(menu_check('Interact',1,uimode=='interact',0))setmode('interact')
		if(menu_check('Widgets' ,1,uimode=='object'  ,0))setmode('object')
		menu_separator()
		if(menu_check('Select'     ,1,uimode=='draw'&&dr.tool=='select'     ))settool('select'     )
		if(menu_check('Lasso'      ,1,uimode=='draw'&&dr.tool=='lasso'      ))settool('lasso'      )
		if(menu_check('Pencil'     ,1,uimode=='draw'&&dr.tool=='pencil'     ))settool('pencil'     )
		if(menu_check('Line'       ,1,uimode=='draw'&&dr.tool=='line'       ))settool('line'       )
		if(menu_check('Flood'      ,1,uimode=='draw'&&dr.tool=='fill'       ))settool('fill'       )
		if(menu_check('Box'        ,1,uimode=='draw'&&dr.tool=='rect'       ))settool('rect'       )
		if(menu_check('Filled Box' ,1,uimode=='draw'&&dr.tool=='fillrect'   ))settool('fillrect'   )
		if(menu_check('Oval'       ,1,uimode=='draw'&&dr.tool=='ellipse'    ))settool('ellipse'    )
		if(menu_check('Filled Oval',1,uimode=='draw'&&dr.tool=='fillellipse'))settool('fillellipse')
		if(menu_check('Polygon'    ,1,uimode=='draw'&&dr.tool=='poly'       ))settool('poly'       )
	}
	if(uimode=='draw'||uimode=='object'){
		menu_bar('View',ms.type==null)
		if(menu_check('Show Widgets'      ,1,dr.show_widgets))dr.show_widgets^=1
		if(menu_check('Show Widget Bounds',1,ob.show_bounds ))ob.show_bounds ^=1
		if(menu_check('Show Widget Names' ,1,ob.show_names  ))ob.show_names  ^=1
		if(menu_check('Show Cursor Info'  ,1,ob.show_cursor ))ob.show_cursor ^=1
		menu_separator()
		if(menu_check('Show Grid Overlay',1,dr.show_grid))dr.show_grid^=1
		if(menu_item('Grid Size...',1))modal_enter('grid')
		menu_separator()
		if(menu_check('Show Animation'   ,1,dr.show_anim ))dr.show_anim ^=1
		if(menu_check('Transparency Mask',1,dr.trans_mask))dr.trans_mask^=1
		if(menu_check('Fat Bits'         ,1,dr.fatbits   )){
			if(uimode!='draw')setmode('draw')
			dr.fatbits^=1;if(dr.fatbits)fat_offset(rcenter(bg_has_sel()||bg_has_lasso()?dr.sel_here:frame.clip,rect()))
		}
	}
	if(uimode=='draw'){
		menu_bar('Style',ms.type==null)
		if(menu_item('Stroke...',1))modal_enter('pattern')
		if(menu_item('Fill...'  ,1))modal_enter('fill'   )
		if(menu_item('Brush...' ,1))modal_enter('brush'  )
		menu_separator()
		if(menu_check('Transparency',1,dr.trans))dr.trans^=1
	}
	if(uimode=='object'){
		menu_bar('Widgets',ms.type==null&&!has_parent(ifield(deck,'card')))
		if(menu_item('New Button...',ob.sel.length==0))ob_create([lmd([lms('type')],[lms('button')])])
		if(menu_item('New Field...' ,ob.sel.length==0))ob_create([lmd([lms('type')],[lms('field' )])])
		if(menu_item('New Slider...',ob.sel.length==0))ob_create([lmd([lms('type')],[lms('slider')])])
		if(menu_item('New Canvas...',ob.sel.length==0))ob_create([lmd([lms('type')],[lms('canvas')])])
		if(menu_item('New Grid...'  ,ob.sel.length==0))ob_create([lmd([lms('type')],[lms('grid'  )])])
		menu_separator()
		let al=1,as=1,at=1,ai=1,an=1
		ob.sel.map(unpack_widget).map(w=>{al&=w.locked, as&=w.show=='solid', at&=w.show=='transparent', ai&=w.show=='invert', an&=w.show=='none'})
		if(menu_check('Locked'          ,ob.sel.length,ob.sel.length&&al))ob_edit_prop('locked',lmn(!al))
		if(menu_check('Show Solid'      ,ob.sel.length,ob.sel.length&&as))ob_edit_prop('show',lms('solid'      ))
		if(menu_check('Show Transparent',ob.sel.length,ob.sel.length&&at))ob_edit_prop('show',lms('transparent'))
		if(menu_check('Show Inverted'   ,ob.sel.length,ob.sel.length&&ai))ob_edit_prop('show',lms('invert'     ))
		if(menu_check('Show None'       ,ob.sel.length,ob.sel.length&&an))ob_edit_prop('show',lms('none'       ))
		menu_separator()
		if(menu_item('Font...'  ,ob.sel.length))modal_enter('fonts')
		if(menu_item('Script...',ob.sel.length)){
			if(ob.sel.reduce((m,v)=>m&&ob.sel[0].script==v.script,1)){setscript(ob.sel)}else{
				modal_enter('multiscript')
					ms.message=lms('Not all of the selected widgets\nhave the same script.\nEdit them all together anyway?')
					ms.verb=lms('Edit')
			}
		}
		if(menu_item('Properties...',ob.sel.length==1)||(ob.sel.length==1&&ev.action&&ms.type==null))object_properties(ob.sel[0])
	}
}

main_view=_=>{
	const card=ifield(deck,'card'), back=ifield(card,'image'), wids=ifield(card,'widgets'), pal=deck.patterns.pal.pix
	if(back.size.x>0&&back.size.y>0&&ms.type!='trans'){
		if(dr.fatbits){draw_fat(back,pal,frame_count,0,FAT,dr.offset)}
		else{image_paste(rpair(rect(),back.size),frame.clip,back,frame.image,1)}
	}
	if(uimode=='draw'&&in_layer())bg_tools()
	if(dr.tool=='select'&&(dr.sel_start.w>0||dr.sel_start.h>0))draw_rect(card_to_fat(dr.sel_start),dr.fill)
	bg_lasso_preview()
	const livesel=bg_select()
	if(((uimode=='object'||uimode=='draw')&&dr.show_grid)||ms.type=='grid'){
		for(let x=0;x<frame.size.x;x++){const r=card_to_fat(rect(x*dr.grid_size.x,0,1,frame.size.y));draw_vline(r.x,r.y,r.y+r.h,13)}
		for(let y=0;y<frame.size.y;y++){const r=card_to_fat(rect(0,y*dr.grid_size.y,frame.size.x,1));draw_hline(r.x,r.x+r.w,r.y,13)}
	}
	const eb=ev;if(uimode!='interact')ev=event_state()
	if(uimode=='interact'||(dr.show_widgets&&!dr.fatbits))wids.v.map(w=>{
		if(button_is(w)){
			const v=lb(ifield(w,'value')), p=unpack_button(w)
			if(widget_button(w,p,v)&&p.style=='check')iwrite(w,lms('value'),lmn(!v)),mark_dirty()
		}
		if(slider_is(w)){widget_slider(w,unpack_slider(w))}
		if(canvas_is(w)){const v=canvas_image(w,0);widget_canvas(w,unpack_canvas(w),v)}
		if(grid_is(w)){
			const p=unpack_grid(w),v=unpack_grid_value(w);widget_grid(w,p,v)
			if(wid.gt==w)wid.gv=v
		}
		if(field_is(w)){
			if(wid.ft==w){widget_field(w,wid.f,wid.fv)}
			else{const p=unpack_field(w),v=unpack_field_value(w);widget_field(w,p,v);if(wid.ft==w)wid.fv=v}
		}
	})
	else if(dr.show_widgets&&dr.fatbits)wids.v.map(w=>{draw_boxinv(pal,card_to_fat(unpack_widget(w).size))})
	ev=eb
	if(uimode=='draw'){if(bg_has_sel())draw_handles(livesel);draw_box(livesel,0,ANTS)}
	if(wid.pending_grid_edit){wid.pending_grid_edit=0;modal_enter('gridcell')}
	if(uimode=='object')object_editor()
	if((uimode=='object'&&ob.show_names)||(uimode=='draw'&&dr.show_widgets&&dr.fatbits)||ms.type=='listen'){
		wids.v.map(wid=>{
			const size=card_to_fat(unpack_widget(wid).size)
			const n=ls(ifield(wid,'name')),s=font_textsize(FONT_BODY,n)
			draw_text_outlined(rect(size.x,size.y-s.y,s.x,s.y),n,FONT_BODY)
		})
	}
	if(ob.show_cursor&&ms.type==null&&uimode in {draw:1,object:1}){
		if(uimode=='draw'&&bg_has_sel()){
			let r=livesel,l=r;if(dr.fatbits){const p=fat_to_card(r);l=rint(rect(p.x,p.y,r.w/FAT,r.h/FAT))}
			const t=ls(dyad.format(lms('(%3i,%3i,%3i,%3i)'),lml([l.x,l.y,l.w,l.h].map(lmn)))),s=font_textsize(FONT_BODY,t)
			draw_text_outlined(rect(r.x,r.y-s.y,s.x,s.y),t,FONT_BODY)
		}
		else if(ev.drag){
			const a=dr.fatbits?fat_to_card(ev.dpos):ev.dpos, b=dr.fatbits?fat_to_card(ev.pos):ev.pos, r=rect(b.x,b.y,b.x-a.x,b.y-a.y)
			const t=ls(dyad.format(lms('(%3i,%3i,%3i,%3i)'),lml([r.x,r.y,r.w,r.h].map(lmn)))),s=font_textsize(FONT_BODY,t)
			draw_text_outlined(rect(ev.pos.x,ev.pos.y-s.y,s.x,s.y),t,FONT_BODY)
		}
		else{
			const c=dr.fatbits?fat_to_card(ev.pos):ev.pos
			const t=ls(dyad.format(lms('(%3i,%3i)'),lml([c.x,c.y].map(lmn)))),s=font_textsize(FONT_BODY,t)
			draw_text_outlined(rect(ev.pos.x,ev.pos.y-s.y,s.x,s.y),t,FONT_BODY)
		}
	}
}
gestures=_=>{
	if(!enable_gestures)return
	const card=ifield(deck,'card')
	if(!in_layer()||uimode!='interact'||(!ev.drag&&!ev.mu))return          // must be in the right state of mind
	if(card.widgets.v.some(x=>dover(unpack_widget(x).size)))return         // must touch grass
	const d=rsub(ev.pos,ev.dpos);if(Math.sqrt(d.x*d.x+d.y*d.y)<50)return   // must be emphatic
	if(Math.abs(d.x)<2*Math.abs(d.y)&&Math.abs(d.y)<2*Math.abs(d.x))return // must be highly directional
	const dir=Math.abs(d.x)>2*Math.abs(d.y)?(d.x<0?'left':'right'):(d.y<0?'up':'down')
	image_paste(rect(ev.pos.x-8,ev.pos.y-8,16,16),frame.clip,GESTURES[dir],frame.image,0)
	if(ev.mu)msg.target_navigate=card,msg.arg_navigate=lms(dir)
}

validate_modules=_=>{
	for(let z=0;z<count(deck.modules);z++){
		const err=ifield(deck.modules.v[z],'error');if(!count(err))continue
		modal_enter('alert'),ms.message=rtext_cast(lms(`The module "${ls(deck.modules.k[z])}" failed to initialize: ${ls(err)}`));break
	}
}
load_deck=d=>{
	deck=d, dirty=0, wid.active=-1, wid.hist=[], au.hist=[], doc_hist=[], doc_hist_cursor=0, dr=draw_state()
	FONT_BODY=dget(deck.fonts,lms('body')),FONT_MENU=dget(deck.fonts,lms('menu')),FONT_MONO=dget(deck.fonts,lms('mono'))
	fb=image_make(getpair(ifield(ifield(deck,'card'),'size'))),context=frame=draw_frame(fb),validate_modules(),setmode('interact'),msg.next_view=1
	seed=0|(new Date().getTime()/1000),n_play([NONE,lms('loop')])
}
tick=_=>{
	msg.pending_drag=0,msg.pending_halt=0,frame=context,uicursor=0,fb.pix.fill(0)
	menu_setup(),all_menus(),widget_setup()
	if(uimode=='script'){script_editor()}else{main_view()}
	modals(),gestures(),menu_finish()
	if(uimode=='draw'&&dr.fatbits)draw_icon(rect(frame.size.x-14,2),ZOOM,1)
	if(uimode=='interact'&&ev.drag&&ob.sel.length&&lb(ifield(ob.sel[0],'draggable')))iwrite(ob.sel[0],lms('pos'),lmpair(rsub(ev.pos,ob.prev))),mark_dirty()
	q('#display').style.cursor=uicursor||'default'
	for(let x=0;x<=1;x++)for(let y=0;y<=1;y++)draw_icon(rect(x*(context.size.x-5),y*(context.size.y-5)),CORNERS[x+y*2],1)
	const used=interpret()
	if(uimode=='interact'&&profiler){
		const r=rect(frame.size.x-60,2,50,12)
		draw_text(inset(r,2),ls(dyad.format(lms('%0.2f%%'),lmn(100*used/FRAME_QUOTA))),FONT_BODY,1)
		draw_invert(deck.patterns.pal.pix,rect(r.x+1,r.y,ceil((r.w-2)*(used/FRAME_QUOTA)),r.h)),draw_box(r,0,1)
	}
	if(msg.pending_loop)sfx_doloop()
}

let id=null
sync=_=>{
	toolbars()
	ev.shortcuts={}
	ev.mu=ev.md=ev.click=ev.dclick=ev.tab=ev.action=ev.dir=ev.exit=ev.eval=ev.scroll=ev.rdown=ev.rup=0
	if(ev.clicktime)ev.clicktime--;if(ev.clicklast)ev.clicklast--
	if(ev.pos.x!=ev.dpos.x||ev.pos.y!=ev.dpos.y)ev.clicklast=0
	wid.cursor_timer=(wid.cursor_timer+1)%(2*FIELD_CURSOR_DUTY)
	if(wid.change_timer){wid.change_timer--;if(wid.change_timer==0)field_change()}
	const anim=deck.patterns.anim, pal=deck.patterns.pal.pix, mask=dr.trans_mask&&uimode=='draw'
	const anim_ants   =(x,y)=>(0|((x+y+(0|(frame_count/2)))/3))%2?15:0
	const anim_pattern=(pix,x,y)=>pix<28||pix>31?pix: anim[pix-28][(0|(frame_count/4))%max(1,anim[pix-28].length)]
	const draw_pattern=(pix,x,y)=>pix<2?(pix?1:0): pix>31?(pix==32?0:1): pal_pat(pal,pix,x,y)&1
	const draw_color  =(pix,x,y)=>pix==ANTS?anim_ants(x,y): pix>47?0: pix>31?pix-32: draw_pattern(pix,x,y)?15:0
	if(!id||id.width!=fb.size.x||id.height!=fb.size.y)id=new ImageData(fb.size.x,fb.size.y)
	for(let z=0,d=0,y=0;y<id.height;y++)for(let x=0;x<id.width;x++,z++,d+=4){
		const pix=fb.pix[z], a=anim_pattern(pix,x,y), c=(a==0&&mask)?13:draw_color(a,x,y), cv=COLORS[c]
		id.data[d  ]=0xFF&(cv>>16)
		id.data[d+1]=0xFF&(cv>> 8)
		id.data[d+2]=0xFF&(cv    )
		id.data[d+3]=0xFF
	}
	const r=q('#render');r.getContext('2d').putImageData(id,0,0)
	const g=q('#display').getContext('2d');g.imageSmoothingEnabled=false,g.save(),g.scale(zoom,zoom),g.drawImage(r,0,0),g.restore()
	pending_tick=1,frame_count++
}

move=(x,y)=>{if(!msg.pending_drag)pointer.prev=pointer.pos;pointer.pos=ev.pos=rect(x,y);if(pointer.held)msg.pending_drag=1}
down=(x,y,alt)=>{
	ev.rawdpos=ev.rawpos
	move(x,y),pointer.held=ev.drag=1;pointer.start=ev.dpos=pointer.pos,ev.md=1,ev.clicktime=10;if(alt)ev.rdown=1;initaudio()}
up=(x,y,alt)=>{
	move(x,y),pointer.held=ev.drag=0,pointer.end=pointer.pos,ev.mu=1;if(alt)ev.rup=1
	if(ev.clicktime)ev.click=1;ev.clicktime=0;if(ev.clicklast)ev.dclick=1;ev.clicklast=DOUBLE_CLICK_DELAY
	if(ev.callback&&ev.callback_rect&&over(ev.callback_rect)&&(ev.callback_drag||dover(ev.callback_rect)))ev.callback()
	ev.callback=null,ev.callback_rect=null,ev.callback_drag=0
}
mouse=(e,f)=>{
	ev.rawpos=rect(e.pageX,e.pageY);const c=q('#display').getBoundingClientRect()
	f(0|((e.pageX-c.x)/zoom),0|((e.pageY-c.y)/zoom),e.button!=0); e.preventDefault()
}
touch=(e,f)=>{const t=e.targetTouches[0]||{}; mouse({pageX:t.clientX, pageY:t.clientY, preventDefault:_=>e.preventDefault,button:0},f)}
loop=_=>{tick(),sync(),pump=setTimeout(loop,1000/60)} // 60fps
resize=_=>{
	const b=q('body'), screen=rect(b.clientWidth,b.clientHeight)
	zoom=max(1,0|min(screen.x/fb.size.x,screen.y/fb.size.y))
	tzoom=0|min((screen.x-(zoom*fb.size.x))/(2*toolsize.x),screen.y/toolsize.y)
	const tz=tzoom*toolbars_enable
	const c =q('#display');c .width=fb.size .x*zoom,c.height =fb.size .y*zoom
	const tl=q('#ltools' );tl.width=toolsize.x*tz  ,tl.height=toolsize.y*tz
	const tr=q('#rtools' );tr.width=toolsize.x*tz  ,tr.height=toolsize.y*tz
	const r =q('#render' );r .width=fb.size .x     ,r .height=fb.size .y
	const rl=q('#lrender');rl.width=toolsize.x     ,rl.height=toolsize.y
	const rr=q('#rrender');rr.width=toolsize.x     ,rr.height=toolsize.y
}
window.onresize=_=>{resize(),sync()}
q('body').addEventListener('mousedown'  ,e=>mouse(e,down))
q('body').addEventListener('mouseup'    ,e=>mouse(e,up  ))
q('body').addEventListener('mousemove'  ,e=>mouse(e,move))
q('body').addEventListener('contextmenu',e=>e.preventDefault())
q('body').addEventListener('touchstart' ,e=>{e.preventDefault(),touch(e,down)},{passive:false})
q('body').addEventListener('touchend'   ,e=>touch({targetTouches:e.changedTouches,preventDefault:e.preventDefault},up))
q('body').addEventListener('touchmove'  ,e=>touch(e,move))
q('body').onwheel=e=>ev.scroll=e.deltaY<0?-1:e.deltaY>0?1:0
q('body').onkeydown=e=>{
	initaudio()
	if(e.shiftKey)ev.shift=1
	if(e.key=='ArrowUp'   )ev.dir='up'
	if(e.key=='ArrowDown' )ev.dir='down'
	if(e.key=='ArrowLeft' )ev.dir='left'
	if(e.key=='ArrowRight')ev.dir='right'
	if(e.metaKey||e.ctrlKey)ev.alt=1,ev.shortcuts[e.shiftKey?(e.key.toUpperCase()):e.key]=1
	else if(e.key.length==1&&wid.infield)field_input(e.key)
	else if(uimode=='draw'&&ms.type==null){if(e.key=='Backspace'||e.key=='Delete')bg_delete_selection()}
	else if(uimode=='object'&&ms.type==null){if(e.key=='Backspace'||e.key=='Delete')ob_destroy()}
	else if(ms.type=='recording'&&!wid.infield&&au.mode=='stopped'){if(e.key=='Backspace'||e.key=='Delete')sound_delete()}
	if     (wid.ingrid )grid_keys (e.key,e.shiftKey)
	else if(wid.infield)field_keys(e.key,e.shiftKey)
	if(uimode=='script')sc.status=''
	if(e.key==' '&&!wid.infield)ev.action=1
	if(e.key=='Enter')ev.action=1
	if(e.key=='Tab')ev.tab=1
	if(e.key=='l'&&ms.type==null&&!wid.ingrid&&!wid.infield)ev.shortcuts['l']=1
	if((e.metaKey||e.ctrlKey)&&e.key in {c:1,x:1,v:1}){}
	else{e.preventDefault()}
}
q('body').onkeyup=e=>{
	if(e.key=='Meta'||e.key=='Control'||e.metaKey||e.ctrlKey)ev.alt=0
	if(e.key=='Enter'&&e.shiftKey)ev.eval=1
	if(e.key=='Shift'||e.shiftKey)ev.shift=0
	if(e.key=='m'&&uimode=='draw'&&in_layer())ev.hidemenu^=1
	if(e.key=='t'&&uimode=='draw'&&in_layer())dr.trans^=1
	if(e.key=='Escape')ev.exit=1
	if(!wid.infield&&uimode=='interact'){
		if(e.key=='ArrowUp'   )msg.target_navigate=ifield(deck,'card'),msg.arg_navigate=lms('up'   )
		if(e.key=='ArrowDown' )msg.target_navigate=ifield(deck,'card'),msg.arg_navigate=lms('down' )
		if(e.key=='ArrowLeft' )msg.target_navigate=ifield(deck,'card'),msg.arg_navigate=lms('left' )
		if(e.key=='ArrowRight')msg.target_navigate=ifield(deck,'card'),msg.arg_navigate=lms('right')
	}
	e.preventDefault()
}
let local_clipboard=''
getclipboard=after=>{
	const t=document.createElement('textarea');t.style.top='0',t.style.left='0',t.position='fixed',t.onpaste=e=>{e.stopPropagation()}
	document.body.appendChild(t),t.focus(),t.select();const success=document.execCommand('paste');const x=t.value;document.body.removeChild(t)
	if(success){after(x);return}                                       // this approach seems to work fine for Safari
	try{navigator.clipboard.readText().then(text=>{after(text)},_=>_)} // this works in Chrome, but *explicitly* does not work in Firefox
	catch(e){after(local_clipboard)}                                   // so Firefox gets to use a baby-jail local clipboard
}
setclipboard=x=>{
	local_clipboard=x
	const t=document.createElement('textarea');t.value=x,t.style.top='0',t.style.left='0',t.position='fixed',t.oncopy=e=>{e.stopPropagation()}
	document.body.appendChild(t),t.focus(),t.select();const success=document.execCommand('copy');document.body.removeChild(t);if(success)return;
	try{navigator.clipboard.writeText(x).then(_=>_,_=>_)}catch(e){console.log(e)} // generally not necessary, but worth having as a last resort
}
docut=_=>{
	if(wid.fv){
		const s=rtext_span(wid.fv.table,wid.cursor),i=rtext_is_image(s);field_keys('Delete',0)
		return i?image_write(i):ls(rtext_string(s))
	}
	else if(ms.type=='recording'&&au.mode=='stopped'){const r=sound_write(sound_selected());sound_delete();return r}
	else if(ms.type==null&&uimode=='draw'&&(bg_has_sel()||bg_has_lasso())){
		const i=bg_has_lasso()?image_mask(dr.limbo,dr.mask): dr.limbo?bg_scaled_limbo():bg_copy_selection(dr.sel_here)
		bg_scoop_selection(),bg_delete_selection();return image_write(i)
	}
	else if(ms.type==null&&uimode=='object'&&ob.sel.length){ob_order();const r=ls(card_copy(ifield(deck,'card'),lml(ob.sel)));ob_destroy();return r}
	return null
}
docopy=_=>{
	if(wid.gv){return ls(n_writecsv([wid.gv.table,grid_format()]))}
	else if(wid.fv){
		const s=rtext_span(wid.fv.table,wid.cursor),i=rtext_is_image(s)
		return i?image_write(i):ls(rtext_string(s))
	}
	else if(ms.type=='recording'&&au.mode=='stopped'){return sound_write(sound_selected())}
	else if(ms.type==null&&uimode=='draw'&&(bg_has_sel()||bg_has_lasso())){
		const i=bg_has_lasso()?image_mask(dr.limbo,dr.mask): dr.limbo?bg_scaled_limbo():bg_copy_selection(dr.sel_here);return image_write(i)
	}
	else if(ms.type==null&&uimode=='object'&&ob.sel.length){ob_order();return ls(card_copy(ifield(deck,'card'),lml(ob.sel)))}
	return null
}
dopaste=x=>{
	if(ms.type==null&&/^%%IMG[012]/.test(x)){
		const i=image_read(x);if(i.size.x==0||i.size.y==0)return
		if(wid.fv){
			if(wid.f.style!='rich'){field_input(x)}
			else{field_edit(lms(''),i,'i',wid.cursor)}
		}else{setmode('draw'),bg_paste(i)}
	}
	else if(ms.type=='recording'&&au.mode=='stopped'&&/^%%SND0/.test(x)){sound_edit(sound_replace(sound_read(x)))}
	else if(ms.type==null&&/^%%WGT0/.test(x)){setmode('object'),card_paste(ifield(deck,'card'),lms(x))}
	else if(ms.type==null&&/^%%CRD0/.test(x)){
		const c=deck_paste(deck,lms(x))
		const card=ifield(deck,'card'), n=ln(ifield(card,'index'));iwrite(c,lms('index'),lmn(n+1)),n_go([c],deck)
	}
	else if(wid.gv&&!wid.g.locked&&ms.type==null){grid_edit(n_readcsv([lms(x),lms(wid.g.format)]))}
	else if(wid.fv){field_input(x)}
}
cutcard=_=>{const c=ifield(deck,'card');setclipboard(ls(deck_copy(deck,c))),deck_remove(deck,c),mark_dirty()}
copycard=_=>{const c=ifield(deck,'card');setclipboard(ls(deck_copy(deck,c)))}
copycanvasimg=_=>{setclipboard(image_write(canvas_image(ob.sel[0],1))),frame=context}
pasteascanvas=_=>getclipboard(t=>{ob_create([lmd([lms('type'),lms('image')],[lms('canvas'),lms(t)])]),frame=context})
pasteintocanvas=_=>getclipboard(t=>{const i=image_read(t),c=ob.sel[0];iwrite(c,lms('size'),ifield(i,'size')),c.image=i})
menucut=_=>{const r=docut();if(r)setclipboard(r)}
menucopy=_=>{const r=docopy();if(r)setclipboard(r)}
menupaste=_=>getclipboard(t=>{if(t.length)dopaste(t)})
document.oncut=e=>{const r=docut();if(r)e.clipboardData.setData('text/plain',r),local_clipboard=r;e.preventDefault()}
document.oncopy=e=>{const r=docopy();if(r)e.clipboardData.setData('text/plain',r),local_clipboard=r;e.preventDefault()}
document.onpaste=e=>{
	const src=e.clipboardData||e.originalEvent.clipboardData
	if(src.items[0]){
		const file=src.items[0].getAsFile()
		if(file&&/^image\//.test(file.type)){load_image(file);return}
		if(file&&/^audio\//.test(file.type)){load_sound(file);return}
	}
	dopaste(src.getData('text/plain')),e.preventDefault()
}
q('body').ondragover=e=>e.preventDefault()
q('body').ondrop=e=>{
	e.preventDefault();const file=e.dataTransfer.files.item(0);if(!file)return
	if(/\.(psv|csv)$/i.test(file.name)&&!has_parent(ifield(deck,'card'))){
		file.text().then(t=>{
			const data=n_readcsv([lms(t),NONE,lms(file.type=='text/csv'?',':'|')])
			setmode('object'),ob_create([lmd([lms('type'),lms('value')],[lms('grid'),monad.cols(data)])])
		})
	}
	if(/\.(html|deck)$/i.test(file.name)){
		file.text().then(t=>{
			modal_enter('resources'),ms.message=deck_read(t),ms.grid=gridtab(res_enumerate(ms.message))
		})
	}
	if(/^image\//.test(file.type)){load_image(file)}
	if(/^audio\//.test(file.type)){load_sound(file)}
}

pushstate(lmenv()),load_deck(deck_read(q('script[language="decker"]').innerText))
const tag=decodeURI(document.URL.split('#')[1]||'');if(tag.length)iwrite(deck,lms('card'),lms(tag))
resize(),(pump&&clearTimeout(pump)),loop()
