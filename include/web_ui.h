#ifndef WEB_UI_H
#define WEB_UI_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Mecanum Car Controller</title>
    <style>
        *,*::before,*::after{margin:0;padding:0;box-sizing:border-box}
        :root{
            --bg:#080c18;--surface:rgba(255,255,255,0.04);
            --glass:rgba(255,255,255,0.055);--glass-border:rgba(255,255,255,0.09);
            --accent:#00e5ff;--accent-glow:rgba(0,229,255,0.25);--accent-dim:rgba(0,229,255,0.1);
            --purple:#7b61ff;--purple-glow:rgba(123,97,255,0.25);
            --danger:#ff3d71;--danger-glow:rgba(255,61,113,0.3);
            --warn:#ffaa00;--success:#0cce6b;
            --text:#e8eaf6;--text-dim:rgba(232,234,246,0.4);
            --radius:16px;
        }
        html,body{font-family:-apple-system,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);height:100%;overflow-y:auto;overflow-x:hidden}
        body{display:flex;flex-direction:column;align-items:center;-webkit-tap-highlight-color:transparent;user-select:none;padding-bottom:20px}
        body::before{content:'';position:fixed;inset:0;background:radial-gradient(ellipse 500px 350px at 20% 10%,rgba(0,229,255,0.06),transparent),radial-gradient(ellipse 400px 400px at 80% 90%,rgba(123,97,255,0.04),transparent);z-index:-1}
        @keyframes fadeIn{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:translateY(0)}}
        @keyframes pulse{0%,100%{opacity:1}50%{opacity:.35}}
        @keyframes spin{from{transform:rotate(0)}to{transform:rotate(360deg)}}

        .header{width:100%;padding:14px 16px 4px;text-align:center;animation:fadeIn .4s ease-out}
        .header h1{font-size:1.2rem;font-weight:900;letter-spacing:-.5px;background:linear-gradient(135deg,var(--accent),var(--purple));-webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
        .status-bar{display:flex;justify-content:center;align-items:center;gap:12px;margin-top:4px;font-size:.65rem;color:var(--text-dim)}
        .status-dot{width:6px;height:6px;border-radius:50%;display:inline-block;margin-right:3px;vertical-align:middle;background:var(--success);box-shadow:0 0 6px var(--success);animation:pulse 2s ease-in-out infinite}
        .action-label{font-weight:600;transition:color .2s}
        .action-label.moving{color:var(--accent)}

        .card{width:min(94%,400px);background:var(--glass);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);border:1px solid var(--glass-border);border-radius:var(--radius);padding:12px 14px;margin-bottom:8px;animation:fadeIn .5s ease-out both}

        .card-title{font-size:.55rem;font-weight:700;text-transform:uppercase;letter-spacing:1.4px;color:var(--text-dim);margin-bottom:6px;display:flex;align-items:center;gap:6px}
        .badge{font-size:.5rem;padding:2px 6px;border-radius:6px;font-weight:700}
        .badge-on{background:rgba(12,206,107,.15);color:var(--success);border:1px solid rgba(12,206,107,.25)}
        .badge-off{background:rgba(255,170,0,.1);color:var(--warn);border:1px solid rgba(255,170,0,.2)}
        .badge-cal{background:rgba(0,229,255,.1);color:var(--accent);border:1px solid rgba(0,229,255,.2)}

        /* ── Compass HUD ── */
        .compass-wrap{display:flex;align-items:center;justify-content:center;gap:16px}
        .compass-ring{width:100px;height:100px;border-radius:50%;border:2px solid var(--glass-border);position:relative;background:radial-gradient(circle,rgba(0,229,255,.03),transparent)}
        .compass-ring::before{content:'N';position:absolute;top:2px;left:50%;transform:translateX(-50%);font-size:.5rem;font-weight:900;color:var(--danger)}
        .compass-ring::after{content:'S';position:absolute;bottom:2px;left:50%;transform:translateX(-50%);font-size:.45rem;font-weight:600;color:var(--text-dim)}
        .compass-needle{position:absolute;top:50%;left:50%;width:3px;height:40px;margin-left:-1.5px;margin-top:-30px;transform-origin:50% 75%;transition:transform .3s ease;border-radius:2px;background:linear-gradient(to bottom,var(--danger) 40%,var(--text-dim) 40%)}
        .compass-center{position:absolute;top:50%;left:50%;width:8px;height:8px;margin:-4px;border-radius:50%;background:var(--accent);box-shadow:0 0 8px var(--accent-glow)}
        .heading-text{text-align:center}
        .heading-val{font-size:2rem;font-weight:900;color:var(--accent);letter-spacing:-1px;line-height:1}
        .heading-unit{font-size:.7rem;color:var(--text-dim);font-weight:600}
        .heading-dir{font-size:.65rem;color:var(--text-dim);margin-top:2px}

        /* ── Compass actions row ── */
        .compass-actions{display:flex;gap:6px;margin-top:8px;justify-content:center}
        .chip-btn{font-family:inherit;font-size:.55rem;font-weight:700;padding:5px 12px;border-radius:20px;border:1px solid var(--glass-border);background:var(--glass);color:var(--text-dim);cursor:pointer;transition:all .15s;touch-action:none;outline:none;text-transform:uppercase;letter-spacing:.5px}
        .chip-btn:active{transform:scale(.94)}
        .chip-btn.active{border-color:var(--accent);color:var(--accent);background:var(--accent-dim)}
        .chip-btn.cal{border-color:var(--purple);color:var(--purple)}
        .chip-btn.cal:active{background:rgba(123,97,255,.15)}
        .chip-btn.cal.spinning{animation:pulse 1s infinite;pointer-events:none}

        /* ── Speed ── */
        .speed-row{display:flex;align-items:center;gap:10px}
        .speed-val{min-width:32px;text-align:center;font-size:1.1rem;font-weight:900;color:var(--accent)}
        .slider-wrap{flex:1}
        input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:5px;border-radius:3px;background:var(--accent-dim);outline:none}
        input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px var(--accent-glow);cursor:pointer;border:2px solid var(--bg)}
        .speed-labels{display:flex;justify-content:space-between;font-size:.5rem;color:var(--text-dim);margin-top:1px}

        /* ── 8-way D-Pad ── */
        .dpad{display:grid;grid-template-columns:1fr 1fr 1fr;grid-template-rows:1fr 1fr 1fr;gap:6px;width:220px;height:220px;margin:0 auto}
        .dpad-btn{border:none;border-radius:12px;font-family:inherit;font-size:.5rem;font-weight:700;letter-spacing:.3px;text-transform:uppercase;cursor:pointer;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:1px;transition:all .1s;-webkit-tap-highlight-color:transparent;outline:none;touch-action:none}
        .dpad-btn svg{width:18px;height:18px;fill:currentColor}
        .btn-dir{background:linear-gradient(145deg,rgba(0,229,255,.1),rgba(0,229,255,.03));border:1px solid rgba(0,229,255,.15);color:var(--accent)}
        .btn-dir:active,.btn-dir.active{background:linear-gradient(145deg,rgba(0,229,255,.28),rgba(0,229,255,.1));box-shadow:0 0 18px var(--accent-glow);transform:scale(.93)}
        .btn-diag{background:linear-gradient(145deg,rgba(123,97,255,.1),rgba(123,97,255,.03));border:1px solid rgba(123,97,255,.15);color:var(--purple)}
        .btn-diag:active,.btn-diag.active{background:linear-gradient(145deg,rgba(123,97,255,.28),rgba(123,97,255,.1));box-shadow:0 0 18px var(--purple-glow);transform:scale(.93)}
        .btn-stop{background:linear-gradient(145deg,rgba(255,61,113,.12),rgba(255,61,113,.03));border:1px solid rgba(255,61,113,.18);color:var(--danger);border-radius:50%}
        .btn-stop:active,.btn-stop.active{background:linear-gradient(145deg,rgba(255,61,113,.35),rgba(255,61,113,.1));box-shadow:0 0 18px var(--danger-glow);transform:scale(.88)}

        /* ── Precise Turn ── */
        .turn-row{display:flex;gap:6px;align-items:center;justify-content:center;flex-wrap:wrap}
        .turn-btn{font-family:inherit;font-size:.7rem;font-weight:900;padding:10px 16px;border-radius:12px;border:1px solid rgba(123,97,255,.2);background:linear-gradient(145deg,rgba(123,97,255,.1),rgba(123,97,255,.03));color:var(--purple);cursor:pointer;transition:all .12s;touch-action:none;outline:none;min-width:60px;text-align:center}
        .turn-btn:active{background:linear-gradient(145deg,rgba(123,97,255,.3),rgba(123,97,255,.1));box-shadow:0 0 16px var(--purple-glow);transform:scale(.93)}
        .dial-wrap{display:flex;flex-direction:column;align-items:center;gap:8px;margin-top:8px}
        .dial-container{position:relative;width:160px;height:160px;touch-action:none}
        .dial-canvas{width:160px;height:160px;cursor:pointer}
        .dial-center-text{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);text-align:center;pointer-events:none}
        .dial-angle{font-size:1.6rem;font-weight:900;color:var(--purple);line-height:1}
        .dial-label{font-size:.5rem;color:var(--text-dim);text-transform:uppercase;letter-spacing:1px}
        .dial-go{font-family:inherit;font-size:.7rem;font-weight:800;padding:10px 28px;border-radius:12px;border:1px solid rgba(123,97,255,.3);background:linear-gradient(145deg,rgba(123,97,255,.18),rgba(123,97,255,.06));color:var(--purple);cursor:pointer;outline:none;touch-action:none;transition:all .12s}
        .dial-go:active{transform:scale(.93);box-shadow:0 0 16px var(--purple-glow)}

        .footer{padding:6px;font-size:.5rem;color:var(--text-dim);text-align:center;margin-top:4px}

        @media (max-height:700px){.compass-ring{width:80px;height:80px}.heading-val{font-size:1.5rem}.dpad{width:190px;height:190px;gap:4px}.card{padding:10px 12px;margin-bottom:6px}}
        @media (min-height:900px){.dpad{width:260px;height:260px;gap:8px}}
    </style>
