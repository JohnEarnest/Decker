
// potentially unsafe/nonportable scripting APIs

function js_to_lil(x){
	if(x==null||x==undefined)return NIL
	if(deck_is(x)||card_is(x)||widget_is(x))return x
	if('number'==typeof x)return lmn(x)
	if('string'==typeof x)return lms(clchars(x))
	if(Array.isArray(x))return lml(x.slice(0).map(js_to_lil))
	if('object'==typeof x){
		if(Object.getPrototypeOf(x)==Uint8Array.prototype)return array_make(x.length,'u8',0,x)
		return lmd(Object.keys(x).map(js_to_lil),Object.values(x).map(js_to_lil))
	}
	if('function'==typeof x)return lmnat(args=>js_to_lil(x.apply(null,args.map(lil_to_js))))
	return NIL
}
function lil_to_js(x){
	if(deck_is(x)||card_is(x)||widget_is(x))return x
	if(lin(x))return ln(x)
	if(lis(x))return ls(x)
	if(lil(x))return ll(x).slice(0).map(lil_to_js)
	if(lid(x))return x.k.reduce((r,k,i)=>{r[ls(k)]=lil_to_js(x.v[i]);return r},{})
	if(array_is(x))return x.slice?null: x.data
	if(lion(x))return (...args)=>{
		// note that this ignores quota, and can therefore lock up Decker if misused!
		const p=lmblk();blk_lit(p,x),blk_lit(p,js_to_lil(args)),blk_op(p,op.CALL)
		const e=lmenv();pushstate(e),issue(e,p);while(running())runop();const r=arg();popstate()
		return lil_to_js(r)
	}
	return null
}
interface_danger=lmi((self,i,x)=>{
	if(ikey(i,'js'))return lmnat(args=>{
		try{
			let r=args[0]==undefined?null:eval(ls(args[0]))
			if(('function'==typeof r)&&(args.length>1)){r=r.apply(null,args.slice(1).map(lil_to_js))}
			return js_to_lil(r)
		}catch(e){console.log('danger.js[] error',e);return NIL}
	})
	return x?x:NIL
},'danger')
ext_add_constant=(k,v)=>{ext_constants[k]=js_to_lil(v)}
endanger=_=>{ext_constants.danger=interface_danger}
if(DANGEROUS)endanger()

