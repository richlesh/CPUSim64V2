/* Lightbox viewer for images and Mermaid diagrams.
 * Activated by right-clicking on an image or mermaid diagram.
 * Supports zoom in/out, scroll, and drag-to-pan.
 */
(function() {
    'use strict';

    var overlay, container, wrapper, toolbar, currentScale, isDragging, startX, startY, scrollLeftStart, scrollTopStart;
    var naturalW, naturalH;
    var MIN_SCALE = 0.1;
    var MAX_SCALE = 5.0;
    var SCALE_STEP = 0.25;
    var WHEEL_STEP = 0.1;

    function createOverlay() {
        if (overlay) return;

        overlay = document.createElement('div');
        overlay.className = 'lightbox-overlay';

        toolbar = document.createElement('div');
        toolbar.className = 'lightbox-toolbar';

        var zoomIn = document.createElement('button');
        zoomIn.innerHTML = '+';
        zoomIn.title = 'Zoom In';
        zoomIn.onclick = function(e) { e.stopPropagation(); zoom(SCALE_STEP); };

        var zoomOut = document.createElement('button');
        zoomOut.innerHTML = '\u2212'; // minus sign
        zoomOut.title = 'Zoom Out';
        zoomOut.onclick = function(e) { e.stopPropagation(); zoom(-SCALE_STEP); };

        var fitBtn = document.createElement('button');
        fitBtn.innerHTML = '\u2922'; // fit/reset icon
        fitBtn.title = 'Fit to Window';
        fitBtn.onclick = function(e) { e.stopPropagation(); fitToWindow(); };

        var closeBtn = document.createElement('button');
        closeBtn.innerHTML = '\u00d7'; // × close
        closeBtn.title = 'Close';
        closeBtn.onclick = function(e) { e.stopPropagation(); closeLightbox(); };

        toolbar.appendChild(zoomIn);
        toolbar.appendChild(zoomOut);
        toolbar.appendChild(fitBtn);
        toolbar.appendChild(closeBtn);

        container = document.createElement('div');
        container.className = 'lightbox-container';

        wrapper = document.createElement('div');
        wrapper.className = 'lightbox-wrapper';
        container.appendChild(wrapper);

        overlay.appendChild(toolbar);
        overlay.appendChild(container);
        document.body.appendChild(overlay);

        // Close on click outside the content
        overlay.addEventListener('click', function(e) {
            if (e.target === overlay) closeLightbox();
        });

        // Close on Escape
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && overlay.classList.contains('active')) {
                closeLightbox();
            }
        });

        // Mouse wheel zoom
        container.addEventListener('wheel', function(e) {
            e.preventDefault();
            var delta = e.deltaY < 0 ? WHEEL_STEP : -WHEEL_STEP;
            zoom(delta);
        }, { passive: false });

        // Drag to pan
        container.addEventListener('mousedown', function(e) {
            if (e.button !== 0) return;
            isDragging = true;
            startX = e.clientX;
            startY = e.clientY;
            scrollLeftStart = container.scrollLeft;
            scrollTopStart = container.scrollTop;
            container.classList.add('grabbing');
            e.preventDefault();
        });

        document.addEventListener('mousemove', function(e) {
            if (!isDragging) return;
            var dx = e.clientX - startX;
            var dy = e.clientY - startY;
            container.scrollLeft = scrollLeftStart - dx;
            container.scrollTop = scrollTopStart - dy;
        });

        document.addEventListener('mouseup', function() {
            if (isDragging) {
                isDragging = false;
                container.classList.remove('grabbing');
            }
        });
    }

    function zoom(delta) {
        var newScale = Math.max(MIN_SCALE, Math.min(MAX_SCALE, currentScale + delta));
        if (newScale === currentScale) return;

        // Remember scroll center before zoom
        var cx = (container.scrollLeft + container.clientWidth / 2) / (naturalW * currentScale);
        var cy = (container.scrollTop + container.clientHeight / 2) / (naturalH * currentScale);

        currentScale = newScale;
        applyScale();

        // Restore scroll center after zoom
        container.scrollLeft = cx * naturalW * currentScale - container.clientWidth / 2;
        container.scrollTop = cy * naturalH * currentScale - container.clientHeight / 2;
    }

    function applyScale() {
        var w = Math.round(naturalW * currentScale);
        var h = Math.round(naturalH * currentScale);
        wrapper.style.width = w + 'px';
        wrapper.style.height = h + 'px';
        wrapper.style.minWidth = w + 'px';
        wrapper.style.minHeight = h + 'px';
        var content = wrapper.querySelector('img, svg');
        if (content) {
            content.style.width = w + 'px';
            content.style.height = h + 'px';
        }
        // Center the wrapper when smaller than the container
        var cw = container.clientWidth;
        var ch = container.clientHeight;
        var padX = Math.max(0, (cw - w) / 2);
        var padY = Math.max(0, (ch - h) / 2);
        wrapper.style.marginLeft = padX + 'px';
        wrapper.style.marginRight = padX + 'px';
        wrapper.style.marginTop = padY + 'px';
        wrapper.style.marginBottom = padY + 'px';
    }

    function fitToWindow() {
        var vw = container.clientWidth;
        var vh = container.clientHeight;
        currentScale = Math.min(vw / naturalW, vh / naturalH, MAX_SCALE);
        applyScale();
        // Center the content
        container.scrollLeft = (container.scrollWidth - container.clientWidth) / 2;
        container.scrollTop = (container.scrollHeight - container.clientHeight) / 2;
    }

    function openLightbox(element) {
        createOverlay();
        wrapper.innerHTML = '';
        currentScale = 1.0;

        if (element.tagName === 'IMG') {
            var img = document.createElement('img');
            img.src = element.src;
            img.alt = element.alt || '';
            wrapper.appendChild(img);

            var doFit = function() {
                naturalW = img.naturalWidth || img.width;
                naturalH = img.naturalHeight || img.height;
                // Delay fitToWindow to next frame so container has layout dimensions
                requestAnimationFrame(function() { fitToWindow(); });
            };

            overlay.classList.add('active');
            if (img.complete && img.naturalWidth > 0) {
                doFit();
            } else {
                img.onload = doFit;
            }
        } else if (element.tagName === 'svg' || (element.closest && element.closest('.mermaid'))) {
            var svg = element.tagName === 'svg' ? element : element.closest('.mermaid').querySelector('svg');
            if (!svg) return;
            var clone = svg.cloneNode(true);
            // Get the natural size from the SVG's rendered size in the document
            var rect = svg.getBoundingClientRect();
            naturalW = rect.width || 600;
            naturalH = rect.height || 400;
            // Also check viewBox for a better natural size
            var vb = svg.getAttribute('viewBox');
            if (vb) {
                var parts = vb.split(/[\s,]+/);
                if (parts.length === 4) {
                    naturalW = parseFloat(parts[2]) || naturalW;
                    naturalH = parseFloat(parts[3]) || naturalH;
                }
            }
            clone.removeAttribute('style');
            clone.style.width = '100%';
            clone.style.height = '100%';
            wrapper.appendChild(clone);
            overlay.classList.add('active');
            // Delay to let layout settle
            setTimeout(function() { fitToWindow(); }, 50);
        }
    }

    function closeLightbox() {
        if (overlay) {
            overlay.classList.remove('active');
            wrapper.innerHTML = '';
        }
    }

    // Intercept right-click on images and mermaid diagrams
    document.addEventListener('contextmenu', function(e) {
        var target = e.target;

        // Check if right-clicking on an image (but not emoji)
        if (target.tagName === 'IMG' && !target.classList.contains('emoji')) {
            e.preventDefault();
            e.stopPropagation();
            openLightbox(target);
            return;
        }

        // Check if right-clicking on a mermaid SVG or element inside one
        var mermaidEl = target.closest('.mermaid');
        if (mermaidEl) {
            var svg = mermaidEl.querySelector('svg');
            if (svg) {
                e.preventDefault();
                e.stopPropagation();
                openLightbox(svg);
                return;
            }
        }

        // Check if right-clicking directly on an SVG inside a mermaid div
        if (target.tagName === 'svg' || target.closest('svg')) {
            var parentMermaid = (target.tagName === 'svg' ? target : target.closest('svg')).closest('.mermaid');
            if (parentMermaid) {
                e.preventDefault();
                e.stopPropagation();
                openLightbox(target.tagName === 'svg' ? target : target.closest('svg'));
                return;
            }
        }
    }, true); // Use capture phase to fire before other contextmenu handlers
})();
