(window.KR=window.KR||{})["espantalho/halloween"]=(function(){
const drawDummy=(()=>{// Espantalho skin 1 — Pumpkin Test Dummy. Independent cosmetic SVG lab.
// Rig follows queueScarecrow: fixed base, six-coil spring, crossbar, hanging gloves.
const ink='#211a25',yellow='#ea7324',shade='#a44424',white='#f8d873',n=v=>Number(v.toFixed(4))||0;
const DUMMY_PUMPKIN_PERIOD=6;
function dummyPumpkinFrame(time=0,state='normal',view='front'){
 const t=n(((time%6)+6)%6),a=t*Math.PI/3,s=Math.sin(a),q=Math.sin(a*2),p=t-1;
 const active=p>=0&&p<2.8,decay=active?Math.pow(1-p/2.8,2):0,hit=decay*Math.sin(p*Math.PI*4),lift=active?Math.sin(p/2.8*Math.PI):0;
 const impact=['hit','body','uppercut'].includes(state),x=s*.8+(state==='hit'?hit*15:0),y=q*.6+(state==='body'?lift*9:state==='uppercut'?-lift*15:0);
 const angle=s*1.1+(state==='hit'?-hit*12:0),sx=state==='body'?1+lift*.1:state==='uppercut'?1-lift*.06:1,sy=state==='body'?1-lift*.14:state==='uppercut'?1+lift*.11:1;
 const radians=angle*Math.PI/180,ex=x+20*sx*Math.cos(radians)+39*sy*Math.sin(radians),ey=-46+y+20*sx*Math.sin(radians)-39*sy*Math.cos(radians);
 const gl=(-6+q*3.5-angle*.7)*Math.PI/180,gr=(6+q*3.5-angle*.7)*Math.PI/180;
 const spring=Array.from({length:6},(_,i)=>{const t0=i/6,t1=(i+1)/6,cx=(i%2?-12:12)+x*(t0+.08),cy=-14+(-32+y)*(t0+1/12);return ' Q '+n(cx)+' '+n(cy)+' '+n(x*t1)+' '+n(-14+(-32+y)*t1);}).join('');
 const blaze=state==='approved'?10:state==='uppercut'?-12:0;
 return {time:t,
 fire:'M -18 -126 Q '+n(-24+s*2)+' -139 -13 '+n(-147-blaze)+' L -12 -138 Q -6 '+n(-151-blaze-q*3)+' -4 '+n(-155-blaze)+' Q 3 -147 1 -139 Q 11 '+n(-149-blaze)+' 10 '+n(-155-blaze+s*3)+' Q 26 -142 18 -126 Z',
 fireInner:'M -11 -127 Q -12 -137 -6 '+n(-145-blaze*.6+q*2)+' Q 0 -136 2 -131 Q 8 -137 10 '+n(-144-blaze*.5+s*2)+' Q 17 -135 12 -126 Z',
 tails:'M -25 -36 Q '+n(-32-s*2)+' -19 '+n(-35-s*3)+' -2 L -26 -8 L -25 1 L -16 -9 L -13 -4 L -12 -31 Z M 25 -36 Q '+n(32+s*2)+' -19 '+n(35+s*3)+' -2 L 26 -8 L 25 1 L 16 -9 L 13 -4 L 12 -31 Z',
 bats:'translate('+n(s*3)+' '+n(q*3)+')',
 targetOrbit:'rotate('+n(t*60)+')',scan:'translate(0 '+n((1+s)*27)+')',cubes:'translate('+n(s*2)+' '+n(q*3)+')',upper:'translate('+n(x)+' '+n(-46+y)+')',sway:'rotate('+n(angle)+')',squash:'scale('+n(sx)+' '+n(sy)+')',gloveLeft:'rotate('+n(-6+q*3.5-angle*.7)+')',gloveRight:'rotate('+n(6+q*3.5-angle*.7)+')',head:'rotate('+n(state==='uppercut'?hit*5:s*.5)+' 0 -108)',
 spring:'M 0 -14'+spring,
 sensorLeft:'M -27 -69 Q '+n(-36+s*1.6)+' -55 '+n(-40-21*Math.sin(gl))+' '+n(-70+21*Math.cos(gl)),
 sensorRight:'M 27 -69 Q '+n(36-s*1.6)+' -55 '+n(40-21*Math.sin(gr))+' '+n(-70+21*Math.cos(gr)),
 baseWire:'M 24 -3 C '+n(23+x*.6)+' '+n(-30+y*.5)+' '+n(29+x*.5)+' '+n(-40+y*.7)+' '+n(ex)+' '+n(ey),
 lamp:n(.85+.12*Math.sin(a*4)),meter:n(impact?Math.abs(hit):.2+.1*s),reaction:n(hit)};
}


function web(x,y,r=12){
 return '<g data-part="autumn-cobweb" transform="translate('+x+' '+y+')" fill="none" stroke="#b8a0c3" stroke-width=".65" opacity=".8"><path d="M '+(-r)+' 0 H '+r+' M 0 '+(-r)+' V '+r+' M '+(-r*.7)+' '+(-r*.7)+' L '+r*.7+' '+r*.7+' M '+r*.7+' '+(-r*.7)+' L '+(-r*.7)+' '+r*.7+'"/>'+[.4,.75].map(s=>'<path d="M 0 '+(-r*s)+' Q '+r*s*.2+' '+(-r*s*.4)+' '+r*s*.7+' '+(-r*s*.7)+' Q '+r*s*.4+' '+(-r*s*.2)+' '+r*s+' 0 Q '+r*s*.4+' '+r*s*.2+' '+r*s*.7+' '+r*s*.7+' Q '+r*s*.2+' '+r*s*.4+' 0 '+r*s+' Q '+(-r*s*.2)+' '+r*s*.4+' '+(-r*s*.7)+' '+r*s*.7+' Q '+(-r*s*.4)+' '+r*s*.2+' '+(-r*s)+' 0 Q '+(-r*s*.4)+' '+(-r*s*.2)+' '+(-r*s*.7)+' '+(-r*s*.7)+' Q '+(-r*s*.2)+' '+(-r*s*.4)+' 0 '+(-r*s)+' Z"/>').join('')+'</g>';
}
function glove(side,view,f){
 const left=side==='left',x=left?-40:40;
 return '<g transform="translate('+x+' -70)"><g data-cel-transform="'+(left?'gloveLeft':'gloveRight')+'" transform="'+f[left?'gloveLeft':'gloveRight']+'"><path d="M 0 0 Q -3 9 0 22" stroke="'+ink+'" stroke-width="3"/><path d="M 0 1 Q -2 11 0 21" stroke="#9b8750" stroke-width="1"/><g data-part="twisted-branch-boxing-fist" stroke="'+ink+'" stroke-width="1.6" stroke-linejoin="round"><path d="M -8 18 H 8 L 6 27 H -6 Z" fill="#425534"/><path d="M -10 26 L -6 23 L 7 24 L 12 29 L 10 43 L 5 47 L -8 44 L -12 36 Z" fill="#765036"/><path d="M -9 28 L -5 26 L -3 39 L -5 43 L -9 40 Z M -2 25 L 3 26 L 4 42 L 0 45 Z M 5 26 L 9 28 L 8 40 L 5 43 Z" fill="#9c6c42" stroke-width="1"/><path d="M '+(left?'9 29 L 15 32 L 12 39 L 8 40':'-9 29 L -15 32 L -12 39 L -8 40')+' Z" fill="#7a573b"/><path d="M -7 30 L -6 36 M 0 29 L 1 38 M 6 30 L 6 36" stroke="#c1925a" stroke-width=".8"/><path d="M -8 21 L 7 24 M -7 25 L 6 27" stroke="#a8a376" stroke-width="1.4"/><path d="M -8 39 Q 0 36 9 42" fill="none" stroke="#3a6b3b" stroke-width="1.5"/></g></g></g>';
}
function dummyPumpkinHands({view='front',time=0,state='normal'}={}){
 const f=dummyPumpkinFrame(time,state,view);return glove('left',view,f)+glove('right',view,f);
}
function face(f,state,view){
 const back=view==='back',hurt=['hit','body','uppercut'].includes(state),id='pumpkin-'+view+'-'+state;
 const pumpkin='M -22 -117 C -28 -131 -14 -134 -6 -130 Q 0 -134 7 -130 C 20 -135 29 -124 24 -112 Q 29 -95 16 -88 Q 7 -82 0 -87 Q -10 -82 -19 -91 Q -28 -99 -22 -117 Z';
 const eyes=hurt?'<path d="M -17 -114 L -8 -107 L -17 -104 L -11 -111 Z M 17 -114 L 8 -107 L 17 -104 L 11 -111 Z" fill="#ffe789" stroke="#813923" stroke-width="1.8"/>':'<path d="M -18 -116 L -6 -111 L -11 -103 Z M 18 -116 L 6 -111 L 11 -103 Z" fill="#ffe789" stroke="#813923" stroke-width="2"/>';
 const mouth=hurt?'<path d="M -5 -101 L 4 -102 L 8 -96 L 4 -90 L -4 -90 L -8 -96 Z" fill="#ffdd66" stroke="#813923" stroke-width="2"/>':'<path d="M -19 -103 L -12 -99 L -9 -102 L -5 -96 L 0 -99 L 4 -96 L 8 -101 L 12 -99 L 19 -104 L 15 -94 L 8 -91 L 5 -94 L 1 -90 L -4 -94 L -8 -91 L -15 -95 Z" fill="#ffdd66" stroke="#813923" stroke-width="1.8" stroke-linejoin="round"/>';
 return '<g data-cel-transform="head" transform="'+f.head+'"><path d="M -4 -129 L -5 -138 L 1 -143 L 5 -140 L 1 -136 L 4 -129 Z" fill="#436432" stroke="'+ink+'" stroke-width="1.5"/><g data-part="carved-pumpkin-head"><path d="'+pumpkin+'" fill="#d96127" stroke="'+ink+'" stroke-width="2.8"/><path d="M -6 -130 C -17 -119 -17 -98 -7 -87 M 7 -130 C 18 -118 17 -98 7 -87 M 0 -130 V -87" stroke="#aa4425" stroke-width="1.4" fill="none"/><path d="M -17 -124 Q -23 -115 -20 -104 M -6 -126 Q -11 -119 -11 -116" stroke="#f39c3e" stroke-width="2" fill="none"/>'+(back?'<path d="M -16 -118 Q -20 -102 -13 -94 M 14 -118 Q 19 -105 13 -95" fill="none" stroke="#e87e31" stroke-width="1.8"/><path d="M -7 -128 L -3 -123 M 5 -128 L 9 -123" stroke="#f3a445" stroke-width="1"/>':'<g data-part="candlelit-carved-eyes-and-toothed-mouth" data-live-lamp="0" opacity="'+f.lamp+'">'+eyes+'<path d="M 0 -110 L -4 -104 H 4 Z" fill="#ffd55f" stroke="#813923" stroke-width="1.2"/>'+mouth+'</g>')+'</g></g>';
}
function renderDummyPumpkinSvg({view='front',state='normal',time=0,armed=true}={}){
 const f=dummyPumpkinFrame(time,state,view),back=view==='back',d='M -16 -82 C -30 -70 -34 -45 -28 -20 L -23 -14 L -17 -19 L -11 -13 L -4 -17 L 3 -12 L 10 -17 L 16 -13 L 21 -19 L 28 -20 C 34 -45 30 -70 16 -82 Z';
 const base='<g data-part="fixed-haunted-timber-base" stroke="'+ink+'" stroke-width="2.3"><ellipse cy="2" rx="51" ry="8" fill="#100f18" opacity=".4" stroke="none"/><path d="M -10 4 L -6 -14 H 6 L 10 4 Z" fill="#4c4230"/><rect x="-44" y="-8" width="88" height="10" rx="2" fill="#665034"/><path d="M -40 -5 L -20 -4 L -3 -6 M 6 -5 L 18 -3 L 38 -5" stroke="#a0814e" stroke-width="1"/><rect x="-14" y="-14" width="28" height="8" rx="2" fill="#53534b" stroke-width="1.6"/>'+[-32,32].map(x=>'<circle cx="'+x+'" cy="-3" r="2.5" fill="#84816a" stroke-width=".8"/>').join('')+'<path d="M -37 0 L -30 -2 L -23 0 M 20 1 L 28 -2 L 37 0" fill="none" stroke="#52693d" stroke-width="2"/><path d="M -43 0 L -47 3 M 43 0 L 47 3" stroke="#3d4e31" stroke-width="1.5"/></g>';
 const spring='<g data-part="six-coil-rusty-haunted-spring"><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="'+ink+'" stroke-width="7"/><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="#8b805f" stroke-width="2.4"/><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="#b5a678" stroke-width=".7"/></g>';
 const post='<g data-part="original-scarecrow-crossbar-rig" stroke="'+ink+'" stroke-width="2.4" stroke-linejoin="round"><path d="M -5 0 L -7 -133 L 0 -137 L 7 -133 L 5 0 Z" fill="#685034"/><path d="M -3 -132 L -2 -3" stroke="#a7804e" stroke-width="1"/><path d="M -52 -74 L -26 -75 L 0 -72 L 25 -75 L 52 -73 L 55 -67 L 29 -65 L 0 -66 L -28 -65 L -53 -66 Z" fill="#6e5335"/><path d="M -48 -71 L -27 -72 M 28 -71 L 48 -70" stroke="#a58250" stroke-width="1"/><path d="M -52 -69 L -60 -73 L -56 -68 L -62 -65 L -51 -66 M 52 -69 L 60 -73 L 56 -68 L 62 -65 L 51 -66" fill="#74683d" stroke-width="1.3"/><path d="M -45 -71 Q -43 -81 -37 -77 Q -40 -71 -45 -71 M 43 -70 Q 49 -80 51 -74 Q 49 -69 43 -70" fill="#49663b" stroke-width="1.1"/></g>';
 const tails='<g data-part="windblown-torn-purple-coattails"><path data-cel-path="tails" d="'+f.tails+'" fill="#452e57" stroke="'+ink+'" stroke-width="1.9"/></g>';
 const torso='<g data-part="ragged-autumn-overcoat-original-sack-silhouette"><path d="'+d+'" fill="#664375" stroke="'+ink+'" stroke-width="3"/><path d="M 6 -81 H 16 C 30 -70 34 -45 28 -20 L 21 -19 L 16 -13 L 10 -17 Q 19 -49 6 -81 Z" fill="#422c52"/><path d="M -20 -74 Q -28 -56 -26 -38" stroke="#9b689e" stroke-width="1.5" fill="none"/>'+(back?web(0,-54,21)+'<path d="M -4 -78 L 2 -73 L -2 -69 L 3 -63" fill="none" stroke="#c19b6b" stroke-width="1.2"/><ellipse cx="10" cy="-43" rx="2" ry="2.8" fill="#251d31"/><path d="M 8 -45 L 5 -47 M 8 -43 L 4 -43 M 8 -41 L 5 -39 M 12 -45 L 15 -47 M 12 -43 L 16 -43 M 12 -41 L 15 -39" stroke="#251d31" stroke-width=".8"/>':'<path d="M -9 -79 H 9 L 14 -28 L 8 -22 L 0 -26 L -7 -22 L -14 -28 Z" fill="#3a5334" stroke="'+ink+'" stroke-width="1.4"/><path d="M -16 -81 L -5 -77 L -11 -67 L -8 -61 L -19 -64 Z M 16 -81 L 5 -77 L 11 -67 L 8 -61 L 19 -64 Z" fill="#80528c" stroke="'+ink+'" stroke-width="1.4"/>'+[-58,-44,-31].map(y=>'<path d="M -4 '+y+' l 8 0" stroke="#c7b78f" stroke-width="2.2" stroke-linecap="round"/><path d="M 0 '+(y-2)+' v 4" stroke="#7b7655" stroke-width=".7"/>').join('')+web(18,-53,10)+'<path d="M -24 -37 L -16 -40 L -14 -30 L -23 -27 Z" fill="#aa7340" stroke="'+ink+'" stroke-width="1.1"/><path d="M -25 -35 L -15 -36 M -23 -30 L -15 -31" stroke="#ccb382" stroke-width=".7"/>')+'<path d="M -25 -25 Q 0 -18 25 -25" fill="none" stroke="#93865d" stroke-width="2"/><path d="M -3 -22 Q -7 -14 -5 -7 M 2 -22 Q 7 -15 4 -9" fill="none" stroke="#a59766" stroke-width="1.5"/><path d="M -18 -82 L -24 -87 L -11 -83 L -13 -91 L -4 -85 L 0 -91 L 5 -84 L 13 -90 L 11 -83 L 24 -87 L 18 -80 Z" fill="#8d7540" stroke="'+ink+'" stroke-width="1.2"/><path d="M -12 -85 L -6 -82 M 10 -85 L 6 -82" stroke="#c3a458" stroke-width="1"/></g>';
 const bats=state==='approved'?'<g data-part="victory-flying-miniature-bats"><g data-cel-transform="bats" transform="'+f.bats+'">'+[[-43,-182,1],[38,-191,.8],[-18,-207,.55]].map(([x,y,s])=>'<g transform="translate('+x+' '+y+') scale('+s+')"><path d="M 0 1 L -4 -5 L -12 -8 L -8 -1 L -6 -3 L -4 2 L -1 3 L 0 5 L 1 3 L 4 2 L 6 -3 L 8 -1 L 12 -8 L 4 -5 Z" fill="#654378" stroke="'+ink+'" stroke-width="1.1"/><path d="M -1 -1 L -1 -3 L 0 -2 L 1 -3 L 1 -1" fill="#a27098" stroke="none"/></g>').join('')+'</g></g>':'';
 return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="-95 -240 190 260" width="570" height="780" data-skin="dummy-pumpkin" data-state="'+state+'" role="img" aria-label="Espantalho Halloween — '+view+' — '+state+'">'+base+spring+'<g data-cel-transform="upper" transform="'+f.upper+'"><g data-cel-transform="sway" transform="'+f.sway+'"><g data-cel-transform="squash" transform="'+f.squash+'">'+post+tails+(armed?dummyPumpkinHands({view,time,state}):'')+torso+face(f,state,view)+'</g></g></g>'+bats+'</svg>';
}
function renderDummyPumpkinAnimatedSvg(options={}){
 const state=options.state||'normal',view=options.view||'front',times=Array.from({length:121},(_,i)=>i/20);
 let svg=renderDummyPumpkinSvg({...options,time:0});
 svg=svg.replace(/(<path data-cel-path="([^"]+)"[^>]+)\/>/g,(_,tag,part)=>tag+'><animate attributeName="d" values="'+times.map(t=>dummyPumpkinFrame(t,state,view)[part]).join(';')+'" dur="6s" repeatCount="indefinite"/></path>');
 svg=svg.replace(/(<g data-cel-transform="([^"]+)" transform="[^"]+">)/g,(_,tag,part)=>{
 if(part==='targetOrbit')return tag+'<animateTransform attributeName="transform" type="rotate" from="0" to="360" dur="6s" repeatCount="indefinite"/>';
 const type=['upper','scan','cubes','bats'].includes(part)?'translate':part==='squash'?'scale':'rotate';
 const values=times.map(t=>dummyPumpkinFrame(t,state,view)[part].match(/\(([^)]+)\)/)[1]);
 return tag+'<animateTransform attributeName="transform" type="'+type+'" values="'+values.join(';')+'" dur="6s" repeatCount="indefinite"/>';
 });
 svg=svg.replace(/(<g data-live-lamp="0"[^>]*>)/g,(_,tag)=>tag+'<animate attributeName="opacity" values="'+times.map(t=>dummyPumpkinFrame(t,state,view).lamp).join(';')+'" dur="6s" repeatCount="indefinite"/>');
 return svg;
}

return o=>{if(o.phase)o.phase=Number(o.phase);return renderDummyPumpkinSvg(o);};})();const skinId=3;const DUMMY_ACTIONS={"idle":{"kind":0,"dir":1,"duration":3.2,"loop":true,"label":"Parado · mola"},"hit1":{"kind":1,"dir":1,"duration":0.26666666666666666,"label":"Recebe jab · esquerda"},"hit2":{"kind":2,"dir":1,"duration":0.4,"label":"Recebe gancho · esquerda"},"hit3":{"kind":3,"dir":1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · esquerda"},"hit4":{"kind":4,"dir":1,"duration":0.6333333333333333,"label":"Recebe uppercut · esquerda"},"dummy_hit1_R":{"kind":1,"dir":-1,"duration":0.26666666666666666,"label":"Recebe jab · direita"},"dummy_hit2_R":{"kind":2,"dir":-1,"duration":0.4,"label":"Recebe gancho · direita"},"dummy_hit3_R":{"kind":3,"dir":-1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · direita"},"dummy_hit4_R":{"kind":4,"dir":-1,"duration":0.6333333333333333,"label":"Recebe uppercut · direita"},"dummy_hit0_L":{"kind":0,"dir":1,"duration":0.4,"label":"Recebe impacto genérico · esquerda"},"dummy_hit0_R":{"kind":0,"dir":-1,"duration":0.4,"label":"Recebe impacto genérico · direita"},"dummy_cross_L":{"kind":2,"dir":1,"duration":0.3333333333333333,"label":"Recebe direto · esquerda"},"dummy_cross_R":{"kind":2,"dir":-1,"duration":0.3333333333333333,"label":"Recebe direto · direita"}};
// Direct transcription of queueScarecrow's procedural reaction equations.
const n=x=>+x.toFixed(5),clamp=p=>Math.max(0,Math.min(1,p));
function nodes(s){const out=[],re=/<\/?([\w:-]+)\b[^>]*>/g;let start=-1,depth=0,m;while((m=re.exec(s))){const close=m[0].startsWith('</'),self=m[0].endsWith('/>');if(!close&&!depth)start=m.index;if(!close&&!self)depth++;if(close)depth--;if(!depth&&start>=0){out.push(s.slice(start,re.lastIndex));start=-1;}}return out;}
const inside=s=>s.replace(/^\s*<[^>]+>/,'').replace(/<\/[^>]+>\s*$/,'');
function nativePose(kind,p,dir,time,hit){
 let angle=0,x=0,y=0,sx=1,sy=1,pop=0,tilt=0,vibrate=0;const frame=Math.floor(time*60),decay=Math.pow(1-p,1.3),wave=Math.sin(p*Math.PI);
 if(hit){switch(kind){
  case 1:angle=-dir*.12*decay*Math.sin(p*Math.PI*5);x=-dir*8*decay*Math.sin(p*Math.PI*5);vibrate=decay*Math.sin(p*Math.PI*8)*4;break;
  case 2:angle=-dir*.28*decay*Math.sin(p*Math.PI*3.5);x=-dir*20*decay*Math.sin(p*Math.PI*3.5);vibrate=decay*Math.sin(p*Math.PI*6)*6;tilt=dir*.22*decay;pop=8*wave;break;
  case 3:sx=1+.14*wave;sy=1-.20*wave;y=12*wave;vibrate=decay*5;break;
  case 4:sy=1+.24*wave;sx=1-.12*wave;y=-22*wave;pop=12*wave;tilt=(frame%2===0?.14:-.14)*wave;break;
  default:angle=-dir*.15*decay*Math.sin(p*Math.PI*4);x=-dir*12*decay;break;
 }}else{angle=Math.sin(time*60*.05)*.02;x=Math.sin(time*60*.05)*1.5;y=Math.sin(time*60*.08);}
 const swing=-angle*42+Math.sin(time*60*.06)*3.5,topX=x*.3+vibrate,topY=-46+y*.3;
 const spring='M0 -14'+Array.from({length:6},(_,i)=>{const a=i/6,b=(i+1)/6;return ` Q${n((i%2?-12:12)+topX*a)} ${n(-14+(topY+14)*(a+1/12))} ${n(topX*b)} ${n(-14+(topY+14)*b)}`;}).join('');
 return{angle,x,y,sx,sy,pop,tilt,vibrate,swing,spring,hurt:hit,shock:hit&&(kind===2||kind===4)};
}
function adapt(o,P,state){
 let svg=drawDummy({...o,state,time:skinId===0?0:o.time,animate:false});
 const upper=`translate(${n(P.x)} ${n(-46+P.y)})`,sway=`rotate(${n(P.angle*180/Math.PI)})`,squash=`scale(${n(P.sx)} ${n(P.sy)})`,hat=`translate(0 ${n(-P.pop)}) rotate(${n(P.tilt*180/Math.PI)} 0 -126)`;
 function walk(s){
  let tag=s.match(/^<[^>]+>/)?.[0]||'',cel=tag.match(/data-cel-transform="([^"]+)"/)?.[1],part=tag.match(/data-part="([^"]+)"/)?.[1];
  if(/^<(defs|style)\b/.test(tag))return s;
  if(/^<path/.test(tag)&&(cel==='spring'||/data-cel-path="spring"/.test(tag)||skinId===0&&/^<path d="M 0 -14 /.test(tag)))return s.replace(/\bd="[^"]+"/,'d="'+P.spring+'"');
  if(/^<path/.test(tag)&&/data-cel-path="sensor(Left|Right)"/.test(tag)){
   const left=tag.includes('sensorLeft'),sign=left?-1:1,r=(P.swing+sign*6)*Math.PI/180,x=sign*40-22*Math.sin(r),y=-70+22*Math.cos(r);
   return s.replace(/\bd="[^"]+"/,`d="M${sign*27} -69 Q${sign*36} -55 ${n(x)} ${n(y)}"`);
  }
  if(/^<path/.test(tag)&&(part==='base-impact-sensor-lead'||/data-cel-path="baseWire"/.test(tag))){
   const r=P.angle,ex=P.x+20*P.sx*Math.cos(r)+39*P.sy*Math.sin(r),ey=-46+P.y+20*P.sx*Math.sin(r)-39*P.sy*Math.cos(r);
   return s.replace(/\bd="[^"]+"/,`d="M24 -3 C${n(23+P.x*.6)} ${n(-30+P.y*.5)} ${n(29+P.x*.5)} ${n(-40+P.y*.7)} ${n(ex)} ${n(ey)}"`);
  }
  if(/^<(g|svg)\b/.test(tag)&&!tag.endsWith('/>')){
   if(cel==='upper')tag=tag.replace(/ transform="[^"]+"/,' transform="'+upper+'"');
   if(cel==='sway')tag=tag.replace(/ transform="[^"]+"/,' transform="'+sway+'"');
   if(cel==='squash')tag=tag.replace(/ transform="[^"]+"/,' transform="'+squash+'"');
   if(cel==='head')tag=tag.replace(/ transform="[^"]+"/,' transform="rotate(0 0 -108)"');
   if(cel==='gloveLeft'||cel==='gloveRight')tag=tag.replace(/ transform="[^"]+"/,' transform="rotate('+n(P.swing+(cel==='gloveLeft'?-6:6))+')"');
   const originalUpper=skinId===0&&tag.includes('transform="translate(0 -46) rotate(0)"');
   if(originalUpper)tag=tag.replace('translate(0 -46) rotate(0)',upper+' '+sway+' '+squash);
   if(skinId===0&&/transform="translate\((-40|40) -70\)/.test(tag))tag=tag.replace(/rotate\([^)]+\)/,'rotate('+n(P.swing+(tag.includes('(-40')?-6:6))+')');
   let children=nodes(inside(s)).map(walk);
   if(originalUpper){
    const h=children.findIndex(c=>c.startsWith('<path d="M -18 -126'));
    if(h>=0)children.splice(h,children.length-h,`<g data-part="native-loose-straw-hat" transform="${hat}">${children.slice(h).join('')}</g>`);
   }
   // Equivalent detachable pieces: beacon, hologram head or pumpkin head.
   if(skinId===1&&part==='red-test-beacon')return `<g data-part="native-loose-beacon" transform="${hat}">${tag}${children.join('')}</g></g>`;
   if(skinId>0&&cel==='head')return `<g data-part="native-loose-themed-head" transform="${hat}">${tag}${children.join('')}</g></g>`;
   return tag+children.join('')+(s.match(/<\/[^>]+>\s*$/)?.[0]||'');
  }return s;
 }
 svg=walk(svg);
 if(skinId===0&&P.shock&&o.view!=='back')svg=svg.replace('<path d="M -13 -97 Q 0 -91 13 -97 M -10 -97 v 5 M -5 -98 v 5 M 0 -98 v 5 M 5 -98 v 5 M 10 -97 v 5" fill="none" stroke-width="2.4"/>','<g data-part="native-shock-mouth"><ellipse cy="-94" rx="6" ry="7" fill="#371414" stroke-width="2.4"/><path d="M1 -94 L7 -90" fill="none" stroke="#fad43c" stroke-width="2"/></g>');
 return svg;
}
return o=>{
 const action=o.action||'idle',aliases={hit:'hit1',body:'hit3',uppercut:'hit4'},id=o.action||aliases[o.state]||'idle',record=DUMMY_ACTIONS[id],t=Math.max(0,o.time||0);
 if(!record)return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-95 -220 190 240" data-combat="${action}" data-unavailable="true"><text y="-108" text-anchor="middle" fill="#bbc6d5" font-size="8">Boneco de treino: selecione uma reação.</text></svg>`;
 const duration=o.actionDuration||record.duration+(record.loop?0:.45),elapsed=record.loop?t%record.duration:t%duration,p=clamp(elapsed/record.duration),hit=!record.loop&&elapsed<record.duration,P=nativePose(record.kind,p,record.dir,t,hit),state=hit?record.kind===3?'body':record.kind===4?'uppercut':'hit':o.state==='approved'?'approved':'normal';
 let svg=adapt(o,P,state);
 if(o.action)svg=svg.replace(/viewBox="[^"]+"/,'viewBox="-105 -280 210 300"');
 return svg.replace(/^<svg\b/,`<svg data-combat="${id}" data-native-reaction="${record.kind}" data-hit-dir="${record.dir}" data-reaction-progress="${n(p)}" data-native-pose="${[P.x,P.y,P.angle,P.sx,P.sy,P.pop,P.tilt,P.vibrate].map(n).join(' ')}"`);
};

})();
