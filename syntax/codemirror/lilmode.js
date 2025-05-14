/**
* Lil Syntax mode for CodeMirror:
* https://codemirror.net
**/

CodeMirror.defineMode('lil',(config,options)=>{
	const KEYWORDS={
		while:1,each:1,send:1,on:1,if:1,elseif:1,else:1,end:1,do:1,with:1,local:1,select:1,extract:1,update:1,insert:1,
		into:1,from:1,where:1,by:1,orderby:1,asc:1,desc:1,
	}
	const VERBS={
		// monads
		floor:1,cos:1,sin:1,tan:1,exp:1,ln:1,sqrt:1,unit:1,mag:1,heading:1,sum:1,prod:1,raze:1,max:1,min:1,count:1,
		first:1,last:1,keys:1,range:1,list:1,typeof:1,flip:1,rows:1,cols:1,table:1,
		// dyads
		split:1,fuse:1,dict:1,take:1,drop:1,limit:1,in:1,unless:1,join:1,cross:1,parse:1,format:1,like:1,window:1,fill:1,
	}
	return{
		lineComment:'# ',
		startState:_=>({mode:0}),
		copyState:x=>({mode:x.mode}),
		token:(stream,state)=>{
			// lil supports multiline strings, which require some special handling in codemirror:
			if(state.mode=='str'){
				if(stream.match('"')){state.mode=0;return'string'}
				if(stream.match(/\\[\\n"]/))return'atom' // valid escape sequence
				if(stream.match('\\'))return'string'     // bogus escape sequence
				stream.match(/^[^"\\]*/);return'string'
			}
			if(stream.match('"')){state.mode='str';return'string'}
			// line comments, keywords/builtins, function definitions:
			if(stream.match('#')||stream.match(/[\t\n\r ]+\//))return stream.skipToEnd(),'comment'
			const n=stream.match(/^[a-zA-Z_?][a-zA-Z0-9_?]*/)
			if(state.mode=='fname'&&n){state.mode=0;return 'property'}
			if(n=='on'){state.mode='fname'}
			if(KEYWORDS[n])return 'keyword'
			if(VERBS[n])return 'variable-2'
			// calls/indexing:
			if(n&&stream.match(/^[ \t]*\[/,false))return 'variable-2'
			if(!n)stream.next()
			return null
		}
	}
})
