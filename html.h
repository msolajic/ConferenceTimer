const char HTML_CODE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <!-- Viewport tag optimization ensuring fully responsive rendering scaling layout setups on mobile displays -->
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta charset="utf-8">
    <title>Timer Control Dashboard</title>
    <style>
        /* Theme Configuration Styles - Modern Dark UI Concept design */
        body { font-family: Arial, sans-serif; text-align: center; background-color: #1e1e1e; color: white; padding: 20px; }
        .card { background: #2d2d2d; padding: 20px; border-radius: 10px; max-width: 400px; margin: 0 auto; box-shadow: 0 4px 8px rgba(0,0,0,0.2); }
        
        /* Monospaced large clock layout presentation targeting high legibility across conference rooms */
        .display { font-size: 48px; font-family: 'Courier New', monospace; background: #111; color: #2ecc71; padding: 15px; border-radius: 5px; margin: 20px 0; border: 2px solid #333; letter-spacing: 2px; font-weight: bold; }
        
        /* Dashboard Control Interactive button elements styles presets */
        button { font-size: 18px; padding: 12px 24px; margin: 10px 5px; cursor: pointer; border: none; border-radius: 5px; color: white; font-weight: bold; width: 42%; }
        .btn-start { background-color: #2ecc71; }
        .btn-pause { background-color: #f39c12; }
        .btn-reset { background-color: #e74c3c; width: 88%; }
        
        /* Quick Interval Selector Layout setup matrix grids presets */
        .quickset-container { display: flex; justify-content: space-between; max-width: 88%; margin: 10px auto 20px auto; gap: 5px; }
        .btn-quick { font-size: 14px; padding: 8px 0; margin: 0; background-color: #444; width: 23%; transition: background 0.2s; }
        .btn-quick:hover { background-color: #555; }
        .btn-quick:active { background-color: #2ecc71; }

        /* Lockout design states: Applied preventing interference while the timer countdown sequence is operational */
        .btn-quick:disabled { background-color: #222; color: #555; cursor: not-allowed; opacity: 0.6; }

        input { font-size: 22px; padding: 8px; width: 100px; text-align: center; margin-bottom: 10px; border-radius: 5px; border: 1px solid #555; background: #333; color: white; }
        label { font-size: 16px; color: #bbb; }
        .status { font-size: 12px; color: #777; margin-top: 15px; }
        .notice { font-size: 12px; color: #aaa; margin-top: 15px; }
        
        /* Overrun alert system animation: Flashes the web dashboard display block red if time counts into negatives */
        @keyframes blink { 50% { opacity: 0.3; } }
    </style>
</head>
<body>
    <div class="card">
        <h2>⏱️ Conference Timer Control</h2>
        <hr style="border-color: #444;">
        
        <!-- Live Real-Time Synchronized Clock Display Viewport -->
        <div class="display" id="webDisplay">--:--</div>

        <!-- Quick Set presets interface selectors segment layout -->
        <label>Quick Time setting:</label>
        <div class="quickset-container">
            <button class="btn-quick" id="btn30" onclick="setAndReset(30)">30 min</button>
            <button class="btn-quick" id="btn45" onclick="setAndReset(45)">45 min</button>
            <button class="btn-quick" id="btn60" onclick="setAndReset(60)">60 min</button>
            <button class="btn-quick" id="btn90" onclick="setAndReset(90)">90 min</button>
        </div>

        <!-- Direct Numerical Minute configuration layout components input variables -->
        <label>Manual set (minutes):</label><br>
        <input type="number" id="minutes" value="30" min="1" max="99" oninput="if(this.value > 99) this.value = 99; if(this.value < 1 && this.value !== '') this.value = 1;">
        <br>
        <button class="btn-start" onclick="sendCommand('start')">Start</button>
        <button class="btn-pause" onclick="sendCommand('pause')">Pause</button>
        <button class="btn-reset" onclick="sendCommand('reset')">Reset</button>
        
        <!-- Status indicator updating user regarding connection pipelines status tracking metrics -->
        <div class="status" id="wsStatus">Connecting to timer...</div>
        <div class="notice" id="wsNotice">by Marko Šolajić and ✨Gemini</div>
    </div>
    <script>
        let ws;             // Global reference holder tracking active persistent WebSocket pipeline instance
        let oldTime = "";   // Holds former historical tick value to observe data activity state shifts
        let checkTimer = null; // Thread loop tracker observing continuous execution states intervals

        /**
         * Initializes continuous real-time full-duplex communication infrastructure channel connecting with ESP8266
         */
        function initializeWebSocket() {
            // Establish pipeline communicating dynamically targeting active host IP configuration path via WebSocket port 81
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            
            // Connection successful routine
            ws.onopen = function() {
                document.getElementById('wsStatus').innerText = "Connected with timer";
                document.getElementById('wsStatus').style.color = "#2ecc71";
                
                // Fire evaluation cycle loops mapping data state shifts running every 1.2 seconds
                if(checkTimer) clearInterval(checkTimer);
                checkTimer = setInterval(checkTimerWorking, 1200);
            };
            
            // Fallback recovery route handles sudden pipeline structural termination faults
            ws.onclose = function() {
                document.getElementById('wsStatus').innerText = "Connection lost. Trying again...";
                document.getElementById('wsStatus').style.color = "#e74c3c";
                if(checkTimer) clearInterval(checkTimer);
                manageQuickButtons(false); // Unset input interface protective lockout elements if communication breaks down
                setTimeout(initializeWebSocket, 2000); // Re-fire instantiation retry loops recursively every 2 seconds
            };
            
            // Event monitoring parser handling raw live pipeline packet streams updates pushing down from MCU
            ws.onmessage = function(event) {
                let timeStr = event.data;
                let displayElement = document.getElementById('webDisplay');
                displayElement.innerText = timeStr;
                
                // Identify time resource limit overrun indicators (Look for negative prefix strings)
                if (timeStr.indexOf('-') === 0) {
                    displayElement.style.color = "#ff3b30"; // Apply dangerous red alerting state accent color properties
                    displayElement.style.animation = "blink 1s infinite"; // Trigger active alarming css keyframe flashing styles
                } else {
                    displayElement.style.color = "#2ecc71"; // Re-establish standard stable system operating green colors
                    displayElement.style.animation = "none";
                }
            };
        }

        /**
         * Activity confirmation state checker engine. Monitors string changes over time intervals
         * to automatically enforce protection logic lockouts.
         */
        function checkTimerWorking() {
            let currentTime = document.getElementById('webDisplay').innerText;
            
            // If the time values change sequentially across cycles, it indicates the countdown execution routine IS RUNNING
            if (currentTime !== oldTime && oldTime !== "" && currentTime !== "--:--") {
                manageQuickButtons(true);  // Enforce protection lockout ruleset configurations
            } else {
                manageQuickButtons(false); // Disengage interface lockout protections if stopped or stable
            }
            oldTime = currentTime; // Update cache tracker values to preserve comparison continuity paths
        }

        /**
         * Utility script processing state shifts setting form properties to either lock or unlock user input.
         */
        function manageQuickButtons(lock) {
            document.getElementById('btn30').disabled = lock;
            document.getElementById('btn45').disabled = lock;
            document.getElementById('btn60').disabled = lock;
            document.getElementById('btn90').disabled = lock;
            document.getElementById('minutes').disabled = lock; // Lock out manual integer variables forms to prevent data collision runtime issues
        }

        /**
         * Handles background asynchronous AJAX transmission queries updating runtime operations status.
         */
        function sendCommand(action) {
            let url = 'command?action=' + action;
            if (action === 'start' || action === 'reset') {
                let min = document.getElementById('minutes').value;
                url += '&minutes=' + min;
            }
            
            // Dispatch non-blocking background fetch request packet streams
            fetch(url).then(() => {
                // Proactively adjust button lockout state options instantly to guarantee responsive tactile handling execution feedback
                if (action === 'start') manageQuickButtons(true);
                if (action === 'pause' || action === 'reset') manageQuickButtons(false);
            }).catch(err => console.error('Transmission processing failure error:', err));
        }

        /**
         * Pipeline utility function allowing rapid single-tap parameters overwrite modifications re-instantiations.
         */
        function setAndReset(minutes) {
            document.getElementById('minutes').value = minutes;
            sendCommand('reset');
        }

        // Start initialization routines upon DOM layout generation resolution phases
        window.onload = initializeWebSocket;
    </script>
</body>
</html>
)=====";


// --- ADDITIONAL FULL-SCREEN DISPLAY PAGE FOR OTHER DEVICES ---
const char HTML_FULLSCREEN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta charset="utf-8">
    <title>Fullscreen Timer Display</title>
    <style>
        html, body {
            margin: 0;
            padding: 0;
            width: 100%;
            height: 100%;
            background-color: #111;
            display: flex;
            justify-content: center;
            align-items: center;
            overflow: hidden;
            cursor: pointer; /* Indicates the page is interactive */
        }
        
        /* Fully responsive massive monospaced clock display scaled using viewport width units */
        .fs-display {
            font-family: 'Courier New', monospace;
            font-size: 22vw; 
            color: #2ecc71;
            font-weight: bold;
            letter-spacing: 4px;
            text-align: center;
            user-select: none;
        }

        @keyframes blink { 50% { opacity: 0.3; } }
    </style>
</head>
<body>
    <div class="fs-display" id="fullscreenDisplay">--:--</div>

    <script>
        let ws;

        // Toggle browser native fullscreen mode on double-click anywhere on the page
        document.addEventListener('dblclick', function() {
            if (!document.fullscreenElement) {
                document.documentElement.requestFullscreen().catch(err => {
                    console.error("Error attempting to enable fullscreen:", err.message);
                });
            } else {
                if (document.exitFullscreen) {
                    document.exitFullscreen();
                }
            }
        });

        function initializeWebSocket() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            
            ws.onmessage = function(event) {
                let timeStr = event.data;
                let displayElement = document.getElementById('fullscreenDisplay');
                displayElement.innerText = timeStr;
                
                // Trigger warning states matching the controller logic if negative time is reached
                if (timeStr.indexOf('-') === 0) {
                    displayElement.style.color = "#ff3b30";
                    displayElement.style.animation = "blink 1s infinite";
                } else {
                    displayElement.style.color = "#2ecc71";
                    displayElement.style.animation = "none";
                }
            };
            
            ws.onclose = function() {
                setTimeout(initializeWebSocket, 2000);
            };
        }

        window.onload = initializeWebSocket;
    </script>
</body>
</html>
)=====";