#ifndef WEB_UI_H
#define WEB_UI_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>Mecanum HUD Core</title>
    <style>
        *, *::before, *::after { box-sizing: border-box; }
        :root {
            --bg-void: #03050a;
            --accent-cyan: #00f0ff;
            --accent-violet: #8a2be2;
            --danger-red: #ff2a55;
            --tech-green: #39ff14;
            --glass-bg: rgba(10, 15, 30, 0.4);
            --glass-border: rgba(0, 240, 255, 0.15);
        }
        
        body, html {
            margin: 0; padding: 0; width: 100%; height: 100%;
            background: var(--bg-void); color: #fff;
            font-family: 'SF Pro Display', -apple-system, sans-serif;
            overflow: hidden; touch-action: none; user-select: none; -webkit-user-select: none;
        }

        /* ── Cosmic Background Animation ── */
        body::before {
            content: ''; position: absolute; inset: -50%; z-index: -2;
            background: radial-gradient(circle at 50% 50%, rgba(0,240,255,0.05) 0%, transparent 40%),
                        radial-gradient(circle at 80% 20%, rgba(138,43,226,0.05) 0%, transparent 30%);
            animation: cosmicRotate 30s linear infinite;
        }
        @keyframes cosmicRotate { 100% { transform: rotate(360deg); } }

        /* ── 3-Column Layout ── */
        .layout {
            display: flex; width: 100%; height: 100%; position: relative; z-index: 1;
        }
        
        /* ── Control Zones (Left & Right) ── */
        .control-zone {
            flex: 1; height: 100%; display: flex; align-items: center; justify-content: center;
            position: relative; transition: all 0.3s ease;
        }
        .control-zone::after {
            content: ''; position: absolute; inset: 20px; border: 2px dashed rgba(255,255,255,0.03); 
            border-radius: 30px; pointer-events: none; transition: all 0.3s;
        }
        .control-zone.active::after {
            border-style: solid; border-color: rgba(255,255,255,0.1);
            background: radial-gradient(circle, rgba(255,255,255,0.02) 0%, transparent 70%);
        }
        
        .zone-label {
            font-size: 1.2rem; font-weight: 900; letter-spacing: 6px;
            text-transform: uppercase; writing-mode: vertical-rl;
            color: rgba(255,255,255,0.06); pointer-events: none;
            transition: color 0.3s; text-shadow: 0 0 10px transparent;
        }
        .control-zone.active .zone-label { color: rgba(255,255,255,0.2); text-shadow: 0 0 15px rgba(255,255,255,0.3); }

        .left-zone  { border-right: 1px solid rgba(255,255,255,0.05); }
        .right-zone { border-left: 1px solid rgba(255,255,255,0.05); }

        /* ── Central HUD (The Core) ── */
        .hud-center {
            width: 320px; height: 100%; flex-shrink: 0; display: flex; flex-direction: column;
            align-items: center; padding: 15px; position: relative;
            background: linear-gradient(to right, rgba(0,0,0,0.4), rgba(0,0,0,0.6), rgba(0,0,0,0.4));
            box-shadow: 0 0 40px rgba(0,0,0,0.8);
        }

        /* ── Futuristic Titles ── */
        .hud-title {
            font-size: 0.9rem; font-weight: 900; letter-spacing: 3px; text-transform: uppercase;
            background: linear-gradient(90deg, var(--accent-cyan), var(--accent-violet));
            -webkit-background-clip: text; color: transparent; margin-bottom: 5px;
            animation: textGlow 2s ease-in-out infinite alternate;
        }
        @keyframes textGlow { 
            0% { text-shadow: 0 0 5px rgba(0,240,255,0.1); } 
            100% { text-shadow: 0 0 15px rgba(0,240,255,0.4); } 
        }

        .status-txt { font-size: 0.55rem; font-weight: 700; color: rgba(255,255,255,0.4); letter-spacing: 1px; margin-bottom: 20px; display:flex; align-items:center; gap:5px; }
        .status-dot { width: 6px; height: 6px; border-radius: 50%; background: var(--tech-green); box-shadow: 0 0 10px var(--tech-green); animation: breathTech 2s infinite; }
        @keyframes breathTech { 0%, 100% { opacity: 0.4; } 50% { opacity: 1; } }

        /* ── Breathing HUD Modules ── */
        .hud-module {
            width: 100%; border-radius: 16px; background: var(--glass-bg);
            border: 1px solid var(--glass-border); padding: 15px; margin-bottom: 12px;
            backdrop-filter: blur(10px); position: relative; overflow: hidden;
        }
        /* Breathing neon underside effect */
        .hud-module::before {
            content: ''; position: absolute; bottom: 0; left: 10%; width: 80%; height: 2px;
            background: var(--accent-cyan); box-shadow: 0 0 15px var(--accent-cyan);
            opacity: 0.3; animation: lineBreath 3s infinite alternate;
        }
        @keyframes lineBreath { 0% { opacity: 0.1; box-shadow: 0 0 5px var(--accent-cyan); } 100% { opacity: 0.6; box-shadow: 0 0 20px var(--accent-cyan); } }

        /* ── Compass Target ── */
        .compass-ring {
            width: 70px; height: 70px; margin: 0 auto; border-radius: 50%;
            border: 2px solid rgba(255,255,255,0.1); position: relative;
            background: radial-gradient(circle, rgba(0,240,255,0.1), transparent);
            display: flex; align-items: center; justify-content: center;
        }
        .compass-ring::after { content: ''; position: absolute; inset: -4px; border-radius: 50%; border: 1px dashed var(--accent-cyan); opacity: 0.2; animation: spinRev 20s linear infinite; }
        @keyframes spinRev { 100% { transform: rotate(-360deg); } }

        .compass-needle {
            position: absolute; width: 2px; height: 100%; padding: 4px; pointer-events: none;
            transition: transform 0.15s ease-out;
        }
        .compass-needle::before {
            content:''; display:block; width: 100%; height: 50%; background: var(--danger-red);
            box-shadow: 0 0 10px var(--danger-red); border-radius: 2px;
        }
        
        .heading-display { text-align: center; margin-top: 8px; }
        .heading-val { font-size: 1.5rem; font-weight: 900; color: #fff; text-shadow: 0 0 15px rgba(255,255,255,0.5); font-variant-numeric: tabular-nums; }
        .heading-unit { font-size: 0.7rem; color: var(--accent-cyan); }

        /* ── Switches & Toggles ── */
        .switch-row { display: flex; justify-content: space-between; gap: 10px; margin-top: 5px; }
        .toggle-btn {
            flex: 1; padding: 12px 10px; border-radius: 10px; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.1);
            color: rgba(255,255,255,0.6); font-size: 0.65rem; font-weight: 800; text-transform: uppercase; cursor: pointer;
            transition: all 0.2s; position: relative; overflow: hidden;
        }
        .toggle-btn.active {
            background: rgba(0,240,255,0.1); border-color: var(--accent-cyan); color: #fff;
            box-shadow: 0 0 15px rgba(0,240,255,0.2) inset;
        }
        .toggle-btn.active::after {
            content: ''; position: absolute; top: 0; left: 0; width: 100%; height: 2px; background: var(--accent-cyan); box-shadow: 0 0 10px var(--accent-cyan);
        }
        .toggle-btn:active { transform: scale(0.95); }

        /* ── Core Stop Button ── */
        .btn-stop {
            width: 100%; padding: 18px; border-radius: 12px; background: rgba(255,42,85,0.1); border: 1px solid rgba(255,42,85,0.3);
            color: var(--danger-red); font-size: 1.1rem; font-weight: 900; letter-spacing: 4px; cursor: pointer; text-transform: uppercase;
            box-shadow: 0 0 20px rgba(255,42,85,0.15); transition: all 0.1s; margin-top: auto;
        }
        .btn-stop:active { background: var(--danger-red); color: #fff; box-shadow: 0 0 40px var(--danger-red); transform: scale(0.96); }

        /* ── Thrust Slider (Sci-fi) ── */
        .slider-wrap { margin-top: auto; margin-bottom: 10px; width: 100%; }
        .slider-label { font-size: 0.6rem; color: rgba(255,255,255,0.5); text-transform: uppercase; letter-spacing: 2px; margin-bottom: 8px; display: block; text-align: center; }
        input[type="range"] {
            -webkit-appearance: none; width: 100%; height: 4px; background: rgba(255,255,255,0.1); border-radius: 2px; outline: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none; width: 18px; height: 18px; background: var(--accent-cyan); border-radius: 50%;
            cursor: pointer; box-shadow: 0 0 15px var(--accent-cyan);
        }

        /* ── Virtual Joysticks (The Visual Nodes) ── */
        .stick-base {
            position: absolute; width: 140px; height: 140px; border-radius: 50%;
            border: 2px dashed rgba(255,255,255,0.15); background: radial-gradient(circle, rgba(255,255,255,0.02) 20%, transparent 60%);
            transform: translate(-50%, -50%); display: none; pointer-events: none; z-index: 10;
        }
        .stick-core {
            position: absolute; width: 60px; height: 60px; border-radius: 50%;
            top: 50%; left: 50%; transform: translate(-50%, -50%);
            box-shadow: 0 10px 30px rgba(0,0,0,0.8), inset 0 0 20px rgba(255,255,255,0.2);
        }
        /* Specific glow colors for each side */
        #baseL .stick-core { background: radial-gradient(circle at 30% 30%, #fff, var(--accent-cyan)); box-shadow: 0 0 30px rgba(0,240,255,0.4); }
        #baseR .stick-core { background: radial-gradient(circle at 30% 30%, #fff, var(--accent-violet)); box-shadow: 0 0 30px rgba(138,43,226,0.4); }
        
        .pulse-wave { position: absolute; inset: 0; border-radius: 50%; border: 2px solid var(--accent-cyan); opacity: 0; pointer-events: none; }
        @keyframes emitPulse { 0% { transform: scale(1); opacity: 0.8; } 100% { transform: scale(2.5); opacity: 0; } }
        .emitting .pulse-wave { animation: emitPulse 1s ease-out infinite; }
        
        /* Interactive Animation trigger on HUD */
        body.driving .hud-center { box-shadow: 0 0 60px rgba(0,240,255,0.15); }
        body.driving .hud-module { border-color: rgba(0,240,255,0.3); }

        @media (max-width: 700px) {
            .hud-center { width: 260px; padding: 10px; }
            .compass-ring { width: 50px; height: 50px; }
            .btn-stop { padding: 14px; }
        }
    </style>
</head>
<body>

    <div class="layout">
        <!-- LEFT CONTROL: TRANSLATE -->
        <div class="control-zone left-zone" id="zoneL">
            <div class="zone-label">Translate</div>
        </div>

        <!-- CENTER HUD: COMMAND CORE -->
        <div class="hud-center">
            <div class="hud-title">Vector Core</div>
            <div class="status-txt"><span class="status-dot"></span>SYS_ONLINE // <span id="statusLbl">IDLE</span></div>

            <!-- Compass Module -->
            <div class="hud-module">
                <div class="compass-ring"><div class="compass-needle" id="needle"></div></div>
                <div class="heading-display">
                    <span class="heading-val" id="headingVal">0</span><span class="heading-unit">&deg;</span>
                </div>
            </div>

            <!-- Assist Modules -->
            <div class="hud-module" style="padding: 10px;">
                <div class="switch-row">
                    <button class="toggle-btn active" id="btnCompass" onclick="toggleCompass()">PA-Lock</button>
                    <button class="toggle-btn" id="btnCruise" onclick="toggleCruise()">Cruise</button>
                </div>
            </div>

            <!-- Thrust Limiter -->
            <div class="slider-wrap">
                <span class="slider-label">Reactor Output</span>
                <input type="range" id="speedSlider" min="80" max="255" value="255">
            </div>

            <button class="btn-stop" onclick="emergencyStop()">E-STOP</button>
        </div>

        <!-- RIGHT CONTROL: YAW -->
        <div class="control-zone right-zone" id="zoneR">
            <div class="zone-label">Rotation</div>
        </div>
    </div>

    <!-- The physical joysticks that spawn on touch -->
    <div class="stick-base" id="baseL"><div class="pulse-wave"></div><div class="stick-core" id="nubL"></div></div>
    <div class="stick-base" id="baseR"><div class="pulse-wave"></div><div class="stick-core" id="nubR"></div></div>

    <script>
        const J_RADIUS = 60; 
        let vx = 0, vy = 0, wz = 0;
        let cruiseMode = false;
        let activePointers = {}; 

        // Elements
        const baseL = document.getElementById('baseL'), nubL = document.getElementById('nubL');
        const baseR = document.getElementById('baseR'), nubR = document.getElementById('nubR');
        const zoneL = document.getElementById('zoneL'), zoneR = document.getElementById('zoneR');
        const speedSel = document.getElementById('speedSlider');
        const statusLbl = document.getElementById('statusLbl');
        let lastSentParams = "";

        // Throttled Comm Link
        setInterval(() => {
            if (vx===0 && vy===0 && wz===0 && lastSentParams==="stop") {
                document.body.classList.remove('driving');
                return;
            }
            let thrust = speedSel.value;
            let params = `vx=${vx.toFixed(2)}&vy=${vy.toFixed(2)}&wz=${wz.toFixed(2)}&speed=${thrust}`;
            
            if (params !== lastSentParams) {
                fetch('/vector?' + params).catch(()=>{});
                lastSentParams = (vx===0 && vy===0 && wz===0) ? "stop" : params;
                
                if(lastSentParams === "stop") {
                    statusLbl.textContent = "IDLE";
                    document.body.classList.remove('driving');
                } else {
                    statusLbl.textContent = "VECTORING";
                    document.body.classList.add('driving');
                }
            }
        }, 50);

        function EmergencyStop() {
            vx = 0; vy = 0; wz = 0;
            cruiseMode = false;
            document.getElementById('btnCruise').classList.remove('active');
            fetch('/stop').catch(()=>{});
            
            // visual reset
            endStickVisually(baseL, nubL, zoneL);
            endStickVisually(baseR, nubR, zoneR);
        }
        window.emergencyStop = EmergencyStop;

        window.toggleCruise = () => {
            cruiseMode = !cruiseMode;
            document.getElementById('btnCruise').classList.toggle('active', cruiseMode);
        };
        window.toggleCompass = () => {
            fetch('/compass_toggle').then(r=>r.json()).then(d=>{
                document.getElementById('btnCompass').classList.toggle('active', d.compass);
            }).catch(()=>{});
        };

        function startStickVisually(base, zone, x, y) {
            base.style.display = 'block';
            base.style.left = x + 'px';
            base.style.top = y + 'px';
            nubL.style.transition = 'none'; // removing snapping transition during drag
            nubR.style.transition = 'none';
            zone.classList.add('active');
            base.classList.add('emitting');
        }

        function endStickVisually(base, nub, zone) {
            base.classList.remove('emitting');
            base.style.display = 'none';
            nub.style.transition = 'transform 0.2s cubic-bezier(0.34, 1.56, 0.64, 1)';
            nub.style.transform = `translate(-50%, -50%)`;
            zone.classList.remove('active');
        }

        // Multitouch Handling
        document.body.addEventListener('pointerdown', (e) => {
            if (e.target.tagName === 'BUTTON' || e.target.tagName === 'INPUT') return;
            // Determine left or right zone based on screen click
            let isLeft = e.clientX < window.innerWidth / 2;
            let base = isLeft ? baseL : baseR;
            let zone = isLeft ? zoneL : zoneR;
            
            startStickVisually(base, zone, e.clientX, e.clientY);

            activePointers[e.pointerId] = {
                left: isLeft, sx: e.clientX, sy: e.clientY
            };
            e.target.setPointerCapture(e.pointerId);
        });

        document.body.addEventListener('pointermove', (e) => {
            let p = activePointers[e.pointerId];
            if (!p) return;

            let dx = e.clientX - p.sx;
            let dy = e.clientY - p.sy;
            
            let dist = Math.sqrt(dx*dx + dy*dy);
            if (dist > J_RADIUS) {
                dx = (dx / dist) * J_RADIUS;
                dy = (dy / dist) * J_RADIUS;
            }

            let nub = p.left ? nubL : nubR;
            nub.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;

            let nx = dx / J_RADIUS;
            let ny = -dy / J_RADIUS; 

            if (p.left) {
                vx = nx; vy = ny;
            } else {
                wz = nx; // Yaw uses X-axis slide on right screen
            }
        });

        function pointerEnd(e) {
            let p = activePointers[e.pointerId];
            if (!p) return;

            let base = p.left ? baseL : baseR;
            let nub = p.left ? nubL : nubR;
            let zone = p.left ? zoneL : zoneR;

            endStickVisually(base, nub, zone);

            if (!cruiseMode || !p.left) { // Cruise mode only holds translation, not rotation
                if (p.left) { vx = 0; vy = 0; }
                else { wz = 0; }
            }
            
            delete activePointers[e.pointerId];
        }

        document.body.addEventListener('pointerup', pointerEnd);
        document.body.addEventListener('pointercancel', pointerEnd);

        // Telemetry Polling (Compass HUD)
        setInterval(() => {
            fetch('/heading').then(r=>r.json()).then(d=>{
                if(d.heading >= 0) {
                    document.getElementById('headingVal').textContent = d.heading.toFixed(0);
                    // Add rotational transition to needle instead of instant snap
                    document.getElementById('needle').style.transform = `rotate(${d.heading}deg)`;
                }
            }).catch(()=>{});
        }, 150);
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
