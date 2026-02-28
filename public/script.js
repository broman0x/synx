function animateCounter(elementId, target, duration) {
    var element = document.getElementById(elementId);
    if (!element) return;
    var start = 0;
    var increment = target / (duration / 16);
    var current = start;
    var timer = setInterval(function() {
        current += increment;
        if (current >= target) {
            current = target;
            clearInterval(timer);
        }
        element.textContent = Math.floor(current).toLocaleString();
    }, 16);
}

function startUptimeCounter() {
    var element = document.getElementById('uptime');
    if (!element) return;
    var startTime = Date.now();
    setInterval(function() {
        var elapsed = Date.now() - startTime;
        var hours = Math.floor(elapsed / 3600000);
        var minutes = Math.floor((elapsed % 3600000) / 60000);
        var seconds = Math.floor((elapsed % 60000) / 1000);
        element.textContent = 
            String(hours).padStart(2, '0') + ':' +
            String(minutes).padStart(2, '0') + ':' +
            String(seconds).padStart(2, '0');
    }, 1000);
}

function copyCode() {
    var code = document.getElementById('install-code').textContent;
    navigator.clipboard.writeText(code).then(function() {
        var btn = document.querySelector('.copy-btn');
        btn.classList.add('copied');
        btn.querySelector('span').textContent = 'Disalin!';
        setTimeout(function() {
            btn.classList.remove('copied');
            btn.querySelector('span').textContent = 'Salin';
        }, 2000);
    });
}

function initSmoothScroll() {
    var anchors = document.querySelectorAll('a[href^="#"]');
    anchors.forEach(function(anchor) {
        anchor.addEventListener('click', function(e) {
            var href = this.getAttribute('href');
            if (href !== '#') {
                e.preventDefault();
                var target = document.querySelector(href);
                if (target) {
                    target.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                }
            }
        });
    });
}

document.addEventListener('DOMContentLoaded', function() {
    startUptimeCounter();
    initSmoothScroll();
    animateCounter('requests-count', 231450, 2000);
    animateCounter('connections', 1024, 1500);
    window.copyCode = copyCode;
});
