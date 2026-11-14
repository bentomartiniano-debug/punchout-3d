(window.KR=window.KR||{})["espantalho/crash"]=(function(){
const drawDummy=(()=>{// Espantalho skin 1 — Crash Test Dummy. Independent cosmetic SVG lab.
// Rig follows queueScarecrow: fixed base, six-coil spring, crossbar, hanging gloves.
const ink='#202328',yellow='#f2c832',shade='#b58924',white='#f3f1dc',n=v=>Number(v.toFixed(4))||0;
const DUMMY_CRASH_PERIOD=6;
function dummyCrashFrame(time=0,state='normal',view='front'){
 const t=n(((time%6)+6)%6),a=t*Math.PI/3,s=Math.sin(a),q=Math.sin(a*2),p=t-1;
 const active=p>=0&&p<2.8,decay=active?Math.pow(1-p/2.8,2):0,hit=decay*Math.sin(p*Math.PI*4),lift=active?Math.sin(p/2.8*Math.PI):0;
 const impact=['hit','body','uppercut'].includes(state),x=s*.8+(state==='hit'?hit*15:0),y=q*.6+(state==='body'?lift*9:state==='uppercut'?-lift*15:0);
 const angle=s*1.1+(state==='hit'?-hit*12:0),sx=state==='body'?1+lift*.1:state==='uppercut'?1-lift*.06:1,sy=state==='body'?1-lift*.14:state==='uppercut'?1+lift*.11:1;
 const radians=angle*Math.PI/180,ex=x+20*sx*Math.cos(radians)+39*sy*Math.sin(radians),ey=-46+y+20*sx*Math.sin(radians)-39*sy*Math.cos(radians);
 const gl=(-6+q*3.5-angle*.7)*Math.PI/180,gr=(6+q*3.5-angle*.7)*Math.PI/180;
 const spring=Array.from({length:6},(_,i)=>{const t0=i/6,t1=(i+1)/6,cx=(i%2?-12:12)+x*(t0+.08),cy=-14+(-32+y)*(t0+1/12);return ' Q '+n(cx)+' '+n(cy)+' '+n(x*t1)+' '+n(-14+(-32+y)*t1);}).join('');
 return {time:t,upper:'translate('+n(x)+' '+n(-46+y)+')',sway:'rotate('+n(angle)+')',squash:'scale('+n(sx)+' '+n(sy)+')',gloveLeft:'rotate('+n(-6+q*3.5-angle*.7)+')',gloveRight:'rotate('+n(6+q*3.5-angle*.7)+')',head:'rotate('+n(state==='uppercut'?hit*5:s*.5)+' 0 -108)',
 spring:'M 0 -14'+spring,
 sensorLeft:'M -27 -69 Q '+n(-36+s*1.6)+' -55 '+n(-40-21*Math.sin(gl))+' '+n(-70+21*Math.cos(gl)),
 sensorRight:'M 27 -69 Q '+n(36-s*1.6)+' -55 '+n(40-21*Math.sin(gr))+' '+n(-70+21*Math.cos(gr)),
 baseWire:'M 24 -3 C '+n(23+x*.6)+' '+n(-30+y*.5)+' '+n(29+x*.5)+' '+n(-40+y*.7)+' '+n(ex)+' '+n(ey),
 lamp:n(state==='approved'?.65+.3*Math.sin(a*4):.25),meter:n(impact?Math.abs(hit):.2+.1*s),reaction:n(hit)};
}
function target(x,y,r=8){
 return '<g data-part="crash-calibration-target" transform="translate('+x+' '+y+')" stroke="'+ink+'" stroke-width="1"><circle r="'+r+'" fill="'+yellow+'"/><path d="M 0 0 V '+(-r)+' A '+r+' '+r+' 0 0 1 '+r+' 0 Z M 0 0 V '+r+' A '+r+' '+r+' 0 0 1 '+(-r)+' 0 Z" fill="'+ink+'" stroke="none"/><circle r="'+(r*.3)+'" fill="'+yellow+'" stroke-width=".7"/><path d="M '+(-r-2)+' 0 H '+(r+2)+' M 0 '+(-r-2)+' V '+(r+2)+'" fill="none" stroke-width=".6"/></g>';
}
function sensor(x,y,back=false){
 return '<g data-part="adhesive-impact-sensor" transform="translate('+x+' '+y+')" stroke="'+ink+'" stroke-width=".9"><rect x="-4" y="-3.5" width="8" height="7" rx="1" fill="'+white+'"/><rect x="-2.5" y="-2" width="5" height="4" fill="#484c50"/><circle r="1.2" fill="#e86442" stroke="none"/><path d="M -3.5 -2.5 L -2 -3 M 2 3 L 3.5 2.5" stroke="#a4a89d" stroke-width=".5"/></g>';
}
function glove(side,view,f){
 const left=side==='left',x=left?-40:40,back=view==='back';
 return '<g transform="translate('+x+' -70)"><g data-cel-transform="'+(left?'gloveLeft':'gloveRight')+'" transform="'+f[left?'gloveLeft':'gloveRight']+'"><path d="M 0 0 V 22" stroke="'+ink+'" stroke-width="2.7"/><path d="M 0 1 V 21" stroke="#929c9f" stroke-width="1"/><g data-part="rubber-calibration-boxing-glove" stroke="'+ink+'" stroke-width="2" stroke-linejoin="round"><rect x="-8" y="18" width="16" height="7" rx="2" fill="'+white+'"/><path d="M -8 23 H 8" stroke="#a5aaa1" stroke-width="1"/><rect x="-12" y="24" width="24" height="22" rx="7" fill="'+yellow+'"/><path d="'+(left?'M -11 28 Q -6 25 -4 26 V 43 L -9 44 Q -12 40 -11 28 Z':'M 11 28 Q 6 25 4 26 V 43 L 9 44 Q 12 40 11 28 Z')+'" fill="'+shade+'" stroke="none"/><ellipse cx="'+(left?10:-10)+'" cy="33" rx="5" ry="8" fill="'+yellow+'"/><path d="M -6 28 H 6" stroke="#fff099" stroke-width="1.2"/>'+(back?target(0,35,5):'<path d="M -6 36 H 6 M -6 39 H 6" stroke="#9b781f" stroke-width=".9"/>')+sensor(0,21)+'</g></g></g>';
}
function face(f,state,view){
 const back=view==='back',hurt=['hit','body','uppercut'].includes(state);
 const eyes=hurt?'<path d="M -13 -112 L -5 -104 M -5 -112 L -13 -104 M 5 -112 L 13 -104 M 13 -112 L 5 -104" fill="none" stroke="'+ink+'" stroke-width="2.2"/>':'<circle cx="-9" cy="-108" r="4.8" fill="'+ink+'"/><circle cx="9" cy="-108" r="4.8" fill="'+ink+'"/><path d="M -11 -110 L -7 -106 M -7 -110 L -11 -106 M 7 -110 L 11 -106 M 11 -110 L 7 -106" stroke="#dfc75c" stroke-width="1"/>';
 return '<g data-cel-transform="head" transform="'+f.head+'"><g data-part="molded-yellow-test-head" stroke="'+ink+'"><ellipse cy="-108" rx="21" ry="23" fill="'+yellow+'" stroke-width="3"/><path d="M 9 -128 C 25 -120 25 -96 10 -88 Q 18 -107 9 -128 Z" fill="'+shade+'" stroke="none"/><path d="M -14 -122 Q -19 -114 -17 -103" stroke="#fff09b" stroke-width="2" fill="none"/>'+(back?'<path d="M -14 -115 Q 0 -122 14 -115 M -15 -102 Q 0 -109 15 -102" stroke="#9c7923" stroke-width="1.3" fill="none"/>'+target(0,-108,10)+sensor(0,-91):target(0,-123,5)+eyes+'<path d="M 0 -106 L -3 -101 H 3 Z" fill="#c88c31" stroke-width="1"/>'+(hurt?'<ellipse cy="-95" rx="5" ry="6" fill="#43433b" stroke-width="1.2"/>':'<path d="M -10 -98 Q 0 -88 10 -98" fill="none" stroke-width="1.8"/><path d="M -7 -98 L -8 -94 M -2 -96 V -92 M 3 -96 V -92 M 8 -98 L 9 -94" stroke-width="1"/>'))+'</g>'+(state==='approved'?'<g data-part="red-test-beacon" stroke="'+ink+'"><path d="M -6 -131 V -137 Q 0 -144 6 -137 V -131 Z" fill="#c84132" stroke-width="1.5"/><path d="M -7 -132 H 7" stroke="#62696c" stroke-width="2"/><g data-live-lamp="0" opacity="'+f.lamp+'"><path d="M -3 -136 Q 0 -140 3 -136" stroke="#ffc4a0" stroke-width="2" fill="none"/><path d="M -11 -138 L -15 -141 M 11 -138 L 15 -141 M 0 -145 V -149" stroke="#f58455" stroke-width="1.4"/></g></g>':'')+'</g>';
}
function dummyCrashHands({view='front',time=0,state='normal'}={}){
 const f=dummyCrashFrame(time,state,view);return glove('left',view,f)+glove('right',view,f);
}
function renderDummyCrashSvg({view='front',state='normal',time=0,armed=true}={}){
 const f=dummyCrashFrame(time,state,view),back=view==='back',d='M -16 -82 C -30 -70 -34 -45 -28 -20 C -26 -12 -18 -14 0 -14 C 18 -14 26 -12 28 -20 C 34 -45 30 -70 16 -82 Z';
 const base='<g data-part="fixed-impact-test-base" stroke="'+ink+'" stroke-width="2.5"><ellipse cy="2" rx="51" ry="8" fill="#080b10" opacity=".3" stroke="none"/><path d="M -10 4 L -6 -14 H 6 L 10 4 Z" fill="#484b4d"/><rect x="-44" y="-8" width="88" height="10" rx="2" fill="#3b4146"/><path d="M -39 -5 H 39" stroke="#7f8c91" stroke-width="1"/><rect x="-14" y="-14" width="28" height="8" rx="2" fill="#747e83" stroke-width="2"/>'+[-32,32].map(x=>'<circle cx="'+x+'" cy="-3" r="2.5" fill="#a1a8a6" stroke-width="1"/>').join('')+'<path d="M -24 -7 L -20 0 M -14 -7 L -10 0 M 10 -7 L 14 0 M 20 -7 L 24 0" stroke="'+yellow+'" stroke-width="3"/>'+sensor(24,-3)+'</g>';
 const spring='<g data-part="six-coil-impact-shock-absorber"><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="'+ink+'" stroke-width="7"/><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="#b2bdc0" stroke-width="2.4"/></g>';
 const post='<g data-part="original-scarecrow-crossbar-rig" stroke="'+ink+'" stroke-width="2.5"><rect x="-6" y="-135" width="12" height="135" rx="2" fill="#50565a"/><path d="M -3 -131 V -3" stroke="#8b959b" stroke-width="1"/><rect x="-52" y="-74" width="104" height="10" rx="2" fill="#464e54"/><path d="M -48 -72 H 48" stroke="#8c9da1" stroke-width="1"/><path d="M -50 -73 L -44 -65 M -40 -73 L -34 -65 M 34 -73 L 40 -65 M 44 -73 L 50 -65" stroke="'+yellow+'" stroke-width="3"/>'+target(-45,-69,4)+target(45,-69,4)+'</g>';
 const torso='<g data-part="rubber-dummy-original-sack-silhouette"><path d="'+d+'" fill="'+yellow+'" stroke="'+ink+'" stroke-width="3.2"/><path d="M 4 -82 H 16 C 30 -70 34 -45 28 -20 C 26 -12 18 -14 4 -14 Q 16 -43 4 -82 Z" fill="'+shade+'"/><path d="M -16 -77 Q -29 -56 -25 -32" fill="none" stroke="#ffe98d" stroke-width="2"/><path d="M -18 -69 Q 0 -63 18 -69 M -25 -30 Q 0 -23 25 -30" fill="none" stroke="#9c7a25" stroke-width="1.2"/>'+target(0,-53,13)+sensor(-20,-39)+sensor(20,-39)+(back?'<g data-part="rear-sensor-recorder" stroke="'+ink+'"><rect x="-12" y="-76" width="24" height="12" rx="2" fill="#343b40" stroke-width="1.4"/><path d="M -8 -72 H 3 M -8 -68 H -2" stroke="#9ed4c0" stroke-width="1"/><circle cx="8" cy="-70" r="1.5" fill="#dd7251" stroke="none"/></g>':'<path d="M -11 -74 H 11 V -69 H -11 Z" fill="'+white+'" stroke="'+ink+'" stroke-width=".7"/><text x="0" y="-70" font-family="monospace" font-size="3.5" fill="'+ink+'" text-anchor="middle">KO / TEST 09</text>')+'<path d="M -26 -25 Q 0 -20 26 -25 L 25 -19 Q 0 -14 -25 -19 Z" fill="#343b40" stroke="'+ink+'" stroke-width="1.7"/><path d="M -10 -21 H 10" stroke="'+white+'" stroke-width="2"/><path d="'+d+'" fill="none" stroke="'+ink+'" stroke-width="3.2"/><rect x="-12" y="-86" width="24" height="5" rx="2" fill="#3e474d" stroke="'+ink+'" stroke-width="2"/><path d="M -7 -85 L -5 -82 M 0 -85 L 2 -82 M 7 -85 L 9 -82" stroke="#89989d" stroke-width="1"/></g>';
 const wires=armed?'<g data-part="flexible-wrist-sensor-leads"><path data-cel-path="sensorLeft" d="'+f.sensorLeft+'" fill="none" stroke="'+ink+'" stroke-width="2.6"/><path data-cel-path="sensorLeft" d="'+f.sensorLeft+'" fill="none" stroke="#e5e4c6" stroke-width="1"/><path data-cel-path="sensorRight" d="'+f.sensorRight+'" fill="none" stroke="'+ink+'" stroke-width="2.6"/><path data-cel-path="sensorRight" d="'+f.sensorRight+'" fill="none" stroke="#d57f53" stroke-width="1"/></g>':'';
 const approved=state==='approved'?'<g data-part="five-star-impact-approval"><rect x="-46" y="-217" width="92" height="18" rx="4" fill="#25392e" stroke="#b8cc9b" stroke-width="1.5"/><text x="0" y="-208" font-family="Arial" font-size="8" text-anchor="middle" fill="#f5d760">★★★★★</text><text x="0" y="-201" font-family="Arial" font-size="4.5" font-weight="bold" text-anchor="middle" fill="#e6efdb">5 ESTRELAS / IMPACT APPROVED</text></g>':'';
 return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="-95 -220 190 240" width="570" height="720" data-skin="dummy-crash" data-state="'+state+'" role="img" aria-label="Espantalho Crash Test Dummy — '+view+' — '+state+'">'+base+spring+'<path data-cel-path="baseWire" data-part="base-impact-sensor-lead" d="'+f.baseWire+'" fill="none" stroke="#db6a46" stroke-width="1.5"/><g data-cel-transform="upper" transform="'+f.upper+'"><g data-cel-transform="sway" transform="'+f.sway+'"><g data-cel-transform="squash" transform="'+f.squash+'">'+post+(armed?dummyCrashHands({view,time,state}):'')+torso+wires+face(f,state,view)+'</g></g></g>'+approved+'</svg>';
}
function renderDummyCrashAnimatedSvg(options={}){
 const state=options.state||'normal',view=options.view||'front',times=Array.from({length:121},(_,i)=>i/20);
 let svg=renderDummyCrashSvg({...options,time:0});
 svg=svg.replace(/(<path data-cel-path="([^"]+)"[^>]+)\/>/g,(_,tag,part)=>tag+'><animate attributeName="d" values="'+times.map(t=>dummyCrashFrame(t,state,view)[part]).join(';')+'" dur="6s" repeatCount="indefinite"/></path>');
 svg=svg.replace(/(<g data-cel-transform="([^"]+)" transform="[^"]+">)/g,(_,tag,part)=>{
 const type=part==='upper'?'translate':part==='squash'?'scale':'rotate';
 const values=times.map(t=>dummyCrashFrame(t,state,view)[part].match(/\(([^)]+)\)/)[1]);
 return tag+'<animateTransform attributeName="transform" type="'+type+'" values="'+values.join(';')+'" dur="6s" repeatCount="indefinite"/>';
 });
 svg=svg.replace(/(<g data-live-lamp="0"[^>]*>)/g,(_,tag)=>tag+'<animate attributeName="opacity" values="'+times.map(t=>dummyCrashFrame(t,state,view).lamp).join(';')+'" dur="6s" repeatCount="indefinite"/>');
 return svg;
}

return o=>{if(o.phase)o.phase=Number(o.phase);return renderDummyCrashSvg(o);};})();const skinId=1;const DUMMY_ACTIONS={"idle":{"kind":0,"dir":1,"duration":3.2,"loop":true,"label":"Parado · mola"},"hit1":{"kind":1,"dir":1,"duration":0.26666666666666666,"label":"Recebe jab · esquerda"},"hit2":{"kind":2,"dir":1,"duration":0.4,"label":"Recebe gancho · esquerda"},"hit3":{"kind":3,"dir":1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · esquerda"},"hit4":{"kind":4,"dir":1,"duration":0.6333333333333333,"label":"Recebe uppercut · esquerda"},"dummy_hit1_R":{"kind":1,"dir":-1,"duration":0.26666666666666666,"label":"Recebe jab · direita"},"dummy_hit2_R":{"kind":2,"dir":-1,"duration":0.4,"label":"Recebe gancho · direita"},"dummy_hit3_R":{"kind":3,"dir":-1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · direita"},"dummy_hit4_R":{"kind":4,"dir":-1,"duration":0.6333333333333333,"label":"Recebe uppercut · direita"},"dummy_hit0_L":{"kind":0,"dir":1,"duration":0.4,"label":"Recebe impacto genérico · esquerda"},"dummy_hit0_R":{"kind":0,"dir":-1,"duration":0.4,"label":"Recebe impacto genérico · direita"},"dummy_cross_L":{"kind":2,"dir":1,"duration":0.3333333333333333,"label":"Recebe direto · esquerda"},"dummy_cross_R":{"kind":2,"dir":-1,"duration":0.3333333333333333,"label":"Recebe direto · direita"}};
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