</head>
<body>
    <div class="header">
        <h1>&#9889; Mecanum Car</h1>
        <div class="status-bar">
            <span><span class="status-dot"></span>WiFi</span>
            <span class="action-label" id="actionLabel">Idle</span>
        </div>
    </div>

    <!-- Compass HUD -->
    <div class="card">
        <div class="card-title">Compass <span class="badge badge-on" id="compassBadge">ON</span> <span class="badge badge-off" id="calBadge" style="display:none">NOT CAL</span></div>
        <div class="compass-wrap">
            <div class="compass-ring">
                <div class="compass-needle" id="needle"></div>
                <div class="compass-center"></div>
            </div>
            <div class="heading-text">
                <div class="heading-val" id="headingVal">--</div>
                <div class="heading-unit">&deg;</div>
                <div class="heading-dir" id="headingDir">--</div>
            </div>
        </div>
        <div class="compass-actions">
            <button class="chip-btn active" id="compassToggle" onclick="toggleCompass()">Compass Assist</button>
            <button class="chip-btn cal" id="calBtn" onclick="startCalibration()">Calibrate</button>
        </div>
    </div>

    <!-- Speed -->
    <div class="card">
        <div class="card-title">Speed</div>
        <div class="speed-row">
            <div class="speed-val" id="speedVal">180</div>
            <div class="slider-wrap">
                <input type="range" id="speedSlider" min="80" max="255" value="180">
                <div class="speed-labels"><span>Slow</span><span>Max</span></div>
            </div>
        </div>
    </div>

    <!-- 8-way D-Pad -->
    <div class="card">
        <div class="card-title">Drive</div>
        <div class="dpad">
            <button class="dpad-btn btn-diag" data-cmd="diag_fl">
                <svg viewBox="0 0 24 24"><path d="M4 4h7v2H7.41L18 16.59V12h2v8h-8v-2h4.59L6 7.41V12H4z"/></svg>
            </button>
            <button class="dpad-btn btn-dir" data-cmd="forward">
                <svg viewBox="0 0 24 24"><path d="M12 4l-7 7h4.5v9h5v-9H19z"/></svg>FWD
            </button>
            <button class="dpad-btn btn-diag" data-cmd="diag_fr">
                <svg viewBox="0 0 24 24"><path d="M20 4h-7v2h4.59L6 17.59V12H4v8h8v-2H7.41L18 6.41V12h2z"/></svg>
            </button>
            <button class="dpad-btn btn-dir" data-cmd="strafe_left">
                <svg viewBox="0 0 24 24"><path d="M4 12l7-7v4.5h9v5h-9V19z"/></svg>
            </button>
            <button class="dpad-btn btn-stop" data-cmd="stop">
                <svg viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="2"/></svg>
            </button>
            <button class="dpad-btn btn-dir" data-cmd="strafe_right">
                <svg viewBox="0 0 24 24"><path d="M20 12l-7-7v4.5H4v5h9V19z"/></svg>
            </button>
            <button class="dpad-btn btn-diag" data-cmd="diag_bl">
                <svg viewBox="0 0 24 24"><path d="M4 20h7v-2H7.41L18 7.41V12h2V4h-8v2h4.59L6 16.59V12H4z"/></svg>
            </button>
            <button class="dpad-btn btn-dir" data-cmd="backward">
                <svg viewBox="0 0 24 24"><path d="M12 20l7-7h-4.5V4h-5v9H5z"/></svg>BWD
            </button>
            <button class="dpad-btn btn-diag" data-cmd="diag_br">
                <svg viewBox="0 0 24 24"><path d="M20 20h-7v-2h4.59L6 6.41V12H4V4h8v2H7.41L18 17.59V12h2z"/></svg>
            </button>
        </div>
    </div>

    <!-- Precise Turn -->
    <div class="card">
        <div class="card-title">Precise Turn</div>
        <div class="turn-row">
            <button class="turn-btn" onclick="precTurn(-90)">-90&deg;</button>
            <button class="turn-btn" onclick="precTurn(-45)">-45&deg;</button>
            <button class="turn-btn" onclick="precTurn(45)">+45&deg;</button>
            <button class="turn-btn" onclick="precTurn(90)">+90&deg;</button>
        </div>
        <div class="dial-wrap">
            <div class="dial-container" id="dialContainer">
                <canvas class="dial-canvas" id="dialCanvas" width="320" height="320"></canvas>
                <div class="dial-center-text">
                    <div class="dial-angle" id="dialAngle">0</div>
                    <div class="dial-label">degrees</div>
                </div>
            </div>
            <button class="dial-go" id="dialGo">TURN</button>
        </div>
    </div>

    <!-- Spin Turn (free rotate) -->
    <div class="card">
        <div class="card-title">Spin</div>
        <div class="turn-row">
            <button class="dpad-btn btn-dir" data-cmd="left" style="width:80px;height:50px;border-radius:12px">
                <svg viewBox="0 0 24 24"><path d="M12.5 8c-2.65 0-5.05.99-6.9 2.6L2 7v9h9l-3.62-3.62c1.39-1.16 3.16-1.88 5.12-1.88 3.54 0 6.55 2.31 7.6 5.5l2.37-.78C21.08 11.03 17.15 8 12.5 8z"/></svg>
            </button>
            <button class="dpad-btn btn-dir" data-cmd="right" style="width:80px;height:50px;border-radius:12px">
                <svg viewBox="0 0 24 24"><path d="M11.5 8c2.65 0 5.05.99 6.9 2.6L22 7v9h-9l3.62-3.62c-1.39-1.16-3.16-1.88-5.12-1.88-3.54 0-6.55 2.31-7.6 5.5l-2.37-.78C2.92 11.03 6.85 8 11.5 8z"/></svg>
            </button>
        </div>
    </div>

    <div class="footer">ESP32-S3 WiFi AP &middot; 192.168.4.1</div>

    <script>
    (function(){
        var speedSlider=document.getElementById('speedSlider'),speedVal=document.getElementById('speedVal');
        var actionLabel=document.getElementById('actionLabel'),needle=document.getElementById('needle');
        var headingVal=document.getElementById('headingVal'),headingDir=document.getElementById('headingDir');
        var compassBadge=document.getElementById('compassBadge'),calBadge=document.getElementById('calBadge');
        var compassToggle=document.getElementById('compassToggle'),calBtn=document.getElementById('calBtn');
        var buttons=document.querySelectorAll('.dpad-btn'),currentSpeed=speedSlider.value;

        speedSlider.addEventListener('input',function(){currentSpeed=speedSlider.value;speedVal.textContent=currentSpeed});

        function xhr(url,cb){var x=new XMLHttpRequest();x.open('GET',url,true);if(cb)x.onload=function(){if(x.status===200)cb(x.responseText)};x.send()}
        function sendCmd(cmd){var url=cmd==='stop'?'/stop':'/'+cmd+'?speed='+currentSpeed;xhr(url)}
        function setAction(t,moving){actionLabel.textContent=t;if(moving)actionLabel.classList.add('moving');else actionLabel.classList.remove('moving')}

        var cmdLabels={forward:'Forward',backward:'Backward',left:'Spin L',right:'Spin R',strafe_left:'Strafe L',strafe_right:'Strafe R',diag_fl:'Diag FL',diag_fr:'Diag FR',diag_bl:'Diag BL',diag_br:'Diag BR',stop:'Stopped'};

        buttons.forEach(function(btn){
            var cmd=btn.dataset.cmd,pressed=false;
            function press(e){if(pressed)return;pressed=true;e.preventDefault();btn.classList.add('active');sendCmd(cmd);setAction(cmdLabels[cmd]||cmd,cmd!=='stop')}
            function release(e){if(!pressed)return;pressed=false;e.preventDefault();btn.classList.remove('active');if(cmd!=='stop'){sendCmd('stop');setAction('Idle',false)}}
            btn.addEventListener('pointerdown',press);
            btn.addEventListener('pointerup',release);
            btn.addEventListener('pointercancel',release);
            btn.addEventListener('pointerleave',function(e){if(pressed)release(e)});
            btn.addEventListener('click',function(e){e.preventDefault();if(!pressed){sendCmd(cmd);setAction(cmdLabels[cmd]||cmd,cmd!=='stop');btn.classList.add('active');setTimeout(function(){btn.classList.remove('active');if(cmd!=='stop'){sendCmd('stop');setAction('Idle',false)}},200)}});
        });

        function dirName(h){var d=['N','NE','E','SE','S','SW','W','NW'];return d[Math.round(h/45)%8]}

        function pollHeading(){
            xhr('/heading',function(t){
                try{
                    var d=JSON.parse(t),h=d.heading;
                    if(h>=0){
                        headingVal.textContent=h.toFixed(1);
                        headingDir.textContent=dirName(h);
                        needle.style.transform='rotate('+h+'deg)';
                    }
                    compassBadge.textContent=d.compass?'ON':'OFF';
                    compassBadge.className='badge '+(d.compass?'badge-on':'badge-off');
                    compassToggle.classList.toggle('active',d.compass);
                    if(d.calibrated){calBadge.style.display='none'}
                    else{calBadge.style.display='';calBadge.textContent='NOT CAL'}
                    if(d.mode==='calibrating'){calBtn.classList.add('spinning');calBtn.textContent='Spinning...';setAction('Calibrating',true)}
                    else{calBtn.classList.remove('spinning');calBtn.textContent='Calibrate'}
                    if(d.mode==='turning')setAction('Turning...',true);
                }catch(e){}
            });
        }
        setInterval(pollHeading,300);pollHeading();

        window.toggleCompass=function(){xhr('/compass_toggle')};
        window.startCalibration=function(){
            if(confirm('Place the car on a flat surface. It will spin slowly for 8 seconds to calibrate the compass.\n\nReady?')){
                xhr('/calibrate');calBtn.classList.add('spinning');calBtn.textContent='Spinning...'
            }
        };
        window.precTurn=function(a){a=parseFloat(a);if(isNaN(a))return;xhr('/turn_angle?angle='+a);setAction('Turn '+a+'\u00b0',true)};

        // ── Circular Angle Dial ──
        (function(){
            var canvas=document.getElementById('dialCanvas'),ctx=canvas.getContext('2d');
            var container=document.getElementById('dialContainer');
            var angleDisp=document.getElementById('dialAngle'),goBtn=document.getElementById('dialGo');
            var W=320,H=320,cx=W/2,cy=H/2,R=120,handleR=14;
            var selectedAngle=0,dragging=false;
            var purple='#7b61ff',purpleDim='rgba(123,97,255,0.15)',purpleGlow='rgba(123,97,255,0.4)';
            var textDim='rgba(232,234,246,0.35)',text='#e8eaf6',bg='#080c18';

            function drawDial(){
                ctx.clearRect(0,0,W,H);
                // Outer ring
                ctx.beginPath();ctx.arc(cx,cy,R,0,2*Math.PI);ctx.strokeStyle='rgba(255,255,255,0.08)';ctx.lineWidth=2;ctx.stroke();
                // Tick marks and labels
                var labels=['0','90','180','270'];
                for(var i=0;i<360;i+=15){
                    var rad=(i-90)*Math.PI/180;
                    var isMajor=i%90===0,isMid=i%45===0;
                    var inner=isMajor?R-16:isMid?R-10:R-6;
                    ctx.beginPath();
                    ctx.moveTo(cx+Math.cos(rad)*inner,cy+Math.sin(rad)*inner);
                    ctx.lineTo(cx+Math.cos(rad)*(R-1),cy+Math.sin(rad)*(R-1));
                    ctx.strokeStyle=isMajor?'rgba(255,255,255,0.3)':isMid?'rgba(255,255,255,0.15)':'rgba(255,255,255,0.07)';
                    ctx.lineWidth=isMajor?2:1;ctx.stroke();
                    if(isMajor){
                        var lx=cx+Math.cos(rad)*(R+14),ly=cy+Math.sin(rad)*(R+14);
                        ctx.font='bold 11px -apple-system,Roboto,sans-serif';ctx.fillStyle=textDim;ctx.textAlign='center';ctx.textBaseline='middle';
                        ctx.fillText(labels[i/90],lx,ly);
                    }
                }
                // Arc from 0 to selected angle
                if(selectedAngle!==0){
                    var startRad=-Math.PI/2;
                    var endRad=(selectedAngle-90)*Math.PI/180;
                    ctx.beginPath();
                    ctx.arc(cx,cy,R-8,startRad,endRad,selectedAngle<0);
                    ctx.strokeStyle=purple;ctx.lineWidth=4;ctx.lineCap='round';ctx.stroke();
                    // Glow
                    ctx.beginPath();
                    ctx.arc(cx,cy,R-8,startRad,endRad,selectedAngle<0);
                    ctx.strokeStyle=purpleGlow;ctx.lineWidth=8;ctx.stroke();
                }
                // Handle
                var hRad=(selectedAngle-90)*Math.PI/180;
                var hx=cx+Math.cos(hRad)*R,hy=cy+Math.sin(hRad)*R;
                ctx.beginPath();ctx.arc(hx,hy,handleR,0,2*Math.PI);
                ctx.fillStyle=purple;ctx.fill();
                ctx.shadowColor=purpleGlow;ctx.shadowBlur=12;
                ctx.beginPath();ctx.arc(hx,hy,handleR-3,0,2*Math.PI);
                ctx.fillStyle=bg;ctx.fill();
                ctx.shadowBlur=0;
                ctx.beginPath();ctx.arc(hx,hy,handleR-6,0,2*Math.PI);
                ctx.fillStyle=purple;ctx.fill();
                // Center dot
                ctx.beginPath();ctx.arc(cx,cy,3,0,2*Math.PI);ctx.fillStyle='rgba(255,255,255,0.15)';ctx.fill();
            }

            function getAngleFromEvent(e){
                var rect=canvas.getBoundingClientRect();
                var scaleX=W/rect.width,scaleY=H/rect.height;
                var x,y;
                if(e.touches){x=e.touches[0].clientX-rect.left;y=e.touches[0].clientY-rect.top}
                else{x=e.clientX-rect.left;y=e.clientY-rect.top}
                x*=scaleX;y*=scaleY;
                var ang=Math.atan2(y-cy,x-cx)*180/Math.PI+90;
                // Snap to nearest 5 degrees
                ang=Math.round(ang/5)*5;
                if(ang>180)ang-=360;
                if(ang<-180)ang+=360;
                return ang;
            }

            function onStart(e){e.preventDefault();dragging=true;selectedAngle=getAngleFromEvent(e);angleDisp.textContent=selectedAngle;drawDial()}
            function onMove(e){if(!dragging)return;e.preventDefault();selectedAngle=getAngleFromEvent(e);angleDisp.textContent=selectedAngle;drawDial()}
            function onEnd(){dragging=false}

            canvas.addEventListener('pointerdown',onStart);canvas.addEventListener('pointermove',onMove);
            canvas.addEventListener('pointerup',onEnd);canvas.addEventListener('pointercancel',onEnd);canvas.addEventListener('pointerleave',onEnd);

            goBtn.addEventListener('click',function(){if(selectedAngle!==0)precTurn(selectedAngle)});

            drawDial();
        })();
    })();
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
