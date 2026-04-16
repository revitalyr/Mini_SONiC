class MermaidHeightAdjuster {
    constructor() {
        this.scale = 100;
        this.currentDiagramId = null;
    }

    async initialize() {
        await mermaid.initialize({
            startOnLoad: false,
            theme: 'default',
            themeVariables: {
                primaryColor: '#667eea',
                primaryTextColor: '#333',
                lineColor: '#667eea',
                secondaryColor: '#764ba2',
                fontSize: '14px'
            },
            flowchart: { useMaxWidth: false, htmlLabels: true }
        });

        this.setupTabSwitching();
        this.setupScaleControls();

        // Render first tab by default
        const firstTab = document.querySelector('.nav-tab');
        if (firstTab) {
            this.switchTab(firstTab);
        }
    }

    setupTabSwitching() {
        const tabs = document.querySelectorAll('.nav-tab');
        tabs.forEach(tab => {
            tab.addEventListener('click', (e) => {
                this.switchTab(e.target);
            });
        });
    }

    setupScaleControls() {
        // Use event delegation for scale buttons
        document.addEventListener('click', (e) => {
            if (e.target.closest('[data-scale-delta]')) {
                const delta = parseInt(e.target.closest('[data-scale-delta]').dataset.scaleDelta);
                this.changeScale(delta);
            } else if (e.target.closest('[data-reset-scale]')) {
                this.resetScale();
            }
        });
    }

    switchTab(tabElement) {
        // Update active tab
        document.querySelectorAll('.nav-tab').forEach(t => t.classList.remove('active'));
        tabElement.classList.add('active');

        // Show corresponding content
        const tabId = tabElement.dataset.tab;
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.remove('active');
        });
        const targetContent = document.getElementById(tabId);
        if (targetContent) {
            targetContent.classList.add('active');

            // Render Mermaid diagram in this tab
            setTimeout(async () => {
                const diagramElement = targetContent.querySelector('.mermaid');
                if (diagramElement && !diagramElement.hasAttribute('data-processed')) {
                    try {
                        await mermaid.run({
                            nodes: [diagramElement]
                        });
                        diagramElement.setAttribute('data-processed', 'true');
                        this.adjustDiagram(diagramElement);
                        this.applyScale(diagramElement);
                    } catch (error) {
                        console.error('Mermaid rendering error:', error);
                    }
                } else if (diagramElement) {
                    // Diagram already processed, just adjust and apply scale
                    this.adjustDiagram(diagramElement);
                    this.applyScale(diagramElement);
                }
            }, 100);
        }
    }

    adjustDiagram(container) {
        const svg = container.querySelector('svg');
        if (svg) {
            const bbox = svg.getBBox();
            const padding = 40;
            
            // Set SVG dimensions
            svg.setAttribute('width', bbox.width + padding);
            svg.setAttribute('height', bbox.height + padding);
            
            // Ensure container can accommodate the diagram
            container.style.minHeight = (bbox.height + padding + 100) + 'px';
        }
    }

    changeScale(delta) {
        this.scale = Math.max(50, Math.min(200, this.scale + delta));
        this.updateScaleDisplay();
        this.applyCurrentScale();
    }

    resetScale() {
        this.scale = 100;
        this.updateScaleDisplay();
        this.applyCurrentScale();
    }

    updateScaleDisplay() {
        const displays = document.querySelectorAll('.scale-display');
        displays.forEach(display => {
            display.textContent = this.scale + '%';
        });
    }

    applyCurrentScale() {
        const activeTab = document.querySelector('.tab-content.active');
        if (activeTab) {
            const diagramElement = activeTab.querySelector('.mermaid');
            if (diagramElement) {
                this.applyScale(diagramElement);
            }
        }
    }

    applyScale(container) {
        const svg = container.querySelector('svg');
        if (svg) {
            svg.style.transform = `scale(${this.scale / 100})`;
            svg.style.transformOrigin = 'top left';
            svg.style.transition = 'transform 0.2s ease';
        }
    }
}

// Initialize on DOM load
document.addEventListener('DOMContentLoaded', async () => {
    const adjuster = new MermaidHeightAdjuster();
    await adjuster.initialize();
});
