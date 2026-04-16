# Mini SONiC Documentation

This directory contains the interactive technical documentation for the Mini SONiC project.

## Files

- **index.html** - Main interactive documentation with Mermaid diagrams
- **mermaid-height-util.js** - JavaScript utility for diagram scaling and tab navigation
- **README.md** - This file

## Usage

### Opening the Documentation

The documentation can be opened directly in a web browser:

```bash
# Open directly in browser
open docs/index.html

# Or navigate to the file and double-click
```

**Note:** The documentation requires an internet connection to load Mermaid.js from CDN:
```
https://cdn.jsdelivr.net/npm/mermaid@10.6.1/dist/mermaid.min.js
```

### Features

- **Interactive Tabs:** Navigate between different documentation sections
- **Mermaid Diagrams:** Visual architecture and flow diagrams
- **Zoom Controls:** Scale diagrams between 50% and 200%
- **Responsive Design:** Works on desktop and mobile devices
- **Gradient UI:** Modern gradient background and card design

### Documentation Sections

1. **📊 Overview** - High-level system overview and key features
2. **🏗️ Architecture** - Component architecture and module dependencies
3. **🧩 Modules** - C++20 module structure and descriptions
4. **🔄 Data Flow** - Packet processing flow and LPM trie implementation
5. **💻 Cross-Platform** - Platform support and abstraction layer
6. **🚀 Build** - Build process and targets

### Diagram Controls

Each diagram has zoom controls in the top-right corner:
- **−** - Decrease zoom by 10%
- **+** - Increase zoom by 10%
- **⟲** - Reset zoom to 100%

### Browser Compatibility

- Chrome/Edge: Full support
- Firefox: Full support
- Safari: Full support
- Mobile browsers: Responsive design supported

### Technical Details

- **Framework:** Pure HTML/CSS/JavaScript (no external frameworks)
- **Diagram Library:** Mermaid.js 10.6.1
- **Styling:** Inline CSS (no external stylesheets)
- **Icons:** Emoji characters for tab navigation

## Development

### Modifying the Documentation

To update the documentation:

1. Edit `index.html` to modify content or add new sections
2. Add new Mermaid diagrams using the syntax:
   ```html
   <div class="mermaid" id="section-diagram">
   graph TD
       A[Node A] --> B[Node B]
   </div>
   ```
3. Ensure new diagrams have unique IDs
4. Test changes by opening `index.html` in a browser

### Adding New Tabs

1. Add a new tab button in the `.nav-tabs` section:
   ```html
   <button class="nav-tab" data-tab="newsection">🔖 New Section</button>
   ```
2. Add corresponding tab content:
   ```html
   <div class="tab-content" id="newsection">
       <!-- Content here -->
   </div>
   ```
3. Add a Mermaid diagram if needed with a unique ID

### Customizing Colors

Color scheme is defined in the `<style>` section of `index.html`:

- Primary gradient: `#667eea` to `#764ba2`
- Success color: `#28a745`
- Warning color: `#ffc107`
- Error color: `#dc3545`
- Neutral color: `#6c757d`

## License

This documentation is part of the Mini SONiC project.
