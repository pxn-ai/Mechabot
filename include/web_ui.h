#ifndef WEB_UI_H
#define WEB_UI_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>Omni-Directional Controller</title>
    <style>
        *, *::before, *::after { box-sizing: border-box; }
        :root {
            --bg: #060913; --surface: rgba(255,255,255,0.05);
            --accent: #00e5ff; --purple: #7b61ff; --danger: #ff3d71; --success: #0cce6b; --text: #e8eaf6;
        }
        body, html { 
            margin: 0; padding: 0; width: 100%; height: 100%; 
            background: var(--bg); color: var(--text); 
            font-family: -apple-system, sans-serif;
            overflow: hidden; touch-action: none; /* Block native scrolling */
            user-select: none; -webkit-user-select: none;
        }

        /* ── Header & Status ── */
        .top-bar { position: absolute; top: 0; left: 0; width: 100%; padding: 12px; display: flex; justify-content: space-between; align-items: flex-start; z-index: 10; pointer-events: none; }
        .logo { font-size: 1.1rem; font-weight: 900; background: linear-gradient(135deg, var(--accent), var(--purple)); -webkit-background-clip: text; color: transparent; }
        
        .hud-panel { background: rgba(0,0,0,0.5); backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.1); border-radius: 12px; padding: 10px; pointer-events: auto; }
        
        /* ── Compact Compass ── */
        .compass-wrap { display: flex; align-items: center; gap: 10px; }
        .compass-ring { width: 50px; height: 50px; border-radius: 50%; border: 2px solid rgba(255,255,255,0.1); position: relative; }
        .compass-needle { position: absolute; top: 50%; left: 50%; width: 2px; height: 22px; margin-left: -1px; margin-top: -16px; background: linear-gradient(to bottom, var(--danger) 50%, #fff 50%); transform-origin: 50% 72.7%; transition: transform 0.2s; border-radius: 2px; }
        .compass-data { text-align: right; }
        .heading-val { font-size: 1.4rem; font-weight: 900; color: var(--accent); line-height: 1; }
        .heading-lbl { font-size: 0.6rem; color: rgba(255,255,255,0.5); text-transform: uppercase; font-weight: 700; }

        /* ── Action Buttons ── */
        .action-row { display: flex; gap: 8px; margin-top: 8px; flex-wrap: wrap; justify-content: flex-end; }
        .btn { padding: 6px 12px; border-radius: 8px; font-size: 0.65rem; font-weight: 800; border: 1px solid rgba(255,255,255,0.2); background: var(--surface); color: var(--text); cursor: pointer; text-transform: uppercase; touch-action: auto; outline: none; transition: transform 0.1s; }
        .btn:active { transform: scale(0.9); }
        .btn.active { background: rgba(0,229,255,0.2); border-color: var(--accent); color: var(--accent); }
        .btn-stop { background: rgba(255,61,113,0.15); border-color: rgba(255,61,113,0.3); color: var(--danger); font-size: 0.9rem; padding: 12px 24px; box-shadow: 0 0 15px rgba(255,61,113,0.2); width: 100%; margin-top: 10px; }
        .btn-stop:active { background: var(--danger); color: #fff; transform: scale(0.95); }

        /* ── Speed ── */
        .speed-slider { width: 100%; margin-top: 10px; }
        .speed-slider input { width: 100%; }

        /* ── Joysticks Area ── */
        .stick-zone { position: absolute; top: 0; bottom: 0; width: 50%; z-index: 1; display: flex; flex-direction: column; justify-content: center; align-items: center; color: rgba(255,255,255,0.1); font-weight: 900; font-size: 1.5rem; text-transform: uppercase; letter-spacing: 2px; }
        .stick-zone.left { left: 0; border-right: 1px dashed rgba(255,255,255,0.05); }
        .stick-zone.right { right: 0; }
        
        .joystick-base { position: absolute; width: 120px; height: 120px; border-radius: 50%; background: rgba(255,255,255,0.05); border: 2px solid rgba(255,255,255,0.1); box-shadow: 0 0 20px rgba(0,0,0,0.5); transform: translate(-50%, -50%); display: none; pointer-events: none; }
        .joystick-nub { position: absolute; width: 50px; height: 50px; border-radius: 50%; background: linear-gradient(135deg, rgba(255,255,255,0.8), rgba(255,255,255,0.4)); box-shadow: 0 5px 15px rgba(0,0,0,0.5); top: 50%; left: 50%; transform: translate(-50%, -50%); }
        .stick-zone.left .joystick-nub { background: linear-gradient(135deg, var(--accent), rgba(0,229,255,0.5)); }
        .stick-zone.right .joystick-nub { background: linear-gradient(135deg, var(--purple), rgba(123,97,255,0.5)); }

    </style>
</head>
<body>

    <!-- Overlay UI -->
    <div class="top-bar">
        <div class="logo">&#9889; MECANUM<br><span style="font-size:0.6rem;color:rgba(255,255,255,0.5);font-weight:600">Dual Stick Vector Drive</span></div>
        
        <div class="hud-panel">
            <div class="compass-wrap">
                <div class="compass-ring"><div class="compass-needle" id="needle"></div></div>
                <div class="compass-data">
                    <div class="heading-val"><span id="headingVal">0</span>&deg;</div>
                    <div class="heading-lbl" id="statusLbl">IDLE</div>
                </div>
            </div>
            
            <div class="action-row">
                <button class="btn active" id="btnCompass" onclick="toggleCompass()">P-Assist</button>
                <button class="btn" id="btnCruise" onclick="toggleCruise()">Cruise</button>
            </div>
            
            <div class="speed-slider">
                <span style="font-size:0.5rem;color:rgba(255,255,255,0.5);text-transform:uppercase">Thrust Limit</span>
                <input type="range" id="speedSlider" min="80" max="255" value="255">
            </div>

            <button class="btn btn-stop" onclick="emergencyStop()">STOP</button>
        </div>
    </div>

    <!-- Touch Zones -->
    <div class="stick-zone left" id="zoneL">TRANSLATE<br><span style="font-size:0.6rem">(Move)</span></div>
    <div class="stick-zone right" id="zoneR">YAW<br><span style="font-size:0.6rem">(Rotate)</span></div>

    <!-- Joystick Visuals -->
    <div class="joystick-base" id="baseL"><div class="joystick-nub" id="nubL"></div></div>
    <div class="joystick-base" id="baseR"><div class="joystick-nub" id="nubR"></div></div>

    <script>
        const J_RADIUS = 60; // Max drag distance
        let vx = 0, vy = 0, wz = 0;
        let cruiseMode = false;
        let activePointers = {}; // pointerId -> {zone, startX, startY}

        // DOM elements
        const baseL = document.getElementById('baseL'), nubL = document.getElementById('nubL');
        const baseR = document.getElementById('baseR'), nubR = document.getElementById('nubR');
        const speedSel = document.getElementById('speedSlider');
        const statusLbl = document.getElementById('statusLbl');
        let lastSentParams = "";

        // API Throttle
        setInterval(() => {
            if (vx===0 && vy===0 && wz===0 && lastSentParams==="stop") return;
            let thrust = speedSel.value;
            let params = `vx=${vx.toFixed(2)}&vy=${vy.toFixed(2)}&wz=${wz.toFixed(2)}&speed=${thrust}`;
            if (params !== lastSentParams) {
                fetch('/vector?' + params).catch(()=>{});
                lastSentParams = (vx===0 && vy===0 && wz===0) ? "stop" : params;
                statusLbl.textContent = lastSentParams === "stop" ? "IDLE" : "DRIVING";
            }
        }, 50);

        function EmergencyStop() {
            vx = 0; vy = 0; wz = 0;
            cruiseMode = false;
            document.getElementById('btnCruise').classList.remove('active');
            fetch('/stop').catch(()=>{});
            nubL.style.transform = `translate(-50%, -50%)`;
            nubR.style.transform = `translate(-50%, -50%)`;
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

        // Pointer Events for Joysticks
        document.body.addEventListener('pointerdown', (e) => {
            if (e.target.tagName === 'BUTTON' || e.target.tagName === 'INPUT') return;
            let isLeft = e.clientX < window.innerWidth / 2;
            let base = isLeft ? baseL : baseR;
            
            // Show base
            base.style.display = 'block';
            base.style.left = e.clientX + 'px';
            base.style.top = e.clientY + 'px';

            activePointers[e.pointerId] = {
                left: isLeft,
                sx: e.clientX, sy: e.clientY
            };
            
            // Set capture so we track outside elements
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

            // Calculate normalized -1.0 to 1.0 (inverted Y so UP is positive)
            let nx = dx / J_RADIUS;
            let ny = -dy / J_RADIUS; 

            if (p.left) {
                vx = nx; vy = ny;
            } else {
                wz = nx; // Right stick only governs Z-rotation via X-axis
            }
        });

        function pointerEnd(e) {
            let p = activePointers[e.pointerId];
            if (!p) return;

            let base = p.left ? baseL : baseR;
            let nub = p.left ? nubL : nubR;

            base.style.display = 'none';
            nub.style.transform = `translate(-50%, -50%)`;

            if (!cruiseMode || p.left === false) {
                if (p.left) { vx = 0; vy = 0; }
                else { wz = 0; }
            }
            
            delete activePointers[e.pointerId];
        }

        document.body.addEventListener('pointerup', pointerEnd);
        document.body.addEventListener('pointercancel', pointerEnd);

        // Compass updater
        setInterval(() => {
            fetch('/heading').then(r=>r.json()).then(d=>{
                if(d.heading >= 0) {
                    document.getElementById('headingVal').textContent = d.heading.toFixed(0);
                    document.getElementById('needle').style.transform = `rotate(${d.heading}deg)`;
                }
            }).catch(()=>{});
        }, 200);

    </script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
