# Mini SONiC Documentation

## Files

- `index.html` - Interactive documentation with Mermaid diagrams
- `mermaid-height-util.js` - Diagram scaling and tab navigation utility
- `README.md` - This file

## Usage

### Viewing Documentation

Open `index.html` directly in a web browser. No build process required.

**Dependency:** Requires internet connection to load Mermaid.js from CDN:
```
https://cdn.jsdelivr.net/npm/mermaid@10.6.1/dist/mermaid.min.js
```

### Features

- Interactive tab navigation
- Mermaid diagram rendering
- Zoom controls (50%-200%)
- Responsive design
- Inline CSS styling

### Documentation Sections

1. Overview - System overview and features
2. Architecture - Component architecture and dependencies
3. Modules - C++20 module structure
4. Data Flow - Packet processing and LPM trie
5. Cross-Platform - Platform support and abstraction
6. Build - Build process and targets

### Diagram Controls

- `-` - Decrease zoom by 10%
- `+` - Increase zoom by 10%
- `⟲` - Reset zoom to 100%

### Browser Compatibility

- Chrome/Edge: Supported
- Firefox: Supported
- Safari: Supported
- Mobile: Responsive design

### Technical Specifications

- Framework: HTML/CSS/JavaScript
- Diagram Library: Mermaid.js 10.6.1
- Styling: Inline CSS
- Icons: Emoji characters

## Modification

### Updating Content

1. Edit `index.html`
2. Add Mermaid diagrams with unique IDs:
   ```html
   <div class="mermaid" id="section-diagram">
   graph TD
       A[Node A] --> B[Node B]
   </div>
   ```
3. Verify in browser

### Adding Tabs

1. Add tab button in `.nav-tabs`:
   ```html
   <button class="nav-tab" data-tab="newsection">🔖 New Section</button>
   ```
2. Add tab content:
   ```html
   <div class="tab-content" id="newsection">
   </div>
   ```
3. Add Mermaid diagram with unique ID if required

### Color Scheme

Defined in `<style>` section of `index.html`:

- Primary gradient: `#667eea` to `#764ba2`
- Success: `#28a745`
- Warning: `#ffc107`
- Error: `#dc3545`
- Neutral: `#6c757d`
