(function() {
    "use strict";

    const OLED_WIDTH = 128;
    const OLED_HEIGHT = 62;
    const OLED_BG = "#020605";
    const OLED_FG = "#bfffe9";

    function clamp(value, min, max) {
        return Math.min(max, Math.max(min, value));
    }

    function drawRackOled({
        canvas,
        level,
        masterDist,
        maxMasterDist,
        presetName,
    }) {
        if(!canvas) {
            return;
        }

        const ctx = canvas.getContext("2d");
        const normalizedLevel = clamp(level * 1.35, 0, 1);
        const bars = Math.round(normalizedLevel * 22);

        ctx.imageSmoothingEnabled = false;
        ctx.clearRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
        ctx.fillStyle = OLED_BG;
        ctx.fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
        ctx.fillStyle = OLED_FG;
        ctx.font = "8px monospace";
        ctx.textBaseline = "top";
        ctx.fillText(`P:${presetName}`, 4, 4);
        ctx.fillText("VU", 4, 18);

        ctx.strokeStyle = OLED_FG;
        ctx.strokeRect(22, 17, 96, 9);
        ctx.fillRect(24, 19, bars * 4, 5);

        drawDivider(ctx);
        drawMoodFace(ctx, masterDist, maxMasterDist);
    }

    function drawDivider(ctx) {
        const dividerY = 34;

        for(let x = 0; x < OLED_WIDTH; x += 4) {
            ctx.fillRect(x, dividerY, 1, 1);
        }
    }

    function drawMoodFace(ctx, masterDist, maxMasterDist) {
        const mood = clamp(masterDist / maxMasterDist, 0, 1);
        const faceX = 55;
        const faceY = 46;
        const mouthCurve = Math.round((0.5 - mood) * 5);

        ctx.fillStyle = OLED_FG;
        ctx.strokeStyle = OLED_FG;
        ctx.beginPath();
        ctx.arc(64, 50, 12, 0, Math.PI * 2);
        ctx.stroke();

        ctx.fillRect(faceX, faceY, 2, 2);
        ctx.fillRect(faceX + 16, faceY, 2, 2);

        if(mood > 0.72) {
            ctx.fillRect(faceX - 1, faceY - 2, 4, 1);
            ctx.fillRect(faceX + 15, faceY - 2, 4, 1);
        }

        for(let x = 0; x <= 18; x += 2) {
            const normalized = (x - 9) / 9;
            const y = Math.min(
                OLED_HEIGHT - 1,
                faceY + 6 + Math.round((normalized * normalized - 1) * mouthCurve)
            );

            ctx.fillRect(faceX + x, y, 2, 1);
        }
    }

    window.KickiOled = {
        drawRackOled,
    };
})();
