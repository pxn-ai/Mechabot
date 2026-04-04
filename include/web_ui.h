#ifndef WEB_UI_H
#define WEB_UI_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>4WD Car Controller</title>
    <style>
        *, *::before, *::after { margin: 0; padding: 0; box-sizing: border-box; }
        :root {
            --bg: #0a0e1a;
            --surface: rgba(255,255,255,0.04);
            --glass: rgba(255,255,255,0.06);
            --glass-border: rgba(255,255,255,0.1);
            --accent: #00e5ff;
            --accent-glow: rgba(0,229,255,0.3);
            --accent-dim: rgba(0,229,255,0.12);
            --danger: #ff3d71;
            --danger-glow: rgba(255,61,113,0.3);
            --warn: #ffaa00;
            --success: #0cce6b;
            --text: #e8eaf6;
            --text-dim: rgba(232,234,246,0.45);
            --radius: 18px;
        }
        html, body {
            font-family: -apple-system, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg);
            color: var(--text);
            height: 100%;
            overflow: hidden;
        }
        body {
            display: flex;
            flex-direction: column;
            align-items: center;
            -webkit-tap-highlight-color: transparent;
            user-select: none;
        }
        body::before {
            content: '';
            position: fixed; inset: 0;
            background:
                radial-gradient(ellipse 600px 400px at 15% 15%, rgba(0,229,255,0.07), transparent),
                radial-gradient(ellipse 500px 500px at 85% 85%, rgba(123,97,255,0.05), transparent);
            z-index: -1;
            animation: bgShift 15s ease-in-out infinite alternate;
        }
        @keyframes bgShift { to { filter: hue-rotate(25deg); } }
        @keyframes pulse { 0%,100%{ opacity:1; } 50%{ opacity:0.35; } }
        @keyframes fadeIn { from { opacity:0; transform:translateY(8px); } to { opacity:1; transform:translateY(0); } }

        .header {
            width: 100%; padding: 16px 20px 6px; text-align: center;
            animation: fadeIn 0.5s ease-out;
        }
        .header h1 {
            font-size: 1.3rem; font-weight: 900; letter-spacing: -0.5px;
            background: linear-gradient(135deg, var(--accent), #7b61ff);
            -webkit-background-clip: text; background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .status-bar {
            display: flex; justify-content: center; align-items: center;
            gap: 16px; margin-top: 5px; font-size: 0.7rem; color: var(--text-dim);
        }
        .status-dot {
            width: 7px; height: 7px; border-radius: 50%;
            display: inline-block; margin-right: 4px; vertical-align: middle;
            background: var(--success); box-shadow: 0 0 8px var(--success);
            animation: pulse 2s ease-in-out infinite;
        }
        .action-label { font-weight: 600; color: var(--text-dim); transition: color 0.2s; }
        .action-label.moving { color: var(--accent); }

        .card {
            width: min(92%, 380px);
            background: var(--glass);
            backdrop-filter: blur(24px); -webkit-backdrop-filter: blur(24px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius);
            padding: 14px 16px; margin-bottom: 10px;
            animation: fadeIn 0.6s ease-out both;
        }
        .card:nth-child(2) { animation-delay: 0.1s; }
        .card:nth-child(3) { animation-delay: 0.2s; }
        .card:nth-child(4) { animation-delay: 0.3s; }
        .card-title {
            font-size: 0.6rem; font-weight: 700; text-transform: uppercase;
            letter-spacing: 1.4px; color: var(--text-dim); margin-bottom: 8px;
        }



        .speed-section { display: flex; align-items: center; gap: 12px; }
        .speed-val { min-width: 36px; text-align: center; font-size: 1.2rem; font-weight: 900; color: var(--accent); }
        .slider-wrap { flex: 1; }
        input[type="range"] {
            -webkit-appearance: none; appearance: none;
            width: 100%; height: 6px; border-radius: 3px;
            background: var(--accent-dim); outline: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none; width: 24px; height: 24px; border-radius: 50%;
            background: var(--accent); box-shadow: 0 0 12px var(--accent-glow);
            cursor: pointer; border: 3px solid var(--bg);
        }
        .speed-labels { display: flex; justify-content: space-between; font-size: 0.55rem; color: var(--text-dim); margin-top: 2px; }

        .dpad {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            grid-template-rows: 1fr 1fr 1fr;
            gap: 8px; width: 240px; height: 240px; margin: 0 auto;
        }
        .dpad-btn {
            border: none; border-radius: 14px;
            font-family: inherit; font-size: 0.65rem; font-weight: 700;
            letter-spacing: 0.5px; text-transform: uppercase; cursor: pointer;
            display: flex; flex-direction: column; align-items: center;
            justify-content: center; gap: 2px; transition: all 0.12s ease;
            -webkit-tap-highlight-color: transparent; outline: none;
            touch-action: none;
        }
        .dpad-btn svg { width: 24px; height: 24px; fill: currentColor; }
        .btn-dir {
            background: linear-gradient(145deg, rgba(0,229,255,0.12), rgba(0,229,255,0.04));
            border: 1px solid rgba(0,229,255,0.18); color: var(--accent);
        }
        .btn-dir:active, .btn-dir.active {
            background: linear-gradient(145deg, rgba(0,229,255,0.32), rgba(0,229,255,0.12));
            box-shadow: 0 0 22px var(--accent-glow), inset 0 0 10px rgba(0,229,255,0.08);
            transform: scale(0.94);
        }
        .btn-stop {
            background: linear-gradient(145deg, rgba(255,61,113,0.15), rgba(255,61,113,0.04));
            border: 1px solid rgba(255,61,113,0.2); color: var(--danger); border-radius: 50%;
        }
        .btn-stop:active, .btn-stop.active {
            background: linear-gradient(145deg, rgba(255,61,113,0.4), rgba(255,61,113,0.12));
            box-shadow: 0 0 22px var(--danger-glow); transform: scale(0.9);
        }
        .btn-fwd   { grid-column: 2; grid-row: 1; }
        .btn-left  { grid-column: 1; grid-row: 2; }
        .btn-stop  { grid-column: 2; grid-row: 2; }
        .btn-right { grid-column: 3; grid-row: 2; }
        .btn-bwd   { grid-column: 2; grid-row: 3; }

        .footer {
            margin-top: auto; padding: 8px;
            font-size: 0.55rem; color: var(--text-dim); text-align: center;
        }

        @media (max-height: 680px) {
            .dpad { width: 200px; height: 200px; gap: 6px; }
            .card { padding: 10px 12px; margin-bottom: 6px; }
            .header { padding: 10px 16px 4px; }
            .header h1 { font-size: 1.1rem; }
        }
        @media (min-height: 900px) { .dpad { width: 270px; height: 270px; } }
    </style>
</head>
<body>
    <div class="header">
        <h1>&#9889; 4WD Car Controller</h1>
        <div class="status-bar">
            <span><span class="status-dot"></span>WiFi Connected</span>
            <span class="action-label" id="actionLabel">Idle</span>
        </div>
    </div>



    <div class="card">
        <div class="card-title">Speed</div>
        <div class="speed-section">
            <div class="speed-val" id="speedVal">180</div>
            <div class="slider-wrap">
                <input type="range" id="speedSlider" min="80" max="255" value="180">
                <div class="speed-labels"><span>Slow</span><span>Max</span></div>
            </div>
        </div>
    </div>

    <div class="card">
        <div class="card-title">Controls</div>
        <div class="dpad">
            <button class="dpad-btn btn-dir btn-fwd" data-cmd="forward">
                <svg viewBox="0 0 24 24"><path d="M12 4l-8 8h5v8h6v-8h5z"/></svg>FWD
            </button>
            <button class="dpad-btn btn-dir btn-left" data-cmd="left">
                <svg viewBox="0 0 24 24"><path d="M4 12l8-8v5h8v6h-8v5z"/></svg>LEFT
            </button>
            <button class="dpad-btn btn-stop" data-cmd="stop">
                <svg viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="2"/></svg>STOP
            </button>
            <button class="dpad-btn btn-dir btn-right" data-cmd="right">
                <svg viewBox="0 0 24 24"><path d="M20 12l-8-8v5H4v6h8v5z"/></svg>RIGHT
            </button>
            <button class="dpad-btn btn-dir btn-bwd" data-cmd="backward">
                <svg viewBox="0 0 24 24"><path d="M12 20l8-8h-5V4H9v8H4z"/></svg>BWD
            </button>
        </div>
    </div>

    <div class="footer">ESP32-S3 WiFi AP &middot; 192.168.4.1</div>

    <script>
    (function() {
        var speedSlider = document.getElementById('speedSlider');
        var speedVal    = document.getElementById('speedVal');
        var actionLabel = document.getElementById('actionLabel');
        var buttons     = document.querySelectorAll('.dpad-btn');
        var currentSpeed = speedSlider.value;

        speedSlider.addEventListener('input', function() {
            currentSpeed = speedSlider.value;
            speedVal.textContent = currentSpeed;
        });

        function sendCmd(cmd) {
            var url = cmd === 'stop' ? '/stop' : '/' + cmd + '?speed=' + currentSpeed;
            var xhr = new XMLHttpRequest();
            xhr.open('GET', url, true);
            xhr.send();
        }

        function setAction(cmd) {
            if (cmd === 'stop') {
                actionLabel.textContent = 'Stopped';
                actionLabel.classList.remove('moving');
            } else {
                actionLabel.textContent = cmd.charAt(0).toUpperCase() + cmd.slice(1);
                actionLabel.classList.add('moving');
            }
        }

        buttons.forEach(function(btn) {
            var cmd = btn.dataset.cmd;
            var pressed = false;

            function press(e) {
                if (pressed) return;
                pressed = true;
                e.preventDefault();
                btn.classList.add('active');
                sendCmd(cmd);
                setAction(cmd);
            }

            function release(e) {
                if (!pressed) return;
                pressed = false;
                e.preventDefault();
                btn.classList.remove('active');
                if (cmd !== 'stop') {
                    sendCmd('stop');
                    actionLabel.textContent = 'Idle';
                    actionLabel.classList.remove('moving');
                }
            }

            btn.addEventListener('pointerdown', press);
            btn.addEventListener('pointerup', release);
            btn.addEventListener('pointercancel', release);
            btn.addEventListener('pointerleave', function(e) { if (pressed) release(e); });

            btn.addEventListener('click', function(e) {
                e.preventDefault();
                if (!pressed) {
                    sendCmd(cmd);
                    setAction(cmd);
                    btn.classList.add('active');
                    setTimeout(function() {
                        btn.classList.remove('active');
                        if (cmd !== 'stop') {
                            sendCmd('stop');
                            actionLabel.textContent = 'Idle';
                            actionLabel.classList.remove('moving');
                        }
                    }, 200);
                }
            });
        });


    })();
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
