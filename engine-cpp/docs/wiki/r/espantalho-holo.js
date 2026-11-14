(window.KR=window.KR||{})["espantalho/holo"]=(function(){
const drawDummy=(()=>{// Espantalho skin 1 — Holo Test Dummy. Independent cosmetic SVG lab.
// Rig follows queueScarecrow: fixed base, six-coil spring, crossbar, hanging gloves.
const ink='#05202a',yellow='#36d782',shade='#0d806a',white='#9af6d7',n=v=>Number(v.toFixed(4))||0;
const DUMMY_HOLO_PERIOD=6;
function dummyHoloFrame(time=0,state='normal',view='front'){
 const t=n(((time%6)+6)%6),a=t*Math.PI/3,s=Math.sin(a),q=Math.sin(a*2),p=t-1;
 const active=p>=0&&p<2.8,decay=active?Math.pow(1-p/2.8,2):0,hit=decay*Math.sin(p*Math.PI*4),lift=active?Math.sin(p/2.8*Math.PI):0;
 const impact=['hit','body','uppercut'].includes(state),x=s*.8+(state==='hit'?hit*15:0),y=q*.6+(state==='body'?lift*9:state==='uppercut'?-lift*15:0);
 const angle=s*1.1+(state==='hit'?-hit*12:0),sx=state==='body'?1+lift*.1:state==='uppercut'?1-lift*.06:1,sy=state==='body'?1-lift*.14:state==='uppercut'?1+lift*.11:1;
 const radians=angle*Math.PI/180,ex=x+20*sx*Math.cos(radians)+39*sy*Math.sin(radians),ey=-46+y+20*sx*Math.sin(radians)-39*sy*Math.cos(radians);
 const gl=(-6+q*3.5-angle*.7)*Math.PI/180,gr=(6+q*3.5-angle*.7)*Math.PI/180;
 const spring=Array.from({length:6},(_,i)=>{const t0=i/6,t1=(i+1)/6,cx=(i%2?-12:12)+x*(t0+.08),cy=-14+(-32+y)*(t0+1/12);return ' Q '+n(cx)+' '+n(cy)+' '+n(x*t1)+' '+n(-14+(-32+y)*t1);}).join('');
 return {time:t,
 targetOrbit:'rotate('+n(t*60)+')',scan:'translate(0 '+n((1+s)*27)+')',cubes:'translate('+n(s*2)+' '+n(q*3)+')',upper:'translate('+n(x)+' '+n(-46+y)+')',sway:'rotate('+n(angle)+')',squash:'scale('+n(sx)+' '+n(sy)+')',gloveLeft:'rotate('+n(-6+q*3.5-angle*.7)+')',gloveRight:'rotate('+n(6+q*3.5-angle*.7)+')',head:'rotate('+n(state==='uppercut'?hit*5:s*.5)+' 0 -108)',
 spring:'M 0 -14'+spring,
 sensorLeft:'M -27 -69 Q '+n(-36+s*1.6)+' -55 '+n(-40-21*Math.sin(gl))+' '+n(-70+21*Math.cos(gl)),
 sensorRight:'M 27 -69 Q '+n(36-s*1.6)+' -55 '+n(40-21*Math.sin(gr))+' '+n(-70+21*Math.cos(gr)),
 baseWire:'M 24 -3 C '+n(23+x*.6)+' '+n(-30+y*.5)+' '+n(29+x*.5)+' '+n(-40+y*.7)+' '+n(ex)+' '+n(ey),
 lamp:n(state==='approved'?.3+.65*(.5+.5*Math.cos(a)):.9-(impact?Math.abs(hit)*.35:0)+s*.05),meter:n(impact?Math.abs(hit):.2+.1*s),reaction:n(hit)};
}

function target(f){
 return '<g data-part="rotating-holographic-accuracy-target" transform="translate(0 -53)"><circle r="15" fill="#02281f" fill-opacity=".6" stroke="#1fbe9a" stroke-width="1"/><circle r="10" fill="none" stroke="#49ec98" stroke-width="1.3"/><g data-cel-transform="targetOrbit" transform="'+f.targetOrbit+'"><path d="M -14 -7 A 16 16 0 0 1 14 -7 M 14 7 A 16 16 0 0 1 -14 7" fill="none" stroke="#72eaf0" stroke-width="2"/><path d="M -19 0 H -12 M 12 0 H 19 M 0 -19 V -12 M 0 12 V 19" stroke="#b1ffe6" stroke-width="1.5"/></g><circle r="4" fill="#98ffe0" stroke="#4febc8" stroke-width="1.2"/><path d="M -7 0 H 7 M 0 -7 V 7" stroke="#0b7667" stroke-width=".8"/></g>';
}
function glove(side,view,f){
 const left=side==='left',x=left?-40:40;
 return '<g transform="translate('+x+' -70)"><g data-cel-transform="'+(left?'gloveLeft':'gloveRight')+'" transform="'+f[left?'gloveLeft':'gloveRight']+'"><path d="M 0 0 V 22" stroke="#298b99" stroke-width="2"/><path d="M 0 1 V 21" stroke="#a8fff1" stroke-width=".6"/><g data-part="wireframe-hanging-boxing-glove" stroke="#48dca5" stroke-width="1.6" stroke-linejoin="round"><rect x="-8" y="18" width="16" height="7" rx="2" fill="#0a5150" fill-opacity=".55"/><rect x="-12" y="24" width="24" height="22" rx="7" fill="#0c7362" fill-opacity=".3"/><path d="M -9 29 L 0 26 L 9 29 V 41 L 0 45 L -9 41 Z M -12 35 H 12 M 0 25 V 45 M -9 29 L 9 41 M 9 29 L -9 41" stroke="#2aaf9c" stroke-width=".8" fill="none"/><ellipse cx="'+(left?10:-10)+'" cy="33" rx="5" ry="8" fill="#0c7362" fill-opacity=".3" stroke-width="1.3"/><path d="M -6 21 H 6" stroke="#a6ffee" stroke-width="1"/><circle cy="35" r="3.5" fill="none" stroke="#8bf6de" stroke-width=".9"/></g></g></g>';
}
function dummyHoloHands({view='front',time=0,state='normal'}={}){
 const f=dummyHoloFrame(time,state,view);return glove('left',view,f)+glove('right',view,f);
}
function face(f,state,view){
 const back=view==='back',hurt=['hit','body','uppercut'].includes(state);
 const faceMesh='<path d="M -18 -119 Q 0 -127 18 -119 M -21 -108 Q 0 -116 21 -108 M -18 -96 Q 0 -104 18 -96 M 0 -131 V -85 M -12 -126 Q -20 -108 -12 -90 M 12 -126 Q 20 -108 12 -90 M -18 -119 L 18 -96 M 18 -119 L -18 -96" stroke="#23a88d" stroke-width=".7" fill="none"/>';
 const eyes=hurt?'<path d="M -13 -112 L -5 -104 M -5 -112 L -13 -104 M 5 -112 L 13 -104 M 13 -112 L 5 -104" stroke="#b1ffec" stroke-width="1.9" fill="none"/>':'<circle cx="-9" cy="-108" r="5" fill="#062b2c" stroke="#52e4c0" stroke-width="1"/><circle cx="9" cy="-108" r="4.4" fill="#052e34" stroke="#52e4c0" stroke-width="1"/><path d="M -11 -110 L -7 -106 M -7 -110 L -11 -106" stroke="#b4ffdf" stroke-width="1.3"/><circle cx="9" cy="-108" r="1.7" fill="#94fff1" stroke="none"/>';
 return '<g data-cel-transform="head" transform="'+f.head+'"><g data-part="translucent-polygonal-hologram-head"><ellipse cy="-108" rx="21" ry="23" fill="#0e6654" fill-opacity=".28" stroke="#51dfab" stroke-width="2"/>'+faceMesh+(back?'<path d="M -8 -115 L 0 -121 L 8 -115 V -102 L 0 -96 L -8 -102 Z" fill="#083d3e" fill-opacity=".65" stroke="#66e1dd" stroke-width="1"/><path d="M -4 -111 H 4 M -4 -106 H 4" stroke="#88fff0" stroke-width="1.1"/>':eyes+'<path d="M 0 -106 L -3 -101 H 3 Z" fill="#4de5c6" stroke="#98ffe6" stroke-width=".6"/>'+(hurt?'<ellipse cy="-95" rx="5" ry="6" fill="#043235" stroke="#91ffde" stroke-width="1.2"/>':'<path d="M -10 -98 Q 0 -88 10 -98 M -7 -98 L -8 -94 M -2 -96 V -92 M 3 -96 V -92 M 8 -98 L 9 -94" fill="none" stroke="#9dffdc" stroke-width="1.2"/>'))+'<path d="M -12 -125 L -6 -128 M -19 -117 V -110" stroke="#b4ffe4" stroke-width="1.1" fill="none"/></g></g>';
}
function renderDummyHoloSvg({view='front',state='normal',time=0,armed=true}={}){
 const f=dummyHoloFrame(time,state,view),back=view==='back',id='holo-'+view+'-'+state,d='M -16 -82 C -30 -70 -34 -45 -28 -20 C -26 -12 -18 -14 0 -14 C 18 -14 26 -12 28 -20 C 34 -45 30 -70 16 -82 Z';
 const defs='<defs><clipPath id="'+id+'-body"><path d="'+d+'"/></clipPath></defs>';
 const base='<g data-part="fixed-hologram-projector-base" stroke="'+ink+'" stroke-width="2"><ellipse cy="2" rx="51" ry="8" fill="#010a11" opacity=".45" stroke="none"/><path d="M -10 4 L -6 -14 H 6 L 10 4 Z" fill="#203545"/><rect x="-44" y="-8" width="88" height="10" rx="2" fill="#142b3b"/><path d="M -39 -5 H 39" stroke="#3e6a7b" stroke-width="1"/><rect x="-14" y="-14" width="28" height="8" rx="2" fill="#205463"/><path d="M -10 -11 H 10" stroke="#9bffe7" stroke-width="1.5"/>'+[-32,32].map(x=>'<circle cx="'+x+'" cy="-3" r="2.5" fill="#25b399" stroke-width="1"/>').join('')+'<path d="M -23 -5 H -18 M 18 -5 H 23" stroke="#66e9e2" stroke-width="1.5"/></g>';
 const spring='<g data-part="six-coil-holographic-spring"><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="#123e49" stroke-width="6"/><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="#56c9bb" stroke-width="2"/><path data-cel-path="spring" d="'+f.spring+'" fill="none" stroke="#b4ffea" stroke-width=".6"/></g>';
 const post='<g data-part="original-scarecrow-crossbar-rig" fill="#073a3d" fill-opacity=".28" stroke="#278c89" stroke-width="1.3"><rect x="-6" y="-135" width="12" height="135" rx="2"/><path d="M -3 -131 V -3" stroke="#43beaa" stroke-width=".6"/><rect x="-52" y="-74" width="104" height="10" rx="2"/><path d="M -48 -72 L -40 -65 L -32 -72 L -24 -65 M 24 -65 L 32 -72 L 40 -65 L 48 -72" fill="none" stroke="#46dfc8" stroke-width=".8"/><circle cx="-46" cy="-69" r="2" fill="#a3ffdf"/><circle cx="46" cy="-69" r="2" fill="#a3ffdf"/></g>';
 const mesh='<g clip-path="url(#'+id+'-body)" fill="none" stroke="#26b482" stroke-width=".8"><path d="M -21 -74 Q 0 -67 21 -74 M -28 -61 Q 0 -54 28 -61 M -30 -46 Q 0 -39 30 -46 M -29 -31 Q 0 -24 29 -31 M -16 -82 Q -26 -49 -18 -14 M 0 -82 V -14 M 16 -82 Q 26 -49 18 -14 M -22 -73 L 27 -32 M 22 -73 L -27 -32 M -27 -60 L 18 -14 M 27 -60 L -18 -14"/><g data-cel-transform="scan" transform="'+f.scan+'"><path d="M -32 -81 H 32" stroke="#98ffe7" stroke-width="1.5" opacity=".7"/><path d="M -32 -79 H 32" stroke="#55e3ef" stroke-width="3" opacity=".17"/></g></g>';
 const torso='<g data-part="translucent-wireframe-original-sack-silhouette"><path d="'+d+'" fill="#127653" fill-opacity=".22" stroke="#4be397" stroke-width="2.2"/>'+mesh+(back?'<path d="M -10 -76 H 10 V -66 H -10 Z" fill="#03343a" fill-opacity=".6" stroke="#5fc8c5" stroke-width="1"/><path d="M -6 -73 H 6 M -6 -69 H 2" stroke="#93ffdc" stroke-width=".7"/>':'<path d="M -11 -76 H 11 V -70 H -11 Z" fill="#06393e" stroke="#56bda6" stroke-width=".6"/><text x="0" y="-72" font-family="monospace" font-size="3.4" fill="#aaffdf" text-anchor="middle">SIM / 09</text>')+target(f)+'<path d="M -25 -25 Q 0 -19 25 -25 M -24 -21 Q 0 -15 24 -21" stroke="#60dfd1" stroke-width="1" fill="none"/><path d="'+d+'" fill="none" stroke="#52eaa9" stroke-width="2"/><rect x="-12" y="-86" width="24" height="5" rx="2" fill="#0a3e43" fill-opacity=".6" stroke="#65d8ca" stroke-width="1.2"/></g>';
 const wires=armed?'<g data-part="flexible-hologram-wrist-tethers" fill="none" stroke="#4dc4d7" stroke-width="1"><path data-cel-path="sensorLeft" d="'+f.sensorLeft+'"/><path data-cel-path="sensorRight" d="'+f.sensorRight+'"/></g>':'';
 const glitch=['hit','body','uppercut'].includes(state)?'<g data-part="impact-digital-interference" data-live-lamp="0" opacity="'+f.lamp+'" stroke="#b3fff2" fill="none" stroke-width=".9"><path d="M -34 -56 H -22 M 22 -37 H 36 M -20 -109 H -14 M 15 -95 H 25"/><path d="M -31 -53 H -27 M 28 -34 H 33" stroke="#43c4ec"/></g>':'';
 const cubes=state==='approved'?'<g data-part="reboot-voxel-reassembly"><text x="0" y="-208" text-anchor="middle" font-family="monospace" font-size="6" fill="#9cffd9">REBOOT / SIMULATION READY</text><g data-cel-transform="cubes" transform="'+f.cubes+'">'+[[-24,-183,5],[19,-196,6],[-5,-193,4],[33,-174,4],[-34,-163,4]].map(([x,y,r])=>'<g transform="translate('+x+' '+y+')"><path d="M 0 0 L '+r+' '+(-r*.5)+' L '+r*2+' 0 L '+r*2+' '+r+' L '+r+' '+r*1.5+' L 0 '+r+' Z M 0 0 L '+r+' '+r*.5+' L '+r*2+' 0 M '+r+' '+r*.5+' V '+r*1.5+'" fill="#0c8065" fill-opacity=".3" stroke="#6aeac5" stroke-width=".8"/></g>').join('')+'</g></g>':'';
 return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="-95 -220 190 240" width="570" height="720" data-skin="dummy-holo" data-state="'+state+'" role="img" aria-label="Espantalho Holo-Dummy — '+view+' — '+state+'">'+defs+base+spring+'<path data-cel-path="baseWire" d="'+f.baseWire+'" fill="none" stroke="#43bbbd" stroke-width="1" opacity=".55"/><g data-live-lamp="0" opacity="'+f.lamp+'"><g data-cel-transform="upper" transform="'+f.upper+'"><g data-cel-transform="sway" transform="'+f.sway+'"><g data-cel-transform="squash" transform="'+f.squash+'">'+post+(armed?dummyHoloHands({view,time,state}):'')+torso+wires+face(f,state,view)+glitch+'</g></g></g></g>'+cubes+'</svg>';
}
function renderDummyHoloAnimatedSvg(options={}){
 const state=options.state||'normal',view=options.view||'front',times=Array.from({length:121},(_,i)=>i/20);
 let svg=renderDummyHoloSvg({...options,time:0});
 svg=svg.replace(/(<path data-cel-path="([^"]+)"[^>]+)\/>/g,(_,tag,part)=>tag+'><animate attributeName="d" values="'+times.map(t=>dummyHoloFrame(t,state,view)[part]).join(';')+'" dur="6s" repeatCount="indefinite"/></path>');
 svg=svg.replace(/(<g data-cel-transform="([^"]+)" transform="[^"]+">)/g,(_,tag,part)=>{
 if(part==='targetOrbit')return tag+'<animateTransform attributeName="transform" type="rotate" from="0" to="360" dur="6s" repeatCount="indefinite"/>';
 const type=['upper','scan','cubes'].includes(part)?'translate':part==='squash'?'scale':'rotate';
 const values=times.map(t=>dummyHoloFrame(t,state,view)[part].match(/\(([^)]+)\)/)[1]);
 return tag+'<animateTransform attributeName="transform" type="'+type+'" values="'+values.join(';')+'" dur="6s" repeatCount="indefinite"/>';
 });
 svg=svg.replace(/(<g data-live-lamp="0"[^>]*>)/g,(_,tag)=>tag+'<animate attributeName="opacity" values="'+times.map(t=>dummyHoloFrame(t,state,view).lamp).join(';')+'" dur="6s" repeatCount="indefinite"/>');
 return svg;
}

return o=>{if(o.phase)o.phase=Number(o.phase);return renderDummyHoloSvg(o);};})();const skinId=2;const DUMMY_ACTIONS={"idle":{"kind":0,"dir":1,"duration":3.2,"loop":true,"label":"Parado · mola"},"hit1":{"kind":1,"dir":1,"duration":0.26666666666666666,"label":"Recebe jab · esquerda"},"hit2":{"kind":2,"dir":1,"duration":0.4,"label":"Recebe gancho · esquerda"},"hit3":{"kind":3,"dir":1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · esquerda"},"hit4":{"kind":4,"dir":1,"duration":0.6333333333333333,"label":"Recebe uppercut · esquerda"},"dummy_hit1_R":{"kind":1,"dir":-1,"duration":0.26666666666666666,"label":"Recebe jab · direita"},"dummy_hit2_R":{"kind":2,"dir":-1,"duration":0.4,"label":"Recebe gancho · direita"},"dummy_hit3_R":{"kind":3,"dir":-1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · direita"},"dummy_hit4_R":{"kind":4,"dir":-1,"duration":0.6333333333333333,"label":"Recebe uppercut · direita"},"dummy_hit0_L":{"kind":0,"dir":1,"duration":0.4,"label":"Recebe impacto genérico · esquerda"},"dummy_hit0_R":{"kind":0,"dir":-1,"duration":0.4,"label":"Recebe impacto genérico · direita"},"dummy_cross_L":{"kind":2,"dir":1,"duration":0.3333333333333333,"label":"Recebe direto · esquerda"},"dummy_cross_R":{"kind":2,"dir":-1,"duration":0.3333333333333333,"label":"Recebe direto · direita"}};
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
