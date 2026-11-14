(window.KR=window.KR||{})["espantalho/original"]=(function(){
const drawDummy=(()=>{// SVG transcription of queueScarecrow in app/ring_renderer.cpp; same local units.
return o=>{
 const t=o.time||0,a=Math.sin(t*2),hit=o.state!=='normal',x=hit?Math.sin(t*9)*Math.exp(-(t%3))*12:a*1.5,y=o.state==='body'?Math.sin(t*2)*8:o.state==='uppercut'?-Math.abs(Math.sin(t*2))*18:Math.sin(t*3)*.8;
 const line='stroke="#202328" stroke-linejoin="round"',seam=(x,y,n,dy)=>Array.from({length:n},(_,i)=>`<path d="M ${x-3} ${y+i*dy} h 6"/>`).join('');
 const glove=(x,left)=>`<g transform="translate(${x} -70) rotate(${Math.sin(t*2.6)*3.5+(left?-6:6)})"><path d="M 0 0 V 22" stroke="#c8aa6e" stroke-width="2"/><g transform="translate(0 22)" ${line}><rect x="-8" y="-4" width="16" height="7" rx="2" fill="#ebebe6" stroke-width="2"/><rect x="-12" y="2" width="24" height="22" rx="8" fill="#b9242a" stroke-width="2.5"/><rect x="${left?-12:2}" y="3" width="10" height="20" rx="6" fill="#7a141a" stroke="none"/><ellipse cx="${left?10:-10}" cy="10" rx="5" ry="8" fill="#b9242a" stroke-width="2"/></g></g>`;
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="-95 -240 190 260"><g ${line}><ellipse cy="0" rx="46" ry="15" fill="#000" opacity=".25" stroke="none"/><path d="M -10 4 L -6 -14 H 6 L 10 4 Z" fill="#4e3018" stroke-width="2.5"/><rect x="-44" y="-8" width="88" height="10" rx="2" fill="#734b2a" stroke-width="2.5"/><path d="M -40 -5 H 40" stroke="#96663c"/><rect x="-14" y="-14" width="28" height="8" rx="2" fill="#464b52"/><path d="M 0 -14 ${Array.from({length:6},(_,i)=>`Q ${i%2?12:-12} ${-17-i*5.33} ${x*(i+1)/6} ${-14+(-32+y)*(i+1)/6}`).join(' ')}" fill="none" stroke="#373c44" stroke-width="7"/><path d="M 0 -14 ${Array.from({length:6},(_,i)=>`Q ${i%2?12:-12} ${-17-i*5.33} ${x*(i+1)/6} ${-14+(-32+y)*(i+1)/6}`).join(' ')}" fill="none" stroke="#afb9c3" stroke-width="2.4"/>
 <g transform="translate(${x} ${-46+y}) rotate(${a*1.15})"><rect x="-6" y="-135" width="12" height="135" rx="2" fill="#734b2a" stroke-width="2.5"/><rect x="-52" y="-74" width="104" height="10" rx="2" fill="#734b2a" stroke-width="2.5"/><path d="M -52 -73 L -66 -75 L -62 -69 L -68 -65 L -52 -64 M 52 -73 L 66 -75 L 62 -69 L 68 -65 L 52 -64" fill="#fad43c" stroke-width="1.8"/>${o.armed?glove(-40,true)+glove(40,false):''}
 <path d="M -20 -14 L -15 -7 L -12 -14 L -6 -8 L 0 -14 L 6 -8 L 12 -14 L 18 -8 L 20 -14" fill="#fad43c"/><path d="M -16 -82 C -30 -70 -34 -45 -28 -20 C -26 -12 -18 -14 0 -14 C 18 -14 26 -12 28 -20 C 34 -45 30 -70 16 -82 Z" fill="#d4a85c" stroke-width="3.2"/><path d="M 4 -82 H 16 C 30 -70 34 -45 28 -20 Q 22 -12 4 -14 Z" fill="#9c7434" opacity=".6" stroke="none"/><rect x="7" y="-62" width="16" height="15" rx="2" fill="#3462a2" stroke-width="1.8"/><rect x="-22" y="-38" width="13" height="12" rx="2" fill="#b92d2d"/><path d="M -1 -78 L 1 -26" fill="none" stroke-width="2.2"/>${seam(0,-74,7,7)}<rect x="-26" y="-25" width="52" height="6" rx="3" fill="#daba7a" stroke-width="2.2"/><circle cy="-22" r="4.5" fill="#daba7a"/><path d="M -2 -20 Q -6 -15 -5 -9 M 2 -20 Q 8 -17 9 -11" fill="none" stroke="#daba7a" stroke-width="3"/><path d="M -18 -82 L -15 -74 L -12 -83 L -10 -74 L -7 -83 L -5 -74 L -2 -83 L 0 -74 L 3 -83 L 5 -74 L 8 -83 L 10 -74 L 13 -83 L 15 -74 L 18 -82" fill="#fad43c"/><rect x="-12" y="-86" width="24" height="5" rx="2.5" fill="#daba7a"/>
 <ellipse cy="-108" rx="21" ry="23" fill="#daac62" stroke-width="3.2"/><path d="M 12 -128 Q 30 -108 12 -88" fill="#a27838" opacity=".6" stroke="none"/>${o.view==='back'?`<path d="M 0 -124 V -92" fill="none" stroke-width="2"/>${seam(0,-120,5,6)}`:`<ellipse cx="-12" cy="-103" rx="4.5" ry="3" fill="#eb5f5f" opacity=".45" stroke="none"/><ellipse cx="12" cy="-103" rx="4.5" ry="3" fill="#eb5f5f" opacity=".45" stroke="none"/>${hit?'<path d="M -13 -112 l 8 8 m 0 -8 l -8 8 M 5 -112 l 8 8 m 0 -8 l -8 8" fill="none" stroke-width="3"/>':'<circle cx="-9" cy="-108" r="5.5" fill="#2d1e14" stroke-width="2"/><path d="M -11.5 -110.5 l 5 5 m 0 -5 l -5 5" stroke="#faf0b4" stroke-width="1.6"/><circle cx="8.5" cy="-108.5" r="4.2" fill="#141619"/><circle cx="7.2" cy="-109.8" r="1.2" fill="white" stroke="none"/>'}<path d="M 0 -107 l -3.5 5 h 7 Z" fill="#be4b37" stroke-width="1.5"/><path d="M -13 -97 Q 0 -91 13 -97 M -10 -97 v 5 M -5 -98 v 5 M 0 -98 v 5 M 5 -98 v 5 M 10 -97 v 5" fill="none" stroke-width="2.4"/>`}
 <path d="M -18 -126 L -26 -134 L -16 -130 L -10 -138 L -5 -131 L 3 -139 L 8 -130 L 18 -136 L 17 -126" fill="#fad43c" stroke-width="1.8"/><ellipse cy="-126" rx="32" ry="8.5" fill="#e4bc52" stroke-width="3"/><path d="M -16 -128 L -11 -150 C -5 -153 5 -153 11 -150 L 16 -128 Z" fill="#e4bc52" stroke-width="2.5"/><path d="M 3 -152 L 11 -150 L 16 -128 H 3" fill="#a8802d" opacity=".6" stroke="none"/><path d="M -14 -136 Q 0 -132 14 -136 L 16 -128 Q 0 -124 -16 -128 Z" fill="#b9242a" stroke-width="2"/></g></g></svg>`;
};
})();const skinId=0;const DUMMY_ACTIONS={"idle":{"kind":0,"dir":1,"duration":3.2,"loop":true,"label":"Parado · mola"},"hit1":{"kind":1,"dir":1,"duration":0.26666666666666666,"label":"Recebe jab · esquerda"},"hit2":{"kind":2,"dir":1,"duration":0.4,"label":"Recebe gancho · esquerda"},"hit3":{"kind":3,"dir":1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · esquerda"},"hit4":{"kind":4,"dir":1,"duration":0.6333333333333333,"label":"Recebe uppercut · esquerda"},"dummy_hit1_R":{"kind":1,"dir":-1,"duration":0.26666666666666666,"label":"Recebe jab · direita"},"dummy_hit2_R":{"kind":2,"dir":-1,"duration":0.4,"label":"Recebe gancho · direita"},"dummy_hit3_R":{"kind":3,"dir":-1,"duration":0.4666666666666667,"label":"Recebe golpe no corpo · direita"},"dummy_hit4_R":{"kind":4,"dir":-1,"duration":0.6333333333333333,"label":"Recebe uppercut · direita"},"dummy_hit0_L":{"kind":0,"dir":1,"duration":0.4,"label":"Recebe impacto genérico · esquerda"},"dummy_hit0_R":{"kind":0,"dir":-1,"duration":0.4,"label":"Recebe impacto genérico · direita"},"dummy_cross_L":{"kind":2,"dir":1,"duration":0.3333333333333333,"label":"Recebe direto · esquerda"},"dummy_cross_R":{"kind":2,"dir":-1,"duration":0.3333333333333333,"label":"Recebe direto · direita"}};
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
